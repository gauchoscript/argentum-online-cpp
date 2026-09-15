#include <doctest/doctest.h>

#include "server/modHechizos.hpp"
#include "server/Declares.hpp"
#include "server/Protocol.hpp"

#include <string>
#include <vector>

namespace {

void SetupTestEnvironment() {
    ao::ResetSpellsCallbacks();

    if (UserList.size() < 10) {
        UserList.resize(10);
    }
    if (ObjDataList.size() < 50) {
        ObjDataList.resize(50);
    }
    if (Hechizos.size() < 50) {
        Hechizos.resize(50);
    }

    if (MapInfoList.size() < 10) {
        MapInfoList.resize(10);
    }
    MapInfoList[1] = MapInfo{};
    MapInfoList[1].Pk = true;

    if (MapData.size() < 2 * 101 * 101) {
        MapData.assign(2 * 101 * 101, MapBlock{});
    }

    IntervaloParalizado = 500;
    IntervaloInvocacion = 1000;

    // Inicialización limpia de UserList[1]
    auto& u = UserList[1];
    u = User{};
    u.flags.UserLogged = true;
    u.char_appearance.CharIndex = 1;
    u.Pos.Map = 1;
    u.Pos.X = 50;
    u.Pos.Y = 50;
    u.Genero = eGenero::Hombre;
    u.clase = eClass::Mage;
    u.Stats.MinHp = 100;
    u.Stats.MaxHp = 100;
    u.Stats.MinMAN = 1000;
    u.Stats.MaxMAN = 1000;
    u.Stats.MinSta = 500;
    u.Stats.MaxSta = 500;
    u.Stats.UserSkills[static_cast<std::size_t>(eSkill::Magia)] = 100;
}

} // namespace

TEST_SUITE("modHechizos_Fase1") {

TEST_CASE("Fase 1: TieneHechizo busca linealmente en los 35 slots") {
    SetupTestEnvironment();
    auto& u = UserList[1];

    CHECK_FALSE(ao::TieneHechizo(10, 1));
    CHECK_FALSE(ao::TieneHechizo(10, 0));
    CHECK_FALSE(ao::TieneHechizo(10, 99));

    u.Stats.UserHechizos[1] = 5;
    u.Stats.UserHechizos[18] = 10;
    u.Stats.UserHechizos[35] = 25;

    CHECK(ao::TieneHechizo(5, 1));
    CHECK(ao::TieneHechizo(10, 1));
    CHECK(ao::TieneHechizo(25, 1));
    CHECK_FALSE(ao::TieneHechizo(99, 1));
}

TEST_CASE("Fase 1: AgregarHechizo asigna en primer slot libre y deduce pergamino") {
    SetupTestEnvironment();
    auto& u = UserList[1];

    // Configurar pergamino en inventario slot 1
    constexpr std::int16_t OBJ_PERGAMINO = 10;
    constexpr std::int16_t HECHIZO_MISIL = 2;
    ObjDataList[OBJ_PERGAMINO].HechizoIndex = HECHIZO_MISIL;

    u.Invent.Object[1].ObjIndex = OBJ_PERGAMINO;
    u.Invent.Object[1].Amount = 1;

    bool item_quitado = false;
    std::uint8_t slot_quitado = 0;
    std::int16_t cant_quitada = 0;

    ao::SpellsCallbacks cbs{};
    cbs.QuitarUserInvItem = [&](std::int16_t user_idx, std::uint8_t slot, std::int16_t amount) {
        if (user_idx == 1) {
            item_quitado = true;
            slot_quitado = slot;
            cant_quitada = amount;
        }
    };
    ao::SetSpellsCallbacks(cbs);

    // 1. Agregar hechizo en libro vacío -> va al slot 1
    ao::AgregarHechizo(1, 1);
    CHECK(u.Stats.UserHechizos[1] == HECHIZO_MISIL);
    CHECK(item_quitado);
    CHECK(slot_quitado == 1);
    CHECK(cant_quitada == 1);

    // 2. Intentar agregar el mismo hechizo nuevamente -> rechazado por duplicado
    item_quitado = false;
    ao::AgregarHechizo(1, 1);
    CHECK_FALSE(item_quitado);

    // 3. Agregar otro hechizo cuando los slots 1..34 están ocupados
    constexpr std::int16_t OBJ_PERGAMINO_2 = 11;
    constexpr std::int16_t HECHIZO_TORMENTA = 3;
    ObjDataList[OBJ_PERGAMINO_2].HechizoIndex = HECHIZO_TORMENTA;
    u.Invent.Object[2].ObjIndex = OBJ_PERGAMINO_2;
    u.Invent.Object[2].Amount = 1;

    for (int i = 1; i <= 34; ++i) {
        u.Stats.UserHechizos[i] = static_cast<std::int16_t>(100 + i);
    }
    u.Stats.UserHechizos[35] = 0;

    ao::AgregarHechizo(1, 2);
    CHECK(u.Stats.UserHechizos[35] == HECHIZO_TORMENTA);
    CHECK(item_quitado);

    // 4. Intentar agregar cuando el libro está 100% lleno (slots 1..35 ocupados)
    item_quitado = false;
    constexpr std::int16_t OBJ_PERGAMINO_3 = 12;
    constexpr std::int16_t HECHIZO_DESCARGA = 4;
    ObjDataList[OBJ_PERGAMINO_3].HechizoIndex = HECHIZO_DESCARGA;
    u.Invent.Object[3].ObjIndex = OBJ_PERGAMINO_3;
    u.Invent.Object[3].Amount = 1;

    ao::AgregarHechizo(1, 3);
    CHECK_FALSE(item_quitado);
    CHECK_FALSE(ao::TieneHechizo(HECHIZO_DESCARGA, 1));
}

TEST_CASE("Fase 1: DesplazarHechizo reordena correctamente y respeta límites") {
    SetupTestEnvironment();
    auto& u = UserList[1];

    u.Stats.UserHechizos[1] = 10;
    u.Stats.UserHechizos[2] = 20;
    u.Stats.UserHechizos[3] = 30;
    u.Stats.UserHechizos[34] = 40;
    u.Stats.UserHechizos[35] = 50;

    // Límite superior: Mover arriba el slot 1 no debe hacer nada
    ao::DesplazarHechizo(1, 1, 1);
    CHECK(u.Stats.UserHechizos[1] == 10);
    CHECK(u.Stats.UserHechizos[2] == 20);

    // Mover arriba el slot 2 -> intercambia 1 y 2
    ao::DesplazarHechizo(1, 1, 2);
    CHECK(u.Stats.UserHechizos[1] == 20);
    CHECK(u.Stats.UserHechizos[2] == 10);

    // Mover abajo el slot 1 -> vuelve a intercambiar 1 y 2
    ao::DesplazarHechizo(1, -1, 1);
    CHECK(u.Stats.UserHechizos[1] == 10);
    CHECK(u.Stats.UserHechizos[2] == 20);

    // Límite inferior: Mover abajo el slot 35 no debe hacer nada
    ao::DesplazarHechizo(1, -1, 35);
    CHECK(u.Stats.UserHechizos[34] == 40);
    CHECK(u.Stats.UserHechizos[35] == 50);

    // Mover abajo el slot 34 -> intercambia 34 y 35
    ao::DesplazarHechizo(1, -1, 34);
    CHECK(u.Stats.UserHechizos[34] == 50);
    CHECK(u.Stats.UserHechizos[35] == 40);

    // Parámetros inválidos
    ao::DesplazarHechizo(1, 0, 5);  // Dirección inválida
    ao::DesplazarHechizo(1, 1, 0);  // Slot fuera de rango
    ao::DesplazarHechizo(1, 1, 36); // Slot fuera de rango
    CHECK(u.Stats.UserHechizos[34] == 50);
}

TEST_CASE("Fase 1: PuedeLanzar valida vida, báculos, skill, estamina y maná") {
    SetupTestEnvironment();
    auto& u = UserList[1];

    constexpr std::int16_t SPELL_TEST = 1;
    auto& h = Hechizos[SPELL_TEST];
    h = tHechizo{};
    h.Nombre = "Test Spell";
    h.ManaRequerido = 100;
    h.StaRequerido = 50;
    h.MinSkill = 20;

    // 1. Caso base: Todo válido
    CHECK(ao::PuedeLanzar(1, SPELL_TEST));

    // 2. Muerto
    u.flags.Muerto = 1;
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_TEST));
    u.flags.Muerto = 0;

    // 3. Skill insuficiente
    u.Stats.UserSkills[static_cast<std::size_t>(eSkill::Magia)] = 19;
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_TEST));
    u.Stats.UserSkills[static_cast<std::size_t>(eSkill::Magia)] = 20;
    CHECK(ao::PuedeLanzar(1, SPELL_TEST));

    // 4. Estamina insuficiente
    u.Stats.MinSta = 49;
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_TEST));
    u.Genero = eGenero::Mujer;
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_TEST));
    u.Stats.MinSta = 50;
    CHECK(ao::PuedeLanzar(1, SPELL_TEST));

    // 5. Maná insuficiente
    u.Stats.MinMAN = 99;
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_TEST));
    u.Stats.MinMAN = 100;
    CHECK(ao::PuedeLanzar(1, SPELL_TEST));

    // 6. Requisito de báculo para Magos (NeedStaff)
    h.NeedStaff = 30;
    u.clase = eClass::Mage;

    // Sin báculo equipado
    u.Invent.WeaponEqpObjIndex = 0;
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_TEST));

    // Con báculo de menor poder
    constexpr std::int16_t STAFF_DEBIL = 20;
    ObjDataList[STAFF_DEBIL].StaffPower = 29;
    u.Invent.WeaponEqpObjIndex = STAFF_DEBIL;
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_TEST));

    // Con báculo de poder suficiente
    constexpr std::int16_t STAFF_PODEROSO = 21;
    ObjDataList[STAFF_PODEROSO].StaffPower = 30;
    u.Invent.WeaponEqpObjIndex = STAFF_PODEROSO;
    CHECK(ao::PuedeLanzar(1, SPELL_TEST));

    // Clase no-mago no requiere báculo aunque NeedStaff > 0
    u.clase = eClass::Cleric;
    u.Invent.WeaponEqpObjIndex = 0;
    CHECK(ao::PuedeLanzar(1, SPELL_TEST));
}

TEST_CASE("Fase 1: Bonificaciones de Druida con Flauta Élfica en PuedeLanzar") {
    SetupTestEnvironment();
    auto& u = UserList[1];
    u.clase = eClass::Druid;
    u.Invent.AnilloEqpObjIndex = ao::FLAUTAELFICA;

    // 1. Hechizo de Mimetismo: 50% de descuento en maná
    constexpr std::int16_t SPELL_MIMETISMO = 10;
    Hechizos[SPELL_MIMETISMO] = tHechizo{};
    Hechizos[SPELL_MIMETISMO].Mimetiza = 1;
    Hechizos[SPELL_MIMETISMO].ManaRequerido = 100;
    Hechizos[SPELL_MIMETISMO].MinSkill = 0;
    Hechizos[SPELL_MIMETISMO].StaRequerido = 0;

    u.Stats.MinMAN = 49;
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_MIMETISMO));
    u.Stats.MinMAN = 50; // 100 * 0.5 = 50
    CHECK(ao::PuedeLanzar(1, SPELL_MIMETISMO));

    // Sin Flauta Élfica no tiene descuento
    u.Invent.AnilloEqpObjIndex = 0;
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_MIMETISMO));
    u.Invent.AnilloEqpObjIndex = ao::FLAUTAELFICA;

    // 2. Invocaciones: 30% de descuento (requiere 70%)
    constexpr std::int16_t SPELL_INVOCACION = 11;
    Hechizos[SPELL_INVOCACION] = tHechizo{};
    Hechizos[SPELL_INVOCACION].Tipo = TipoHechizo::uInvocacion;
    Hechizos[SPELL_INVOCACION].ManaRequerido = 100;

    u.Stats.MinMAN = 69;
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_INVOCACION));
    u.Stats.MinMAN = 70; // 100 * 0.7 = 70
    CHECK(ao::PuedeLanzar(1, SPELL_INVOCACION));

    // 3. Magia general: 10% de descuento (requiere 90%)
    constexpr std::int16_t SPELL_GENERAL = 12;
    Hechizos[SPELL_GENERAL] = tHechizo{};
    Hechizos[SPELL_GENERAL].ManaRequerido = 100;

    u.Stats.MinMAN = 89;
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_GENERAL));
    u.Stats.MinMAN = 90; // 100 * 0.9 = 90
    CHECK(ao::PuedeLanzar(1, SPELL_GENERAL));

    // 4. Apocalipsis (índice 25) NO tiene descuento del 10%
    Hechizos[ao::APOCALIPSIS_SPELL_INDEX] = tHechizo{};
    Hechizos[ao::APOCALIPSIS_SPELL_INDEX].ManaRequerido = 1000;

    u.Stats.MinMAN = 900;
    CHECK_FALSE(ao::PuedeLanzar(1, ao::APOCALIPSIS_SPELL_INDEX));
    u.Stats.MinMAN = 1000;
    CHECK(ao::PuedeLanzar(1, ao::APOCALIPSIS_SPELL_INDEX));

    // 5. Warp de mascotas: Exige 100% de maná y al menos 1 mascota
    constexpr std::int16_t SPELL_WARP = 13;
    Hechizos[SPELL_WARP] = tHechizo{};
    Hechizos[SPELL_WARP].Warp = 1;
    Hechizos[SPELL_WARP].ManaRequerido = 50;

    u.Stats.MaxMAN = 1000;
    u.Stats.MinMAN = 999; // No tiene toda la maná
    u.NroMascotas = 1;
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_WARP));

    u.Stats.MinMAN = 1000;
    u.NroMascotas = 0; // No tiene mascotas
    CHECK_FALSE(ao::PuedeLanzar(1, SPELL_WARP));

    u.NroMascotas = 1; // Maná al 100% y tiene mascotas
    CHECK(ao::PuedeLanzar(1, SPELL_WARP));
}

TEST_CASE("Fase 1: DecirPalabrasMagicas revela usuarios ocultos y respeta admin invisible") {
    SetupTestEnvironment();
    auto& u = UserList[1];

    u.flags.Oculto = 1;
    u.Counters.TiempoOculto = 50;
    u.flags.invisible = 0;
    u.flags.AdminInvisible = 0;

    bool invisible_changed = false;
    bool set_invi_val = true;
    ao::SpellsCallbacks cbs{};
    cbs.SetInvisible = [&](std::int16_t user_idx, std::int16_t, bool invi) {
        if (user_idx == 1) {
            invisible_changed = true;
            set_invi_val = invi;
        }
    };
    ao::SetSpellsCallbacks(cbs);

    // Usuario oculto al decir palabras mágicas se revela
    ao::DecirPalabrasMagicas("Vas Ort Flam", 1);
    CHECK(u.flags.Oculto == 0);
    CHECK(u.Counters.TiempoOculto == 0);
    CHECK(invisible_changed);
    CHECK_FALSE(set_invi_val);

    // Admin invisible no desoculta ni emite
    u.flags.AdminInvisible = 1;
    u.flags.Oculto = 1;
    u.Counters.TiempoOculto = 50;
    invisible_changed = false;

    ao::DecirPalabrasMagicas("Vas Ort Flam", 1);
    CHECK(u.flags.Oculto == 1);
    CHECK(u.Counters.TiempoOculto == 50);
    CHECK_FALSE(invisible_changed);
}

} // TEST_SUITE("modHechizos_Fase1")

TEST_SUITE("modHechizos_Fase2") {

TEST_CASE("Fase 2: Curación en usuario escala con 3% * ELV y respeta tope de MaxHp") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    auto& target = UserList[2];

    target = User{};
    target.flags.UserLogged = true;
    target.char_appearance.CharIndex = 2;
    target.Stats.MinHp = 50;
    target.Stats.MaxHp = 200;

    caster.flags.TargetUser = 2;
    caster.flags.Hechizo = 1;

    auto& h = Hechizos[1];
    h = tHechizo{};
    h.SubeHP = 1;
    h.MinHp = 100;
    h.MaxHp = 100;

    // Con ELV = 1: daño = 100 + Porcentaje(100, 3) = 103. Vida final = 50 + 103 = 153
    caster.Stats.ELV = 1;
    CHECK(ao::HechizoPropUsuario(1));
    CHECK(target.Stats.MinHp == 153);

    // Con ELV = 25: daño = 100 + Porcentaje(100, 75) = 175. Vida final se capea a MaxHp (200)
    target.Stats.MinHp = 50;
    caster.Stats.ELV = 25;
    CHECK(ao::HechizoPropUsuario(1));
    CHECK(target.Stats.MinHp == 200);

    // Rechazo de curación a usuario muerto
    target.flags.Muerto = true;
    CHECK_FALSE(ao::HechizoPropUsuario(1));

    // Rechazo si el casteador está en consulta
    target.flags.Muerto = false;
    caster.flags.EnConsulta = true;
    CHECK_FALSE(ao::HechizoPropUsuario(1));
}

TEST_CASE("Fase 2: Daño mágico PvP aplica modificador de báculo, bardo y mitigación antimagia") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    auto& target = UserList[2];

    target = User{};
    target.flags.UserLogged = true;
    target.char_appearance.CharIndex = 2;
    target.Stats.MinHp = 200;
    target.Stats.MaxHp = 200;

    caster.flags.TargetUser = 2;
    caster.flags.Hechizo = 2;
    caster.clase = eClass::Mage;
    caster.Stats.ELV = 0; // Sin escalado por nivel para simplificar

    auto& h = Hechizos[2];
    h = tHechizo{};
    h.SubeHP = 2;
    h.MinHp = 100;
    h.MaxHp = 100;
    h.StaffAffected = true;

    // 1. Autoataque prohibido
    caster.flags.TargetUser = 1;
    CHECK_FALSE(ao::HechizoPropUsuario(1));
    caster.flags.TargetUser = 2;

    // 2. Mago desarmado sin vara con StaffAffected: penalización al 70%
    caster.Invent.WeaponEqpObjIndex = 0;
    target.Stats.MinHp = 200;
    CHECK(ao::HechizoPropUsuario(1));
    // Daño = 100 * 0.7 = 70. Vida restante = 200 - 70 = 130
    CHECK(target.Stats.MinHp == 130);

    // 3. Mago con báculo equipado (StaffDamageBonus = 20): (100 * (20 + 70)) / 100 = 90
    target.Stats.MinHp = 200;
    auto& staff = ObjDataList[10];
    staff = ObjData{};
    staff.StaffDamageBonus = 20;
    caster.Invent.WeaponEqpObjIndex = 10;
    CHECK(ao::HechizoPropUsuario(1));
    CHECK(target.Stats.MinHp == 110);

    // 4. Bardo con Laud Élfico equipado (+4% daño)
    caster.clase = eClass::Bard;
    h.StaffAffected = false;
    caster.Invent.AnilloEqpObjIndex = ao::LAUDELFICO;
    target.Stats.MinHp = 200;
    CHECK(ao::HechizoPropUsuario(1));
    // Daño = 100 * 1.04 = 104. Vida = 200 - 104 = 96
    CHECK(target.Stats.MinHp == 96);
    caster.Invent.AnilloEqpObjIndex = 0;

    // 5. Mitigación de Casco y Anillo antimagia de la víctima
    target.Stats.MinHp = 200;
    auto& helmet = ObjDataList[20];
    helmet = ObjData{};
    helmet.DefensaMagicaMin = 30;
    helmet.DefensaMagicaMax = 30;
    target.Invent.CascoEqpObjIndex = 20;

    auto& ring = ObjDataList[21];
    ring = ObjData{};
    ring.DefensaMagicaMin = 20;
    ring.DefensaMagicaMax = 20;
    target.Invent.AnilloEqpObjIndex = 21;

    CHECK(ao::HechizoPropUsuario(1));
    // Daño base 100 - 30 (casco) - 20 (anillo) = 50. Vida = 200 - 50 = 150
    CHECK(target.Stats.MinHp == 150);

    // 6. Mitigación superior al daño clampea a 0 sin subdesbordamiento
    target.Stats.MinHp = 200;
    helmet.DefensaMagicaMin = 150;
    helmet.DefensaMagicaMax = 150;
    CHECK(ao::HechizoPropUsuario(1));
    CHECK(target.Stats.MinHp == 200);
}

TEST_CASE("Fase 2: Muerte de usuario en PvP dispara callbacks de combate y estadísticas") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    auto& target = UserList[2];

    target = User{};
    target.flags.UserLogged = true;
    target.Stats.MinHp = 30;
    target.Stats.MaxHp = 100;
    target.flags.AtacablePor = 0;

    caster.flags.TargetUser = 2;
    caster.flags.Hechizo = 3;
    caster.Stats.ELV = 0;

    auto& h = Hechizos[3];
    h = tHechizo{};
    h.SubeHP = 2;
    h.MinHp = 50;
    h.MaxHp = 50;

    bool atacado_disparado = false;
    bool store_frag_disparado = false;
    bool contar_muerte_disparado = false;
    bool act_stats_disparado = false;
    bool user_die_disparado = false;

    ao::SpellsCallbacks cbs{};
    cbs.UsuarioAtacadoPorUsuario = [&](std::int16_t att, std::int16_t vic) {
        if (att == 1 && vic == 2) atacado_disparado = true;
    };
    cbs.StoreFrag = [&](std::int16_t killer, std::int16_t vic) {
        if (killer == 1 && vic == 2) store_frag_disparado = true;
    };
    cbs.ContarMuerte = [&](std::int16_t vic, std::int16_t killer) {
        if (vic == 2 && killer == 1) contar_muerte_disparado = true;
    };
    cbs.ActStats = [&](std::int16_t vic, std::int16_t killer) {
        if (vic == 2 && killer == 1) act_stats_disparado = true;
    };
    cbs.UserDie = [&](std::int16_t vic) {
        if (vic == 2) user_die_disparado = true;
    };
    ao::SetSpellsCallbacks(cbs);

    CHECK(ao::HechizoPropUsuario(1));
    CHECK(target.Stats.MinHp == 0);
    CHECK(atacado_disparado);
    CHECK(store_frag_disparado);
    CHECK(contar_muerte_disparado);
    CHECK(act_stats_disparado);
    CHECK(user_die_disparado);

    // Si la víctima ya estaba en estado AtacablePor el agresor (legítima defensa), NO cuenta frag
    target.Stats.MinHp = 30;
    target.flags.AtacablePor = 1;
    store_frag_disparado = false;
    contar_muerte_disparado = false;

    CHECK(ao::HechizoPropUsuario(1));
    CHECK_FALSE(store_frag_disparado);
    CHECK_FALSE(contar_muerte_disparado);
}

TEST_CASE("Fase 2: REPLICACIÓN BUG #38 — Magia pura de maná/energía no muta atributos, traslada residuo si es combinada") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    auto& target = UserList[2];

    target = User{};
    target.flags.UserLogged = true;
    target.Stats.MinMAN = 200;
    target.Stats.MaxMAN = 500;
    target.Stats.MinSta = 100;
    target.Stats.MaxSta = 300;

    caster.flags.TargetUser = 2;
    caster.flags.Hechizo = 4;

    // 1. Magia pura de Maná: MiMana y MaMana están definidos, pero NO se evalúan en VB6
    auto& h1 = Hechizos[4];
    h1 = tHechizo{};
    h1.SubeMana = 1;
    h1.MiMana = 100;
    h1.MaMana = 100;

    CHECK(ao::HechizoPropUsuario(1));
    // Bug #38: variable local daño es 0 => MinMAN suma 0!
    CHECK(target.Stats.MinMAN == 200);

    // 2. Magia pura de Energía (Stamina): daño es 0 => MinSta suma 0!
    auto& h2 = Hechizos[5];
    h2 = tHechizo{};
    h2.SubeSta = 1;
    h2.MinSta = 50;
    h2.MaxSta = 50;
    caster.flags.Hechizo = 5;

    CHECK(ao::HechizoPropUsuario(1));
    CHECK(target.Stats.MinSta == 100);

    // 3. Hechizo combinado: SubeHP = 1 primero calcula daño, y luego SubeMana = 1 toma ese daño residual
    auto& h3 = Hechizos[6];
    h3 = tHechizo{};
    h3.SubeHP = 1;
    h3.MinHp = 80;
    h3.MaxHp = 80;
    h3.SubeMana = 1;
    h3.MiMana = 10; // Debería ser ignorado por el Bug #38
    h3.MaMana = 10;
    caster.Stats.ELV = 0;
    caster.flags.Hechizo = 6;

    target.Stats.MinHp = 50;
    target.Stats.MaxHp = 200;
    target.Stats.MinMAN = 200;

    CHECK(ao::HechizoPropUsuario(1));
    // HP subió en 80 (50 + 80 = 130)
    CHECK(target.Stats.MinHp == 130);
    // Bug #38: MinMAN subió en los 80 residuales calculados por SubeHP (200 + 80 = 280), NO en 10!
    CHECK(target.Stats.MinMAN == 280);
}

TEST_CASE("Fase 2: Modificadores de Fuerza y Agilidad con timers canónicos (1200 / 700) y clamping") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    auto& target = UserList[2];

    target = User{};
    target.flags.UserLogged = true;
    target.Stats.UserAtributos[eAtributos::Fuerza] = 18;
    target.Stats.UserAtributosBackUP[eAtributos::Fuerza] = 18;
    target.Stats.UserAtributos[eAtributos::Agilidad] = 18;
    target.Stats.UserAtributosBackUP[eAtributos::Agilidad] = 18;

    caster.flags.TargetUser = 2;

    // 1. Aumento de Fuerza: Timer = 1200, cap en min(MAXATRIBUTOS 40, Backup * 2 = 36)
    auto& h_str_up = Hechizos[7];
    h_str_up = tHechizo{};
    h_str_up.SubeFuerza = 1;
    h_str_up.MinFuerza = 25;
    h_str_up.MaxFuerza = 25;
    caster.flags.Hechizo = 7;

    CHECK(ao::HechizoPropUsuario(1));
    CHECK(target.flags.DuracionEfecto == 1200);
    CHECK(target.flags.TomoPocion);
    // 18 + 25 = 43, pero el cap es min(40, 18 * 2 = 36) => 36
    CHECK(target.Stats.UserAtributos[eAtributos::Fuerza] == 36);

    // 2. Disminución de Fuerza: Timer = 700, suelo en MINATRIBUTOS (6)
    auto& h_str_down = Hechizos[8];
    h_str_down = tHechizo{};
    h_str_down.SubeFuerza = 2;
    h_str_down.MinFuerza = 35;
    h_str_down.MaxFuerza = 35;
    caster.flags.Hechizo = 8;

    CHECK(ao::HechizoPropUsuario(1));
    CHECK(target.flags.DuracionEfecto == 700);
    // 36 - 35 = 1, pero el suelo es MINATRIBUTOS = 6
    CHECK(target.Stats.UserAtributos[eAtributos::Fuerza] == MINATRIBUTOS);

    // 3. Aumento de Agilidad
    auto& h_agi_up = Hechizos[9];
    h_agi_up = tHechizo{};
    h_agi_up.SubeAgilidad = 1;
    h_agi_up.MinAgilidad = 10;
    h_agi_up.MaxAgilidad = 10;
    caster.flags.Hechizo = 9;

    CHECK(ao::HechizoPropUsuario(1));
    CHECK(target.flags.DuracionEfecto == 1200);
    CHECK(target.Stats.UserAtributos[eAtributos::Agilidad] == 28);

    // 4. Disminución de Agilidad
    auto& h_agi_down = Hechizos[10];
    h_agi_down = tHechizo{};
    h_agi_down.SubeAgilidad = 2;
    h_agi_down.MinAgilidad = 30;
    h_agi_down.MaxAgilidad = 30;
    caster.flags.Hechizo = 10;

    CHECK(ao::HechizoPropUsuario(1));
    CHECK(target.flags.DuracionEfecto == 700);
    CHECK(target.Stats.UserAtributos[eAtributos::Agilidad] == MINATRIBUTOS);
}

TEST_CASE("Fase 2: HechizoPropNPC gestiona curación, daño mágico, defensa mágica y reparto de experiencia") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    caster.Stats.ELV = 10;

    if (Npclist.size() < 10) {
        Npclist.fill(npc{});
    }
    auto& monster = Npclist[1];
    monster = npc{};
    monster.Numero = 1;
    monster.Stats.MinHp = 100;
    monster.Stats.MaxHp = 300;
    monster.Stats.defM = 20;

    // 1. Curación sobre criatura (SubeHP = 1)
    auto& h_heal = Hechizos[11];
    h_heal = tHechizo{};
    h_heal.SubeHP = 1;
    h_heal.MinHp = 50;
    h_heal.MaxHp = 50;

    bool cast_heal = false;
    ao::HechizoPropNPC(11, 1, 1, cast_heal);
    CHECK(cast_heal);
    // daño = 50 + Porcentaje(50, 3 * 10 = 30) = 50 + 15 = 65. HP = 100 + 65 = 165
    CHECK(monster.Stats.MinHp == 165);

    // 2. Daño sobre criatura (SubeHP = 2) con mitigación defM y experiencia
    auto& h_dmg = Hechizos[12];
    h_dmg = tHechizo{};
    h_dmg.SubeHP = 2;
    h_dmg.MinHp = 100;
    h_dmg.MaxHp = 100;

    bool exp_calculada = false;
    std::int32_t exp_dmg_dado = 0;
    bool muere_npc_disparado = false;

    ao::SpellsCallbacks cbs{};
    cbs.CalcularDarExp = [&](std::int16_t u_idx, std::int16_t n_idx, std::int32_t dmg) {
        if (u_idx == 1 && n_idx == 1) {
            exp_calculada = true;
            exp_dmg_dado = dmg;
        }
    };
    cbs.MuereNpc = [&](std::int16_t n_idx, std::int16_t u_idx) {
        if (n_idx == 1 && u_idx == 1) muere_npc_disparado = true;
    };
    ao::SetSpellsCallbacks(cbs);

    bool cast_dmg = false;
    monster.Stats.MinHp = 200;
    ao::HechizoPropNPC(12, 1, 1, cast_dmg);
    CHECK(cast_dmg);
    // daño base = 100 + 30% = 130. Menos defM (20) = 110. Vida = 200 - 110 = 90
    CHECK(monster.Stats.MinHp == 90);
    CHECK(exp_calculada);
    CHECK(exp_dmg_dado == 110);
    CHECK_FALSE(muere_npc_disparado);

    // 3. Muerte de la criatura al vaciarse la vida
    monster.Stats.MinHp = 50;
    ao::HechizoPropNPC(12, 1, 1, cast_dmg);
    CHECK(monster.Stats.MinHp == 0);
    CHECK(muere_npc_disparado);
}

} // TEST_SUITE("modHechizos_Fase2")

TEST_SUITE("modHechizos_Fase3") {

TEST_CASE("Fase 3: Inmunidad absoluta de SUPERANILLO contra parálisis, inmovilización y estupidez") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    auto& target = UserList[2];

    target = User{};
    target.flags.UserLogged = true;
    target.char_appearance.CharIndex = 2;
    target.Invent.AnilloEqpObjIndex = ao::SUPERANILLO;

    caster.flags.TargetUser = 2;

    // 1. Hechizo de Parálisis
    auto& h_para = Hechizos[20];
    h_para = tHechizo{};
    h_para.Paraliza = 1;
    caster.flags.Hechizo = 20;

    bool cast_para = false;
    ao::HechizoEstadoUsuario(1, cast_para);
    CHECK(cast_para);
    CHECK(target.flags.Paralizado == 0);
    CHECK(target.Counters.Paralisis == 0);

    // 2. Hechizo de Inmovilización
    auto& h_inmo = Hechizos[21];
    h_inmo = tHechizo{};
    h_inmo.Inmoviliza = 1;
    caster.flags.Hechizo = 21;

    bool cast_inmo = false;
    ao::HechizoEstadoUsuario(1, cast_inmo);
    CHECK(cast_inmo);
    CHECK(target.flags.Inmovilizado == 0);
    CHECK(target.flags.Paralizado == 0);

    // 3. Hechizo de Estupidez
    auto& h_dumb = Hechizos[22];
    h_dumb = tHechizo{};
    h_dumb.Estupidez = 1;
    caster.flags.Hechizo = 22;

    bool cast_dumb = false;
    ao::HechizoEstadoUsuario(1, cast_dumb);
    CHECK_FALSE(cast_dumb);
    CHECK(target.flags.Estupidez == 0);
    CHECK(target.Counters.Ceguera == 0);
}

TEST_CASE("Fase 3: Parálisis regular y remoción en usuario y NPC") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    auto& target = UserList[2];

    target = User{};
    target.flags.UserLogged = true;
    target.char_appearance.CharIndex = 2;
    target.Invent.AnilloEqpObjIndex = 0;

    caster.flags.TargetUser = 2;

    // 1. Parálisis regular en usuario
    auto& h_para = Hechizos[23];
    h_para = tHechizo{};
    h_para.Paraliza = 1;
    caster.flags.Hechizo = 23;

    bool cast_para = false;
    ao::HechizoEstadoUsuario(1, cast_para);
    CHECK(cast_para);
    CHECK(target.flags.Paralizado == 1);
    CHECK(target.Counters.Paralisis == IntervaloParalizado);

    // 2. Remoción de parálisis en usuario
    auto& h_rem_para = Hechizos[24];
    h_rem_para = tHechizo{};
    h_rem_para.RemoverParalisis = 1;
    caster.flags.Hechizo = 24;

    bool cast_rem = false;
    ao::HechizoEstadoUsuario(1, cast_rem);
    CHECK(cast_rem);
    CHECK(target.flags.Paralizado == 0);
    CHECK(target.flags.Inmovilizado == 0);

    // 3. Parálisis en NPC con AfectaParalisis = 0 (vulnerable)
    if (Npclist.size() < 10) Npclist.fill(npc{});
    auto& monster = Npclist[1];
    monster = npc{};
    monster.flags.AfectaParalisis = 0;

    bool npc_cast_para = false;
    ao::HechizoEstadoNPC(1, 23, npc_cast_para, 1);
    CHECK(npc_cast_para);
    CHECK(monster.flags.Paralizado == 1);
    CHECK(monster.Contadores.Paralisis == IntervaloParalizado);

    // 4. Parálisis en NPC con AfectaParalisis = 1 (inmune)
    auto& boss = Npclist[2];
    boss = npc{};
    boss.flags.AfectaParalisis = 1;

    bool boss_cast_para = false;
    ao::HechizoEstadoNPC(2, 23, boss_cast_para, 1);
    CHECK_FALSE(boss_cast_para);
    CHECK(boss.flags.Paralizado == 0);

    // 5. Remoción de parálisis en NPC: amo vs guardia faccionario
    monster.flags.Paralizado = 1;
    monster.MaestroUser = 1; // Caster es su amo
    bool npc_rem_para = false;
    ao::HechizoEstadoNPC(1, 24, npc_rem_para, 1);
    CHECK(npc_rem_para);
    CHECK(monster.flags.Paralizado == 0);
    CHECK(monster.Contadores.Paralisis == 0);

    // Guardia Real requiere ser Armada Real
    auto& guard = Npclist[3];
    guard = npc{};
    guard.NPCtype = eNPCType::GuardiaReal;
    guard.flags.Paralizado = 1;

    bool es_armada_user = false;
    ao::SpellsCallbacks cbs{};
    cbs.EsArmada = [&](std::int16_t u_idx) { return u_idx == 1 && es_armada_user; };
    ao::SetSpellsCallbacks(cbs);

    bool guard_rem = false;
    ao::HechizoEstadoNPC(3, 24, guard_rem, 1);
    CHECK_FALSE(guard_rem);
    CHECK(guard.flags.Paralizado == 1);

    es_armada_user = true;
    ao::HechizoEstadoNPC(3, 24, guard_rem, 1);
    CHECK(guard_rem);
    CHECK(guard.flags.Paralizado == 0);
}

TEST_CASE("Fase 3: REPLICACIÓN BUG #39 — Muerte del lanzador por costo vital de resucitar no aborta la resurrección") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    auto& target = UserList[2];

    target = User{};
    target.flags.UserLogged = true;
    target.flags.Muerto = 1;
    target.char_appearance.CharIndex = 2;
    target.Stats.ELV = 70; // 70 * 0.015 = 1.05 (quita más del 100% de la vida)

    caster = User{};
    caster.flags.UserLogged = true;
    caster.char_appearance.CharIndex = 1;
    caster.clase = eClass::Mage;
    caster.Stats.MinHp = 10; // Vida baja
    caster.Stats.MaxHp = 100;
    caster.Stats.MinSta = 500;
    caster.Stats.MaxSta = 500; // Energía llena obligatoria
    caster.flags.Privilegios = PlayerType::UserPlayer;
    caster.flags.TargetUser = 2;

    auto& staff = ObjDataList[5];
    staff = ObjData{};
    staff.StaffPower = 100;
    caster.Invent.WeaponEqpObjIndex = 5;

    auto& h_resu = Hechizos[25];
    h_resu = tHechizo{};
    h_resu.Revivir = 1;
    h_resu.NeedStaff = 10;
    caster.flags.Hechizo = 25;

    bool caster_murio = false;
    bool target_revivio = false;

    ao::SpellsCallbacks cbs{};
    cbs.UserDie = [&](std::int16_t u_idx) {
        if (u_idx == 1) caster_murio = true;
    };
    cbs.RevivirUsuario = [&](std::int16_t t_idx) {
        if (t_idx == 2) target_revivio = true;
    };
    ao::SetSpellsCallbacks(cbs);

    bool resu_casteado = true;
    ao::HechizoEstadoUsuario(1, resu_casteado);

    // Bug #39: El casteador murió por el costo vital
    CHECK(caster_murio);
    // hechizo_casteado devuelve false...
    CHECK_FALSE(resu_casteado);
    // ...¡PERO revivir_usuario se ejecutó de todos modos debido a la omisión de Exit Sub!
    CHECK(target_revivio);
}

TEST_CASE("Fase 3: REPLICACIÓN BUG #40 — Colisión de contadores entre Estupidez y Ceguera") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    auto& target = UserList[2];

    target = User{};
    target.flags.UserLogged = true;
    target.char_appearance.CharIndex = 2;

    caster.flags.TargetUser = 2;

    auto& h_dumb = Hechizos[26];
    h_dumb = tHechizo{};
    h_dumb.Estupidez = 1;

    auto& h_blind = Hechizos[27];
    h_blind = tHechizo{};
    h_blind.Ceguera = 1;

    // 1. Aplica Estupidez: Bug #40 guarda IntervaloParalizado en Counters.Ceguera
    caster.flags.Hechizo = 26;
    bool dumb_casteado = false;
    ao::HechizoEstadoUsuario(1, dumb_casteado);
    CHECK(dumb_casteado);
    CHECK(target.flags.Estupidez == 1);
    CHECK(target.Counters.Ceguera == IntervaloParalizado); // 500

    // 2. Aplica Ceguera: Sobrescribe Counters.Ceguera con IntervaloParalizado / 3
    caster.flags.Hechizo = 27;
    bool blind_casteado = false;
    ao::HechizoEstadoUsuario(1, blind_casteado);
    CHECK(blind_casteado);
    CHECK(target.flags.Ceguera == 1);
    CHECK(target.Counters.Ceguera == IntervaloParalizado / 3); // 166: se pisó el valor anterior
}

TEST_CASE("Fase 3: Mimetismo con criaturas exclusivo de Druida con Flauta Élfica activando Ignorado") {
    SetupTestEnvironment();
    auto& caster = UserList[1];

    if (Npclist.size() < 10) Npclist.fill(npc{});
    auto& monster = Npclist[1];
    monster = npc{};
    monster.char_appearance.body = 55;
    monster.char_appearance.Head = 22;

    auto& h_mimi = Hechizos[28];
    h_mimi = tHechizo{};
    h_mimi.Mimetiza = 1;

    // 1. Clase Mago: rechaza mimetizarse con criaturas
    caster.clase = eClass::Mage;
    bool cast_mimi = false;
    ao::HechizoEstadoNPC(1, 28, cast_mimi, 1);
    CHECK_FALSE(cast_mimi);
    CHECK(caster.flags.Mimetizado == 0);

    // 2. Clase Druida sin flauta: mimetiza pero flags.Ignorado permanece false
    caster.clase = eClass::Druid;
    caster.char_appearance.body = 1;
    caster.char_appearance.Head = 1;
    caster.char_appearance.CascoAnim = 10;
    caster.char_appearance.ShieldAnim = 11;
    caster.char_appearance.WeaponAnim = 12;
    caster.Invent.AnilloEqpObjIndex = 0;

    cast_mimi = false;
    ao::HechizoEstadoNPC(1, 28, cast_mimi, 1);
    CHECK(cast_mimi);
    CHECK(caster.flags.Mimetizado == 1);
    CHECK(caster.CharMimetizado.body == 1);
    CHECK(caster.CharMimetizado.Head == 1);
    CHECK(caster.char_appearance.body == 55);
    CHECK(caster.char_appearance.Head == 22);
    CHECK(caster.char_appearance.CascoAnim == NingunCasco);
    CHECK(caster.char_appearance.ShieldAnim == NingunEscudo);
    CHECK(caster.char_appearance.WeaponAnim == NingunArma);
    CHECK_FALSE(caster.flags.Ignorado);

    // 3. Clase Druida con Flauta Élfica: activa flags.Ignorado
    caster.flags.Mimetizado = 0;
    caster.Invent.AnilloEqpObjIndex = ao::FLAUTAELFICA;

    cast_mimi = false;
    ao::HechizoEstadoNPC(1, 28, cast_mimi, 1);
    CHECK(cast_mimi);
    CHECK(caster.flags.Mimetizado == 1);
    CHECK(caster.flags.Ignorado);
}

TEST_CASE("Fase 3: HechizoInvocacion bloqueado en zonas seguras y acotado por MAXMASCOTAS") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    caster.Pos.Map = 1;
    caster.Pos.X = 50;
    caster.Pos.Y = 50;
    caster.flags.TargetMap = 1;
    caster.flags.TargetX = 51;
    caster.flags.TargetY = 51;

    auto& h_invo = Hechizos[29];
    h_invo = tHechizo{};
    h_invo.cant = 5; // Pide 5, pero MAXMASCOTAS es 3
    h_invo.NumNpc = 10;
    caster.flags.Hechizo = 29;

    std::int16_t next_spawn_id = 1;
    ao::SpellsCallbacks cbs{};
    cbs.SpawnNpc = [&](std::int16_t num, const WorldPos&, bool, bool) {
        if (next_spawn_id < 10) {
            Npclist[next_spawn_id].Numero = num;
            return next_spawn_id++;
        }
        return static_cast<std::int16_t>(0);
    };
    ao::SetSpellsCallbacks(cbs);

    // 1. Bloqueo en mapa no-PK (zona segura)
    MapInfoList[1].Pk = false;
    bool invo_casteada = false;
    ao::HechizoInvocacion(1, invo_casteada);
    CHECK_FALSE(invo_casteada);
    CHECK(caster.NroMascotas == 0);

    // 2. Invocación en zona insegura: invoca solo hasta MAXMASCOTAS (3)
    MapInfoList[1].Pk = true;
    invo_casteada = false;
    ao::HechizoInvocacion(1, invo_casteada);
    CHECK(invo_casteada);
    CHECK(caster.NroMascotas == MAXMASCOTAS);
    CHECK(caster.MascotasIndex[1] == 1);
    CHECK(caster.MascotasIndex[2] == 2);
    CHECK(caster.MascotasIndex[3] == 3);

    // 3. Intento adicional con libro de mascotas lleno es ignorado
    invo_casteada = false;
    ao::HechizoInvocacion(1, invo_casteada);
    CHECK_FALSE(invo_casteada);
    CHECK(caster.NroMascotas == MAXMASCOTAS);

    // 4. Bifurcación Warp de mascota
    auto& h_warp = Hechizos[30];
    h_warp = tHechizo{};
    h_warp.Warp = 1;
    caster.flags.Hechizo = 30;

    bool warp_ejecutado = false;
    cbs.FarthestPet = [&](std::int16_t u_idx) { return u_idx == 1 ? 3 : 0; };
    cbs.WarpMascota = [&](std::int16_t u_idx, std::int16_t pet_idx) {
        if (u_idx == 1 && pet_idx == 3) warp_ejecutado = true;
    };
    ao::SetSpellsCallbacks(cbs);

    bool warp_casteado = false;
    ao::HechizoInvocacion(1, warp_casteado);
    CHECK(warp_casteado);
    CHECK(warp_ejecutado);
}

TEST_CASE("Fase 3: HechizoTerrenoEstado remueve invisibilidad en área de 17x17 celdas") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    caster.flags.TargetMap = 1;
    caster.flags.TargetX = 50;
    caster.flags.TargetY = 50;

    auto& target = UserList[2];
    target = User{};
    target.flags.UserLogged = true;
    target.flags.invisible = 1;
    target.char_appearance.CharIndex = 2;

    // Ubicamos al target en (55, 55), dentro de 50 +- 8
    const std::size_t idx = (static_cast<std::size_t>(1) * 101 + static_cast<std::size_t>(55)) * 101 + static_cast<std::size_t>(55);
    MapData[idx].UserIndex = 2;

    auto& h_terr = Hechizos[31];
    h_terr = tHechizo{};
    h_terr.RemueveInvisibilidadParcial = 1;
    h_terr.FXgrh = 100;
    h_terr.loops = 1;
    caster.flags.Hechizo = 31;

    bool b = false;
    ao::HechizoTerrenoEstado(1, b);
    CHECK(b);

    // Admin invisible no es revelado visualmente
    target.flags.AdminInvisible = 1;
    b = false;
    ao::HechizoTerrenoEstado(1, b);
    CHECK(b);
}

} // TEST_SUITE("modHechizos_Fase3")

TEST_SUITE("modHechizos_Fase4") {

TEST_CASE("Fase 4: CanSupportUser - Matriz Completa de Reglas Morales y Legales") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    auto& target = UserList[2];
    target = User{};
    target.flags.UserLogged = true;
    target.char_appearance.CharIndex = 2;
    target.Pos = caster.Pos;
    target.Stats.MinHp = 50;
    target.Stats.MaxHp = 100;
    target.Reputacion.NobleRep = 1000;

    caster.Reputacion.NobleRep = 1000;
    caster.flags.Seguro = true;

    // 1. Auto-soporte siempre permitido
    CHECK(ao::CanSupportUser(1, 1, false));

    // 2. En consulta bloqueado
    caster.flags.EnConsulta = true;
    CHECK_FALSE(ao::CanSupportUser(1, 2, false));
    caster.flags.EnConsulta = false;

    // 3. En arena (Trigger 6) siempre permitido
    ao::SpellsCallbacks cbs{};
    cbs.TriggerZonaPelea = [](std::int16_t, std::int16_t) -> std::int16_t {
        return static_cast<std::int16_t>(eTrigger6::TRIGGER6_PERMITE);
    };
    ao::SetSpellsCallbacks(cbs);
    CHECK(ao::CanSupportUser(1, 2, false));

    // Desactivamos trigger de arena
    cbs.TriggerZonaPelea = nullptr;
    ao::SetSpellsCallbacks(cbs);

    // 4. Target Criminal
    cbs.Criminal = [](std::int16_t u_idx) {
        return u_idx == 2; // Target es criminal
    };
    ao::SetSpellsCallbacks(cbs);

    // 4.1 Caster Armada Real no puede ayudar a criminales
    cbs.EsArmada = [](std::int16_t u_idx) {
        return u_idx == 1;
    };
    ao::SetSpellsCallbacks(cbs);
    CHECK_FALSE(ao::CanSupportUser(1, 2, false));

    // 4.2 Caster Ciudadano con Seguro puesto no puede ayudar a criminales
    cbs.EsArmada = nullptr;
    ao::SetSpellsCallbacks(cbs);
    caster.flags.Seguro = true;
    CHECK_FALSE(ao::CanSupportUser(1, 2, false));

    // 4.3 Caster Ciudadano sin Seguro con DoCriminal = true -> se vuelve criminal
    caster.flags.Seguro = false;
    bool volvio_criminal = false;
    cbs.VolverCriminal = [&](std::int16_t u_idx) {
        if (u_idx == 1) volvio_criminal = true;
    };
    ao::SetSpellsCallbacks(cbs);
    CHECK(ao::CanSupportUser(1, 2, true));
    CHECK(volvio_criminal);

    // 4.4 Caster Ciudadano sin Seguro con DoCriminal = false -> pierde nobleza y gana bandido
    caster.Reputacion.NobleRep = 2000;
    caster.Reputacion.BandidoRep = 0;
    caster.flags.Privilegios = PlayerType::UserPlayer;
    CHECK(ao::CanSupportUser(1, 2, false));
    CHECK(caster.Reputacion.NobleRep == 1000); // 2000 - 50% = 1000
    CHECK(caster.Reputacion.BandidoRep == 10000);

    // 5. Target Ciudadano
    cbs.Criminal = [](std::int16_t) { return false; }; // Ambos ciudadanos
    ao::SetSpellsCallbacks(cbs);

    // 5.1 Caster Legión del Caos no puede ayudar a ciudadanos
    cbs.EsCaos = [](std::int16_t u_idx) { return u_idx == 1; };
    ao::SetSpellsCallbacks(cbs);
    CHECK_FALSE(ao::CanSupportUser(1, 2, false));
    cbs.EsCaos = nullptr;
    ao::SetSpellsCallbacks(cbs);

    // 5.2 Target en estado atacable por un tercero (AtacablePor = 3)
    target.flags.AtacablePor = 3;

    // Caster Armada no puede ayudar a atacables
    cbs.EsArmada = [](std::int16_t u_idx) { return u_idx == 1; };
    ao::SetSpellsCallbacks(cbs);
    CHECK_FALSE(ao::CanSupportUser(1, 2, false));
    cbs.EsArmada = nullptr;
    ao::SetSpellsCallbacks(cbs);

    // Caster con seguro puesto no puede ayudar
    caster.flags.Seguro = true;
    CHECK_FALSE(ao::CanSupportUser(1, 2, false));

    // Caster sin seguro puede ayudar pero pierde nobleza
    caster.flags.Seguro = false;
    caster.Reputacion.NobleRep = 1000;
    caster.Reputacion.BandidoRep = 0;
    CHECK(ao::CanSupportUser(1, 2, false));
    CHECK(caster.Reputacion.NobleRep == 500);
    CHECK(caster.Reputacion.BandidoRep == 10000);

    // 5.3 Target en estado atacable por el propio caster (AtacablePor == CasterIndex) -> permitido sin penalización
    target.flags.AtacablePor = 1;
    caster.Reputacion.NobleRep = 1000;
    caster.Reputacion.BandidoRep = 0;
    CHECK(ao::CanSupportUser(1, 2, false));
    CHECK(caster.Reputacion.NobleRep == 1000);
    CHECK(caster.Reputacion.BandidoRep == 0);
}

TEST_CASE("Fase 4: DisNobAuBan - Pérdida de Nobleza, Ganancia de Bandido y Expulsión Real") {
    SetupTestEnvironment();
    auto& user = UserList[1];
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.Reputacion.NobleRep = 8000;
    user.Reputacion.BandidoRep = 500;
    user.Faccion.ArmadaReal = 1;

    bool expulsado_real = false;
    bool refresh_status = false;
    ao::SpellsCallbacks cbs{};
    cbs.ExpulsarFaccionReal = [&](std::int16_t u_idx) {
        if (u_idx == 1) expulsado_real = true;
    };
    cbs.RefreshCharStatus = [&](std::int16_t u_idx) {
        if (u_idx == 1) refresh_status = true;
    };
    // Inicialmente no criminal; si nobleza cae a 0, pasa a ser criminal
    cbs.Criminal = [](std::int16_t u_idx) {
        return UserList[u_idx].Reputacion.NobleRep == 0;
    };
    ao::SetSpellsCallbacks(cbs);

    // 1. En zona de pelea / arena -> no se aplica penalización
    const std::size_t arena_idx = (static_cast<std::size_t>(user.Pos.Map) * 101 + static_cast<std::size_t>(user.Pos.X)) * 101 + static_cast<std::size_t>(user.Pos.Y);
    MapData[arena_idx].trigger = eTrigger::ZONAPELEA;
    ao::DisNobAuBan(1, 4000, 10000);
    CHECK(user.Reputacion.NobleRep == 8000);
    CHECK(user.Reputacion.BandidoRep == 500);

    // 2. Fuera de arena: pérdida parcial
    MapData[arena_idx].trigger = eTrigger::NADA;
    ao::DisNobAuBan(1, 3000, 5000);
    CHECK(user.Reputacion.NobleRep == 5000);
    CHECK(user.Reputacion.BandidoRep == 5500);
    CHECK_FALSE(expulsado_real);
    CHECK_FALSE(refresh_status);

    // 3. Pérdida total que lleva la nobleza a 0 -> cae en criminalidad y expulsa de armada
    ao::DisNobAuBan(1, 10000, 20000);
    CHECK(user.Reputacion.NobleRep == 0);
    CHECK(user.Reputacion.BandidoRep == 25500);
    CHECK(expulsado_real);
    CHECK(refresh_status);
}

TEST_CASE("Fase 4: LanzarHechizo - Quirk 7.3 (Validación Exclusiva en Eje Y) y Ruteo de Targets") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    caster.Pos.Map = 1;
    caster.Pos.X = 50;
    caster.Pos.Y = 50;
    caster.Stats.MinMAN = 500;
    caster.Stats.MinSta = 200;
    caster.Counters.Trabajando = 5;
    caster.Counters.Ocultando = 3;

    auto& target_npc = Npclist[1];
    target_npc = npc{};
    target_npc.Pos.Map = 1;
    target_npc.Pos.X = 50;
    target_npc.Pos.Y = 50;
    target_npc.Stats.MinHp = 100;
    target_npc.Stats.MaxHp = 100;

    auto& target_u = UserList[2];
    target_u = User{};
    target_u.flags.UserLogged = true;
    target_u.char_appearance.CharIndex = 2;
    target_u.Pos.Map = 1;
    target_u.Pos.X = 50;
    target_u.Pos.Y = 50;
    target_u.Stats.MinHp = 50;
    target_u.Stats.MaxHp = 100;

    // Hechizo de prueba
    auto& h = Hechizos[40];
    h = tHechizo{};
    h.Nombre = "Curar";
    h.Target = TargetType::uUsuarios;
    h.Tipo = TipoHechizo::uPropiedades;
    h.SubeHP = 1;
    h.MinHp = 20;
    h.MaxHp = 20;
    h.ManaRequerido = 50;
    h.StaRequerido = 10;
    h.MinSkill = 10;

    // 1. En consulta aborta de inmediato
    caster.flags.EnConsulta = true;
    ao::LanzarHechizo(40, 1);
    CHECK(caster.Stats.MinMAN == 500); // No consumió nada
    caster.flags.EnConsulta = false;

    // 2. Quirk 7.3: Rango de visión vertical (RANGO_VISION_Y = 6)
    // Caso: Delta X grande (ej. 25 tiles), pero Delta Y = 4 (<= 6)
    caster.flags.TargetUser = 2;
    target_u.Pos.X = 75;
    target_u.Pos.Y = 54; // |54 - 50| = 4 <= 6
    ao::LanzarHechizo(40, 1);
    CHECK(caster.Stats.MinMAN == 450); // Casteado exitosamente
    CHECK(caster.Counters.Trabajando == 4);
    CHECK(caster.Counters.Ocultando == 2);

    // Caso: Delta Y = 7 (> 6) -> falla por distancia
    caster.Stats.MinMAN = 500;
    caster.flags.TargetUser = 2;
    target_u.Pos.X = 50;
    target_u.Pos.Y = 57; // |57 - 50| = 7 > 6
    ao::LanzarHechizo(40, 1);
    CHECK(caster.Stats.MinMAN == 500); // No se lanzó

    // 3. Ruteo a NPC con Quirk 7.3
    auto& h_npc = Hechizos[41];
    h_npc = tHechizo{};
    h_npc.Nombre = "Ataque Criatura";
    h_npc.Target = TargetType::uNPC;
    h_npc.Tipo = TipoHechizo::uPropiedades;
    h_npc.SubeHP = 2;
    h_npc.MinHp = 15;
    h_npc.MaxHp = 15;
    h_npc.ManaRequerido = 30;
    h_npc.StaRequerido = 5;

    caster.flags.TargetNPC = 1;
    target_npc.Pos.X = 80;
    target_npc.Pos.Y = 55; // Delta Y = 5 <= 6
    ao::LanzarHechizo(41, 1);
    CHECK(caster.Stats.MinMAN == 470);

    // TargetNPC lejano en Y (Delta Y = 8)
    caster.flags.TargetNPC = 1;
    target_npc.Pos.Y = 58; // Delta Y = 8 > 6
    ao::LanzarHechizo(41, 1);
    CHECK(caster.Stats.MinMAN == 470); // No consumió
}

TEST_CASE("Fase 4: HandleHechizo* - Consumo de Recursos, Descuentos de Druida y Progresión") {
    SetupTestEnvironment();
    auto& caster = UserList[1];
    caster.clase = eClass::Druid;
    caster.Invent.AnilloEqpObjIndex = FLAUTAELFICA;
    caster.Stats.MinMAN = 1000;
    caster.Stats.MaxMAN = 1000;
    caster.Stats.MinSta = 500;
    caster.Stats.MaxSta = 500;

    bool skill_subido = false;
    ao::SpellsCallbacks cbs{};
    cbs.SubirSkill = [&](std::int16_t u_idx, eSkill skill, bool sube) {
        if (u_idx == 1 && skill == eSkill::Magia && sube) {
            skill_subido = true;
        }
    };
    ao::SetSpellsCallbacks(cbs);

    // 1. Terreno con Invocación Warp -> consume toda la maná
    auto& h_warp = Hechizos[42];
    h_warp = tHechizo{};
    h_warp.Tipo = TipoHechizo::uInvocacion;
    h_warp.Warp = 1;
    h_warp.ManaRequerido = 100;
    h_warp.StaRequerido = 50;

    cbs.FarthestPet = [](std::int16_t) { return 1; };
    cbs.WarpMascota = [](std::int16_t, std::int16_t) {};
    ao::SetSpellsCallbacks(cbs);

    ao::HandleHechizoTerreno(1, 42);
    CHECK(caster.Stats.MinMAN == 0); // Toda la maná consumida
    CHECK(caster.Stats.MinSta == 450);
    CHECK(skill_subido);

    // 2. Terreno con Invocación regular para Druida con Flauta -> 30% descuento (paga 70%)
    skill_subido = false;
    caster.Stats.MinMAN = 1000;
    auto& h_invo = Hechizos[43];
    h_invo = tHechizo{};
    h_invo.Tipo = TipoHechizo::uInvocacion;
    h_invo.Warp = 0;
    h_invo.cant = 1;
    h_invo.NumNpc = 5;
    h_invo.ManaRequerido = 200;
    h_invo.StaRequerido = 20;

    cbs.SpawnNpc = [](std::int16_t, const WorldPos&, bool, bool) -> std::int16_t { return 2; };
    cbs.FreeMascotaIndex = [](std::int16_t) -> std::int16_t { return 1; };
    ao::SetSpellsCallbacks(cbs);

    ao::HandleHechizoTerreno(1, 43);
    // 200 * 0.7 = 140 consumidos -> 1000 - 140 = 860
    CHECK(caster.Stats.MinMAN == 860);
    CHECK(caster.Stats.MinSta == 430);

    // 3. NPC con Mimetismo para Druida con Flauta -> 50% descuento y flags.Ignorado = true
    auto& npc_mimi = Npclist[1];
    npc_mimi = npc{};
    npc_mimi.Pos = caster.Pos;
    caster.flags.TargetNPC = 1;

    auto& h_mimi = Hechizos[44];
    h_mimi = tHechizo{};
    h_mimi.Tipo = TipoHechizo::uEstado;
    h_mimi.Mimetiza = 1;
    h_mimi.ManaRequerido = 300;
    h_mimi.StaRequerido = 10;

    caster.flags.Ignorado = false;
    ao::HandleHechizoNPC(1, 44);
    // 300 * 0.5 = 150 consumidos -> 860 - 150 = 710
    CHECK(caster.Stats.MinMAN == 710);
    CHECK(caster.flags.Ignorado);
    CHECK(caster.flags.TargetNPC == 0); // Reseteado

    // 4. Hechizo Apocalipsis para Druida con Flauta -> NO tiene 10% de descuento
    auto& target_u = UserList[2];
    target_u = User{};
    target_u.flags.UserLogged = true;
    target_u.char_appearance.CharIndex = 2;
    target_u.Pos = caster.Pos;
    target_u.Stats.MinHp = 100;
    target_u.Stats.MaxHp = 100;

    auto& h_apoca = Hechizos[APOCALIPSIS_SPELL_INDEX];
    h_apoca = tHechizo{};
    h_apoca.Tipo = TipoHechizo::uPropiedades;
    h_apoca.SubeHP = 2;
    h_apoca.MinHp = 10;
    h_apoca.MaxHp = 10;
    h_apoca.ManaRequerido = 400;
    h_apoca.StaRequerido = 10;

    caster.flags.TargetUser = 2;
    ao::HandleHechizoUsuario(1, APOCALIPSIS_SPELL_INDEX);
    // Paga 400 completos sin el 10% de descuento -> 710 - 400 = 310
    CHECK(caster.Stats.MinMAN == 310);
    CHECK(caster.flags.TargetUser == 0); // Reseteado
}

TEST_CASE("Fase 4: NpcLanzaSpellSobreUser - Quirk 7.1 (Mensaje de Daño en Curación), Quirk 7.2 (Estupidez) y Mitigaciones") {
    SetupTestEnvironment();
    auto& npc_hostil = Npclist[1];
    npc_hostil = npc{};
    npc_hostil.name = "Sacerdote Maligno";
    npc_hostil.CanAttack = 1;
    npc_hostil.NPCtype = eNPCType::Comun;

    auto& user = UserList[1];
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.Stats.MinHp = 50;
    user.Stats.MaxHp = 100;
    user.Invent.CascoEqpObjIndex = 0;
    user.Invent.AnilloEqpObjIndex = 0;

    IntervaloInvisible = 350;
    IntervaloParalizado = 500;

    // 1. Quirk 7.1: SubeHP == 1 cura al usuario y consume CanAttack
    auto& h_cura = Hechizos[45];
    h_cura = tHechizo{};
    h_cura.SubeHP = 1;
    h_cura.MinHp = 30;
    h_cura.MaxHp = 30;

    ao::NpcLanzaSpellSobreUser(1, 1, 45);
    CHECK(npc_hostil.CanAttack == 0);
    CHECK(user.Stats.MinHp == 80); // Curó 30 puntos

    // 2. CanAttack == 0 bloquea siguientes lanzamientos
    ao::NpcLanzaSpellSobreUser(1, 1, 45);
    CHECK(user.Stats.MinHp == 80); // No cambió

    // 3. Mitigación de Casco y Anillo antimagia en daño
    npc_hostil.CanAttack = 1;
    auto& h_dmg = Hechizos[46];
    h_dmg = tHechizo{};
    h_dmg.SubeHP = 2;
    h_dmg.MinHp = 50;
    h_dmg.MaxHp = 50;

    auto& casco = ObjDataList[10];
    casco = ObjData{};
    casco.DefensaMagicaMin = 10;
    casco.DefensaMagicaMax = 10;
    user.Invent.CascoEqpObjIndex = 10;

    auto& anillo = ObjDataList[11];
    anillo = ObjData{};
    anillo.DefensaMagicaMin = 15;
    anillo.DefensaMagicaMax = 15;
    user.Invent.AnilloEqpObjIndex = 11;

    // Daño = 50 - 10 (casco) - 15 (anillo) = 25 -> 80 - 25 = 55
    ao::NpcLanzaSpellSobreUser(1, 1, 46);
    CHECK(user.Stats.MinHp == 55);

    // 4. Muerte y llamada a UserDie con MaestroUser
    npc_hostil.CanAttack = 1;
    npc_hostil.MaestroUser = 3;
    user.Invent.CascoEqpObjIndex = 0;
    user.Invent.AnilloEqpObjIndex = 0;

    bool user_murio = false;
    std::int16_t frag_killer = 0;
    ao::SpellsCallbacks cbs{};
    cbs.UserDie = [&](std::int16_t u_idx) {
        if (u_idx == 1) user_murio = true;
    };
    cbs.StoreFrag = [&](std::int16_t killer, std::int16_t) {
        frag_killer = killer;
    };
    ao::SetSpellsCallbacks(cbs);

    h_dmg.MinHp = 100;
    h_dmg.MaxHp = 100;
    ao::NpcLanzaSpellSobreUser(1, 1, 46);
    CHECK(user.Stats.MinHp == 0);
    CHECK(user_murio);
    CHECK(frag_killer == 3);

    // 5. Inmunidad de SUPERANILLO contra Parálisis y Estupidez
    user.Stats.MinHp = 100;
    user.flags.Paralizado = 0;
    user.flags.Estupidez = 0;
    user.Invent.AnilloEqpObjIndex = ao::SUPERANILLO;

    npc_hostil.CanAttack = 1;
    auto& h_para = Hechizos[47];
    h_para = tHechizo{};
    h_para.Paraliza = 1;

    ao::NpcLanzaSpellSobreUser(1, 1, 47);
    CHECK(user.flags.Paralizado == 0); // Rechazado por el anillo

    npc_hostil.CanAttack = 1;
    auto& h_estup = Hechizos[48];
    h_estup = tHechizo{};
    h_estup.Estupidez = 1;

    ao::NpcLanzaSpellSobreUser(1, 1, 48);
    CHECK(user.flags.Estupidez == 0); // Rechazado por el anillo

    // 6. Quirk 7.2: Estupidez asigna Counters.Ceguera = IntervaloInvisible
    user.Invent.AnilloEqpObjIndex = 0;
    npc_hostil.CanAttack = 1;
    ao::NpcLanzaSpellSobreUser(1, 1, 48);
    CHECK(user.flags.Estupidez == 1);
    CHECK(user.Counters.Ceguera == 350); // IntervaloInvisible asignado en Ceguera
}

TEST_CASE("Fase 4: NpcLanzaSpellSobreNpc - Combate Mágico entre Criaturas y Muerte") {
    SetupTestEnvironment();
    auto& atacante = Npclist[1];
    atacante = npc{};
    atacante.CanAttack = 1;
    atacante.MaestroUser = 2; // Amo del atacante

    auto& victima = Npclist[2];
    victima = npc{};
    victima.Stats.MinHp = 50;
    victima.Stats.MaxHp = 100;

    auto& h_ofensivo = Hechizos[49];
    h_ofensivo = tHechizo{};
    h_ofensivo.SubeHP = 2;
    h_ofensivo.MinHp = 20;
    h_ofensivo.MaxHp = 20;

    // 1. Daño regular entre criaturas
    ao::NpcLanzaSpellSobreNpc(1, 2, 49);
    CHECK(atacante.CanAttack == 0);
    CHECK(victima.Stats.MinHp == 30);

    // 2. Muerte de la criatura -> invoca MuereNpc con el amo del atacante
    atacante.CanAttack = 1;
    h_ofensivo.MinHp = 40;
    h_ofensivo.MaxHp = 40;

    std::int16_t muerto_idx = 0;
    std::int16_t killer_idx = 0;
    ao::SpellsCallbacks cbs{};
    cbs.MuereNpc = [&](std::int16_t n_idx, std::int16_t k_idx) {
        muerto_idx = n_idx;
        killer_idx = k_idx;
    };
    ao::SetSpellsCallbacks(cbs);

    ao::NpcLanzaSpellSobreNpc(1, 2, 49);
    CHECK(victima.Stats.MinHp == 0);
    CHECK(muerto_idx == 2);
    CHECK(killer_idx == 2); // MaestroUser del atacante
}

} // TEST_SUITE("modHechizos_Fase4")
