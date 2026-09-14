#include <doctest/doctest.h>

#include "server/SistemaCombate.hpp"
#include "server/Declares.hpp"
#include "server/TCP.hpp"
#include "server/FileIO.hpp"

#include <vector>
#include <string_view>

namespace {

void SetupCombatTestEnvironment() {
    UserList.clear();
    for (int i = 0; i < 10; ++i) {
        UserList.emplace_back();
    }
    for (std::size_t i = 1; i < UserList.size(); ++i) {
        UserList[i].flags.UserLogged = true;
        UserList[i].clase = eClass::Warrior;
        UserList[i].Pos = WorldPos{1, 50, 50};
        UserList[i].char_appearance.heading = eHeading::NORTH;
        UserList[i].Stats.MinSta = 50;
        UserList[i].Stats.MaxSta = 100;
        UserList[i].Stats.MinHp = 100;
        UserList[i].Stats.MaxHp = 100;
        UserList[i].Stats.MinHIT = 1;
        UserList[i].Stats.MaxHIT = 2;
        UserList[i].Stats.UserAtributos[eAtributos::Fuerza] = 18;
        UserList[i].Stats.UserAtributos[eAtributos::Agilidad] = 18;
    }

    // Inicializar Npclist
    for (std::size_t i = 0; i < Npclist.size(); ++i) {
        Npclist[i] = npc{};
    }
    Npclist[1].Stats.MinHIT = 10;
    Npclist[1].Stats.MaxHIT = 20;
    Npclist[1].Stats.MinHp = 100;
    Npclist[1].Stats.def = 15;
    Npclist[1].PoderAtaque = 80;
    Npclist[1].PoderEvasion = 60;
    Npclist[1].NPCtype = eNPCType::Comun;

    // NPC Dragón (ítem 2)
    Npclist[2].Stats.MinHIT = 50;
    Npclist[2].Stats.MaxHIT = 100;
    Npclist[2].Stats.MinHp = 5000;
    Npclist[2].Stats.def = 80;
    Npclist[2].PoderAtaque = 200;
    Npclist[2].PoderEvasion = 150;
    Npclist[2].NPCtype = eNPCType::DRAGON;

    // Inicializar ObjDataList
    ObjDataList.assign(60, ObjData{});

    // Arma melee común (ítem 10)
    ObjDataList[10].name = "Espada Larga";
    ObjDataList[10].OBJType = eOBJType::otWeapon;
    ObjDataList[10].proyectil = 0;
    ObjDataList[10].Municion = 0;
    ObjDataList[10].Apuñala = 0;
    ObjDataList[10].MinHIT = 10;
    ObjDataList[10].MaxHIT = 15;

    // Daga para apuñalar (ítem 11)
    ObjDataList[11].name = "Daga";
    ObjDataList[11].OBJType = eOBJType::otWeapon;
    ObjDataList[11].proyectil = 0;
    ObjDataList[11].Municion = 0;
    ObjDataList[11].Apuñala = 1;
    ObjDataList[11].MinHIT = 4;
    ObjDataList[11].MaxHIT = 8;

    // Arco simple (ítem 20)
    ObjDataList[20].name = "Arco Simple";
    ObjDataList[20].OBJType = eOBJType::otWeapon;
    ObjDataList[20].proyectil = 1;
    ObjDataList[20].Municion = 1;
    ObjDataList[20].MinHIT = 6;
    ObjDataList[20].MaxHIT = 10;

    // Flecha ligera (ítem 30)
    ObjDataList[30].name = "Flecha Ligera";
    ObjDataList[30].OBJType = eOBJType::otFlechas;
    ObjDataList[30].proyectil = 1;
    ObjDataList[30].MinHIT = 1;
    ObjDataList[30].MaxHIT = 2;

    // Flecha pesada (ítem 31)
    ObjDataList[31].name = "Flecha Pesada";
    ObjDataList[31].OBJType = eOBJType::otFlechas;
    ObjDataList[31].proyectil = 1;
    ObjDataList[31].MinHIT = 10;
    ObjDataList[31].MaxHIT = 20;

    // Espada Mata Dragones (ítem 402 / EspadaMataDragonesIndex)
    if (ObjDataList.size() <= static_cast<std::size_t>(EspadaMataDragonesIndex)) {
        ObjDataList.resize(EspadaMataDragonesIndex + 1);
    }
    ObjDataList[EspadaMataDragonesIndex].name = "Espada Mata Dragones";
    ObjDataList[EspadaMataDragonesIndex].OBJType = eOBJType::otWeapon;
    ObjDataList[EspadaMataDragonesIndex].MinHIT = 1;
    ObjDataList[EspadaMataDragonesIndex].MaxHIT = 1;

    // Guante de lucha (ítem 40)
    ObjDataList[40].name = "Guantelete";
    ObjDataList[40].OBJType = eOBJType::otAnillo;
    ObjDataList[40].Guante = 1;
    ObjDataList[40].MinHIT = 2;
    ObjDataList[40].MaxHIT = 4;

    // Escudo (ítem 50)
    ObjDataList[50].name = "Escudo de Hierro";
    ObjDataList[50].OBJType = eOBJType::otESCUDO;
    ObjDataList[50].MinDef = 5;
    ObjDataList[50].MaxDef = 10;

    // Casco (ítem 51)
    ObjDataList[51].name = "Casco de Hierro";
    ObjDataList[51].OBJType = eOBJType::otCASCO;
    ObjDataList[51].MinDef = 5;
    ObjDataList[51].MaxDef = 10;

    // Armadura (ítem 52)
    ObjDataList[52].name = "Cota de Malla";
    ObjDataList[52].OBJType = eOBJType::otArmadura;
    ObjDataList[52].MinDef = 15;
    ObjDataList[52].MaxDef = 25;

    // Daga Envenenada (ítem 53)
    ObjDataList[53].name = "Daga Envenenada";
    ObjDataList[53].OBJType = eOBJType::otWeapon;
    ObjDataList[53].Envenena = 1;
    ObjDataList[53].Apuñala = 1;
    ObjDataList[53].MinHIT = 5;
    ObjDataList[53].MaxHIT = 8;

    // Flecha Envenenada (ítem 54)
    ObjDataList[54].name = "Flecha Envenenada";
    ObjDataList[54].OBJType = eOBJType::otFlechas;
    ObjDataList[54].proyectil = 1;
    ObjDataList[54].Envenena = 1;
    ObjDataList[54].MinHIT = 2;
    ObjDataList[54].MaxHIT = 4;

    // Espada Reforzada (ítem 55)
    ObjDataList[55].name = "Espada Reforzada";
    ObjDataList[55].OBJType = eOBJType::otWeapon;
    ObjDataList[55].Refuerzo = 5;
    ObjDataList[55].MinHIT = 12;
    ObjDataList[55].MaxHIT = 16;

    // Modificadores de Clase para Warrior
    const auto warrior_idx = static_cast<std::size_t>(eClass::Warrior);
    ModClaseList[warrior_idx].Escudo = 1.0;
    ModClaseList[warrior_idx].Evasion = 1.0;
    ModClaseList[warrior_idx].AtaqueArmas = 1.0;
    ModClaseList[warrior_idx].AtaqueProyectiles = 0.9;
    ModClaseList[warrior_idx].AtaqueWrestling = 0.8;
    ModClaseList[warrior_idx].DañoArmas = 1.0;
    ModClaseList[warrior_idx].DañoProyectiles = 1.0;
    ModClaseList[warrior_idx].DañoWrestling = 1.0;

    // Modificadores de Clase para Assasin
    const auto assassin_idx = static_cast<std::size_t>(eClass::Assasin);
    ModClaseList[assassin_idx].DañoArmas = 1.0;
    ModClaseList[assassin_idx].DañoProyectiles = 1.0;
    ModClaseList[assassin_idx].DañoWrestling = 1.0;

    MapData.assign(2 * 101 * 101, MapBlock{});
    MapInfoList.assign(10, MapInfo{});
    for (auto& mi : MapInfoList) {
        mi.Pk = true;
    }

    TCP::SetSendDataHook(nullptr);
    // Reset de hooks y providers
    ao::ResetCombatHooks();
}

} // namespace

TEST_SUITE("SistemaCombate - Fase 1: G1 Evasión, Poder Ofensivo y Utilidades") {

    TEST_CASE("MinimoInt y MaximoInt manejan cotas y comparaciones correctamente") {
        CHECK(ao::MinimoInt(10, 20) == 10);
        CHECK(ao::MinimoInt(50, -5) == -5);
        CHECK(ao::MinimoInt(15, 15) == 15);

        CHECK(ao::MaximoInt(10, 20) == 20);
        CHECK(ao::MaximoInt(50, -5) == 50);
        CHECK(ao::MaximoInt(-10, -30) == -10);
        CHECK(ao::MaximoInt(15, 15) == 15);
    }

    TEST_CASE("PoderEvasionEscudo calcula el índice defensivo exacto de escudo") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        auto& user = UserList[user_idx];

        user.Stats.UserSkills[eSkill::Defensa] = 50;
        CHECK(ao::PoderEvasionEscudo(user_idx) == 25);

        user.Stats.UserSkills[eSkill::Defensa] = 35;
        CHECK(ao::PoderEvasionEscudo(user_idx) == 17);

        user.Stats.UserSkills[eSkill::Defensa] = 0;
        CHECK(ao::PoderEvasionEscudo(user_idx) == 0);
    }

    TEST_CASE("PoderEvasion preserva division flotante / 33.0 y bono por nivel") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        auto& user = UserList[user_idx];

        user.Stats.UserAtributos[eAtributos::Agilidad] = 18;
        user.Stats.UserSkills[eSkill::Tacticas] = 10;
        user.Stats.ELV = 10;
        CHECK(ao::PoderEvasion(user_idx) == 15);

        user.Stats.ELV = 16;
        CHECK(ao::PoderEvasion(user_idx) == 25);

        user.Stats.UserSkills[eSkill::Tacticas] = 100;
        user.Stats.UserAtributos[eAtributos::Agilidad] = 20;
        user.Stats.ELV = 25;
        CHECK(ao::PoderEvasion(user_idx) == 193);
    }

    TEST_CASE("PoderAtaqueArma cubre los 4 tramos de habilidad y bono por nivel") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        auto& user = UserList[user_idx];
        user.Stats.UserAtributos[eAtributos::Agilidad] = 18;
        user.Stats.ELV = 10;

        user.Stats.UserSkills[eSkill::Armas] = 20;
        CHECK(ao::PoderAtaqueArma(user_idx) == 20);

        user.Stats.UserSkills[eSkill::Armas] = 40;
        CHECK(ao::PoderAtaqueArma(user_idx) == 58);

        user.Stats.UserSkills[eSkill::Armas] = 70;
        CHECK(ao::PoderAtaqueArma(user_idx) == 106);

        user.Stats.UserSkills[eSkill::Armas] = 100;
        CHECK(ao::PoderAtaqueArma(user_idx) == 154);

        user.Stats.ELV = 14;
        CHECK(ao::PoderAtaqueArma(user_idx) == 159);
    }

    TEST_CASE("PoderAtaqueProyectil y PoderAtaqueWrestling aplican sus modificadores y tramos") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        auto& user = UserList[user_idx];
        user.Stats.UserAtributos[eAtributos::Agilidad] = 20;
        user.Stats.ELV = 12;

        user.Stats.UserSkills[eSkill::Proyectiles] = 50;
        CHECK(ao::PoderAtaqueProyectil(user_idx) == 63);

        user.Stats.UserSkills[eSkill::Wrestling] = 100;
        CHECK(ao::PoderAtaqueWrestling(user_idx) == 128);
    }

    TEST_CASE("PoderAtaqueModificado rutea segun proyectil o cuerpo a cuerpo") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        auto& user = UserList[user_idx];
        user.Stats.UserAtributos[eAtributos::Agilidad] = 20;
        user.Stats.ELV = 12;
        user.Stats.UserSkills[eSkill::Armas] = 40;
        user.Stats.UserSkills[eSkill::Proyectiles] = 40;

        user.Invent.WeaponEqpObjIndex = 10;
        CHECK(ao::PoderAtaqueModificado(user_idx) == 60);

        user.Invent.WeaponEqpObjIndex = 20;
        CHECK(ao::PoderAtaqueModificado(user_idx) == 54);
    }

    TEST_CASE("AlcanzaEspacio distingue melee de proyectiles y valida limites") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;

        CHECK(ao::AlcanzaEspacio(user_idx, 10) == 1);
        CHECK(ao::AlcanzaEspacio(user_idx, 20) == ao::MAXDISTANCIAARCO);
        CHECK(ao::AlcanzaEspacio(user_idx, 20) == 18);
        CHECK(ao::AlcanzaEspacio(user_idx, 0) == 1);
    }

    TEST_CASE("ArcoYFlecha y CheckArmasMuniciones validan compatibilidad de proyectiles") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        auto& user = UserList[user_idx];

        user.Invent.WeaponEqpObjIndex = 0;
        user.Invent.MunicionEqpObjIndex = 0;
        CHECK_FALSE(ao::ArcoYFlecha(user_idx));

        user.Invent.WeaponEqpObjIndex = 10;
        CHECK_FALSE(ao::ArcoYFlecha(user_idx));

        user.Invent.WeaponEqpObjIndex = 20;
        CHECK_FALSE(ao::ArcoYFlecha(user_idx));

        user.Invent.MunicionEqpObjIndex = 30;
        CHECK(ao::ArcoYFlecha(user_idx));

        CHECK(ao::CheckArmasMuniciones(user_idx, 20, 30));
        CHECK_FALSE(ao::CheckArmasMuniciones(user_idx, 20, 10));
    }

    TEST_CASE("ArmaParaApuñalar verifica flag de item") {
        SetupCombatTestEnvironment();
        CHECK_FALSE(ao::ArmaParaApuñalar(10));
        CHECK(ao::ArmaParaApuñalar(11));
        CHECK_FALSE(ao::ArmaParaApuñalar(0));
    }

    TEST_CASE("SameClan y SameParty validan pertenencia mutua no nula") {
        SetupCombatTestEnvironment();
        const std::int16_t user_a = 1;
        const std::int16_t user_b = 2;

        UserList[user_a].GuildIndex = 0;
        UserList[user_b].GuildIndex = 0;
        CHECK_FALSE(ao::SameClan(user_a, user_b));

        UserList[user_a].GuildIndex = 5;
        UserList[user_b].GuildIndex = 5;
        CHECK(ao::SameClan(user_a, user_b));

        UserList[user_b].GuildIndex = 8;
        CHECK_FALSE(ao::SameClan(user_a, user_b));

        UserList[user_a].PartyIndex = 0;
        UserList[user_b].PartyIndex = 0;
        CHECK_FALSE(ao::SameParty(user_a, user_b));

        UserList[user_a].PartyIndex = 3;
        UserList[user_b].PartyIndex = 3;
        CHECK(ao::SameParty(user_a, user_b));

        UserList[user_b].PartyIndex = 4;
        CHECK_FALSE(ao::SameParty(user_a, user_b));
    }

    TEST_CASE("TriggerZonaPelea evalua celdas de arena y combate permitido") {
        SetupCombatTestEnvironment();
        const std::int16_t user_a = 1;
        const std::int16_t user_b = 2;

        UserList[user_a].Pos = WorldPos{1, 50, 50};
        UserList[user_b].Pos = WorldPos{1, 50, 51};

        auto get_idx = [](std::int16_t map, std::int16_t x, std::int16_t y) {
            return (static_cast<std::size_t>(map) * 101 + static_cast<std::size_t>(x)) * 101 + static_cast<std::size_t>(y);
        };

        const auto idx_a = get_idx(1, 50, 50);
        const auto idx_b = get_idx(1, 50, 51);

        MapData[idx_a].trigger = eTrigger::NADA;
        MapData[idx_b].trigger = eTrigger::NADA;
        CHECK(ao::TriggerZonaPelea(user_a, user_b) == eTrigger6::TRIGGER6_AUSENTE);

        MapData[idx_a].trigger = eTrigger::ZONAPELEA;
        MapData[idx_b].trigger = eTrigger::ZONAPELEA;
        CHECK(ao::TriggerZonaPelea(user_a, user_b) == eTrigger6::TRIGGER6_PERMITE);

        MapData[idx_b].trigger = eTrigger::NADA;
        CHECK(ao::TriggerZonaPelea(user_a, user_b) == eTrigger6::TRIGGER6_PROHIBE);
    }

    TEST_CASE("IntervaloPermiteAtacar controla cooldown determinista con TimeProvider") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        IntervaloUserPuedeAtacar = 1500;

        std::int32_t mock_tick = 10000;
        ao::SetCombatTimeProvider([&mock_tick]() {
            return mock_tick;
        });

        UserList[user_idx].Counters.TimerPuedeAtacar = 8000;

        CHECK(ao::IntervaloPermiteAtacar(user_idx));
        CHECK(UserList[user_idx].Counters.TimerPuedeAtacar == 10000);

        CHECK_FALSE(ao::IntervaloPermiteAtacar(user_idx));

        mock_tick = 11000;
        CHECK_FALSE(ao::IntervaloPermiteAtacar(user_idx));

        mock_tick = 11500;
        CHECK(ao::IntervaloPermiteAtacar(user_idx));
        CHECK(UserList[user_idx].Counters.TimerPuedeAtacar == 11500);

        ao::SetCombatTimeProvider(nullptr);
    }
}

TEST_SUITE("SistemaCombate - Fase 2: G2 Daño Bruto y Acierto RNG") {

    TEST_CASE("ProbExito acota rígidamente entre 10 y 90 (clamp)") {
        // Diferencia nula: 50 + 0 = 50
        CHECK(ao::ProbExito(100, 100) == 50);

        // Ventaja de 50 puntos: 50 + 50 * 0.4 = 50 + 20 = 70
        CHECK(ao::ProbExito(150, 100) == 70);

        // Desventaja de 50 puntos: 50 - 50 * 0.4 = 50 - 20 = 30
        CHECK(ao::ProbExito(100, 150) == 30);

        // Ventaja masiva: 50 + 500 * 0.4 = 250 -> clamp 90
        CHECK(ao::ProbExito(600, 100) == 90);

        // Desventaja extrema: 50 - 500 * 0.4 = -150 -> clamp 10
        CHECK(ao::ProbExito(100, 600) == 10);
    }

    TEST_CASE("CalcularDaño - Replicación Estricta del Bug #37 (Omisión de Flechas en DañoMaxArma)") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        auto& user = UserList[user_idx];

        // Fuerza = 20 -> Fuerza - 15 = 5 (excedente)
        user.Stats.UserAtributos[eAtributos::Fuerza] = 20;
        user.Invent.WeaponEqpObjIndex = 20; // Arco Simple: MinHIT = 6, MaxHIT = 10, proyectil = 1, Municion = 1

        // Inyectar generador RNG fijo (devuelve siempre el máximo para comparar cotas)
        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            return upper;
        });

        // Caso A: Con Flecha Ligera (ítem 30: MinHIT = 1, MaxHIT = 2)
        // DañoArma = 10 (arco) + 2 (flecha) = 12
        // DañoMaxArma = 10 (SÓLO EL ARCO, ignora flecha por Bug #37)
        // Bono Fuerza = (10 / 5) * (20 - 15) = 2 * 5 = 10
        // DañoUsuario = 2 (Stats.MaxHIT)
        // Total esperado = 3 * 12 + 10 + 2 = 36 + 10 + 2 = 48
        user.Invent.MunicionEqpObjIndex = 30;
        const auto daño_con_flecha_ligera = ao::CalcularDaño(user_idx, 0);
        CHECK(daño_con_flecha_ligera == 48);

        // Caso B: Con Flecha Pesada (ítem 31: MinHIT = 10, MaxHIT = 20)
        // DañoArma = 10 (arco) + 20 (flecha) = 30
        // DañoMaxArma = 10 (SÓLO EL ARCO, Bug #37 comprobado: NO cambia a 30!)
        // Bono Fuerza = (10 / 5) * (20 - 15) = 2 * 5 = 10 (IDÉNTICO a Caso A!)
        // DañoUsuario = 2
        // Total esperado = 3 * 30 + 10 + 2 = 90 + 10 + 2 = 102
        user.Invent.MunicionEqpObjIndex = 31;
        const auto daño_con_flecha_pesada = ao::CalcularDaño(user_idx, 0);
        CHECK(daño_con_flecha_pesada == 102);

        // Si el Bug #37 se hubiera "arreglado" erróneamente sumando la flecha a DañoMaxArma:
        // DañoMaxArma habría sido 10 + 20 = 30
        // Bono Fuerza habría sido (30 / 5) * 5 = 30 (20 puntos más -> total 122)
        // Verificamos que se preservó estrictamente 102 (paridad con VB6).
        CHECK(daño_con_flecha_pesada != 122);
    }

    TEST_CASE("CalcularDaño - Combate Desarmado (Wrestling) con y sin Guantes") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        auto& user = UserList[user_idx];

        user.Invent.WeaponEqpObjIndex = 0; // Sin arma
        user.Invent.AnilloEqpObjIndex = 0; // Sin guante
        user.Stats.UserAtributos[eAtributos::Fuerza] = 20;

        // Inyectar RNG con retorno mínimo
        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            return lower;
        });

        // Wrestling base: Min = 4, Max = 9
        // DañoArma = 4
        // DañoMaxArma = 9
        // Bono Fuerza = (9 / 5) * 5 = 1.8 * 5 = 9.0
        // DañoUsuario = 1 (Stats.MinHIT)
        // Total = 3 * 4 + 9.0 + 1 = 12 + 9 + 1 = 22
        CHECK(ao::CalcularDaño(user_idx, 0) == 22);

        // Con Guantelete en slot de anillo (ítem 40: MinHIT = 2, MaxHIT = 4)
        // DañoMin = 4 + 2 = 6, DañoMax = 9 + 4 = 13
        // DañoArma = 6
        // DañoMaxArma = 13 -> Bono Fuerza = (13 / 5) * 5 = 2.6 * 5 = 13.0
        // DañoUsuario = 1
        // Total = 3 * 6 + 13.0 + 1 = 18 + 13 + 1 = 32
        user.Invent.AnilloEqpObjIndex = 40;
        CHECK(ao::CalcularDaño(user_idx, 0) == 32);
    }

    TEST_CASE("CalcularDaño - Espada Mata Dragones contra Dragón vs otros objetivos") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        auto& user = UserList[user_idx];
        user.Invent.WeaponEqpObjIndex = EspadaMataDragonesIndex;

        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            return lower;
        });

        // Contra Dragón (NPC 2): Daño exacto letal = MinHp (5000) + def (80) = 5080
        CHECK(ao::CalcularDaño(user_idx, 2) == 5080);

        // Contra criatura común (NPC 1): Daño fijado en 1
        // DañoArma = 1, DañoMaxArma = 1
        // Bono Fuerza con Fuerza 18: (1 / 5) * (18 - 15) = 0.2 * 3 = 0.6
        // DañoUsuario = 1
        // Total = (3 * 1 + 0.6 + 1) * 1.0 = 4.6 -> floor = 4
        CHECK(ao::CalcularDaño(user_idx, 1) == 4);

        // Contra Usuario (PvP, NpcIndex = 0): DañoArma = 1, DañoMaxArma = 1
        CHECK(ao::CalcularDaño(user_idx, 0) == 4);
    }

    TEST_CASE("UserImpactoUser penaliza un 25% la evasión de víctimas meditando") {
        SetupCombatTestEnvironment();
        const std::int16_t atacante = 1;
        const std::int16_t victima = 2;

        UserList[atacante].Stats.ELV = 12;
        UserList[atacante].Stats.UserAtributos[eAtributos::Agilidad] = 18;
        UserList[atacante].Invent.WeaponEqpObjIndex = 10;
        UserList[atacante].Stats.UserSkills[eSkill::Armas] = 50;

        UserList[victima].Stats.ELV = 12;
        UserList[victima].Stats.UserAtributos[eAtributos::Agilidad] = 18;
        UserList[victima].Invent.EscudoEqpObjIndex = 0;
        UserList[victima].Stats.UserSkills[eSkill::Tacticas] = 44;

        // Inyectamos un generador determinista que retorna 60
        std::int32_t fixed_roll = 60;
        ao::SetCombatRandomProvider([&fixed_roll](std::int32_t lower, std::int32_t upper) {
            return fixed_roll;
        });

        // Caso 1: Víctima despierta (no medita)
        // PoderAtk (68) = PoderDef (68) -> ProbExito = 50.
        // Roll = 60 > 50 -> Fallo
        UserList[victima].flags.Meditando = false;
        CHECK_FALSE(ao::UserImpactoUser(atacante, victima));

        // Caso 2: Víctima meditando
        // ProbEvadir = (100 - 50) * 0.75 = 37.5
        // ProbExito = 100 - 37.5 = 62.5 -> 62%
        // Roll = 60 <= 62 -> Acierto exitoso!
        UserList[victima].flags.Meditando = true;
        CHECK(ao::UserImpactoUser(atacante, victima));
    }

    TEST_CASE("NpcImpactoUser, UserImpactoNpc y NpcImpactoNpc resuelven tiradas probabilísticas") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        const std::int16_t npc_idx = 1;

        // Roll siempre bajo (1) -> Éxito garantizado
        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            return 1;
        });
        CHECK(ao::UserImpactoNpc(user_idx, npc_idx));
        CHECK(ao::NpcImpactoUser(npc_idx, user_idx));
        CHECK(ao::NpcImpactoNpc(npc_idx, npc_idx));

        // Roll máximo (100) -> Fallo garantizado (máxima prob es 90)
        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            return 100;
        });
        CHECK_FALSE(ao::UserImpactoNpc(user_idx, npc_idx));
        CHECK_FALSE(ao::NpcImpactoUser(npc_idx, user_idx));
        CHECK_FALSE(ao::NpcImpactoNpc(npc_idx, npc_idx));
    }

    TEST_CASE("NpcDaño y consultas auxiliares NpcImpacto, NpcEvasion, UserImpacto") {
        SetupCombatTestEnvironment();
        const std::int16_t npc_idx = 1;
        const std::int16_t user_idx = 1;

        UserList[user_idx].Stats.UserSkills[eSkill::Armas] = 30;

        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            return lower;
        });

        // NPC 1: MinHIT = 10, MaxHIT = 20
        CHECK(ao::NpcDaño(npc_idx) == 10);
        CHECK(ao::NpcImpacto(npc_idx) == 80);
        CHECK(ao::NpcEvasion(npc_idx) == 60);

        // UserImpacto retorna PoderAtaqueModificado
        CHECK(ao::UserImpacto(user_idx) > 0);
    }
}

TEST_SUITE("SistemaCombate - Fase 3: G3 Deducción de Daño, Absorciones y Peculiaridades") {

    TEST_CASE("Criterio 7.1: Espada Mata Dragones - letalidad contra dragón y destrucción, daño 1 contra otros") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        const std::int16_t dragon_idx = 2;
        const std::int16_t comun_idx = 1;
        const std::int16_t victim_user = 2;

        UserList[user_idx].Invent.WeaponEqpObjIndex = EspadaMataDragonesIndex;

        bool quitar_objetos_llamado = false;
        std::int16_t quitar_obj_index = 0;
        std::int32_t quitar_amount = 0;
        std::int16_t quitar_user = 0;
        ao::SetCombatQuitarObjetosHook([&](std::int16_t obj, std::int32_t amt, std::int16_t usr) {
            quitar_objetos_llamado = true;
            quitar_obj_index = obj;
            quitar_amount = amt;
            quitar_user = usr;
        });

        bool muere_npc_llamado = false;
        ao::SetMuereNpcHook([&](std::int16_t n, std::int16_t u) {
            muere_npc_llamado = true;
        });

        // 1. Atacar al dragón: letal exacto = MinHp + def = 5000 + 80 = 5080
        const auto daño_dragon = ao::UserDañoNpc(user_idx, dragon_idx);
        CHECK(daño_dragon == 5080);
        CHECK(Npclist[dragon_idx].Stats.MinHp <= 0);
        CHECK(quitar_objetos_llamado);
        CHECK(quitar_obj_index == EspadaMataDragonesIndex);
        CHECK(quitar_amount == 1);
        CHECK(quitar_user == user_idx);
        CHECK(muere_npc_llamado);

        // 2. Atacar a criatura común: daño fijado en exactamente 1
        quitar_objetos_llamado = false;
        Npclist[comun_idx].Stats.MinHp = 100;
        const auto daño_comun = ao::UserDañoNpc(user_idx, comun_idx);
        CHECK(daño_comun == 1);
        CHECK(Npclist[comun_idx].Stats.MinHp == 99);
        CHECK_FALSE(quitar_objetos_llamado);

        // 3. Atacar a jugador (PvP): daño fijado en exactamente 1
        UserList[victim_user].Stats.MinHp = 100;
        const auto daño_pvp = ao::UserDañoUser(user_idx, victim_user);
        CHECK(daño_pvp == 1);
        CHECK(UserList[victim_user].Stats.MinHp == 99);
        CHECK_FALSE(quitar_objetos_llamado);
    }

    TEST_CASE("Criterio 7.2: Asimetría de Apuñalamiento ZaMa - PvE bruto vs PvP neto") {
        SetupCombatTestEnvironment();
        const std::int16_t atacante = 1;
        const std::int16_t victima_npc = 1;
        const std::int16_t victima_user = 2;

        UserList[atacante].clase = eClass::Assasin;
        UserList[atacante].Invent.WeaponEqpObjIndex = 11; // Daga: Apuñala = 1
        UserList[atacante].Stats.UserSkills[eSkill::Apuñalar] = 50;

        // Determinismo RNG: valores mínimos para que CalcularDaño retorne un valor predecible
        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            return lower;
        });

        std::int32_t daño_recibido_apuñalar = 0;
        std::int16_t hook_victim_user = -1;
        std::int16_t hook_victim_npc = -1;
        ao::SetDoApuñalarHook([&](std::int16_t usr, std::int16_t v_usr, std::int16_t v_npc, std::int32_t dmg) {
            daño_recibido_apuñalar = dmg;
            hook_victim_user = v_usr;
            hook_victim_npc = v_npc;
        });

        // 1. En PvE: DoApuñalar recibe DañoBase BRUTO antes de restar la defensa del NPC
        Npclist[victima_npc].Stats.MinHp = 500;
        Npclist[victima_npc].Stats.def = 15;
        const auto daño_neto_npc = ao::UserDañoNpc(atacante, victima_npc);
        CHECK(hook_victim_npc == victima_npc);
        CHECK(hook_victim_user == 0);
        // Daño base bruto recibido en el hook debe ser mayor al daño neto aplicado al NPC
        CHECK(daño_recibido_apuñalar == daño_neto_npc + Npclist[victima_npc].Stats.def);

        // 2. En PvP: DoApuñalar recibe daño NETO residual después de restar defensas
        UserList[victima_user].Stats.MinHp = 500;
        UserList[victima_user].Invent.ArmourEqpObjIndex = 52; // Cota de Malla
        UserList[victima_user].Invent.EscudoEqpObjIndex = 50; // Escudo
        UserList[victima_user].Invent.CascoEqpObjIndex = 51;  // Casco

        daño_recibido_apuñalar = 0;
        const auto daño_neto_pvp = ao::UserDañoUser(atacante, victima_user);
        CHECK(hook_victim_user == victima_user);
        CHECK(hook_victim_npc == 0);
        // En PvP, el hook recibe EXACTAMENTE el daño neto post-absorción
        CHECK(daño_recibido_apuñalar == daño_neto_pvp);
    }

    TEST_CASE("Absorción de Armadura vs Casco según sorteo de zona de impacto") {
        SetupCombatTestEnvironment();
        const std::int16_t atacante = 1;
        const std::int16_t victima = 2;

        UserList[atacante].Invent.WeaponEqpObjIndex = 10; // Espada Larga (MinHIT=10, MaxHIT=15)
        UserList[victima].Stats.MinHp = 200;
        UserList[victima].Invent.CascoEqpObjIndex = 51;   // Casco: MinDef = 5, MaxDef = 10
        UserList[victima].Invent.ArmourEqpObjIndex = 52;  // Armadura: MinDef = 15, MaxDef = 25
        UserList[victima].Invent.EscudoEqpObjIndex = 50;  // Escudo: MinDef = 5, MaxDef = 10

        // Caso 1: Sorteo de impacto en CABEZA (bCabeza = 1)
        // Solo absorbe casco (5 en min)
        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            if (lower == static_cast<std::int32_t>(PartesCuerpo::bCabeza) && upper == static_cast<std::int32_t>(PartesCuerpo::bTorso)) {
                return static_cast<std::int32_t>(PartesCuerpo::bCabeza);
            }
            return lower;
        });
        UserList[victima].Stats.MinHp = 200;
        const auto daño_cabeza = ao::UserDañoUser(atacante, victima);

        // Caso 2: Sorteo de impacto en TORSO (bTorso = 6)
        // Absorbe armadura (15) + escudo (5) = 20 en min
        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            if (lower == static_cast<std::int32_t>(PartesCuerpo::bCabeza) && upper == static_cast<std::int32_t>(PartesCuerpo::bTorso)) {
                return static_cast<std::int32_t>(PartesCuerpo::bTorso);
            }
            return lower;
        });
        UserList[victima].Stats.MinHp = 200;
        const auto daño_torso = ao::UserDañoUser(atacante, victima);

        // Como el torso absorbe 20 (armadura + escudo) y la cabeza absorbe 5 (casco),
        // el daño a la cabeza debe ser mayor al daño al torso
        CHECK(daño_cabeza > daño_torso);
        CHECK(daño_cabeza - daño_torso == 15); // Diferencia exacta de absorción (20 - 5 = 15)
    }

    TEST_CASE("Clamp de Absorción: golpe con absorción superior al daño bruto inflige 0 y no cura") {
        SetupCombatTestEnvironment();
        const std::int16_t atacante = 1;
        const std::int16_t victima = 2;

        // Desarmado y stats mínimos
        UserList[atacante].Invent.WeaponEqpObjIndex = 0;
        UserList[atacante].Stats.MinHIT = 1;
        UserList[atacante].Stats.MaxHIT = 1;
        UserList[atacante].Stats.UserAtributos[eAtributos::Fuerza] = 10;

        // Defensor con armadura masiva
        UserList[victima].Stats.MinHp = 100;
        UserList[victima].Invent.ArmourEqpObjIndex = 52;
        ObjDataList[52].MinDef = 500;
        ObjDataList[52].MaxDef = 500;

        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            if (lower == static_cast<std::int32_t>(PartesCuerpo::bCabeza)) {
                return static_cast<std::int32_t>(PartesCuerpo::bTorso);
            }
            return lower;
        });

        const auto daño_infligido = ao::UserDañoUser(atacante, victima);
        CHECK(daño_infligido == 0);
        // HP no debe aumentar (no cura) ni decrecer
        CHECK(UserList[victima].Stats.MinHp == 100);

        // Mismo clamp en UserDañoNpc con NPC de defensa extrema
        const std::int16_t npc_idx = 1;
        Npclist[npc_idx].Stats.MinHp = 100;
        Npclist[npc_idx].Stats.def = 500;
        const auto daño_npc = ao::UserDañoNpc(atacante, npc_idx);
        CHECK(daño_npc == 0);
        CHECK(Npclist[npc_idx].Stats.MinHp == 100);
    }

    TEST_CASE("CalcularDarExp: cálculo proporcional, acumulación y consumo de ExpCount") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        const std::int16_t npc_idx = 1;

        UserList[user_idx].Stats.Exp = 0;
        Npclist[npc_idx].Stats.MaxHp = 1000;
        Npclist[npc_idx].Stats.MinHp = 1000;
        Npclist[npc_idx].GiveEXP = 2000;
        Npclist[npc_idx].flags.ExpCount = 2000;

        // Golpe 1: 250 de daño -> 250 * (2000 / 1000) = 500 de exp
        ao::CalcularDarExp(user_idx, npc_idx, 250);
        CHECK(UserList[user_idx].Stats.Exp == 500);
        CHECK(Npclist[npc_idx].flags.ExpCount == 1500);

        // Golpe 2: 500 de daño -> 500 * 2 = 1000 de exp
        ao::CalcularDarExp(user_idx, npc_idx, 500);
        CHECK(UserList[user_idx].Stats.Exp == 1500);
        CHECK(Npclist[npc_idx].flags.ExpCount == 500);

        // Golpe 3 letal: 1000 de daño (clamp a MinHp = 250 si restamos HP o se agota ExpCount)
        Npclist[npc_idx].Stats.MinHp = 250;
        ao::CalcularDarExp(user_idx, npc_idx, 250);
        CHECK(UserList[user_idx].Stats.Exp == 2000);
        CHECK(Npclist[npc_idx].flags.ExpCount == 0);

        // Verificación de clamp contra MAXEXP
        UserList[user_idx].Stats.Exp = MAXEXP - 10;
        Npclist[npc_idx].Stats.MaxHp = 100;
        Npclist[npc_idx].Stats.MinHp = 100;
        Npclist[npc_idx].GiveEXP = 500;
        Npclist[npc_idx].flags.ExpCount = 500;
        ao::CalcularDarExp(user_idx, npc_idx, 100);
        CHECK(UserList[user_idx].Stats.Exp == MAXEXP);
    }

    TEST_CASE("RestarCriminalidad reduce BandidoRep y LadronesRep y clamp a cero") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;

        // BandidoRep
        UserList[user_idx].Reputacion.BandidoRep = 150;
        ao::RestarCriminalidad(user_idx);
        CHECK(UserList[user_idx].Reputacion.BandidoRep == 50); // 150 - vlASALTO(100) = 50

        ao::RestarCriminalidad(user_idx);
        CHECK(UserList[user_idx].Reputacion.BandidoRep == 0);  // 50 - 100 = -50 -> clamp a 0

        // LadronesRep
        UserList[user_idx].Reputacion.BandidoRep = 0;
        UserList[user_idx].Reputacion.LadronesRep = 80;
        ao::RestarCriminalidad(user_idx);
        CHECK(UserList[user_idx].Reputacion.LadronesRep == 30); // 80 - (vlCAZADOR(5)*10 = 50) = 30

        ao::RestarCriminalidad(user_idx);
        CHECK(UserList[user_idx].Reputacion.LadronesRep == 0);  // clamp a 0
    }

    TEST_CASE("UserEnvenena aplica estado y NpcDañoUser interrumpe meditación") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        const std::int16_t victima = 2;
        const std::int16_t npc_idx = 1;

        // 1. UserEnvenena con arma envenenada
        UserList[user_idx].Invent.WeaponEqpObjIndex = 53; // Daga Envenenada (Envenena = 1)
        UserList[victima].flags.Envenenado = 0;

        // Tirada exitosa (<= 60)
        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            return 30;
        });
        ao::UserEnvenena(user_idx, victima);
        CHECK(UserList[victima].flags.Envenenado == 1);

        // Tirada fallida (> 60)
        UserList[victima].flags.Envenenado = 0;
        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            return 80;
        });
        ao::UserEnvenena(user_idx, victima);
        CHECK(UserList[victima].flags.Envenenado == 0);

        // 2. NpcDañoUser interrumpe meditación
        UserList[victima].Stats.MinHp = 100;
        UserList[victima].flags.Meditando = true;
        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            return lower;
        });
        ao::NpcDañoUser(npc_idx, victima);
        CHECK_FALSE(UserList[victima].flags.Meditando);
        CHECK(UserList[victima].Stats.MinHp < 100);
    }
}

TEST_SUITE("SistemaCombate - Fase 4: G4 Orquestación de Ataque, Protocolo y Hooks") {

    TEST_CASE("UsuarioAtaca valida cooldown, cansancio de stamina y ruteo a celda frontal") {
        SetupCombatTestEnvironment();
        const std::int16_t atacante = 1;
        const std::int16_t victima = 2;

        UserList[atacante].Pos = WorldPos{1, 50, 50};
        UserList[atacante].char_appearance.heading = eHeading::NORTH;
        UserList[atacante].Stats.MinSta = 50;

        // Celda frontal al atacante mirando al NORTE: (50, 49)
        const auto frontal_idx = (1 * 101 + 50) * 101 + 49;
        MapData[frontal_idx].UserIndex = victima;
        UserList[victima].Pos = WorldPos{1, 50, 49};
        UserList[victima].Stats.MinHp = 100;
        UserList[victima].Stats.MaxHp = 100;

        // Caso 1: Cooldown impide atacar
        ao::SetCombatTimeProvider([]() { return 1000; });
        UserList[atacante].Counters.TimerPuedeAtacar = 2000;
        const auto sta_antes = UserList[atacante].Stats.MinSta;
        ao::UsuarioAtaca(atacante);
        CHECK(UserList[atacante].Stats.MinSta == sta_antes);

        // Caso 2: Cooldown permite atacar pero stamina es insuficiente (< 10)
        UserList[atacante].Counters.TimerPuedeAtacar = 0;
        ao::SetCombatTimeProvider([]() { return 10000; });
        UserList[atacante].Stats.MinSta = 5;
        ao::UsuarioAtaca(atacante);
        CHECK(UserList[atacante].Stats.MinSta == 5);

        // Caso 3: Stamina suficiente (>= 10) -> descuenta stamina y ataca
        UserList[atacante].Counters.TimerPuedeAtacar = 0;
        ao::SetCombatTimeProvider([]() { return 20000; });
        UserList[atacante].Stats.MinSta = 20;
        ao::SetCombatRandomProvider([](std::int32_t lower, std::int32_t upper) {
            if (lower == 1 && upper == 10) return 5; // consume 5 de stamina
            return lower;
        });
        ao::UsuarioAtaca(atacante);
        CHECK(UserList[atacante].Stats.MinSta == 15);
    }

    TEST_CASE("Criterio 7.4: Sigilo de GM Invisible - audio SND_SWING privado si AdminInvisible == 1") {
        SetupCombatTestEnvironment();
        const std::int16_t gm = 1;
        const std::int16_t normal_user = 2;
        const std::int16_t observador = 3;

        // Configuramos ConnGroups para el mapa 1
        ConnGroups.assign(2, ConnGroup{});
        ConnGroups[1].CountEntrys = 1;
        ConnGroups[1].UserEntrys.assign(2, 0);
        ConnGroups[1].UserEntrys[1] = observador;

        UserList[observador].flags.UserLogged = true;
        UserList[observador].ConnIDValida = 1;
        UserList[observador].Pos = WorldPos{1, 50, 50};
        UserList[observador].AreasInfo.AreaReciveX = 0xFFFF;
        UserList[observador].AreasInfo.AreaReciveY = 0xFFFF;

        UserList[gm].AreasInfo.AreaPerteneceX = 1;
        UserList[gm].AreasInfo.AreaPerteneceY = 1;

        UserList[normal_user].AreasInfo.AreaPerteneceX = 1;
        UserList[normal_user].AreasInfo.AreaPerteneceY = 1;

        ao::SetCombatTimeProvider([]() { return 10000; });
        UserList[gm].Counters.TimerPuedeAtacar = 0;
        UserList[gm].Pos = WorldPos{1, 50, 50};
        UserList[gm].char_appearance.heading = eHeading::NORTH;
        UserList[gm].Stats.MinSta = 50;
        UserList[gm].flags.AdminInvisible = 1;

        std::vector<std::int16_t> receptores;
        TCP::SetSendDataHook([&receptores](std::int16_t slot, std::string_view) {
            receptores.push_back(slot);
        });

        // Ataque al aire de GM Invisible
        ao::UsuarioAtaca(gm);

        // Debe enviarse ÚNICAMENTE al socket del GM, no a ToPCArea
        CHECK(receptores.size() == 1);
        CHECK(receptores[0] == gm);

        // Ahora probamos usuario normal con AdminInvisible == 0
        receptores.clear();
        UserList[normal_user].Counters.TimerPuedeAtacar = 0;
        UserList[normal_user].Pos = WorldPos{1, 50, 50};
        UserList[normal_user].char_appearance.heading = eHeading::NORTH;
        UserList[normal_user].Stats.MinSta = 50;
        UserList[normal_user].flags.AdminInvisible = 0;

        ao::UsuarioAtaca(normal_user);
        // Para usuario normal, SendData ToPCArea transmite al observador en el área
        CHECK(receptores.size() == 1);
        CHECK(receptores[0] == observador);
    }

    TEST_CASE("Criterio 7.3: Legítima Defensa - AtacablePor bypass de karma, StoreFrag y ContarMuerte") {
        SetupCombatTestEnvironment();
        const std::int16_t atacante = 1;
        const std::int16_t agresor_victima = 2;

        UserList[atacante].Reputacion.BandidoRep = 0;
        UserList[atacante].Reputacion.NobleRep = 1000;
        UserList[agresor_victima].Stats.MinHp = 50;
        UserList[agresor_victima].Stats.MaxHp = 100;
        UserList[atacante].Reputacion.PlebeRep = 100;
        UserList[agresor_victima].Reputacion.PlebeRep = 100;

        // Marcamos legítima defensa: el agresor atacó antes al atacante
        UserList[agresor_victima].flags.AtacablePor = atacante;
        UserList[atacante].flags.Seguro = true;

        // 1. PuedeAtacar debe permitirlo aún con Seguro activo
        CHECK(ao::PuedeAtacar(atacante, agresor_victima));

        bool criminal_called = false;
        bool store_frag_called = false;
        bool contar_muerte_called = false;
        bool user_die_called = false;

        ao::SetVolverCriminalHook([&](std::int16_t) { criminal_called = true; });
        ao::SetStoreFragHook([&](std::int16_t, std::int16_t) { store_frag_called = true; });
        ao::SetContarMuerteHook([&](std::int16_t, std::int16_t) { contar_muerte_called = true; });
        ao::SetUserDieHook([&](std::int16_t) { user_die_called = true; });

        // 2. Al ser atacado, NO aumenta BandidoRep ni llama VolverCriminalHook
        ao::UsuarioAtacadoPorUsuario(atacante, agresor_victima);
        CHECK(UserList[atacante].Reputacion.BandidoRep == 0);
        CHECK_FALSE(criminal_called);

        // 3. Golpe mortal en legítima defensa
        ao::SetCombatRandomProvider([](std::int32_t, std::int32_t) {
            return 100; // daño letal
        });
        UserList[agresor_victima].Stats.MinHp = 10;
        ao::UserDañoUser(atacante, agresor_victima);

        // La víctima murió
        CHECK(user_die_called);
        // Criterio 7.3: bypass total de StoreFrag y ContarMuerte
        CHECK_FALSE(store_frag_called);
        CHECK_FALSE(contar_muerte_called);
        CHECK_FALSE(criminal_called);
    }

    TEST_CASE("UsuarioAtacadoPorUsuario interrumpe meditación y cancela salida") {
        SetupCombatTestEnvironment();
        const std::int16_t atacante = 1;
        const std::int16_t victima = 2;

        UserList[victima].flags.Meditando = true;
        UserList[victima].char_appearance.FX = 4;
        UserList[victima].char_appearance.loops = 10;
        UserList[victima].Counters.Saliendo = true;

        bool cancel_exit_called = false;
        ao::SetCancelExitHook([&](std::int16_t user_idx) {
            if (user_idx == victima) {
                cancel_exit_called = true;
                UserList[user_idx].Counters.Saliendo = false;
            }
        });

        ao::UsuarioAtacadoPorUsuario(atacante, victima);

        CHECK_FALSE(UserList[victima].flags.Meditando);
        CHECK(UserList[victima].char_appearance.FX == 0);
        CHECK(UserList[victima].char_appearance.loops == 0);
        CHECK_FALSE(UserList[victima].Counters.Saliendo);
        CHECK(cancel_exit_called);
    }

    TEST_CASE("PuedeAtacar matriz legal: Seguro, Zonas Seguras y Trigger 6") {
        SetupCombatTestEnvironment();
        const std::int16_t user1 = 1;
        const std::int16_t user2 = 2;

        UserList[user1].Pos = WorldPos{1, 50, 50};
        UserList[user2].Pos = WorldPos{1, 50, 51};

        // Ciudadano con seguro atacando a ciudadano sin AtacablePor -> false
        UserList[user1].flags.Seguro = true;
        CHECK_FALSE(ao::PuedeAtacar(user1, user2));

        // Ciudadano sin seguro atacando a ciudadano en mapa seguro (Pk = false)
        UserList[user1].flags.Seguro = false;
        MapInfoList[1].Pk = false;
        CHECK_FALSE(ao::PuedeAtacar(user1, user2));

        // En mapa seguro con Trigger 6 (ZONAPELEA) en ambas celdas -> permite
        const auto idx1 = (1 * 101 + 50) * 101 + 50;
        const auto idx2 = (1 * 101 + 50) * 101 + 51;
        MapData[idx1].trigger = eTrigger::ZONAPELEA;
        MapData[idx2].trigger = eTrigger::ZONAPELEA;
        CHECK(ao::PuedeAtacar(user1, user2));

        // Trigger 6 solo en una celda -> prohibe
        MapData[idx2].trigger = eTrigger::NADA;
        CHECK_FALSE(ao::PuedeAtacar(user1, user2));

        // Armada Real vs Armada Real -> prohibido
        MapData[idx1].trigger = eTrigger::NADA;
        MapInfoList[1].Pk = true;
        UserList[user1].Faccion.ArmadaReal = 1;
        UserList[user2].Faccion.ArmadaReal = 1;
        CHECK_FALSE(ao::PuedeAtacar(user1, user2));

        // Legión Oscura vs Legión Oscura -> prohibido
        UserList[user1].Faccion.ArmadaReal = 0;
        UserList[user2].Faccion.ArmadaReal = 0;
        UserList[user1].Faccion.FuerzasCaos = 1;
        UserList[user2].Faccion.FuerzasCaos = 1;
        UserList[user1].Reputacion.AsesinoRep = 1000;
        UserList[user2].Reputacion.AsesinoRep = 1000;
        CHECK_FALSE(ao::PuedeAtacar(user1, user2));
    }

    TEST_CASE("PuedeAtacarNPC y reglas de Guardia Real y Guardia del Caos") {
        SetupCombatTestEnvironment();
        const std::int16_t user_idx = 1;
        const std::int16_t npc_guardia = 1;

        UserList[user_idx].Pos = WorldPos{1, 50, 50};
        Npclist[npc_guardia].Pos = WorldPos{1, 50, 51};
        Npclist[npc_guardia].Attackable = 1;
        Npclist[npc_guardia].Hostile = 0;

        // Guardia Real atacada por miembro de Armada Real -> false
        Npclist[npc_guardia].NPCtype = eNPCType::GuardiaReal;
        UserList[user_idx].Faccion.ArmadaReal = 1;
        CHECK_FALSE(ao::PuedeAtacarNPC(user_idx, npc_guardia));

        // Guardia Real atacada por ciudadano con seguro activo -> false
        UserList[user_idx].Faccion.ArmadaReal = 0;
        UserList[user_idx].flags.Seguro = true;
        CHECK_FALSE(ao::PuedeAtacarNPC(user_idx, npc_guardia));

        // Guardia Real atacada por ciudadano sin seguro -> true y vuelve criminal
        UserList[user_idx].flags.Seguro = false;
        bool criminal_called = false;
        ao::SetVolverCriminalHook([&](std::int16_t) { criminal_called = true; });
        CHECK(ao::PuedeAtacarNPC(user_idx, npc_guardia));
        CHECK(criminal_called);

        // Guardia del Caos atacada por miembro del Caos -> false
        Npclist[npc_guardia].NPCtype = eNPCType::Guardiascaos;
        UserList[user_idx].Faccion.FuerzasCaos = 1;
        CHECK_FALSE(ao::PuedeAtacarNPC(user_idx, npc_guardia));
    }

    TEST_CASE("Mascotas: MuereNpc, RestarCriaturasEntrenador, AllFollowAmo y AllMascotasAtacanUser") {
        SetupCombatTestEnvironment();
        const std::int16_t amo = 1;
        const std::int16_t victim = 2;
        const std::int16_t pet = 1;

        UserList[amo].MascotasIndex[1] = pet;
        UserList[amo].MascotasType[1] = 10;
        UserList[amo].NroMascotas = 1;
        Npclist[pet].MaestroUser = amo;

        // 1. AllFollowAmo
        ao::AllFollowAmo(amo);
        CHECK(Npclist[pet].Target == 0);
        CHECK(Npclist[pet].TargetNPC == 0);
        CHECK(Npclist[pet].Movement == TipoAI::SigueAmo);

        // 2. AllMascotasAtacanUser
        UserList[victim].name = "Victima";
        ao::AllMascotasAtacanUser(victim, amo);
        CHECK(Npclist[pet].flags.AttackedBy == "Victima");
        CHECK(Npclist[pet].Movement == TipoAI::NPCDEFENSA);
        CHECK(Npclist[pet].Hostile == 1);
        CHECK(Npclist[pet].Target == victim);

        // 3. MuereNpc
        bool muere_npc_called = false;
        ao::SetMuereNpcHook([&](std::int16_t n_idx, std::int16_t u_idx) {
            if (n_idx == pet && u_idx == amo) muere_npc_called = true;
        });
        ao::MuereNpc(pet, amo);
        CHECK(muere_npc_called);
        CHECK(UserList[amo].MascotasIndex[1] == 0);
        CHECK(UserList[amo].MascotasType[1] == 0);
        CHECK(UserList[amo].NroMascotas == 0);
    }
}

