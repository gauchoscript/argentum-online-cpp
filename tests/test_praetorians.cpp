#include <doctest/doctest.h>
#include <string>
#include "server/praetorians.hpp"
#include "server/Declares.hpp"

namespace {
inline std::size_t mapBlockIndex(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return (static_cast<std::size_t>(map) * 101 + static_cast<std::size_t>(x)) * 101 + static_cast<std::size_t>(y);
}
}

TEST_SUITE("praetorians - G1 Infrastructure and Spatial Navigation") {
    TEST_CASE("esPretoriano maps NPC numbers correctly") {
        Npclist[1].Numero = praetorians::PRCLER_NPC;
        Npclist[2].Numero = praetorians::PRMAGO_NPC;
        Npclist[3].Numero = praetorians::PRCAZA_NPC;
        Npclist[4].Numero = praetorians::PRKING_NPC;
        Npclist[5].Numero = praetorians::PRGUER_NPC;
        Npclist[6].Numero = 500;

        CHECK(praetorians::esPretoriano(1) == 1);
        CHECK(praetorians::esPretoriano(2) == 2);
        CHECK(praetorians::esPretoriano(3) == 3);
        CHECK(praetorians::esPretoriano(4) == 4);
        CHECK(praetorians::esPretoriano(5) == 5);
        CHECK(praetorians::esPretoriano(6) == 0);
        CHECK(praetorians::esPretoriano(0) == 0);
    }

    TEST_CASE("EstoyLejos and EstoyMuyLejos range evaluations") {
        MAPA_PRETORIANO = 10;
        Npclist[1].Pos = {5, 30, 25};
        CHECK_FALSE(praetorians::EstoyLejos(1));
        CHECK_FALSE(praetorians::EstoyMuyLejos(1));

        Npclist[1].Pos = {10, 30, 25};
        CHECK_FALSE(praetorians::EstoyLejos(1));
        CHECK_FALSE(praetorians::EstoyMuyLejos(1));

        Npclist[1].Pos = {10, 20, 25};
        CHECK(praetorians::EstoyLejos(1));

        Npclist[1].Pos = {10, 30, 45};
        CHECK(praetorians::EstoyMuyLejos(1));
    }

    TEST_CASE("EsAlcanzable line-of-sight checks") {
        if (UserList.size() <= 2) {
            UserList.resize(10);
        }
        MAPA_PRETORIANO = 10;
        Npclist[1].Pos = {10, 30, 25};
        UserList[1].Pos = {10, 35, 25};
        CHECK(praetorians::EsAlcanzable(1, 1));
        UserList[1].Pos = {10, 5, 5};
        CHECK_FALSE(praetorians::EsAlcanzable(1, 1));
    }

    TEST_CASE("CasperBlock and LiberarCasperBlock ghost displacement") {
        if (UserList.size() <= 5) {
            UserList.resize(10);
        }
        MAPA_PRETORIANO = 10;
        if (MapData.size() <= mapBlockIndex(10, 100, 100)) {
            MapData.resize((10 + 1) * 101 * 101);
        }
        Npclist[1].Pos = {10, 30, 25};
        Npclist[1].char_appearance.CharIndex = 100;
        Npclist[1].CanAttack = 1;
        praetorians::PraetorianCallbacks cb{};
        cb.LegalPos = [](std::int16_t map, std::int16_t x, std::int16_t y) { return (x == 31 && y == 26); };
        bool send_data_called = false;
        cb.SendData = [&](std::int16_t target, std::int16_t index, const std::string& msg) { send_data_called = true; };
        cb.PrepareMessageCharacterMove = [](std::int16_t char_index, std::int16_t x, std::int16_t y) { return std::string("MOVE"); };
        cb.PrepareMessageChatOverHead = [](const std::string& text, std::int16_t char_index, std::int32_t color) { return std::string("CHAT"); };
        praetorians::SetCallbacks(cb);
        MapData[mapBlockIndex(10, 31, 25)].UserIndex = 1;
        MapData[mapBlockIndex(10, 29, 25)].UserIndex = 2;
        MapData[mapBlockIndex(10, 30, 26)].UserIndex = 3;
        MapData[mapBlockIndex(10, 30, 24)].UserIndex = 4;
        UserList[1].flags.Muerto = 1;
        UserList[2].flags.Muerto = 1;
        UserList[3].flags.Muerto = 1;
        UserList[4].flags.Muerto = 1;
        CHECK(praetorians::CasperBlock(1));
        praetorians::LiberarCasperBlock(1);
        CHECK(Npclist[1].Pos.X == 31);
        CHECK(Npclist[1].Pos.Y == 26);
        CHECK(send_data_called);
        MapData[mapBlockIndex(10, 31, 25)].UserIndex = 0;
        MapData[mapBlockIndex(10, 29, 25)].UserIndex = 0;
        MapData[mapBlockIndex(10, 30, 26)].UserIndex = 0;
        MapData[mapBlockIndex(10, 30, 24)].UserIndex = 0;
        praetorians::ResetCallbacks();
    }
}

TEST_SUITE("praetorians - Fase 2: Movimiento y Navegacion") {
    TEST_CASE("MoverArr, MoverAba, MoverIzq, MoverDer dispatch exact heading") {
        Npclist[1].Pos = {10, 30, 25};
        praetorians::PraetorianCallbacks cb{};
        cb.LegalPos = [](std::int16_t map, std::int16_t x, std::int16_t y) { return true; };
        std::uint8_t last_heading = 0;
        cb.MoveNPCChar = [&](std::int16_t npc, std::uint8_t heading) { last_heading = heading; };
        praetorians::SetCallbacks(cb);

        praetorians::MoverArr(1);
        CHECK(last_heading == static_cast<std::uint8_t>(eHeading::NORTH));

        praetorians::MoverAba(1);
        CHECK(last_heading == static_cast<std::uint8_t>(eHeading::SOUTH));

        praetorians::MoverIzq(1);
        CHECK(last_heading == static_cast<std::uint8_t>(eHeading::WEST));

        praetorians::MoverDer(1);
        CHECK(last_heading == static_cast<std::uint8_t>(eHeading::EAST));

        praetorians::ResetCallbacks();
    }

    TEST_CASE("GreedyWalkTo straight clear path reduces Manhattan distance") {
        MAPA_PRETORIANO = 10;
        Npclist[1].Pos = {10, 30, 25};
        praetorians::PraetorianCallbacks cb{};
        cb.LegalPos = [](std::int16_t map, std::int16_t x, std::int16_t y) { return true; };
        std::uint8_t moved_heading = 0;
        cb.MoveNPCChar = [&](std::int16_t npc, std::uint8_t heading) {
            moved_heading = heading;
            if (heading == static_cast<std::uint8_t>(eHeading::EAST)) Npclist[npc].Pos.X++;
        };
        praetorians::SetCallbacks(cb);

        praetorians::GreedyWalkTo(1, 10, 35, 25);
        CHECK(moved_heading == static_cast<std::uint8_t>(eHeading::EAST));
        CHECK(Npclist[1].Pos.X == 31);

        praetorians::ResetCallbacks();
    }

    TEST_CASE("GreedyWalkTo L-obstacle quirk behavior") {
        MAPA_PRETORIANO = 10;
        Npclist[1].Pos = {10, 30, 25};
        praetorians::PraetorianCallbacks cb{};
        // Block (31, 25) and (30, 26) creating an L-corner obstacle ahead
        cb.LegalPos = [](std::int16_t map, std::int16_t x, std::int16_t y) {
            if (x == 31 && y == 25) return false;
            if (x == 30 && y == 26) return false;
            return true;
        };
        bool move_called = false;
        cb.MoveNPCChar = [&](std::int16_t npc, std::uint8_t heading) { move_called = true; };
        praetorians::SetCallbacks(cb);

        // Target is (35, 28): dx=5, dy=3. Major axis EAST blocked, minor axis SOUTH blocked.
        praetorians::GreedyWalkTo(1, 10, 35, 28);
        CHECK_FALSE(move_called); // Stops without complex pathfinding around corner

        praetorians::ResetCallbacks();
    }

    TEST_CASE("VolverAlCentro routes to Alcoba 1 (X < 50) and Alcoba 2 (X >= 50)") {
        MAPA_PRETORIANO = 10;
        praetorians::PraetorianCallbacks cb{};
        cb.LegalPos = [](std::int16_t map, std::int16_t x, std::int16_t y) { return true; };
        std::uint8_t heading1 = 0, heading2 = 0;

        cb.MoveNPCChar = [&](std::int16_t npc, std::uint8_t heading) {
            if (npc == 1) heading1 = heading;
            if (npc == 2) heading2 = heading;
        };
        praetorians::SetCallbacks(cb);

        Npclist[1].Pos = {10, 20, 25}; // Left half -> Alcoba 1 (35, 25) -> move EAST
        praetorians::VolverAlCentro(1);
        CHECK(heading1 == static_cast<std::uint8_t>(eHeading::EAST));

        Npclist[2].Pos = {10, 80, 25}; // Right half -> Alcoba 2 (67, 25) -> move WEST
        praetorians::VolverAlCentro(2);
        CHECK(heading2 == static_cast<std::uint8_t>(eHeading::WEST));

        praetorians::ResetCallbacks();
    }

    TEST_CASE("CambiarAlcoba transitions sequentially through waypoints 1..8") {
        MAPA_PRETORIANO = 10;
        Npclist[1].Pos = {10, 10, 10};
        Npclist[1].Invent.ArmourEqpSlot = 0; // Uninitialized -> sets to 1

        praetorians::PraetorianCallbacks cb{};
        cb.LegalPos = [](std::int16_t map, std::int16_t x, std::int16_t y) { return true; };
        praetorians::SetCallbacks(cb);

        praetorians::CambiarAlcoba(1);
        CHECK(Npclist[1].Invent.ArmourEqpSlot == 1);

        // Place NPC at Waypoint 1 (35, 14) -> should advance slot to 2
        Npclist[1].Pos = {10, 35, 14};
        praetorians::CambiarAlcoba(1);
        CHECK(Npclist[1].Invent.ArmourEqpSlot == 2);

        // Place NPC at Waypoint 8 (35, 25) when slot is 8 -> should wrap around to 1
        Npclist[1].Invent.ArmourEqpSlot = 8;
        Npclist[1].Pos = {10, 35, 25};
        praetorians::CambiarAlcoba(1);
        CHECK(Npclist[1].Invent.ArmourEqpSlot == 1);

        praetorians::ResetCallbacks();
    }
}

TEST_SUITE("praetorians - Fase 3: Magia Tactica e Inmolacion") {
    TEST_CASE("EsMagoOClerigo identifies Mage and Cleric classes correctly") {
        if (UserList.size() <= 5) UserList.resize(10);

        UserList[1].clase = eClass::Mage;
        UserList[2].clase = eClass::Cleric;
        UserList[3].clase = eClass::Warrior;
        UserList[4].clase = eClass::Hunter;

        CHECK(praetorians::EsMagoOClerigo(1));
        CHECK(praetorians::EsMagoOClerigo(2));
        CHECK_FALSE(praetorians::EsMagoOClerigo(3));
        CHECK_FALSE(praetorians::EsMagoOClerigo(4));
        CHECK_FALSE(praetorians::EsMagoOClerigo(0));
    }

    TEST_CASE("Support spells NPCCuraLevesNPC caps HP at MaxHp") {
        if (Hechizos.size() <= 2) Hechizos.resize(5);
        Hechizos[1].ManaRequerido = 10;
        Hechizos[1].MinHp = 30;
        Hechizos[1].MaxHp = 50;

        Npclist[1].Spells.push_back(1);
        Npclist[1].Stats.MinMAN = 100;

        Npclist[2].Stats.MaxHp = 100;
        Npclist[2].Stats.MinHp = 80;

        praetorians::NPCCuraLevesNPC(1, 2, 0);
        CHECK(Npclist[1].Stats.MinMAN == 90);
        CHECK(Npclist[2].Stats.MinHp == 100); // Capped at MaxHp
    }

    TEST_CASE("Support spells NPCRemueveParalisisNPC and NPCRemueveVenenoNPC clear flags") {
        if (Hechizos.size() <= 2) Hechizos.resize(5);
        Hechizos[1].ManaRequerido = 10;

        Npclist[1].Spells.clear();
        Npclist[1].Spells.push_back(1);
        Npclist[1].Stats.MinMAN = 100;

        Npclist[2].Veneno = 1;
        Npclist[2].flags.Paralizado = 1;
        Npclist[2].flags.Inmovilizado = 1;

        praetorians::NPCRemueveVenenoNPC(1, 2, 0);
        CHECK(Npclist[2].Veneno == 0);

        praetorians::NPCRemueveParalisisNPC(1, 2, 0);
        CHECK(Npclist[2].flags.Paralizado == 0);
        CHECK(Npclist[2].flags.Inmovilizado == 0);
    }

    TEST_CASE("NPCRemueveInvisibilidad disables invisible and Oculto flags") {
        if (UserList.size() <= 2) UserList.resize(10);
        if (Hechizos.size() <= 2) Hechizos.resize(5);
        Hechizos[1].ManaRequerido = 10;

        Npclist[1].Spells.clear();
        Npclist[1].Spells.push_back(1);
        Npclist[1].Stats.MinMAN = 100;

        UserList[1].flags.invisible = 1;
        UserList[1].flags.Oculto = 1;

        praetorians::NPCRemueveInvisibilidad(1, 1, 0);
        CHECK(UserList[1].flags.invisible == 0);
        CHECK(UserList[1].flags.Oculto == 0);
    }

    TEST_CASE("MagoDestruyeWand countdown and kamikaze death") {
        if (UserList.size() <= 2) UserList.resize(10);
        if (MapData.size() <= mapBlockIndex(10, 100, 100)) {
            MapData.resize((10 + 1) * 101 * 101);
        }
        Npclist[1].Pos = {10, 30, 25};
        Npclist[1].Invent.BarcoSlot = 0; // Uninitialized -> sets to 3

        bool muere_called = false;
        praetorians::PraetorianCallbacks cb{};
        cb.MuereNpc = [&](std::int16_t npc, std::int16_t attacker) { muere_called = true; };
        praetorians::SetCallbacks(cb);

        praetorians::MagoDestruyeWand(1, 3, 0);
        CHECK(Npclist[1].Invent.BarcoSlot == 3);
        CHECK_FALSE(muere_called);

        praetorians::MagoDestruyeWand(1, 3, 0);
        CHECK(Npclist[1].Invent.BarcoSlot == 2);
        CHECK_FALSE(muere_called);

        praetorians::MagoDestruyeWand(1, 3, 0);
        CHECK(Npclist[1].Invent.BarcoSlot == 1);
        CHECK(muere_called); // BarcoSlot <= 1 triggers explosion and MuereNpc

        praetorians::ResetCallbacks();
    }
}

TEST_SUITE("praetorians - Fase 4: IA de Combate y Formacion Militar") {
    TEST_CASE("PRREY_AI returns to center if displaced") {
        MAPA_PRETORIANO = 10;
        Npclist[1].Pos = {10, 20, 25}; // Far left (outside center alcove)

        bool move_called = false;
        praetorians::PraetorianCallbacks cb{};
        cb.LegalPos = [](std::int16_t map, std::int16_t x, std::int16_t y) { return true; };
        cb.MoveNPCChar = [&](std::int16_t npc, std::uint8_t heading) { move_called = true; };
        praetorians::SetCallbacks(cb);

        praetorians::PRREY_AI(1);
        CHECK(move_called);

        praetorians::ResetCallbacks();
    }

    TEST_CASE("PRGUER_AI melee attack at dist 1 and pursuit at dist > 1") {
        if (UserList.size() <= 2) UserList.resize(10);
        MAPA_PRETORIANO = 10;
        Npclist[1].Pos = {10, 30, 25};

        UserList[1].Pos = {10, 31, 25}; // Dist 1
        UserList[1].flags.Muerto = 0;

        bool ataca_called = false;
        bool move_called = false;

        praetorians::PraetorianCallbacks cb{};
        cb.LegalPos = [](std::int16_t map, std::int16_t x, std::int16_t y) { return true; };
        cb.NpcAtacaUser = [&](std::int16_t npc, std::int16_t target) { ataca_called = true; };
        cb.MoveNPCChar = [&](std::int16_t npc, std::uint8_t heading) { move_called = true; };
        praetorians::SetCallbacks(cb);

        praetorians::PRGUER_AI(1);
        CHECK(ataca_called);
        CHECK_FALSE(move_called);

        // Move user further away (dist > 1) -> pursuit via GreedyWalkTo
        ataca_called = false;
        UserList[1].Pos = {10, 35, 25};
        praetorians::PRGUER_AI(1);
        CHECK_FALSE(ataca_called);
        CHECK(move_called);

        praetorians::ResetCallbacks();
    }

    TEST_CASE("PRCLER_AI ally support priority cascade") {
        if (Hechizos.size() <= 2) Hechizos.resize(5);
        Hechizos[1].ManaRequerido = 10;
        Hechizos[1].MinHp = 30;
        Hechizos[1].MaxHp = 50;

        MAPA_PRETORIANO = 10;
        Npclist[1].Pos = {10, 30, 25}; // Cleric
        Npclist[1].Spells.clear();
        Npclist[1].Spells.push_back(1);
        Npclist[1].Stats.MinMAN = 100;

        Npclist[2].Numero = praetorians::PRGUER_NPC; // Ally Warrior
        Npclist[2].Pos = {10, 32, 25};
        Npclist[2].Veneno = 1;
        Npclist[2].flags.Paralizado = 1;
        Npclist[2].Stats.MaxHp = 100;
        Npclist[2].Stats.MinHp = 50;

        // 1. Poison removal prioritized over paralysis and healing
        praetorians::PRCLER_AI(1);
        CHECK(Npclist[2].Veneno == 0);
        CHECK(Npclist[2].flags.Paralizado == 1);

        // 2. Paralysis removal prioritized over healing
        praetorians::PRCLER_AI(1);
        CHECK(Npclist[2].flags.Paralizado == 0);
        CHECK(Npclist[2].Stats.MinHp == 50);

        // 3. Healing performed when poison and paralysis are clear
        praetorians::PRCLER_AI(1);
        CHECK(Npclist[2].Stats.MinHp > 50);

        praetorians::ResetCallbacks();
    }

    TEST_CASE("PRMAGO_AI kamikaze activation when MinHp < 750") {
        if (UserList.size() <= 2) UserList.resize(10);
        MAPA_PRETORIANO = 10;
        Npclist[1].Pos = {10, 30, 25};
        Npclist[1].Stats.MinHp = 500; // Agony (< 750)
        Npclist[1].Invent.BarcoSlot = 0;

        praetorians::PRMAGO_AI(1);
        CHECK(Npclist[1].Invent.BarcoSlot == 6);
    }

    TEST_CASE("CrearClanPretoriano instantiates 8 NPCs and sets pretorianosVivos = 7") {
        MAPA_PRETORIANO = 10;
        int created_count = 0;

        praetorians::PraetorianCallbacks cb{};
        cb.CrearNPC = [&](std::int16_t npc_num, std::int16_t map, const WorldPos& pos) {
            created_count++;
        };
        praetorians::SetCallbacks(cb);

        // King at X >= 50 -> spawn clan in Alcoba 1 (X=35, Y=25)
        praetorians::CrearClanPretoriano(60);
        CHECK(created_count == 8);
        CHECK(praetorians::pretorianosVivos == 7);

        praetorians::ResetCallbacks();
    }
}

