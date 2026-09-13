#include <doctest/doctest.h>
#include "server/modSendData.hpp"
#include "server/Declares.hpp"
#include "server/TCP.hpp"

#include <vector>
#include <string>
#include <utility>

using namespace ao::net::send_data;

namespace {

struct SentMessage {
    std::int16_t slot{0};
    std::string data;
};

struct TestContext {
    std::vector<SentMessage> sent_messages;

    TestContext() {
        // Blanquear estado global
        NumMaps = 5;
        LastUser = 10;
        UserList.clear();
        UserList.resize(20);
        Npclist.fill(npc{});
        ConnGroups.assign(6, ConnGroup{});

        for (auto& cg : ConnGroups) {
            cg.UserEntrys.assign(20, 0);
        }

        TCP::SetSendDataHook([this](std::int16_t slot, std::string_view data) {
            sent_messages.push_back({slot, std::string(data)});
        });
    }

    ~TestContext() {
        TCP::SetSendDataHook(nullptr);
    }

    [[nodiscard]] bool received_by(std::int16_t slot) const {
        for (const auto& msg : sent_messages) {
            if (msg.slot == slot) {
                return true;
            }
        }
        return false;
    }

    [[nodiscard]] std::size_t count_received_by(std::int16_t slot) const {
        std::size_t count = 0;
        for (const auto& msg : sent_messages) {
            if (msg.slot == slot) {
                ++count;
            }
        }
        return count;
    }
};

} // namespace

TEST_SUITE("modSendData - Pureza Matemática de Bitmasks (Paso 1)") {

    TEST_CASE("AreaPerteneceMask calcula 1 << (pos / 9)") {
        CHECK(AreaPerteneceMask(0) == 1);
        CHECK(AreaPerteneceMask(1) == 1);
        CHECK(AreaPerteneceMask(8) == 1);
        CHECK(AreaPerteneceMask(9) == 2);
        CHECK(AreaPerteneceMask(10) == 2);
        CHECK(AreaPerteneceMask(17) == 2);
        CHECK(AreaPerteneceMask(18) == 4);
        CHECK(AreaPerteneceMask(50) == 32);     // 50 / 9 = 5 -> 1 << 5 = 32
        CHECK(AreaPerteneceMask(99) == 2048);   // 99 / 9 = 11 -> 1 << 11 = 2048
        CHECK(AreaPerteneceMask(100) == 2048);  // 100 / 9 = 11 -> 2048
    }

    TEST_CASE("AreaReciveMask genera cono de 3 áreas adyacentes") {
        // Área 0 (borde izquierdo/superior): área 0 y 1
        CHECK(AreaReciveMask(0) == (1 | 2)); // 3

        // Área 1: áreas 0, 1 y 2
        CHECK(AreaReciveMask(1) == (1 | 2 | 4)); // 7

        // Área 5 (central): áreas 4, 5 y 6
        CHECK(AreaReciveMask(5) == ((1 << 4) | (1 << 5) | (1 << 6))); // 16 + 32 + 64 = 112

        // Área 11 (borde derecho/inferior): áreas 10 y 11
        CHECK(AreaReciveMask(11) == ((1 << 10) | (1 << 11))); // 1024 + 2048 = 3072
    }

    TEST_CASE("IsValidMapIndex valida cotas seguras") {
        NumMaps = 10;
        CHECK_FALSE(IsValidMapIndex(0));
        CHECK_FALSE(IsValidMapIndex(-1));
        CHECK(IsValidMapIndex(1));
        CHECK(IsValidMapIndex(5));
        CHECK(IsValidMapIndex(10));
        CHECK_FALSE(IsValidMapIndex(11));
    }
}

TEST_SUITE("modSendData - Ruteo Geográfico de Áreas y Mapas (Paso 2)") {

    TEST_CASE("SendToUserArea difunde a usuarios en la misma área o área adyacente") {
        TestContext ctx;

        // Usuario 1: emisor en Mapa 1, coord (10, 10) -> Area 1
        UserList[1].Pos = {1, 10, 10};
        UserList[1].AreasInfo.AreaPerteneceX = AreaPerteneceMask(10);
        UserList[1].AreasInfo.AreaPerteneceY = AreaPerteneceMask(10);
        UserList[1].AreasInfo.AreaReciveX = AreaReciveMask(1);
        UserList[1].AreasInfo.AreaReciveY = AreaReciveMask(1);
        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;
        UserList[1].flags.UserLogged = true;

        // Usuario 2: receptor en misma área en Mapa 1
        UserList[2].Pos = {1, 12, 12};
        UserList[2].AreasInfo.AreaPerteneceX = AreaPerteneceMask(12);
        UserList[2].AreasInfo.AreaPerteneceY = AreaPerteneceMask(12);
        UserList[2].AreasInfo.AreaReciveX = AreaReciveMask(1);
        UserList[2].AreasInfo.AreaReciveY = AreaReciveMask(1);
        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;
        UserList[2].flags.UserLogged = true;

        // Usuario 3: receptor lejano en Mapa 1 -> Area 10
        UserList[3].Pos = {1, 95, 95};
        UserList[3].AreasInfo.AreaPerteneceX = AreaPerteneceMask(95);
        UserList[3].AreasInfo.AreaPerteneceY = AreaPerteneceMask(95);
        UserList[3].AreasInfo.AreaReciveX = AreaReciveMask(10);
        UserList[3].AreasInfo.AreaReciveY = AreaReciveMask(10);
        UserList[3].ConnID = 3;
        UserList[3].ConnIDValida = true;
        UserList[3].flags.UserLogged = true;

        // Usuario 4: en otro mapa (Mapa 2)
        UserList[4].Pos = {2, 10, 10};
        UserList[4].AreasInfo.AreaPerteneceX = AreaPerteneceMask(10);
        UserList[4].AreasInfo.AreaPerteneceY = AreaPerteneceMask(10);
        UserList[4].AreasInfo.AreaReciveX = AreaReciveMask(1);
        UserList[4].AreasInfo.AreaReciveY = AreaReciveMask(1);
        UserList[4].ConnID = 4;
        UserList[4].ConnIDValida = true;
        UserList[4].flags.UserLogged = true;

        ConnGroups[1].CountEntrys = 3;
        ConnGroups[1].UserEntrys[1] = 1;
        ConnGroups[1].UserEntrys[2] = 2;
        ConnGroups[1].UserEntrys[3] = 3;

        ConnGroups[2].CountEntrys = 1;
        ConnGroups[2].UserEntrys[1] = 4;

        const std::string payload = "HOLA_AREA";
        SendToUserArea(1, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()));

        CHECK(ctx.received_by(1));
        CHECK(ctx.received_by(2));
        CHECK_FALSE(ctx.received_by(3));
        CHECK_FALSE(ctx.received_by(4));
    }

    TEST_CASE("SendToUserAreaButIndex excluye al propio emisor") {
        TestContext ctx;

        UserList[1].Pos = {1, 10, 10};
        UserList[1].AreasInfo.AreaPerteneceX = AreaPerteneceMask(10);
        UserList[1].AreasInfo.AreaPerteneceY = AreaPerteneceMask(10);
        UserList[1].AreasInfo.AreaReciveX = AreaReciveMask(1);
        UserList[1].AreasInfo.AreaReciveY = AreaReciveMask(1);
        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;
        UserList[1].flags.UserLogged = true;

        UserList[2].Pos = {1, 10, 10};
        UserList[2].AreasInfo.AreaPerteneceX = AreaPerteneceMask(10);
        UserList[2].AreasInfo.AreaPerteneceY = AreaPerteneceMask(10);
        UserList[2].AreasInfo.AreaReciveX = AreaReciveMask(1);
        UserList[2].AreasInfo.AreaReciveY = AreaReciveMask(1);
        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;
        UserList[2].flags.UserLogged = true;

        ConnGroups[1].CountEntrys = 2;
        ConnGroups[1].UserEntrys[1] = 1;
        ConnGroups[1].UserEntrys[2] = 2;

        const std::string payload = "TEXT_TEST";
        SendToUserAreaButIndex(1, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()));

        CHECK_FALSE(ctx.received_by(1)); // Excluido
        CHECK(ctx.received_by(2));       // Recibe
    }

    TEST_CASE("SendToMap y SendToMapButIndex") {
        TestContext ctx;

        for (int i = 1; i <= 3; ++i) {
            UserList[i].Pos = {1, 10, 10};
            UserList[i].ConnID = i;
            UserList[i].ConnIDValida = true;
            UserList[i].flags.UserLogged = true;
        }

        ConnGroups[1].CountEntrys = 3;
        ConnGroups[1].UserEntrys[1] = 1;
        ConnGroups[1].UserEntrys[2] = 2;
        ConnGroups[1].UserEntrys[3] = 3;

        const std::string p1 = "MAP_MSG";
        SendToMap(1, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p1.data()), p1.size()));

        CHECK(ctx.received_by(1));
        CHECK(ctx.received_by(2));
        CHECK(ctx.received_by(3));

        ctx.sent_messages.clear();
        SendToMapButIndex(2, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p1.data()), p1.size()));

        CHECK(ctx.received_by(1));
        CHECK_FALSE(ctx.received_by(2)); // Excluido
        CHECK(ctx.received_by(3));
    }
}

TEST_SUITE("modSendData - Mitigación del Bug #22 y Canales Globales (Paso 4)") {

    TEST_CASE("Canales globales exigen UserLogged == true mitigando Bug #22") {
        TestContext ctx;
        LastUser = 3;

        // Usuario 1: Conectado y Autenticado (Logged = true)
        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;
        UserList[1].flags.UserLogged = true;
        UserList[1].Reputacion.NobleRep = 100; // Ciudadano

        // Usuario 2: Conexión abierta pero en pantalla de Login (Logged = false)
        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;
        UserList[2].flags.UserLogged = false;
        UserList[2].Reputacion.NobleRep = 100; // Ciudadano residual

        // Usuario 3: Desconectado
        UserList[3].ConnID = -1;
        UserList[3].ConnIDValida = false;
        UserList[3].flags.UserLogged = false;

        const std::string p = "GLOBAL_CIUDADANOS";
        SendToCiudadanos(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p.data()), p.size()));

        CHECK(ctx.received_by(1));
        CHECK_FALSE(ctx.received_by(2)); // Bloqueado por UserLogged == false (Bug #22)
        CHECK_FALSE(ctx.received_by(3));
    }

    TEST_CASE("Canales de staff filtran correctamente por privilegios y UserLogged") {
        TestContext ctx;
        LastUser = 4;

        // Admin logueado
        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;
        UserList[1].flags.UserLogged = true;
        UserList[1].flags.Privilegios = PlayerType::Admin;

        // Admin NO logueado (en login)
        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;
        UserList[2].flags.UserLogged = false;
        UserList[2].flags.Privilegios = PlayerType::Admin;

        // Usuario mortal normal logueado
        UserList[3].ConnID = 3;
        UserList[3].ConnIDValida = true;
        UserList[3].flags.UserLogged = true;
        UserList[3].flags.Privilegios = PlayerType::UserPlayer;

        const std::string p = "ADMIN_MSG";
        SendToAdmins(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p.data()), p.size()));

        CHECK(ctx.received_by(1));
        CHECK_FALSE(ctx.received_by(2)); // Mitigación Bug #22
        CHECK_FALSE(ctx.received_by(3)); // No es staff
    }
}

TEST_SUITE("modSendData - Ruteo Social y de Clanes (Paso 3)") {

    TEST_CASE("SendToUserGuildArea filtra por GuildIndex y permite Dios sin RM") {
        TestContext ctx;

        // Emisor: Miembro Clan 1
        UserList[1].Pos = {1, 10, 10};
        UserList[1].AreasInfo.AreaPerteneceX = AreaPerteneceMask(10);
        UserList[1].AreasInfo.AreaPerteneceY = AreaPerteneceMask(10);
        UserList[1].AreasInfo.AreaReciveX = AreaReciveMask(1);
        UserList[1].AreasInfo.AreaReciveY = AreaReciveMask(1);
        UserList[1].GuildIndex = 1;
        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;
        UserList[1].flags.UserLogged = true;

        // Receptor 1: Mismo clan
        UserList[2].Pos = {1, 10, 10};
        UserList[2].AreasInfo.AreaPerteneceX = AreaPerteneceMask(10);
        UserList[2].AreasInfo.AreaPerteneceY = AreaPerteneceMask(10);
        UserList[2].AreasInfo.AreaReciveX = AreaReciveMask(1);
        UserList[2].AreasInfo.AreaReciveY = AreaReciveMask(1);
        UserList[2].GuildIndex = 1;
        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;
        UserList[2].flags.UserLogged = true;

        // Receptor 2: Clan diferente
        UserList[3].Pos = {1, 10, 10};
        UserList[3].AreasInfo.AreaPerteneceX = AreaPerteneceMask(10);
        UserList[3].AreasInfo.AreaPerteneceY = AreaPerteneceMask(10);
        UserList[3].AreasInfo.AreaReciveX = AreaReciveMask(1);
        UserList[3].AreasInfo.AreaReciveY = AreaReciveMask(1);
        UserList[3].GuildIndex = 2;
        UserList[3].ConnID = 3;
        UserList[3].ConnIDValida = true;
        UserList[3].flags.UserLogged = true;

        // Receptor 3: GM Dios espía (sin RM) en clan diferente
        UserList[4].Pos = {1, 10, 10};
        UserList[4].AreasInfo.AreaPerteneceX = AreaPerteneceMask(10);
        UserList[4].AreasInfo.AreaPerteneceY = AreaPerteneceMask(10);
        UserList[4].AreasInfo.AreaReciveX = AreaReciveMask(1);
        UserList[4].AreasInfo.AreaReciveY = AreaReciveMask(1);
        UserList[4].GuildIndex = 99;
        UserList[4].ConnID = 4;
        UserList[4].ConnIDValida = true;
        UserList[4].flags.UserLogged = true;
        UserList[4].flags.Privilegios = PlayerType::Dios;

        ConnGroups[1].CountEntrys = 4;
        ConnGroups[1].UserEntrys[1] = 1;
        ConnGroups[1].UserEntrys[2] = 2;
        ConnGroups[1].UserEntrys[3] = 3;
        ConnGroups[1].UserEntrys[4] = 4;

        const std::string msg = "CLAN_CHAT";
        SendToUserGuildArea(1, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(msg.data()), msg.size()));

        CHECK(ctx.received_by(1));
        CHECK(ctx.received_by(2));
        CHECK_FALSE(ctx.received_by(3));
        CHECK(ctx.received_by(4)); // Dios espía recibe
    }

    TEST_CASE("SendToUserPartyArea") {
        TestContext ctx;

        // Emisor Party 5
        UserList[1].Pos = {1, 10, 10};
        UserList[1].AreasInfo.AreaPerteneceX = AreaPerteneceMask(10);
        UserList[1].AreasInfo.AreaPerteneceY = AreaPerteneceMask(10);
        UserList[1].AreasInfo.AreaReciveX = AreaReciveMask(1);
        UserList[1].AreasInfo.AreaReciveY = AreaReciveMask(1);
        UserList[1].PartyIndex = 5;
        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;

        // Compañero Party 5
        UserList[2].Pos = {1, 10, 10};
        UserList[2].AreasInfo.AreaPerteneceX = AreaPerteneceMask(10);
        UserList[2].AreasInfo.AreaPerteneceY = AreaPerteneceMask(10);
        UserList[2].AreasInfo.AreaReciveX = AreaReciveMask(1);
        UserList[2].AreasInfo.AreaReciveY = AreaReciveMask(1);
        UserList[2].PartyIndex = 5;
        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;

        // Otra Party 6
        UserList[3].Pos = {1, 10, 10};
        UserList[3].AreasInfo.AreaPerteneceX = AreaPerteneceMask(10);
        UserList[3].AreasInfo.AreaPerteneceY = AreaPerteneceMask(10);
        UserList[3].AreasInfo.AreaReciveX = AreaReciveMask(1);
        UserList[3].AreasInfo.AreaReciveY = AreaReciveMask(1);
        UserList[3].PartyIndex = 6;
        UserList[3].ConnID = 3;
        UserList[3].ConnIDValida = true;

        ConnGroups[1].CountEntrys = 3;
        ConnGroups[1].UserEntrys[1] = 1;
        ConnGroups[1].UserEntrys[2] = 2;
        ConnGroups[1].UserEntrys[3] = 3;

        const std::string msg = "PARTY_CHAT";
        SendToUserPartyArea(1, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(msg.data()), msg.size()));

        CHECK(ctx.received_by(1));
        CHECK(ctx.received_by(2));
        CHECK_FALSE(ctx.received_by(3));
    }
}

TEST_SUITE("modSendData - Dispatcher Maestro y AlertarFaccionarios (Paso 5)") {

    TEST_CASE("SendData despacha correctamente según SendTarget") {
        TestContext ctx;
        LastUser = 2;

        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;
        UserList[1].flags.UserLogged = true;

        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;
        UserList[2].flags.UserLogged = true;

        SendData(SendTarget::ToAll, 0, "TEST_DISPATCH");
        CHECK(ctx.received_by(1));
        CHECK(ctx.received_by(2));

        ctx.sent_messages.clear();
        SendData(SendTarget::ToAllButIndex, 1, "TEST_EXCLUDE");
        CHECK_FALSE(ctx.received_by(1));
        CHECK(ctx.received_by(2));
    }

    TEST_CASE("AlertarFaccionarios calcula orientación cardinal y notifica a la misma facción") {
        TestContext ctx;

        // Emisor Caos en (50, 50)
        UserList[1].Pos = {1, 50, 50};
        UserList[1].Faccion.FuerzasCaos = 1;
        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;

        // Receptor Caos en (50, 60) -> Emisor está al Norte respecto al Receptor
        UserList[2].Pos = {1, 50, 60};
        UserList[2].Faccion.FuerzasCaos = 1;
        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;

        // Receptor Armada Real en (50, 60) -> Facción contraria
        UserList[3].Pos = {1, 50, 60};
        UserList[3].Faccion.ArmadaReal = 1;
        UserList[3].ConnID = 3;
        UserList[3].ConnIDValida = true;

        ConnGroups[1].CountEntrys = 3;
        ConnGroups[1].UserEntrys[1] = 1;
        ConnGroups[1].UserEntrys[2] = 2;
        ConnGroups[1].UserEntrys[3] = 3;

        AlertarFaccionarios(1);

        CHECK_FALSE(ctx.received_by(1)); // El emisor no se notifica a sí mismo
        CHECK(ctx.received_by(2));       // Mismo bando recibe
        CHECK_FALSE(ctx.received_by(3)); // Bando contrario no recibe

        // Verificar orientación cardinal en el texto
        REQUIRE(ctx.sent_messages.size() == 1);
        CHECK(ctx.sent_messages[0].data.find("Norte") != std::string::npos);
    }
}
