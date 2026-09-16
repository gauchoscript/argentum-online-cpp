#include "server/ModFacciones.hpp"
#include "server/Declares.hpp"

#include <doctest/doctest.h>
#include <vector>

TEST_SUITE("ModFacciones - Fase 1: Infraestructura y Títulos") {

    TEST_CASE("GetArmourAmount - Paridad exacta de unidades por rango") {
        SUBCASE("Rango 1") {
            CHECK(ModFacciones::GetArmourAmount(1, eTipoDefArmors::ieBaja) == 10);
            CHECK(ModFacciones::GetArmourAmount(1, eTipoDefArmors::ieMedia) == 2);
            CHECK(ModFacciones::GetArmourAmount(1, eTipoDefArmors::ieAlta) == 1);
        }

        SUBCASE("Rango 4") {
            CHECK(ModFacciones::GetArmourAmount(4, eTipoDefArmors::ieBaja) == 4);
            CHECK(ModFacciones::GetArmourAmount(4, eTipoDefArmors::ieMedia) == 8);
            CHECK(ModFacciones::GetArmourAmount(4, eTipoDefArmors::ieAlta) == 5);
        }

        SUBCASE("Rango 7") {
            CHECK(ModFacciones::GetArmourAmount(7, eTipoDefArmors::ieBaja) == 2);
            CHECK(ModFacciones::GetArmourAmount(7, eTipoDefArmors::ieMedia) == 5);
            CHECK(ModFacciones::GetArmourAmount(7, eTipoDefArmors::ieAlta) == 9);
        }

        SUBCASE("Rango 10") {
            CHECK(ModFacciones::GetArmourAmount(10, eTipoDefArmors::ieBaja) == 2);
            CHECK(ModFacciones::GetArmourAmount(10, eTipoDefArmors::ieMedia) == 3);
            CHECK(ModFacciones::GetArmourAmount(10, eTipoDefArmors::ieAlta) == 14);
        }

        SUBCASE("Rango 14") {
            CHECK(ModFacciones::GetArmourAmount(14, eTipoDefArmors::ieBaja) == 1);
            CHECK(ModFacciones::GetArmourAmount(14, eTipoDefArmors::ieMedia) == 3);
            CHECK(ModFacciones::GetArmourAmount(14, eTipoDefArmors::ieAlta) == 19);
        }

        SUBCASE("Rango 15") {
            CHECK(ModFacciones::GetArmourAmount(15, eTipoDefArmors::ieBaja) == 1);
            CHECK(ModFacciones::GetArmourAmount(15, eTipoDefArmors::ieMedia) == 3);
            CHECK(ModFacciones::GetArmourAmount(15, eTipoDefArmors::ieAlta) == 20);
        }
    }

    TEST_CASE("TituloReal - Escalafones Armada Real") {
        UserList.resize(10);
        const std::int16_t user_index = 1;

        SUBCASE("Límites fuera de rango de usuario") {
            CHECK(ModFacciones::TituloReal(0) == "");
            CHECK(ModFacciones::TituloReal(100) == "");
        }

        SUBCASE("Mapeo de los 15 escalafones") {
            UserList[user_index].Faccion.RecompensasReal = 0;
            CHECK(ModFacciones::TituloReal(user_index) == "Aprendiz");

            UserList[user_index].Faccion.RecompensasReal = 1;
            CHECK(ModFacciones::TituloReal(user_index) == "Escudero");

            UserList[user_index].Faccion.RecompensasReal = 2;
            CHECK(ModFacciones::TituloReal(user_index) == "Soldado");

            UserList[user_index].Faccion.RecompensasReal = 3;
            CHECK(ModFacciones::TituloReal(user_index) == "Sargento");

            UserList[user_index].Faccion.RecompensasReal = 4;
            CHECK(ModFacciones::TituloReal(user_index) == "Teniente");

            UserList[user_index].Faccion.RecompensasReal = 5;
            CHECK(ModFacciones::TituloReal(user_index) == "Comandante");

            UserList[user_index].Faccion.RecompensasReal = 6;
            CHECK(ModFacciones::TituloReal(user_index) == "Capitán");

            UserList[user_index].Faccion.RecompensasReal = 7;
            CHECK(ModFacciones::TituloReal(user_index) == "Senescal");

            UserList[user_index].Faccion.RecompensasReal = 8;
            CHECK(ModFacciones::TituloReal(user_index) == "Mariscal");

            UserList[user_index].Faccion.RecompensasReal = 9;
            CHECK(ModFacciones::TituloReal(user_index) == "Condestable");

            UserList[user_index].Faccion.RecompensasReal = 10;
            CHECK(ModFacciones::TituloReal(user_index) == "Ejecutor Imperial");

            UserList[user_index].Faccion.RecompensasReal = 11;
            CHECK(ModFacciones::TituloReal(user_index) == "Protector del Reino");

            UserList[user_index].Faccion.RecompensasReal = 12;
            CHECK(ModFacciones::TituloReal(user_index) == "Avatar de la Justicia");

            UserList[user_index].Faccion.RecompensasReal = 13;
            CHECK(ModFacciones::TituloReal(user_index) == "Guardián del Bien");

            UserList[user_index].Faccion.RecompensasReal = 14;
            CHECK(ModFacciones::TituloReal(user_index) == "Campeón de la Luz");

            UserList[user_index].Faccion.RecompensasReal = 20;
            CHECK(ModFacciones::TituloReal(user_index) == "Campeón de la Luz");
        }
    }

    TEST_CASE("TituloCaos - Escalafones Legión Oscura") {
        UserList.resize(10);
        const std::int16_t user_index = 1;

        SUBCASE("Límites fuera de rango de usuario") {
            CHECK(ModFacciones::TituloCaos(0) == "");
            CHECK(ModFacciones::TituloCaos(100) == "");
        }

        SUBCASE("Mapeo de los 15 escalafones") {
            UserList[user_index].Faccion.RecompensasCaos = 0;
            CHECK(ModFacciones::TituloCaos(user_index) == "Acólito");

            UserList[user_index].Faccion.RecompensasCaos = 1;
            CHECK(ModFacciones::TituloCaos(user_index) == "Alma Corrupta");

            UserList[user_index].Faccion.RecompensasCaos = 2;
            CHECK(ModFacciones::TituloCaos(user_index) == "Paria");

            UserList[user_index].Faccion.RecompensasCaos = 3;
            CHECK(ModFacciones::TituloCaos(user_index) == "Condenado");

            UserList[user_index].Faccion.RecompensasCaos = 4;
            CHECK(ModFacciones::TituloCaos(user_index) == "Esbirro");

            UserList[user_index].Faccion.RecompensasCaos = 5;
            CHECK(ModFacciones::TituloCaos(user_index) == "Sanguinario");

            UserList[user_index].Faccion.RecompensasCaos = 6;
            CHECK(ModFacciones::TituloCaos(user_index) == "Corruptor");

            UserList[user_index].Faccion.RecompensasCaos = 7;
            CHECK(ModFacciones::TituloCaos(user_index) == "Heraldo Impío");

            UserList[user_index].Faccion.RecompensasCaos = 8;
            CHECK(ModFacciones::TituloCaos(user_index) == "Caballero de la Oscuridad");

            UserList[user_index].Faccion.RecompensasCaos = 9;
            CHECK(ModFacciones::TituloCaos(user_index) == "Señor del Miedo");

            UserList[user_index].Faccion.RecompensasCaos = 10;
            CHECK(ModFacciones::TituloCaos(user_index) == "Ejecutor Infernal");

            UserList[user_index].Faccion.RecompensasCaos = 11;
            CHECK(ModFacciones::TituloCaos(user_index) == "Protector del Averno");

            UserList[user_index].Faccion.RecompensasCaos = 12;
            CHECK(ModFacciones::TituloCaos(user_index) == "Avatar de la Destrucción");

            UserList[user_index].Faccion.RecompensasCaos = 13;
            CHECK(ModFacciones::TituloCaos(user_index) == "Guardián del Mal");

            UserList[user_index].Faccion.RecompensasCaos = 14;
            CHECK(ModFacciones::TituloCaos(user_index) == "Campeón de la Oscuridad");

            UserList[user_index].Faccion.RecompensasCaos = 99;
            CHECK(ModFacciones::TituloCaos(user_index) == "Campeón de la Oscuridad");
        }
    }

    TEST_CASE("Callbacks - Inyección y Reset") {
        bool log_real_called = false;
        ModFacciones::FactionCallbacks cb{};
        cb.LogEjercitoReal = [&](std::string_view msg) {
            log_real_called = true;
        };

        ModFacciones::SetFactionCallbacks(cb);
        ModFacciones::ResetFactionCallbacks();
        CHECK_FALSE(log_real_called);
    }
}

TEST_SUITE("ModFacciones - Fase 2: Recompensas y Armaduras") {

    TEST_CASE("GiveFactionArmours - Entrega de armaduras en inventario y desborde a piso") {
        UserList.resize(10);
        const std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].clase = static_cast<eClass>(1);
        UserList[user_index].raza = static_cast<eRaza>(1);
        UserList[user_index].Faccion.RecompensasReal = 0;

        ArmadurasFaccion[1][1].Armada[1] = 500;
        ArmadurasFaccion[1][1].Armada[2] = 501;
        ArmadurasFaccion[1][1].Armada[3] = 502;

        SUBCASE("Entrega exitosa en inventario") {
            std::vector<Obj> items_recibidos;
            ModFacciones::FactionCallbacks cb{};
            cb.MeterItemEnInventario = [&](std::int16_t u, const Obj& item) {
                items_recibidos.push_back(item);
                return true;
            };
            ModFacciones::SetFactionCallbacks(cb);

            ModFacciones::GiveFactionArmours(user_index, false);

            REQUIRE(items_recibidos.size() == 3);
            CHECK(items_recibidos[0].ObjIndex == 500);
            CHECK(items_recibidos[0].Amount == 10);
            CHECK(items_recibidos[1].ObjIndex == 501);
            CHECK(items_recibidos[1].Amount == 2);
            CHECK(items_recibidos[2].ObjIndex == 502);
            CHECK(items_recibidos[2].Amount == 1);

            ModFacciones::ResetFactionCallbacks();
        }

        SUBCASE("Desborde a TirarItemAlPiso cuando inventario está lleno") {
            std::vector<Obj> items_tirados;
            ModFacciones::FactionCallbacks cb{};
            cb.MeterItemEnInventario = [](std::int16_t u, const Obj& item) {
                return false;
            };
            cb.TirarItemAlPiso = [&](const WorldPos& pos, const Obj& item) {
                items_tirados.push_back(item);
            };
            ModFacciones::SetFactionCallbacks(cb);

            ModFacciones::GiveFactionArmours(user_index, false);

            REQUIRE(items_tirados.size() == 3);
            CHECK(items_tirados[0].ObjIndex == 500);
            CHECK(items_tirados[1].ObjIndex == 501);
            CHECK(items_tirados[2].ObjIndex == 502);

            ModFacciones::ResetFactionCallbacks();
        }
    }

    TEST_CASE("GiveExpReward - Acreditación, Clamping a MAXEXP y callbacks") {
        UserList.resize(10);
        const std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].Stats.Exp = 1000;
        RecompensaFacciones[1] = 5000;

        bool console_msg_called = false;
        bool check_level_called = false;
        std::string console_msg_text;

        ModFacciones::FactionCallbacks cb{};
        cb.WriteConsoleMsg = [&](std::int16_t u, std::string_view msg, std::uint8_t font) {
            console_msg_called = true;
            console_msg_text = msg;
        };
        cb.CheckUserLevel = [&](std::int16_t u) {
            check_level_called = true;
        };
        ModFacciones::SetFactionCallbacks(cb);

        SUBCASE("Suma estándar de exp y notificación") {
            ModFacciones::GiveExpReward(user_index, 1);
            CHECK(UserList[user_index].Stats.Exp == 6000);
            CHECK(console_msg_called);
            CHECK(console_msg_text.find("5000") != std::string::npos);
            CHECK(check_level_called);
        }

        SUBCASE("Clamping a MAXEXP") {
            UserList[user_index].Stats.Exp = MAXEXP - 100;
            ModFacciones::GiveExpReward(user_index, 1);
            CHECK(UserList[user_index].Stats.Exp == MAXEXP);
        }

        ModFacciones::ResetFactionCallbacks();
    }

    TEST_CASE("RecompensaArmadaReal - Metas de criminales, niveles y nobleza") {
        UserList.resize(10);
        const std::int16_t user_index = 1;

        std::string overhead_msg;
        bool overhead_called = false;
        bool armours_given = false;

        ModFacciones::FactionCallbacks cb{};
        cb.WriteChatOverHead = [&](std::int16_t u, std::string_view msg, std::int16_t target_npc, std::uint32_t color) {
            overhead_called = true;
            overhead_msg = msg;
        };
        cb.MeterItemEnInventario = [&](std::int16_t u, const Obj& item) {
            armours_given = true;
            return true;
        };
        ModFacciones::SetFactionCallbacks(cb);

        SUBCASE("Rechazo por muertes insuficientes") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CriminalesMatados = 30;
            UserList[user_index].Faccion.NextRecompensa = 70;

            ModFacciones::RecompensaArmadaReal(user_index);

            CHECK(overhead_called);
            CHECK(overhead_msg.find("Mata 40 criminales más") != std::string::npos);
            CHECK(UserList[user_index].Faccion.RecompensasReal == 0);
            CHECK_FALSE(armours_given);
        }

        SUBCASE("Rechazo por nivel insuficiente (Rango 6 requiere nivel 27)") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CriminalesMatados = 640;
            UserList[user_index].Faccion.NextRecompensa = 640;
            UserList[user_index].Stats.ELV = 25;

            ModFacciones::RecompensaArmadaReal(user_index);

            CHECK(overhead_called);
            CHECK(overhead_msg.find("te faltan 2 niveles") != std::string::npos);
            CHECK(UserList[user_index].Faccion.RecompensasReal == 0);
            CHECK_FALSE(armours_given);
        }

        SUBCASE("Rechazo por nobleza insuficiente (Rango 10 requiere 2.000.000 nobleza)") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CriminalesMatados = 2500;
            UserList[user_index].Faccion.NextRecompensa = 2500;
            UserList[user_index].Stats.ELV = 30;
            UserList[user_index].Reputacion.NobleRep = 1500000;

            ModFacciones::RecompensaArmadaReal(user_index);

            CHECK(overhead_called);
            CHECK(overhead_msg.find("500000 puntos de nobleza") != std::string::npos);
            CHECK(UserList[user_index].Faccion.RecompensasReal == 0);
            CHECK_FALSE(armours_given);
        }

        SUBCASE("Promoción exitosa a Rango 1") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CriminalesMatados = 70;
            UserList[user_index].Faccion.NextRecompensa = 70;
            UserList[user_index].Stats.ELV = 25;
            UserList[user_index].clase = static_cast<eClass>(1);
            UserList[user_index].raza = static_cast<eRaza>(1);

            ModFacciones::RecompensaArmadaReal(user_index);

            CHECK(UserList[user_index].Faccion.RecompensasReal == 1);
            CHECK(UserList[user_index].Faccion.NextRecompensa == 130);
            CHECK(armours_given);
            CHECK(overhead_called);
            CHECK(overhead_msg.find("Aquí tienes tu recompensa Escudero") != std::string::npos);
        }

        ModFacciones::ResetFactionCallbacks();
    }

    TEST_CASE("RecompensaCaos - Metas de ciudadanos y niveles (sin nobleza)") {
        UserList.resize(10);
        const std::int16_t user_index = 1;

        std::string overhead_msg;
        bool overhead_called = false;
        bool armours_given = false;

        ModFacciones::FactionCallbacks cb{};
        cb.WriteChatOverHead = [&](std::int16_t u, std::string_view msg, std::int16_t target_npc, std::uint32_t color) {
            overhead_called = true;
            overhead_msg = msg;
        };
        cb.MeterItemEnInventario = [&](std::int16_t u, const Obj& item) {
            armours_given = true;
            return true;
        };
        ModFacciones::SetFactionCallbacks(cb);

        SUBCASE("Rechazo por ciudadanos matados insuficientes") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CiudadanosMatados = 100;
            UserList[user_index].Faccion.NextRecompensa = 160;

            ModFacciones::RecompensaCaos(user_index);

            CHECK(overhead_called);
            CHECK(overhead_msg.find("Mata 60 ciudadanos más") != std::string::npos);
            CHECK(UserList[user_index].Faccion.RecompensasCaos == 0);
            CHECK_FALSE(armours_given);
        }

        SUBCASE("Rechazo por nivel insuficiente (Rango 6 requiere nivel 27)") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CiudadanosMatados = 1500;
            UserList[user_index].Faccion.NextRecompensa = 1500;
            UserList[user_index].Stats.ELV = 24;

            ModFacciones::RecompensaCaos(user_index);

            CHECK(overhead_called);
            CHECK(overhead_msg.find("te faltan 3 niveles") != std::string::npos);
            CHECK(UserList[user_index].Faccion.RecompensasCaos == 0);
            CHECK_FALSE(armours_given);
        }

        SUBCASE("Promoción exitosa a Rango 1 sin exigir nobleza") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CiudadanosMatados = 160;
            UserList[user_index].Faccion.NextRecompensa = 160;
            UserList[user_index].Stats.ELV = 25;
            UserList[user_index].Reputacion.NobleRep = 0;
            UserList[user_index].clase = static_cast<eClass>(1);
            UserList[user_index].raza = static_cast<eRaza>(1);

            ModFacciones::RecompensaCaos(user_index);

            CHECK(UserList[user_index].Faccion.RecompensasCaos == 1);
            CHECK(UserList[user_index].Faccion.NextRecompensa == 300);
            CHECK(armours_given);
            CHECK(overhead_called);
            CHECK(overhead_msg.find("Alma Corrupta") != std::string::npos);
        }

        ModFacciones::ResetFactionCallbacks();
    }
}

TEST_SUITE("ModFacciones - Fase 3: Enrolamiento y Expulsión") {

    TEST_CASE("EnlistarArmadaReal - Verificación de las 9 condiciones") {
        UserList.resize(10);
        const std::int16_t user_index = 1;

        std::string overhead_msg;
        bool log_real_called = false;
        bool armours_given = false;

        ModFacciones::FactionCallbacks cb{};
        cb.IsCriminal = [](std::int16_t u) { return false; };
        cb.WriteChatOverHead = [&](std::int16_t u, std::string_view msg, std::int16_t target_npc, std::uint32_t color) {
            overhead_msg = msg;
        };
        cb.MeterItemEnInventario = [&](std::int16_t u, const Obj& item) {
            armours_given = true;
            return true;
        };
        cb.GetGuildAlignment = [](std::int16_t g) { return "Real"; };
        cb.LogEjercitoReal = [&](std::string_view text) { log_real_called = true; };

        ModFacciones::SetFactionCallbacks(cb);

        SUBCASE("Rechazo 1: Ya pertenece a Armada Real") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.ArmadaReal = 1;

            ModFacciones::EnlistarArmadaReal(user_index);
            CHECK(overhead_msg.find("Ya perteneces a las tropas reales") != std::string::npos);
        }

        SUBCASE("Rechazo 2: Pertenece a Fuerzas del Caos") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.FuerzasCaos = 1;

            ModFacciones::EnlistarArmadaReal(user_index);
            CHECK(overhead_msg.find("seguidor de las sombras") != std::string::npos);
        }

        SUBCASE("Rechazo 3: Es criminal") {
            UserList[user_index] = User{};
            ModFacciones::FactionCallbacks cb_crim = cb;
            cb_crim.IsCriminal = [](std::int16_t u) { return true; };
            ModFacciones::SetFactionCallbacks(cb_crim);

            ModFacciones::EnlistarArmadaReal(user_index);
            CHECK(overhead_msg.find("No se permiten criminales") != std::string::npos);
        }

        SUBCASE("Rechazo 4: Criminales matados < 30") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CriminalesMatados = 29;

            ModFacciones::EnlistarArmadaReal(user_index);
            CHECK(overhead_msg.find("debes matar al menos 30 criminales") != std::string::npos);
        }

        SUBCASE("Rechazo 5: Nivel < 25") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CriminalesMatados = 30;
            UserList[user_index].Stats.ELV = 24;

            ModFacciones::EnlistarArmadaReal(user_index);
            CHECK(overhead_msg.find("debes ser al menos de nivel 25") != std::string::npos);
        }

        SUBCASE("Rechazo 6: Ciudadanos matados > 0") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CriminalesMatados = 30;
            UserList[user_index].Stats.ELV = 25;
            UserList[user_index].Faccion.CiudadanosMatados = 1;

            ModFacciones::EnlistarArmadaReal(user_index);
            CHECK(overhead_msg.find("Has asesinado gente inocente") != std::string::npos);
        }

        SUBCASE("Rechazo 7: Reenlistadas > 4") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CriminalesMatados = 30;
            UserList[user_index].Stats.ELV = 25;
            UserList[user_index].Faccion.CiudadanosMatados = 0;
            UserList[user_index].Faccion.Reenlistadas = 5;

            ModFacciones::EnlistarArmadaReal(user_index);
            CHECK(overhead_msg.find("expulsado de las fuerzas reales demasiadas veces") != std::string::npos);
        }

        SUBCASE("Rechazo 8: Nobleza < 1.000.000") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CriminalesMatados = 30;
            UserList[user_index].Stats.ELV = 25;
            UserList[user_index].Faccion.CiudadanosMatados = 0;
            UserList[user_index].Faccion.Reenlistadas = 0;
            UserList[user_index].Reputacion.NobleRep = 999999;

            ModFacciones::EnlistarArmadaReal(user_index);
            CHECK(overhead_msg.find("ser aún más noble") != std::string::npos);
        }

        SUBCASE("Rechazo 9: Clan neutro") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CriminalesMatados = 30;
            UserList[user_index].Stats.ELV = 25;
            UserList[user_index].Faccion.CiudadanosMatados = 0;
            UserList[user_index].Faccion.Reenlistadas = 0;
            UserList[user_index].Reputacion.NobleRep = 1000000;
            UserList[user_index].GuildIndex = 1;

            ModFacciones::FactionCallbacks cb_neut = cb;
            cb_neut.GetGuildAlignment = [](std::int16_t g) { return "Neutral"; };
            ModFacciones::SetFactionCallbacks(cb_neut);

            ModFacciones::EnlistarArmadaReal(user_index);
            CHECK(overhead_msg.find("Perteneces a un clan neutro") != std::string::npos);
        }

        SUBCASE("Enrolamiento Exitoso") {
            UserList[user_index] = User{};
            UserList[user_index].name = "ImperialHero";
            UserList[user_index].Faccion.CriminalesMatados = 30;
            UserList[user_index].Stats.ELV = 25;
            UserList[user_index].Faccion.CiudadanosMatados = 0;
            UserList[user_index].Faccion.Reenlistadas = 0;
            UserList[user_index].Reputacion.NobleRep = 1000000;
            UserList[user_index].clase = static_cast<eClass>(1);
            UserList[user_index].raza = static_cast<eRaza>(1);

            ModFacciones::EnlistarArmadaReal(user_index);

            CHECK(UserList[user_index].Faccion.ArmadaReal == 1);
            CHECK(UserList[user_index].Faccion.Reenlistadas == 1);
            CHECK(UserList[user_index].Faccion.RecibioArmaduraReal == 1);
            CHECK(UserList[user_index].Faccion.RecibioExpInicialReal == 1);
            CHECK(UserList[user_index].Faccion.NextRecompensa == 70);
            CHECK(armours_given);
            CHECK(log_real_called);
            CHECK(overhead_msg.find("¡Bienvenido al ejército real!!!") != std::string::npos);
        }

        ModFacciones::ResetFactionCallbacks();
    }

    TEST_CASE("EnlistarCaos - Verificación de reglas, Quirk 7.1 y Quirk 7.3") {
        UserList.resize(10);
        const std::int16_t user_index = 1;

        std::string overhead_msg;
        bool log_caos_called = false;
        bool armours_given = false;

        ModFacciones::FactionCallbacks cb{};
        cb.IsCriminal = [](std::int16_t u) { return true; }; // Debe ser criminal
        cb.WriteChatOverHead = [&](std::int16_t u, std::string_view msg, std::int16_t target_npc, std::uint32_t color) {
            overhead_msg = msg;
        };
        cb.MeterItemEnInventario = [&](std::int16_t u, const Obj& item) {
            armours_given = true;
            return true;
        };
        cb.GetGuildAlignment = [](std::int16_t g) { return "Caos"; };
        cb.LogEjercitoCaos = [&](std::string_view text) { log_caos_called = true; };

        ModFacciones::SetFactionCallbacks(cb);

        SUBCASE("Rechazo: No es criminal") {
            UserList[user_index] = User{};
            ModFacciones::FactionCallbacks cb_ciuda = cb;
            cb_ciuda.IsCriminal = [](std::int16_t u) { return false; };
            ModFacciones::SetFactionCallbacks(cb_ciuda);

            ModFacciones::EnlistarCaos(user_index);
            CHECK(overhead_msg.find("¡Lárgate de aquí, bufón!!!") != std::string::npos);
        }

        SUBCASE("Quirk 7.1: Bloqueo Irreversible por RecibioExpInicialReal == 1") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.RecibioExpInicialReal = 1;
            UserList[user_index].Faccion.CiudadanosMatados = 70;
            UserList[user_index].Stats.ELV = 25;

            ModFacciones::EnlistarCaos(user_index);
            CHECK(overhead_msg.find("No permitiré que ningún insecto real ingrese a mis tropas") != std::string::npos);
            CHECK(UserList[user_index].Faccion.FuerzasCaos == 0);
        }

        SUBCASE("Quirk 7.3: Rechazo por rebelión cuando Reenlistadas == 200") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.CiudadanosMatados = 70;
            UserList[user_index].Stats.ELV = 25;
            UserList[user_index].Faccion.Reenlistadas = 200;

            ModFacciones::EnlistarCaos(user_index);
            CHECK(overhead_msg.find("durante tu rebeldía has atacado a mi ejército") != std::string::npos);
            CHECK(UserList[user_index].Faccion.FuerzasCaos == 0);
        }

        SUBCASE("Enrolamiento Exitoso en el Caos") {
            UserList[user_index] = User{};
            UserList[user_index].name = "CaosLord";
            UserList[user_index].Faccion.CiudadanosMatados = 70;
            UserList[user_index].Stats.ELV = 25;
            UserList[user_index].clase = static_cast<eClass>(1);
            UserList[user_index].raza = static_cast<eRaza>(1);

            ModFacciones::EnlistarCaos(user_index);

            CHECK(UserList[user_index].Faccion.FuerzasCaos == 1);
            CHECK(UserList[user_index].Faccion.Reenlistadas == 1);
            CHECK(UserList[user_index].Faccion.RecibioArmaduraCaos == 1);
            CHECK(UserList[user_index].Faccion.RecibioExpInicialCaos == 1);
            CHECK(UserList[user_index].Faccion.NextRecompensa == 160);
            CHECK(armours_given);
            CHECK(log_caos_called);
            CHECK(overhead_msg.find("¡Bienvenido al lado oscuro!!!") != std::string::npos);
        }

        ModFacciones::ResetFactionCallbacks();
    }

    TEST_CASE("ExpulsarFaccion - Quirk 7.2 y diferenciación de avisos") {
        UserList.resize(10);
        const std::int16_t user_index = 1;
        ObjDataList.resize(100);

        std::string console_msg;
        std::vector<std::uint8_t> desequipados;
        bool refresh_status_called = false;

        ModFacciones::FactionCallbacks cb{};
        cb.WriteConsoleMsg = [&](std::int16_t u, std::string_view msg, std::uint8_t font) {
            console_msg = msg;
        };
        cb.Desequipar = [&](std::int16_t u, std::uint8_t slot) {
            desequipados.push_back(slot);
        };
        cb.RefreshCharStatus = [&](std::int16_t u) {
            refresh_status_called = true;
        };
        ModFacciones::SetFactionCallbacks(cb);

        SUBCASE("Expulsión Armada Real con desequipamiento selectivo") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.ArmadaReal = 1;
            UserList[user_index].Invent.ArmourEqpObjIndex = 10;
            UserList[user_index].Invent.ArmourEqpSlot = 1;
            ObjDataList[10].Real = 1; // Armadura Real

            UserList[user_index].Invent.EscudoEqpObjIndex = 20;
            UserList[user_index].Invent.EscudoEqpSlot = 2;
            ObjDataList[20].Real = 0; // Escudo Común (NO real)

            UserList[user_index].flags.Navegando = true;

            ModFacciones::ExpulsarFaccionReal(user_index, true);

            CHECK(UserList[user_index].Faccion.ArmadaReal == 0);
            CHECK(console_msg.find("Has sido expulsado del ejército real") != std::string::npos);
            REQUIRE(desequipados.size() == 1);
            CHECK(desequipados[0] == 1); // Solo la armadura real
            CHECK(refresh_status_called);
        }

        SUBCASE("Retiro voluntario Caos") {
            UserList[user_index] = User{};
            UserList[user_index].Faccion.FuerzasCaos = 1;

            ModFacciones::ExpulsarFaccionCaos(user_index, false);

            CHECK(UserList[user_index].Faccion.FuerzasCaos == 0);
            CHECK(console_msg.find("Te has retirado de la Legión Oscura") != std::string::npos);
            CHECK(desequipados.empty());
        }

        ModFacciones::ResetFactionCallbacks();
    }
}
