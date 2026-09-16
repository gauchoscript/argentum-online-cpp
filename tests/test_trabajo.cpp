#include <doctest/doctest.h>
#include "server/Trabajo.hpp"
#include "server/Declares.hpp"
#include <vector>

TEST_SUITE("Trabajo - G1: Modificadores y Verificación de Materiales") {

    TEST_CASE("Modificadores por clase") {
        std::int16_t dummy_user = 1;
        UserList[dummy_user] = User{};

        SUBCASE("ModNavegacion") {
            // Pirata -> 1.0
            CHECK(Trabajo::ModNavegacion(eClass::Pirat, dummy_user) == doctest::Approx(1.0f));

            // Worker sin 100 pesca -> 2.0
            UserList[dummy_user].clase = eClass::Worker;
            UserList[dummy_user].Stats.UserSkills[static_cast<std::size_t>(eSkill::Pesca)] = 50;
            CHECK(Trabajo::ModNavegacion(eClass::Worker, dummy_user) == doctest::Approx(2.0f));

            // Worker con 100 pesca -> 1.71
            UserList[dummy_user].Stats.UserSkills[static_cast<std::size_t>(eSkill::Pesca)] = 100;
            CHECK(Trabajo::ModNavegacion(eClass::Worker, dummy_user) == doctest::Approx(1.71f));

            // Otra clase -> 2.0
            CHECK(Trabajo::ModNavegacion(eClass::Warrior, dummy_user) == doctest::Approx(2.0f));
        }

        SUBCASE("ModFundicion") {
            CHECK(Trabajo::ModFundicion(eClass::Worker) == doctest::Approx(1.0f));
            CHECK(Trabajo::ModFundicion(eClass::Warrior) == doctest::Approx(3.0f));
            CHECK(Trabajo::ModFundicion(eClass::Mage) == doctest::Approx(3.0f));
        }

        SUBCASE("ModCarpinteria") {
            CHECK(Trabajo::ModCarpinteria(eClass::Worker) == 1);
            CHECK(Trabajo::ModCarpinteria(eClass::Warrior) == 3);
            CHECK(Trabajo::ModCarpinteria(eClass::Mage) == 3);
        }

        SUBCASE("ModHerreriA") {
            CHECK(Trabajo::ModHerreriA(eClass::Worker) == doctest::Approx(1.0f));
            CHECK(Trabajo::ModHerreriA(eClass::Warrior) == doctest::Approx(3.0f));
            CHECK(Trabajo::ModHerreriA(eClass::Mage) == doctest::Approx(3.0f));
        }

        SUBCASE("ModDomar") {
            CHECK(Trabajo::ModDomar(eClass::Worker) == 1);
            CHECK(Trabajo::ModDomar(eClass::Hunter) == 1);
            CHECK(Trabajo::ModDomar(eClass::Druid) == 1);
            CHECK(Trabajo::ModDomar(eClass::Warrior) == 2);
            CHECK(Trabajo::ModDomar(eClass::Mage) == 2);
        }
    }

    TEST_CASE("MaxItemsConstruibles por nivel de personaje") {
        std::int16_t user_index = 1;
        UserList[user_index] = User{};

        // Nivel 1..4 -> CInt((ELV-4)/5) <= 0 -> std::max(1, 0) = 1
        UserList[user_index].Stats.ELV = 1;
        CHECK(Trabajo::MaxItemsConstruibles(user_index) == 1);

        UserList[user_index].Stats.ELV = 4;
        CHECK(Trabajo::MaxItemsConstruibles(user_index) == 1);

        // Nivel 9 -> (9-4)/5 = 1 -> 1
        UserList[user_index].Stats.ELV = 9;
        CHECK(Trabajo::MaxItemsConstruibles(user_index) == 1);

        // Nivel 14 -> (14-4)/5 = 2 -> 2
        UserList[user_index].Stats.ELV = 14;
        CHECK(Trabajo::MaxItemsConstruibles(user_index) == 2);

        // Nivel 25 -> (25-4)/5 = 4.2 -> CInt(4.2) = 4
        UserList[user_index].Stats.ELV = 25;
        CHECK(Trabajo::MaxItemsConstruibles(user_index) == 4);
    }

    TEST_CASE("QuitarSta y notificaciones") {
        std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].Stats.MinSta = 50;

        bool updated = false;
        Trabajo::TrabajoHooks hooks;
        hooks.WriteUpdateSta = [&](std::int16_t u) {
            CHECK(u == user_index);
            updated = true;
        };
        Trabajo::SetHooks(hooks);

        // Descontar estamina parcial
        Trabajo::QuitarSta(user_index, 20);
        CHECK(UserList[user_index].Stats.MinSta == 30);
        CHECK(updated == true);

        // Descontar más estamina de la disponible -> piso en 0
        updated = false;
        Trabajo::QuitarSta(user_index, 100);
        CHECK(UserList[user_index].Stats.MinSta == 0);
        CHECK(updated == true);

        Trabajo::ResetHooks();
    }

    TEST_CASE("TieneObjetos y QuitarObjetos con inventario fragmentado") {
        std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].CurrentInventorySlots = 10;
        UserList[user_index].Invent.NroItems = 3;

        std::int16_t item_id = 100;
        // Slot 1: 5 unidades
        UserList[user_index].Invent.Object[1].ObjIndex = item_id;
        UserList[user_index].Invent.Object[1].Amount = 5;

        // Slot 4: 10 unidades
        UserList[user_index].Invent.Object[4].ObjIndex = item_id;
        UserList[user_index].Invent.Object[4].Amount = 10;

        // Slot 7: 15 unidades
        UserList[user_index].Invent.Object[7].ObjIndex = item_id;
        UserList[user_index].Invent.Object[7].Amount = 15;

        SUBCASE("TieneObjetos") {
            CHECK(Trabajo::TieneObjetos(item_id, 30, user_index) == true);
            CHECK(Trabajo::TieneObjetos(item_id, 31, user_index) == false);
            CHECK(Trabajo::TieneObjetos(item_id, 1, user_index) == true);
        }

        SUBCASE("QuitarObjetos parcial (slot 1 agotado, slot 4 parcial)") {
            Trabajo::QuitarObjetos(item_id, 8, user_index);

            // Slot 1 debe quedar vacío
            CHECK(UserList[user_index].Invent.Object[1].ObjIndex == 0);
            CHECK(UserList[user_index].Invent.Object[1].Amount == 0);

            // Slot 4 debe quedar con 7
            CHECK(UserList[user_index].Invent.Object[4].ObjIndex == item_id);
            CHECK(UserList[user_index].Invent.Object[4].Amount == 7);

            // Slot 7 intacto
            CHECK(UserList[user_index].Invent.Object[7].ObjIndex == item_id);
            CHECK(UserList[user_index].Invent.Object[7].Amount == 15);
        }

        SUBCASE("QuitarObjetos total") {
            Trabajo::QuitarObjetos(item_id, 30, user_index);

            CHECK(UserList[user_index].Invent.Object[1].Amount == 0);
            CHECK(UserList[user_index].Invent.Object[4].Amount == 0);
            CHECK(UserList[user_index].Invent.Object[7].Amount == 0);
        }
    }

    TEST_CASE("Verificación de Materiales de Herrería y Carpintería") {
        if (ObjDataList.size() < 300) {
            ObjDataList.resize(300);
        }

        std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].CurrentInventorySlots = 10;
        UserList[user_index].Invent.NroItems = 3;

        std::int16_t recipe_item = 50;
        ObjDataList[recipe_item] = ObjData{};
        ObjDataList[recipe_item].LingH = 2;
        ObjDataList[recipe_item].LingP = 1;
        ObjDataList[recipe_item].LingO = 0;
        ObjDataList[recipe_item].SkHerreria = 20;

        // Cargar lingotes en inventario: 4 de hierro (slot 1), 2 de plata (slot 2)
        UserList[user_index].Invent.Object[1].ObjIndex = LingoteHierro;
        UserList[user_index].Invent.Object[1].Amount = 4;

        UserList[user_index].Invent.Object[2].ObjIndex = LingotePlata;
        UserList[user_index].Invent.Object[2].Amount = 2;

        SUBCASE("HerreroTieneMateriales") {
            // 2 items consumen 4 Hierro y 2 Plata -> disponible exacto
            CHECK(Trabajo::HerreroTieneMateriales(user_index, recipe_item, 2) == true);
            // 3 items consumen 6 Hierro -> no alcanza
            CHECK(Trabajo::HerreroTieneMateriales(user_index, recipe_item, 3) == false);
        }

        SUBCASE("PuedeConstruir con skill suficiente") {
            UserList[user_index].clase = eClass::Worker; // ModHerreriA = 1.0
            UserList[user_index].Stats.UserSkills[static_cast<std::size_t>(eSkill::Herreria)] = 20;
            CHECK(Trabajo::PuedeConstruir(user_index, recipe_item, 2) == true);

            // Con menos skill falla
            UserList[user_index].Stats.UserSkills[static_cast<std::size_t>(eSkill::Herreria)] = 19;
            CHECK(Trabajo::PuedeConstruir(user_index, recipe_item, 2) == false);
        }

        SUBCASE("HerreroQuitarMateriales") {
            Trabajo::HerreroQuitarMateriales(user_index, recipe_item, 2);
            CHECK(UserList[user_index].Invent.Object[1].Amount == 0);
            CHECK(UserList[user_index].Invent.Object[2].Amount == 0);
        }

        SUBCASE("PuedeConstruirHerreria y PuedeConstruirCarpintero") {
            ArmasHerrero = {0, 10, 20, 30};
            ArmadurasHerrero = {0, 40, 50};
            ObjCarpintero = {0, 100, 200};

            CHECK(Trabajo::PuedeConstruirHerreria(20) == true);
            CHECK(Trabajo::PuedeConstruirHerreria(40) == true);
            CHECK(Trabajo::PuedeConstruirHerreria(99) == false);

            CHECK(Trabajo::PuedeConstruirCarpintero(100) == true);
            CHECK(Trabajo::PuedeConstruirCarpintero(50) == false);
        }

        SUBCASE("CarpinteroTieneMateriales y CarpinteroQuitarMateriales") {
            std::int16_t bow_item = 80;
            ObjDataList[bow_item] = ObjData{};
            ObjDataList[bow_item].Madera = 5;
            ObjDataList[bow_item].MaderaElfica = 2;

            UserList[user_index].Invent.Object[3].ObjIndex = Leña;
            UserList[user_index].Invent.Object[3].Amount = 10;

            UserList[user_index].Invent.Object[4].ObjIndex = LeñaElfica;
            UserList[user_index].Invent.Object[4].Amount = 4;

            CHECK(Trabajo::CarpinteroTieneMateriales(user_index, bow_item, 2) == true);
            CHECK(Trabajo::CarpinteroTieneMateriales(user_index, bow_item, 3) == false);

            Trabajo::CarpinteroQuitarMateriales(user_index, bow_item, 2);
            CHECK(UserList[user_index].Invent.Object[3].Amount == 0);
            CHECK(UserList[user_index].Invent.Object[4].Amount == 0);
        }
    }
}

TEST_SUITE("Trabajo - G2: Recolección de Recursos") {

    TEST_CASE("DoTalar: Extracción, Azar y Rendimiento por Clase/Nivel") {
        std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].Stats.MinSta = 50;
        UserList[user_index].Stats.ELV = 1;

        Trabajo::TrabajoHooks hooks;
        bool item_added = false;
        bool dropped_on_floor = false;
        bool skill_raised = false;
        bool skill_success = false;
        Obj last_item{};

        hooks.MeterItemEnInventario = [&](std::int16_t u, const Obj& item) {
            item_added = true;
            last_item = item;
            return true;
        };

        hooks.TirarItemAlPiso = [&](const WorldPos& pos, const Obj& item) {
            dropped_on_floor = true;
            last_item = item;
        };

        hooks.SubirSkill = [&](std::int16_t u, std::int16_t skill, bool exito) {
            CHECK(u == user_index);
            CHECK(skill == static_cast<std::int16_t>(eSkill::Talar));
            skill_raised = true;
            skill_success = exito;
        };

        SUBCASE("Éxito en Tala para clase no Trabajador (1 leña fija)") {
            UserList[user_index].clase = eClass::Warrior;
            UserList[user_index].Stats.UserSkills[static_cast<std::size_t>(eSkill::Talar)] = 50;

            // Forzar res <= 6 en tirada
            hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
                return 1;
            };
            Trabajo::SetHooks(hooks);

            Trabajo::DoTalar(user_index, false);

            CHECK(item_added == true);
            CHECK(last_item.ObjIndex == Leña);
            CHECK(last_item.Amount == 1);
            CHECK(skill_raised == true);
            CHECK(skill_success == true);
            CHECK(UserList[user_index].Reputacion.PlebeRep == vlProleta);
            CHECK(UserList[user_index].Counters.Trabajando == 1);
            // Estamina descontada para clase general (EsfuerzoTalarGeneral)
            CHECK(UserList[user_index].Stats.MinSta == 50 - EsfuerzoTalarGeneral);

            Trabajo::ResetHooks();
        }

        SUBCASE("Éxito en Tala para Trabajador Novato Nivel 1 (quirk de 1 a 2 leñas)") {
            UserList[user_index].clase = eClass::Worker;
            UserList[user_index].Stats.ELV = 1;
            UserList[user_index].Stats.UserSkills[static_cast<std::size_t>(eSkill::Talar)] = 100;

            int call_count = 0;
            hooks.RandomNumber = [&](std::int32_t min, std::int32_t max) {
                call_count++;
                if (call_count == 1) return 1; // res <= 6
                return 2; // RandomNumber(1, CantidadItems = 2) -> 2
            };
            Trabajo::SetHooks(hooks);

            Trabajo::DoTalar(user_index, true); // Leña élfica

            CHECK(item_added == true);
            CHECK(last_item.ObjIndex == LeñaElfica);
            CHECK(last_item.Amount == 2);
            CHECK(skill_success == true);
            CHECK(UserList[user_index].Stats.MinSta == 50 - EsfuerzoTalarLeñador);

            Trabajo::ResetHooks();
        }

        SUBCASE("Desborde a suelo cuando mochila está llena") {
            UserList[user_index].clase = eClass::Warrior;
            hooks.MeterItemEnInventario = [](std::int16_t u, const Obj& item) {
                return false; // Mochila llena
            };
            hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
                return 1;
            };
            Trabajo::SetHooks(hooks);

            Trabajo::DoTalar(user_index, false);

            CHECK(item_added == false);
            CHECK(dropped_on_floor == true);
            CHECK(last_item.ObjIndex == Leña);

            Trabajo::ResetHooks();
        }

        SUBCASE("Falla en tirada de Tala") {
            UserList[user_index].clase = eClass::Warrior;
            hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
                return 100; // res > 6 -> Falla
            };
            Trabajo::SetHooks(hooks);

            Trabajo::DoTalar(user_index, false);

            CHECK(item_added == false);
            CHECK(skill_raised == true);
            CHECK(skill_success == false);

            Trabajo::ResetHooks();
        }
    }

    TEST_CASE("DoMineria: Yacimiento objetivo y entrega de minerales") {
        if (ObjDataList.size() < 200) {
            ObjDataList.resize(200);
        }

        std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].Stats.MinSta = 50;
        UserList[user_index].clase = eClass::Worker;
        UserList[user_index].Stats.ELV = 14; // MaxItemsConstruibles = 2 -> CantidadItems = 3

        std::int16_t yacimiento_obj = 150;
        std::int16_t mineral_id = 88;
        ObjDataList[yacimiento_obj] = ObjData{};
        ObjDataList[yacimiento_obj].MineralIndex = mineral_id;
        UserList[user_index].flags.TargetObj = yacimiento_obj;

        Trabajo::TrabajoHooks hooks;
        bool item_added = false;
        Obj last_item{};

        hooks.MeterItemEnInventario = [&](std::int16_t u, const Obj& item) {
            item_added = true;
            last_item = item;
            return true;
        };

        int call_count = 0;
        hooks.RandomNumber = [&](std::int32_t min, std::int32_t max) {
            call_count++;
            if (call_count == 1) return 1; // res <= 5
            return 3; // RandomNumber(1, 3) -> 3
        };

        Trabajo::SetHooks(hooks);

        Trabajo::DoMineria(user_index);

        CHECK(item_added == true);
        CHECK(last_item.ObjIndex == mineral_id);
        CHECK(last_item.Amount == 3);
        CHECK(UserList[user_index].Stats.MinSta == 50 - EsfuerzoExcavarMinero);
        CHECK(UserList[user_index].Counters.Trabajando == 1);

        Trabajo::ResetHooks();
    }

    TEST_CASE("DoPescar y DoPescarRed") {
        std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].Stats.MinSta = 50;

        Trabajo::TrabajoHooks hooks;
        bool item_added = false;
        Obj last_item{};

        hooks.MeterItemEnInventario = [&](std::int16_t u, const Obj& item) {
            item_added = true;
            last_item = item;
            return true;
        };

        SUBCASE("DoPescar con caña") {
            UserList[user_index].clase = eClass::Warrior;
            hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
                return 1; // Exito
            };
            Trabajo::SetHooks(hooks);

            Trabajo::DoPescar(user_index);

            CHECK(item_added == true);
            CHECK(last_item.ObjIndex == Pescado);
            CHECK(last_item.Amount == 1);
            CHECK(UserList[user_index].Stats.MinSta == 50 - EsfuerzoPescarGeneral);

            Trabajo::ResetHooks();
        }

        SUBCASE("DoPescarRed para Pescador (variedad de peces)") {
            UserList[user_index].clase = eClass::Worker;
            int call_count = 0;
            hooks.RandomNumber = [&](std::int32_t min, std::int32_t max) {
                call_count++;
                if (call_count == 1) return 1; // res < 6
                if (call_count == 2) return 4; // Amount = 4
                return 2; // Index de pez = 2 -> PESCADO3
            };
            Trabajo::SetHooks(hooks);

            Trabajo::DoPescarRed(user_index);

            CHECK(item_added == true);
            CHECK(last_item.ObjIndex == PESCADO3);
            CHECK(last_item.Amount == 4);
            CHECK(UserList[user_index].Stats.MinSta == 50 - EsfuerzoPescarPescador);

            Trabajo::ResetHooks();
        }
    }
}

TEST_SUITE("Trabajo - G3: Manufactura, Fundición y Upgrades") {

    TEST_CASE("Fundición de Minerales (DoLingotes / FundirMineral)") {
        if (ObjDataList.size() < 400) {
            ObjDataList.resize(400);
        }
        if (UserList.size() < 2) {
            UserList.resize(2);
        }

        std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].clase = eClass::Worker;
        UserList[user_index].CurrentInventorySlots = 10;
        UserList[user_index].Invent.NroItems = 1;

        std::int16_t mineral_slot = 2;
        UserList[user_index].flags.TargetObjInvSlot = mineral_slot;

        Trabajo::TrabajoHooks hooks;
        bool lingote_added = false;
        Obj last_item{};

        hooks.MeterItemEnInventario = [&](std::int16_t u, const Obj& item) {
            lingote_added = true;
            last_item = item;
            return true;
        };

        SUBCASE("FundirMineral con 50 minerales de Hierro -> 1 Lingote de Hierro") {
            UserList[user_index].flags.TargetObjInvIndex = Trabajo::MineralHierro;
            ObjDataList[Trabajo::MineralHierro] = ObjData{};
            ObjDataList[Trabajo::MineralHierro].OBJType = eOBJType::otMinerales;
            ObjDataList[Trabajo::MineralHierro].MinSkill = 10;
            UserList[user_index].Stats.UserSkills[static_cast<std::size_t>(eSkill::Mineria)] = 20;

            UserList[user_index].Invent.Object[mineral_slot].ObjIndex = Trabajo::MineralHierro;
            UserList[user_index].Invent.Object[mineral_slot].Amount = 50;

            Trabajo::SetHooks(hooks);

            Trabajo::FundirMineral(user_index);

            CHECK(lingote_added == true);
            CHECK(last_item.ObjIndex == LingoteHierro);
            CHECK(last_item.Amount == 1);
            CHECK(UserList[user_index].Invent.Object[mineral_slot].Amount == 0);
            CHECK(UserList[user_index].Invent.Object[mineral_slot].ObjIndex == 0);

            Trabajo::ResetHooks();
        }

        SUBCASE("Rechazo por tener 49 minerales (menos de 50)") {
            UserList[user_index].flags.TargetObjInvIndex = Trabajo::MineralHierro;
            ObjDataList[Trabajo::MineralHierro] = ObjData{};
            ObjDataList[Trabajo::MineralHierro].OBJType = eOBJType::otMinerales;
            ObjDataList[Trabajo::MineralHierro].MinSkill = 10;
            UserList[user_index].Stats.UserSkills[static_cast<std::size_t>(eSkill::Mineria)] = 20;

            UserList[user_index].Invent.Object[mineral_slot].ObjIndex = Trabajo::MineralHierro;
            UserList[user_index].Invent.Object[mineral_slot].Amount = 49;

            Trabajo::SetHooks(hooks);

            Trabajo::FundirMineral(user_index);

            CHECK(lingote_added == false);
            CHECK(UserList[user_index].Invent.Object[mineral_slot].Amount == 49);

            Trabajo::ResetHooks();
        }
    }

    TEST_CASE("FundirArmas y DoFundir: Recuperación de lingotes") {
        if (ObjDataList.size() < 500) {
            ObjDataList.resize(500);
        }
        if (UserList.size() < 2) {
            UserList.resize(2);
        }

        std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].clase = eClass::Worker;
        UserList[user_index].CurrentInventorySlots = 10;
        UserList[user_index].Invent.NroItems = 1;

        std::int16_t weapon_item = 450;
        std::uint8_t weapon_slot = 3;

        ObjDataList[weapon_item] = ObjData{};
        ObjDataList[weapon_item].OBJType = eOBJType::otWeapon;
        ObjDataList[weapon_item].SkHerreria = 30;
        ObjDataList[weapon_item].LingH = 100; // 100 lingotes de hierro
        ObjDataList[weapon_item].LingP = 50;  // 50 lingotes de plata

        UserList[user_index].flags.TargetObjInvIndex = weapon_item;
        UserList[user_index].flags.TargetObjInvSlot = weapon_slot;

        UserList[user_index].Invent.Object[weapon_slot].ObjIndex = weapon_item;
        UserList[user_index].Invent.Object[weapon_slot].Amount = 1;
        UserList[user_index].Stats.UserSkills[static_cast<std::size_t>(eSkill::Herreria)] = 50;

        Trabajo::TrabajoHooks hooks;
        std::vector<Obj> added_items;

        hooks.MeterItemEnInventario = [&](std::int16_t u, const Obj& item) {
            added_items.push_back(item);
            return true;
        };

        hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
            return 20; // 20% de recuperación
        };

        Trabajo::SetHooks(hooks);

        Trabajo::FundirArmas(user_index);

        // La pieza original de arma fue destruida
        CHECK(UserList[user_index].Invent.Object[weapon_slot].Amount == 0);
        CHECK(UserList[user_index].Invent.Object[weapon_slot].ObjIndex == 0);

        // Se entregaron 20 lingotes de hierro (100 * 20%) y 10 lingotes de plata (50 * 20%)
        REQUIRE(added_items.size() == 2);
        CHECK(added_items[0].ObjIndex == LingoteHierro);
        CHECK(added_items[0].Amount == 20);
        CHECK(added_items[1].ObjIndex == LingotePlata);
        CHECK(added_items[1].Amount == 10);
        CHECK(UserList[user_index].Counters.Trabajando == 1);

        Trabajo::ResetHooks();
    }

    TEST_CASE("HerreroConstruirItem y CarpinteroConstruirItem") {
        if (ObjDataList.size() < 600) {
            ObjDataList.resize(600);
        }

        if (UserList.size() < 2) {
            UserList.resize(2);
        }

        std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].clase = eClass::Worker;
        UserList[user_index].Stats.MinSta = 50;
        UserList[user_index].CurrentInventorySlots = 10;
        UserList[user_index].Invent.NroItems = 2;

        ArmasHerrero = {0, 550};
        ObjCarpintero = {0, 580};

        Trabajo::TrabajoHooks hooks;
        bool wave_sent = false;
        std::uint8_t wave_num = 0;
        Obj crafted_item{};

        hooks.SendAreaWave = [&](std::int16_t u, std::uint8_t wave) {
            wave_sent = true;
            wave_num = wave;
        };

        hooks.MeterItemEnInventario = [&](std::int16_t u, const Obj& item) {
            crafted_item = item;
            return true;
        };

        SUBCASE("Fabricación en Herrería") {
            std::int16_t sword_id = 550;
            ObjDataList[sword_id] = ObjData{};
            ObjDataList[sword_id].OBJType = eOBJType::otWeapon;
            ObjDataList[sword_id].LingH = 10;
            ObjDataList[sword_id].SkHerreria = 20;

            UserList[user_index].Construir.PorCiclo = 1;
            UserList[user_index].Construir.Cantidad = 1;
            UserList[user_index].Stats.UserSkills[static_cast<std::size_t>(eSkill::Herreria)] = 30;

            UserList[user_index].Invent.Object[1].ObjIndex = LingoteHierro;
            UserList[user_index].Invent.Object[1].Amount = 15;

            Trabajo::SetHooks(hooks);

            Trabajo::HerreroConstruirItem(user_index, sword_id);

            CHECK(crafted_item.ObjIndex == sword_id);
            CHECK(crafted_item.Amount == 1);
            CHECK(UserList[user_index].Invent.Object[1].Amount == 5); // 15 - 10 = 5
            CHECK(wave_sent == true);
            CHECK(wave_num == MARTILLOHERRERO);
            CHECK(UserList[user_index].Stats.MinSta == 50 - Trabajo::GASTO_ENERGIA_TRABAJADOR);

            Trabajo::ResetHooks();
        }

        SUBCASE("Fabricación en Carpintería") {
            std::int16_t staff_id = 580;
            ObjDataList[staff_id] = ObjData{};
            ObjDataList[staff_id].OBJType = eOBJType::otWeapon;
            ObjDataList[staff_id].Madera = 15;
            ObjDataList[staff_id].SkCarpinteria = 15;

            UserList[user_index].Construir.PorCiclo = 1;
            UserList[user_index].Construir.Cantidad = 1;
            UserList[user_index].Stats.UserSkills[static_cast<std::size_t>(eSkill::Carpinteria)] = 20;

            UserList[user_index].Invent.Object[2].ObjIndex = Leña;
            UserList[user_index].Invent.Object[2].Amount = 20;

            Trabajo::SetHooks(hooks);

            Trabajo::CarpinteroConstruirItem(user_index, staff_id);

            CHECK(crafted_item.ObjIndex == staff_id);
            CHECK(crafted_item.Amount == 1);
            CHECK(UserList[user_index].Invent.Object[2].Amount == 5); // 20 - 15 = 5
            CHECK(wave_sent == true);
            CHECK(wave_num == LABUROCARPINTERO);

            Trabajo::ResetHooks();
        }
    }

    TEST_CASE("DoUpgrade: Mejora de Equipamiento") {
        if (ObjDataList.size() < 700) {
            ObjDataList.resize(700);
        }
        if (UserList.size() < 2) {
            UserList.resize(2);
        }

        std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].clase = eClass::Worker;
        UserList[user_index].Stats.MinSta = 50;
        UserList[user_index].CurrentInventorySlots = 10;
        UserList[user_index].Invent.NroItems = 3;

        std::int16_t base_sword = 600;
        std::int16_t upgraded_sword = 601;

        ArmasHerrero = {0, upgraded_sword};

        ObjDataList[base_sword] = ObjData{};
        ObjDataList[base_sword].OBJType = eOBJType::otWeapon;
        ObjDataList[base_sword].Upgrade = upgraded_sword;
        ObjDataList[base_sword].LingH = 5;

        ObjDataList[upgraded_sword] = ObjData{};
        ObjDataList[upgraded_sword].OBJType = eOBJType::otWeapon;
        ObjDataList[upgraded_sword].SkHerreria = 25;

        // Equipar martillo de herrero
        UserList[user_index].Invent.WeaponEqpObjIndex = MARTILLO_HERRERO;
        UserList[user_index].Stats.UserSkills[static_cast<std::size_t>(eSkill::Herreria)] = 30;

        // Cargar espada base y lingotes
        UserList[user_index].Invent.Object[1].ObjIndex = base_sword;
        UserList[user_index].Invent.Object[1].Amount = 1;

        UserList[user_index].Invent.Object[2].ObjIndex = LingoteHierro;
        UserList[user_index].Invent.Object[2].Amount = 10;

        Trabajo::TrabajoHooks hooks;
        Obj upgraded_item{};

        hooks.MeterItemEnInventario = [&](std::int16_t u, const Obj& item) {
            upgraded_item = item;
            return true;
        };

        Trabajo::SetHooks(hooks);

        Trabajo::DoUpgrade(user_index, base_sword);

        // Entregó espada mejorada
        CHECK(upgraded_item.ObjIndex == upgraded_sword);
        CHECK(upgraded_item.Amount == 1);

        // Descontó espada base y 5 lingotes de hierro (10 - 5 = 5)
        Trabajo::ResetHooks();
    }
}

TEST_SUITE("Trabajo - G4: Habilidades de Combate, Hurto, Desarme y Domesticación") {

    TEST_CASE("DoApuñalar: Multiplicador Asesino (1.5x) vs Otras Clases (1.4x)") {
        if (UserList.size() < 3) {
            UserList.resize(3);
        }

        std::int16_t attacker = 1;
        std::int16_t victim_user = 2;

        UserList[attacker] = User{};
        UserList[attacker].Stats.UserSkills[static_cast<std::size_t>(eSkill::Apuñalar)] = 100;

        UserList[victim_user] = User{};
        UserList[victim_user].Stats.MinHp = 500;
        UserList[victim_user].name = "Victima";

        Trabajo::TrabajoHooks hooks;
        hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
            return 0; // Garantiza éxito en la tirada
        };

        SUBCASE("Asesino apuñala a usuario -> 1.5x daño") {
            UserList[attacker].clase = eClass::Assasin;
            UserList[victim_user].Stats.MinHp = 500;

            Trabajo::SetHooks(hooks);
            Trabajo::DoApuñalar(attacker, 0, victim_user, 100);

            CHECK(UserList[victim_user].Stats.MinHp == 500 - 150);
            Trabajo::ResetHooks();
        }

        SUBCASE("Guerrero apuñala a usuario -> 1.4x daño") {
            UserList[attacker].clase = eClass::Warrior;
            UserList[victim_user].Stats.MinHp = 500;

            Trabajo::SetHooks(hooks);
            Trabajo::DoApuñalar(attacker, 0, victim_user, 100);

            CHECK(UserList[victim_user].Stats.MinHp == 500 - 140);
            Trabajo::ResetHooks();
        }
    }

    TEST_CASE("Bug #42 en DoGolpeCritico: Reducción del daño al 75%") {
        if (UserList.size() < 3) {
            UserList.resize(3);
        }
        if (ObjDataList.size() < 500) {
            ObjDataList.resize(500);
        }

        std::int16_t bandit = 1;
        std::int16_t victim_user = 2;
        std::int16_t espada_vikinga = 480;

        UserList[bandit] = User{};
        UserList[bandit].clase = eClass::Bandit;
        UserList[bandit].Invent.WeaponEqpSlot = 1;
        UserList[bandit].Invent.WeaponEqpObjIndex = espada_vikinga;
        UserList[bandit].Stats.UserSkills[static_cast<std::size_t>(eSkill::Wrestling)] = 100;

        ObjDataList[espada_vikinga] = ObjData{};
        ObjDataList[espada_vikinga].name = "Espada Vikinga";

        UserList[victim_user] = User{};
        UserList[victim_user].Stats.MinHp = 300;
        UserList[victim_user].name = "VictimaCritico";

        Trabajo::TrabajoHooks hooks;
        hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
            return 0; // Garantiza éxito crítico
        };

        Trabajo::SetHooks(hooks);
        Trabajo::DoGolpeCritico(bandit, 0, victim_user, 100);

        // Con Bug #42 replicado, el daño de 100 se convierte en 75 (100 * 0.75)
        CHECK(UserList[victim_user].Stats.MinHp == 300 - 75);

        Trabajo::ResetHooks();
    }

    TEST_CASE("DoAcuchillar: Incremento a 1.25x por pirata") {
        if (UserList.size() < 3) {
            UserList.resize(3);
        }

        std::int16_t pirate = 1;
        std::int16_t victim_user = 2;

        UserList[pirate] = User{};
        UserList[pirate].clase = eClass::Pirat;
        UserList[pirate].Invent.WeaponEqpSlot = 1;

        UserList[victim_user] = User{};
        UserList[victim_user].Stats.MinHp = 200;

        Trabajo::TrabajoHooks hooks;
        hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
            return 0; // Exito
        };

        Trabajo::SetHooks(hooks);
        Trabajo::DoAcuchillar(pirate, 0, victim_user, 100);

        // PROB_ACUCHILLAR y DAÑO_ACUCHILLAR (1.25x)
        CHECK(UserList[victim_user].Stats.MinHp == 200 - static_cast<std::int16_t>(100 * DAÑO_ACUCHILLAR));

        Trabajo::ResetHooks();
    }

    TEST_CASE("ObjEsRobable: Exclusión de llaves, barcos, equipados y faccionarios") {
        if (UserList.size() < 2) {
            UserList.resize(2);
        }
        if (ObjDataList.size() < 100) {
            ObjDataList.resize(100);
        }

        std::int16_t victim = 1;
        UserList[victim] = User{};

        std::int16_t item_key = 10;
        std::int16_t item_boat = 11;
        std::int16_t item_armada = 12;
        std::int16_t item_potion = 13;

        ObjDataList[item_key].OBJType = eOBJType::otLlaves;
        ObjDataList[item_boat].OBJType = eOBJType::otBarcos;
        ObjDataList[item_armada].OBJType = eOBJType::otWeapon;
        ObjDataList[item_armada].Real = 1;
        ObjDataList[item_potion].OBJType = eOBJType::otPociones;

        UserList[victim].Invent.Object[1].ObjIndex = item_key;
        UserList[victim].Invent.Object[2].ObjIndex = item_boat;
        UserList[victim].Invent.Object[3].ObjIndex = item_armada;
        UserList[victim].Invent.Object[4].ObjIndex = item_potion;
        UserList[victim].Invent.Object[4].Equipped = 1;
        UserList[victim].Invent.Object[5].ObjIndex = item_potion;
        UserList[victim].Invent.Object[5].Equipped = 0;

        CHECK(Trabajo::ObjEsRobable(victim, 1) == false); // Llaves
        CHECK(Trabajo::ObjEsRobable(victim, 2) == false); // Barcos
        CHECK(Trabajo::ObjEsRobable(victim, 3) == false); // Faccionario
        CHECK(Trabajo::ObjEsRobable(victim, 4) == false); // Equipado
        CHECK(Trabajo::ObjEsRobable(victim, 5) == true);  // Poción no equipada
    }

    TEST_CASE("DoRobar: Transferencia de oro y criminalidad") {
        if (UserList.size() < 3) {
            UserList.resize(3);
        }

        std::int16_t thief = 1;
        std::int16_t victim = 2;

        UserList[thief] = User{};
        UserList[thief].clase = eClass::Thief;
        UserList[thief].Stats.MinSta = 50;
        UserList[thief].Stats.ELV = 10;
        UserList[thief].Invent.AnilloEqpObjIndex = Trabajo::GUANTE_HURTO;
        UserList[thief].Stats.UserSkills[static_cast<std::size_t>(eSkill::Robar)] = 100;
        UserList[thief].Stats.GLD = 0;

        UserList[victim] = User{};
        UserList[victim].Stats.GLD = 1000;
        UserList[victim].name = "VictimaOro";

        Trabajo::TrabajoHooks hooks;
        bool became_criminal = false;

        hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
            if (min == 1 && max == 5) return 1; // res < 3 -> éxito en robo
            if (min == 1 && max == 50) return 30; // > 25 para ir a la rama de robar oro
            return 500; // 500 monedas de oro
        };

        hooks.VolverCriminal = [&](std::int16_t u) {
            became_criminal = true;
        };

        Trabajo::SetHooks(hooks);

        Trabajo::DoRobar(thief, victim);

        CHECK(UserList[thief].Stats.GLD == 500);
        CHECK(UserList[victim].Stats.GLD == 500);
        CHECK(became_criminal == true);

        Trabajo::ResetHooks();
    }

    TEST_CASE("Desarmar y DoDesequipar: Desarme con Wrestling") {
        if (UserList.size() < 3) {
            UserList.resize(3);
        }

        std::int16_t attacker = 1;
        std::int16_t victim = 2;

        UserList[attacker] = User{};
        UserList[attacker].Stats.UserSkills[static_cast<std::size_t>(eSkill::Wrestling)] = 100;
        UserList[attacker].Stats.ELV = 30;
        UserList[attacker].Invent.AnilloEqpObjIndex = Trabajo::GUANTE_HURTO;
        UserList[attacker].Invent.WeaponEqpObjIndex = 0;

        UserList[victim] = User{};
        UserList[victim].Invent.WeaponEqpSlot = 2;
        UserList[victim].Invent.WeaponEqpObjIndex = 50;

        Trabajo::TrabajoHooks hooks;
        bool unequipped = false;
        std::uint8_t unequipped_slot = 0;

        hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
            return 1; // Garantiza éxito
        };

        hooks.Desequipar = [&](std::int16_t u, std::uint8_t slot) {
            unequipped = true;
            unequipped_slot = slot;
        };

        Trabajo::SetHooks(hooks);

        Trabajo::Desarmar(attacker, victim);

        CHECK(unequipped == true);
        CHECK(unequipped_slot == 2);

        Trabajo::ResetHooks();
    }

    TEST_CASE("DoDomar: Domesticación y límite de 2 mascotas por tipo de NPC") {
        if (UserList.size() < 2) {
            UserList.resize(2);
        }

        std::int16_t user = 1;
        std::int16_t wolf_npc1 = 2;
        std::int16_t wolf_npc2 = 3;
        std::int16_t wolf_npc3 = 4;

        UserList[user] = User{};
        UserList[user].Stats.UserAtributos[static_cast<std::size_t>(eAtributos::Carisma)] = 18;
        UserList[user].Stats.UserSkills[static_cast<std::size_t>(eSkill::Domar)] = 100;
        UserList[user].NroMascotas = 0;

        Npclist[wolf_npc1] = npc{};
        Npclist[wolf_npc1].Numero = 10;
        Npclist[wolf_npc1].flags.Domable = 100;

        Npclist[wolf_npc2] = npc{};
        Npclist[wolf_npc2].Numero = 10;
        Npclist[wolf_npc2].flags.Domable = 100;

        Npclist[wolf_npc3] = npc{};
        Npclist[wolf_npc3].Numero = 10;
        Npclist[wolf_npc3].flags.Domable = 100;

        Trabajo::TrabajoHooks hooks;
        hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
            return 1; // Garantiza domado
        };

        Trabajo::SetHooks(hooks);

        // Domar lobo 1
        Trabajo::DoDomar(user, wolf_npc1);
        CHECK(UserList[user].NroMascotas == 1);
        CHECK(Npclist[wolf_npc1].MaestroUser == user);

        // Domar lobo 2
        Trabajo::DoDomar(user, wolf_npc2);
        CHECK(UserList[user].NroMascotas == 2);
        CHECK(Npclist[wolf_npc2].MaestroUser == user);

        // Intentar domar lobo 3 (mismo tipo 10) -> Debe ser rechazado por límite de 2 mascotas del mismo tipo
        Trabajo::DoDomar(user, wolf_npc3);
        Trabajo::ResetHooks();
    }
}

TEST_SUITE("Trabajo - G5: Gestión de Estados y Entorno") {

    TEST_CASE("TratarDeHacerFogata: Consumo de leña y creación de fogata") {
        if (UserList.size() < 2) {
            UserList.resize(2);
        }
        if (MapData.size() < 10000) {
            MapData.resize(10000);
        }

        std::int16_t user = 1;
        std::int16_t map = 1;
        std::int16_t x = 50;
        std::int16_t y = 50;

        UserList[user] = User{};
        UserList[user].Pos = WorldPos{map, x, y};
        UserList[user].Stats.UserSkills[static_cast<std::size_t>(eSkill::Supervivencia)] = 50;

        std::size_t idx = static_cast<std::size_t>((map - 1) * 10000 + (y - 1) * 100 + (x - 1));

        Trabajo::TrabajoHooks hooks;
        hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
            return 1; // Éxito en hacer fogata
        };

        SUBCASE("Crear fogata con 9 leñas -> genera 3 fogatas apagadas") {
            MapData[idx] = MapBlock{};
            MapData[idx].ObjInfo.ObjIndex = 58; // Leña
            MapData[idx].ObjInfo.Amount = 9;

            Trabajo::SetHooks(hooks);

            Trabajo::TratarDeHacerFogata(map, x, y, user);

            CHECK(MapData[idx].ObjInfo.ObjIndex == FOGATA_APAG);
            CHECK(MapData[idx].ObjInfo.Amount == 3); // 9 / 3 = 3
            CHECK(UserList[user].flags.TargetObj == FOGATA_APAG);

            Trabajo::ResetHooks();
        }

        SUBCASE("Rechazo por tener menos de 3 leñas (2 leñas)") {
            MapData[idx] = MapBlock{};
            MapData[idx].ObjInfo.ObjIndex = 58; // Leña
            MapData[idx].ObjInfo.Amount = 2;

            Trabajo::SetHooks(hooks);

            Trabajo::TratarDeHacerFogata(map, x, y, user);

            CHECK(MapData[idx].ObjInfo.ObjIndex == 58);
            CHECK(MapData[idx].ObjInfo.Amount == 2);

            Trabajo::ResetHooks();
        }
    }

    TEST_CASE("DoNavega: Alternancia de navegación y requisitos de skill") {
        if (UserList.size() < 2) {
            UserList.resize(2);
        }

        std::int16_t user = 1;
        UserList[user] = User{};
        UserList[user].clase = eClass::Pirat; // ModNavegacion = 1.0
        UserList[user].Stats.UserSkills[static_cast<std::size_t>(eSkill::Navegacion)] = 60;

        std::int16_t boat_slot = 1;
        UserList[user].Invent.Object[boat_slot].ObjIndex = 200;

        ObjData boat_obj{};
        boat_obj.MinSkill = 50;

        Trabajo::TrabajoHooks hooks;
        bool nav_toggled = false;

        hooks.WriteNavigateToggle = [&](std::int16_t u) {
            nav_toggled = true;
        };

        Trabajo::SetHooks(hooks);

        // Equipar barco (comienza a navegar)
        Trabajo::DoNavega(user, boat_obj, boat_slot);
        CHECK(UserList[user].flags.Navegando == 1);
        CHECK(nav_toggled == true);

        // Desequipar barco (termina de navegar)
        nav_toggled = false;
        Trabajo::DoNavega(user, boat_obj, boat_slot);
        CHECK(UserList[user].flags.Navegando == 0);
        CHECK(nav_toggled == true);

        Trabajo::ResetHooks();
    }

    TEST_CASE("DoOcultarse y DoPermanecerOculto: Sigilo e invisibilidad") {
        if (UserList.size() < 2) {
            UserList.resize(2);
        }

        std::int16_t user = 1;
        UserList[user] = User{};
        UserList[user].Stats.UserSkills[static_cast<std::size_t>(eSkill::Ocultarse)] = 100;

        Trabajo::TrabajoHooks hooks;
        bool set_invisible = false;
        bool invisible_val = false;

        hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
            return 1; // Éxito
        };

        hooks.SetInvisible = [&](std::int16_t u, std::int16_t char_idx, bool invi) {
            set_invisible = true;
            invisible_val = invi;
        };

        Trabajo::SetHooks(hooks);

        // Ocultarse exitoso
        Trabajo::DoOcultarse(user);
        CHECK(UserList[user].flags.Oculto == 1);
        CHECK(set_invisible == true);
        CHECK(invisible_val == true);

        // Vencer temporizador de ocultamiento
        UserList[user].Counters.TiempoOculto = 1;
        set_invisible = false;

        Trabajo::DoPermanecerOculto(user);
        CHECK(UserList[user].flags.Oculto == 0);
        CHECK(set_invisible == true);
        CHECK(invisible_val == false);

        Trabajo::ResetHooks();
    }

    TEST_CASE("DoAdminInvisible: Alternancia del estado invisible de GM") {
        if (UserList.size() < 2) {
            UserList.resize(2);
        }

        std::int16_t user = 1;
        UserList[user] = User{};
        UserList[user].char_appearance.body = 10;
        UserList[user].char_appearance.Head = 5;

        Trabajo::TrabajoHooks hooks;
        bool invi_toggled = false;
        bool invi_val = false;

        hooks.SetInvisible = [&](std::int16_t u, std::int16_t char_idx, bool invi) {
            invi_toggled = true;
            invi_val = invi;
        };

        Trabajo::SetHooks(hooks);

        // Activar AdminInvisible
        Trabajo::DoAdminInvisible(user);
        CHECK(UserList[user].flags.AdminInvisible == 1);
        CHECK(UserList[user].char_appearance.body == 0);
        CHECK(UserList[user].char_appearance.Head == 0);
        CHECK(invi_toggled == true);
        CHECK(invi_val == true);

        // Desactivar AdminInvisible
        invi_toggled = false;
        Trabajo::DoAdminInvisible(user);
        CHECK(UserList[user].flags.AdminInvisible == 0);
        CHECK(UserList[user].char_appearance.body == 10);
        CHECK(UserList[user].char_appearance.Head == 5);
        CHECK(invi_toggled == true);
        CHECK(invi_val == false);

        Trabajo::ResetHooks();
    }

    TEST_CASE("DoMeditar: Regeneración de maná y detención al MaxMAN") {
        if (UserList.size() < 2) {
            UserList.resize(2);
        }

        std::int16_t user = 1;
        UserList[user] = User{};
        UserList[user].Stats.UserSkills[static_cast<std::size_t>(eSkill::Meditar)] = 100;
        UserList[user].Counters.tInicioMeditar = 0;
        UserList[user].Stats.MaxMAN = 100;
        UserList[user].Stats.MinMAN = 50;
        UserList[user].flags.Meditando = true;

        Trabajo::TrabajoHooks hooks;
        hooks.GetTickCount = []() -> std::uint32_t {
            return 10000; // Supera TIEMPO_INICIOMEDITAR
        };
        hooks.RandomNumber = [](std::int32_t min, std::int32_t max) {
            return 1; // Éxito en la tirada
        };

        bool meditate_stopped = false;
        hooks.WriteMeditateToggle = [&](std::int16_t u) {
            meditate_stopped = true;
        };

        Trabajo::SetHooks(hooks);

        // Meditar 1er ciclo -> Recupera maná
        Trabajo::DoMeditar(user);
        CHECK(UserList[user].Stats.MinMAN > 50);

        // Llenar maná al tope y meditar de nuevo -> Debe finalizar la meditación
        UserList[user].Stats.MinMAN = 100;
        Trabajo::DoMeditar(user);

        CHECK(UserList[user].flags.Meditando == false);
        CHECK(meditate_stopped == true);

        Trabajo::ResetHooks();
    }
}
