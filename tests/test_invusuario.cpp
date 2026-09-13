#include <doctest/doctest.h>
#include "server/InvUsuario.hpp"
#include "server/Modulo_InventANDobj.hpp"
#include "server/Declares.hpp"
#include "server/TCP.hpp"
#include <vector>
#include <string>

namespace {

inline std::size_t block_index(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return (static_cast<std::size_t>(map) * 101 + static_cast<std::size_t>(x)) * 101 + static_cast<std::size_t>(y);
}

struct TestEnv {
    std::vector<std::pair<std::int16_t, std::string>> sent_packets;

    TestEnv() {
        NumMaps = 2;
        LastUser = 5;
        UserList.clear();
        for (int i = 0; i < 10; ++i) {
            UserList.emplace_back();
        }
        if (MapData.size() < 3 * 101 * 101) {
            MapData.assign(3 * 101 * 101, MapBlock{});
        }
        ObjDataList.assign(1001, ObjData{});
        NumObjDatas = 1000;
        if (ConnGroups.size() < 3) {
            ConnGroups.assign(3, ConnGroup{});
            for (auto& cg : ConnGroups) {
                cg.UserEntrys.assign(10, 0);
            }
        }

        // Registrar hook para capturar paquetes de red
        TCP::SetSendDataHook([this](std::int16_t slot, std::string_view data) {
            sent_packets.emplace_back(slot, std::string(data));
        });

        // Configurar ítems de prueba
        ObjDataList[1].name = "Manzana";
        ObjDataList[1].OBJType = eOBJType::otUseOnce;
        ObjDataList[1].GrhIndex = 100;
        ObjDataList[1].Agarrable = 0; // Agarrable
        ObjDataList[1].MinHam = 30;

        ObjDataList[2].name = "Espada";
        ObjDataList[2].OBJType = eOBJType::otWeapon;
        ObjDataList[2].GrhIndex = 101;
        ObjDataList[2].Agarrable = 0;

        ObjDataList[3].name = "Item No Caíble";
        ObjDataList[3].OBJType = eOBJType::otUseOnce;
        ObjDataList[3].GrhIndex = 102;
        ObjDataList[3].NoSeCae = 1;

        ObjDataList[4].name = "Item Newbie";
        ObjDataList[4].OBJType = eOBJType::otUseOnce;
        ObjDataList[4].GrhIndex = 103;
        ObjDataList[4].Newbie = 1;

        ObjDataList[5].name = "Oro";
        ObjDataList[5].OBJType = eOBJType::otGuita;
        ObjDataList[5].GrhIndex = 104;
        ObjDataList[5].Agarrable = 0;

        ObjDataList[6].name = "Estatua No Agarrable";
        ObjDataList[6].OBJType = eOBJType::otUseOnce;
        ObjDataList[6].GrhIndex = 105;
        ObjDataList[6].Agarrable = 1; // 1 = No agarrable en VB6

        ObjDataList[7].name = "Llave";
        ObjDataList[7].OBJType = eOBJType::otLlaves;
        ObjDataList[7].GrhIndex = 106;

        ObjDataList[8].name = "Barco";
        ObjDataList[8].OBJType = eOBJType::otBarcos;
        ObjDataList[8].GrhIndex = 107;

        ObjDataList[9].name = "Armadura Real";
        ObjDataList[9].OBJType = eOBJType::otArmadura;
        ObjDataList[9].GrhIndex = 108;
        ObjDataList[9].Real = 1;
        ObjDataList[9].Ropaje = 50;

        ObjDataList[10].name = "Armadura Caos";
        ObjDataList[10].OBJType = eOBJType::otArmadura;
        ObjDataList[10].GrhIndex = 109;
        ObjDataList[10].Caos = 1;
        ObjDataList[10].Ropaje = 51;

        // Ítem 11: Escudo
        ObjDataList[11].name = "Escudo de Hierro";
        ObjDataList[11].OBJType = eOBJType::otESCUDO;
        ObjDataList[11].ShieldAnim = 3;

        // Ítem 12: Casco
        ObjDataList[12].name = "Casco de Hierro";
        ObjDataList[12].OBJType = eOBJType::otCASCO;
        ObjDataList[12].CascoAnim = 4;

        // Ítem 13: Espada Dos Manos
        ObjDataList[13].name = "Gran Espada Dos Manos";
        ObjDataList[13].OBJType = eOBJType::otWeapon;
        ObjDataList[13].WeaponAnim = 5;
        ObjDataList[13].DosManos = 1;

        // Ítem 14: Flechas
        ObjDataList[14].name = "Flechas Cazadoras";
        ObjDataList[14].OBJType = eOBJType::otFlechas;

        // Ítem 15: Anillo
        ObjDataList[15].name = "Anillo Mágico";
        ObjDataList[15].OBJType = eOBJType::otAnillo;

        // Ítem 16: Mochila (MochilaType = 2 -> 20 + 2*5 = 30 slots)
        ObjDataList[16].name = "Mochila Aventurera";
        ObjDataList[16].OBJType = eOBJType::otMochilas;
        ObjDataList[16].MochilaType = 2;

        // Ítem 17: Ropaje Femenino Humano
        ObjDataList[17].name = "Vestido Femenino";
        ObjDataList[17].OBJType = eOBJType::otArmadura;
        ObjDataList[17].Mujer = 1;
        ObjDataList[17].Hombre = 0;
        ObjDataList[17].Ropaje = 52;

        // Ítem 18: Túnica Exclusiva Drow
        ObjDataList[18].name = "Túnica Drow";
        ObjDataList[18].OBJType = eOBJType::otArmadura;
        ObjDataList[18].RazaDrow = 1;
        ObjDataList[18].Ropaje = 53;

        // Ítem 19: Ropaje Enano
        ObjDataList[19].name = "Armadura Enana";
        ObjDataList[19].OBJType = eOBJType::otArmadura;
        ObjDataList[19].RazaEnana = 1;
        ObjDataList[19].Ropaje = 54;

        // Ítem 20: Arma con Requisito de Atributos
        ObjDataList[20].name = "Martillo Pesado";
        ObjDataList[20].OBJType = eOBJType::otWeapon;
        ObjDataList[20].MinFuerza = 18;
        ObjDataList[20].MinAgilidad = 15;
        ObjDataList[20].WeaponAnim = 6;

        // Ítems de G4: Consumibles, Pociones, Pergaminos y Herramientas
        // Ítem 21: Bebida
        ObjDataList[21].name = "Botella de Agua";
        ObjDataList[21].OBJType = eOBJType::otBebidas;
        ObjDataList[21].MinSed = 30;

        // Ítem 22: Poción de Vida
        ObjDataList[22].name = "Poción Roja";
        ObjDataList[22].OBJType = eOBJType::otPociones;
        ObjDataList[22].TipoPocion = 3;
        ObjDataList[22].MinModificador = 40;
        ObjDataList[22].MaxModificador = 40;
        ObjDataList[22].DuracionEfecto = 0;

        // Ítem 23: Poción de Maná
        ObjDataList[23].name = "Poción Azul";
        ObjDataList[23].OBJType = eOBJType::otPociones;
        ObjDataList[23].TipoPocion = 4;
        ObjDataList[23].MinModificador = 25;
        ObjDataList[23].MaxModificador = 25;
        ObjDataList[23].DuracionEfecto = 0;

        // Ítem 24: Poción de Agilidad
        ObjDataList[24].name = "Poción Amarilla";
        ObjDataList[24].OBJType = eOBJType::otPociones;
        ObjDataList[24].TipoPocion = 1;
        ObjDataList[24].MinModificador = 5;
        ObjDataList[24].DuracionEfecto = 500;

        // Ítem 25: Poción de Fuerza
        ObjDataList[25].name = "Poción Verde";
        ObjDataList[25].OBJType = eOBJType::otPociones;
        ObjDataList[25].TipoPocion = 2;
        ObjDataList[25].MinModificador = 4;
        ObjDataList[25].DuracionEfecto = 500;

        // Ítem 26: Poción Antídoto
        ObjDataList[26].name = "Poción Violeta";
        ObjDataList[26].OBJType = eOBJType::otPociones;
        ObjDataList[26].TipoPocion = 5;

        // Ítem 27: Pergamino Mágico
        ObjDataList[27].name = "Pergamino de Destello";
        ObjDataList[27].OBJType = eOBJType::otPergaminos;
        ObjDataList[27].HechizoIndex = 7;

        // Herramientas de trabajo en sus índices legacy
        ObjDataList[CAÑA_PESCA].name = "Caña de Pescar";
        ObjDataList[CAÑA_PESCA].OBJType = eOBJType::otWeapon;

        ObjDataList[HACHA_LEÑADOR].name = "Hacha de Leñador";
        ObjDataList[HACHA_LEÑADOR].OBJType = eOBJType::otWeapon;

        ObjDataList[PIQUETE_MINERO].name = "Piquete de Minero";
        ObjDataList[PIQUETE_MINERO].OBJType = eOBJType::otWeapon;

        ObjDataList[MARTILLO_HERRERO].name = "Martillo de Herrero";
        ObjDataList[MARTILLO_HERRERO].OBJType = eOBJType::otWeapon;

        ObjDataList[SERRUCHO_CARPINTERO].name = "Serrucho de Carpintero";
        ObjDataList[SERRUCHO_CARPINTERO].OBJType = eOBJType::otWeapon;

        // Ítem oficial de oro en Argentum Online
        ObjDataList[iORO].name = "Oro";
        ObjDataList[iORO].OBJType = eOBJType::otGuita;
        ObjDataList[iORO].GrhIndex = 500;
        ObjDataList[iORO].Agarrable = 0;
    }

    ~TestEnv() {
        TCP::SetSendDataHook(nullptr);
        InvUsuario::SetQuitarUserInvItemHook(nullptr);
        InvUsuario::SetUpdateUserInvHook(nullptr);
        InvUsuario::SetMeterItemEnInventarioHook(nullptr);
        InvUsuario::SetChangeUserInvHook(nullptr);
        InvUsuario::SetDesequiparHook(nullptr);
        InvUsuario::SetTilelibreHook(nullptr);
        InvUsuario::SetChangeUserCharHook(nullptr);
        InvUsuario::SetDarCuerpoDesnudoHook(nullptr);
        InvUsuario::SetLearnSpellHook(nullptr);
        InvUsuario::SetNavegaHook(nullptr);
        InvUsuario::SetWorkRequestTargetHook(nullptr);
        Modulo_InventANDobj::SetMakeObjHook(nullptr);
        Modulo_InventANDobj::SetTilelibreHook(nullptr);
    }
};

} // namespace

TEST_SUITE("InvUsuario - Fase 1: G1 Mutaciones en el Mundo y Suelo") {

TEST_CASE("G1: MakeObj y EraseObj - Creación, acumulación y borrado físico") {
    TestEnv env;
    constexpr std::int16_t MAP = 1;
    constexpr std::int16_t X = 50;
    constexpr std::int16_t Y = 50;
    const std::size_t idx = block_index(MAP, X, Y);

    MapData[idx].ObjInfo = Obj{};

    SUBCASE("MakeObj en celda vacía asigna objeto") {
        Obj item{1, 100};
        InvUsuario::MakeObj(item, MAP, X, Y);

        CHECK(MapData[idx].ObjInfo.ObjIndex == 1);
        CHECK(MapData[idx].ObjInfo.Amount == 100);
    }

    SUBCASE("MakeObj en celda ocupada por el mismo ítem acumula cantidades en 32 bits") {
        Obj item1{1, 200};
        InvUsuario::MakeObj(item1, MAP, X, Y);

        Obj item2{1, 300};
        InvUsuario::MakeObj(item2, MAP, X, Y);

        CHECK(MapData[idx].ObjInfo.ObjIndex == 1);
        CHECK(MapData[idx].ObjInfo.Amount == 500);
    }

    SUBCASE("EraseObj descuenta unidades parcialmente") {
        Obj item{1, 500};
        InvUsuario::MakeObj(item, MAP, X, Y);

        InvUsuario::EraseObj(200, MAP, X, Y);
        CHECK(MapData[idx].ObjInfo.ObjIndex == 1);
        CHECK(MapData[idx].ObjInfo.Amount == 300);
    }

    SUBCASE("EraseObj consume total y blanquea la celda a 0") {
        Obj item{1, 300};
        InvUsuario::MakeObj(item, MAP, X, Y);

        InvUsuario::EraseObj(300, MAP, X, Y);
        CHECK(MapData[idx].ObjInfo.ObjIndex == 0);
        CHECK(MapData[idx].ObjInfo.Amount == 0);
    }

    SUBCASE("EraseObj con exceso blanquea la celda") {
        Obj item{1, 50};
        InvUsuario::MakeObj(item, MAP, X, Y);

        InvUsuario::EraseObj(100, MAP, X, Y);
        CHECK(MapData[idx].ObjInfo.ObjIndex == 0);
        CHECK(MapData[idx].ObjInfo.Amount == 0);
    }
}

TEST_CASE("G1: DropObj - Arrojar ítems al suelo y validaciones") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    constexpr std::int16_t MAP = 1;
    constexpr std::int16_t X = 30;
    constexpr std::int16_t Y = 30;
    const std::size_t idx = block_index(MAP, X, Y);

    auto& user = UserList[USER];
    user.Pos = WorldPos{MAP, X, Y};
    user.flags.Privilegios = PlayerType::UserPlayer;

    MapData[idx].ObjInfo = Obj{};

    SUBCASE("DropObj normal transfiere ítem al suelo") {
        user.Invent.Object[1] = UserOBJ{2, 50, 0}; // 50 espadas

        std::int16_t quitados = 0;
        InvUsuario::SetQuitarUserInvItemHook([&](std::int16_t u, std::uint8_t slot, std::int16_t cant) {
            quitados += cant;
            user.Invent.Object[slot].Amount -= cant;
        });

        InvUsuario::DropObj(USER, 1, 20, MAP, X, Y);

        CHECK(MapData[idx].ObjInfo.ObjIndex == 2);
        CHECK(MapData[idx].ObjInfo.Amount == 20);
        CHECK(quitados == 20);
        CHECK(user.Invent.Object[1].Amount == 30);
    }

    SUBCASE("DropObj de ítem no-caíble se bloquea sin modificar suelo ni inventario") {
        user.Invent.Object[1] = UserOBJ{3, 1, 0}; // NoSeCae = 1

        bool hook_invocado = false;
        InvUsuario::SetQuitarUserInvItemHook([&](std::int16_t, std::uint8_t, std::int16_t) {
            hook_invocado = true;
        });

        InvUsuario::DropObj(USER, 1, 1, MAP, X, Y);

        CHECK_FALSE(hook_invocado);
        CHECK(MapData[idx].ObjInfo.ObjIndex == 0);
        CHECK(user.Invent.Object[1].Amount == 1);
    }

    SUBCASE("DropObj de ítem newbie por usuario normal se bloquea") {
        user.Invent.Object[1] = UserOBJ{4, 1, 0}; // Newbie = 1
        user.flags.Privilegios = PlayerType::UserPlayer;

        bool hook_invocado = false;
        InvUsuario::SetQuitarUserInvItemHook([&](std::int16_t, std::uint8_t, std::int16_t) {
            hook_invocado = true;
        });

        InvUsuario::DropObj(USER, 1, 1, MAP, X, Y);

        CHECK_FALSE(hook_invocado);
        CHECK(MapData[idx].ObjInfo.ObjIndex == 0);
    }
}

TEST_CASE("G1: Bug #29 - Exploit Histórico de Duplicación en DropObj por Desfase de Cantidades") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    constexpr std::int16_t MAP = 1;
    constexpr std::int16_t X = 40;
    constexpr std::int16_t Y = 40;
    const std::size_t idx = block_index(MAP, X, Y);

    auto& user = UserList[USER];
    user.Pos = WorldPos{MAP, X, Y};
    user.flags.Privilegios = PlayerType::UserPlayer;

    // Escenario histórico del Bug #29:
    // 1. La celda ya tiene 9.990 unidades del ítem 1 (Manzanas)
    MapData[idx].ObjInfo = Obj{1, 9990};

    // 2. El usuario tiene 1.000 manzanas en su ranura 1
    user.Invent.Object[1] = UserOBJ{1, 1000, 0};

    std::int16_t cant_quitada = 0;
    InvUsuario::SetQuitarUserInvItemHook([&](std::int16_t u, std::uint8_t slot, std::int16_t cant) {
        cant_quitada = cant;
        user.Invent.Object[slot].Amount -= cant;
    });

    // 3. El usuario intenta arrojar sus 1.000 manzanas sobre la celda
    InvUsuario::DropObj(USER, 1, 1000, MAP, X, Y);

    // Verificación rigurosa del Bug #29:
    // a) QuitarUserInvItem solo descontó el num recortado: 10.000 - 9.990 = 10 unidades
    CHECK(cant_quitada == 10);
    CHECK(user.Invent.Object[1].Amount == 990);

    // b) MakeObj recibió la estructura Obj con Amount original (1.000), sumando 9.990 + 1.000 = 10.990
    CHECK(MapData[idx].ObjInfo.Amount == 10990);

    // c) El usuario retiene 990 en inventario y el suelo acumuló 10.990 (se duplicaron 990 unidades de la nada)
    CHECK(user.Invent.Object[1].Amount + MapData[idx].ObjInfo.Amount == 990 + 10990);
    CHECK((user.Invent.Object[1].Amount + MapData[idx].ObjInfo.Amount) == (1000 + 9990 + 990));
}

TEST_CASE("G1: GetObj - Recolección de Oro e Ítems desde el Suelo") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    constexpr std::int16_t MAP = 1;
    constexpr std::int16_t X = 25;
    constexpr std::int16_t Y = 25;
    const std::size_t idx = block_index(MAP, X, Y);

    auto& user = UserList[USER];
    user.Pos = WorldPos{MAP, X, Y};
    user.Stats.GLD = 500;

    SUBCASE("GetObj de Oro suma directamente a la billetera y borra el suelo") {
        MapData[idx].ObjInfo = Obj{5, 2500}; // 2500 monedas de oro

        InvUsuario::GetObj(USER);

        CHECK(user.Stats.GLD == 3000); // 500 + 2500
        CHECK(MapData[idx].ObjInfo.ObjIndex == 0);
        CHECK(MapData[idx].ObjInfo.Amount == 0);
    }

    SUBCASE("GetObj de ítem normal delega en MeterItemEnInventario y borra el suelo") {
        MapData[idx].ObjInfo = Obj{2, 10}; // 10 espadas

        bool metido_hook = false;
        InvUsuario::SetMeterItemEnInventarioHook([&](std::int16_t u, Obj& item) -> bool {
            metido_hook = true;
            CHECK(u == USER);
            CHECK(item.ObjIndex == 2);
            CHECK(item.Amount == 10);
            return true;
        });

        InvUsuario::GetObj(USER);

        CHECK(metido_hook);
        CHECK(MapData[idx].ObjInfo.ObjIndex == 0);
        CHECK(MapData[idx].ObjInfo.Amount == 0);
    }

    SUBCASE("GetObj de ítem no agarrable (Agarrable == 1) no se recoge") {
        MapData[idx].ObjInfo = Obj{6, 1}; // Estatua no agarrable

        bool metido_hook = false;
        InvUsuario::SetMeterItemEnInventarioHook([&](std::int16_t, Obj&) -> bool {
            metido_hook = true;
            return true;
        });

        InvUsuario::GetObj(USER);

        CHECK_FALSE(metido_hook);
        CHECK(MapData[idx].ObjInfo.ObjIndex == 6);
        CHECK(MapData[idx].ObjInfo.Amount == 1);
    }
}

TEST_CASE("G1: Interoperabilidad con Modulo_InventANDobj::SetMakeObjHook") {
    TestEnv env;
    constexpr std::int16_t MAP = 1;
    constexpr std::int16_t X = 60;
    constexpr std::int16_t Y = 60;
    const std::size_t idx = block_index(MAP, X, Y);

    MapData[idx].ObjInfo = Obj{};

    // Conectar el hook de Modulo_InventANDobj hacia InvUsuario::MakeObj
    Modulo_InventANDobj::SetMakeObjHook(InvUsuario::MakeObj);

    // Mock de Tilelibre que retorna exactamente la celda destino
    Modulo_InventANDobj::SetTilelibreHook([](const WorldPos& pos, WorldPos& n_pos, const Obj&, bool, bool) {
        n_pos = pos;
    });

    Obj item{2, 15};
    WorldPos origin{MAP, X, Y};
    WorldPos res = Modulo_InventANDobj::TirarItemAlPiso(origin, item);

    CHECK(res.X == X);
    CHECK(res.Y == Y);
    CHECK(MapData[idx].ObjInfo.ObjIndex == 2);
    CHECK(MapData[idx].ObjInfo.Amount == 15);

    // Limpieza de hooks
    Modulo_InventANDobj::SetMakeObjHook(nullptr);
    Modulo_InventANDobj::SetTilelibreHook(nullptr);
}

} // TEST_SUITE

TEST_SUITE("InvUsuario - Fase 2: G2 Gestión Base de Inventario y Descarte") {

TEST_CASE("G2: MeterItemEnInventario - Apilamiento, nuevo slot y mochila llena") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    InvUsuario::LimpiarInventario(USER);
    user.CurrentInventorySlots = 20;

    SUBCASE("Apilamiento en slot existente con el mismo ObjIndex") {
        user.Invent.Object[1] = UserOBJ{2, 50, 0}; // 50 espadas
        user.Invent.NroItems = 1;

        Obj mi_obj{2, 30}; // 30 espadas más
        bool res = InvUsuario::MeterItemEnInventario(USER, mi_obj);

        CHECK(res);
        CHECK(user.Invent.Object[1].Amount == 80);
        CHECK(user.Invent.NroItems == 1);
    }

    SUBCASE("Ocupación de nuevo slot libre e incremento de NroItems") {
        user.Invent.Object[1] = UserOBJ{2, 50, 0};
        user.Invent.NroItems = 1;

        Obj mi_obj{1, 10}; // 10 manzanas
        bool res = InvUsuario::MeterItemEnInventario(USER, mi_obj);

        CHECK(res);
        CHECK(user.Invent.Object[2].ObjIndex == 1);
        CHECK(user.Invent.Object[2].Amount == 10);
        CHECK(user.Invent.NroItems == 2);
    }

    SUBCASE("Rechazo ante mochila llena (20/20 slots ocupados)") {
        user.CurrentInventorySlots = 20;
        user.Invent.NroItems = 20;
        for (std::uint8_t i = 1; i <= 20; ++i) {
            user.Invent.Object[i] = UserOBJ{static_cast<std::int16_t>(10 + i), 1, 0};
        }

        Obj mi_obj{1, 5}; // Ítem nuevo sin slot disponible
        bool res = InvUsuario::MeterItemEnInventario(USER, mi_obj);
        CHECK_FALSE(res);
        CHECK(user.Invent.NroItems == 20);
    }

    SUBCASE("Comportamiento cuando supera 10.000: busca nuevo slot si el existente no alcanza") {
        user.Invent.Object[1] = UserOBJ{1, 9500, 0};
        user.Invent.NroItems = 1;

        Obj mi_obj{1, 800}; // 9500 + 800 = 10300 > 10.000
        bool res = InvUsuario::MeterItemEnInventario(USER, mi_obj);

        // En VB6: al no entrar en el slot 1, pasa al slot 2 vacío y mete los 800 allí
        CHECK(res);
        CHECK(user.Invent.Object[1].Amount == 9500);
        CHECK(user.Invent.Object[2].ObjIndex == 1);
        CHECK(user.Invent.Object[2].Amount == 800);
        CHECK(user.Invent.NroItems == 2);
    }
}

TEST_CASE("G2: QuitarUserInvItem - Deducción, blanqueo y desequipado") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    InvUsuario::LimpiarInventario(USER);
    user.CurrentInventorySlots = 20;

    user.Invent.Object[1] = UserOBJ{2, 50, 0};
    user.Invent.NroItems = 1;

    SUBCASE("Deducción parcial mantiene el slot ocupado") {
        InvUsuario::QuitarUserInvItem(USER, 1, 20);

        CHECK(user.Invent.Object[1].ObjIndex == 2);
        CHECK(user.Invent.Object[1].Amount == 30);
        CHECK(user.Invent.NroItems == 1);
    }

    SUBCASE("Deducción total blanquea el slot y decrementa NroItems") {
        InvUsuario::QuitarUserInvItem(USER, 1, 50);

        CHECK(user.Invent.Object[1].ObjIndex == 0);
        CHECK(user.Invent.Object[1].Amount == 0);
        CHECK(user.Invent.NroItems == 0);
    }

    SUBCASE("Deducción total de slot intermedio (slot 2): no desplaza slots 1 ni 3 (grilla dispersa)") {
        user.Invent.Object[1] = UserOBJ{1, 10, 0}; // Slot 1: 10 manzanas
        user.Invent.Object[2] = UserOBJ{2, 5, 0};  // Slot 2: 5 espadas
        user.Invent.Object[3] = UserOBJ{5, 100, 0}; // Slot 3: 100 oros
        user.Invent.NroItems = 3;

        InvUsuario::QuitarUserInvItem(USER, 2, 5);

        // Slot 2 vaciado
        CHECK(user.Invent.Object[2].ObjIndex == 0);
        CHECK(user.Invent.Object[2].Amount == 0);
        CHECK(user.Invent.Object[2].Equipped == 0);
        CHECK(user.Invent.NroItems == 2);

        // Slots 1 y 3 permanecen exactamente en sus posiciones originales (SIN compactación)
        CHECK(user.Invent.Object[1].ObjIndex == 1);
        CHECK(user.Invent.Object[1].Amount == 10);

        CHECK(user.Invent.Object[3].ObjIndex == 5);
        CHECK(user.Invent.Object[3].Amount == 100);
    }

    SUBCASE("Deducción total de ítem equipado dispara DesequiparHook") {
        user.Invent.Object[1].Equipped = 1;

        bool desequipado_invocado = false;
        InvUsuario::SetDesequiparHook([&](std::int16_t u, std::uint8_t slot) {
            desequipado_invocado = true;
            CHECK(u == USER);
            CHECK(slot == 1);
        });

        InvUsuario::QuitarUserInvItem(USER, 1, 50);

        CHECK(desequipado_invocado);
        CHECK(user.Invent.Object[1].ObjIndex == 0);
    }
}

TEST_CASE("G2: LimpiarInventario y TieneObjetosRobables") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.CurrentInventorySlots = 20;

    user.Invent.Object[1] = UserOBJ{2, 1, 1}; // Espada equipada
    user.Invent.WeaponEqpSlot = 1;
    user.Invent.WeaponEqpObjIndex = 2;
    user.Invent.NroItems = 1;

    CHECK(InvUsuario::TieneObjetosRobables(USER));

    SUBCASE("LimpiarInventario resetea todo a 0") {
        InvUsuario::LimpiarInventario(USER);

        CHECK(user.Invent.NroItems == 0);
        CHECK(user.Invent.WeaponEqpSlot == 0);
        CHECK(user.Invent.WeaponEqpObjIndex == 0);
        for (std::uint8_t i = 1; i <= 30; ++i) {
            CHECK(user.Invent.Object[i].ObjIndex == 0);
            CHECK(user.Invent.Object[i].Amount == 0);
            CHECK(user.Invent.Object[i].Equipped == 0);
        }
        CHECK_FALSE(InvUsuario::TieneObjetosRobables(USER));
    }

    SUBCASE("TieneObjetosRobables da false si solo posee barco o llave") {
        InvUsuario::LimpiarInventario(USER);
        user.Invent.Object[1] = UserOBJ{7, 1, 0}; // Llave (otLlaves)
        user.Invent.Object[2] = UserOBJ{8, 1, 0}; // Barco (otBarcos)
        user.Invent.NroItems = 2;

        CHECK_FALSE(InvUsuario::TieneObjetosRobables(USER));
    }
}

TEST_CASE("G2: QuitarNewbieObj y Quirk de Actualización Incondicional") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    InvUsuario::LimpiarInventario(USER);
    user.CurrentInventorySlots = 20;

    user.Invent.Object[1] = UserOBJ{4, 1, 0};  // Newbie
    user.Invent.Object[2] = UserOBJ{2, 10, 0}; // No Newbie (Espada)
    user.Invent.NroItems = 2;

    std::vector<std::uint8_t> slots_actualizados;
    InvUsuario::SetChangeUserInvHook([&](std::int16_t, std::uint8_t slot, const UserOBJ&) {
        slots_actualizados.push_back(slot);
    });

    InvUsuario::QuitarNewbieObj(USER);

    // El ítem newbie debe haber sido eliminado
    CHECK(user.Invent.Object[1].ObjIndex == 0);
    // La espada permanece intacta
    CHECK(user.Invent.Object[2].ObjIndex == 2);
    CHECK(user.Invent.Object[2].Amount == 10);

    // Quirk histórico de VB6: UpdateUserInv se ejecutó incondicionalmente
    // tanto para el slot 1 (newbie) como para el slot 2 (no-newbie).
    CHECK(slots_actualizados.size() >= 2);
    bool actualizo_slot_2 = false;
    for (auto s : slots_actualizados) {
        if (s == 2) actualizo_slot_2 = true;
    }
    CHECK(actualizo_slot_2);
}

TEST_CASE("G2: Consultas de ítems: ItemSeCae, ItemNewbie, getObjType") {
    TestEnv env;

    CHECK(InvUsuario::ItemSeCae(2));       // Espada común cae
    CHECK_FALSE(InvUsuario::ItemSeCae(3)); // NoSeCae = 1 (no cae)
    CHECK_FALSE(InvUsuario::ItemSeCae(7)); // Llave (otLlaves, no cae)
    CHECK_FALSE(InvUsuario::ItemSeCae(8)); // Barco (otBarcos, no cae)
    CHECK_FALSE(InvUsuario::ItemSeCae(9)); // Armadura Real (Real = 1, no cae)
    CHECK_FALSE(InvUsuario::ItemSeCae(10)); // Armadura Caos (Caos = 1, no cae)
    CHECK_FALSE(InvUsuario::ItemSeCae(0));  // Fuera de rango
    CHECK_FALSE(InvUsuario::ItemSeCae(1005)); // Fuera de rango

    CHECK(InvUsuario::ItemNewbie(4));       // Newbie
    CHECK_FALSE(InvUsuario::ItemNewbie(2)); // No Newbie

    CHECK(InvUsuario::getObjType(1) == eOBJType::otUseOnce);
    CHECK(InvUsuario::getObjType(2) == eOBJType::otWeapon);
    CHECK(InvUsuario::getObjType(7) == eOBJType::otLlaves);
    CHECK(InvUsuario::getObjType(8) == eOBJType::otBarcos);
}

TEST_CASE("G2: TirarTodo - Oro Protegido por Nivel y Zona Segura (Trigger 6)") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    constexpr std::int16_t MAP = 1;
    constexpr std::int16_t X = 30;
    constexpr std::int16_t Y = 30;
    const std::size_t idx = block_index(MAP, X, Y);

    auto& user = UserList[USER];
    user.Pos = WorldPos{MAP, X, Y};
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);

    // Conectar hooks espaciales para TirarOro
    Modulo_InventANDobj::SetMakeObjHook(InvUsuario::MakeObj);
    Modulo_InventANDobj::SetTilelibreHook([](const WorldPos& pos, WorldPos& n_pos, const Obj&, bool, bool) {
        n_pos = pos;
    });

    SUBCASE("Usuario Nivel 20 con 300.000 de oro: solo tira 100.000 y protege 200.000") {
        MapData[idx].trigger = eTrigger::NADA;
        user.Stats.ELV = 20;
        user.Stats.GLD = 300000;
        user.Invent.Object[1] = UserOBJ{2, 1, 0}; // 1 espada
        user.Invent.NroItems = 1;

        InvUsuario::TirarTodo(USER);

        // Oro protegido: 20 * 10.000 = 200.000.
        // Cantidad tirada: 300.000 - 200.000 = 100.000.
        // Saldo final en billetera: 200.000
        CHECK(user.Stats.GLD == 200000);
        CHECK(user.Invent.Object[1].ObjIndex == 0); // Espada tirada
    }

    SUBCASE("Usuario en celda con Trigger 6 no arroja ítems ni oro") {
        MapData[idx].trigger = static_cast<eTrigger>(6); // ZONAPELEA / Trigger 6
        user.Stats.ELV = 20;
        user.Stats.GLD = 300000;
        user.Invent.Object[1] = UserOBJ{2, 1, 0};
        user.Invent.NroItems = 1;

        InvUsuario::TirarTodo(USER);

        // Intacto: nada se tiró
        CHECK(user.Stats.GLD == 300000);
        CHECK(user.Invent.Object[1].ObjIndex == 2);
        CHECK(user.Invent.Object[1].Amount == 1);
        CHECK(user.Invent.NroItems == 1);
    }
}

TEST_CASE("G2: TirarOro Estándar y Replicación del Bug #30 (> 500k)") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    constexpr std::int16_t MAP = 1;
    constexpr std::int16_t X = 35;
    constexpr std::int16_t Y = 35;

    auto& user = UserList[USER];
    user.Pos = WorldPos{MAP, X, Y};

    // Conectar hooks espaciales para TirarItemAlPiso
    Modulo_InventANDobj::SetMakeObjHook(InvUsuario::MakeObj);
    Modulo_InventANDobj::SetTilelibreHook([](const WorldPos& pos, WorldPos& n_pos, const Obj&, bool, bool) {
        n_pos = pos;
    });

    SUBCASE("TirarOro normal (25.000) genera 3 pilas y descuenta exacto") {
        user.Stats.GLD = 50000;

        InvUsuario::TirarOro(25000, USER);

        // Se deben haber arrojado 25.000 monedas:
        // 50.000 - 25.000 = 25.000 restante
        CHECK(user.Stats.GLD == 25000);
    }

    SUBCASE("Bug #30: TirarOro (> 500k) evapora silenciosamente el remanente Extra de la billetera") {
        // Escenario histórico del Bug #30:
        // 1. Usuario posee 1.000.000 de oro
        user.Stats.GLD = 1000000;

        // Contador de cantidad física arrojada al suelo
        std::int32_t oro_tirado_al_suelo = 0;
        Modulo_InventANDobj::SetMakeObjHook([&](const Obj& obj, std::int16_t, std::int16_t, std::int16_t) {
            if (obj.ObjIndex == iORO) {
                oro_tirado_al_suelo += obj.Amount;
            }
        });

        // 2. Ejecuta /TIRARORO 800000 (excede 500k por 300k)
        InvUsuario::TirarOro(800000, USER);

        // Verificación rigurosa del Bug #30:
        // a) En el mundo SOLO se arrojaron 500.000 monedas (50 pilas de 10.000)
        CHECK(oro_tirado_al_suelo == 500000);

        // b) De la billetera se descontaron los 500.000 arrojados + los 300.000 de Extra evaporados
        // Saldo final: 1.000.000 - 500.000 - 300.000 = 200.000
        CHECK(user.Stats.GLD == 200000);
    }
}

TEST_CASE("G2: TirarTodosLosItems y Descarte con Tilelibre") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    constexpr std::int16_t MAP = 1;
    constexpr std::int16_t X = 70;
    constexpr std::int16_t Y = 70;

    auto& user = UserList[USER];
    user.Pos = WorldPos{MAP, X, Y};
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);

    user.Invent.Object[1] = UserOBJ{2, 10, 0}; // Espada (cae)
    user.Invent.Object[2] = UserOBJ{3, 1, 0};  // NoSeCae = 1 (no cae)
    user.Invent.NroItems = 2;

    SUBCASE("Ítems caíbles son arrojados a celdas devueltas por Tilelibre") {
        std::int16_t items_arrojados = 0;
        InvUsuario::SetTilelibreHook([&](const WorldPos& pos, WorldPos& n_pos, const Obj&, bool, bool) {
            n_pos = WorldPos{pos.Map, static_cast<std::int16_t>(pos.X + 1), pos.Y};
            items_arrojados++;
        });

        InvUsuario::TirarTodosLosItems(USER);

        // Solo la espada se arroja
        CHECK(items_arrojados == 1);
        CHECK(user.Invent.Object[1].ObjIndex == 0); // Descontado
        CHECK(user.Invent.Object[2].ObjIndex == 3); // Retenido (NoSeCae)
    }

    SUBCASE("Si Tilelibre devuelve (0, 0), el ítem se retiene en el inventario") {
        InvUsuario::SetTilelibreHook([](const WorldPos&, WorldPos& n_pos, const Obj&, bool, bool) {
            n_pos = WorldPos{0, 0, 0}; // Sin espacio
        });

        InvUsuario::TirarTodosLosItems(USER);

        // Al retornar (0, 0), DropObj no se convoca y el ítem se retiene
        CHECK(user.Invent.Object[1].ObjIndex == 2);
        CHECK(user.Invent.Object[1].Amount == 10);
    }
}

} // TEST_SUITE

TEST_SUITE("InvUsuario - G3: Restricciones y Sistema de Equipamiento") {

TEST_CASE("G3: Validaciones de Clase, Género, Facción, Raza y Atributos") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.flags.Privilegios = PlayerType::UserPlayer;

    SUBCASE("ClasePuedeUsarItem: rechazo si la clase está prohibida y bypass para GM") {
        // Ítem 2 prohibido para Magos (CP1 = Mago)
        ObjDataList[2].ClaseProhibida[1] = eClass::Mage;
        user.clase = eClass::Mage;

        std::string motivo;
        CHECK_FALSE(InvUsuario::ClasePuedeUsarItem(USER, 2, &motivo));
        CHECK(motivo == "Tu clase no puede usar este objeto.");

        // Guerrero sí puede
        user.clase = eClass::Warrior;
        CHECK(InvUsuario::ClasePuedeUsarItem(USER, 2));

        // Admin/GM puede usarlo aunque sea Mago
        user.clase = eClass::Mage;
        user.flags.Privilegios = PlayerType::Admin;
        CHECK(InvUsuario::ClasePuedeUsarItem(USER, 2));
    }

    SUBCASE("SexoPuedeUsarItem: validación de género masculino / femenino") {
        user.Genero = eGenero::Hombre;
        std::string motivo;

        // Ítem 17 es para mujer
        CHECK_FALSE(InvUsuario::SexoPuedeUsarItem(USER, 17, &motivo));
        CHECK(motivo == "Tu género no puede usar este objeto.");

        user.Genero = eGenero::Mujer;
        CHECK(InvUsuario::SexoPuedeUsarItem(USER, 17));
    }

    SUBCASE("FaccionPuedeUsarItem: Armada Real vs Fuerzas del Caos") {
        std::string motivo;

        // Ítem 9: Armadura Real
        user.Faccion.ArmadaReal = 0;
        user.Reputacion.NobleRep = 1000;
        CHECK_FALSE(InvUsuario::FaccionPuedeUsarItem(USER, 9, &motivo));
        CHECK(motivo == "Tu alineación no puede usar este objeto.");

        user.Faccion.ArmadaReal = 1;
        CHECK(InvUsuario::FaccionPuedeUsarItem(USER, 9));

        // Criminal con ítem real -> rechazo
        user.Reputacion.NobleRep = 0;
        user.Reputacion.AsesinoRep = 5000; // Criminal
        CHECK_FALSE(InvUsuario::FaccionPuedeUsarItem(USER, 9));

        // Ítem 10: Armadura Caos
        user.Faccion.FuerzasCaos = 1;
        CHECK(InvUsuario::FaccionPuedeUsarItem(USER, 10));

        // Si se vuelve ciudadano no puede usar Caos
        user.Reputacion.AsesinoRep = 0;
        user.Reputacion.NobleRep = 5000;
        CHECK_FALSE(InvUsuario::FaccionPuedeUsarItem(USER, 10));
    }

    SUBCASE("CheckRazaUsaRopa: Enano/Gnomo vs Humano/Elfo/Drow y exclusividad Drow") {
        std::string motivo;

        // Ítem 19: Ropaje Enano (RazaEnana = 1)
        user.raza = eRaza::Humano;
        CHECK_FALSE(InvUsuario::CheckRazaUsaRopa(USER, 19, &motivo));
        CHECK(motivo == "Tu raza no puede usar este objeto.");

        user.raza = eRaza::Enano;
        CHECK(InvUsuario::CheckRazaUsaRopa(USER, 19));

        user.raza = eRaza::Gnomo;
        CHECK(InvUsuario::CheckRazaUsaRopa(USER, 19));

        // Ítem 18: Túnica Exclusiva Drow
        user.raza = eRaza::Humano;
        CHECK_FALSE(InvUsuario::CheckRazaUsaRopa(USER, 18, &motivo));

        user.raza = eRaza::Drow;
        CHECK(InvUsuario::CheckRazaUsaRopa(USER, 18));
    }
}

TEST_CASE("G3: Ciclo de Equipamiento, Toggle y Sustitución de Ranura") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.clase = eClass::Warrior;
    user.raza = eRaza::Humano;
    user.Genero = eGenero::Hombre;
    user.Stats.UserAtributos[eAtributos::Fuerza] = 20;
    user.Stats.UserAtributos[eAtributos::Agilidad] = 20;

    SUBCASE("Toggle de Equipamiento en el mismo slot") {
        user.Invent.Object[1] = UserOBJ{2, 1, 0}; // Espada común
        user.Invent.NroItems = 1;

        // 1. Equipar
        InvUsuario::EquiparInvItem(USER, 1);
        CHECK(user.Invent.Object[1].Equipped == 1);
        CHECK(user.Invent.WeaponEqpSlot == 1);
        CHECK(user.Invent.WeaponEqpObjIndex == 2);

        // 2. Invocar nuevamente sobre el mismo slot -> Desequipar (Toggle)
        InvUsuario::EquiparInvItem(USER, 1);
        CHECK(user.Invent.Object[1].Equipped == 0);
        CHECK(user.Invent.WeaponEqpSlot == 0);
        CHECK(user.Invent.WeaponEqpObjIndex == 0);
    }

    SUBCASE("Sustitución de Ranura al equipar otro ítem del mismo tipo") {
        user.Invent.Object[1] = UserOBJ{2, 1, 0};  // Espada 1
        user.Invent.Object[2] = UserOBJ{20, 1, 0}; // Martillo Pesado (Espada 2)
        user.Invent.NroItems = 2;

        // Equipar espada 1
        InvUsuario::EquiparInvItem(USER, 1);
        CHECK(user.Invent.Object[1].Equipped == 1);
        CHECK(user.Invent.WeaponEqpSlot == 1);

        // Equipar martillo en slot 2 -> desequipa slot 1 automáticamente
        InvUsuario::EquiparInvItem(USER, 2);
        CHECK(user.Invent.Object[1].Equipped == 0);
        CHECK(user.Invent.Object[2].Equipped == 1);
        CHECK(user.Invent.WeaponEqpSlot == 2);
        CHECK(user.Invent.WeaponEqpObjIndex == 20);
    }

    SUBCASE("Validación de Fuerza y Agilidad mínimas") {
        user.Invent.Object[1] = UserOBJ{20, 1, 0}; // Requiere Fuerza 18 y Agilidad 15
        user.Invent.NroItems = 1;

        user.Stats.UserAtributos[eAtributos::Fuerza] = 12; // Insuficiente
        InvUsuario::EquiparInvItem(USER, 1);
        CHECK(user.Invent.Object[1].Equipped == 0);
        CHECK(user.Invent.WeaponEqpSlot == 0);

        user.Stats.UserAtributos[eAtributos::Fuerza] = 18;
        user.Stats.UserAtributos[eAtributos::Agilidad] = 10; // Insuficiente
        InvUsuario::EquiparInvItem(USER, 1);
        CHECK(user.Invent.Object[1].Equipped == 0);

        user.Stats.UserAtributos[eAtributos::Agilidad] = 15; // Ahora sí cumple ambos
        InvUsuario::EquiparInvItem(USER, 1);
        CHECK(user.Invent.Object[1].Equipped == 1);
        CHECK(user.Invent.WeaponEqpSlot == 1);
    }
}

TEST_CASE("G3: Regla Arma Dos Manos vs Escudo") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.clase = eClass::Warrior;
    user.raza = eRaza::Humano;
    user.Genero = eGenero::Hombre;
    user.Stats.UserAtributos[eAtributos::Fuerza] = 20;
    user.Stats.UserAtributos[eAtributos::Agilidad] = 20;

    user.Invent.Object[1] = UserOBJ{11, 1, 0}; // Escudo de Hierro
    user.Invent.Object[2] = UserOBJ{13, 1, 0}; // Espada Dos Manos (DosManos = 1)
    user.Invent.NroItems = 2;

    SUBCASE("Con escudo equipado, equipar arma a dos manos desequipa el escudo") {
        // Equipar escudo
        InvUsuario::EquiparInvItem(USER, 1);
        CHECK(user.Invent.Object[1].Equipped == 1);
        CHECK(user.Invent.EscudoEqpSlot == 1);

        // Equipar arma a dos manos
        InvUsuario::EquiparInvItem(USER, 2);
        CHECK(user.Invent.Object[2].Equipped == 1);
        CHECK(user.Invent.WeaponEqpSlot == 2);

        // El escudo debe haberse desequipado automáticamente
        CHECK(user.Invent.Object[1].Equipped == 0);
        CHECK(user.Invent.EscudoEqpSlot == 0);
        CHECK(user.Invent.EscudoEqpObjIndex == 0);
    }

    SUBCASE("Con arma a dos manos equipada, intentar equipar escudo se rechaza") {
        // Equipar arma a dos manos
        InvUsuario::EquiparInvItem(USER, 2);
        CHECK(user.Invent.Object[2].Equipped == 1);
        CHECK(user.Invent.WeaponEqpSlot == 2);

        // Intentar equipar escudo
        InvUsuario::EquiparInvItem(USER, 1);

        // Rechazo: el escudo permanece sin equipar
        CHECK(user.Invent.Object[1].Equipped == 0);
        CHECK(user.Invent.EscudoEqpSlot == 0);
        CHECK(user.Invent.Object[2].Equipped == 1);
    }
}

TEST_CASE("G3: Mochila Expandida y Caída de Ítems al Desequipar") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    constexpr std::int16_t MAP = 1;
    constexpr std::int16_t X = 50;
    constexpr std::int16_t Y = 50;

    auto& user = UserList[USER];
    user.Pos = WorldPos{MAP, X, Y};
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);
    user.flags.Privilegios = PlayerType::UserPlayer;

    user.Invent.Object[1] = UserOBJ{16, 1, 0}; // Mochila (expande a 30 slots)
    user.Invent.NroItems = 1;

    // Conectar hooks de descarte para TirarTodosLosItemsEnMochila
    std::int16_t items_caidos = 0;
    InvUsuario::SetTilelibreHook([&](const WorldPos& pos, WorldPos& n_pos, const Obj&, bool, bool) {
        n_pos = pos;
        items_caidos++;
    });
    Modulo_InventANDobj::SetMakeObjHook(InvUsuario::MakeObj);

    // 1. Equipar mochila
    InvUsuario::EquiparInvItem(USER, 1);
    CHECK(user.Invent.Object[1].Equipped == 1);
    CHECK(user.Invent.MochilaEqpSlot == 1);
    CHECK(user.Invent.MochilaEqpObjIndex == 16);
    CHECK(user.CurrentInventorySlots == 30);

    // 2. Colocar un ítem en el slot 25
    user.Invent.Object[25] = UserOBJ{2, 1, 0}; // Espada en slot expandido
    user.Invent.NroItems = 2;

    // 3. Desequipar la mochila
    InvUsuario::Desequipar(USER, 1);

    // Verificaciones:
    // a) El límite de inventario regresa a 20 slots normales
    CHECK(user.CurrentInventorySlots == 20);
    CHECK(user.Invent.MochilaEqpSlot == 0);
    CHECK(user.Invent.MochilaEqpObjIndex == 0);

    // b) El ítem del slot 25 fue purgado y arrojado al suelo
    CHECK(items_caidos == 1);
    CHECK(user.Invent.Object[25].ObjIndex == 0);
    CHECK(user.Invent.Object[25].Amount == 0);
}

TEST_CASE("G3: Integración G2/G3 - Desequipamiento Forzado por Cantidad Cero en QuitarUserInvItem") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.clase = eClass::Hunter;

    // Equipar flechas en el slot 3
    user.Invent.Object[3] = UserOBJ{14, 50, 0}; // 50 flechas
    user.Invent.NroItems = 1;

    InvUsuario::EquiparInvItem(USER, 3);
    CHECK(user.Invent.Object[3].Equipped == 1);
    CHECK(user.Invent.MunicionEqpSlot == 3);
    CHECK(user.Invent.MunicionEqpObjIndex == 14);

    // Quitar 50 flechas completas
    InvUsuario::QuitarUserInvItem(USER, 3, 50);

    // Debe haberse limpiado el slot y disparado Desequipar
    CHECK(user.Invent.Object[3].ObjIndex == 0);
    CHECK(user.Invent.Object[3].Amount == 0);
    CHECK(user.Invent.Object[3].Equipped == 0);
    CHECK(user.Invent.MunicionEqpSlot == 0);
    CHECK(user.Invent.MunicionEqpObjIndex == 0);
}

} // TEST_SUITE

TEST_SUITE("InvUsuario - G4: Uso de Ítems e Interacción") {

TEST_CASE("G4: Consumo de Alimentos y Bebidas - Hambre, Sed, Topes y Descuento de Slot") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.flags.Muerto = 0;

    // 1. Consumo de Comida (Manzana, MinHam = 30)
    user.Stats.MinHam = 20;
    user.Stats.MaxHam = 100;
    user.flags.Hambre = 1;

    user.Invent.Object[1] = UserOBJ{1, 3, 0}; // 3 manzanas
    user.Invent.NroItems = 1;

    InvUsuario::UseInvItem(USER, 1);

    // Verificaciones de comida
    CHECK(user.Stats.MinHam == 50); // 20 + 30
    CHECK(user.flags.Hambre == 0);
    CHECK(user.Invent.Object[1].Amount == 2); // Consumió 1

    // Consumir dos veces más para comprobar tope MaxHam
    InvUsuario::UseInvItem(USER, 1); // 50 + 30 = 80
    CHECK(user.Stats.MinHam == 80);
    CHECK(user.Invent.Object[1].Amount == 1);

    InvUsuario::UseInvItem(USER, 1); // 80 + 30 = 110 -> tope 100
    CHECK(user.Stats.MinHam == 100);
    CHECK(user.Invent.Object[1].Amount == 0);
    CHECK(user.Invent.Object[1].ObjIndex == 0);

    // 2. Consumo de Bebida (Botella de Agua, MinSed = 30)
    user.Stats.MinAGU = 15;
    user.Stats.MaxAGU = 100;
    user.flags.Sed = 1;

    user.Invent.Object[2] = UserOBJ{21, 2, 0}; // 2 botellas
    user.Invent.NroItems = 1;

    InvUsuario::UseInvItem(USER, 2);

    // Verificaciones de bebida
    CHECK(user.Stats.MinAGU == 45); // 15 + 30
    CHECK(user.flags.Sed == 0);
    CHECK(user.Invent.Object[2].Amount == 1);
}

TEST_CASE("G4: Consumo de Pociones - Curación HP, Restauración Maná y Antídoto de Veneno") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.flags.Muerto = 0;

    // 1. Poción de Vida (Poción Roja, TipoPocion = 3, MinModificador = 40)
    user.Stats.MinHp = 30;
    user.Stats.MaxHp = 100;
    user.Invent.Object[1] = UserOBJ{22, 2, 0}; // 2 pociones de vida
    user.Invent.NroItems = 1;

    InvUsuario::UseInvItem(USER, 1);
    CHECK(user.Stats.MinHp == 70); // 30 + 40
    CHECK(user.Invent.Object[1].Amount == 1);

    // Segunda poción -> tope en MaxHp
    InvUsuario::UseInvItem(USER, 1);
    CHECK(user.Stats.MinHp == 100); // Acotado a 100
    CHECK(user.Invent.Object[1].Amount == 0);

    // 2. Poción de Maná (Poción Azul, TipoPocion = 4, MinModificador = 25)
    user.Stats.MinMAN = 10;
    user.Stats.MaxMAN = 100;
    user.Invent.Object[2] = UserOBJ{23, 1, 0};
    user.Invent.NroItems = 1;

    InvUsuario::UseInvItem(USER, 2);
    CHECK(user.Stats.MinMAN == 35); // 10 + 25
    CHECK(user.Invent.Object[2].Amount == 0);

    // 3. Poción Antídoto (Poción Violeta, TipoPocion = 5)
    user.flags.Envenenado = 1;
    user.Invent.Object[3] = UserOBJ{26, 1, 0};
    user.Invent.NroItems = 1;

    InvUsuario::UseInvItem(USER, 3);
    CHECK(user.flags.Envenenado == 0);
    CHECK(user.Invent.Object[3].Amount == 0);
}

TEST_CASE("G4: Consumo de Pociones - Modificadores Temporales de Atributos (Agilidad y Fuerza)") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.flags.Muerto = 0;

    // 1. Poción de Agilidad (Poción Amarilla, TipoPocion = 1, mod = 5, duracion = 500)
    user.Stats.UserAtributos[static_cast<std::size_t>(eAtributos::Agilidad)] = 18;
    user.Invent.Object[1] = UserOBJ{24, 2, 0};
    user.Invent.NroItems = 1;

    InvUsuario::UseInvItem(USER, 1);
    CHECK(user.Stats.UserAtributos[static_cast<std::size_t>(eAtributos::Agilidad)] == 23);
    CHECK(user.flags.DuracionEfecto == 500);
    CHECK(user.Invent.Object[1].Amount == 1);

    // 2. Poción de Fuerza (Poción Verde, TipoPocion = 2, mod = 4, duracion = 500)
    user.Stats.UserAtributos[static_cast<std::size_t>(eAtributos::Fuerza)] = 20;
    user.Invent.Object[2] = UserOBJ{25, 1, 0};
    user.Invent.NroItems = 1;

    InvUsuario::UseInvItem(USER, 2);
    CHECK(user.Stats.UserAtributos[static_cast<std::size_t>(eAtributos::Fuerza)] == 24);
    CHECK(user.flags.DuracionEfecto == 500);
    CHECK(user.Invent.Object[2].Amount == 0);
}

TEST_CASE("G4: Consumo de Oro (otGuita) - Acreditación en Billetera y Vaciado de Slot") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.flags.Muerto = 0;

    user.Stats.GLD = 1500;
    user.Invent.Object[1] = UserOBJ{iORO, 3500, 0};
    user.Invent.NroItems = 1;

    InvUsuario::UseInvItem(USER, 1);

    CHECK(user.Stats.GLD == 5000); // 1500 + 3500
    CHECK(user.Invent.Object[1].ObjIndex == 0);
    CHECK(user.Invent.Object[1].Amount == 0);
    CHECK(user.Invent.NroItems == 0);
}

TEST_CASE("G4: Ítem Equipable Redirigido - Uso de Arma de Combate delega en EquiparInvItem") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.flags.Muerto = 0;
    user.clase = eClass::Warrior;
    user.raza = eRaza::Humano;
    user.Genero = eGenero::Hombre;
    user.Pos.Map = 1;
    user.Pos.X = 50;
    user.Pos.Y = 50;
    user.Stats.UserAtributos[eAtributos::Fuerza] = 20;
    user.Stats.UserAtributos[eAtributos::Agilidad] = 20;
    user.Stats.MinSta = 50;
    user.Stats.MaxSta = 100;

    // Espada común (ítem 2, otWeapon, no es herramienta)
    user.Invent.Object[1] = UserOBJ{2, 1, 0};
    user.Invent.NroItems = 1;

    CHECK(user.Invent.Object[1].Amount == 1);
    CHECK(user.Invent.Object[1].ObjIndex == 2);
    CHECK(user.CurrentInventorySlots == 20);
    CHECK(InvUsuario::ClasePuedeUsarItem(USER, 2));
    CHECK(InvUsuario::FaccionPuedeUsarItem(USER, 2));

    // Usar la espada debe equiparla
    InvUsuario::UseInvItem(USER, 1);
    CHECK(user.flags.TargetObjInvIndex == 2);
    CHECK(user.flags.TargetObjInvSlot == 1);
    CHECK(user.Invent.Object[1].Equipped == 1);
    CHECK(user.Invent.WeaponEqpSlot == 1);
    CHECK(user.Invent.WeaponEqpObjIndex == 2);

    // Usarla de nuevo opera como toggle y la desequipa
    InvUsuario::UseInvItem(USER, 1);
    CHECK(user.Invent.Object[1].Equipped == 0);
    CHECK(user.Invent.WeaponEqpSlot == 0);
    CHECK(user.Invent.WeaponEqpObjIndex == 0);
}

TEST_CASE("G4: Pergaminos de Hechizos - Delegación en LearnSpellHook y Consumo Condicional") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.flags.Muerto = 0;
    user.Stats.MaxMAN = 100;
    user.flags.Hambre = 0;
    user.flags.Sed = 0;

    user.Invent.Object[1] = UserOBJ{27, 1, 0}; // Pergamino con HechizoIndex = 7
    user.Invent.NroItems = 1;

    bool hook_llamado = false;
    std::int16_t hechizo_aprendido = 0;
    bool retornar_exito = false;

    InvUsuario::SetLearnSpellHook([&](std::int16_t u_idx, std::int16_t s_idx) -> bool {
        hook_llamado = true;
        hechizo_aprendido = s_idx;
        return retornar_exito;
    });

    // 1. Intento fallido de aprendizaje -> No debe consumirse el pergamino
    retornar_exito = false;
    InvUsuario::UseInvItem(USER, 1);
    CHECK(hook_llamado == true);
    CHECK(hechizo_aprendido == 7);
    CHECK(user.Invent.Object[1].Amount == 1); // No consumido

    // 2. Intento exitoso de aprendizaje -> Se consume el pergamino
    hook_llamado = false;
    retornar_exito = true;
    InvUsuario::UseInvItem(USER, 1);
    CHECK(hook_llamado == true);
    CHECK(user.Invent.Object[1].Amount == 0); // Consumido
    CHECK(user.Invent.Object[1].ObjIndex == 0);
}

TEST_CASE("G4: Barcos y Navegación - Delegación en NavegaHook") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.flags.Muerto = 0;

    user.Invent.Object[1] = UserOBJ{8, 1, 0}; // Barco (otBarcos)
    user.Invent.NroItems = 1;

    bool navega_invocado = false;
    std::int16_t user_recibido = 0;
    std::int16_t barco_recibido = 0;

    InvUsuario::SetNavegaHook([&](std::int16_t u_idx, const Obj& barco_obj) {
        navega_invocado = true;
        user_recibido = u_idx;
        barco_recibido = barco_obj.ObjIndex;
    });

    InvUsuario::UseInvItem(USER, 1);

    CHECK(navega_invocado == true);
    CHECK(user_recibido == USER);
    CHECK(barco_recibido == 8);
}

TEST_CASE("G4: Herramientas de Trabajo - Delegación en WorkRequestTargetHook según Herramienta Equipada") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.CurrentInventorySlots = 20;
    InvUsuario::LimpiarInventario(USER);
    user.flags.Privilegios = PlayerType::UserPlayer;
    user.flags.Muerto = 0;

    eSkill skill_solicitado = static_cast<eSkill>(0);
    int llamadas_trabajo = 0;

    InvUsuario::SetWorkRequestTargetHook([&](std::int16_t, eSkill skill) {
        skill_solicitado = skill;
        llamadas_trabajo++;
    });

    // 1. Piquete de Minero: no equipado -> no llama hook
    user.Invent.Object[1] = UserOBJ{PIQUETE_MINERO, 1, 0};
    user.Invent.NroItems = 1;
    user.Invent.WeaponEqpObjIndex = 0;

    InvUsuario::UseInvItem(USER, 1);
    CHECK(llamadas_trabajo == 0);

    // Equipar el piquete y volver a usar
    user.Invent.WeaponEqpObjIndex = PIQUETE_MINERO;
    InvUsuario::UseInvItem(USER, 1);
    CHECK(llamadas_trabajo == 1);
    CHECK(skill_solicitado == eSkill::Mineria);

    // 2. Hacha de Leñador
    user.Invent.Object[2] = UserOBJ{HACHA_LEÑADOR, 1, 0};
    user.Invent.WeaponEqpObjIndex = HACHA_LEÑADOR;
    InvUsuario::UseInvItem(USER, 2);
    CHECK(llamadas_trabajo == 2);
    CHECK(skill_solicitado == eSkill::Talar);

    // 3. Martillo de Herrero
    user.Invent.Object[3] = UserOBJ{MARTILLO_HERRERO, 1, 0};
    user.Invent.WeaponEqpObjIndex = MARTILLO_HERRERO;
    InvUsuario::UseInvItem(USER, 3);
    CHECK(llamadas_trabajo == 3);
    CHECK(skill_solicitado == eSkill::Herreria);

    // 4. Caña de Pescar
    user.Invent.Object[4] = UserOBJ{CAÑA_PESCA, 1, 0};
    user.Invent.WeaponEqpObjIndex = CAÑA_PESCA;
    InvUsuario::UseInvItem(USER, 4);
    CHECK(llamadas_trabajo == 4);
    CHECK(skill_solicitado == eSkill::Pesca);
}

TEST_CASE("G4: Despacho de Construibles - EnivarArmas, EnivarObj y EnivarArmaduras") {
    TestEnv env;
    constexpr std::int16_t USER = 1;
    auto& user = UserList[USER];
    user.ConnID = 1;
    user.ConnIDValida = true;

    // 1. Despachar armas construibles
    user.outgoingData = std::make_unique<clsByteQueue>();
    InvUsuario::EnivarArmasConstruibles(USER);
    CHECK(user.outgoingData != nullptr);
    CHECK(user.outgoingData->length() > 0);

    // 2. Despachar objetos construibles
    user.outgoingData = std::make_unique<clsByteQueue>();
    InvUsuario::EnivarObjConstruibles(USER);
    CHECK(user.outgoingData != nullptr);
    CHECK(user.outgoingData->length() > 0);

    // 3. Despachar armaduras construibles
    user.outgoingData = std::make_unique<clsByteQueue>();
    InvUsuario::EnivarArmadurasConstruibles(USER);
    CHECK(user.outgoingData != nullptr);
    CHECK(user.outgoingData->length() > 0);
}

} // TEST_SUITE


