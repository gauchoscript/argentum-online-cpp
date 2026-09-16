#include <doctest/doctest.h>
#include <queue>
#include "server/PathFinding.hpp"
#include "server/Declares.hpp"

TEST_SUITE("PathFinding - Fase 1: Validaciones y Tránsito") {
    TEST_CASE("Limites - Coordenadas dentro y fuera de la grilla 1..100") {
        CHECK(Limites(1, 1));
        CHECK(Limites(100, 100));
        CHECK(Limites(50, 50));

        CHECK_FALSE(Limites(0, 10));
        CHECK_FALSE(Limites(10, 0));
        CHECK_FALSE(Limites(101, 50));
        CHECK_FALSE(Limites(50, 101));
        CHECK_FALSE(Limites(-1, 50));
        CHECK_FALSE(Limites(50, -1));
    }

    TEST_CASE("IsWalkable - Reglas de transitabilidad sobre MapData") {
        const std::int16_t map = 1;
        const std::size_t reqSize = (static_cast<std::size_t>(map) + 1) * 101 * 101;
        if (MapData.size() < reqSize) {
            MapData.resize(reqSize);
        }

        const std::int16_t npc_index = 1;
        Npclist[npc_index].PFINFO.TargetUser = 5;

        auto get_idx = [](std::int16_t m, std::int16_t r, std::int16_t c) {
            return (static_cast<std::size_t>(m) * 101 + static_cast<std::size_t>(r)) * 101 + static_cast<std::size_t>(c);
        };

        // Celda libre
        std::int16_t r1 = 10, c1 = 10;
        MapData[get_idx(map, r1, c1)] = MapBlock{};
        CHECK(IsWalkable(map, r1, c1, npc_index));

        // Celda bloqueada estáticamente
        std::int16_t r2 = 10, c2 = 11;
        MapData[get_idx(map, r2, c2)] = MapBlock{};
        MapData[get_idx(map, r2, c2)].Blocked = 1;
        CHECK_FALSE(IsWalkable(map, r2, c2, npc_index));

        // Celda ocupada por otro NPC
        std::int16_t r3 = 10, c3 = 12;
        MapData[get_idx(map, r3, c3)] = MapBlock{};
        MapData[get_idx(map, r3, c3)].NpcIndex = 2;
        CHECK_FALSE(IsWalkable(map, r3, c3, npc_index));

        // Celda ocupada por un usuario cualquiera (distinto al TargetUser = 5)
        std::int16_t r4 = 10, c4 = 13;
        MapData[get_idx(map, r4, c4)] = MapBlock{};
        MapData[get_idx(map, r4, c4)].UserIndex = 99;
        CHECK_FALSE(IsWalkable(map, r4, c4, npc_index));

        // Celda ocupada por el TargetUser asignado al NPC (TargetUser = 5)
        std::int16_t r5 = 10, c5 = 14;
        MapData[get_idx(map, r5, c5)] = MapBlock{};
        MapData[get_idx(map, r5, c5)].UserIndex = 5;
        CHECK(IsWalkable(map, r5, c5, npc_index));
    }
}

TEST_SUITE("PathFinding - Fase 2: Inicialización y Adyacencias Deterministas") {
    TEST_CASE("InitializeTable - Origen a 0, vecinos a 1000 y Replicación del Bug #44") {
        tIntermidiateWork T[101][101];
        tVertice origin{ .X = 50, .Y = 50 };

        // Ensucia deliberadamente una celda fuera del subcuadrante de max_steps=10 (ej. Y = 50 + 10 + 5 = 65)
        T[65][50].DistV = 42;
        T[65][50].Known = true;

        InitializeTable(T, origin, 10);

        // Origen debe tener DistV = 0
        CHECK_EQ(T[50][50].DistV, 0);
        CHECK_FALSE(T[50][50].Known);

        // Celdas vecinas dentro del subcuadrante deben haber sido reseteadas a 1000
        CHECK_EQ(T[49][50].DistV, 1000);
        CHECK_EQ(T[51][50].DistV, 1000);
        CHECK_EQ(T[50][49].DistV, 1000);
        CHECK_EQ(T[50][51].DistV, 1000);

        // Bug #44: La celda fuera de la sub-grilla conserva su valor sucio 42 intacto
        CHECK_EQ(T[65][50].DistV, 42);
        CHECK(T[65][50].Known);
    }

    TEST_CASE("ProcessAdjacents - Orden determinista N-S-O-E e incremento de distancias") {
        const std::int16_t map = 1;
        const std::size_t reqSize = (static_cast<std::size_t>(map) + 1) * 101 * 101;
        if (MapData.size() < reqSize) {
            MapData.resize(reqSize);
        }

        // Limpia el área del mapa alrededor de (50, 50)
        auto get_idx = [](std::int16_t m, std::int16_t r, std::int16_t c) {
            return (static_cast<std::size_t>(m) * 101 + static_cast<std::size_t>(r)) * 101 + static_cast<std::size_t>(c);
        };
        for (std::int16_t r = 48; r <= 52; ++r) {
            for (std::int16_t c = 48; c <= 52; ++c) {
                MapData[get_idx(map, r, c)] = MapBlock{};
            }
        }

        tIntermidiateWork T[101][101];
        tVertice origin{ .X = 50, .Y = 50 };
        InitializeTable(T, origin, 10);

        std::queue<tVertice> queue;
        std::int16_t vfila = 50;
        std::int16_t vcolu = 50;
        std::int16_t npc_index = 1;

        ProcessAdjacents(map, T, vfila, vcolu, npc_index, queue);

        // Se deben haber encolado 4 vecinos
        REQUIRE_EQ(queue.size(), 4);

        // 1. Norte: (vfila - 1, vcolu) = (49, 50) -> Vertice{ X=50, Y=49 }
        tVertice n1 = queue.front(); queue.pop();
        CHECK_EQ(n1.X, 50);
        CHECK_EQ(n1.Y, 49);
        CHECK_EQ(T[49][50].DistV, 1);
        CHECK_EQ(T[49][50].PrevV.X, 50);
        CHECK_EQ(T[49][50].PrevV.Y, 50);

        // 2. Sur: (vfila + 1, vcolu) = (51, 50) -> Vertice{ X=50, Y=51 }
        tVertice n2 = queue.front(); queue.pop();
        CHECK_EQ(n2.X, 50);
        CHECK_EQ(n2.Y, 51);
        CHECK_EQ(T[51][50].DistV, 1);
        CHECK_EQ(T[51][50].PrevV.X, 50);
        CHECK_EQ(T[51][50].PrevV.Y, 50);

        // 3. Oeste: (vfila, vcolu - 1) = (50, 49) -> Vertice{ X=49, Y=50 }
        tVertice n3 = queue.front(); queue.pop();
        CHECK_EQ(n3.X, 49);
        CHECK_EQ(n3.Y, 50);
        CHECK_EQ(T[50][49].DistV, 1);
        CHECK_EQ(T[50][49].PrevV.X, 50);
        CHECK_EQ(T[50][49].PrevV.Y, 50);

        // 4. Este: (vfila, vcolu + 1) = (50, 51) -> Vertice{ X=51, Y=50 }
        tVertice n4 = queue.front(); queue.pop();
        CHECK_EQ(n4.X, 51);
        CHECK_EQ(n4.Y, 50);
        CHECK_EQ(T[50][51].DistV, 1);
        CHECK_EQ(T[50][51].PrevV.X, 50);
        CHECK_EQ(T[50][51].PrevV.Y, 50);
    }

    TEST_CASE("ProcessAdjacents - Filtrado de obstáculos") {
        const std::int16_t map = 1;
        auto get_idx = [](std::int16_t m, std::int16_t r, std::int16_t c) {
            return (static_cast<std::size_t>(m) * 101 + static_cast<std::size_t>(r)) * 101 + static_cast<std::size_t>(c);
        };
        for (std::int16_t r = 48; r <= 52; ++r) {
            for (std::int16_t c = 48; c <= 52; ++c) {
                MapData[get_idx(map, r, c)] = MapBlock{};
            }
        }

        // Bloquea el Norte (vfila - 1 = 49, vcolu = 50)
        MapData[get_idx(map, 49, 50)].Blocked = 1;

        tIntermidiateWork T[101][101];
        tVertice origin{ .X = 50, .Y = 50 };
        InitializeTable(T, origin, 10);

        std::queue<tVertice> queue;
        ProcessAdjacents(map, T, 50, 50, 1, queue);

        // Solo 3 vecinos encolados (Norte fue filtrado)
        REQUIRE_EQ(queue.size(), 3);

        // Primer elemento encolado debe ser Sur (50, 51)
        tVertice first = queue.front();
        CHECK_EQ(first.X, 50);
        CHECK_EQ(first.Y, 51);
    }
}

TEST_SUITE("PathFinding - Fase 3: Búsqueda y Reconstrucción") {
    auto setup_map = [](std::int16_t map) {
        const std::size_t reqSize = (static_cast<std::size_t>(map) + 1) * 101 * 101;
        if (MapData.size() < reqSize) {
            MapData.resize(reqSize);
        }
        auto get_idx = [](std::int16_t m, std::int16_t r, std::int16_t c) {
            return (static_cast<std::size_t>(m) * 101 + static_cast<std::size_t>(r)) * 101 + static_cast<std::size_t>(c);
        };
        for (std::int16_t r = 1; r <= 100; ++r) {
            for (std::int16_t c = 1; c <= 100; ++c) {
                MapData[get_idx(map, r, c)] = MapBlock{};
            }
        }
    };

    TEST_CASE("SeekPath - Camino Directo en línea recta") {
        const std::int16_t map = 1;
        setup_map(map);

        const std::int16_t npc_index = 1;
        Npclist[npc_index].Pos = WorldPos{ .Map = map, .X = 50, .Y = 50 };
        // Target en VB6 se setea invirtiendo coordenadas en AI_NPC.bas: Target.X = TargetUser.Pos.Y, Target.Y = TargetUser.Pos.X
        // Para perseguir al objetivo en el mundo (53, 50): Target.X = 50 (Y-world), Target.Y = 53 (X-world)
        Npclist[npc_index].PFINFO.Target.X = 50;
        Npclist[npc_index].PFINFO.Target.Y = 53;

        SeekPath(npc_index);

        CHECK_FALSE(Npclist[npc_index].PFINFO.NoPath);
        CHECK_EQ(Npclist[npc_index].PFINFO.PathLenght, 3);
        CHECK_EQ(Npclist[npc_index].PFINFO.CurPos, 1);

        // Replicación Bug #45: las coordenadas en Path son del mundo (X=51, Y=50), (X=52, Y=50), (X=53, Y=50)
        REQUIRE_GE(Npclist[npc_index].PFINFO.Path.size(), 4);
        CHECK_EQ(Npclist[npc_index].PFINFO.Path[1].X, 51);
        CHECK_EQ(Npclist[npc_index].PFINFO.Path[1].Y, 50);

        CHECK_EQ(Npclist[npc_index].PFINFO.Path[2].X, 52);
        CHECK_EQ(Npclist[npc_index].PFINFO.Path[2].Y, 50);

        CHECK_EQ(Npclist[npc_index].PFINFO.Path[3].X, 53);
        CHECK_EQ(Npclist[npc_index].PFINFO.Path[3].Y, 50);
    }

    TEST_CASE("SeekPath - Evasión de Obstáculo Estático") {
        const std::int16_t map = 1;
        setup_map(map);

        auto get_idx = [](std::int16_t m, std::int16_t r, std::int16_t c) {
            return (static_cast<std::size_t>(m) * 101 + static_cast<std::size_t>(r)) * 101 + static_cast<std::size_t>(c);
        };

        const std::int16_t npc_index = 1;
        Npclist[npc_index].Pos = WorldPos{ .Map = map, .X = 50, .Y = 50 };
        // Target en mundo (52, 50) -> Target.X = 50 (Y-world), Target.Y = 52 (X-world)
        Npclist[npc_index].PFINFO.Target.X = 50;
        Npclist[npc_index].PFINFO.Target.Y = 52;

        // Bloquea el camino directo en el mundo (51, 50)
        MapData[get_idx(map, 51, 50)].Blocked = 1;

        SeekPath(npc_index);

        // Debe bordear el bloqueo y hallar una ruta válida (> 2 pasos)
        CHECK_FALSE(Npclist[npc_index].PFINFO.NoPath);
        CHECK_GT(Npclist[npc_index].PFINFO.PathLenght, 2);
    }

    TEST_CASE("SeekPath - Objetivo Inalcanzable") {
        const std::int16_t map = 1;
        setup_map(map);

        auto get_idx = [](std::int16_t m, std::int16_t r, std::int16_t c) {
            return (static_cast<std::size_t>(m) * 101 + static_cast<std::size_t>(r)) * 101 + static_cast<std::size_t>(c);
        };

        const std::int16_t npc_index = 1;
        Npclist[npc_index].Pos = WorldPos{ .Map = map, .X = 50, .Y = 50 };
        // Target en mundo (55, 50) -> Target.X = 50, Target.Y = 55
        Npclist[npc_index].PFINFO.Target.X = 50;
        Npclist[npc_index].PFINFO.Target.Y = 55;

        // Encierra el objetivo (55, 50) con bloqueos en las 4 direcciones del mundo
        MapData[get_idx(map, 54, 50)].Blocked = 1;
        MapData[get_idx(map, 56, 50)].Blocked = 1;
        MapData[get_idx(map, 55, 49)].Blocked = 1;
        MapData[get_idx(map, 55, 51)].Blocked = 1;

        SeekPath(npc_index);

        CHECK(Npclist[npc_index].PFINFO.NoPath);
        CHECK_EQ(Npclist[npc_index].PFINFO.PathLenght, 0);
    }

    TEST_CASE("SeekPath - Replicación del Bug #43: steps no incrementado") {
        const std::int16_t map = 1;
        setup_map(map);

        const std::int16_t npc_index = 1;
        Npclist[npc_index].Pos = WorldPos{ .Map = map, .X = 50, .Y = 50 };
        // Target en mundo (55, 50) -> Target.X = 50, Target.Y = 55
        Npclist[npc_index].PFINFO.Target.X = 50;
        Npclist[npc_index].PFINFO.Target.Y = 55;

        // Invocación pasando MaxSteps = 2 cuando la distancia requerida es 5
        SeekPath(npc_index, 2);

        // Debido al Bug #43 (steps permanece en 0 y no incrementa), el bucle BFS no se corta prematuramente por MaxSteps
        CHECK_FALSE(Npclist[npc_index].PFINFO.NoPath);
        CHECK_EQ(Npclist[npc_index].PFINFO.PathLenght, 5);
    }
}
