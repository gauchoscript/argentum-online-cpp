#include <doctest/doctest.h>
#include "server/mdlCOmercioConUsuario.hpp"
#include "server/InvUsuario.hpp"
#include "server/Modulo_InventANDobj.hpp"
#include "server/Declares.hpp"
#include "server/Protocol.hpp"
#include "server/clsByteQueue.hpp"
#include <vector>
#include <string>
#include <memory>
#include <cstdint>

#include "server/TCP.hpp"
#include <map>

namespace {

using namespace ao;
using namespace ao::net::protocol;

struct TradeUserTestEnv {
    std::map<std::int16_t, clsByteQueue> flushed_buffers;

    TradeUserTestEnv() {
        MaxUsers = 10;
        LastUser = 10;
        UserList.clear();
        UserList.resize(15);
        ObjDataList.assign(100, ObjData{});
        NumObjDatas = 99;

        InvUsuario::SetUpdateUserInvHook(nullptr);

        TCP::SetSendDataHook([this](std::int16_t slot, std::string_view data) {
            flushed_buffers[slot].WriteASCIIStringFixed(std::string(data));
        });

        ObjDataList[1].name = "Poción Roja";
        ObjDataList[1].Valor = 100;

        ObjDataList[2].name = "Espada Larga";
        ObjDataList[2].Valor = 500;

        for (std::int16_t u = 1; u <= 5; ++u) {
            UserList[u].outgoingData = std::make_unique<clsByteQueue>();
            UserList[u].flags.UserLogged = true;
            UserList[u].flags.Comerciando = false;
            UserList[u].flags.Muerto = 0;
            UserList[u].flags.TargetUser = 0;
            UserList[u].Stats.GLD = 100000;
            UserList[u].CurrentInventorySlots = 30;
            UserList[u].name = "Heroe" + std::to_string(u);
            UserList[u].ComUsu = tCOmercioUsuario{};
        }
    }

    ~TradeUserTestEnv() {
        TCP::SetSendDataHook(nullptr);
    }
};

} // namespace

TEST_SUITE("Comercio P2P - G2") {

    TEST_CASE("IniciarComercioConUsuario - Primer pedido invita al destinatario") {
        TradeUserTestEnv env;

        // Usuario 1 intenta comerciar con Usuario 2 (primer paso)
        UserList[1].ComUsu.DestUsu = 2;
        UserList[1].ComUsu.DestNick = UserList[2].name;

        IniciarComercioConUsuario(1, 2);

        // No se abre la ventana todavía
        CHECK_FALSE(UserList[1].flags.Comerciando);
        CHECK_FALSE(UserList[2].flags.Comerciando);
        // El destinatario debe tener como TargetUser al usuario 1
        CHECK(UserList[2].flags.TargetUser == 1);

        // Se envió el mensaje de consola al usuario 2 mediante FlushBuffer
        auto& q = env.flushed_buffers[2];
        CHECK(q.length() > 0);
        auto opcode = q.ReadByte();
        CHECK(opcode == static_cast<std::uint8_t>(ServerPacketID::ConsoleMsg));
    }

    TEST_CASE("IniciarComercioConUsuario - Reciprocidad abre ventana para ambos") {
        TradeUserTestEnv env;

        // Configurar reciprocidad mutua
        UserList[1].ComUsu.DestUsu = 2;
        UserList[1].ComUsu.DestNick = UserList[2].name;

        UserList[2].ComUsu.DestUsu = 1;
        UserList[2].ComUsu.DestNick = UserList[1].name;

        IniciarComercioConUsuario(2, 1);

        // Ambos quedan comerciando
        CHECK(UserList[1].flags.Comerciando);
        CHECK(UserList[2].flags.Comerciando);

        // Verificar paquete UserCommerceInit recibido en ambos sockets (vía FlushBuffer o outgoingData)
        auto has_packet = [](clsByteQueue& q, ServerPacketID target) {
            while (q.length() > 0) {
                if (q.ReadByte() == static_cast<std::uint8_t>(target)) return true;
            }
            return false;
        };

        bool init1 = has_packet(*UserList[1].outgoingData, ServerPacketID::UserCommerceInit) ||
                     has_packet(env.flushed_buffers[1], ServerPacketID::UserCommerceInit);
        bool init2 = has_packet(*UserList[2].outgoingData, ServerPacketID::UserCommerceInit) ||
                     has_packet(env.flushed_buffers[2], ServerPacketID::UserCommerceInit);

        CHECK(init1);
        CHECK(init2);
    }

    TEST_CASE("AgregarOferta - Modificacion de oro e items") {
        TradeUserTestEnv env;

        // Establecer sesion valida entre 1 y 2
        UserList[1].ComUsu.DestUsu = 2;
        UserList[1].ComUsu.DestNick = UserList[2].name;
        UserList[2].ComUsu.DestUsu = 1;
        UserList[2].ComUsu.DestNick = UserList[1].name;

        // 1. Agregar oro
        AgregarOferta(1, GOLD_OFFER_SLOT, 0, 5000, true);
        CHECK(UserList[1].ComUsu.GoldAmount == 5000);

        // 2. Restar oro (sin volverse negativo)
        AgregarOferta(1, GOLD_OFFER_SLOT, 0, -2000, true);
        CHECK(UserList[1].ComUsu.GoldAmount == 3000);

        AgregarOferta(1, GOLD_OFFER_SLOT, 0, -10000, true);
        CHECK(UserList[1].ComUsu.GoldAmount == 0);

        // 3. Agregar item en slot 1
        AgregarOferta(1, 1, 2, 5, false);
        CHECK(UserList[1].ComUsu.Objeto[1] == 2);
        CHECK(UserList[1].ComUsu.cant[1] == 5);

        // 4. Modificar cantidad en slot 1
        AgregarOferta(1, 1, 0, 3, false);
        CHECK(UserList[1].ComUsu.Objeto[1] == 2);
        CHECK(UserList[1].ComUsu.cant[1] == 8);

        // 5. Restar hasta que se remueva el objeto
        AgregarOferta(1, 1, 0, -8, false);
        CHECK(UserList[1].ComUsu.Objeto[1] == 0);
        CHECK(UserList[1].ComUsu.cant[1] == 0);
    }

    TEST_CASE("AgregarOferta - Bloqueo de modificacion cuando Confirmo es true (aborta con FinComerciarUsu)") {
        TradeUserTestEnv env;

        UserList[1].ComUsu.DestUsu = 2;
        UserList[1].ComUsu.DestNick = UserList[2].name;
        UserList[2].ComUsu.DestUsu = 1;
        UserList[2].ComUsu.DestNick = UserList[1].name;
        UserList[1].flags.Comerciando = true;
        UserList[2].flags.Comerciando = true;

        // El usuario 1 confirma su oferta
        UserList[1].ComUsu.Confirmo = true;
        UserList[1].ComUsu.GoldAmount = 1000;

        // Intentar agregar mas oro con oferta confirmada (trampa -> aborta con FinComerciarUsu)
        AgregarOferta(1, GOLD_OFFER_SLOT, 0, 500, true);

        // La sesión debe haberse cancelado para ambos
        CHECK_FALSE(UserList[1].flags.Comerciando);
        CHECK_FALSE(UserList[2].flags.Comerciando);
        CHECK(UserList[1].ComUsu.DestUsu == 0);
        CHECK(UserList[2].ComUsu.DestUsu == 0);
        CHECK(UserList[1].ComUsu.GoldAmount == 0);
    }

    TEST_CASE("EnviarOferta - Despacho de ChangeUserTradeSlot") {
        TradeUserTestEnv env;

        UserList[1].ComUsu.DestUsu = 2;
        UserList[1].ComUsu.DestNick = UserList[2].name;
        UserList[2].ComUsu.DestUsu = 1;
        UserList[2].ComUsu.DestNick = UserList[1].name;

        // Usuario 2 cargo una oferta de 500 de oro y un item en slot 1
        UserList[2].ComUsu.GoldAmount = 500;
        UserList[2].ComUsu.Objeto[1] = 2; // Espada
        UserList[2].ComUsu.cant[1] = 3;

        // Usuario 1 recibe la oferta de oro de Usuario 2
        EnviarOferta(1, GOLD_OFFER_SLOT);

        auto& q = env.flushed_buffers[1];
        CHECK(q.length() > 0);
        auto opcode = q.ReadByte();
        CHECK(opcode == static_cast<std::uint8_t>(ServerPacketID::ChangeUserTradeSlot));
        auto slot = q.ReadByte();
        auto obj = q.ReadInteger();
        auto amount = q.ReadLong();

        CHECK(slot == GOLD_OFFER_SLOT);
        CHECK(obj == iORO);
        CHECK(amount == 500);
    }

    TEST_CASE("PuedeSeguirComerciando - Guardianes de sesion y reset en falla") {
        TradeUserTestEnv env;

        UserList[1].ComUsu.DestUsu = 2;
        UserList[1].ComUsu.DestNick = UserList[2].name;
        UserList[2].ComUsu.DestUsu = 1;
        UserList[2].ComUsu.DestNick = UserList[1].name;
        UserList[1].flags.Comerciando = true;
        UserList[2].flags.Comerciando = true;

        CHECK(PuedeSeguirComerciando(1));

        // 1. Muerte del interlocutor
        UserList[2].flags.Muerto = 1;
        CHECK_FALSE(PuedeSeguirComerciando(1));
        // Debe haber cerrado la sesion
        CHECK_FALSE(UserList[1].flags.Comerciando);
        CHECK(UserList[1].ComUsu.DestUsu == 0);

        // 2. DestNick no coincide
        UserList[1].ComUsu.DestUsu = 2;
        UserList[1].ComUsu.DestNick = "OtroNombre";
        UserList[2].flags.Muerto = 0;
        UserList[2].ComUsu.DestUsu = 1;
        UserList[2].ComUsu.DestNick = UserList[1].name;
        CHECK_FALSE(PuedeSeguirComerciando(1));
    }

    TEST_CASE("FinComerciarUsu - Reseteo integral de estado") {
        TradeUserTestEnv env;

        UserList[1].ComUsu.DestUsu = 2;
        UserList[1].ComUsu.DestNick = UserList[2].name;
        UserList[1].ComUsu.GoldAmount = 50000;
        UserList[1].ComUsu.Objeto[1] = 5;
        UserList[1].ComUsu.cant[1] = 10;
        UserList[1].ComUsu.Acepto = true;
        UserList[1].ComUsu.Confirmo = true;
        UserList[1].flags.Comerciando = true;

        FinComerciarUsu(1);

        CHECK(UserList[1].ComUsu.DestUsu == 0);
        CHECK(UserList[1].ComUsu.DestNick.empty());
        CHECK(UserList[1].ComUsu.GoldAmount == 0);
        CHECK(UserList[1].ComUsu.Objeto[1] == 0);
        CHECK(UserList[1].ComUsu.cant[1] == 0);
        CHECK_FALSE(UserList[1].ComUsu.Acepto);
        CHECK_FALSE(UserList[1].ComUsu.Confirmo);
        CHECK_FALSE(UserList[1].flags.Comerciando);

        // Verifica despacho de UserCommerceEnd
        auto* q = UserList[1].outgoingData.get();
        CHECK(q->length() > 0);
        auto opcode = q->ReadByte();
        CHECK(opcode == static_cast<std::uint8_t>(ServerPacketID::UserCommerceEnd));
    }

}

TEST_SUITE("Comercio P2P - G3") {

    TEST_CASE("AceptarComercioUsu - Espera bilateral hasta confirmacion mutua") {
        TradeUserTestEnv env;

        UserList[1].ComUsu.DestUsu = 2;
        UserList[1].ComUsu.DestNick = UserList[2].name;
        UserList[2].ComUsu.DestUsu = 1;
        UserList[2].ComUsu.DestNick = UserList[1].name;
        UserList[1].flags.Comerciando = true;
        UserList[2].flags.Comerciando = true;

        UserList[1].ComUsu.GoldAmount = 1000;
        UserList[2].ComUsu.GoldAmount = 2000;

        // 1. Usuario 1 pulsa Aceptar -> solo se marca Acepto = true, sin transferir
        AceptarComercioUsu(1);

        CHECK(UserList[1].ComUsu.Acepto);
        CHECK_FALSE(UserList[2].ComUsu.Acepto);
        // Siguen comerciando y con sus saldos inalterados
        CHECK(UserList[1].flags.Comerciando);
        CHECK(UserList[2].flags.Comerciando);
        CHECK(UserList[1].Stats.GLD == 100000);
        CHECK(UserList[2].Stats.GLD == 100000);

        // 2. Usuario 2 pulsa Aceptar -> se ejecuta la transferencia y finaliza el comercio
        AceptarComercioUsu(2);

        CHECK_FALSE(UserList[1].flags.Comerciando);
        CHECK_FALSE(UserList[2].flags.Comerciando);
        // Usuario 1 dio 1000 y recibio 2000 -> 100000 - 1000 + 2000 = 101000
        CHECK(UserList[1].Stats.GLD == 101000);
        // Usuario 2 dio 2000 y recibio 1000 -> 100000 - 2000 + 1000 = 99000
        CHECK(UserList[2].Stats.GLD == 99000);
    }

    TEST_CASE("AceptarComercioUsu - Omicion de MAXORO en transferencia (Bug #36)") {
        TradeUserTestEnv env;

        UserList[1].ComUsu.DestUsu = 2;
        UserList[1].ComUsu.DestNick = UserList[2].name;
        UserList[2].ComUsu.DestUsu = 1;
        UserList[2].ComUsu.DestNick = UserList[1].name;
        UserList[1].flags.Comerciando = true;
        UserList[2].flags.Comerciando = true;

        // Usuario 1 tiene 10.000.000 y ofrece 5.000.000
        UserList[1].Stats.GLD = 10000000;
        UserList[1].ComUsu.GoldAmount = 5000000;

        // Usuario 2 ya posee 89.000.000 de oro (cerca del tope MAXORO de 90M)
        UserList[2].Stats.GLD = 89000000;
        UserList[2].ComUsu.GoldAmount = 0;

        UserList[1].ComUsu.Acepto = true;
        AceptarComercioUsu(2);

        // En estricta paridad con el Bug #36, el oro no se clamplea a MAXORO (90M),
        // sino que se le acreditan directamente los 5M alcanzando 94.000.000 de oro.
        CHECK(UserList[1].Stats.GLD == 5000000);
        CHECK(UserList[2].Stats.GLD == 94000000);
    }

    TEST_CASE("AceptarComercioUsu - Acreditacion previa al debito (Bug #35)") {
        TradeUserTestEnv env;

        UserList[1].ComUsu.DestUsu = 2;
        UserList[1].ComUsu.DestNick = UserList[2].name;
        UserList[2].ComUsu.DestUsu = 1;
        UserList[2].ComUsu.DestNick = UserList[1].name;
        UserList[1].flags.Comerciando = true;
        UserList[2].flags.Comerciando = true;

        // Usuario 1 ofrece ítem 1 (Poción Roja, cant 10)
        UserList[1].ComUsu.Objeto[1] = 1;
        UserList[1].ComUsu.cant[1] = 10;
        UserList[1].Invent.Object[1].ObjIndex = 1;
        UserList[1].Invent.Object[1].Amount = 10;
        UserList[1].Invent.NroItems = 1;

        bool quitado_en_emisor = false;
        bool entregado_antes_de_quitar = false;

        auto quitar_hook = [&](std::int16_t obj, std::int32_t cant, std::int16_t u) {
            quitado_en_emisor = true;
            // Verificar si el receptor ya tiene el objeto acreditado en su inventario
            if (UserList[2].Invent.NroItems > 0 && UserList[2].Invent.Object[1].ObjIndex == 1) {
                entregado_antes_de_quitar = true;
            }
            // Ejecutar sustracción en emisor
            UserList[u].Invent.Object[1] = UserOBJ{};
            UserList[u].Invent.NroItems = 0;
        };

        UserList[1].ComUsu.Acepto = true;
        AceptarComercioUsu(2, quitar_hook, nullptr);

        CHECK(quitado_en_emisor);
        CHECK(entregado_antes_de_quitar);
        CHECK(UserList[2].Invent.Object[1].ObjIndex == 1);
        CHECK(UserList[2].Invent.Object[1].Amount == 10);
    }

    TEST_CASE("AceptarComercioUsu - Evaporacion por piso saturado (Bug #34)") {
        TradeUserTestEnv env;

        UserList[1].ComUsu.DestUsu = 2;
        UserList[1].ComUsu.DestNick = UserList[2].name;
        UserList[2].ComUsu.DestUsu = 1;
        UserList[2].ComUsu.DestNick = UserList[1].name;
        UserList[1].flags.Comerciando = true;
        UserList[2].flags.Comerciando = true;

        // Usuario 1 ofrece espada (ítem 2, cant 1)
        UserList[1].ComUsu.Objeto[1] = 2;
        UserList[1].ComUsu.cant[1] = 1;
        UserList[1].Invent.Object[1].ObjIndex = 2;
        UserList[1].Invent.Object[1].Amount = 1;
        UserList[1].Invent.NroItems = 1;

        // Llenar inventario de Usuario 2 para que falle MeterItemEnInventario
        for (std::uint8_t s = 1; s <= MAX_INVENTORY_SLOTS; ++s) {
            UserList[2].Invent.Object[s].ObjIndex = static_cast<std::int16_t>(10 + s);
            UserList[2].Invent.Object[s].Amount = 1;
        }
        UserList[2].Invent.NroItems = 30;

        bool tirar_piso_llamado = false;
        bool debitado_del_emisor = false;

        auto tirar_piso_saturado = [&](const WorldPos& pos, const Obj& obj) -> WorldPos {
            tirar_piso_llamado = true;
            // Simula suelo saturado (TileLibre retorna 0,0)
            return WorldPos{pos.Map, 0, 0};
        };

        auto quitar_hook = [&](std::int16_t obj, std::int32_t cant, std::int16_t u) {
            debitado_del_emisor = true;
            UserList[u].Invent.Object[1] = UserOBJ{};
            UserList[u].Invent.NroItems = 0;
        };

        UserList[1].ComUsu.Acepto = true;
        AceptarComercioUsu(2, quitar_hook, tirar_piso_saturado);

        // En estricta paridad con el Bug #34:
        // 1. MeterItemEnInventario falló por mochila llena
        // 2. Se intentó tirar al piso pero este devolvió (0,0) saturado
        CHECK(tirar_piso_llamado);
        // 3. El ítem fue debitado igualmente del emisor
        CHECK(debitado_del_emisor);
        CHECK(UserList[1].Invent.NroItems == 0);
        // 4. El receptor no lo recibió (sigue con sus 30 ítems originales)
        CHECK(UserList[2].Invent.NroItems == 30);
        for (std::uint8_t s = 1; s <= MAX_INVENTORY_SLOTS; ++s) {
            CHECK(UserList[2].Invent.Object[s].ObjIndex != 2);
        }
    }

}

