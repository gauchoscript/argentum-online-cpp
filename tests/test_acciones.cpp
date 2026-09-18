#include "doctest/doctest.h"

#include "server/Acciones.hpp"
#include "server/Declares.hpp"

inline std::size_t mapBlockIndex(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return (static_cast<std::size_t>(map) * 101 * 101) + (static_cast<std::size_t>(y) * 101) + static_cast<std::size_t>(x);
}

TEST_SUITE("Acciones") {

    TEST_CASE("AccionParaPuerta - Apertura, Cierre, Cerrojo con Llave y Resguardo X = 1") {
        Acciones::InitDefaultAccionesCallbacks();

        int16_t map = 1;
        int16_t x = 50;
        int16_t y = 50;
        NumMaps = 10;
        MaxUsers = 100;

        if (MapData.size() < (static_cast<std::size_t>(map) + 1) * 101 * 101) {
            MapData.resize((static_cast<std::size_t>(map) + 1) * 101 * 101);
        }
        if (UserList.size() < 101) {
            UserList.resize(101);
        }
        if (ObjDataList.size() < 1000) {
            ObjDataList.resize(1000);
        }

        int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].Pos = WorldPos{map, x, y};

        int16_t obj_cerrada = 100;
        int16_t obj_abierta = 101;

        ObjDataList[obj_cerrada] = ObjData{};
        ObjDataList[obj_cerrada].OBJType = eOBJType::otPuertas;
        ObjDataList[obj_cerrada].Cerrada = 1;
        ObjDataList[obj_cerrada].Llave = 0;
        ObjDataList[obj_cerrada].IndexAbierta = obj_abierta;
        ObjDataList[obj_cerrada].GrhIndex = 500;

        ObjDataList[obj_abierta] = ObjData{};
        ObjDataList[obj_abierta].OBJType = eOBJType::otPuertas;
        ObjDataList[obj_abierta].Cerrada = 0;
        ObjDataList[obj_abierta].Llave = 0;
        ObjDataList[obj_abierta].IndexCerrada = obj_cerrada;
        ObjDataList[obj_abierta].GrhIndex = 501;

        std::size_t idx = mapBlockIndex(map, x, y);
        std::size_t idx_left = mapBlockIndex(map, x - 1, y);

        MapData[idx].ObjInfo.ObjIndex = obj_cerrada;
        MapData[idx].Blocked = 1;
        MapData[idx_left].Blocked = 1;

        uint16_t last_grh_sent = 0;
        bool area_sent = false;
        bool wave_sent = false;
        bool blocked_called = false;

        Acciones::AccionesCallbacks callbacks{};
        callbacks.SendObjectCreateToArea = [&](int16_t m, int16_t px, int16_t py, uint16_t grh) {
            area_sent = true;
            last_grh_sent = grh;
        };
        callbacks.SendPlayWaveToArea = [&](int16_t m, int16_t px, int16_t py, uint16_t wave) {
            wave_sent = true;
        };
        callbacks.SetTileBlocked = [&](bool b, int16_t m, int16_t px, int16_t py, uint8_t flag) {
            blocked_called = true;
        };

        Acciones::SetAccionesCallbacks(callbacks);

        // 1. Abrir puerta
        Acciones::AccionParaPuerta(map, x, y, user_index);

        CHECK_EQ(MapData[idx].ObjInfo.ObjIndex, obj_abierta);
        CHECK_EQ(MapData[idx].Blocked, 0);
        CHECK_EQ(MapData[idx_left].Blocked, 0);
        CHECK(area_sent);
        CHECK_EQ(last_grh_sent, 501);
        CHECK(wave_sent);
        CHECK(blocked_called);

        // 2. Cerrar puerta
        Acciones::AccionParaPuerta(map, x, y, user_index);

        CHECK_EQ(MapData[idx].ObjInfo.ObjIndex, obj_cerrada);
        CHECK_EQ(MapData[idx].Blocked, 1);
        CHECK_EQ(MapData[idx_left].Blocked, 1);
        CHECK_EQ(last_grh_sent, 500);

        // 3. Resguardo de Seguridad Bug #47: Puerta en X = 1
        int16_t x_edge = 1;
        std::size_t idx_edge = mapBlockIndex(map, x_edge, y);
        MapData[idx_edge].ObjInfo.ObjIndex = obj_cerrada;
        MapData[idx_edge].Blocked = 1;
        UserList[user_index].Pos = WorldPos{map, x_edge, y};

        Acciones::AccionParaPuerta(map, x_edge, y, user_index);
        CHECK_EQ(MapData[idx_edge].ObjInfo.ObjIndex, obj_abierta);
        CHECK_EQ(MapData[idx_edge].Blocked, 0);
    }

    TEST_CASE("AccionParaCartel - Lectura e Invocación de Signal") {
        int16_t map = 1;
        int16_t x = 10;
        int16_t y = 10;

        NumMaps = 10;
        MaxUsers = 100;

        if (MapData.size() < (static_cast<std::size_t>(map) + 1) * 101 * 101) {
            MapData.resize((static_cast<std::size_t>(map) + 1) * 101 * 101);
        }
        if (UserList.size() < 101) {
            UserList.resize(101);
        }
        if (ObjDataList.size() < 1000) {
            ObjDataList.resize(1000);
        }

        int16_t user_index = 1;
        int16_t cartel_obj = 200;
        ObjDataList[cartel_obj] = ObjData{};
        ObjDataList[cartel_obj].OBJType = eOBJType::otCarteles;
        ObjDataList[cartel_obj].texto = "Bienvenidos a ULL";

        std::size_t idx = mapBlockIndex(map, x, y);
        MapData[idx].ObjInfo.ObjIndex = cartel_obj;

        bool signal_written = false;
        Acciones::AccionesCallbacks callbacks{};
        callbacks.WriteShowSignal = [&](int16_t uid, int16_t objid) {
            signal_written = true;
            CHECK_EQ(uid, user_index);
            CHECK_EQ(objid, cartel_obj);
        };
        Acciones::SetAccionesCallbacks(callbacks);

        Acciones::AccionParaCartel(map, x, y, user_index);
        CHECK(signal_written);
    }

    TEST_CASE("AccionParaForo - Rango de Distancia y Despacho") {
        int16_t map = 1;
        int16_t x = 20;
        int16_t y = 20;

        NumMaps = 10;
        MaxUsers = 100;

        if (MapData.size() < (static_cast<std::size_t>(map) + 1) * 101 * 101) {
            MapData.resize((static_cast<std::size_t>(map) + 1) * 101 * 101);
        }
        if (UserList.size() < 101) {
            UserList.resize(101);
        }
        if (ObjDataList.size() < 1000) {
            ObjDataList.resize(1000);
        }

        int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].Pos = WorldPos{map, x, y};

        int16_t foro_obj = 300;
        ObjDataList[foro_obj] = ObjData{};
        ObjDataList[foro_obj].OBJType = eOBJType::otForos;
        ObjDataList[foro_obj].ForoID = "5";

        std::size_t idx = mapBlockIndex(map, x, y);
        MapData[idx].ObjInfo.ObjIndex = foro_obj;

        bool posts_sent = false;
        bool forum_form_shown = false;

        Acciones::AccionesCallbacks callbacks{};
        callbacks.SendPosts = [&](int16_t uid, int16_t fid) -> bool {
            posts_sent = true;
            CHECK_EQ(fid, 5);
            return true;
        };
        callbacks.WriteShowForumForm = [&](int16_t uid) {
            forum_form_shown = true;
        };
        Acciones::SetAccionesCallbacks(callbacks);

        Acciones::AccionParaForo(map, x, y, user_index);
        CHECK(posts_sent);
        CHECK(forum_form_shown);

        // Distancia lejana (> 2 celdas)
        UserList[user_index].Pos = WorldPos{map, 25, 20};
        posts_sent = false;
        forum_form_shown = false;

        Acciones::AccionParaForo(map, x, y, user_index);
        CHECK_FALSE(posts_sent);
        CHECK_FALSE(forum_form_shown);
    }

    TEST_CASE("AccionParaRamita - Supervivencia, Bug #48, Zona Segura y cGarbage TrashCollector") {
        int16_t map = 1;
        int16_t x = 30;
        int16_t y = 30;

        NumMaps = 10;
        MaxUsers = 100;

        if (MapData.size() < (static_cast<std::size_t>(map) + 1) * 101 * 101) {
            MapData.resize((static_cast<std::size_t>(map) + 1) * 101 * 101);
        }
        if (MapInfoList.size() < static_cast<std::size_t>(map) + 1) {
            MapInfoList.resize(static_cast<std::size_t>(map) + 1);
        }
        if (UserList.size() < 101) {
            UserList.resize(101);
        }
        if (ObjDataList.size() < 1000) {
            ObjDataList.resize(1000);
        }

        int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].Pos = WorldPos{map, x, y};
        UserList[user_index].Stats.UserSkills[eSkill::Supervivencia] = 10; // Bug #48 -> Suerte = 1 (100% éxito)

        std::size_t idx = mapBlockIndex(map, x, y);
        MapData[idx].trigger = eTrigger::NADA;
        MapInfoList[map].Pk = true;
        MapInfoList[map].Zona = ""; // No ciudad

        bool obj_made = false;
        bool skill_up = false;
        bool garbage_registered = false;

        Acciones::AccionesCallbacks callbacks{};
        callbacks.MakeObj = [&](const Obj& o, int16_t m, int16_t px, int16_t py) {
            obj_made = true;
            CHECK_EQ(o.ObjIndex, FOGATA);
        };
        callbacks.SubirSkill = [&](int16_t uid, eSkill sk, bool exito) {
            skill_up = true;
            CHECK(exito);
        };
        callbacks.RegisterGarbageFire = [&](int16_t m, int16_t px, int16_t py) {
            garbage_registered = true;
        };
        Acciones::SetAccionesCallbacks(callbacks);

        Acciones::AccionParaRamita(map, x, y, user_index);

        CHECK(obj_made);
        CHECK(skill_up);
        CHECK(garbage_registered);

        // Prohibición en Zona Segura
        MapData[idx].trigger = eTrigger::ZONASEGURA;
        obj_made = false;
        Acciones::AccionParaRamita(map, x, y, user_index);
        CHECK_FALSE(obj_made);
    }

    TEST_CASE("Accion Despachador - TargetInmediato (Bug #49), Comerciantes y Puertas Multitile") {
        int16_t map = 1;
        int16_t x = 40;
        int16_t y = 40;

        NumMaps = 10;
        MaxUsers = 100;

        if (MapData.size() < (static_cast<std::size_t>(map) + 1) * 101 * 101) {
            MapData.resize((static_cast<std::size_t>(map) + 1) * 101 * 101);
        }
        if (UserList.size() < 101) {
            UserList.resize(101);
        }
        if (ObjDataList.size() < 1000) {
            ObjDataList.resize(1000);
        }

        int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].Pos = WorldPos{map, x, y};

        int16_t npc_index = 5;
        Npclist[npc_index] = npc{};
        Npclist[npc_index].Pos = WorldPos{map, x, y};
        Npclist[npc_index].Comercia = 1;

        std::size_t idx = mapBlockIndex(map, x, y);
        MapData[idx].NpcIndex = npc_index;

        bool comercio_iniciado = false;
        Acciones::AccionesCallbacks callbacks{};
        callbacks.IniciarComercioNPC = [&](int16_t uid) {
            comercio_iniciado = true;
        };
        Acciones::SetAccionesCallbacks(callbacks);

        // 1. Clic en NPC comerciante (muta inmediatamente TargetNPC Bug #49)
        Acciones::Accion(user_index, map, x, y);

        CHECK_EQ(UserList[user_index].flags.TargetNPC, npc_index);
        CHECK(comercio_iniciado);

        // 2. Multitile Puerta adyacente en (X + 1, Y)
        MapData[idx].NpcIndex = 0;
        int16_t puerta_obj = 400;
        ObjDataList[puerta_obj] = ObjData{};
        ObjDataList[puerta_obj].OBJType = eOBJType::otPuertas;
        ObjDataList[puerta_obj].Cerrada = 1;
        ObjDataList[puerta_obj].IndexAbierta = puerta_obj;
        ObjDataList[puerta_obj].IndexCerrada = puerta_obj;

        std::size_t idx_right = mapBlockIndex(map, x + 1, y);
        MapData[idx_right].ObjInfo.ObjIndex = puerta_obj;

        Acciones::Accion(user_index, map, x, y);
        CHECK_EQ(UserList[user_index].flags.TargetObj, puerta_obj);
    }
}
