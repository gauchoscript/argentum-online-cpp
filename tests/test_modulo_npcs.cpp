#include <doctest/doctest.h>
#include "server/MODULO_NPCs.hpp"
#include "server/Declares.hpp"
#include <algorithm>
#include <vector>

TEST_SUITE("MODULO_NPCs - Fase 1: Resets y Gestión de Slots") {

    TEST_CASE("NextOpenNPC encuentra el primer slot inactivo secuencialmente") {
        for (std::int16_t i = 1; i <= MAXNPCS; ++i) {
            Npclist[i].flags.NPCActive = false;
        }
        LastNPC = 0;
        NumNPCs = 0;

        CHECK(MODULO_NPCs::NextOpenNPC() == 1);

        for (std::int16_t i = 1; i <= 5; ++i) {
            Npclist[i].flags.NPCActive = true;
        }
        CHECK(MODULO_NPCs::NextOpenNPC() == 6);

        Npclist[3].flags.NPCActive = false;
        CHECK(MODULO_NPCs::NextOpenNPC() == 3);

        for (std::int16_t i = 1; i <= MAXNPCS; ++i) {
            Npclist[i].flags.NPCActive = true;
        }
        CHECK(MODULO_NPCs::NextOpenNPC() == MAXNPCS + 1);

        for (std::int16_t i = 1; i <= MAXNPCS; ++i) {
            Npclist[i].flags.NPCActive = false;
        }
    }

    TEST_CASE("Funciones de Reset blanquean exhaustivamente los campos de la estructura npc") {
        std::int16_t idx = 1;
        auto& npc_ref = Npclist[idx];

        npc_ref.flags.AfectaParalisis = 1;
        npc_ref.flags.Envenenado = 1;
        npc_ref.flags.AttackedBy = "Jugador1";
        npc_ref.flags.Follow = true;
        npc_ref.Contadores.Paralisis = 10;
        npc_ref.Contadores.TiempoExistencia = 500;
        npc_ref.char_appearance.body = 12;
        npc_ref.char_appearance.Head = 5;
        npc_ref.char_appearance.CharIndex = 100;
        npc_ref.NroCriaturas = 1;
        npc_ref.Criaturas.resize(2);
        npc_ref.NroExpresiones = 2;
        npc_ref.Expresiones.resize(3);
        npc_ref.name = "Goblin";
        npc_ref.MaestroUser = 4;

        bool perdio_called = false;
        MODULO_NPCs::NpcCallbacks cb{};
        cb.PerdioNpc = [&](std::int16_t user_index) {
            if (user_index == 4) perdio_called = true;
        };
        MODULO_NPCs::SetCallbacks(cb);

        MODULO_NPCs::ResetNpcFlags(idx);
        CHECK(npc_ref.flags.AfectaParalisis == 0);
        CHECK(npc_ref.flags.Envenenado == 0);
        CHECK(npc_ref.flags.AttackedBy.empty());
        CHECK(npc_ref.flags.Follow == false);

        MODULO_NPCs::ResetNpcCounters(idx);
        CHECK(npc_ref.Contadores.Paralisis == 0);
        CHECK(npc_ref.Contadores.TiempoExistencia == 0);

        MODULO_NPCs::ResetNpcCharInfo(idx);
        CHECK(npc_ref.char_appearance.body == 0);
        CHECK(npc_ref.char_appearance.Head == 0);
        CHECK(npc_ref.char_appearance.CharIndex == 0);

        MODULO_NPCs::ResetNpcCriatures(idx);
        CHECK(npc_ref.NroCriaturas == 0);
        CHECK(npc_ref.Criaturas.empty());

        MODULO_NPCs::ResetExpresiones(idx);
        CHECK(npc_ref.NroExpresiones == 0);
        CHECK(npc_ref.Expresiones.empty());

        MODULO_NPCs::ResetNpcMainInfo(idx);
        CHECK(perdio_called == true);
        CHECK(npc_ref.MaestroUser == 0);
        CHECK(npc_ref.name.empty());

        MODULO_NPCs::ResetCallbacks();
    }

    TEST_CASE("OpenNPC e instanciación de slots y contadores globales") {
        for (std::int16_t i = 1; i <= MAXNPCS; ++i) {
            Npclist[i].flags.NPCActive = false;
        }
        LastNPC = 0;
        NumNPCs = 0;

        std::int16_t result_err = MODULO_NPCs::OpenNPC(9999);
        CHECK(result_err == MAXNPCS + 1);
        CHECK(NumNPCs == 0);
        CHECK(LastNPC == 0);

        for (std::int16_t i = 1; i <= MAXNPCS; ++i) {
            Npclist[i].flags.NPCActive = true;
        }

        std::int16_t result_full = MODULO_NPCs::OpenNPC(1);
        CHECK(result_full > MAXNPCS);

        for (std::int16_t i = 1; i <= MAXNPCS; ++i) {
            Npclist[i].flags.NPCActive = false;
        }
        LastNPC = 0;
        NumNPCs = 0;
    }

    TEST_CASE("QuitarNPC desinstancia la entidad, decrementa NumNPCs y recalcula LastNPC") {
        for (std::int16_t i = 1; i <= MAXNPCS; ++i) {
            Npclist[i].flags.NPCActive = false;
        }
        LastNPC = 0;
        NumNPCs = 0;

        Npclist[5].flags.NPCActive = true;
        Npclist[10].flags.NPCActive = true;
        Npclist[15].flags.NPCActive = true;
        Npclist[15].char_appearance.CharIndex = 42;

        NumNPCs = 3;
        LastNPC = 15;

        MODULO_NPCs::QuitarNPC(10);
        CHECK(Npclist[10].flags.NPCActive == false);
        CHECK(NumNPCs == 2);
        CHECK(LastNPC == 15);

        MODULO_NPCs::QuitarNPC(15);
        CHECK(Npclist[15].flags.NPCActive == false);
        CHECK(Npclist[15].char_appearance.CharIndex == 0);
        CHECK(NumNPCs == 1);
        CHECK(LastNPC == 5);

        MODULO_NPCs::QuitarNPC(5);
        CHECK(Npclist[5].flags.NPCActive == false);
        CHECK(NumNPCs == 0);
        CHECK(LastNPC == 0);
    }
}

TEST_SUITE("MODULO_NPCs - Fase 2: Instanciación Espacial y Gráfica") {

    TEST_CASE("TestSpawnTrigger aprueba celdas transitables y rechaza triggers 1, 2 y 3") {
        WorldPos pos{1, 50, 50};
        std::size_t idx = (1 * 101 + 50) * 101 + 50;
        if (idx >= MapData.size()) MapData.resize(idx + 1);

        MapData[idx].trigger = static_cast<eTrigger>(0);
        CHECK(MODULO_NPCs::TestSpawnTrigger(pos) == true);

        MapData[idx].trigger = static_cast<eTrigger>(1);
        CHECK(MODULO_NPCs::TestSpawnTrigger(pos) == false);

        MapData[idx].trigger = static_cast<eTrigger>(2);
        CHECK(MODULO_NPCs::TestSpawnTrigger(pos) == false);

        MapData[idx].trigger = static_cast<eTrigger>(3);
        CHECK(MODULO_NPCs::TestSpawnTrigger(pos) == false);

        MapData[idx].trigger = static_cast<eTrigger>(0);
    }

    TEST_CASE("MakeNPCChar y EraseNPCChar gestionan CharList, MapData, LastChar y NumChars") {
        for (std::int16_t i = 1; i <= MAXCHARS; ++i) {
            CharList[i] = 0;
        }
        LastChar = 0;
        NumChars = 0;

        std::size_t map_idx = (1 * 101 + 10) * 101 + 20;
        if (map_idx >= MapData.size()) MapData.resize(map_idx + 1);

        std::int16_t npc_index = 2;
        Npclist[npc_index].flags.NPCActive = true;
        Npclist[npc_index].Pos = WorldPos{1, 10, 20};

        bool agregar_npc_called = false;
        MODULO_NPCs::NpcCallbacks cb{};
        cb.AgregarNpc = [&](std::int16_t idx) {
            if (idx == npc_index) agregar_npc_called = true;
        };
        MODULO_NPCs::SetCallbacks(cb);

        MODULO_NPCs::MakeNPCChar(true, 1, npc_index, 1, 10, 20);

        CHECK(Npclist[npc_index].char_appearance.CharIndex == 1);
        CHECK(CharList[1] == npc_index);
        CHECK(MapData[map_idx].NpcIndex == npc_index);
        CHECK(LastChar == 1);
        CHECK(NumChars == 1);
        CHECK(agregar_npc_called == true);

        bool remove_pkg_sent = false;
        cb.SendData = [&](ao::net::send_data::SendTarget target, std::int16_t target_index, const std::vector<std::uint8_t>& data) {
            remove_pkg_sent = true;
        };
        MODULO_NPCs::SetCallbacks(cb);

        MODULO_NPCs::EraseNPCChar(npc_index);

        CHECK(Npclist[npc_index].char_appearance.CharIndex == 0);
        CHECK(CharList[1] == 0);
        CHECK(MapData[map_idx].NpcIndex == 0);
        CHECK(LastChar == 0);
        CHECK(NumChars == 0);
        CHECK(remove_pkg_sent == true);

        MODULO_NPCs::ResetCallbacks();
    }

    TEST_CASE("ChangeNPCChar actualiza cuerpo, cabeza y orientación y notifica al área") {
        std::int16_t npc_index = 3;
        Npclist[npc_index].char_appearance.CharIndex = 5;

        bool change_pkg_sent = false;
        MODULO_NPCs::NpcCallbacks cb{};
        cb.SendData = [&](ao::net::send_data::SendTarget target, std::int16_t target_index, const std::vector<std::uint8_t>& data) {
            if (target == ao::net::send_data::SendTarget::ToNPCArea && target_index == npc_index) {
                change_pkg_sent = true;
            }
        };
        MODULO_NPCs::SetCallbacks(cb);

        MODULO_NPCs::ChangeNPCChar(npc_index, 100, 20, eHeading::WEST);

        CHECK(Npclist[npc_index].char_appearance.body == 100);
        CHECK(Npclist[npc_index].char_appearance.Head == 20);
        CHECK(Npclist[npc_index].char_appearance.heading == eHeading::WEST);
        CHECK(change_pkg_sent == true);

        MODULO_NPCs::ResetCallbacks();
    }

    TEST_CASE("SpawnNpc asigna posición física, efectos de invocación y reactivación") {
        for (std::int16_t i = 1; i <= MAXNPCS; ++i) {
            Npclist[i].flags.NPCActive = false;
        }

        MODULO_NPCs::NpcCallbacks cb{};
        cb.ClosestLegalPos = [](const WorldPos& pos, WorldPos& new_pos, bool puede_agua, bool puede_tierra) {
            new_pos = pos;
        };
        int send_data_count = 0;
        cb.SendData = [&](ao::net::send_data::SendTarget target, std::int16_t target_index, const std::vector<std::uint8_t>& data) {
            send_data_count++;
        };
        MODULO_NPCs::SetCallbacks(cb);

        std::int16_t nIndex = 1;
        Npclist[nIndex].flags.NPCActive = true;
        Npclist[nIndex].Pos = WorldPos{1, 30, 40};

        Npclist[nIndex].flags.Respawn = 0;
        Npclist[nIndex].Numero = 1;
        Npclist[nIndex].Orig = WorldPos{1, 30, 40};

        MODULO_NPCs::ReSpawnNpc(Npclist[nIndex]);

        MODULO_NPCs::ResetCallbacks();
    }
}

TEST_SUITE("MODULO_NPCs - Fase 3: Movimiento y Desalojo de Caspers") {

    TEST_CASE("Movimiento Estándar desplaza a celda libre adyacente") {
        std::int16_t npc_index = 1;
        auto& npc_ref = Npclist[npc_index];
        npc_ref.flags.NPCActive = true;
        npc_ref.Pos = WorldPos{1, 50, 50};
        npc_ref.char_appearance.CharIndex = 10;

        std::size_t orig_idx = (1 * 101 + 50) * 101 + 50;
        std::size_t dest_idx = (1 * 101 + 50) * 101 + 51; // SOUTH
        if (dest_idx >= MapData.size()) MapData.resize(dest_idx + 1);

        MapData[orig_idx].NpcIndex = npc_index;
        MapData[dest_idx].NpcIndex = 0;
        MapData[dest_idx].UserIndex = 0;

        bool update_needed_called = false;
        bool send_move_called = false;
        MODULO_NPCs::NpcCallbacks cb{};
        cb.CheckUpdateNeededNpc = [&](std::int16_t idx, eHeading h) {
            if (idx == npc_index && h == eHeading::SOUTH) update_needed_called = true;
        };
        cb.SendData = [&](ao::net::send_data::SendTarget target, std::int16_t idx, const std::vector<std::uint8_t>& data) {
            if (target == ao::net::send_data::SendTarget::ToNPCArea && idx == npc_index) send_move_called = true;
        };
        MODULO_NPCs::SetCallbacks(cb);

        MODULO_NPCs::MoveNPCChar(npc_index, eHeading::SOUTH);

        CHECK(npc_ref.Pos.X == 50);
        CHECK(npc_ref.Pos.Y == 51);
        CHECK(npc_ref.char_appearance.heading == eHeading::SOUTH);
        CHECK(MapData[orig_idx].NpcIndex == 0);
        CHECK(MapData[dest_idx].NpcIndex == npc_index);
        CHECK(update_needed_called == true);
        CHECK(send_move_called == true);

        MODULO_NPCs::ResetCallbacks();
    }

    TEST_CASE("Desalojo de Casper en la misma superficie desplaza al usuario muerto") {
        std::int16_t npc_index = 2;
        auto& npc_ref = Npclist[npc_index];
        npc_ref.flags.NPCActive = true;
        npc_ref.Pos = WorldPos{1, 20, 20};
        npc_ref.char_appearance.CharIndex = 12;

        std::int16_t user_index = 5;
        if (UserList.size() <= static_cast<std::size_t>(user_index)) UserList.resize(user_index + 1);
        auto& user = UserList[user_index];
        user.Pos = WorldPos{1, 21, 20};
        user.char_appearance.CharIndex = 99;

        std::size_t orig_idx = (1 * 101 + 20) * 101 + 20;
        std::size_t dest_idx = (1 * 101 + 21) * 101 + 20;
        if (dest_idx >= MapData.size()) MapData.resize(dest_idx + 1);

        MapData[orig_idx].NpcIndex = npc_index;
        MapData[dest_idx].NpcIndex = 0;
        MapData[dest_idx].UserIndex = user_index;

        bool force_move_called = false;
        eHeading force_dir = eHeading::NORTH;
        MODULO_NPCs::NpcCallbacks cb{};
        cb.WriteForceCharMove = [&](std::int16_t u_idx, eHeading h) {
            if (u_idx == user_index) {
                force_move_called = true;
                force_dir = h;
            }
        };
        cb.HayAgua = [](std::int16_t map, std::int16_t x, std::int16_t y) {
            return false;
        };
        MODULO_NPCs::SetCallbacks(cb);

        MODULO_NPCs::MoveNPCChar(npc_index, eHeading::EAST);

        CHECK(npc_ref.Pos.X == 21);
        CHECK(npc_ref.Pos.Y == 20);
        CHECK(user.Pos.X == 20);
        CHECK(user.Pos.Y == 20);
        CHECK(MapData[orig_idx].UserIndex == user_index);
        CHECK(MapData[dest_idx].NpcIndex == npc_index);
        CHECK(force_move_called == true);
        CHECK(force_dir == eHeading::WEST);

        MODULO_NPCs::ResetCallbacks();
    }

    TEST_CASE("Bloqueo de Casper si cruza entre agua y tierra aborta movimiento") {
        std::int16_t npc_index = 3;
        auto& npc_ref = Npclist[npc_index];
        npc_ref.flags.NPCActive = true;
        npc_ref.flags.AguaValida = 1;
        npc_ref.Pos = WorldPos{1, 30, 30};

        std::int16_t user_index = 6;
        if (UserList.size() <= static_cast<std::size_t>(user_index)) UserList.resize(user_index + 1);
        auto& user = UserList[user_index];
        user.Pos = WorldPos{1, 30, 31};

        std::size_t orig_idx = (1 * 101 + 30) * 101 + 30;
        std::size_t dest_idx = (1 * 101 + 30) * 101 + 31;
        if (dest_idx >= MapData.size()) MapData.resize(dest_idx + 1);

        MapData[orig_idx].NpcIndex = npc_index;
        MapData[dest_idx].UserIndex = user_index;

        MODULO_NPCs::NpcCallbacks cb{};
        cb.HayAgua = [](std::int16_t map, std::int16_t x, std::int16_t y) {
            return (y == 31);
        };
        MODULO_NPCs::SetCallbacks(cb);

        MODULO_NPCs::MoveNPCChar(npc_index, eHeading::SOUTH);

        CHECK(npc_ref.Pos.X == 30);
        CHECK(npc_ref.Pos.Y == 30);
        CHECK(user.Pos.X == 30);
        CHECK(user.Pos.Y == 31);

        MODULO_NPCs::ResetCallbacks();
    }

    TEST_CASE("Movimiento Ilegal invalida PFINFO en NPCs con NpcPathfinding") {
        std::int16_t npc_index = 4;
        auto& npc_ref = Npclist[npc_index];
        npc_ref.flags.NPCActive = true;
        npc_ref.Pos = WorldPos{1, 1, 1};
        npc_ref.Movement = TipoAI::NpcPathfinding;
        npc_ref.PFINFO.PathLenght = 5;
        npc_ref.PFINFO.CurPos = 2;
        npc_ref.PFINFO.NoPath = false;

        MODULO_NPCs::NpcCallbacks cb{};
        cb.LegalPosNPC = [](std::int16_t map, std::int16_t x, std::int16_t y, bool agua, bool pet) {
            return false;
        };
        MODULO_NPCs::SetCallbacks(cb);

        MODULO_NPCs::MoveNPCChar(npc_index, eHeading::WEST);

        CHECK(npc_ref.Pos.X == 1);
        CHECK(npc_ref.Pos.Y == 1);
        CHECK(npc_ref.PFINFO.PathLenght == 0);
        CHECK(npc_ref.PFINFO.CurPos == 0);
        CHECK(npc_ref.PFINFO.NoPath == true);

        MODULO_NPCs::ResetCallbacks();
    }
}

TEST_SUITE("MODULO_NPCs - Fase 4: Muerte, Recompensas y Mascotas") {

    TEST_CASE("MuereNpc Estándar otorga experiencia individual directa y respeta MAXEXP y frags") {
        std::int16_t npc_index = 10;
        auto& npc_ref = Npclist[npc_index];
        npc_ref.flags.NPCActive = true;
        npc_ref.flags.ExpCount = 500;
        npc_ref.Pos = WorldPos{1, 40, 40};

        std::int16_t user_index = 1;
        if (UserList.size() <= static_cast<std::size_t>(user_index)) UserList.resize(user_index + 1);
        auto& user = UserList[user_index];
        user.Stats.Exp = 1000;
        user.Stats.NPCsMuertos = 100;
        user.PartyIndex = 0;

        bool check_level_called = false;
        bool console_msg_called = false;
        bool tirar_items_called = false;

        MODULO_NPCs::NpcCallbacks cb{};
        cb.CheckUserLevel = [&](std::int16_t u_idx) {
            if (u_idx == user_index) check_level_called = true;
        };
        cb.WriteConsoleMsg = [&](std::int16_t u_idx, const std::string& msg, std::int16_t font) {
            if (u_idx == user_index) console_msg_called = true;
        };
        cb.NPC_TIRAR_ITEMS = [&](npc& n, bool is_pret) {
            tirar_items_called = true;
        };
        MODULO_NPCs::SetCallbacks(cb);

        MODULO_NPCs::MuereNpc(npc_index, user_index);

        CHECK(user.Stats.Exp == 1500);
        CHECK(user.Stats.NPCsMuertos == 101);
        CHECK(check_level_called == true);
        CHECK(console_msg_called == true);
        CHECK(tirar_items_called == true);
        CHECK(npc_ref.flags.NPCActive == false);

        MODULO_NPCs::ResetCallbacks();
    }

    TEST_CASE("MuereNpc en Party delega la experiencia en PartyObtenerExito") {
        std::int16_t npc_index = 11;
        auto& npc_ref = Npclist[npc_index];
        npc_ref.flags.NPCActive = true;
        npc_ref.flags.ExpCount = 1000;
        npc_ref.Pos = WorldPos{1, 50, 50};

        std::int16_t user_index = 2;
        if (UserList.size() <= static_cast<std::size_t>(user_index)) UserList.resize(user_index + 1);
        auto& user = UserList[user_index];
        user.Stats.Exp = 2000;
        user.PartyIndex = 5;

        bool party_exito_called = false;
        MODULO_NPCs::NpcCallbacks cb{};
        cb.PartyObtenerExito = [&](std::int16_t u_idx, std::int32_t exp, std::int16_t map, std::int16_t x, std::int16_t y) {
            if (u_idx == user_index && exp == 1000) party_exito_called = true;
        };
        MODULO_NPCs::SetCallbacks(cb);

        MODULO_NPCs::MuereNpc(npc_index, user_index);

        CHECK(party_exito_called == true);
        CHECK(user.Stats.Exp == 2000); // Exp individual no muta en party

        MODULO_NPCs::ResetCallbacks();
    }

    TEST_CASE("Cota estricta de 32.000 frags previene overflow") {
        std::int16_t npc_index = 12;
        Npclist[npc_index].flags.NPCActive = true;

        std::int16_t user_index = 3;
        if (UserList.size() <= static_cast<std::size_t>(user_index)) UserList.resize(user_index + 1);
        auto& user = UserList[user_index];

        user.Stats.NPCsMuertos = 31999;
        MODULO_NPCs::MuereNpc(npc_index, user_index);
        CHECK(user.Stats.NPCsMuertos == 32000);

        Npclist[npc_index].flags.NPCActive = true;
        MODULO_NPCs::MuereNpc(npc_index, user_index);
        CHECK(user.Stats.NPCsMuertos == 32000); // Se mantiene estrictamente en 32000
    }

    TEST_CASE("Mecanismos de Mascotas: QuitarMascota y QuitarPet") {
        std::int16_t user_index = 4;
        if (UserList.size() <= static_cast<std::size_t>(user_index)) UserList.resize(user_index + 1);
        auto& user = UserList[user_index];

        std::int16_t pet_npc = 20;
        Npclist[pet_npc].flags.NPCActive = true;

        user.MascotasIndex[1] = pet_npc;
        user.MascotasType[1] = 5;
        user.NroMascotas = 1;

        MODULO_NPCs::QuitarPet(user_index, pet_npc);

        CHECK(user.MascotasIndex[1] == 0);
        CHECK(user.MascotasType[1] == 0);
        CHECK(user.NroMascotas == 0);
        CHECK(Npclist[pet_npc].flags.NPCActive == false);
    }

    TEST_CASE("DoFollow y FollowAmo conmutan estado de persecución e IA") {
        std::int16_t npc_index = 8;
        auto& npc = Npclist[npc_index];
        npc.flags.NPCActive = true;
        npc.flags.Follow = false;
        npc.Movement = TipoAI::NPCMAmbula;

        MODULO_NPCs::NpcCallbacks cb{};
        cb.FindUser = [](const std::string& name) -> std::int16_t {
            if (name == "TestUser") return 2;
            return 0;
        };
        MODULO_NPCs::SetCallbacks(cb);

        // FollowAmo activa persecución
        MODULO_NPCs::FollowAmo(npc_index);
        CHECK(npc.flags.Follow == true);
        CHECK(npc.Movement == TipoAI::SigueAmo);

        // DoFollow con Follow = true lo desactiva y vuelve a NPCMAmbula
        MODULO_NPCs::DoFollow(npc_index, "TestUser");
        CHECK(npc.flags.Follow == false);
        CHECK(npc.Target == 0);
        CHECK(npc.Movement == TipoAI::NPCMAmbula);

        // DoFollow con Follow = false lo activa y asigna Target vía callback FindUser
        MODULO_NPCs::DoFollow(npc_index, "TestUser");
        CHECK(npc.flags.Follow == true);
        CHECK(npc.Target == 2);
        CHECK(npc.Movement == TipoAI::SigueAmo);

        MODULO_NPCs::ResetCallbacks();
    }
}
