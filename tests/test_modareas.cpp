#include <doctest/doctest.h>
#include "server/ModAreas.hpp"
#include "server/Declares.hpp"
#include "server/Protocol.hpp"

TEST_SUITE("ModAreas") {

TEST_CASE("Fase 1: Tablas precalculadas de AreasRecive") {
    ModAreas::InitAreas();

    SUBCASE("Franja 0 (coords 1..8): bits 0 y 1 activos (valor 3)") {
        CHECK(ModAreas::GetAreaRecive(0) == 3);
        CHECK((ModAreas::GetAreaRecive(0) & (1 << 0)) != 0);
        CHECK((ModAreas::GetAreaRecive(0) & (1 << 1)) != 0);
        CHECK((ModAreas::GetAreaRecive(0) & (1 << 2)) == 0);
    }

    SUBCASE("Franja 1 (coords 9..17): bits 0, 1 y 2 activos (valor 7)") {
        CHECK(ModAreas::GetAreaRecive(1) == 7);
        CHECK((ModAreas::GetAreaRecive(1) & (1 << 0)) != 0);
        CHECK((ModAreas::GetAreaRecive(1) & (1 << 1)) != 0);
        CHECK((ModAreas::GetAreaRecive(1) & (1 << 2)) != 0);
        CHECK((ModAreas::GetAreaRecive(1) & (1 << 3)) == 0);
    }

    SUBCASE("Franja 5 (coords 45..53): bits 4, 5 y 6 activos (valor 112)") {
        // (1 << 4) | (1 << 5) | (1 << 6) = 16 + 32 + 64 = 112
        CHECK(ModAreas::GetAreaRecive(5) == 112);
        CHECK((ModAreas::GetAreaRecive(5) & (1 << 4)) != 0);
        CHECK((ModAreas::GetAreaRecive(5) & (1 << 5)) != 0);
        CHECK((ModAreas::GetAreaRecive(5) & (1 << 6)) != 0);
        CHECK((ModAreas::GetAreaRecive(5) & (1 << 3)) == 0);
        CHECK((ModAreas::GetAreaRecive(5) & (1 << 7)) == 0);
    }

    SUBCASE("Franja 11 (coords 99..100): bits 10 y 11 activos (valor 3072)") {
        // (1 << 10) | (1 << 11) = 1024 + 2048 = 3072
        CHECK(ModAreas::GetAreaRecive(11) == 3072);
        CHECK((ModAreas::GetAreaRecive(11) & (1 << 10)) != 0);
        CHECK((ModAreas::GetAreaRecive(11) & (1 << 11)) != 0);
        CHECK((ModAreas::GetAreaRecive(11) & (1 << 9)) == 0);
    }

    SUBCASE("Cotas fuera de rango retornan 0") {
        CHECK(ModAreas::GetAreaRecive(-1) == 0);
        CHECK(ModAreas::GetAreaRecive(12) == 0);
    }
}

TEST_CASE("Fase 1: Matriz precalculada de AreasInfo en esquinas y franjas") {
    ModAreas::InitAreas();

    SUBCASE("Esquina superior izquierda (1, 1): FranjaX=0, FranjaY=0 -> (0+1)*(0+1) = 1") {
        CHECK(ModAreas::GetAreaID(1, 1) == 1);
    }

    SUBCASE("Borde de Franja 0 (8, 8): FranjaX=0, FranjaY=0 -> (0+1)*(0+1) = 1") {
        CHECK(ModAreas::GetAreaID(8, 8) == 1);
        CHECK(ModAreas::GetAreaID(1, 8) == 1);
        CHECK(ModAreas::GetAreaID(8, 1) == 1);
    }

    SUBCASE("Inicio de Franja 1 (9, 9): FranjaX=1, FranjaY=1 -> (1+1)*(1+1) = 4") {
        CHECK(ModAreas::GetAreaID(9, 9) == 4);
    }

    SUBCASE("Esquina inferior derecha (100, 100): FranjaX=11, FranjaY=11 -> (11+1)*(11+1) = 144") {
        CHECK(ModAreas::GetAreaID(100, 100) == 144);
        CHECK(ModAreas::GetAreaID(99, 99) == 144);
        CHECK(ModAreas::GetAreaID(99, 100) == 144);
        CHECK(ModAreas::GetAreaID(100, 99) == 144);
    }

    SUBCASE("Coordenadas fuera de mapa retornan 0") {
        CHECK(ModAreas::GetAreaID(0, 0) == 0);
        CHECK(ModAreas::GetAreaID(0, 50) == 0);
        CHECK(ModAreas::GetAreaID(50, 0) == 0);
        CHECK(ModAreas::GetAreaID(101, 50) == 0);
        CHECK(ModAreas::GetAreaID(50, 101) == 0);
    }
}

TEST_CASE("Fase 1: Preservación estricta de colisiones históricas de AreaID (Bug #24)") {
    ModAreas::InitAreas();

    // Franja (1, 5) -> X en [9..17], Y en [45..53] -> (1+1)*(5+1) = 2*6 = 12
    const auto id1 = ModAreas::GetAreaID(15, 50);

    // Franja (2, 3) -> X en [18..26], Y en [27..35] -> (2+1)*(3+1) = 3*4 = 12
    const auto id2 = ModAreas::GetAreaID(20, 30);

    // Franja (5, 1) -> X en [45..53], Y en [9..17] -> (5+1)*(1+1) = 6*2 = 12
    const auto id3 = ModAreas::GetAreaID(50, 15);

    // Franja (0, 11) -> X en [1..8], Y en [99..100] -> (0+1)*(11+1) = 1*12 = 12
    const auto id4 = ModAreas::GetAreaID(5, 100);

    CHECK(id1 == 12);
    CHECK(id2 == 12);
    CHECK(id3 == 12);
    CHECK(id4 == 12);
    CHECK(id1 == id2);
    CHECK(id2 == id3);
    CHECK(id3 == id4);
}

TEST_CASE("Fase 1: Inicialización limpia y dimensionamiento de ConnGroups") {
    NumMaps = 3;
    ModAreas::InitAreas();

    CHECK(ConnGroups.size() == 4); // 0 (dummy), 1, 2, 3

    for (int map = 1; map <= 3; ++map) {
        CHECK(ConnGroups[map].CountEntrys == 0);
        CHECK(ConnGroups[map].OptValue == 1);
        CHECK(ConnGroups[map].UserEntrys.size() == 1);
        CHECK(ConnGroups[map].UserEntrys[0] == 0);
    }
}

TEST_CASE("Fase 1: Estructura AreaInfo y constante centinela USER_NUEVO") {
    CHECK(ModAreas::USER_NUEVO == 255);

    ModAreas::AreaInfo info{};
    info.MinX = -9;
    info.MinY = -9;
    info.AreaPerteneceX = 1;
    info.AreaPerteneceY = 1;
    info.AreaReciveX = 3;
    info.AreaReciveY = 3;
    info.AreaID = 1;

    // Constatar que MinX y MinY son enteros con signo que retienen valores negativos (Bug #25)
    CHECK(info.MinX == -9);
    CHECK(info.MinY == -9);
}

TEST_CASE("Fase 2: Gestión de membresía de mapas (AgregarUser y QuitarUser)") {
    NumMaps = 2;
    ModAreas::InitAreas();

    UserList.resize(10);
    for (std::size_t u = 0; u < UserList.size(); ++u) {
        UserList[u].AreasInfo = ModAreas::AreaInfo{};
    }

    SUBCASE("T2.1: Agregar usuarios secuencialmente en mapa vacío y verificar orden contiguo") {
        ModAreas::AgregarUser(1, 1);
        CHECK(ConnGroups[1].CountEntrys == 1);
        REQUIRE(ConnGroups[1].UserEntrys.size() == 2);
        CHECK(ConnGroups[1].UserEntrys[1] == 1);

        ModAreas::AgregarUser(2, 1);
        CHECK(ConnGroups[1].CountEntrys == 2);
        REQUIRE(ConnGroups[1].UserEntrys.size() == 3);
        CHECK(ConnGroups[1].UserEntrys[1] == 1);
        CHECK(ConnGroups[1].UserEntrys[2] == 2);

        ModAreas::AgregarUser(3, 1);
        CHECK(ConnGroups[1].CountEntrys == 3);
        REQUIRE(ConnGroups[1].UserEntrys.size() == 4);
        CHECK(ConnGroups[1].UserEntrys[1] == 1);
        CHECK(ConnGroups[1].UserEntrys[2] == 2);
        CHECK(ConnGroups[1].UserEntrys[3] == 3);
    }

    SUBCASE("T2.2: Detección e ignorado de usuarios duplicados") {
        ModAreas::AgregarUser(5, 1);
        CHECK(ConnGroups[1].CountEntrys == 1);
        CHECK(ConnGroups[1].UserEntrys[1] == 5);

        // Intento de reinsertar al mismo usuario
        ModAreas::AgregarUser(5, 1);
        CHECK(ConnGroups[1].CountEntrys == 1);
        REQUIRE(ConnGroups[1].UserEntrys.size() == 2);
        CHECK(ConnGroups[1].UserEntrys[1] == 5);
    }

    SUBCASE("T2.3: Remoción de elemento intermedio preservando corrimiento exacto a la izquierda") {
        ModAreas::AgregarUser(10, 1);
        ModAreas::AgregarUser(20, 1);
        ModAreas::AgregarUser(30, 1);

        CHECK(ConnGroups[1].CountEntrys == 3);
        CHECK(ConnGroups[1].UserEntrys[1] == 10);
        CHECK(ConnGroups[1].UserEntrys[2] == 20);
        CHECK(ConnGroups[1].UserEntrys[3] == 30);

        // Removemos el usuario intermedio (20)
        ModAreas::QuitarUser(20, 1);

        CHECK(ConnGroups[1].CountEntrys == 2);
        REQUIRE(ConnGroups[1].UserEntrys.size() == 3);
        CHECK(ConnGroups[1].UserEntrys[1] == 10);
        CHECK(ConnGroups[1].UserEntrys[2] == 30);
    }

    SUBCASE("T2.4: Remoción de extremos (primero y último elemento)") {
        ModAreas::AgregarUser(1, 1);
        ModAreas::AgregarUser(2, 1);
        ModAreas::AgregarUser(3, 1);

        // Remover primero (1)
        ModAreas::QuitarUser(1, 1);
        CHECK(ConnGroups[1].CountEntrys == 2);
        CHECK(ConnGroups[1].UserEntrys[1] == 2);
        CHECK(ConnGroups[1].UserEntrys[2] == 3);

        // Remover último actual (3)
        ModAreas::QuitarUser(3, 1);
        CHECK(ConnGroups[1].CountEntrys == 1);
        CHECK(ConnGroups[1].UserEntrys[1] == 2);

        // Remover único restante (2)
        ModAreas::QuitarUser(2, 1);
        CHECK(ConnGroups[1].CountEntrys == 0);
        CHECK(ConnGroups[1].UserEntrys.size() == 1);
        CHECK(ConnGroups[1].UserEntrys[0] == 0);
    }

    SUBCASE("T2.5: Remoción de usuario inexistente es idempotente y no modifica la estructura") {
        ModAreas::AgregarUser(1, 1);
        ModAreas::AgregarUser(2, 1);

        ModAreas::QuitarUser(99, 1); // No existe
        CHECK(ConnGroups[1].CountEntrys == 2);
        REQUIRE(ConnGroups[1].UserEntrys.size() == 3);
        CHECK(ConnGroups[1].UserEntrys[1] == 1);
        CHECK(ConnGroups[1].UserEntrys[2] == 2);
    }

    SUBCASE("T2.6: Blanqueo exhaustivo de AreasInfo en UserList al agregar usuario") {
        UserList[4].AreasInfo.AreaID = 999;
        UserList[4].AreasInfo.AreaPerteneceX = 123;
        UserList[4].AreasInfo.AreaPerteneceY = 456;
        UserList[4].AreasInfo.AreaReciveX = 789;
        UserList[4].AreasInfo.AreaReciveY = 321;

        ModAreas::AgregarUser(4, 1);

        CHECK(UserList[4].AreasInfo.AreaID == 0);
        CHECK(UserList[4].AreasInfo.AreaPerteneceX == 0);
        CHECK(UserList[4].AreasInfo.AreaPerteneceY == 0);
        CHECK(UserList[4].AreasInfo.AreaReciveX == 0);
        CHECK(UserList[4].AreasInfo.AreaReciveY == 0);
    }

    SUBCASE("T2.7: Validación de límites de mapa inválidos") {
        ModAreas::AgregarUser(1, 0);   // Fuera de rango inferior
        ModAreas::AgregarUser(1, 999); // Fuera de rango superior

        ModAreas::QuitarUser(1, 0);
        ModAreas::QuitarUser(1, 999);

        // El mapa válido 1 no se vio afectado
        CHECK(ConnGroups[1].CountEntrys == 0);
    }
}

TEST_CASE("Fase 3: Ciclo de visibilidad de jugadores (CheckUpdateNeededUser)") {
    NumMaps = 2;
    ModAreas::InitAreas();

    // MapData dimensionado para 2 mapas: (2 + 1) * 101 * 101
    MapData.assign(3 * 101 * 101, MapBlock{});
    ObjDataList.assign(10, ObjData{});

    UserList.resize(10);
    for (std::size_t u = 0; u < UserList.size(); ++u) {
        UserList[u].AreasInfo = ModAreas::AreaInfo{};
        UserList[u].outgoingData = std::make_unique<clsByteQueue>();
        UserList[u].char_appearance.CharIndex = static_cast<std::int16_t>(u);
        UserList[u].Pos.Map = 1;
        UserList[u].Pos.X = 1;
        UserList[u].Pos.Y = 1;
        UserList[u].flags.Privilegios = PlayerType::UserPlayer;
        UserList[u].flags.AdminInvisible = 0;
        UserList[u].flags.invisible = 0;
        UserList[u].flags.Oculto = 0;
    }

    auto get_map_idx = [](std::int16_t map, std::int16_t x, std::int16_t y) {
        return (static_cast<std::size_t>(map) * 101 + static_cast<std::size_t>(x)) * 101 + static_cast<std::size_t>(y);
    };

    SUBCASE("T3.1: No-op: Movimiento dentro de la misma franja de área (salida temprana)") {
        const std::int16_t u = 1;
        UserList[u].Pos.X = 10;
        UserList[u].Pos.Y = 10;
        // AreaID para (10, 10) es (10/9+1)*(10/9+1) = 2*2 = 4
        UserList[u].AreasInfo.AreaID = 4;
        UserList[u].outgoingData = std::make_unique<clsByteQueue>();

        ModAreas::CheckUpdateNeededUser(u, eHeading::EAST);

        // Al coincidir AreaID con el precalculado, sale inmediatamente sin emitir WriteAreaChanged
        CHECK(UserList[u].outgoingData->length() == 0);
    }

    SUBCASE("T3.2: Login en esquina extrema (X=5, Y=5) con USER_NUEVO (Bug #25)") {
        const std::int16_t u = 1;
        UserList[u].Pos.X = 5;
        UserList[u].Pos.Y = 5;
        UserList[u].AreasInfo.AreaID = 0; // Obliga actualización
        UserList[u].outgoingData = std::make_unique<clsByteQueue>();

        struct MakeCharCall {
            std::int16_t snd_index;
            std::int16_t user_index;
            std::int16_t x;
            std::int16_t y;
        };
        static std::vector<MakeCharCall> make_char_calls;
        make_char_calls.clear();

        ModAreas::SetMakeUserCharHook([](bool /*to_map*/, std::int16_t snd_index, std::int16_t user_index,
                                         std::int16_t /*map*/, std::int16_t x, std::int16_t y) {
            make_char_calls.push_back({snd_index, user_index, x, y});
        });

        // Colocamos al usuario 1 en el mapa en (5, 5)
        MapData[get_map_idx(1, 5, 5)].UserIndex = u;

        ModAreas::CheckUpdateNeededUser(u, ModAreas::USER_NUEVO, false);

        // Constatar Bug #25: MinX y MinY retienen -9 antes del clamp
        CHECK(UserList[u].AreasInfo.MinX == -9);
        CHECK(UserList[u].AreasInfo.MinY == -9);

        // Constatar emisión de WriteAreaChanged (opcode 89 + X(5) + Y(5) = 3 bytes)
        CHECK(UserList[u].outgoingData->length() == 3);
        const auto opcode = UserList[u].outgoingData->ReadByte();
        CHECK(opcode == static_cast<std::uint8_t>(ao::net::protocol::ServerPacketID::AreaChanged));
        CHECK(UserList[u].outgoingData->ReadByte() == 5);
        CHECK(UserList[u].outgoingData->ReadByte() == 5);

        // MakeUserChar propio enviado al usuario
        REQUIRE(make_char_calls.size() == 1);
        CHECK(make_char_calls[0].snd_index == u);
        CHECK(make_char_calls[0].user_index == u);
        CHECK(make_char_calls[0].x == 5);
        CHECK(make_char_calls[0].y == 5);

        // Actualización de bitmasks y AreaID:
        // FranjaX = 5/9 = 0 -> Recive = 3, Pertenece = 1 (1<<0)
        // FranjaY = 5/9 = 0 -> Recive = 3, Pertenece = 1 (1<<0)
        // AreaID = (0+1)*(0+1) = 1
        CHECK(UserList[u].AreasInfo.AreaReciveX == 3);
        CHECK(UserList[u].AreasInfo.AreaReciveY == 3);
        CHECK(UserList[u].AreasInfo.AreaPerteneceX == 1);
        CHECK(UserList[u].AreasInfo.AreaPerteneceY == 1);
        CHECK(UserList[u].AreasInfo.AreaID == 1);

        ModAreas::SetMakeUserCharHook(nullptr);
    }

    SUBCASE("T3.3: Cruce de frontera hacia el Este (EAST) con offsets relativos") {
        const std::int16_t u = 1;
        // Usuario venía de MinX = -9, MinY = -9 y cruza a la franja X=9..17
        UserList[u].Pos.X = 9;
        UserList[u].Pos.Y = 5;
        UserList[u].AreasInfo.AreaID = 1; // ID previo
        UserList[u].AreasInfo.MinX = -9;
        UserList[u].AreasInfo.MinY = -9;

        static std::vector<std::pair<std::int16_t, std::int16_t>> scanned_tiles;
        scanned_tiles.clear();

        ModAreas::SetMakeUserCharHook([](bool /*to_map*/, std::int16_t /*snd_index*/, std::int16_t /*user_index*/,
                                         std::int16_t /*map*/, std::int16_t x, std::int16_t y) {
            scanned_tiles.emplace_back(x, y);
        });

        // Ponemos un segundo usuario en el tile emergente (18, 5)
        const std::int16_t other = 2;
        MapData[get_map_idx(1, 18, 5)].UserIndex = other;

        ModAreas::CheckUpdateNeededUser(u, eHeading::EAST);

        // En EAST:
        // MinX emergente = MinX(-9) + 27 = 18
        // MaxX emergente = MinX(-9) + 35 = 26
        // MinX persistido = MinX_emergente(18) - 18 = 0
        CHECK(UserList[u].AreasInfo.MinX == 0);
        CHECK(UserList[u].AreasInfo.MinY == -9);

        // Constatar que el tile (18, 5) fue explorado
        REQUIRE_FALSE(scanned_tiles.empty());
        CHECK(scanned_tiles[0].first == 18);
        CHECK(scanned_tiles[0].second == 5);

        // Bitmasks actualizadas para X=9 (franja 1): 1<<1 = 2, Recive = 7
        CHECK(UserList[u].AreasInfo.AreaPerteneceX == 2);
        CHECK(UserList[u].AreasInfo.AreaReciveX == 7);
        CHECK(UserList[u].AreasInfo.AreaID == 2); // (9/9+1)*(5/9+1) = 2*1 = 2

        ModAreas::SetMakeUserCharHook(nullptr);
    }

    SUBCASE("T3.4: Notificación recíproca entre jugadores y FlushBuffer") {
        const std::int16_t u1 = 1;
        const std::int16_t u2 = 2;

        UserList[u1].Pos.X = 9;
        UserList[u1].Pos.Y = 5;
        UserList[u1].AreasInfo.AreaID = 1;
        UserList[u1].AreasInfo.MinX = -9;
        UserList[u1].AreasInfo.MinY = -9;

        UserList[u2].Pos.X = 18;
        UserList[u2].Pos.Y = 5;
        UserList[u2].char_appearance.CharIndex = 202;
        MapData[get_map_idx(1, 18, 5)].UserIndex = u2;

        struct RecipCall {
            std::int16_t snd_index;
            std::int16_t user_index;
        };
        static std::vector<RecipCall> recip_calls;
        recip_calls.clear();

        ModAreas::SetMakeUserCharHook([](bool /*to_map*/, std::int16_t snd_index, std::int16_t user_index,
                                         std::int16_t /*map*/, std::int16_t /*x*/, std::int16_t /*y*/) {
            recip_calls.push_back({snd_index, user_index});
        });

        ModAreas::CheckUpdateNeededUser(u1, eHeading::EAST);

        // Constatar notificación bidireccional recíproca
        REQUIRE(recip_calls.size() == 2);
        // u1 recibe la criatura de u2
        CHECK(recip_calls[0].snd_index == u1);
        CHECK(recip_calls[0].user_index == u2);
        // u2 recibe la criatura de u1
        CHECK(recip_calls[1].snd_index == u2);
        CHECK(recip_calls[1].user_index == u1);

        ModAreas::SetMakeUserCharHook(nullptr);
    }

    SUBCASE("T3.5: Interacción con criaturas (MakeNPCChar), objetos y puertas") {
        const std::int16_t u = 1;
        UserList[u].Pos.X = 9;
        UserList[u].Pos.Y = 5;
        UserList[u].AreasInfo.AreaID = 1;
        UserList[u].AreasInfo.MinX = -9;
        UserList[u].AreasInfo.MinY = -9;
        UserList[u].outgoingData = std::make_unique<clsByteQueue>();

        // 1. NPC en (20, 5)
        MapData[get_map_idx(1, 20, 5)].NpcIndex = 50;

        // 2. Puerta en (22, 5)
        const std::int16_t obj_puerta_idx = 1;
        ObjDataList[obj_puerta_idx].OBJType = eOBJType::otPuertas;
        ObjDataList[obj_puerta_idx].GrhIndex = 400;
        MapData[get_map_idx(1, 22, 5)].ObjInfo.ObjIndex = obj_puerta_idx;
        MapData[get_map_idx(1, 22, 5)].Blocked = 1;
        MapData[get_map_idx(1, 21, 5)].Blocked = 1;

        static std::vector<std::int16_t> npc_calls;
        npc_calls.clear();
        ModAreas::SetMakeNPCCharHook([](bool /*to_map*/, std::int16_t snd_index, std::int16_t npc_index,
                                        std::int16_t /*map*/, std::int16_t /*x*/, std::int16_t /*y*/) {
            npc_calls.push_back(npc_index);
        });

        struct BloqCall {
            std::int16_t snd_index;
            std::int16_t x;
            std::int16_t y;
            bool b;
        };
        static std::vector<BloqCall> bloq_calls;
        bloq_calls.clear();
        ModAreas::SetBloquearHook([](bool /*to_map*/, std::int16_t snd_index, std::int16_t x, std::int16_t y, bool b) {
            bloq_calls.push_back({snd_index, x, y, b});
        });

        ModAreas::CheckUpdateNeededUser(u, eHeading::EAST);

        // Constatar que MakeNPCChar fue despachado para el NPC 50
        REQUIRE(npc_calls.size() == 1);
        CHECK(npc_calls[0] == 50);

        // Constatar que Bloquear fue llamado para (X, Y) y (X-1, Y)
        REQUIRE(bloq_calls.size() == 2);
        CHECK(bloq_calls[0].x == 22);
        CHECK(bloq_calls[0].y == 5);
        CHECK(bloq_calls[0].b == true);

        CHECK(bloq_calls[1].x == 21);
        CHECK(bloq_calls[1].y == 5);
        CHECK(bloq_calls[1].b == true);

        ModAreas::SetMakeNPCCharHook(nullptr);
        ModAreas::SetBloquearHook(nullptr);
    }

    SUBCASE("T3.6: Preservación de la asimetría legacy (no remoción de personajes)") {
        // En AO v0.13.0, CheckUpdateNeededUser NUNCA emite CharacterRemove (BP).
        // El cliente es responsable de eliminar los personajes que quedaron fuera del área
        // gracias a la señal enviada por WriteAreaChanged.
        const std::int16_t u = 1;
        UserList[u].Pos.X = 9;
        UserList[u].Pos.Y = 5;
        UserList[u].AreasInfo.AreaID = 1;
        UserList[u].AreasInfo.MinX = -9;
        UserList[u].AreasInfo.MinY = -9;
        UserList[u].outgoingData = std::make_unique<clsByteQueue>();

        ModAreas::CheckUpdateNeededUser(u, eHeading::EAST);

        // Verificamos el stream de paquetes recibido en outgoingData:
        // Solo debe contener AreaChanged (3 bytes)
        CHECK(UserList[u].outgoingData->length() == 3);
        const auto opcode = UserList[u].outgoingData->ReadByte();
        CHECK(opcode == static_cast<std::uint8_t>(ao::net::protocol::ServerPacketID::AreaChanged));
        CHECK(opcode != static_cast<std::uint8_t>(ao::net::protocol::ServerPacketID::CharacterRemove));
    }
}

TEST_CASE("Fase 4: Ciclo de visibilidad de criaturas (AgregarNpc y CheckUpdateNeededNpc)") {
    NumMaps = 2;
    ModAreas::InitAreas();

    MapData.assign(3 * 101 * 101, MapBlock{});
    if (MapInfoList.size() < 3) {
        MapInfoList.resize(3);
    }
    MapInfoList[1].NumUsers = 0;
    MapInfoList[2].NumUsers = 0;

    auto get_map_idx = [](std::int16_t map, std::int16_t x, std::int16_t y) {
        return (static_cast<std::size_t>(map) * 101 + static_cast<std::size_t>(x)) * 101 + static_cast<std::size_t>(y);
    };

    SUBCASE("T4.1: AgregarNpc inicializa campos espaciales e invoca CheckUpdateNeededNpc con USER_NUEVO") {
        const std::int16_t npc_idx = 1;
        Npclist[npc_idx].Pos.Map = 1;
        Npclist[npc_idx].Pos.X = 5;
        Npclist[npc_idx].Pos.Y = 5;
        Npclist[npc_idx].AreasInfo.AreaID = 99;
        Npclist[npc_idx].AreasInfo.MinX = 123;
        Npclist[npc_idx].AreasInfo.MinY = 456;
        MapInfoList[1].NumUsers = 0;

        ModAreas::AgregarNpc(npc_idx);

        // Constatar Bug #25: MinX y MinY retienen -9 en USER_NUEVO para coords < 9
        CHECK(Npclist[npc_idx].AreasInfo.MinX == -9);
        CHECK(Npclist[npc_idx].AreasInfo.MinY == -9);

        // Actualización de bitmasks y AreaID:
        // FranjaX = 5/9 = 0 -> Recive = 3, Pertenece = 1 (1<<0)
        // FranjaY = 5/9 = 0 -> Recive = 3, Pertenece = 1 (1<<0)
        // AreaID = (0+1)*(0+1) = 1
        CHECK(Npclist[npc_idx].AreasInfo.AreaReciveX == 3);
        CHECK(Npclist[npc_idx].AreasInfo.AreaReciveY == 3);
        CHECK(Npclist[npc_idx].AreasInfo.AreaPerteneceX == 1);
        CHECK(Npclist[npc_idx].AreasInfo.AreaPerteneceY == 1);
        CHECK(Npclist[npc_idx].AreasInfo.AreaID == 1);
    }

    SUBCASE("T4.2: Movimiento de NPC en mapa vacío (NumUsers == 0): sin notificaciones pero actualiza máscaras") {
        const std::int16_t npc_idx = 2;
        Npclist[npc_idx].Pos.Map = 1;
        Npclist[npc_idx].Pos.X = 9; // Cruza a franja 1
        Npclist[npc_idx].Pos.Y = 5;
        Npclist[npc_idx].AreasInfo.AreaID = 1;
        Npclist[npc_idx].AreasInfo.MinX = -9;
        Npclist[npc_idx].AreasInfo.MinY = -9;
        MapInfoList[1].NumUsers = 0;

        static bool npc_hook_called = false;
        npc_hook_called = false;
        ModAreas::SetMakeNPCCharHook([](bool /*to_map*/, std::int16_t /*snd_index*/, std::int16_t /*npc_index*/,
                                        std::int16_t /*map*/, std::int16_t /*x*/, std::int16_t /*y*/) {
            npc_hook_called = true;
        });

        ModAreas::CheckUpdateNeededNpc(npc_idx, eHeading::EAST);

        // No debe haberse despachado MakeNPCChar debido a NumUsers == 0
        CHECK_FALSE(npc_hook_called);

        // Pero sus máscaras y AreaID sí deben haberse actualizado
        CHECK(Npclist[npc_idx].AreasInfo.MinX == 0); // -9 + 27 - 18 = 0
        CHECK(Npclist[npc_idx].AreasInfo.MinY == -9);
        CHECK(Npclist[npc_idx].AreasInfo.AreaPerteneceX == 2); // 1 << 1
        CHECK(Npclist[npc_idx].AreasInfo.AreaReciveX == 7);
        CHECK(Npclist[npc_idx].AreasInfo.AreaID == 2); // (9/9+1)*(5/9+1) = 2*1 = 2

        ModAreas::SetMakeNPCCharHook(nullptr);
    }

    SUBCASE("T4.3: Movimiento de NPC con usuarios presentes (NumUsers > 0): despacha MakeNPCChar") {
        const std::int16_t npc_idx = 3;
        Npclist[npc_idx].Pos.Map = 1;
        Npclist[npc_idx].Pos.X = 9;
        Npclist[npc_idx].Pos.Y = 5;
        Npclist[npc_idx].AreasInfo.AreaID = 1;
        Npclist[npc_idx].AreasInfo.MinX = -9;
        Npclist[npc_idx].AreasInfo.MinY = -9;
        MapInfoList[1].NumUsers = 1;

        // Ponemos un usuario en el tile emergente (20, 5)
        const std::int16_t user_target = 5;
        MapData[get_map_idx(1, 20, 5)].UserIndex = user_target;

        struct NpcDispatch {
            std::int16_t snd_index;
            std::int16_t npc_index;
            std::int16_t x;
            std::int16_t y;
        };
        static std::vector<NpcDispatch> dispatches;
        dispatches.clear();

        ModAreas::SetMakeNPCCharHook([](bool /*to_map*/, std::int16_t snd_index, std::int16_t npc_index,
                                        std::int16_t /*map*/, std::int16_t x, std::int16_t y) {
            dispatches.push_back({snd_index, npc_index, x, y});
        });

        ModAreas::CheckUpdateNeededNpc(npc_idx, eHeading::EAST);

        // Constatar que MakeNPCChar fue despachado al usuario receptor
        REQUIRE(dispatches.size() == 1);
        CHECK(dispatches[0].snd_index == user_target);
        CHECK(dispatches[0].npc_index == npc_idx);
        CHECK(dispatches[0].x == 9);
        CHECK(dispatches[0].y == 5);

        ModAreas::SetMakeNPCCharHook(nullptr);
    }

    SUBCASE("T4.4: Salida temprana en NPC si coincide AreaID") {
        const std::int16_t npc_idx = 4;
        Npclist[npc_idx].Pos.Map = 1;
        Npclist[npc_idx].Pos.X = 10;
        Npclist[npc_idx].Pos.Y = 10;
        // AreaID para (10, 10) es 4
        Npclist[npc_idx].AreasInfo.AreaID = 4;
        MapInfoList[1].NumUsers = 1;

        static bool called = false;
        called = false;
        ModAreas::SetMakeNPCCharHook([](bool /*to_map*/, std::int16_t /*snd_index*/, std::int16_t /*npc_index*/,
                                        std::int16_t /*map*/, std::int16_t /*x*/, std::int16_t /*y*/) {
            called = true;
        });

        ModAreas::CheckUpdateNeededNpc(npc_idx, eHeading::EAST);

        // Al coincidir AreaID, sale de inmediato
        CHECK_FALSE(called);

        ModAreas::SetMakeNPCCharHook(nullptr);
    }

    SUBCASE("T4.5: Ausencia de FlushBuffer en el flujo de NPCs (preservación de asimetría)") {
        const std::int16_t npc_idx = 5;
        Npclist[npc_idx].Pos.Map = 1;
        Npclist[npc_idx].Pos.X = 9;
        Npclist[npc_idx].Pos.Y = 5;
        Npclist[npc_idx].AreasInfo.AreaID = 1;
        Npclist[npc_idx].AreasInfo.MinX = -9;
        Npclist[npc_idx].AreasInfo.MinY = -9;
        MapInfoList[1].NumUsers = 1;

        const std::int16_t user_target = 6;
        UserList.resize(10);
        UserList[user_target].outgoingData = std::make_unique<clsByteQueue>();
        // Simulamos datos encolados en el buffer del usuario
        UserList[user_target].outgoingData->WriteByte(123);
        MapData[get_map_idx(1, 20, 5)].UserIndex = user_target;

        ModAreas::CheckUpdateNeededNpc(npc_idx, eHeading::EAST);

        // A diferencia del flujo de usuarios (donde FlushBuffer vacía la cola del observador),
        // en NPCs no se llama a FlushBuffer, por lo que outgoingData conserva sus bytes intactos.
        CHECK(UserList[user_target].outgoingData->length() == 1);
        CHECK(UserList[user_target].outgoingData->ReadByte() == 123);
    }
}

}
