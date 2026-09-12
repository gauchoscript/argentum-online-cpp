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

    TEST_CASE("area_pertenece_mask calcula 1 << (pos / 9)") {
        CHECK(area_pertenece_mask(0) == 1);
        CHECK(area_pertenece_mask(1) == 1);
        CHECK(area_pertenece_mask(8) == 1);
        CHECK(area_pertenece_mask(9) == 2);
        CHECK(area_pertenece_mask(10) == 2);
        CHECK(area_pertenece_mask(17) == 2);
        CHECK(area_pertenece_mask(18) == 4);
        CHECK(area_pertenece_mask(50) == 32);     // 50 / 9 = 5 -> 1 << 5 = 32
        CHECK(area_pertenece_mask(99) == 2048);   // 99 / 9 = 11 -> 1 << 11 = 2048
        CHECK(area_pertenece_mask(100) == 2048);  // 100 / 9 = 11 -> 2048
    }

    TEST_CASE("area_recive_mask genera cono de 3 áreas adyacentes") {
        // Área 0 (borde izquierdo/superior): área 0 y 1
        CHECK(area_recive_mask(0) == (1 | 2)); // 3

        // Área 1: áreas 0, 1 y 2
        CHECK(area_recive_mask(1) == (1 | 2 | 4)); // 7

        // Área 5 (central): áreas 4, 5 y 6
        CHECK(area_recive_mask(5) == ((1 << 4) | (1 << 5) | (1 << 6))); // 16 + 32 + 64 = 112

        // Área 11 (borde derecho/inferior): áreas 10 y 11
        CHECK(area_recive_mask(11) == ((1 << 10) | (1 << 11))); // 1024 + 2048 = 3072
    }

    TEST_CASE("is_valid_map_index valida cotas seguras") {
        NumMaps = 10;
        CHECK_FALSE(is_valid_map_index(0));
        CHECK_FALSE(is_valid_map_index(-1));
        CHECK(is_valid_map_index(1));
        CHECK(is_valid_map_index(5));
        CHECK(is_valid_map_index(10));
        CHECK_FALSE(is_valid_map_index(11));
    }
}

TEST_SUITE("modSendData - Ruteo Geográfico de Áreas y Mapas (Paso 2)") {

    TEST_CASE("send_to_user_area difunde a usuarios en la misma área o área adyacente") {
        TestContext ctx;

        // Usuario 1: emisor en Mapa 1, coord (10, 10) -> Area 1
        UserList[1].Pos = {1, 10, 10};
        UserList[1].AreasInfo.AreaPerteneceX = area_pertenece_mask(10);
        UserList[1].AreasInfo.AreaPerteneceY = area_pertenece_mask(10);
        UserList[1].AreasInfo.AreaReciveX = area_recive_mask(1);
        UserList[1].AreasInfo.AreaReciveY = area_recive_mask(1);
        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;
        UserList[1].flags.UserLogged = true;

        // Usuario 2: receptor en misma área en Mapa 1
        UserList[2].Pos = {1, 12, 12};
        UserList[2].AreasInfo.AreaPerteneceX = area_pertenece_mask(12);
        UserList[2].AreasInfo.AreaPerteneceY = area_pertenece_mask(12);
        UserList[2].AreasInfo.AreaReciveX = area_recive_mask(1);
        UserList[2].AreasInfo.AreaReciveY = area_recive_mask(1);
        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;
        UserList[2].flags.UserLogged = true;

        // Usuario 3: receptor lejano en Mapa 1 -> Area 10
        UserList[3].Pos = {1, 95, 95};
        UserList[3].AreasInfo.AreaPerteneceX = area_pertenece_mask(95);
        UserList[3].AreasInfo.AreaPerteneceY = area_pertenece_mask(95);
        UserList[3].AreasInfo.AreaReciveX = area_recive_mask(10);
        UserList[3].AreasInfo.AreaReciveY = area_recive_mask(10);
        UserList[3].ConnID = 3;
        UserList[3].ConnIDValida = true;
        UserList[3].flags.UserLogged = true;

        // Usuario 4: en otro mapa (Mapa 2)
        UserList[4].Pos = {2, 10, 10};
        UserList[4].AreasInfo.AreaPerteneceX = area_pertenece_mask(10);
        UserList[4].AreasInfo.AreaPerteneceY = area_pertenece_mask(10);
        UserList[4].AreasInfo.AreaReciveX = area_recive_mask(1);
        UserList[4].AreasInfo.AreaReciveY = area_recive_mask(1);
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
        send_to_user_area(1, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()));

        CHECK(ctx.received_by(1));
        CHECK(ctx.received_by(2));
        CHECK_FALSE(ctx.received_by(3));
        CHECK_FALSE(ctx.received_by(4));
    }

    TEST_CASE("send_to_user_area_but_index excluye al propio emisor") {
        TestContext ctx;

        UserList[1].Pos = {1, 10, 10};
        UserList[1].AreasInfo.AreaPerteneceX = area_pertenece_mask(10);
        UserList[1].AreasInfo.AreaPerteneceY = area_pertenece_mask(10);
        UserList[1].AreasInfo.AreaReciveX = area_recive_mask(1);
        UserList[1].AreasInfo.AreaReciveY = area_recive_mask(1);
        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;
        UserList[1].flags.UserLogged = true;

        UserList[2].Pos = {1, 10, 10};
        UserList[2].AreasInfo.AreaPerteneceX = area_pertenece_mask(10);
        UserList[2].AreasInfo.AreaPerteneceY = area_pertenece_mask(10);
        UserList[2].AreasInfo.AreaReciveX = area_recive_mask(1);
        UserList[2].AreasInfo.AreaReciveY = area_recive_mask(1);
        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;
        UserList[2].flags.UserLogged = true;

        ConnGroups[1].CountEntrys = 2;
        ConnGroups[1].UserEntrys[1] = 1;
        ConnGroups[1].UserEntrys[2] = 2;

        const std::string payload = "TEXT_TEST";
        send_to_user_area_but_index(1, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(payload.data()), payload.size()));

        CHECK_FALSE(ctx.received_by(1)); // Excluido
        CHECK(ctx.received_by(2));       // Recibe
    }

    TEST_CASE("send_to_map y send_to_map_but_index") {
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
        send_to_map(1, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p1.data()), p1.size()));

        CHECK(ctx.received_by(1));
        CHECK(ctx.received_by(2));
        CHECK(ctx.received_by(3));

        ctx.sent_messages.clear();
        send_to_map_but_index(2, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p1.data()), p1.size()));

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
        send_to_ciudadanos(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p.data()), p.size()));

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
        send_to_admins(std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(p.data()), p.size()));

        CHECK(ctx.received_by(1));
        CHECK_FALSE(ctx.received_by(2)); // Mitigación Bug #22
        CHECK_FALSE(ctx.received_by(3)); // No es staff
    }
}

TEST_SUITE("modSendData - Ruteo Social y de Clanes (Paso 3)") {

    TEST_CASE("send_to_user_guild_area filtra por GuildIndex y permite Dios sin RM") {
        TestContext ctx;

        // Emisor: Miembro Clan 1
        UserList[1].Pos = {1, 10, 10};
        UserList[1].AreasInfo.AreaPerteneceX = area_pertenece_mask(10);
        UserList[1].AreasInfo.AreaPerteneceY = area_pertenece_mask(10);
        UserList[1].AreasInfo.AreaReciveX = area_recive_mask(1);
        UserList[1].AreasInfo.AreaReciveY = area_recive_mask(1);
        UserList[1].GuildIndex = 1;
        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;
        UserList[1].flags.UserLogged = true;

        // Receptor 1: Mismo clan
        UserList[2].Pos = {1, 10, 10};
        UserList[2].AreasInfo.AreaPerteneceX = area_pertenece_mask(10);
        UserList[2].AreasInfo.AreaPerteneceY = area_pertenece_mask(10);
        UserList[2].AreasInfo.AreaReciveX = area_recive_mask(1);
        UserList[2].AreasInfo.AreaReciveY = area_recive_mask(1);
        UserList[2].GuildIndex = 1;
        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;
        UserList[2].flags.UserLogged = true;

        // Receptor 2: Clan diferente
        UserList[3].Pos = {1, 10, 10};
        UserList[3].AreasInfo.AreaPerteneceX = area_pertenece_mask(10);
        UserList[3].AreasInfo.AreaPerteneceY = area_pertenece_mask(10);
        UserList[3].AreasInfo.AreaReciveX = area_recive_mask(1);
        UserList[3].AreasInfo.AreaReciveY = area_recive_mask(1);
        UserList[3].GuildIndex = 2;
        UserList[3].ConnID = 3;
        UserList[3].ConnIDValida = true;
        UserList[3].flags.UserLogged = true;

        // Receptor 3: GM Dios espía (sin RM) en clan diferente
        UserList[4].Pos = {1, 10, 10};
        UserList[4].AreasInfo.AreaPerteneceX = area_pertenece_mask(10);
        UserList[4].AreasInfo.AreaPerteneceY = area_pertenece_mask(10);
        UserList[4].AreasInfo.AreaReciveX = area_recive_mask(1);
        UserList[4].AreasInfo.AreaReciveY = area_recive_mask(1);
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
        send_to_user_guild_area(1, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(msg.data()), msg.size()));

        CHECK(ctx.received_by(1));
        CHECK(ctx.received_by(2));
        CHECK_FALSE(ctx.received_by(3));
        CHECK(ctx.received_by(4)); // Dios espía recibe
    }

    TEST_CASE("send_to_user_party_area") {
        TestContext ctx;

        // Emisor Party 5
        UserList[1].Pos = {1, 10, 10};
        UserList[1].AreasInfo.AreaPerteneceX = area_pertenece_mask(10);
        UserList[1].AreasInfo.AreaPerteneceY = area_pertenece_mask(10);
        UserList[1].AreasInfo.AreaReciveX = area_recive_mask(1);
        UserList[1].AreasInfo.AreaReciveY = area_recive_mask(1);
        UserList[1].PartyIndex = 5;
        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;

        // Compañero Party 5
        UserList[2].Pos = {1, 10, 10};
        UserList[2].AreasInfo.AreaPerteneceX = area_pertenece_mask(10);
        UserList[2].AreasInfo.AreaPerteneceY = area_pertenece_mask(10);
        UserList[2].AreasInfo.AreaReciveX = area_recive_mask(1);
        UserList[2].AreasInfo.AreaReciveY = area_recive_mask(1);
        UserList[2].PartyIndex = 5;
        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;

        // Otra Party 6
        UserList[3].Pos = {1, 10, 10};
        UserList[3].AreasInfo.AreaPerteneceX = area_pertenece_mask(10);
        UserList[3].AreasInfo.AreaPerteneceY = area_pertenece_mask(10);
        UserList[3].AreasInfo.AreaReciveX = area_recive_mask(1);
        UserList[3].AreasInfo.AreaReciveY = area_recive_mask(1);
        UserList[3].PartyIndex = 6;
        UserList[3].ConnID = 3;
        UserList[3].ConnIDValida = true;

        ConnGroups[1].CountEntrys = 3;
        ConnGroups[1].UserEntrys[1] = 1;
        ConnGroups[1].UserEntrys[2] = 2;
        ConnGroups[1].UserEntrys[3] = 3;

        const std::string msg = "PARTY_CHAT";
        send_to_user_party_area(1, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(msg.data()), msg.size()));

        CHECK(ctx.received_by(1));
        CHECK(ctx.received_by(2));
        CHECK_FALSE(ctx.received_by(3));
    }
}

TEST_SUITE("modSendData - Dispatcher Maestro y AlertarFaccionarios (Paso 5)") {

    TEST_CASE("send_data despacha correctamente según SendTarget") {
        TestContext ctx;
        LastUser = 2;

        UserList[1].ConnID = 1;
        UserList[1].ConnIDValida = true;
        UserList[1].flags.UserLogged = true;

        UserList[2].ConnID = 2;
        UserList[2].ConnIDValida = true;
        UserList[2].flags.UserLogged = true;

        send_data(SendTarget::ToAll, 0, "TEST_DISPATCH");
        CHECK(ctx.received_by(1));
        CHECK(ctx.received_by(2));

        ctx.sent_messages.clear();
        send_data(SendTarget::ToAllButIndex, 1, "TEST_EXCLUDE");
        CHECK_FALSE(ctx.received_by(1));
        CHECK(ctx.received_by(2));
    }

    TEST_CASE("alertar_faccionarios calcula orientación cardinal y notifica a la misma facción") {
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

        alertar_faccionarios(1);

        CHECK_FALSE(ctx.received_by(1)); // El emisor no se notifica a sí mismo
        CHECK(ctx.received_by(2));       // Mismo bando recibe
        CHECK_FALSE(ctx.received_by(3)); // Bando contrario no recibe

        // Verificar orientación cardinal en el texto
        REQUIRE(ctx.sent_messages.size() == 1);
        CHECK(ctx.sent_messages[0].data.find("Norte") != std::string::npos);
    }
}
