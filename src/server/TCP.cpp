#include "TCP.hpp"

#include <asio.hpp>
#include "Declares.hpp"
#include "SecurityIp.hpp"

#include <array>
#include <vector>
#include <memory>
#include <cstring>
#include <unordered_map>
#include <deque>

namespace {

// Mapeo asociativo sock_id -> slot de usuario (reemplazo moderno de WSAPISock2Usr de wskapiAO.bas)
std::unordered_map<std::uint64_t, std::int16_t> s_sock_to_slot;

// Estado del subsistema de red monohilo Asio (G3, G4 y G5)
constexpr std::size_t SIZE_RCVBUF = 8192;

struct OutgoingQueue {
    std::deque<std::vector<std::uint8_t>> queue;
    std::size_t queued_bytes{0};
    bool is_writing{false};
};

std::unique_ptr<asio::io_context> s_io_context;
std::unique_ptr<asio::ip::tcp::acceptor> s_acceptor;
std::vector<std::unique_ptr<asio::ip::tcp::socket>> s_client_sockets;
std::vector<std::array<std::uint8_t, SIZE_RCVBUF>> s_client_read_buffers;
std::vector<OutgoingQueue> s_client_write_queues;
TCP::PacketHandler s_packet_handler = nullptr;
TCP::CloseUserHandler s_close_user_handler = nullptr;
TCP::SendDataHook s_send_data_hook = nullptr;
std::int32_t s_next_conn_id = 1;
std::string s_bound_ip;

void DoAccept();
void HandleRead(std::int16_t user_index, const asio::error_code& ec, std::size_t bytes_transferred);
void DoWrite(std::int16_t user_index);
void HandleWrite(std::int16_t user_index, const asio::error_code& ec, std::size_t bytes_transferred);

void HandleAccept(asio::ip::tcp::socket socket) {
    asio::error_code ec;
    auto remote_ep = socket.remote_endpoint(ec);
    if (ec) {
        return;
    }

    std::uint32_t client_ip = 0;
    if (remote_ep.address().is_v4()) {
        auto bytes = remote_ep.address().to_v4().to_bytes();
        std::memcpy(&client_ip, bytes.data(), 4);
    } else {
        return;
    }

    // Filtro Anti-Flood síncrono (wskapiAO.bas:402)
    // Mitigación del Bug #20: si se rechaza, el socket se destruye automáticamente
    // mediante RAII al salir del ámbito, cerrándolo explícitamente sin fugas de descriptores.
    if (!SecurityIp::IpSecurityAceptarNuevaConexion(static_cast<std::int32_t>(client_ip))) {
        socket.close(ec);
        return;
    }

    // Asignación de slot de usuario (wskapiAO.bas:444)
    std::int16_t slot = TCP::NextOpenUser();

    // Servidor lleno (wskapiAO.bas:480-494)
    if (slot > MaxUsers || static_cast<std::size_t>(slot) >= UserList.size()) {
        socket.close(ec);
        return;
    }

    // Asegurar tamaño en contenedor de sockets de cliente indexado por slot
    if (static_cast<std::size_t>(slot) >= s_client_sockets.size()) {
        s_client_sockets.resize(std::max<std::size_t>(slot + 1, static_cast<std::size_t>(MaxUsers + 1)));
        s_client_read_buffers.resize(s_client_sockets.size());
        s_client_write_queues.resize(s_client_sockets.size());
    } else if (static_cast<std::size_t>(slot) >= s_client_write_queues.size()) {
        s_client_write_queues.resize(s_client_sockets.size());
    }

    s_client_write_queues[slot].queue.clear();
    s_client_write_queues[slot].queued_bytes = 0;
    s_client_write_queues[slot].is_writing = false;

    // Limpieza de buffers de datos entrantes y salientes (wskapiAO.bas:452-453)
    auto& user = UserList[slot];
    if (user.incomingData) {
        user.incomingData->ReadASCIIStringFixed(user.incomingData->length());
    } else {
        user.incomingData = std::make_unique<clsByteQueue>();
    }

    if (user.outgoingData) {
        user.outgoingData->ReadASCIIStringFixed(user.outgoingData->length());
    } else {
        user.outgoingData = std::make_unique<clsByteQueue>();
    }

    // Asignación de IP en formato punteado (wskapiAO.bas:459)
    user.ip = TCP::GetAscIP(client_ip);

    // Asignación de identificadores de conexión (wskapiAO.bas:474-475)
    std::int32_t conn_id = s_next_conn_id++;
    user.ConnID = conn_id;
    user.ConnIDValida = true;

    // Asociación en mapa asociativo sock_id -> slot (wskapiAO.bas:477)
    TCP::AgregaSlotSock(static_cast<std::uint64_t>(conn_id), slot);

    // Almacenar el socket en el contenedor interno del slot
    s_client_sockets[slot] = std::make_unique<asio::ip::tcp::socket>(std::move(socket));

    // Iniciar recepción continua de datos para el cliente aceptado (G4, wskapiAO.bas:255-275)
    TCP::IniciarLectura(slot);
}

void DoAccept() {
    if (!s_acceptor || !s_acceptor->is_open()) {
        return;
    }

    s_acceptor->async_accept(
        [](const asio::error_code& ec, asio::ip::tcp::socket socket) {
            if (ec) {
                if (ec == asio::error::operation_aborted) {
                    return;
                }
                DoAccept();
                return;
            }

            HandleAccept(std::move(socket));
            DoAccept();
        }
    );
}

void HandleRead(std::int16_t user_index, const asio::error_code& ec, std::size_t bytes_transferred) {
    // Port de wskapiAO.bas líneas 255-293 y 499-518 (EventoSockRead)
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size() ||
        static_cast<std::size_t>(user_index) >= s_client_sockets.size()) {
        return;
    }

    auto& user = UserList[user_index];
    auto& sock = s_client_sockets[user_index];

    if (!sock || user.ConnID == -1) {
        return;
    }

    if (!ec && bytes_transferred > 0) {
        // Escritura directa de los bytes recibidos en la cola FIFO entrante (wskapiAO.bas:507)
        if (!user.incomingData) {
            user.incomingData = std::make_unique<clsByteQueue>();
        }

        user.incomingData->WriteBlock(s_client_read_buffers[user_index].data(), static_cast<std::int32_t>(bytes_transferred));

        // Invocación del callback opcional de notificación de paquetes (Protocol.bas / HandleIncomingData)
        if (s_packet_handler) {
            s_packet_handler(user_index);
        }

        // Re-armado continuo de lectura para el slot (wskapiAO.bas:275)
        TCP::IniciarLectura(user_index);
    } else {
        // Desconexión o error de socket (wskapiAO.bas:277-293, 520-538)
        if (user.flags.UserLogged) {
            TCP::CloseSocketSL(user_index);
            TCP::CerrarUsuario(user_index);
        } else {
            TCP::CloseSocket(user_index);
        }
    }
}

void DoWrite(std::int16_t user_index) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= s_client_sockets.size() ||
        static_cast<std::size_t>(user_index) >= s_client_write_queues.size()) {
        return;
    }

    auto& sock = s_client_sockets[user_index];
    auto& out_q = s_client_write_queues[user_index];

    if (!sock || !sock->is_open() || out_q.queue.empty()) {
        out_q.is_writing = false;
        return;
    }

    asio::async_write(
        *sock,
        asio::buffer(out_q.queue.front()),
        [user_index](const asio::error_code& ec, std::size_t bytes_transferred) {
            HandleWrite(user_index, ec, bytes_transferred);
        }
    );
}

void HandleWrite(std::int16_t user_index, const asio::error_code& ec, std::size_t bytes_transferred) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= s_client_sockets.size() ||
        static_cast<std::size_t>(user_index) >= s_client_write_queues.size()) {
        return;
    }

    auto& sock = s_client_sockets[user_index];
    auto& out_q = s_client_write_queues[user_index];

    if (ec) {
        if (user_index >= 1 && static_cast<std::size_t>(user_index) < UserList.size()) {
            if (UserList[user_index].flags.UserLogged) {
                TCP::CloseSocketSL(user_index);
                TCP::CerrarUsuario(user_index);
            } else {
                TCP::CloseSocket(user_index);
            }
        } else if (sock) {
            asio::error_code close_ec;
            sock->cancel(close_ec);
            sock->close(close_ec);
            sock.reset();
            out_q.queue.clear();
            out_q.queued_bytes = 0;
            out_q.is_writing = false;
        }
        return;
    }

    if (!out_q.queue.empty()) {
        if (out_q.queued_bytes >= bytes_transferred) {
            out_q.queued_bytes -= bytes_transferred;
        } else {
            out_q.queued_bytes = 0;
        }
        out_q.queue.pop_front();
    }

    if (!out_q.queue.empty()) {
        DoWrite(user_index);
    } else {
        out_q.is_writing = false;
    }
}

} // namespace

namespace TCP {

bool IsValidIP(std::string_view ip) {
    if (ip.empty()) {
        return false;
    }

    int dots = 0;
    std::size_t start = 0;

    for (std::size_t i = 0; i <= ip.size(); ++i) {
        if (i == ip.size() || ip[i] == '.') {
            if (i == ip.size() && dots != 3) {
                return false;
            }

            const std::size_t len = i - start;
            if (len == 0 || len > 3) {
                return false;
            }

            int val = 0;
            for (std::size_t j = start; j < i; ++j) {
                const char c = ip[j];
                if (c < '0' || c > '9') {
                    return false;
                }
                val = val * 10 + (c - '0');
            }

            if (val > 255) {
                return false;
            }

            if (i < ip.size()) {
                dots++;
                start = i + 1;
            }
        }
    }

    return dots == 3;
}

std::string GetAscIP(std::uint32_t ip) {
    // Port de wsksock.bas líneas 527-545:
    // lpStr = inet_ntoa(inn)
    // If lpStr Then ... Else GetAscIP = "255.255.255.255"
    if (ip == 0xFFFFFFFFU) {
        return "255.255.255.255";
    }

    std::array<unsigned char, 4> bytes{};
    std::memcpy(bytes.data(), &ip, 4);

    asio::ip::address_v4 addr(bytes);
    return addr.to_string();
}

std::uint32_t GetLongIp(std::string_view ip) {
    // Port de wsksock.bas líneas 801-803:
    // GetLongIp = inet_addr(IPS)
    if (!IsValidIP(ip)) {
        return 0;
    }

    asio::error_code ec;
    auto addr = asio::ip::make_address_v4(std::string(ip), ec);
    if (ec) {
        return 0;
    }

    const auto bytes = addr.to_bytes();
    std::uint32_t net_ip = 0;
    std::memcpy(&net_ip, bytes.data(), 4);
    return net_ip;
}

std::int16_t NextOpenUser() {
    // Port de Modulo_UsUaRiOs.bas líneas 868-883:
    // For LoopC = 1 To MaxUsers + 1
    //     If LoopC > MaxUsers Then Exit For
    //     If (UserList(LoopC).ConnID = -1 And UserList(LoopC).flags.UserLogged = False) Then Exit For
    // Next LoopC
    // NextOpenUser = LoopC

    for (std::int16_t i = 1; i <= MaxUsers; ++i) {
        if (static_cast<std::size_t>(i) < UserList.size()) {
            if (UserList[i].ConnID == -1 && !UserList[i].flags.UserLogged) {
                return i;
            }
        }
    }

    return MaxUsers + 1;
}

void ResetUserSlot(std::int16_t user_index) {
    // Port de TCP.bas líneas 1719-1756:
    // UserList(UserIndex).ConnIDValida = False
    // UserList(UserIndex).ConnID = -1
    // Call ResetContadores(UserIndex)
    // Call ResetUserFlags(UserIndex)
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    user.ConnIDValida = false;
    user.ConnID = -1;
    user.flags.UserLogged = false;

    // Reseteo de contadores básicos de red asociados al slot
    user.Counters.Salir = 0;
    user.Counters.Saliendo = false;
    user.Counters.IdleCount = 0;
    user.ip.clear();

    if (static_cast<std::size_t>(user_index) < s_client_write_queues.size()) {
        s_client_write_queues[user_index].queue.clear();
        s_client_write_queues[user_index].queued_bytes = 0;
        s_client_write_queues[user_index].is_writing = false;
    }

    // TODO(Capa 6/7): Conectar las siguientes rutinas cuando se porten sus módulos respectivos:
    // - LimpiarComercioSeguro(user_index) (mdlComercioConUsuario)
    // - ResetFacciones(user_index) (Facciones)
    // - ResetContadores(user_index) completo
    // - ResetGuildInfo(user_index) (modGuilds)
    // - ResetCharInfo(user_index)
    // - ResetBasicUserInfo(user_index)
    // - ResetReputacion(user_index)
    // - ResetUserFlags(user_index) completo
    // - LimpiarInventario(user_index) (InvUsuario)
    // - ResetUserSpells(user_index)
    // - ResetUserPets(user_index)
    // - ResetUserBanco(user_index) (InvBanco)
}

std::int16_t BuscaSlotSock(std::uint64_t sock_id) {
    // Port de wskapiAO.bas líneas 130-142:
    // BuscaSlotSock = WSAPISock2Usr.Item(CStr(S))
    // hayerror: BuscaSlotSock = -1
    auto it = s_sock_to_slot.find(sock_id);
    if (it != s_sock_to_slot.end()) {
        return it->second;
    }
    return -1;
}

void AgregaSlotSock(std::uint64_t sock_id, std::int16_t slot) {
    // Port de wskapiAO.bas líneas 144-186:
    // WSAPISock2Usr.Add CStr(Slot), CStr(Sock)
    s_sock_to_slot[sock_id] = slot;
}

void BorraSlotSock(std::uint64_t sock_id) {
    // Port de wskapiAO.bas líneas 188-199:
    // WSAPISock2Usr.Remove CStr(Sock)
    s_sock_to_slot.erase(sock_id);
}

void ResetSockSlots() {
    s_sock_to_slot.clear();
}

bool IniciaServidor(std::uint16_t puerto, std::string_view bind_ip) {
    // Port de wsksock.bas líneas 834-896 (ListenForConnect):
    DetenerServidor();

    if (SecurityIp::IpTables.empty()) {
        SecurityIp::InitIpTables();
    }

    s_io_context = std::make_unique<asio::io_context>();

    asio::error_code ec;
    asio::ip::tcp::endpoint endpoint;

    if (bind_ip.empty()) {
        endpoint = asio::ip::tcp::endpoint(asio::ip::tcp::v4(), puerto);
    } else {
        auto addr = asio::ip::make_address_v4(std::string(bind_ip), ec);
        if (ec) {
            return false;
        }
        endpoint = asio::ip::tcp::endpoint(addr, puerto);
    }

    s_acceptor = std::make_unique<asio::ip::tcp::acceptor>(*s_io_context);
    s_acceptor->open(endpoint.protocol(), ec);
    if (ec) {
        s_acceptor.reset();
        return false;
    }

    s_acceptor->set_option(asio::ip::tcp::acceptor::reuse_address(true), ec);
    if (ec) {
        s_acceptor.reset();
        return false;
    }

    s_acceptor->bind(endpoint, ec);
    if (ec) {
        s_acceptor.reset();
        return false;
    }

    // En VB6: listen(S, SOMAXCONN)
    s_acceptor->listen(asio::socket_base::max_listen_connections, ec);
    if (ec) {
        s_acceptor.reset();
        return false;
    }

    s_bound_ip = std::string(bind_ip);
    DoAccept();
    return true;
}

void DetenerServidor() {
    if (s_acceptor) {
        asio::error_code ec;
        s_acceptor->cancel(ec);
        s_acceptor->close(ec);
        s_acceptor.reset();
    }

    for (auto& sock : s_client_sockets) {
        if (sock) {
            asio::error_code ec;
            sock->cancel(ec);
            sock->close(ec);
            sock.reset();
        }
    }
    s_client_sockets.clear();
    s_client_read_buffers.clear();
    s_client_write_queues.clear();

    if (s_io_context) {
        s_io_context->stop();
        s_io_context.reset();
    }

    ResetSockSlots();
    s_next_conn_id = 1;
    s_packet_handler = nullptr;
    s_close_user_handler = nullptr;
    s_bound_ip.clear();
}

std::uint16_t PuertoEscucha() {
    if (s_acceptor && s_acceptor->is_open()) {
        asio::error_code ec;
        auto ep = s_acceptor->local_endpoint(ec);
        if (!ec) {
            return ep.port();
        }
    }
    return 0;
}

void PollRed() {
    if (s_io_context) {
        if (s_io_context->stopped()) {
            s_io_context->restart();
        }
        s_io_context->poll();
    }
}

void IniciarLectura(std::int16_t user_index) {
    // Port de wskapiAO.bas líneas 255-275 (FD_READ / recv con buffer SIZE_RCVBUF)
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= s_client_sockets.size()) {
        return;
    }

    auto& socket_ptr = s_client_sockets[user_index];
    if (!socket_ptr || !socket_ptr->is_open()) {
        return;
    }

    if (static_cast<std::size_t>(user_index) >= s_client_read_buffers.size()) {
        s_client_read_buffers.resize(s_client_sockets.size());
    }

    auto& buf = s_client_read_buffers[user_index];

    socket_ptr->async_read_some(
        asio::buffer(buf),
        [user_index](const asio::error_code& ec, std::size_t bytes_transferred) {
            HandleRead(user_index, ec, bytes_transferred);
        }
    );
}

void SetPacketHandler(PacketHandler handler) {
    s_packet_handler = std::move(handler);
}

void SetSendDataHook(SendDataHook hook) {
    s_send_data_hook = std::move(hook);
}

void EnviarDatosASlot(std::int16_t user_index, std::string_view datos) {
    // Port de TCP.bas:822-843 y wskapiAO.bas:314-352 (WsApiEnviar)
    if (datos.empty()) {
        return;
    }

    if (s_send_data_hook) {
        s_send_data_hook(user_index, datos);
    }

    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size() ||
        static_cast<std::size_t>(user_index) >= s_client_sockets.size()) {
        return;
    }

    auto& user = UserList[user_index];
    auto& sock = s_client_sockets[user_index];

    if (!user.ConnIDValida || user.ConnID == -1 || !sock || !sock->is_open()) {
        return;
    }

    if (static_cast<std::size_t>(user_index) >= s_client_write_queues.size()) {
        s_client_write_queues.resize(s_client_sockets.size());
    }

    auto& out_q = s_client_write_queues[user_index];

    // Mitigación Bug #19 (Safe Backpressure):
    // Si la acumulación saliente excede MAX_OUTGOING_BUFFER_SIZE (64 KB), desconectamos
    // inmediatamente preservando la integridad del estado y el Anti-CombatLog.
    if (out_q.queued_bytes + datos.size() > MAX_OUTGOING_BUFFER_SIZE) {
        if (user.flags.UserLogged) {
            CloseSocketSL(user_index);
            CerrarUsuario(user_index);
        } else {
            CloseSocket(user_index);
        }
        return;
    }

    out_q.queue.emplace_back(datos.begin(), datos.end());
    out_q.queued_bytes += datos.size();

    if (!out_q.is_writing) {
        out_q.is_writing = true;
        DoWrite(user_index);
    }
}

void FlushBuffer(std::int16_t user_index) {
    // Port de Protocol.bas líneas 17233-17249:
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    if (!user.outgoingData) {
        return;
    }

    std::int32_t len = user.outgoingData->length();
    if (len <= 0) {
        return;
    }

    std::string sndData = user.outgoingData->ReadASCIIStringFixed(len);
    EnviarDatosASlot(user_index, sndData);
}

void SetCloseUserHandler(CloseUserHandler handler) {
    s_close_user_handler = std::move(handler);
}

void CloseSocketSL(std::int16_t user_index) {
    // Port de TCP.bas líneas 782-795 y wskapiAO.bas líneas 296-301:
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    if (user.ConnID != -1) {
        BorraSlotSock(static_cast<std::uint64_t>(user.ConnID));
        user.ConnID = -1;
    }
    user.ConnIDValida = false;

    if (static_cast<std::size_t>(user_index) < s_client_sockets.size()) {
        auto& sock = s_client_sockets[user_index];
        if (sock) {
            asio::error_code ec;
            sock->cancel(ec);
            sock->close(ec);
            sock.reset();
        }
    }

    if (static_cast<std::size_t>(user_index) < s_client_write_queues.size()) {
        s_client_write_queues[user_index].queue.clear();
        s_client_write_queues[user_index].queued_bytes = 0;
        s_client_write_queues[user_index].is_writing = false;
    }
}

void CerrarUsuario(std::int16_t user_index) {
    // Port de Modulo_UsUaRiOs.bas líneas 1850-1897:
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    if (!user.flags.UserLogged || user.Counters.Saliendo) {
        return;
    }

    user.Counters.Saliendo = true;

    bool is_pk = false;
    if (user.Pos.Map >= 1 && static_cast<std::size_t>(user.Pos.Map) < MapInfoList.size()) {
        is_pk = MapInfoList[user.Pos.Map].Pk;
    }

    const bool is_regular_user = (user.flags.Privilegios & PlayerType::UserPlayer) != 0;
    if (is_regular_user && is_pk) {
        user.Counters.Salir = (IntervaloCerrarConexion > 0) ? IntervaloCerrarConexion : 10;
    } else {
        user.Counters.Salir = 0;
    }

    // Cancelación inmediata de invisibilidad y ocultamiento (anti-exploit táctico)
    user.flags.invisible = 0;
    user.flags.Oculto = 0;
}

void CloseSocket(std::int16_t user_index) {
    // Port de TCP.bas líneas 610-672:
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];

    // Cierre del socket físico Asio si aún no fue cerrado
    CloseSocketSL(user_index);

    // Ajuste de LastUser si el usuario desconectado estaba en el tope
    if (user_index == LastUser) {
        LastUser--;
        while (LastUser > 0 && (static_cast<std::size_t>(LastUser) >= UserList.size() || !UserList[LastUser].flags.UserLogged)) {
            LastUser--;
        }
    }

    // Vaciar buffers entrantes
    if (user.incomingData) {
        user.incomingData->ReadASCIIStringFixed(user.incomingData->length());
    }

    if (user.flags.UserLogged) {
        if (NumUsers > 0) {
            NumUsers--;
        }

        // Callback desacoplado para CloseUser / persistencia en disco
        if (s_close_user_handler) {
            s_close_user_handler(user_index);
        }
    }

    ResetUserSlot(user_index);
}

void PasarSegundoUsuarios() {
    // Port de General.bas líneas 1555-1569:
    const std::int16_t max_limit = std::min<std::int16_t>(MaxUsers, static_cast<std::int16_t>(UserList.size() - 1));
    for (std::int16_t i = 1; i <= max_limit; ++i) {
        auto& user = UserList[i];
        if (user.flags.UserLogged && user.Counters.Saliendo) {
            user.Counters.Salir--;
            if (user.Counters.Salir <= 0) {
                CloseSocket(i);
            }
        }
    }
}

void WSApiReiniciarSockets() {
    // Port transliterado de wskapiAO.bas líneas 542-582:
    const std::uint16_t puerto = PuertoEscucha();
    const std::string bound_ip = s_bound_ip;
    auto packet_handler = s_packet_handler;
    auto close_handler = s_close_user_handler;

    const std::int16_t max_limit = std::min<std::int16_t>(MaxUsers, static_cast<std::int16_t>(UserList.size() - 1));
    for (std::int16_t i = 1; i <= max_limit; ++i) {
        if (UserList[i].ConnID != -1 || UserList[i].ConnIDValida || UserList[i].flags.UserLogged) {
            CloseSocket(i);
        }
    }

    DetenerServidor();

    if (puerto > 0) {
        IniciaServidor(puerto, bound_ip);
        s_packet_handler = std::move(packet_handler);
        s_close_user_handler = std::move(close_handler);
    }
}

} // namespace TCP
