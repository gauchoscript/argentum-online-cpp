#include "Acciones.hpp"

#include <cmath>
#include <cstdlib>

namespace Acciones {

static AccionesCallbacks s_callbacks;

inline std::size_t mapBlockIndex(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return (static_cast<std::size_t>(map) * 101 * 101) + (static_cast<std::size_t>(y) * 101) + static_cast<std::size_t>(x);
}

inline bool InMapBounds(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return map > 0 && map <= NumMaps && x >= XMinMapSize && x <= XMaxMapSize && y >= YMinMapSize && y <= YMaxMapSize;
}

static constexpr uint8_t FONTTYPE_INFO_VAL = static_cast<uint8_t>(ao::net::protocol::FontTypeNames::FONTTYPE_INFO);

void SetAccionesCallbacks(const AccionesCallbacks& callbacks) {
    s_callbacks = callbacks;
}

void InitDefaultAccionesCallbacks() {
    s_callbacks = AccionesCallbacks{};
}

void Accion(int16_t user_index, int16_t map, int16_t x, int16_t y) {
    if (user_index <= 0 || user_index > MaxUsers) return;

    // Rango visión
    if ((std::abs(UserList[user_index].Pos.Y - y) > RANGO_VISION_Y) ||
        (std::abs(UserList[user_index].Pos.X - x) > RANGO_VISION_X)) {
        return;
    }

    if (!InMapBounds(map, x, y)) return;

    auto& user = UserList[user_index];
    std::size_t idx = mapBlockIndex(map, x, y);

    // Acciones NPCs
    if (MapData[idx].NpcIndex > 0) {
        int16_t temp_index = MapData[idx].NpcIndex;

        // TargetNPC incondicional (Bug #49)
        user.flags.TargetNPC = temp_index;
        auto& npc = Npclist[temp_index];

        if (npc.Comercia == 1) {
            if (user.flags.Muerto == 1) {
                if (s_callbacks.WriteConsoleMsg) {
                    s_callbacks.WriteConsoleMsg(user_index, "¡Estás muerto!!", FONTTYPE_INFO_VAL);
                }
                return;
            }

            if (user.flags.Comerciando) return;

            if (Distancia(npc.Pos, user.Pos) > 3) {
                if (s_callbacks.WriteConsoleMsg) {
                    s_callbacks.WriteConsoleMsg(user_index, "Estás demasiado lejos del vendedor.", FONTTYPE_INFO_VAL);
                }
                return;
            }

            if (s_callbacks.IniciarComercioNPC) {
                s_callbacks.IniciarComercioNPC(user_index);
            }
        }
        else if (npc.NPCtype == eNPCType::Banquero) {
            if (user.flags.Muerto == 1) {
                if (s_callbacks.WriteConsoleMsg) {
                    s_callbacks.WriteConsoleMsg(user_index, "¡Estás muerto!!", FONTTYPE_INFO_VAL);
                }
                return;
            }

            if (user.flags.Comerciando) return;

            if (Distancia(npc.Pos, user.Pos) > 3) {
                if (s_callbacks.WriteConsoleMsg) {
                    s_callbacks.WriteConsoleMsg(user_index, "Estás demasiado lejos del vendedor.", FONTTYPE_INFO_VAL);
                }
                return;
            }

            if (s_callbacks.IniciarDepositoBanco) {
                s_callbacks.IniciarDepositoBanco(user_index);
            }
        }
        else if (npc.NPCtype == eNPCType::Revividor || npc.NPCtype == eNPCType::ResucitadorNewbie) {
            if (Distancia(user.Pos, npc.Pos) > 10) {
                if (s_callbacks.WriteConsoleMsg) {
                    s_callbacks.WriteConsoleMsg(user_index, "El sacerdote no puede curarte debido a que estás demasiado lejos.", FONTTYPE_INFO_VAL);
                }
                return;
            }

            bool es_newbie = s_callbacks.EsNewbie ? s_callbacks.EsNewbie(user_index) : false;
            if (user.flags.Muerto == 1 && (npc.NPCtype == eNPCType::Revividor || es_newbie)) {
                if (s_callbacks.RevivirUsuario) {
                    s_callbacks.RevivirUsuario(user_index);
                }
            }

            if (npc.NPCtype == eNPCType::Revividor || es_newbie) {
                user.Stats.MinHp = user.Stats.MaxHp;
                if (s_callbacks.WriteUpdateUserStats) {
                    s_callbacks.WriteUpdateUserStats(user_index);
                }
            }
        }
    }
    // ¿Es un obj?
    else if (MapData[idx].ObjInfo.ObjIndex > 0) {
        int16_t temp_index = MapData[idx].ObjInfo.ObjIndex;
        user.flags.TargetObj = temp_index;

        switch (ObjDataList[temp_index].OBJType) {
            case eOBJType::otPuertas:
                AccionParaPuerta(map, x, y, user_index);
                break;
            case eOBJType::otCarteles:
                AccionParaCartel(map, x, y, user_index);
                break;
            case eOBJType::otForos:
                AccionParaForo(map, x, y, user_index);
                break;
            case eOBJType::otLeña:
                if (temp_index == FOGATA_APAG && user.flags.Muerto == 0) {
                    AccionParaRamita(map, x, y, user_index);
                }
                break;
            default:
                break;
        }
    }
    // Objetos multitile (validando límites de mapa)
    else if (InMapBounds(map, x + 1, y) && MapData[mapBlockIndex(map, x + 1, y)].ObjInfo.ObjIndex > 0) {
        int16_t temp_index = MapData[mapBlockIndex(map, x + 1, y)].ObjInfo.ObjIndex;
        user.flags.TargetObj = temp_index;

        if (ObjDataList[temp_index].OBJType == eOBJType::otPuertas) {
            AccionParaPuerta(map, x + 1, y, user_index);
        }
    }
    else if (InMapBounds(map, x + 1, y + 1) && MapData[mapBlockIndex(map, x + 1, y + 1)].ObjInfo.ObjIndex > 0) {
        int16_t temp_index = MapData[mapBlockIndex(map, x + 1, y + 1)].ObjInfo.ObjIndex;
        user.flags.TargetObj = temp_index;

        if (ObjDataList[temp_index].OBJType == eOBJType::otPuertas) {
            AccionParaPuerta(map, x + 1, y + 1, user_index);
        }
    }
    else if (InMapBounds(map, x, y + 1) && MapData[mapBlockIndex(map, x, y + 1)].ObjInfo.ObjIndex > 0) {
        int16_t temp_index = MapData[mapBlockIndex(map, x, y + 1)].ObjInfo.ObjIndex;
        user.flags.TargetObj = temp_index;

        if (ObjDataList[temp_index].OBJType == eOBJType::otPuertas) {
            AccionParaPuerta(map, x, y + 1, user_index);
        }
    }
}

void AccionParaForo(int16_t map, int16_t x, int16_t y, int16_t user_index) {
    if (user_index <= 0 || user_index > MaxUsers) return;
    if (!InMapBounds(map, x, y)) return;

    WorldPos pos{map, x, y};

    if (Distancia(pos, UserList[user_index].Pos) > 2) {
        if (s_callbacks.WriteConsoleMsg) {
            s_callbacks.WriteConsoleMsg(user_index, "Estas demasiado lejos.", FONTTYPE_INFO_VAL);
        }
        return;
    }

    std::size_t idx = mapBlockIndex(map, x, y);
    int16_t obj_index = MapData[idx].ObjInfo.ObjIndex;
    int16_t foro_id = ObjDataList[obj_index].ForoID.empty() ? 0 : static_cast<int16_t>(std::stoi(ObjDataList[obj_index].ForoID));

    bool sent = s_callbacks.SendPosts ? s_callbacks.SendPosts(user_index, foro_id) : false;
    if (sent) {
        if (s_callbacks.WriteShowForumForm) {
            s_callbacks.WriteShowForumForm(user_index);
        }
    }
}

void AccionParaPuerta(int16_t map, int16_t x, int16_t y, int16_t user_index) {
    if (user_index <= 0 || user_index > MaxUsers) return;
    if (!InMapBounds(map, x, y)) return;

    auto& user = UserList[user_index];

    if (Distance(user.Pos.X, user.Pos.Y, x, y) <= 2) {
        std::size_t idx = mapBlockIndex(map, x, y);
        int16_t obj_index = MapData[idx].ObjInfo.ObjIndex;

        if (ObjDataList[obj_index].Llave == 0) {
            if (ObjDataList[obj_index].Cerrada == 1) {
                // Abre la puerta
                int16_t abierta_index = ObjDataList[obj_index].IndexAbierta;
                MapData[idx].ObjInfo.ObjIndex = abierta_index;

                if (s_callbacks.SendObjectCreateToArea) {
                    s_callbacks.SendObjectCreateToArea(map, x, y, ObjDataList[abierta_index].GrhIndex);
                }

                // Desbloquea
                MapData[idx].Blocked = 0;
                if (s_callbacks.SetTileBlocked) {
                    s_callbacks.SetTileBlocked(true, map, x, y, 0);
                }

                // Resguardo de seguridad Bug #47: evita x-1 = 0
                if (x > 1) {
                    std::size_t idx_left = mapBlockIndex(map, x - 1, y);
                    MapData[idx_left].Blocked = 0;
                    if (s_callbacks.SetTileBlocked) {
                        s_callbacks.SetTileBlocked(true, map, x - 1, y, 0);
                    }
                }

                if (s_callbacks.SendPlayWaveToArea) {
                    s_callbacks.SendPlayWaveToArea(map, x, y, SND_PUERTA);
                }
            }
            else {
                // Cierra la puerta
                int16_t cerrada_index = ObjDataList[obj_index].IndexCerrada;
                MapData[idx].ObjInfo.ObjIndex = cerrada_index;

                if (s_callbacks.SendObjectCreateToArea) {
                    s_callbacks.SendObjectCreateToArea(map, x, y, ObjDataList[cerrada_index].GrhIndex);
                }

                MapData[idx].Blocked = 1;

                // Resguardo de seguridad Bug #47: evita x-1 = 0
                if (x > 1) {
                    std::size_t idx_left = mapBlockIndex(map, x - 1, y);
                    MapData[idx_left].Blocked = 1;
                    if (s_callbacks.SetTileBlocked) {
                        s_callbacks.SetTileBlocked(true, map, x - 1, y, 1);
                    }
                }

                if (s_callbacks.SetTileBlocked) {
                    s_callbacks.SetTileBlocked(true, map, x, y, 1);
                }

                if (s_callbacks.SendPlayWaveToArea) {
                    s_callbacks.SendPlayWaveToArea(map, x, y, SND_PUERTA);
                }
            }

            user.flags.TargetObj = MapData[idx].ObjInfo.ObjIndex;
        }
        else {
            if (s_callbacks.WriteConsoleMsg) {
                s_callbacks.WriteConsoleMsg(user_index, "La puerta está cerrada con llave.", FONTTYPE_INFO_VAL);
            }
        }
    }
    else {
        if (s_callbacks.WriteConsoleMsg) {
            s_callbacks.WriteConsoleMsg(user_index, "Estás demasiado lejos.", FONTTYPE_INFO_VAL);
        }
    }
}

void AccionParaCartel(int16_t map, int16_t x, int16_t y, int16_t user_index) {
    if (user_index <= 0 || user_index > MaxUsers) return;
    if (!InMapBounds(map, x, y)) return;

    std::size_t idx = mapBlockIndex(map, x, y);
    int16_t obj_index = MapData[idx].ObjInfo.ObjIndex;

    if (ObjDataList[obj_index].OBJType == eOBJType::otCarteles) {
        if (!ObjDataList[obj_index].texto.empty()) {
            if (s_callbacks.WriteShowSignal) {
                s_callbacks.WriteShowSignal(user_index, obj_index);
            }
        }
    }
}

void AccionParaRamita(int16_t map, int16_t x, int16_t y, int16_t user_index) {
    if (user_index <= 0 || user_index > MaxUsers) return;
    if (!InMapBounds(map, x, y)) return;

    uint8_t suerte = 1;
    uint8_t exito = 0;
    Obj obj{};

    WorldPos pos{map, x, y};
    auto& user = UserList[user_index];

    if (Distancia(pos, user.Pos) > 2) {
        if (s_callbacks.WriteConsoleMsg) {
            s_callbacks.WriteConsoleMsg(user_index, "Estás demasiado lejos.", FONTTYPE_INFO_VAL);
        }
        return;
    }

    std::size_t idx = mapBlockIndex(map, x, y);

    if (MapData[idx].trigger == eTrigger::ZONASEGURA || MapInfoList[map].Pk == false) {
        if (s_callbacks.WriteConsoleMsg) {
            s_callbacks.WriteConsoleMsg(user_index, "No puedes hacer fogatas en zona segura.", FONTTYPE_INFO_VAL);
        }
        return;
    }

    uint16_t skill = user.Stats.UserSkills[eSkill::Supervivencia];
    if (skill > 1 && skill < 6) {
        suerte = 3;
    }
    else if (skill >= 6 && skill <= 10) {
        suerte = 2;
    }
    else if (skill >= 10) {
        // Bug #48 replica la condición legacy And .Stats.UserSkills(Supervivencia)
        suerte = 1;
    }

    exito = static_cast<uint8_t>(RandomNumber(1, suerte));

    if (exito == 1) {
        if (MapInfoList[user.Pos.Map].Zona != Ciudad) {
            obj.ObjIndex = FOGATA;
            obj.Amount = 1;

            if (s_callbacks.WriteConsoleMsg) {
                s_callbacks.WriteConsoleMsg(user_index, "Has prendido la fogata.", FONTTYPE_INFO_VAL);
            }

            if (s_callbacks.MakeObj) {
                s_callbacks.MakeObj(obj, map, x, y);
            }

            // Integración con cGarbage / TrashCollector (Capa 0)
            cGarbage fogatita{map, x, y};
            TrashCollector.push_back(fogatita);
            if (s_callbacks.RegisterGarbageFire) {
                s_callbacks.RegisterGarbageFire(map, x, y);
            }

            if (s_callbacks.SubirSkill) {
                s_callbacks.SubirSkill(user_index, eSkill::Supervivencia, true);
            }
        }
        else {
            if (s_callbacks.WriteConsoleMsg) {
                s_callbacks.WriteConsoleMsg(user_index, "La ley impide realizar fogatas en las ciudades.", FONTTYPE_INFO_VAL);
            }
            return;
        }
    }
    else {
        if (s_callbacks.WriteConsoleMsg) {
            s_callbacks.WriteConsoleMsg(user_index, "No has podido hacer fuego.", FONTTYPE_INFO_VAL);
        }
        if (s_callbacks.SubirSkill) {
            s_callbacks.SubirSkill(user_index, eSkill::Supervivencia, false);
        }
    }
}

} // namespace Acciones
