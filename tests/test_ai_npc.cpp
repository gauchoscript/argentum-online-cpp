#include <doctest/doctest.h>
#include "server/AI_NPC.hpp"
#include "server/Declares.hpp"

TEST_CASE("AI_NPC - Fase 1 (G1) Suite de Pruebas") {
    // Reset global state
    AI_NPC::ResetCallbacks();
    if (UserList.size() < 100) UserList.resize(100);

    MinXBorder = 10;
    MaxXBorder = 90;
    MinYBorder = 10;
    MaxYBorder = 90;

    std::int16_t npc_index = 1;
    std::int16_t user_index = 5;

    // Direct access reset for Npclist / UserList testing slots
    Npclist[npc_index] = npc{};
    UserList[user_index] = User{};

    Npclist[npc_index].Pos = {1, 50, 50};
    UserList[user_index].Pos = {1, 50, 50};

    SUBCASE("Despacho de hechizos aleatorios con NpcLanzaUnSpell") {
        Npclist[npc_index].flags.LanzaSpells = 2;
        Npclist[npc_index].Spells.resize(3, 0);
        Npclist[npc_index].Spells[1] = 10; // Spell 1
        Npclist[npc_index].Spells[2] = 25; // Spell 2

        std::int16_t casted_spell = 0;
        std::int16_t target_user_casted = 0;

        AI_NPC::AiNpcCallbacks cb{};
        cb.GetUser = [](std::int16_t u_idx) -> const User& {
            return UserList[u_idx];
        };
        cb.RandomNumber = [](std::int32_t min, std::int32_t max) -> std::int32_t {
            return 2; // Simulate choosing spell slot #2
        };
        cb.NpcLanzaSpellSobreUser = [&](std::int16_t n_idx, std::int16_t u_idx, std::int16_t spell_id) {
            casted_spell = spell_id;
            target_user_casted = u_idx;
        };
        AI_NPC::SetCallbacks(cb);

        AI_NPC::NpcLanzaUnSpell(npc_index, user_index);
        CHECK(casted_spell == 25);
        CHECK(target_user_casted == user_index);

        // Should NOT cast if user is invisible or hidden
        UserList[user_index].flags.invisible = 1;
        casted_spell = 0;
        AI_NPC::NpcLanzaUnSpell(npc_index, user_index);
        CHECK(casted_spell == 0);
    }

    SUBCASE("Despacho de hechizos a NPCs con NpcLanzaUnSpellSobreNpc") {
        std::int16_t target_npc = 2;
        Npclist[npc_index].flags.LanzaSpells = 1;
        Npclist[npc_index].Spells.resize(2, 0);
        Npclist[npc_index].Spells[1] = 42;

        std::int16_t casted_spell = 0;
        std::int16_t target_npc_casted = 0;

        AI_NPC::AiNpcCallbacks cb{};
        cb.RandomNumber = [](std::int32_t min, std::int32_t max) -> std::int32_t {
            return 1;
        };
        cb.NpcLanzaSpellSobreNpc = [&](std::int16_t n_idx, std::int16_t t_npc, std::int16_t spell_id) {
            casted_spell = spell_id;
            target_npc_casted = t_npc;
        };
        AI_NPC::SetCallbacks(cb);

        AI_NPC::NpcLanzaUnSpellSobreNpc(npc_index, target_npc);
        CHECK(casted_spell == 42);
        CHECK(target_npc_casted == target_npc);
    }

    SUBCASE("Evaluación determinista de UserNear con paridad Not Int(dist) > 1") {
        Npclist[npc_index].PFINFO.TargetUser = user_index;
        
        AI_NPC::AiNpcCallbacks cb{};
        cb.GetUser = [](std::int16_t u_idx) -> const User& {
            return UserList[u_idx];
        };
        
        // Distancia 0.0 (misma casilla) -> dist <= 1 -> UserNear = true
        cb.Distancia = [](const WorldPos& p1, const WorldPos& p2) { return 0.0; };
        AI_NPC::SetCallbacks(cb);
        CHECK(AI_NPC::UserNear(npc_index) == true);

        // Distancia 1.0 (adyacente ortogonal) -> Int(1.0) = 1 -> UserNear = true
        cb.Distancia = [](const WorldPos& p1, const WorldPos& p2) { return 1.0; };
        AI_NPC::SetCallbacks(cb);
        CHECK(AI_NPC::UserNear(npc_index) == true);

        // Distancia 1.414 (diagonal) -> Int(1.414) = 1 -> UserNear = true (Paridad VB6 Int(1.414) == 1)
        cb.Distancia = [](const WorldPos& p1, const WorldPos& p2) { return 1.414; };
        AI_NPC::SetCallbacks(cb);
        CHECK(AI_NPC::UserNear(npc_index) == true);

        // Distancia 2.0 (distancia > 1) -> Int(2.0) = 2 -> UserNear = false
        cb.Distancia = [](const WorldPos& p1, const WorldPos& p2) { return 2.0; };
        AI_NPC::SetCallbacks(cb);
        CHECK(AI_NPC::UserNear(npc_index) == false);
    }

    SUBCASE("Inspección de trayectoria con ReCalculatePath y PathEnd") {
        // PathLenght == 0 -> ReCalculatePath debe ser true
        Npclist[npc_index].PFINFO.PathLenght = 0;
        CHECK(AI_NPC::ReCalculatePath(npc_index) == true);

        // PathLenght == CurPos - 1 y UserNear == false -> ReCalculatePath debe ser true
        Npclist[npc_index].PFINFO.PathLenght = 5;
        Npclist[npc_index].PFINFO.CurPos = 6;
        Npclist[npc_index].PFINFO.TargetUser = user_index;
        AI_NPC::AiNpcCallbacks cb{};
        cb.GetUser = [](std::int16_t u_idx) -> const User& { return UserList[u_idx]; };
        cb.Distancia = [](const WorldPos& p1, const WorldPos& p2) { return 3.0; };
        AI_NPC::SetCallbacks(cb);
        CHECK(AI_NPC::ReCalculatePath(npc_index) == true);

        // CurPos == PathLenght -> PathEnd debe ser true
        Npclist[npc_index].PFINFO.PathLenght = 4;
        Npclist[npc_index].PFINFO.CurPos = 4;
        CHECK(AI_NPC::PathEnd(npc_index) == true);
        
        Npclist[npc_index].PFINFO.CurPos = 2;
        CHECK(AI_NPC::PathEnd(npc_index) == false);
    }

    SUBCASE("FollowPath e inversión de coordenadas al avanzar") {
        Npclist[npc_index].Pos = {1, 50, 50};
        Npclist[npc_index].PFINFO.PathLenght = 2;
        Npclist[npc_index].PFINFO.CurPos = 1;
        Npclist[npc_index].PFINFO.Path.resize(3);

        // Guardamos nodo en Path: X=12, Y=34
        Npclist[npc_index].PFINFO.Path[1].X = 12;
        Npclist[npc_index].PFINFO.Path[1].Y = 34;

        WorldPos target_pos_moved{};
        std::uint8_t moved_heading = 0;

        AI_NPC::AiNpcCallbacks cb{};
        cb.FindDirection = [](const WorldPos& from, const WorldPos& to) -> std::uint8_t {
            return 2; // eHeading.EAST
        };
        cb.MoveNPCChar = [&](std::int16_t n_idx, std::uint8_t heading) {
            moved_heading = heading;
        };
        AI_NPC::SetCallbacks(cb);

        bool result = AI_NPC::FollowPath(npc_index);
        CHECK(result == true);
        CHECK(moved_heading == 2);
        CHECK(Npclist[npc_index].PFINFO.CurPos == 2);
    }

    SUBCASE("PathFindingAI: búsqueda en 10x10 e inversión de ejes (Bug #45)") {
        Npclist[npc_index].Pos = {1, 50, 50};
        UserList[user_index].Pos = {1, 55, 52}; // X=55, Y=52
        UserList[user_index].flags.Muerto = 0;
        UserList[user_index].flags.invisible = 0;
        UserList[user_index].flags.Oculto = 0;
        UserList[user_index].flags.AdminPerseguible = 1;

        bool seek_path_called = false;

        AI_NPC::AiNpcCallbacks cb{};
        cb.GetMapUserIndex = [&](std::int16_t map, std::int16_t x, std::int16_t y) -> std::int16_t {
            if (map == 1 && x == 55 && y == 52) return user_index;
            return 0;
        };
        cb.GetUser = [](std::int16_t u_idx) -> const User& {
            return UserList[u_idx];
        };
        cb.SeekPath = [&](std::int16_t n_idx) {
            seek_path_called = true;
        };
        AI_NPC::SetCallbacks(cb);

        bool found = AI_NPC::PathFindingAI(npc_index);
        CHECK(found == true);
        CHECK(seek_path_called == true);
        CHECK(Npclist[npc_index].PFINFO.TargetUser == user_index);
        
        // Inversión estricta de ejes Bug #45: Target.X = user.Pos.Y (52), Target.Y = user.Pos.X (55)
        CHECK(Npclist[npc_index].PFINFO.Target.X == 52);
        CHECK(Npclist[npc_index].PFINFO.Target.Y == 55);
    }
}

TEST_SUITE("AI_NPC - Fase 2: Escaneo Adyacente, Centinelas y Objetos Mágicos") {
    TEST_CASE("GuardiasAI: Comportamiento faccionario de Guardias Reales y del Caos") {
        AI_NPC::ResetCallbacks();
        if (UserList.size() < 100) UserList.resize(100);

        std::int16_t guard_npc = 1;
        std::int16_t citizen_user = 2;
        std::int16_t criminal_user = 3;

        Npclist[guard_npc] = npc{};
        UserList[citizen_user] = User{};
        UserList[criminal_user] = User{};

        Npclist[guard_npc].Pos = {1, 50, 50};
        UserList[citizen_user].Pos = {1, 50, 49}; // North
        UserList[criminal_user].Pos = {1, 51, 50}; // East

        UserList[citizen_user].name = "Ciudadano";
        UserList[citizen_user].flags.Muerto = 0;
        UserList[citizen_user].flags.AdminPerseguible = 1;

        UserList[criminal_user].name = "Criminal";
        UserList[criminal_user].flags.Muerto = 0;
        UserList[criminal_user].flags.AdminPerseguible = 1;

        std::int16_t attacked_user = 0;

        AI_NPC::AiNpcCallbacks cb{};
        cb.InMapBounds = [](std::int16_t map, std::int16_t x, std::int16_t y) -> bool {
            return true;
        };
        cb.HeadtoPos = [](std::uint8_t heading, WorldPos& pos) {
            if (heading == static_cast<std::uint8_t>(eHeading::NORTH)) pos.Y--;
            else if (heading == static_cast<std::uint8_t>(eHeading::EAST)) pos.X++;
        };
        cb.GetMapUserIndex = [&](std::int16_t map, std::int16_t x, std::int16_t y) -> std::int16_t {
            if (x == 50 && y == 49) return citizen_user;
            if (x == 51 && y == 50) return criminal_user;
            return 0;
        };
        cb.GetUser = [](std::int16_t u_idx) -> const User& {
            return UserList[u_idx];
        };
        cb.EsCriminal = [&](std::int16_t u_idx) -> bool {
            return (u_idx == criminal_user);
        };
        cb.NpcAtacaUser = [&](std::int16_t n_idx, std::int16_t u_idx) -> bool {
            attacked_user = u_idx;
            return true;
        };
        AI_NPC::SetCallbacks(cb);

        SUBCASE("Guardia Real ataca únicamente criminales") {
            attacked_user = 0;
            AI_NPC::GuardiasAI(guard_npc, false); // DelCaos = false (Guardia Real)
            CHECK(attacked_user == criminal_user);
        }

        SUBCASE("Guardia del Caos ataca únicamente ciudadanos") {
            attacked_user = 0;
            AI_NPC::GuardiasAI(guard_npc, true); // DelCaos = true (Guardia del Caos)
            CHECK(attacked_user == citizen_user);
        }
    }

    TEST_CASE("GuardiasAI y HostilBuenoAI: Represalia inmediata contra agresor AttackedBy") {
        AI_NPC::ResetCallbacks();
        if (UserList.size() < 100) UserList.resize(100);

        std::int16_t guard_npc = 1;
        std::int16_t attacker_user = 2;

        Npclist[guard_npc] = npc{};
        UserList[attacker_user] = User{};

        Npclist[guard_npc].Pos = {1, 50, 50};
        Npclist[guard_npc].flags.AttackedBy = "Atacante";

        UserList[attacker_user].Pos = {1, 50, 49};
        UserList[attacker_user].name = "Atacante";
        UserList[attacker_user].flags.Muerto = 0;
        UserList[attacker_user].flags.AdminPerseguible = 1;

        std::int16_t attacked_user = 0;

        AI_NPC::AiNpcCallbacks cb{};
        cb.InMapBounds = [](std::int16_t map, std::int16_t x, std::int16_t y) -> bool { return true; };
        cb.HeadtoPos = [](std::uint8_t heading, WorldPos& pos) { if (heading == 1) pos.Y--; };
        cb.GetMapUserIndex = [&](std::int16_t map, std::int16_t x, std::int16_t y) -> std::int16_t {
            if (x == 50 && y == 49) return attacker_user;
            return 0;
        };
        cb.GetUser = [](std::int16_t u_idx) -> const User& { return UserList[u_idx]; };
        cb.EsCriminal = [](std::int16_t u_idx) -> bool { return false; }; // No es criminal
        cb.NpcAtacaUser = [&](std::int16_t n_idx, std::int16_t u_idx) -> bool {
            attacked_user = u_idx;
            return true;
        };
        AI_NPC::SetCallbacks(cb);

        // Guardia Real responde atacando al atacante registrado aunque NO sea criminal
        AI_NPC::GuardiasAI(guard_npc, false);
        CHECK(attacked_user == attacker_user);

        // HostilBuenoAI ataca únicamente si coincide con AttackedBy
        attacked_user = 0;
        AI_NPC::HostilBuenoAI(guard_npc);
        CHECK(attacked_user == attacker_user);
    }

    TEST_CASE("HostilMalvadoAI: Evaluación de UserProtected y tirada 50% de casteo") {
        AI_NPC::ResetCallbacks();
        if (UserList.size() < 100) UserList.resize(100);

        std::int16_t evil_npc = 1;
        std::int16_t user_index = 2;

        Npclist[evil_npc] = npc{};
        UserList[user_index] = User{};

        Npclist[evil_npc].Pos = {1, 50, 50};
        Npclist[evil_npc].flags.LanzaSpells = 1;
        Npclist[evil_npc].Spells.resize(2, 42);

        UserList[user_index].Pos = {1, 50, 49};
        UserList[user_index].flags.Muerto = 0;
        UserList[user_index].flags.AdminPerseguible = 1;

        bool melee_attacked = false;
        bool spell_casted = false;

        AI_NPC::AiNpcCallbacks cb{};
        cb.InMapBounds = [](std::int16_t map, std::int16_t x, std::int16_t y) -> bool { return true; };
        cb.HeadtoPos = [](std::uint8_t heading, WorldPos& pos) { if (heading == 1) pos.Y--; };
        cb.GetMapUserIndex = [&](std::int16_t map, std::int16_t x, std::int16_t y) -> std::int16_t {
            if (x == 50 && y == 49) return user_index;
            return 0;
        };
        cb.GetUser = [](std::int16_t u_idx) -> const User& { return UserList[u_idx]; };
        cb.NpcAtacaUser = [&](std::int16_t n_idx, std::int16_t u_idx) -> bool {
            melee_attacked = true;
            return true;
        };
        cb.NpcLanzaSpellSobreUser = [&](std::int16_t n_idx, std::int16_t u_idx, std::int16_t spell) {
            spell_casted = true;
        };
        AI_NPC::SetCallbacks(cb);

        SUBCASE("Filtro UserProtected inhible ataque") {
            UserList[user_index].flags.Ignorado = 1; // Protegido
            AI_NPC::HostilMalvadoAI(evil_npc);
            CHECK(melee_attacked == false);
            CHECK(spell_casted == false);
        }

        SUBCASE("Tirada 50%: Rnd == 1 ataca melee y castea spell") {
            UserList[user_index].flags.Ignorado = 0;
            cb.RandomNumber = [](std::int32_t min, std::int32_t max) -> std::int32_t {
                if (max == 2) return 1;
                return 1;
            };
            AI_NPC::SetCallbacks(cb);

            AI_NPC::HostilMalvadoAI(evil_npc);
            CHECK(melee_attacked == true);
            CHECK(spell_casted == true);
        }

        SUBCASE("Tirada 50%: Rnd == 2 castea spell sin ataque melee previo") {
            UserList[user_index].flags.Ignorado = 0;
            cb.RandomNumber = [](std::int32_t min, std::int32_t max) -> std::int32_t {
                if (max == 2) return 2;
                return 1;
            };
            AI_NPC::SetCallbacks(cb);

            AI_NPC::HostilMalvadoAI(evil_npc);
            CHECK(melee_attacked == false);
            CHECK(spell_casted == true);
        }
    }

    TEST_CASE("RestoreOldMovement: Restablecimiento en criaturas independientes") {
        std::int16_t npc_index = 1;
        Npclist[npc_index] = npc{};

        Npclist[npc_index].Movement = static_cast<TipoAI>(10);
        Npclist[npc_index].Hostile = 1;
        Npclist[npc_index].flags.OldMovement = static_cast<TipoAI>(2);
        Npclist[npc_index].flags.OldHostil = 0;
        Npclist[npc_index].flags.AttackedBy = "Enemigo";

        SUBCASE("Criatura sin amo (MaestroUser == 0) restaura postura") {
            Npclist[npc_index].MaestroUser = 0;
            AI_NPC::RestoreOldMovement(npc_index);
            CHECK(Npclist[npc_index].Movement == static_cast<TipoAI>(2));
            CHECK(Npclist[npc_index].Hostile == 0);
            CHECK(Npclist[npc_index].flags.AttackedBy.empty() == true);
        }

        SUBCASE("Mascota (MaestroUser > 0) preserva su estado") {
            Npclist[npc_index].MaestroUser = 5;
            AI_NPC::RestoreOldMovement(npc_index);
            CHECK(Npclist[npc_index].Movement == static_cast<TipoAI>(10));
            CHECK(Npclist[npc_index].Hostile == 1);
            CHECK(Npclist[npc_index].flags.AttackedBy == "Enemigo");
        }
    }

    TEST_CASE("AiNpcObjeto: Casteo selectivo con probabilidad 2/3") {
        AI_NPC::ResetCallbacks();
        if (UserList.size() < 100) UserList.resize(100);

        std::int16_t obj_npc = 1;
        std::int16_t user_index = 2;

        Npclist[obj_npc] = npc{};
        Npclist[obj_npc].Pos = {1, 50, 50};
        Npclist[obj_npc].flags.LanzaSpells = 1;
        Npclist[obj_npc].Spells.resize(2, 99);

        UserList[user_index] = User{};
        UserList[user_index].Pos = {1, 52, 51}; // En rango visual <= 8x6
        UserList[user_index].flags.Muerto = 0;
        UserList[user_index].flags.AdminPerseguible = 1;

        std::vector<std::int16_t> map_users = {user_index};
        bool spell_casted = false;

        AI_NPC::AiNpcCallbacks cb{};
        cb.GetConnGroupUserEntries = [&](std::int16_t map) -> const std::vector<std::int16_t>& {
            return map_users;
        };
        cb.GetUser = [](std::int16_t u_idx) -> const User& { return UserList[u_idx]; };
        cb.NpcLanzaSpellSobreUser = [&](std::int16_t n_idx, std::int16_t u_idx, std::int16_t spell) {
            spell_casted = true;
        };
        AI_NPC::SetCallbacks(cb);

        SUBCASE("Probabilidad 2/3 (Rnd < 3): castea el hechizo") {
            cb.RandomNumber = [](std::int32_t min, std::int32_t max) -> std::int32_t {
                if (max == 3) return 2;
                return 1;
            };
            AI_NPC::SetCallbacks(cb);

            AI_NPC::AiNpcObjeto(obj_npc);
            CHECK(spell_casted == true);
        }

        SUBCASE("Probabilidad 1/3 (Rnd == 3): omite el casteo") {
            cb.RandomNumber = [](std::int32_t min, std::int32_t max) -> std::int32_t { return 3; };
            AI_NPC::SetCallbacks(cb);

            spell_casted = false;
            AI_NPC::AiNpcObjeto(obj_npc);
            CHECK(spell_casted == false);
        }
    }
}

TEST_SUITE("AI_NPC - Fase 3: Búsqueda de Blancos, Persecución y Mascotas") {
    TEST_CASE("IrUsuarioCercano: Escaneo frontal cuando inmovilizado y prioridad de Owner") {
        AI_NPC::ResetCallbacks();
        if (UserList.size() < 100) UserList.resize(100);

        std::int16_t npc_index = 1;
        std::int16_t owner_user = 2;
        std::int16_t other_user = 3;

        Npclist[npc_index] = npc{};
        UserList[owner_user] = User{};
        UserList[other_user] = User{};

        Npclist[npc_index].Pos = {1, 50, 50};
        Npclist[npc_index].Owner = owner_user;

        UserList[owner_user].Pos = {1, 52, 50}; // East
        UserList[owner_user].name = "Amo";
        UserList[other_user].Pos = {1, 50, 48}; // North

        std::vector<std::int16_t> map_users = {owner_user, other_user};
        std::uint8_t moved_heading = 0;

        AI_NPC::AiNpcCallbacks cb{};
        cb.GetConnGroupUserEntries = [&](std::int16_t map) -> const std::vector<std::int16_t>& {
            return map_users;
        };
        cb.GetUser = [](std::int16_t u_idx) -> const User& { return UserList[u_idx]; };
        cb.FindDirection = [](const WorldPos& from, const WorldPos& to) -> std::uint8_t {
            if (to.X == 52) return static_cast<std::uint8_t>(eHeading::EAST);
            return static_cast<std::uint8_t>(eHeading::NORTH);
        };
        cb.MoveNPCChar = [&](std::int16_t n_idx, std::uint8_t heading) {
            moved_heading = heading;
        };
        AI_NPC::SetCallbacks(cb);

        SUBCASE("Prioridad de Owner sobre otros usuarios en rango") {
            AI_NPC::IrUsuarioCercano(npc_index);
            CHECK(moved_heading == static_cast<std::uint8_t>(eHeading::EAST));
        }

        SUBCASE("Inmovilizado: escanea solo casilla al frente (North)") {
            Npclist[npc_index].flags.Inmovilizado = 1;
            Npclist[npc_index].char_appearance.heading = eHeading::NORTH;
            Npclist[npc_index].flags.LanzaSpells = 1;
            Npclist[npc_index].Spells.resize(2, 77);

            std::int16_t casted_spell = 0;
            cb.NpcLanzaSpellSobreUser = [&](std::int16_t n_idx, std::int16_t u_idx, std::int16_t spell) {
                casted_spell = spell;
            };
            cb.RandomNumber = [](std::int32_t min, std::int32_t max) -> std::int32_t { return 1; };
            AI_NPC::SetCallbacks(cb);

            AI_NPC::IrUsuarioCercano(npc_index);
            CHECK(casted_spell == 77); // Detectó a other_user al Norte
        }
    }

    TEST_CASE("SeguirAgresor: Regla de seguro para mascotas y exclusión de Elemental de Agua") {
        AI_NPC::ResetCallbacks();
        if (UserList.size() < 100) UserList.resize(100);

        std::int16_t pet_npc = 1;
        std::int16_t master_user = 2;
        std::int16_t victim_user = 3;

        Npclist[pet_npc] = npc{};
        UserList[master_user] = User{};
        UserList[victim_user] = User{};

        Npclist[pet_npc].Pos = {1, 50, 50};
        Npclist[pet_npc].MaestroUser = master_user;
        Npclist[pet_npc].flags.AttackedBy = "Victima";

        UserList[master_user].name = "AmoCiudadano";
        UserList[master_user].flags.Seguro = true; // Seguro activado

        UserList[victim_user].name = "Victima";
        UserList[victim_user].Pos = {1, 50, 49}; // Adyacente

        std::vector<std::int16_t> map_users = {victim_user};
        std::string console_msg_sent;
        bool pet_attacked = false;

        AI_NPC::AiNpcCallbacks cb{};
        cb.GetConnGroupUserEntries = [&](std::int16_t map) -> const std::vector<std::int16_t>& {
            return map_users;
        };
        cb.GetUser = [](std::int16_t u_idx) -> const User& { return UserList[u_idx]; };
        cb.EsCriminal = [](std::int16_t u_idx) -> bool { return false; }; // Ambos son ciudadanos
        cb.WriteConsoleMsg = [&](std::int16_t u_idx, const std::string& msg, std::int16_t font) {
            console_msg_sent = msg;
        };
        cb.NpcAtacaUser = [&](std::int16_t n_idx, std::int16_t u_idx) -> bool {
            pet_attacked = true;
            return true;
        };
        AI_NPC::SetCallbacks(cb);

        SUBCASE("Mascota con seguro activo se niega a atacar a ciudadanos") {
            AI_NPC::SeguirAgresor(pet_npc);
            CHECK(console_msg_sent == "La mascota no atacará a ciudadanos si eres miembro del ejército real o tienes el seguro activado.");
            CHECK(Npclist[pet_npc].flags.AttackedBy.empty() == true);
            CHECK(pet_attacked == false);
        }

        SUBCASE("Elemental de Agua (Numero 92) omite ataque melee") {
            Npclist[pet_npc].MaestroUser = 0; // Sin amo
            Npclist[pet_npc].Numero = AI_NPC::ELEMENTALAGUA; // 92
            pet_attacked = false;

            AI_NPC::SeguirAgresor(pet_npc);
            CHECK(pet_attacked == false); // No ejecuta NpcAtacaUser por ser Elemental de Agua
        }
    }

    TEST_CASE("PersigueCriminal y PersigueCiudadano: Persecución selectiva") {
        AI_NPC::ResetCallbacks();
        if (UserList.size() < 100) UserList.resize(100);

        std::int16_t guard_npc = 1;
        std::int16_t target_user = 2;

        Npclist[guard_npc] = npc{};
        UserList[target_user] = User{};

        Npclist[guard_npc].Pos = {1, 50, 50};
        UserList[target_user].Pos = {1, 53, 50}; // Dentro del rango visual
        UserList[target_user].flags.Muerto = 0;
        UserList[target_user].flags.AdminPerseguible = 1;

        std::vector<std::int16_t> map_users = {target_user};
        std::uint8_t moved_heading = 0;

        AI_NPC::AiNpcCallbacks cb{};
        cb.GetConnGroupUserEntries = [&](std::int16_t map) -> const std::vector<std::int16_t>& {
            return map_users;
        };
        cb.GetUser = [](std::int16_t u_idx) -> const User& { return UserList[u_idx]; };
        cb.FindDirection = [](const WorldPos& from, const WorldPos& to) -> std::uint8_t {
            return static_cast<std::uint8_t>(eHeading::EAST);
        };
        cb.MoveNPCChar = [&](std::int16_t n_idx, std::uint8_t heading) {
            moved_heading = heading;
        };
        AI_NPC::SetCallbacks(cb);

        SUBCASE("PersigueCriminal avanza hacia objetivo criminal") {
            cb.EsCriminal = [](std::int16_t u_idx) -> bool { return true; };
            AI_NPC::SetCallbacks(cb);

            AI_NPC::PersigueCriminal(guard_npc);
            CHECK(moved_heading == static_cast<std::uint8_t>(eHeading::EAST));
        }

        SUBCASE("PersigueCiudadano avanza hacia objetivo ciudadano") {
            moved_heading = 0;
            cb.EsCriminal = [](std::int16_t u_idx) -> bool { return false; };
            AI_NPC::SetCallbacks(cb);

            AI_NPC::PersigueCiudadano(guard_npc);
            CHECK(moved_heading == static_cast<std::uint8_t>(eHeading::EAST));
        }
    }

    TEST_CASE("SeguirAmo: Umbral de seguimiento mayor a 3 baldosas") {
        AI_NPC::ResetCallbacks();
        if (UserList.size() < 100) UserList.resize(100);

        std::int16_t pet_npc = 1;
        std::int16_t master_user = 2;

        Npclist[pet_npc] = npc{};
        UserList[master_user] = User{};

        Npclist[pet_npc].Pos = {1, 50, 50};
        Npclist[pet_npc].MaestroUser = master_user;

        std::uint8_t moved_heading = 0;

        AI_NPC::AiNpcCallbacks cb{};
        cb.GetUser = [](std::int16_t u_idx) -> const User& { return UserList[u_idx]; };
        cb.FindDirection = [](const WorldPos& from, const WorldPos& to) -> std::uint8_t {
            return static_cast<std::uint8_t>(eHeading::EAST);
        };
        cb.MoveNPCChar = [&](std::int16_t n_idx, std::uint8_t heading) {
            moved_heading = heading;
        };

        SUBCASE("Distancia <= 3 baldosas: reposa sin avanzar") {
            UserList[master_user].Pos = {1, 52, 50}; // Distancia 2
            cb.Distancia = [](const WorldPos& p1, const WorldPos& p2) { return 2.0; };
            AI_NPC::SetCallbacks(cb);

            moved_heading = 0;
            AI_NPC::SeguirAmo(pet_npc);
            CHECK(moved_heading == 0);
        }

        SUBCASE("Distancia > 3 baldosas: avanza hacia el amo") {
            UserList[master_user].Pos = {1, 55, 50}; // Distancia 5
            cb.Distancia = [](const WorldPos& p1, const WorldPos& p2) { return 5.0; };
            AI_NPC::SetCallbacks(cb);

            moved_heading = 0;
            AI_NPC::SeguirAmo(pet_npc);
            CHECK(moved_heading == static_cast<std::uint8_t>(eHeading::EAST));
        }
    }

    TEST_CASE("AiNpcAtacaNpc: Frenesí Elemental de Fuego (93) vs Dragón (DRAGON)") {
        AI_NPC::ResetCallbacks();
        std::int16_t fire_elem = 1;
        std::int16_t dragon_npc = 2;

        Npclist[fire_elem] = npc{};
        Npclist[dragon_npc] = npc{};

        Npclist[fire_elem].Pos = {1, 50, 50};
        Npclist[fire_elem].TargetNPC = dragon_npc;
        Npclist[fire_elem].Numero = AI_NPC::ELEMENTALFUEGO; // 93
        Npclist[fire_elem].flags.LanzaSpells = 1;
        Npclist[fire_elem].Spells.resize(2, 50);

        Npclist[dragon_npc].Pos = {1, 50, 51};
        Npclist[dragon_npc].NPCtype = eNPCType::DRAGON;
        Npclist[dragon_npc].flags.LanzaSpells = 1;
        Npclist[dragon_npc].Spells.resize(2, 60);

        int spells_casted = 0;

        AI_NPC::AiNpcCallbacks cb{};
        cb.InMapBounds = [](std::int16_t map, std::int16_t x, std::int16_t y) -> bool { return true; };
        cb.GetMapNpcIndex = [&](std::int16_t map, std::int16_t x, std::int16_t y) -> std::int16_t {
            if (x == 50 && y == 51) return dragon_npc;
            return 0;
        };
        cb.RandomNumber = [](std::int32_t min, std::int32_t max) -> std::int32_t { return 1; };
        cb.NpcLanzaSpellSobreNpc = [&](std::int16_t n_idx, std::int16_t t_npc, std::int16_t spell) {
            spells_casted++;
        };
        AI_NPC::SetCallbacks(cb);

        AI_NPC::AiNpcAtacaNpc(fire_elem);
        // Elemental de Fuego ataca con hechizo y provoca que el Dragón active CanAttack = 1 y contraataque con hechizo
        CHECK(Npclist[dragon_npc].CanAttack == 1);
        CHECK(spells_casted == 2); // 1 casteo de Elemental + 1 contraataque del Dragón
    }

    TEST_CASE("NPCAI: Movimiento al azar MueveAlAzar") {
        AI_NPC::ResetCallbacks();
        std::int16_t test_npc = 1;
        Npclist[test_npc] = npc{};
        Npclist[test_npc].Pos = {1, 50, 50};
        Npclist[test_npc].Movement = TipoAI::MueveAlAzar;

        bool move_called = false;
        AI_NPC::AiNpcCallbacks cb{};
        cb.RandomNumber = [](std::int32_t min, std::int32_t max) -> std::int32_t { return 3; };
        cb.MoveNPCChar = [&](std::int16_t n_idx, std::uint8_t heading) { move_called = true; };
        AI_NPC::SetCallbacks(cb);

        AI_NPC::NPCAI(test_npc);
        CHECK(move_called == true);
    }

    TEST_CASE("NPCAI: GuardiaReal con MueveAlAzar redirige a PersigueCriminal") {
        AI_NPC::ResetCallbacks();
        if (UserList.size() < 100) UserList.resize(100);

        std::int16_t test_npc = 1;
        Npclist[test_npc] = npc{};
        Npclist[test_npc].Pos = {1, 50, 50};
        Npclist[test_npc].NPCtype = eNPCType::GuardiaReal;
        Npclist[test_npc].Movement = TipoAI::MueveAlAzar;

        std::int16_t crim_user = 2;
        UserList[crim_user] = User{};
        UserList[crim_user].Pos = {1, 52, 50};
        UserList[crim_user].flags.Muerto = 0;
        UserList[crim_user].flags.AdminPerseguible = 1;

        std::vector<std::int16_t> map_users = {crim_user};
        bool move_called = false;

        AI_NPC::AiNpcCallbacks cb{};
        cb.RandomNumber = [](std::int32_t min, std::int32_t max) -> std::int32_t { return 1; }; // No mueve al azar (solamente PersigueCriminal)
        cb.MoveNPCChar = [&](std::int16_t n_idx, std::uint8_t heading) { move_called = true; };
        cb.InMapBounds = [](std::int16_t map, std::int16_t x, std::int16_t y) -> bool { return true; };
        cb.HeadtoPos = [](std::uint8_t heading, WorldPos& pos) {
            switch (static_cast<eHeading>(heading)) {
                case eHeading::NORTH: pos.Y--; break;
                case eHeading::EAST:  pos.X++; break;
                case eHeading::SOUTH: pos.Y++; break;
                case eHeading::WEST:  pos.X--; break;
            }
        };
        bool npc_attack_called = false;
        cb.GetMapUserIndex = [&](std::int16_t map, std::int16_t x, std::int16_t y) -> std::int16_t {
            if (x == 50 && y == 51) return crim_user;
            return 0;
        };
        cb.NpcAtacaUser = [&](std::int16_t n_idx, std::int16_t u_idx) -> bool {
            npc_attack_called = true;
            return true;
        };
        cb.GetConnGroupUserEntries = [&](std::int16_t map) -> const std::vector<std::int16_t>& { return map_users; };
        cb.GetUser = [](std::int16_t u_idx) -> const User& { return UserList[u_idx]; };
        cb.EsCriminal = [](std::int16_t u_idx) -> bool { return true; };
        cb.FindDirection = [](const WorldPos& from, const WorldPos& to) -> std::uint8_t {
            return static_cast<std::uint8_t>(eHeading::EAST);
        };
        AI_NPC::SetCallbacks(cb);

        AI_NPC::NPCAI(test_npc);
        CHECK(npc_attack_called == true);
    }

    TEST_CASE("NPCAI: Recuperación de Excepción (ErrorHandler legacy)") {
        AI_NPC::ResetCallbacks();
        std::int16_t test_npc = 1;
        Npclist[test_npc] = npc{};
        Npclist[test_npc].Pos = {1, 50, 50};
        Npclist[test_npc].name = "TestMonster";
        Npclist[test_npc].Movement = TipoAI::NpcMaloAtacaUsersBuenos;

        bool log_error_called = false;
        bool quitar_npc_called = false;
        bool respawn_npc_called = false;

        AI_NPC::AiNpcCallbacks cb{};
        cb.GetConnGroupUserEntries = [](std::int16_t map) -> const std::vector<std::int16_t>& {
            throw std::runtime_error("Excepción simulada en tick de IA");
        };
        cb.GetUser = [](std::int16_t u_idx) -> const User& { return UserList[u_idx]; };
        cb.LogError = [&](const std::string& err) { log_error_called = true; };
        cb.QuitarNPC = [&](std::int16_t n_idx) { quitar_npc_called = true; };
        cb.ReSpawnNpc = [&](const npc& data) { respawn_npc_called = true; };
        AI_NPC::SetCallbacks(cb);

        AI_NPC::NPCAI(test_npc);

        CHECK(log_error_called == true);
        CHECK(quitar_npc_called == true);
        CHECK(respawn_npc_called == true);
    }
}
