#include <doctest/doctest.h>
#include "server/Modulo_InventANDobj.hpp"
#include "server/Declares.hpp"
#include <vector>
#include <deque>

TEST_SUITE("Modulo_InventANDobj - G1") {

TEST_CASE("G1: Gestión de Inventario NPC y Reposición de Stock") {
    constexpr std::int16_t TEST_NPC = 1;

    // Asegurar que ObjDataList tenga suficiente tamaño para las pruebas
    if (ObjDataList.size() < 200) {
        ObjDataList.resize(200);
    }

    SUBCASE("ResetNpcInv: Blanqueo integral de slots y reseteo de contadores") {
        auto& npc = Npclist[TEST_NPC];
        npc.Invent.NroItems = 3;
        npc.InvReSpawn = 2;
        npc.Invent.Object[1].ObjIndex = 5;
        npc.Invent.Object[1].Amount = 10;
        npc.Invent.Object[15].ObjIndex = 20;
        npc.Invent.Object[15].Amount = 50;
        npc.Invent.Object[30].ObjIndex = 99;
        npc.Invent.Object[30].Amount = 100;

        Modulo_InventANDobj::ResetNpcInv(TEST_NPC);

        CHECK(npc.Invent.NroItems == 0);
        CHECK(npc.InvReSpawn == 0);
        for (std::size_t i = 1; i <= MAX_INVENTORY_SLOTS; ++i) {
            CHECK(npc.Invent.Object[i].ObjIndex == 0);
            CHECK(npc.Invent.Object[i].Amount == 0);
        }
    }

    SUBCASE("QuedanItems: Búsqueda lineal de ítems en slots activos") {
        Modulo_InventANDobj::ResetNpcInv(TEST_NPC);
        auto& npc = Npclist[TEST_NPC];

        // Con inventario vacío (NroItems = 0) debe dar false
        CHECK_FALSE(Modulo_InventANDobj::QuedanItems(TEST_NPC, 10));

        // Asignamos dos objetos
        npc.Invent.NroItems = 2;
        npc.Invent.Object[3].ObjIndex = 42;
        npc.Invent.Object[3].Amount = 100;
        npc.Invent.Object[28].ObjIndex = 88;
        npc.Invent.Object[28].Amount = 5;

        CHECK(Modulo_InventANDobj::QuedanItems(TEST_NPC, 42));
        CHECK(Modulo_InventANDobj::QuedanItems(TEST_NPC, 88));
        CHECK_FALSE(Modulo_InventANDobj::QuedanItems(TEST_NPC, 99)); // No presente
        CHECK_FALSE(Modulo_InventANDobj::QuedanItems(TEST_NPC, 0));  // Inválido
        CHECK_FALSE(Modulo_InventANDobj::QuedanItems(-1, 42));       // NPC inválido
    }

    SUBCASE("EncontrarCant: Consulta de stock base mediante hook desacoplado") {
        Modulo_InventANDobj::ResetNpcInv(TEST_NPC);
        auto& npc = Npclist[TEST_NPC];
        npc.Numero = 500;

        Modulo_InventANDobj::SetNPCTemplateLookupHook([](std::int16_t numero) -> std::vector<Modulo_InventANDobj::NPCInventorySlot> {
            if (numero == 500) {
                return {
                    {15, 1000},
                    {25, 250},
                    {35, 50}
                };
            }
            return {};
        });

        CHECK(Modulo_InventANDobj::EncontrarCant(TEST_NPC, 15) == 1000);
        CHECK(Modulo_InventANDobj::EncontrarCant(TEST_NPC, 25) == 250);
        CHECK(Modulo_InventANDobj::EncontrarCant(TEST_NPC, 35) == 50);
        CHECK(Modulo_InventANDobj::EncontrarCant(TEST_NPC, 99) == 0); // No está en la plantilla
    }

    SUBCASE("QuitarNpcInvItem: Consumo parcial de ítems") {
        Modulo_InventANDobj::ResetNpcInv(TEST_NPC);
        auto& npc = Npclist[TEST_NPC];
        npc.Invent.NroItems = 1;
        npc.Invent.Object[1].ObjIndex = 10;
        npc.Invent.Object[1].Amount = 50;

        ObjDataList[10].Crucial = 0;

        Modulo_InventANDobj::QuitarNpcInvItem(TEST_NPC, 1, 20);

        CHECK(npc.Invent.Object[1].ObjIndex == 10);
        CHECK(npc.Invent.Object[1].Amount == 30);
        CHECK(npc.Invent.NroItems == 1);
    }

    SUBCASE("QuitarNpcInvItem: Vaciado de ítem no crucial purga el slot") {
        Modulo_InventANDobj::ResetNpcInv(TEST_NPC);
        auto& npc = Npclist[TEST_NPC];
        npc.Invent.NroItems = 2;
        npc.Invent.Object[1].ObjIndex = 10;
        npc.Invent.Object[1].Amount = 15;
        npc.Invent.Object[2].ObjIndex = 20;
        npc.Invent.Object[2].Amount = 30;

        ObjDataList[10].Crucial = 0;
        ObjDataList[20].Crucial = 0;

        // Descontamos exactamente la cantidad disponible
        Modulo_InventANDobj::QuitarNpcInvItem(TEST_NPC, 1, 15);

        CHECK(npc.Invent.Object[1].ObjIndex == 0);
        CHECK(npc.Invent.Object[1].Amount == 0);
        CHECK(npc.Invent.NroItems == 1);
        CHECK(npc.Invent.Object[2].ObjIndex == 20);
        CHECK(npc.Invent.Object[2].Amount == 30);
    }

    SUBCASE("QuitarNpcInvItem: Vaciado de ítem crucial repone automáticamente el stock original") {
        Modulo_InventANDobj::ResetNpcInv(TEST_NPC);
        auto& npc = Npclist[TEST_NPC];
        npc.Numero = 600;
        npc.Invent.NroItems = 2;
        npc.Invent.Object[1].ObjIndex = 77;
        npc.Invent.Object[1].Amount = 5;
        npc.Invent.Object[2].ObjIndex = 20;
        npc.Invent.Object[2].Amount = 30;

        ObjDataList[77].Crucial = 1; // Ítem crucial
        ObjDataList[20].Crucial = 0;

        Modulo_InventANDobj::SetNPCTemplateLookupHook([](std::int16_t numero) -> std::vector<Modulo_InventANDobj::NPCInventorySlot> {
            if (numero == 600) {
                return {
                    {77, 500},
                    {20, 30}
                };
            }
            return {};
        });

        // Consumimos todo el stock del slot 1
        Modulo_InventANDobj::QuitarNpcInvItem(TEST_NPC, 1, 5);

        // Debe reponerse con el stock base (500) y mantener NroItems = 2
        CHECK(npc.Invent.Object[1].ObjIndex == 77);
        CHECK(npc.Invent.Object[1].Amount == 500);
        CHECK(npc.Invent.NroItems == 2);
    }

    SUBCASE("QuitarNpcInvItem: Ítem crucial NO se repone si aún quedan unidades en otro slot") {
        Modulo_InventANDobj::ResetNpcInv(TEST_NPC);
        auto& npc = Npclist[TEST_NPC];
        npc.Numero = 600;
        npc.Invent.NroItems = 2;
        npc.Invent.Object[1].ObjIndex = 77;
        npc.Invent.Object[1].Amount = 5;
        npc.Invent.Object[2].ObjIndex = 77; // Mismo ítem en slot 2
        npc.Invent.Object[2].Amount = 10;

        ObjDataList[77].Crucial = 1;

        // Vaciamos el slot 1
        Modulo_InventANDobj::QuitarNpcInvItem(TEST_NPC, 1, 5);

        // Al quedar unidades en el slot 2, el slot 1 queda purgado y no se duplica
        CHECK(npc.Invent.Object[1].ObjIndex == 0);
        CHECK(npc.Invent.Object[1].Amount == 0);
        CHECK(npc.Invent.NroItems == 1);
        CHECK(npc.Invent.Object[2].ObjIndex == 77);
        CHECK(npc.Invent.Object[2].Amount == 10);
    }

    SUBCASE("CargarInvent y reposición automática al vaciarse totalmente el inventario") {
        Modulo_InventANDobj::ResetNpcInv(TEST_NPC);
        auto& npc = Npclist[TEST_NPC];
        npc.Numero = 700;
        npc.InvReSpawn = 0;

        Modulo_InventANDobj::SetNPCTemplateLookupHook([](std::int16_t numero) -> std::vector<Modulo_InventANDobj::NPCInventorySlot> {
            if (numero == 700) {
                return {
                    {101, 10},
                    {102, 20},
                    {103, 30}
                };
            }
            return {};
        });

        // 1. Cargar inventario inicial
        Modulo_InventANDobj::CargarInvent(TEST_NPC);

        CHECK(npc.Invent.NroItems == 3);
        CHECK(npc.Invent.Object[1].ObjIndex == 101);
        CHECK(npc.Invent.Object[1].Amount == 10);
        CHECK(npc.Invent.Object[2].ObjIndex == 102);
        CHECK(npc.Invent.Object[2].Amount == 20);
        CHECK(npc.Invent.Object[3].ObjIndex == 103);
        CHECK(npc.Invent.Object[3].Amount == 30);
        CHECK(npc.Invent.Object[4].ObjIndex == 0);

        // 2. Vaciamos todos los ítems (no cruciales)
        ObjDataList[101].Crucial = 0;
        ObjDataList[102].Crucial = 0;
        ObjDataList[103].Crucial = 0;

        Modulo_InventANDobj::QuitarNpcInvItem(TEST_NPC, 1, 10);
        CHECK(npc.Invent.NroItems == 2);

        Modulo_InventANDobj::QuitarNpcInvItem(TEST_NPC, 2, 20);
        CHECK(npc.Invent.NroItems == 1);

        // Al vaciar el último ítem, como InvReSpawn != 1, se dispara CargarInvent automáticamente
        Modulo_InventANDobj::QuitarNpcInvItem(TEST_NPC, 3, 30);

        CHECK(npc.Invent.NroItems == 3);
        CHECK(npc.Invent.Object[1].ObjIndex == 101);
        CHECK(npc.Invent.Object[1].Amount == 10);
        CHECK(npc.Invent.Object[2].ObjIndex == 102);
        CHECK(npc.Invent.Object[2].Amount == 20);
        CHECK(npc.Invent.Object[3].ObjIndex == 103);
        CHECK(npc.Invent.Object[3].Amount == 30);
    }

    SUBCASE("InvReSpawn = 1 previene la recarga automática al vaciarse el inventario") {
        Modulo_InventANDobj::ResetNpcInv(TEST_NPC);
        auto& npc = Npclist[TEST_NPC];
        npc.Numero = 700;
        npc.InvReSpawn = 1; // Bandera que bloquea respawn de inventario
        npc.Invent.NroItems = 1;
        npc.Invent.Object[1].ObjIndex = 101;
        npc.Invent.Object[1].Amount = 10;

        ObjDataList[101].Crucial = 0;

        Modulo_InventANDobj::QuitarNpcInvItem(TEST_NPC, 1, 10);

        // Como InvReSpawn == 1, no se invoca CargarInvent y queda vacío
        CHECK(npc.Invent.NroItems == 0);
        CHECK(npc.Invent.Object[1].ObjIndex == 0);
    }
}

} // TEST_SUITE G1

TEST_SUITE("Modulo_InventANDobj - G2") {

struct DroppedItemRecord {
    WorldPos pos{};
    Obj obj{};
    bool not_pirata{true};
};

TEST_CASE("G2: Fragmentación de Oro (TirarOroNpc)") {
    const WorldPos target_pos{ .Map = 1, .X = 50, .Y = 50 };
    std::vector<DroppedItemRecord> drops_interceptados;

    Modulo_InventANDobj::SetTirarItemAlPisoHook([&](const WorldPos& pos, const Obj& obj, bool not_pirata) -> WorldPos {
        drops_interceptados.push_back({ pos, obj, not_pirata });
        return pos;
    });

    SUBCASE("Monto menor a 10.000: 1 única pila de oro") {
        drops_interceptados.clear();
        Modulo_InventANDobj::TirarOroNpc(500, target_pos);

        REQUIRE(drops_interceptados.size() == 1);
        CHECK(drops_interceptados[0].obj.ObjIndex == iORO);
        CHECK(drops_interceptados[0].obj.Amount == 500);
        CHECK(drops_interceptados[0].pos.Map == 1);
        CHECK(drops_interceptados[0].pos.X == 50);
        CHECK(drops_interceptados[0].pos.Y == 50);
    }

    SUBCASE("Monto exacto a 10.000: 1 única pila de oro máxima") {
        drops_interceptados.clear();
        Modulo_InventANDobj::TirarOroNpc(10000, target_pos);

        REQUIRE(drops_interceptados.size() == 1);
        CHECK(drops_interceptados[0].obj.ObjIndex == iORO);
        CHECK(drops_interceptados[0].obj.Amount == 10000);
    }

    SUBCASE("Monto mayor a 10.000 (25.000): 3 pilas fragmentadas (10k, 10k, 5k)") {
        drops_interceptados.clear();
        Modulo_InventANDobj::TirarOroNpc(25000, target_pos);

        REQUIRE(drops_interceptados.size() == 3);
        CHECK(drops_interceptados[0].obj.ObjIndex == iORO);
        CHECK(drops_interceptados[0].obj.Amount == 10000);

        CHECK(drops_interceptados[1].obj.ObjIndex == iORO);
        CHECK(drops_interceptados[1].obj.Amount == 10000);

        CHECK(drops_interceptados[2].obj.ObjIndex == iORO);
        CHECK(drops_interceptados[2].obj.Amount == 5000);
    }

    SUBCASE("Montos inválidos (0 o negativos): cero llamadas") {
        drops_interceptados.clear();
        Modulo_InventANDobj::TirarOroNpc(0, target_pos);
        Modulo_InventANDobj::TirarOroNpc(-500, target_pos);

        CHECK(drops_interceptados.empty());
    }
}

TEST_CASE("G2: Drops Probabilísticos de Criaturas (NPC_TIRAR_ITEMS)") {
    npc current_npc{};
    current_npc.Pos = WorldPos{ .Map = 1, .X = 20, .Y = 30 };
    current_npc.Drop[1] = tDrops{ .ObjIndex = 50, .Amount = 1 };
    current_npc.Drop[2] = tDrops{ .ObjIndex = 60, .Amount = 2 };
    current_npc.Drop[3] = tDrops{ .ObjIndex = 70, .Amount = 3 };
    current_npc.Drop[4] = tDrops{ .ObjIndex = 80, .Amount = 4 };
    current_npc.Drop[5] = tDrops{ .ObjIndex = 90, .Amount = 5 };

    std::vector<DroppedItemRecord> drops_interceptados;
    Modulo_InventANDobj::SetTirarItemAlPisoHook([&](const WorldPos& pos, const Obj& obj, bool not_pirata) -> WorldPos {
        drops_interceptados.push_back({ pos, obj, not_pirata });
        return pos;
    });

    std::deque<std::int32_t> rng_queue;
    Modulo_InventANDobj::SetRandomGeneratorHook([&](std::int32_t /*min*/, std::int32_t /*max*/) -> std::int32_t {
        if (!rng_queue.empty()) {
            std::int32_t val = rng_queue.front();
            rng_queue.pop_front();
            return val;
        }
        return 99; // Fallback: drop nulo
    });

    SUBCASE("Criatura regular: Roll > 90 (91) produce drop nulo") {
        drops_interceptados.clear();
        rng_queue = { 91 };

        Modulo_InventANDobj::NPC_TIRAR_ITEMS(current_npc, false);

        CHECK(drops_interceptados.empty());
    }

    SUBCASE("Criatura regular: Roll 11..90 (50) selecciona Drop[1]") {
        drops_interceptados.clear();
        rng_queue = { 50 };

        Modulo_InventANDobj::NPC_TIRAR_ITEMS(current_npc, false);

        REQUIRE(drops_interceptados.size() == 1);
        CHECK(drops_interceptados[0].obj.ObjIndex == 50);
        CHECK(drops_interceptados[0].obj.Amount == 1);
    }

    SUBCASE("Criatura regular: Roll <= 10 (5) seguido de tirada fallida (50) selecciona Drop[2]") {
        drops_interceptados.clear();
        rng_queue = { 5, 50 };

        Modulo_InventANDobj::NPC_TIRAR_ITEMS(current_npc, false);

        REQUIRE(drops_interceptados.size() == 1);
        CHECK(drops_interceptados[0].obj.ObjIndex == 60);
        CHECK(drops_interceptados[0].obj.Amount == 2);
    }

    SUBCASE("Criatura regular: Cascada a Drop[3]") {
        drops_interceptados.clear();
        rng_queue = { 10, 8, 50 }; // Primer roll <=10 (pasa a 2), segundo <=10 (pasa a 3), tercero >10 (falla)

        Modulo_InventANDobj::NPC_TIRAR_ITEMS(current_npc, false);

        REQUIRE(drops_interceptados.size() == 1);
        CHECK(drops_interceptados[0].obj.ObjIndex == 70);
        CHECK(drops_interceptados[0].obj.Amount == 3);
    }

    SUBCASE("Criatura regular: Cascada a Drop[4]") {
        drops_interceptados.clear();
        rng_queue = { 10, 5, 3, 50 }; // Pasa a 2, a 3, a 4, falla

        Modulo_InventANDobj::NPC_TIRAR_ITEMS(current_npc, false);

        REQUIRE(drops_interceptados.size() == 1);
        CHECK(drops_interceptados[0].obj.ObjIndex == 80);
        CHECK(drops_interceptados[0].obj.Amount == 4);
    }

    SUBCASE("Criatura regular: Cascada a Drop[5] (Legendario)") {
        drops_interceptados.clear();
        rng_queue = { 1, 1, 1, 1 }; // 4 rolls consecutivos <= 10 -> alcanza Drop[5]

        Modulo_InventANDobj::NPC_TIRAR_ITEMS(current_npc, false);

        REQUIRE(drops_interceptados.size() == 1);
        CHECK(drops_interceptados[0].obj.ObjIndex == 90);
        CHECK(drops_interceptados[0].obj.Amount == 5);
    }

    SUBCASE("Criatura regular: Drop de tipo oro (ObjIndex = iORO = 12) convoca TirarOroNpc") {
        drops_interceptados.clear();
        current_npc.Drop[1] = tDrops{ .ObjIndex = iORO, .Amount = 15000 };
        rng_queue = { 50 }; // Selecciona Drop[1]

        Modulo_InventANDobj::NPC_TIRAR_ITEMS(current_npc, false);

        // 15.000 de oro fragmentado en 10.000 + 5.000
        REQUIRE(drops_interceptados.size() == 2);
        CHECK(drops_interceptados[0].obj.ObjIndex == iORO);
        CHECK(drops_interceptados[0].obj.Amount == 10000);
        CHECK(drops_interceptados[1].obj.ObjIndex == iORO);
        CHECK(drops_interceptados[1].obj.Amount == 5000);
    }
}

TEST_CASE("G2: Rama Pretoriana y Replicación del Bug #27") {
    npc current_npc{};
    current_npc.Pos = WorldPos{ .Map = 2, .X = 10, .Y = 15 };
    current_npc.GiveGLD = 12000;
    current_npc.Invent.NroItems = 2;
    current_npc.Invent.Object[1] = UserOBJ{ .ObjIndex = 100, .Amount = 1, .Equipped = 0 };
    current_npc.Invent.Object[5] = UserOBJ{ .ObjIndex = 200, .Amount = 5, .Equipped = 0 };

    // Matriz de drops regulares
    current_npc.Drop[1] = tDrops{ .ObjIndex = 300, .Amount = 1 };

    std::vector<DroppedItemRecord> drops_interceptados;
    Modulo_InventANDobj::SetTirarItemAlPisoHook([&](const WorldPos& pos, const Obj& obj, bool not_pirata) -> WorldPos {
        drops_interceptados.push_back({ pos, obj, not_pirata });
        return pos;
    });

    SUBCASE("Pretoriano: Arroja todo el inventario ocupado y su GiveGLD fragmentado") {
        drops_interceptados.clear();

        Modulo_InventANDobj::NPC_TIRAR_ITEMS(current_npc, true);

        // Debe arrojar:
        // 1. Objeto 100 (cant 1)
        // 2. Objeto 200 (cant 5)
        // 3. Oro 10.000 (GiveGLD fragmentado)
        // 4. Oro 2.000 (GiveGLD fragmentado restante)
        REQUIRE(drops_interceptados.size() == 4);

        CHECK(drops_interceptados[0].obj.ObjIndex == 100);
        CHECK(drops_interceptados[0].obj.Amount == 1);

        CHECK(drops_interceptados[1].obj.ObjIndex == 200);
        CHECK(drops_interceptados[1].obj.Amount == 5);

        CHECK(drops_interceptados[2].obj.ObjIndex == iORO);
        CHECK(drops_interceptados[2].obj.Amount == 10000);

        CHECK(drops_interceptados[3].obj.ObjIndex == iORO);
        CHECK(drops_interceptados[3].obj.Amount == 2000);
    }

    SUBCASE("Bug #27: Criatura regular con GiveGLD > 0 NUNCA arroja dicho oro") {
        drops_interceptados.clear();
        Modulo_InventANDobj::SetRandomGeneratorHook([](std::int32_t, std::int32_t) -> std::int32_t {
            return 50; // Selecciona Drop[1] (Obj 300)
        });

        // is_pretoriano = false
        Modulo_InventANDobj::NPC_TIRAR_ITEMS(current_npc, false);

        // Solo debe arrojar Drop[1] (Obj 300). Los 12.000 de GiveGLD son ignorados por completo.
        REQUIRE(drops_interceptados.size() == 1);
        CHECK(drops_interceptados[0].obj.ObjIndex == 300);
        CHECK(drops_interceptados[0].obj.Amount == 1);
    }
}

} // TEST_SUITE G2

TEST_SUITE("Modulo_InventANDobj - G3") {

struct MakeObjRecord {
    Obj obj{};
    std::int16_t map{0};
    std::int16_t x{0};
    std::int16_t y{0};
};

struct TilelibreCallRecord {
    WorldPos pos{};
    Obj obj{};
    bool agua{false};
    bool tierra{false};
};

TEST_CASE("G3: Despacho Espacial y Puntos de Extensión (TirarItemAlPiso)") {
    // Desactivar cualquier hook de alto nivel de TirarItemAlPiso para evaluar la lógica interna de G3
    Modulo_InventANDobj::SetTirarItemAlPisoHook(nullptr);

    const WorldPos origin_pos{ .Map = 1, .X = 50, .Y = 50 };
    const Obj sample_obj{ .ObjIndex = 25, .Amount = 10 };

    std::vector<MakeObjRecord> make_obj_records;
    Modulo_InventANDobj::SetMakeObjHook([&](const Obj& obj, std::int16_t map, std::int16_t x, std::int16_t y) {
        make_obj_records.push_back({ obj, map, x, y });
    });

    std::vector<TilelibreCallRecord> tilelibre_records;

    SUBCASE("Despacho exitoso: Tilelibre encuentra celda libre y MakeObj materializa el ítem") {
        make_obj_records.clear();
        tilelibre_records.clear();

        Modulo_InventANDobj::SetTilelibreHook([&](const WorldPos& pos, WorldPos& n_pos, const Obj& obj, bool agua, bool tierra) {
            tilelibre_records.push_back({ pos, obj, agua, tierra });
            n_pos = WorldPos{ .Map = pos.Map, .X = 51, .Y = 50 };
        });

        const WorldPos result_pos = Modulo_InventANDobj::TirarItemAlPiso(origin_pos, sample_obj);

        // Verificación de retorno
        CHECK(result_pos.Map == 1);
        CHECK(result_pos.X == 51);
        CHECK(result_pos.Y == 50);

        // Verificación de Tilelibre
        REQUIRE(tilelibre_records.size() == 1);
        CHECK(tilelibre_records[0].pos.Map == 1);
        CHECK(tilelibre_records[0].pos.X == 50);
        CHECK(tilelibre_records[0].pos.Y == 50);
        CHECK(tilelibre_records[0].obj.ObjIndex == 25);
        CHECK(tilelibre_records[0].obj.Amount == 10);
        CHECK(tilelibre_records[0].agua == true);  // not_pirata = true por defecto
        CHECK(tilelibre_records[0].tierra == true);

        // Verificación de MakeObj
        REQUIRE(make_obj_records.size() == 1);
        CHECK(make_obj_records[0].obj.ObjIndex == 25);
        CHECK(make_obj_records[0].obj.Amount == 10);
        CHECK(make_obj_records[0].map == 1);
        CHECK(make_obj_records[0].x == 51);
        CHECK(make_obj_records[0].y == 50);
    }

    SUBCASE("Propagación de flag not_pirata hacia Tilelibre (agua y tierra)") {
        make_obj_records.clear();
        tilelibre_records.clear();

        Modulo_InventANDobj::SetTilelibreHook([&](const WorldPos& pos, WorldPos& n_pos, const Obj& obj, bool agua, bool tierra) {
            tilelibre_records.push_back({ pos, obj, agua, tierra });
            n_pos = WorldPos{ .Map = pos.Map, .X = 52, .Y = 52 };
        });

        // Caso 1: not_pirata = true -> agua = true, tierra = true
        Modulo_InventANDobj::TirarItemAlPiso(origin_pos, sample_obj, true);
        REQUIRE(tilelibre_records.size() == 1);
        CHECK(tilelibre_records[0].agua == true);
        CHECK(tilelibre_records[0].tierra == true);

        // Caso 2: not_pirata = false -> agua = false, tierra = true
        Modulo_InventANDobj::TirarItemAlPiso(origin_pos, sample_obj, false);
        REQUIRE(tilelibre_records.size() == 2);
        CHECK(tilelibre_records[1].agua == false);
        CHECK(tilelibre_records[1].tierra == true);
    }

    SUBCASE("Replicación del Bug #28: Tilelibre sin tile libre (0, 0) omite MakeObj y retorna silencioso") {
        make_obj_records.clear();
        tilelibre_records.clear();

        Modulo_InventANDobj::SetTilelibreHook([&](const WorldPos& pos, WorldPos& n_pos, const Obj& obj, bool agua, bool tierra) {
            tilelibre_records.push_back({ pos, obj, agua, tierra });
            n_pos = WorldPos{ .Map = pos.Map, .X = 0, .Y = 0 }; // Sin espacio en el mapa
        });

        const WorldPos result_pos = Modulo_InventANDobj::TirarItemAlPiso(origin_pos, sample_obj);

        // Retorna (0, 0)
        CHECK(result_pos.Map == origin_pos.Map);
        CHECK(result_pos.X == 0);
        CHECK(result_pos.Y == 0);

        // Tilelibre fue consultado, pero MakeObj NUNCA debe ser convocado
        CHECK(tilelibre_records.size() == 1);
        CHECK(make_obj_records.empty());
    }

    SUBCASE("Saturación espacial integrada con TirarOroNpc") {
        make_obj_records.clear();
        tilelibre_records.clear();

        // En mapa completamente saturado, Tilelibre devuelve (0, 0)
        Modulo_InventANDobj::SetTilelibreHook([&](const WorldPos& pos, WorldPos& n_pos, const Obj& obj, bool agua, bool tierra) {
            tilelibre_records.push_back({ pos, obj, agua, tierra });
            n_pos = WorldPos{ .Map = pos.Map, .X = 0, .Y = 0 };
        });

        // Tiramos 25.000 de oro -> debe fraccionar en 10.000, 10.000 y 5.000
        Modulo_InventANDobj::TirarOroNpc(25000, origin_pos);

        // Debe completar las 3 iteraciones sin bucle infinito
        CHECK(tilelibre_records.size() == 3);
        // Ningún objeto se materializó físicamente debido a la saturación (Bug #28)
        CHECK(make_obj_records.empty());
    }

    SUBCASE("Bypass hook TirarItemAlPisoHook toma precedencia") {
        make_obj_records.clear();
        tilelibre_records.clear();

        Modulo_InventANDobj::SetTilelibreHook([&](const WorldPos& pos, WorldPos& n_pos, const Obj& obj, bool agua, bool tierra) {
            tilelibre_records.push_back({ pos, obj, agua, tierra });
            n_pos = WorldPos{ .Map = pos.Map, .X = 99, .Y = 99 };
        });

        bool bypass_llamado = false;
        Modulo_InventANDobj::SetTirarItemAlPisoHook([&](const WorldPos& pos, const Obj&, bool) -> WorldPos {
            bypass_llamado = true;
            return WorldPos{ .Map = pos.Map, .X = 11, .Y = 11 };
        });

        const WorldPos res = Modulo_InventANDobj::TirarItemAlPiso(origin_pos, sample_obj);

        CHECK(bypass_llamado);
        CHECK(res.X == 11);
        CHECK(res.Y == 11);
        CHECK(tilelibre_records.empty());
        CHECK(make_obj_records.empty());

        // Limpiar bypass para no afectar subsiguientes tests
        Modulo_InventANDobj::SetTirarItemAlPisoHook(nullptr);
    }
}

} // TEST_SUITE G3

