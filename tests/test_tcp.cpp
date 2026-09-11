#include <doctest/doctest.h>
#include <asio.hpp>
#include "server/TCP.hpp"
#include "server/Declares.hpp"
#include "server/SecurityIp.hpp"

TEST_SUITE("TCP_G1_UtilidadesIP") {

TEST_CASE("TCP - G1: Validación sintáctica de IPs (IsValidIP)") {
    SUBCASE("Direcciones IPv4 sintácticamente válidas") {
        CHECK(TCP::IsValidIP("127.0.0.1"));
        CHECK(TCP::IsValidIP("0.0.0.0"));
        CHECK(TCP::IsValidIP("255.255.255.255"));
        CHECK(TCP::IsValidIP("192.168.1.1"));
        CHECK(TCP::IsValidIP("10.0.0.1"));
        CHECK(TCP::IsValidIP("8.8.8.8"));
        CHECK(TCP::IsValidIP("172.16.254.1"));
    }

    SUBCASE("Octetos fuera del rango [0, 255]") {
        CHECK_FALSE(TCP::IsValidIP("256.1.1.1"));
        CHECK_FALSE(TCP::IsValidIP("1.256.1.1"));
        CHECK_FALSE(TCP::IsValidIP("1.1.256.1"));
        CHECK_FALSE(TCP::IsValidIP("1.1.1.256"));
        CHECK_FALSE(TCP::IsValidIP("999.999.999.999"));
        CHECK_FALSE(TCP::IsValidIP("300.0.0.1"));
    }

    SUBCASE("Caracteres no numéricos y signos espurios") {
        CHECK_FALSE(TCP::IsValidIP("127.0.0.a"));
        CHECK_FALSE(TCP::IsValidIP("a.b.c.d"));
        CHECK_FALSE(TCP::IsValidIP("127.0.0.1x"));
        CHECK_FALSE(TCP::IsValidIP("127.0.-1.1"));
        CHECK_FALSE(TCP::IsValidIP("127.0.+1.1"));
        CHECK_FALSE(TCP::IsValidIP("127.0.0.1#"));
    }

    SUBCASE("Cantidad incorrecta de componentes (distinto de 4 octetos)") {
        CHECK_FALSE(TCP::IsValidIP("127.0.0"));
        CHECK_FALSE(TCP::IsValidIP("127.0.1"));
        CHECK_FALSE(TCP::IsValidIP("127.1"));
        CHECK_FALSE(TCP::IsValidIP("127"));
        CHECK_FALSE(TCP::IsValidIP("1.2.3.4.5"));
        CHECK_FALSE(TCP::IsValidIP("1.2.3.4.5.6"));
    }

    SUBCASE("Cadenas vacías, espacios y puntos consecutivos") {
        CHECK_FALSE(TCP::IsValidIP(""));
        CHECK_FALSE(TCP::IsValidIP("   "));
        CHECK_FALSE(TCP::IsValidIP(" 127.0.0.1"));
        CHECK_FALSE(TCP::IsValidIP("127.0.0.1 "));
        CHECK_FALSE(TCP::IsValidIP("127. 0.0.1"));
        CHECK_FALSE(TCP::IsValidIP("..."));
        CHECK_FALSE(TCP::IsValidIP("127..0.1"));
        CHECK_FALSE(TCP::IsValidIP(".127.0.0.1"));
        CHECK_FALSE(TCP::IsValidIP("127.0.0.1."));
    }
}

TEST_CASE("TCP - G1: Conversión de string a entero en network byte order (GetLongIp)") {
    SUBCASE("Direcciones válidas retornan entero representativo") {
        CHECK(TCP::GetLongIp("127.0.0.1") != 0);
        CHECK(TCP::GetLongIp("192.168.1.1") != 0);
        CHECK(TCP::GetLongIp("10.0.0.1") != 0);
        CHECK(TCP::GetLongIp("255.255.255.255") == 0xFFFFFFFFU);
        CHECK(TCP::GetLongIp("0.0.0.0") == 0);
    }

    SUBCASE("Cadenas inválidas retornan 0") {
        CHECK(TCP::GetLongIp("256.1.1.1") == 0);
        CHECK(TCP::GetLongIp("127.0.0.a") == 0);
        CHECK(TCP::GetLongIp("127.0.1") == 0);
        CHECK(TCP::GetLongIp("1.2.3.4.5") == 0);
        CHECK(TCP::GetLongIp("") == 0);
        CHECK(TCP::GetLongIp("   ") == 0);
        CHECK(TCP::GetLongIp(" 127.0.0.1") == 0);
        CHECK(TCP::GetLongIp("127.0.0.1 ") == 0);
        CHECK(TCP::GetLongIp("invalid_host") == 0);
    }
}

TEST_CASE("TCP - G1: Conversión de entero a string punteado (GetAscIP)") {
    SUBCASE("Valores de borde específicos") {
        CHECK(TCP::GetAscIP(0xFFFFFFFFU) == "255.255.255.255");
        CHECK(TCP::GetAscIP(0) == "0.0.0.0");
    }
}

TEST_CASE("TCP - G1: Paridad y consistencia bidireccional (Round-Trip)") {
    SUBCASE("Conversión round-trip GetAscIP(GetLongIp(ip)) == ip") {
        CHECK(TCP::GetAscIP(TCP::GetLongIp("127.0.0.1")) == "127.0.0.1");
        CHECK(TCP::GetAscIP(TCP::GetLongIp("10.0.0.1")) == "10.0.0.1");
        CHECK(TCP::GetAscIP(TCP::GetLongIp("192.168.1.1")) == "192.168.1.1");
        CHECK(TCP::GetAscIP(TCP::GetLongIp("8.8.8.8")) == "8.8.8.8");
        CHECK(TCP::GetAscIP(TCP::GetLongIp("172.16.254.1")) == "172.16.254.1");
        CHECK(TCP::GetAscIP(TCP::GetLongIp("255.255.255.255")) == "255.255.255.255");
        CHECK(TCP::GetAscIP(TCP::GetLongIp("0.0.0.0")) == "0.0.0.0");
    }
}

} // TEST_SUITE

namespace {

struct UserListTestFixture {
    std::int16_t prev_max_users{MaxUsers};
    std::size_t prev_size{UserList.size()};

    UserListTestFixture(std::int16_t users) {
        MaxUsers = users;
        UserList.clear();
        UserList.resize(MaxUsers + 1);
        for (std::int16_t i = 1; i <= MaxUsers; ++i) {
            UserList[i].ConnID = -1;
            UserList[i].ConnIDValida = false;
            UserList[i].flags.UserLogged = false;
        }
    }

    ~UserListTestFixture() {
        MaxUsers = prev_max_users;
        UserList.clear();
        UserList.resize(prev_size);
    }
};

} // namespace

TEST_SUITE("TCP_G2_GestionSlots") {

TEST_CASE("TCP - G2: Búsqueda de slot libre (NextOpenUser)") {
    UserListTestFixture fix(5);

    SUBCASE("Tabla vacía: asigna slot 1") {
        CHECK(TCP::NextOpenUser() == 1);
    }

    SUBCASE("Asignación continua y huecos intermedios") {
        // Ocupamos slot 1
        UserList[1].ConnID = 101;
        UserList[1].flags.UserLogged = true;
        CHECK(TCP::NextOpenUser() == 2);

        // Ocupamos slot 3 (dejando slot 2 libre)
        UserList[3].ConnID = 103;
        UserList[3].flags.UserLogged = true;
        CHECK(TCP::NextOpenUser() == 2);

        // Ocupamos slot 2
        UserList[2].ConnID = 102;
        UserList[2].flags.UserLogged = true;
        CHECK(TCP::NextOpenUser() == 4);
    }

    SUBCASE("Caso Crítico Anti-CombatLog: slot desconectado pero retenido en mapa no debe reasignarse") {
        // El usuario del slot 1 se desconectó a nivel socket (ConnID = -1), pero
        // su personaje permanece en el mapa retenido por temporizador de combate (UserLogged = true).
        UserList[1].ConnID = -1;
        UserList[1].ConnIDValida = false;
        UserList[1].flags.UserLogged = true;

        // NextOpenUser no debe tomar el slot 1 porque causaría sobrescritura y clone de personaje
        CHECK(TCP::NextOpenUser() == 2);

        // Si también el slot 2 está en estado de retención combate
        UserList[2].ConnID = -1;
        UserList[2].ConnIDValida = false;
        UserList[2].flags.UserLogged = true;
        CHECK(TCP::NextOpenUser() == 3);
    }

    SUBCASE("Servidor lleno: todos ocupados retorna MaxUsers + 1") {
        for (std::int16_t i = 1; i <= 5; ++i) {
            UserList[i].ConnID = 100 + i;
            UserList[i].flags.UserLogged = true;
        }
        CHECK(TCP::NextOpenUser() == 6);
    }
}

TEST_CASE("TCP - G2: Blanqueo y reseteo de slot en memoria (ResetUserSlot)") {
    UserListTestFixture fix(3);

    // Configuramos un usuario activo con datos de red y estado
    UserList[1].ConnID = 456;
    UserList[1].ConnIDValida = true;
    UserList[1].flags.UserLogged = true;
    UserList[1].Counters.Salir = 10;
    UserList[1].Counters.Saliendo = true;
    UserList[1].Counters.IdleCount = 99;
    UserList[1].ip = "192.168.1.50";

    // Ejecutamos ResetUserSlot
    TCP::ResetUserSlot(1);

    CHECK(UserList[1].ConnID == -1);
    CHECK_FALSE(UserList[1].ConnIDValida);
    CHECK_FALSE(UserList[1].flags.UserLogged);
    CHECK(UserList[1].Counters.Salir == 0);
    CHECK_FALSE(UserList[1].Counters.Saliendo);
    CHECK(UserList[1].Counters.IdleCount == 0);
    CHECK(UserList[1].ip.empty());

    // Verificamos seguridad de límites con índices inválidos (no debe corromper memoria)
    TCP::ResetUserSlot(0);
    TCP::ResetUserSlot(-1);
    TCP::ResetUserSlot(999);
}

TEST_CASE("TCP - G2: Mapeo asociativo de sockets (AgregaSlotSock / BuscaSlotSock / BorraSlotSock)") {
    TCP::ResetSockSlots();

    SUBCASE("Búsqueda en mapa vacío de socket inexistente") {
        CHECK(TCP::BuscaSlotSock(12345) == -1);
    }

    SUBCASE("Inserción y consulta correcta de asociaciones socket -> slot") {
        TCP::AgregaSlotSock(1001, 1);
        TCP::AgregaSlotSock(1002, 2);
        TCP::AgregaSlotSock(2005, 5);

        CHECK(TCP::BuscaSlotSock(1001) == 1);
        CHECK(TCP::BuscaSlotSock(1002) == 2);
        CHECK(TCP::BuscaSlotSock(2005) == 5);
        CHECK(TCP::BuscaSlotSock(9999) == -1);
    }

    SUBCASE("Sobrescritura o reasignación de socket existente") {
        TCP::AgregaSlotSock(1001, 1);
        CHECK(TCP::BuscaSlotSock(1001) == 1);

        TCP::AgregaSlotSock(1001, 4);
        CHECK(TCP::BuscaSlotSock(1001) == 4);
    }

    SUBCASE("Eliminación de asociación con BorraSlotSock") {
        TCP::AgregaSlotSock(1001, 1);
        TCP::AgregaSlotSock(1002, 2);

        TCP::BorraSlotSock(1001);
        CHECK(TCP::BuscaSlotSock(1001) == -1);
        CHECK(TCP::BuscaSlotSock(1002) == 2); // No afecta a otros

        // Borrado idempotente de socket no registrado
        TCP::BorraSlotSock(8888);
        CHECK(TCP::BuscaSlotSock(8888) == -1);
    }

    SUBCASE("Limpieza completa con ResetSockSlots") {
        TCP::AgregaSlotSock(1001, 1);
        TCP::AgregaSlotSock(1002, 2);
        TCP::ResetSockSlots();

        CHECK(TCP::BuscaSlotSock(1001) == -1);
        CHECK(TCP::BuscaSlotSock(1002) == -1);
    }
}

} // TEST_SUITE
 
TEST_SUITE("TCP_G3_ListenerYAceptacion") {

TEST_CASE("TCP - G3: Inicio y Parada del servidor") {
    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    CHECK(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    CHECK(port > 0);

    TCP::DetenerServidor();
    CHECK(TCP::PuertoEscucha() == 0);
}

TEST_CASE("TCP - G3: Aceptación Exitosa en Loopback") {
    UserListTestFixture fix(5);
    SecurityIp::InitIpTables();

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    REQUIRE(port > 0);

    asio::io_context client_ioc;
    asio::ip::tcp::socket client_sock(client_ioc);
    asio::error_code ec;
    client_sock.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);

    // Ejecutamos PollRed para procesar el evento de aceptación
    TCP::PollRed();

    // Verificamos que el slot 1 haya sido asignado correctamente
    CHECK(UserList[1].ConnIDValida == true);
    CHECK(UserList[1].ConnID > 0);
    CHECK(UserList[1].ip == "127.0.0.1");
    CHECK(TCP::BuscaSlotSock(static_cast<std::uint64_t>(UserList[1].ConnID)) == 1);

    client_sock.close(ec);
    TCP::DetenerServidor();
}

TEST_CASE("TCP - G3: Rechazo por Anti-Flood (Mitigación Bug #20)") {
    UserListTestFixture fix(5);
    SecurityIp::InitIpTables();

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    REQUIRE(port > 0);

    asio::io_context client_ioc;

    // Primer intento de conexión: debe ser aceptado
    asio::ip::tcp::socket client1(client_ioc);
    asio::error_code ec;
    client1.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);
    TCP::PollRed();

    CHECK(UserList[1].ConnIDValida == true);
    CHECK(UserList[1].ConnID > 0);

    // Segundo intento inmediato desde la misma IP (dentro del segundo de intervalo de SecurityIp):
    asio::ip::tcp::socket client2(client_ioc);
    client2.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);
    TCP::PollRed();

    // El slot 2 NO debe haberse asignado (queda libre con ConnID == -1)
    CHECK(UserList[2].ConnID == -1);
    CHECK_FALSE(UserList[2].ConnIDValida);

    // Mitigación del Bug #20: El socket rechazado se cierra explícitamente mediante RAII.
    // Al intentar leer del socket cliente, se recibe EOF porque el servidor cerró la conexión.
    std::uint8_t dummy[1];
    client2.read_some(asio::buffer(dummy), ec);
    CHECK(ec == asio::error::eof);

    client1.close(ec);
    client2.close(ec);
    TCP::DetenerServidor();
}

TEST_CASE("TCP - G3: Rechazo por Servidor Lleno") {
    UserListTestFixture fix(2);
    SecurityIp::InitIpTables();

    // Ocupamos manualmente la totalidad de los slots disponibles
    UserList[1].ConnID = 101;
    UserList[1].ConnIDValida = true;
    UserList[1].flags.UserLogged = true;

    UserList[2].ConnID = 102;
    UserList[2].ConnIDValida = true;
    UserList[2].flags.UserLogged = true;

    REQUIRE(TCP::NextOpenUser() > MaxUsers);

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    REQUIRE(port > 0);

    asio::io_context client_ioc;
    asio::ip::tcp::socket client(client_ioc);
    asio::error_code ec;
    client.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);

    TCP::PollRed();

    // Los slots existentes no deben haber sido alterados
    CHECK(UserList[1].ConnID == 101);
    CHECK(UserList[2].ConnID == 102);

    // La conexión del cliente entrante debe haber sido cerrada por el servidor
    std::uint8_t dummy[1];
    client.read_some(asio::buffer(dummy), ec);
    CHECK(ec == asio::error::eof);

    client.close(ec);
    TCP::DetenerServidor();
}

} // TEST_SUITE

TEST_SUITE("TCP_G4_RecepcionDatos") {

TEST_CASE("TCP - G4: Recepción de Bloque Pequeño") {
    UserListTestFixture fix(5);
    SecurityIp::InitIpTables();

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    REQUIRE(port > 0);

    asio::io_context client_ioc;
    asio::ip::tcp::socket client_sock(client_ioc);
    asio::error_code ec;
    client_sock.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);

    TCP::PollRed();
    std::int16_t slot = 1;
    REQUIRE(UserList[slot].incomingData != nullptr);

    // Enviar payload de 10 bytes
    std::string payload = "HOLA MUNDO";
    asio::write(client_sock, asio::buffer(payload), ec);
    REQUIRE(!ec);

    // Procesar lectura en el servidor
    TCP::PollRed();

    CHECK(UserList[slot].incomingData->length() == 10);
    CHECK(UserList[slot].incomingData->ReadASCIIStringFixed(10) == "HOLA MUNDO");
    CHECK(UserList[slot].incomingData->length() == 0);

    client_sock.close(ec);
    TCP::DetenerServidor();
}

TEST_CASE("TCP - G4: Recepción de Bloque Grande (4 KB)") {
    UserListTestFixture fix(5);
    SecurityIp::InitIpTables();

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    REQUIRE(port > 0);

    asio::io_context client_ioc;
    asio::ip::tcp::socket client_sock(client_ioc);
    asio::error_code ec;
    client_sock.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);

    TCP::PollRed();
    std::int16_t slot = 1;
    REQUIRE(UserList[slot].incomingData != nullptr);

    // Crear bloque binario determinista de 4096 bytes
    std::vector<std::uint8_t> big_data(4096);
    for (std::size_t i = 0; i < big_data.size(); ++i) {
        big_data[i] = static_cast<std::uint8_t>(i % 256);
    }

    asio::write(client_sock, asio::buffer(big_data), ec);
    REQUIRE(!ec);

    TCP::PollRed();

    CHECK(UserList[slot].incomingData->length() == 4096);

    std::vector<std::uint8_t> received(4096);
    UserList[slot].incomingData->ReadBlock(received, 4096);
    CHECK(received == big_data);
    CHECK(UserList[slot].incomingData->length() == 0);

    client_sock.close(ec);
    TCP::DetenerServidor();
}

TEST_CASE("TCP - G4: Fragmentación de Paquetes en el Stream (FIFO)") {
    UserListTestFixture fix(5);
    SecurityIp::InitIpTables();

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    REQUIRE(port > 0);

    asio::io_context client_ioc;
    asio::ip::tcp::socket client_sock(client_ioc);
    asio::error_code ec;
    client_sock.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);

    TCP::PollRed();
    std::int16_t slot = 1;
    REQUIRE(UserList[slot].incomingData != nullptr);

    // Enviar primera parte
    std::string part1 = "PARTE 1 ";
    asio::write(client_sock, asio::buffer(part1), ec);
    REQUIRE(!ec);
    TCP::PollRed();
    CHECK(UserList[slot].incomingData->length() == 8);

    // Enviar segunda parte
    std::string part2 = "PARTE 2";
    asio::write(client_sock, asio::buffer(part2), ec);
    REQUIRE(!ec);
    TCP::PollRed();
    CHECK(UserList[slot].incomingData->length() == 15);

    // Comprobar orden estricto FIFO
    CHECK(UserList[slot].incomingData->ReadASCIIStringFixed(15) == "PARTE 1 PARTE 2");
    CHECK(UserList[slot].incomingData->length() == 0);

    client_sock.close(ec);
    TCP::DetenerServidor();
}

TEST_CASE("TCP - G4: Aislamiento Multicliente") {
    UserListTestFixture fix(5);
    SecurityIp::InitIpTables();

    static std::uint32_t s_test_ticks = 10000;
    SecurityIp::SetTimeSourceForTesting([]() -> std::uint32_t { return s_test_ticks; });

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    REQUIRE(port > 0);

    asio::io_context client_ioc;

    // Cliente 1
    asio::ip::tcp::socket client1(client_ioc);
    asio::error_code ec;
    client1.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);
    TCP::PollRed();
    REQUIRE(UserList[1].ConnIDValida == true);

    // Avanzamos el reloj simulado para evitar el anti-flood en el segundo cliente
    s_test_ticks += 2000;

    // Cliente 2
    asio::ip::tcp::socket client2(client_ioc);
    client2.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);
    TCP::PollRed();
    REQUIRE(UserList[2].ConnIDValida == true);

    // Hook opcional de paquetes para verificar invocación
    std::vector<std::int16_t> notified_slots;
    TCP::SetPacketHandler([&notified_slots](std::int16_t u_idx) {
        notified_slots.push_back(u_idx);
    });

    // Envío de mensajes distintos
    std::string msg1 = "MSG_CLIENTE_UNO";
    std::string msg2 = "MSG_CLIENTE_DOS";

    asio::write(client1, asio::buffer(msg1), ec);
    REQUIRE(!ec);

    asio::write(client2, asio::buffer(msg2), ec);
    REQUIRE(!ec);

    TCP::PollRed();

    // Comprobamos aislamiento estricto de buffers
    CHECK(UserList[1].incomingData->ReadASCIIStringFixed(static_cast<std::int32_t>(msg1.size())) == "MSG_CLIENTE_UNO");
    CHECK(UserList[2].incomingData->ReadASCIIStringFixed(static_cast<std::int32_t>(msg2.size())) == "MSG_CLIENTE_DOS");

    CHECK(UserList[1].incomingData->length() == 0);
    CHECK(UserList[2].incomingData->length() == 0);

    // Notificaciones recibidas
    CHECK(notified_slots.size() == 2);

    client1.close(ec);
    client2.close(ec);
    TCP::DetenerServidor();
    SecurityIp::ResetTimeSource();
}

} // TEST_SUITE

TEST_SUITE("TCP_G5_EnvioYBackpressure") {

TEST_CASE("TCP - G5: Envío Básico y FlushBuffer") {
    UserListTestFixture fix(5);
    SecurityIp::InitIpTables();

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    REQUIRE(port > 0);

    asio::io_context client_ioc;
    asio::ip::tcp::socket client_sock(client_ioc);
    asio::error_code ec;
    client_sock.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);

    TCP::PollRed();
    std::int16_t slot = 1;
    REQUIRE(UserList[slot].ConnIDValida == true);

    // Escribir en UserList[slot].outgoingData
    std::string msg = "MENSAJE_SALIENTE";
    UserList[slot].outgoingData->WriteASCIIStringFixed(msg);
    CHECK(UserList[slot].outgoingData->length() == static_cast<std::int32_t>(msg.size()));

    // Invocar FlushBuffer
    TCP::FlushBuffer(slot);
    CHECK(UserList[slot].outgoingData->length() == 0);

    // Despachar el envío con PollRed
    TCP::PollRed();

    // Leer en el cliente
    std::vector<char> recv_buf(msg.size());
    asio::read(client_sock, asio::buffer(recv_buf), ec);
    REQUIRE(!ec);
    CHECK(std::string(recv_buf.data(), recv_buf.size()) == "MENSAJE_SALIENTE");

    client_sock.close(ec);
    TCP::DetenerServidor();
}

TEST_CASE("TCP - G5: Múltiples Envíos Encadenados (Queueing)") {
    UserListTestFixture fix(5);
    SecurityIp::InitIpTables();

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    REQUIRE(port > 0);

    asio::io_context client_ioc;
    asio::ip::tcp::socket client_sock(client_ioc);
    asio::error_code ec;
    client_sock.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);

    TCP::PollRed();
    std::int16_t slot = 1;
    REQUIRE(UserList[slot].ConnIDValida == true);

    // Realizar 3 llamadas consecutivas a EnviarDatosASlot antes de correr PollRed
    TCP::EnviarDatosASlot(slot, "P1");
    TCP::EnviarDatosASlot(slot, "P2");
    TCP::EnviarDatosASlot(slot, "P3");

    // Despachar escrituras encoladas
    TCP::PollRed();

    // Leer del cliente y comprobar orden FIFO ("P1P2P3")
    std::vector<char> recv_buf(6);
    asio::read(client_sock, asio::buffer(recv_buf), ec);
    REQUIRE(!ec);
    CHECK(std::string(recv_buf.data(), recv_buf.size()) == "P1P2P3");

    client_sock.close(ec);
    TCP::DetenerServidor();
}

TEST_CASE("TCP - G5: Mitigación Bug #19 (Protección de Backpressure)") {
    UserListTestFixture fix(5);
    SecurityIp::InitIpTables();

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    REQUIRE(port > 0);

    asio::io_context client_ioc;
    asio::ip::tcp::socket client_sock(client_ioc);
    asio::error_code ec;
    client_sock.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);

    TCP::PollRed();
    std::int16_t slot = 1;
    REQUIRE(UserList[slot].ConnIDValida == true);

    // El cliente NO lee del socket.
    // Enviamos ráfagas acumuladas que exceden MAX_OUTGOING_BUFFER_SIZE (64 KB = 65536 bytes).
    std::string chunk(16384, 'A'); // 16 KB

    // 5 * 16 KB = 80 KB > 64 KB
    for (int i = 0; i < 5; ++i) {
        TCP::EnviarDatosASlot(slot, chunk);
    }

    // La saturación se detecta inmediatamente en la 5ta llamada al superar 64 KB.
    // La conexión debe haber sido cerrada preventivamente para mitigar el Bug #19.
    CHECK_FALSE(UserList[slot].ConnIDValida);

    // Procesamos cualquier handler pendiente
    TCP::PollRed();

    // Comprobamos que el cliente reciba EOF al haber sido desconectado por el servidor
    std::vector<char> drain_buf(1024);
    asio::error_code read_ec;
    while (!read_ec) {
        client_sock.read_some(asio::buffer(drain_buf), read_ec);
    }
    CHECK((read_ec == asio::error::eof || read_ec == asio::error::connection_reset));

    client_sock.close(ec);
    TCP::DetenerServidor();
}

} // TEST_SUITE

TEST_SUITE("TCP_G6_CicloVidaYAntiCombatLog") {

TEST_CASE("TCP - G6: Desconexión ordenada en zona segura (salida inmediata)") {
    UserListTestFixture fix(5);
    SecurityIp::InitIpTables();

    MapInfoList.resize(10);
    MapInfoList[1].Pk = false; // Zona segura
    IntervaloCerrarConexion = 10;
    NumUsers = 0;
    LastUser = 0;

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    REQUIRE(port > 0);

    asio::io_context client_ioc;
    asio::ip::tcp::socket client_sock(client_ioc);
    asio::error_code ec;
    client_sock.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);

    TCP::PollRed();
    std::int16_t slot = 1;
    REQUIRE(UserList[slot].ConnIDValida == true);

    // Simulación de usuario autenticado en zona segura
    UserList[slot].flags.UserLogged = true;
    UserList[slot].Pos.Map = 1;
    UserList[slot].flags.Privilegios = PlayerType::UserPlayer;
    NumUsers = 1;
    LastUser = slot;

    // Cliente solicita /SALIR (CerrarUsuario)
    TCP::CerrarUsuario(slot);

    CHECK(UserList[slot].Counters.Saliendo == true);
    CHECK(UserList[slot].Counters.Salir == 0); // Zona segura: 0 segundos

    // Al ejecutarse el ciclo de PasarSegundoUsuarios(), se detecta Salir <= 0 y se cierra
    TCP::PasarSegundoUsuarios();

    CHECK(UserList[slot].flags.UserLogged == false);
    CHECK(UserList[slot].ConnID == -1);
    CHECK(UserList[slot].ConnIDValida == false);
    CHECK(NumUsers == 0);
    CHECK(LastUser == 0);
    CHECK(TCP::NextOpenUser() == slot);

    // Comprobar que el socket físico del cliente recibió el corte de conexión (EOF)
    TCP::PollRed();
    std::vector<char> drain(64);
    asio::error_code rec;
    while (!rec) {
        client_sock.read_some(asio::buffer(drain), rec);
    }
    CHECK((rec == asio::error::eof || rec == asio::error::connection_reset));

    client_sock.close(ec);
    TCP::DetenerServidor();
}

TEST_CASE("TCP - G6: Mecánica Anti-CombatLog en zona PK (retención de entidad por 10s tras corte abrupto)") {
    UserListTestFixture fix(5);
    SecurityIp::InitIpTables();

    MapInfoList.resize(10);
    MapInfoList[2].Pk = true; // Zona PK
    IntervaloCerrarConexion = 10;
    NumUsers = 0;
    LastUser = 0;

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t port = TCP::PuertoEscucha();
    REQUIRE(port > 0);

    asio::io_context client_ioc;
    asio::ip::tcp::socket client_sock(client_ioc);
    asio::error_code ec;
    client_sock.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), port), ec);
    REQUIRE(!ec);

    TCP::PollRed();
    std::int16_t slot = 1;
    REQUIRE(UserList[slot].ConnIDValida == true);

    // Usuario autenticado en zona PK
    UserList[slot].flags.UserLogged = true;
    UserList[slot].Pos.Map = 2;
    UserList[slot].flags.Privilegios = PlayerType::UserPlayer;
    NumUsers = 1;
    LastUser = slot;

    // Corte abrupto de conexión física por el cliente (kill de proceso / cable desconectado)
    client_sock.close(ec);

    // Procesar eventos de red: Asio detecta EOF en HandleRead
    TCP::PollRed();

    // Verificación de Anti-CombatLog:
    // 1. El socket físico está cerrado y desasociado
    CHECK_FALSE(UserList[slot].ConnIDValida);
    CHECK(UserList[slot].ConnID == -1);

    // 2. La entidad lógica PERMANECE logueada en el mapa con la cuenta regresiva de 10 segundos
    CHECK(UserList[slot].flags.UserLogged == true);
    CHECK(UserList[slot].Counters.Saliendo == true);
    CHECK(UserList[slot].Counters.Salir == 10);
    CHECK(NumUsers == 1);

    // 3. El slot NO puede ser reclamado por otra conexión entrante mientras está saliendo
    CHECK(TCP::NextOpenUser() != slot);
    CHECK(TCP::NextOpenUser() == 2);

    TCP::DetenerServidor();
}

TEST_CASE("TCP - G6: Anti-exploit (cancelación inmediata de invisibilidad y ocultamiento)") {
    UserListTestFixture fix(5);

    MapInfoList.resize(10);
    MapInfoList[2].Pk = true; // Zona PK
    IntervaloCerrarConexion = 10;

    std::int16_t slot = 1;
    UserList[slot].flags.UserLogged = true;
    UserList[slot].Pos.Map = 2;
    UserList[slot].flags.Privilegios = PlayerType::UserPlayer;

    // Intento de desconexión táctica estando invisible y oculto
    UserList[slot].flags.invisible = 1;
    UserList[slot].flags.Oculto = 1;

    TCP::CerrarUsuario(slot);

    // Verificación: la invisibilidad y el ocultamiento se anulan en el acto
    CHECK(UserList[slot].flags.invisible == 0);
    CHECK(UserList[slot].flags.Oculto == 0);
    CHECK(UserList[slot].Counters.Saliendo == true);
    CHECK(UserList[slot].Counters.Salir == 10);

    SUBCASE("Idempotencia: una segunda llamada a CerrarUsuario no resetea el contador") {
        UserList[slot].Counters.Salir = 5;
        TCP::CerrarUsuario(slot);
        CHECK(UserList[slot].Counters.Salir == 5);
    }
}

TEST_CASE("TCP - G6: Consunción del temporizador de 10s y liberación definitiva del slot") {
    UserListTestFixture fix(5);

    MapInfoList.resize(10);
    MapInfoList[2].Pk = true;
    IntervaloCerrarConexion = 10;
    NumUsers = 1;
    LastUser = 1;

    std::int16_t slot = 1;
    UserList[slot].flags.UserLogged = true;
    UserList[slot].Pos.Map = 2;
    UserList[slot].flags.Privilegios = PlayerType::UserPlayer;
    UserList[slot].name = "Morgolock";

    TCP::CerrarUsuario(slot);
    REQUIRE(UserList[slot].Counters.Saliendo == true);
    REQUIRE(UserList[slot].Counters.Salir == 10);

    // Registro de callback de persistencia (CloseUserHandler)
    bool handler_invoked = false;
    std::int16_t persisted_slot = -1;
    std::string persisted_name;

    TCP::SetCloseUserHandler([&](std::int16_t u) {
        handler_invoked = true;
        persisted_slot = u;
        persisted_name = UserList[u].name;
    });

    // Avance de los primeros 9 segundos: la entidad debe seguir viva y protegida
    for (int sec = 1; sec <= 9; ++sec) {
        TCP::PasarSegundoUsuarios();
        CHECK(UserList[slot].flags.UserLogged == true);
        CHECK(UserList[slot].Counters.Salir == 10 - sec);
        CHECK_FALSE(handler_invoked);
        CHECK(NumUsers == 1);
        CHECK(TCP::NextOpenUser() != slot);
    }

    // Segundo 10: expiración final del contador (Salir <= 0)
    TCP::PasarSegundoUsuarios();

    CHECK(handler_invoked == true);
    CHECK(persisted_slot == slot);
    CHECK(persisted_name == "Morgolock");

    // El slot quedó completamente liberado y blanqueado
    CHECK(UserList[slot].flags.UserLogged == false);
    CHECK(UserList[slot].ConnID == -1);
    CHECK(UserList[slot].ConnIDValida == false);
    CHECK(NumUsers == 0);
    CHECK(LastUser == 0);
    CHECK(TCP::NextOpenUser() == slot);

    TCP::SetCloseUserHandler(nullptr);
}

} // TEST_SUITE

TEST_SUITE("TCP_G7_DiagnosticoYCierre") {

TEST_CASE("TCP / SecurityIp - G7: Diagnóstico de tablas de IP (DumpTables)") {
    // Inicializar tablas limpias
    SecurityIp::InitIpTables(10);

    // Insertar un par de IPs de prueba mediante IpSecurityAceptarNuevaConexion
    std::int32_t ip1 = static_cast<std::int32_t>(TCP::GetLongIp("192.168.1.100"));
    std::int32_t ip2 = static_cast<std::int32_t>(TCP::GetLongIp("10.0.0.50"));

    CHECK(SecurityIp::IpSecurityAceptarNuevaConexion(ip1));
    CHECK(SecurityIp::IpSecurityAceptarNuevaConexion(ip2));

    std::vector<std::string> log_lines;
    SecurityIp::DumpTables([&](std::string_view line) {
        log_lines.emplace_back(line);
    });

    // Debe haber registrado exactamente las entradas activas (MaxValue == 2)
    REQUIRE(log_lines.size() == 2);

    // Verificar que cada línea contenga la IP convertida por TCP::GetAscIP
    bool found_ip1 = false;
    bool found_ip2 = false;
    for (const auto& line : log_lines) {
        if (line.find("192.168.1.100") != std::string::npos) {
            found_ip1 = true;
        }
        if (line.find("10.0.0.50") != std::string::npos) {
            found_ip2 = true;
        }
    }
    CHECK(found_ip1);
    CHECK(found_ip2);

    // Llamada con nullptr no debe arrojar excepciones (emite a std::clog)
    CHECK_NOTHROW(SecurityIp::DumpTables(nullptr));
}

TEST_CASE("TCP - G7: Reinicio global de sockets y servidor (WSApiReiniciarSockets)") {
    UserListTestFixture fix(5);
    SecurityIp::InitIpTables();

    static std::uint32_t s_restart_ticks = 20000;
    SecurityIp::SetTimeSourceForTesting([]() -> std::uint32_t { return s_restart_ticks; });

    bool ok = TCP::IniciaServidor(0, "127.0.0.1");
    REQUIRE(ok);
    std::uint16_t puerto_original = TCP::PuertoEscucha();
    REQUIRE(puerto_original > 0);

    asio::io_context client_ioc;

    // Conectar Cliente 1
    asio::ip::tcp::socket client1(client_ioc);
    asio::error_code ec;
    client1.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), puerto_original), ec);
    REQUIRE(!ec);

    // Aceptar Cliente 1
    TCP::PollRed();
    REQUIRE(UserList[1].ConnIDValida == true);
    REQUIRE(UserList[1].ConnID != -1);

    // Avanzar reloj simulado para superar el filtro anti-flood de SecurityIp
    s_restart_ticks += 2000;

    // Conectar Cliente 2
    asio::ip::tcp::socket client2(client_ioc);
    client2.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), puerto_original), ec);
    REQUIRE(!ec);

    // Aceptar Cliente 2
    TCP::PollRed();
    REQUIRE(UserList[2].ConnIDValida == true);
    REQUIRE(UserList[2].ConnID != -1);

    // Marcar usuario 1 como logueado para probar que CloseSocket también desloguee
    UserList[1].flags.UserLogged = true;
    NumUsers = 1;
    LastUser = 2;

    // Ejecutar reinicio global de sockets
    TCP::WSApiReiniciarSockets();

    // 1. Verificar que los slots de UserList quedaron limpios
    CHECK(UserList[1].ConnID == -1);
    CHECK_FALSE(UserList[1].ConnIDValida);
    CHECK_FALSE(UserList[1].flags.UserLogged);
    CHECK(UserList[2].ConnID == -1);
    CHECK_FALSE(UserList[2].ConnIDValida);
    CHECK_FALSE(UserList[2].flags.UserLogged);
    CHECK(NumUsers == 0);
    CHECK(LastUser == 0);

    // 2. Procesar red para que los clientes reciban el corte
    TCP::PollRed();

    std::vector<char> drain_buf(64);
    asio::error_code rec1, rec2;
    while (!rec1) {
        client1.read_some(asio::buffer(drain_buf), rec1);
    }
    while (!rec2) {
        client2.read_some(asio::buffer(drain_buf), rec2);
    }
    CHECK((rec1 == asio::error::eof || rec1 == asio::error::connection_reset));
    CHECK((rec2 == asio::error::eof || rec2 == asio::error::connection_reset));

    client1.close(ec);
    client2.close(ec);

    // 3. El servidor continúa escuchando en el mismo puerto
    CHECK(TCP::PuertoEscucha() == puerto_original);

    // Avanzar reloj simulado antes de la nueva conexión
    s_restart_ticks += 2000;

    // 4. Una nueva conexión cliente puede conectarse exitosamente tras el reinicio
    asio::ip::tcp::socket client3(client_ioc);
    client3.connect(asio::ip::tcp::endpoint(asio::ip::make_address_v4("127.0.0.1"), puerto_original), ec);
    REQUIRE(!ec);

    TCP::PollRed();

    // Slot 1 vuelve a estar asignado para la nueva conexión
    CHECK(UserList[1].ConnIDValida == true);
    CHECK(UserList[1].ConnID != -1);

    client3.close(ec);
    TCP::DetenerServidor();
    SecurityIp::ResetTimeSource();
}

} // TEST_SUITE



