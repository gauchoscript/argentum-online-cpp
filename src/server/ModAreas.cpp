#include "ModAreas.hpp"
#include <algorithm>
#include <array>
#include "Protocol.hpp"
#include "TCP.hpp"

namespace ModAreas {

namespace {

// Matriz estática 2D de identificadores de área (1..100, 1..100)
// Índice 0 reservado/no utilizado para mantener indexación base 1
std::array<std::array<std::uint8_t, 101>, 101> s_areas_info{};

// Tabla de máscaras de recepción adyacentes (franjas 0 a 11)
std::array<std::int16_t, 12> s_areas_recive{};

} // namespace

void InitAreas() {
    // 1. Setup AreasRecive: franjas 0 a 11 con máscaras de adyacencia de 3 bits
    for (std::int16_t loop_c = 0; loop_c <= 11; ++loop_c) {
        std::int16_t mask = static_cast<std::int16_t>(1 << loop_c);
        if (loop_c != 0) {
            mask |= static_cast<std::int16_t>(1 << (loop_c - 1));
        }
        if (loop_c != 11) {
            mask |= static_cast<std::int16_t>(1 << (loop_c + 1));
        }
        s_areas_recive[static_cast<std::size_t>(loop_c)] = mask;
    }

    // 2. Setup AreasInfo: matriz de 100x100 con la fórmula literal de VB6 (Bug #24)
    // Usamos 121 IDs de area para saber si pasamos de area "más rápido"
    for (std::int16_t loop_c = 1; loop_c <= 100; ++loop_c) {
        for (std::int16_t loop_x = 1; loop_x <= 100; ++loop_x) {
            s_areas_info[static_cast<std::size_t>(loop_c)][static_cast<std::size_t>(loop_x)] =
                static_cast<std::uint8_t>((loop_c / 9 + 1) * (loop_x / 9 + 1));
        }
    }

    // 3. Dimensionamiento inicial de ConnGroups para los mapas existentes
    if (NumMaps > 0) {
        ConnGroups.assign(static_cast<std::size_t>(NumMaps + 1), ConnGroup{});
        for (std::size_t map = 1; map <= static_cast<std::size_t>(NumMaps); ++map) {
            ConnGroups[map].CountEntrys = 0;
            ConnGroups[map].OptValue = 1;
            ConnGroups[map].UserEntrys.assign(1, 0);
        }
    } else {
        ConnGroups.assign(1, ConnGroup{});
        ConnGroups[0].UserEntrys.assign(1, 0);
    }

    // PosToArea y AreasStats.dat / AreasOptimizacion se omiten conforme a la política
    // de exclusión de código muerto y optimizaciones arcaicas obsoletas (Bug #26).
}

std::uint8_t GetAreaID(std::int16_t x, std::int16_t y) noexcept {
    if (x < 1 || x > 100 || y < 1 || y > 100) {
        return 0;
    }
    return s_areas_info[static_cast<std::size_t>(x)][static_cast<std::size_t>(y)];
}

std::int16_t GetAreaRecive(std::int16_t franja) noexcept {
    if (franja < 0 || franja > 11) {
        return 0;
    }
    return s_areas_recive[static_cast<std::size_t>(franja)];
}

namespace {

MakeUserCharHook s_make_user_char_hook{nullptr};
MakeNPCCharHook s_make_npc_char_hook{nullptr};
BloquearHook s_bloquear_hook{nullptr};

inline std::size_t get_map_block_index(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return (static_cast<std::size_t>(map) * 101 + static_cast<std::size_t>(x)) * 101 + static_cast<std::size_t>(y);
}

inline bool es_objeto_fijo(eOBJType tipo) noexcept {
    return tipo == eOBJType::otForos ||
           tipo == eOBJType::otCarteles ||
           tipo == eOBJType::otArboles ||
           tipo == eOBJType::otYacimiento;
}

void dispatch_make_user_char(bool to_map, std::int16_t snd_index, std::int16_t user_index,
                             std::int16_t map, std::int16_t x, std::int16_t y) {
    if (s_make_user_char_hook) {
        s_make_user_char_hook(to_map, snd_index, user_index, map, x, y);
    }
}

void dispatch_make_npc_char(bool to_map, std::int16_t snd_index, std::int16_t npc_index,
                            std::int16_t map, std::int16_t x, std::int16_t y) {
    if (s_make_npc_char_hook) {
        s_make_npc_char_hook(to_map, snd_index, npc_index, map, x, y);
    }
}

void dispatch_bloquear(bool to_map, std::int16_t snd_index, std::int16_t x, std::int16_t y, bool b) {
    if (s_bloquear_hook) {
        s_bloquear_hook(to_map, snd_index, x, y, b);
    } else {
        if (to_map) {
            // No utilizado en ModAreas con to_map = true
        } else {
            ao::net::protocol::WriteBlockPosition(snd_index, static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y), b);
        }
    }
}

} // namespace

void SetMakeUserCharHook(MakeUserCharHook hook) noexcept {
    s_make_user_char_hook = hook;
}

void SetMakeNPCCharHook(MakeNPCCharHook hook) noexcept {
    s_make_npc_char_hook = hook;
}

void SetBloquearHook(BloquearHook hook) noexcept {
    s_bloquear_hook = hook;
}

void CheckUpdateNeededUser(std::int16_t user_index, std::uint8_t head, bool but_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];

    // 1. Salida temprana si el AreaID coincide con la celda actual (Bug #24, ModAreas.bas:177)
    const std::uint8_t current_cell_area_id = GetAreaID(user.Pos.X, user.Pos.Y);
    if (user.AreasInfo.AreaID == current_cell_area_id) {
        return;
    }

    std::int32_t min_x = user.AreasInfo.MinX;
    std::int32_t min_y = user.AreasInfo.MinY;
    std::int32_t max_x = 0;
    std::int32_t max_y = 0;

    // 2. Cálculo de límites y franjas con aritmética literal de VB6 (Bug #25, ModAreas.bas:183-228)
    if (head == eHeading::NORTH) {
        max_y = min_y - 1;
        min_y = min_y - 9;
        max_x = min_x + 26;
        user.AreasInfo.MinX = static_cast<std::int16_t>(min_x);
        user.AreasInfo.MinY = static_cast<std::int16_t>(min_y);
    } else if (head == eHeading::SOUTH) {
        max_y = min_y + 35;
        min_y = min_y + 27;
        max_x = min_x + 26;
        user.AreasInfo.MinX = static_cast<std::int16_t>(min_x);
        user.AreasInfo.MinY = static_cast<std::int16_t>(min_y - 18);
    } else if (head == eHeading::WEST) {
        max_x = min_x - 1;
        min_x = min_x - 9;
        max_y = min_y + 26;
        user.AreasInfo.MinX = static_cast<std::int16_t>(min_x);
        user.AreasInfo.MinY = static_cast<std::int16_t>(min_y);
    } else if (head == eHeading::EAST) {
        max_x = min_x + 35;
        min_x = min_x + 27;
        max_y = min_y + 26;
        user.AreasInfo.MinX = static_cast<std::int16_t>(min_x - 18);
        user.AreasInfo.MinY = static_cast<std::int16_t>(min_y);
    } else if (head == USER_NUEVO) {
        min_y = ((static_cast<std::int32_t>(user.Pos.Y) / 9) - 1) * 9;
        max_y = min_y + 26;

        min_x = ((static_cast<std::int32_t>(user.Pos.X) / 9) - 1) * 9;
        max_x = min_x + 26;

        user.AreasInfo.MinX = static_cast<std::int16_t>(min_x);
        user.AreasInfo.MinY = static_cast<std::int16_t>(min_y);
    }

    // 3. Clamping local para el bucle de barrido de tiles del mapa (ModAreas.bas:231-234)
    if (min_y < 1) min_y = 1;
    if (min_x < 1) min_x = 1;
    if (max_y > 100) max_y = 100;
    if (max_x > 100) max_x = 100;

    const std::int16_t map = user.Pos.Map;

    // 4. Aviso de cambio de área al cliente para depuración de entidades fuera de rango (ModAreas.bas:238)
    ao::net::protocol::WriteAreaChanged(user_index);

    // 5. Barrido del área rectangular emergente (ModAreas.bas:241-300)
    for (std::int32_t x = min_x; x <= max_x; ++x) {
        for (std::int32_t y = min_y; y <= max_y; ++y) {
            const std::size_t block_idx = get_map_block_index(map, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
            if (block_idx >= MapData.size()) {
                continue;
            }

            const auto& block = MapData[block_idx];

            // <<< User >>>
            if (block.UserIndex != 0) {
                const std::int16_t temp_int = block.UserIndex;

                if (user_index != temp_int) {
                    if (temp_int > 0 && static_cast<std::size_t>(temp_int) < UserList.size()) {
                        const auto& other_user = UserList[static_cast<std::size_t>(temp_int)];

                        // Solo avisa al otro cliente si no es un admin invisible
                        if (other_user.flags.AdminInvisible != 1) {
                            dispatch_make_user_char(false, user_index, temp_int, map, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));

                            // Si el user estaba invisible le avisamos al nuevo cliente de eso
                            if (other_user.flags.invisible != 0 || other_user.flags.Oculto != 0) {
                                const auto privs = static_cast<std::uint32_t>(user.flags.Privilegios);
                                if (privs & (static_cast<std::uint32_t>(PlayerType::UserPlayer) |
                                             static_cast<std::uint32_t>(PlayerType::Consejero) |
                                             static_cast<std::uint32_t>(PlayerType::RoleMaster))) {
                                    ao::net::protocol::WriteSetInvisible(user_index, other_user.char_appearance.CharIndex, true);
                                }
                            }
                        }

                        // Solo avisa al otro cliente si el usuario en movimiento no es admin invisible
                        if (user.flags.AdminInvisible != 1) {
                            dispatch_make_user_char(false, temp_int, user_index, user.Pos.Map, user.Pos.X, user.Pos.Y);

                            if (user.flags.invisible != 0 || user.flags.Oculto != 0) {
                                const auto other_privs = static_cast<std::uint32_t>(other_user.flags.Privilegios);
                                if (other_privs & static_cast<std::uint32_t>(PlayerType::UserPlayer)) {
                                    ao::net::protocol::WriteSetInvisible(temp_int, user.char_appearance.CharIndex, true);
                                }
                            }
                        }

                        TCP::FlushBuffer(temp_int);
                    }
                } else if (head == USER_NUEVO) {
                    if (!but_index) {
                        dispatch_make_user_char(false, user_index, user_index, map, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
                    }
                }
            }

            // <<< Npc >>>
            if (block.NpcIndex != 0) {
                dispatch_make_npc_char(false, user_index, block.NpcIndex, map, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
            }

            // <<< Item >>>
            if (block.ObjInfo.ObjIndex != 0) {
                const std::int16_t item_obj_index = block.ObjInfo.ObjIndex;
                if (item_obj_index > 0 && static_cast<std::size_t>(item_obj_index) < ObjDataList.size()) {
                    const auto& obj = ObjDataList[static_cast<std::size_t>(item_obj_index)];
                    if (!es_objeto_fijo(static_cast<eOBJType>(obj.OBJType))) {
                        ao::net::protocol::WriteObjectCreate(user_index, obj.GrhIndex,
                                                             static_cast<std::uint8_t>(x), static_cast<std::uint8_t>(y));

                        if (obj.OBJType == eOBJType::otPuertas) {
                            dispatch_bloquear(false, user_index, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y), block.Blocked != 0);

                            if (x > 1) {
                                const std::size_t left_block_idx = get_map_block_index(map, static_cast<std::int16_t>(x - 1), static_cast<std::int16_t>(y));
                                if (left_block_idx < MapData.size()) {
                                    dispatch_bloquear(false, user_index, static_cast<std::int16_t>(x - 1), static_cast<std::int16_t>(y),
                                                      MapData[left_block_idx].Blocked != 0);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 6. Actualización de máscaras y precalculados (ModAreas.bas:303-311)
    const std::int16_t temp_franja_x = static_cast<std::int16_t>(user.Pos.X / 9);
    user.AreasInfo.AreaReciveX = GetAreaRecive(temp_franja_x);
    user.AreasInfo.AreaPerteneceX = static_cast<std::int16_t>(1 << temp_franja_x);

    const std::int16_t temp_franja_y = static_cast<std::int16_t>(user.Pos.Y / 9);
    user.AreasInfo.AreaReciveY = GetAreaRecive(temp_franja_y);
    user.AreasInfo.AreaPerteneceY = static_cast<std::int16_t>(1 << temp_franja_y);

    user.AreasInfo.AreaID = current_cell_area_id;
}

void AgregarUser(std::int16_t user_index, std::int16_t map, bool but_index) {
    if (map < 1 || map > NumMaps || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    auto& group = ConnGroups[static_cast<std::size_t>(map)];
    if (group.UserEntrys.empty()) {
        group.UserEntrys.assign(1, 0);
    }

    bool es_nuevo = true;

    // Búsqueda lineal 1-based para prevenir duplicados (ModAreas.bas:411-416)
    for (std::int32_t i = 1; i <= group.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) < group.UserEntrys.size() && group.UserEntrys[static_cast<std::size_t>(i)] == user_index) {
            es_nuevo = false;
            break;
        }
    }

    if (es_nuevo) {
        group.UserEntrys.push_back(user_index);
        group.CountEntrys = static_cast<std::int32_t>(group.UserEntrys.size() - 1);
    }

    // Blanqueo de áreas del usuario
    if (user_index > 0 && static_cast<std::size_t>(user_index) < UserList.size()) {
        auto& areas = UserList[static_cast<std::size_t>(user_index)].AreasInfo;
        areas.AreaID = 0;
        areas.AreaPerteneceX = 0;
        areas.AreaPerteneceY = 0;
        areas.AreaReciveX = 0;
        areas.AreaReciveY = 0;
    }

    CheckUpdateNeededUser(user_index, USER_NUEVO, but_index);
}

void QuitarUser(std::int16_t user_index, std::int16_t map) {
    if (map < 1 || map > NumMaps || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    auto& group = ConnGroups[static_cast<std::size_t>(map)];
    if (group.UserEntrys.empty()) {
        return;
    }

    std::int32_t loop_c = 1;
    for (; loop_c <= group.CountEntrys; ++loop_c) {
        if (static_cast<std::size_t>(loop_c) < group.UserEntrys.size() &&
            group.UserEntrys[static_cast<std::size_t>(loop_c)] == user_index) {
            break;
        }
    }

    // Usuario no encontrado en este mapa (ModAreas.bas:380)
    if (loop_c > group.CountEntrys) {
        return;
    }

    // Remoción y corrimiento lineal a la izquierda preservando contigüidad y orden
    group.UserEntrys.erase(group.UserEntrys.begin() + loop_c);
    group.CountEntrys = static_cast<std::int32_t>(group.UserEntrys.size() - 1);
}

void CheckUpdateNeededNpc(std::int16_t npc_index, std::uint8_t head) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& current_npc = Npclist[static_cast<std::size_t>(npc_index)];

    // 1. Salida temprana si el AreaID coincide con la celda actual (Bug #24, ModAreas.bas:317)
    const std::uint8_t current_cell_area_id = GetAreaID(current_npc.Pos.X, current_npc.Pos.Y);
    if (current_npc.AreasInfo.AreaID == current_cell_area_id) {
        return;
    }

    std::int32_t min_x = current_npc.AreasInfo.MinX;
    std::int32_t min_y = current_npc.AreasInfo.MinY;
    std::int32_t max_x = 0;
    std::int32_t max_y = 0;

    // 2. Cálculo de límites y franjas con aritmética literal de VB6 (Bug #25, ModAreas.bas:323-368)
    if (head == eHeading::NORTH) {
        max_y = min_y - 1;
        min_y = min_y - 9;
        max_x = min_x + 26;
        current_npc.AreasInfo.MinX = static_cast<std::int16_t>(min_x);
        current_npc.AreasInfo.MinY = static_cast<std::int16_t>(min_y);
    } else if (head == eHeading::SOUTH) {
        max_y = min_y + 35;
        min_y = min_y + 27;
        max_x = min_x + 26;
        current_npc.AreasInfo.MinX = static_cast<std::int16_t>(min_x);
        current_npc.AreasInfo.MinY = static_cast<std::int16_t>(min_y - 18);
    } else if (head == eHeading::WEST) {
        max_x = min_x - 1;
        min_x = min_x - 9;
        max_y = min_y + 26;
        current_npc.AreasInfo.MinX = static_cast<std::int16_t>(min_x);
        current_npc.AreasInfo.MinY = static_cast<std::int16_t>(min_y);
    } else if (head == eHeading::EAST) {
        max_x = min_x + 35;
        min_x = min_x + 27;
        max_y = min_y + 26;
        current_npc.AreasInfo.MinX = static_cast<std::int16_t>(min_x - 18);
        current_npc.AreasInfo.MinY = static_cast<std::int16_t>(min_y);
    } else if (head == USER_NUEVO) {
        min_y = ((static_cast<std::int32_t>(current_npc.Pos.Y) / 9) - 1) * 9;
        max_y = min_y + 26;

        min_x = ((static_cast<std::int32_t>(current_npc.Pos.X) / 9) - 1) * 9;
        max_x = min_x + 26;

        current_npc.AreasInfo.MinX = static_cast<std::int16_t>(min_x);
        current_npc.AreasInfo.MinY = static_cast<std::int16_t>(min_y);
    }

    // 3. Clamping local para el bucle de barrido de tiles del mapa (ModAreas.bas:370-373)
    if (min_y < 1) min_y = 1;
    if (min_x < 1) min_x = 1;
    if (max_y > 100) max_y = 100;
    if (max_x > 100) max_x = 100;

    const std::int16_t map = current_npc.Pos.Map;

    // 4. Barrido del área rectangular emergente condicionado a NumUsers != 0 (ModAreas.bas:376-383)
    if (map > 0 && static_cast<std::size_t>(map) < MapInfoList.size() && MapInfoList[static_cast<std::size_t>(map)].NumUsers != 0) {
        for (std::int32_t x = min_x; x <= max_x; ++x) {
            for (std::int32_t y = min_y; y <= max_y; ++y) {
                const std::size_t block_idx = get_map_block_index(map, static_cast<std::int16_t>(x), static_cast<std::int16_t>(y));
                if (block_idx < MapData.size()) {
                    const std::int16_t target_user = MapData[block_idx].UserIndex;
                    if (target_user != 0) {
                        dispatch_make_npc_char(false, target_user, npc_index, map, current_npc.Pos.X, current_npc.Pos.Y);
                    }
                }
            }
        }
    }

    // 5. Actualización de máscaras y precalculados (ModAreas.bas:385-394)
    // Preservación de la asimetría: NO se ejecuta FlushBuffer aquí a diferencia de usuarios.
    const std::int16_t temp_franja_x = static_cast<std::int16_t>(current_npc.Pos.X / 9);
    current_npc.AreasInfo.AreaReciveX = GetAreaRecive(temp_franja_x);
    current_npc.AreasInfo.AreaPerteneceX = static_cast<std::int16_t>(1 << temp_franja_x);

    const std::int16_t temp_franja_y = static_cast<std::int16_t>(current_npc.Pos.Y / 9);
    current_npc.AreasInfo.AreaReciveY = GetAreaRecive(temp_franja_y);
    current_npc.AreasInfo.AreaPerteneceY = static_cast<std::int16_t>(1 << temp_franja_y);

    current_npc.AreasInfo.AreaID = current_cell_area_id;
}

void AgregarNpc(std::int16_t npc_index) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& current_npc = Npclist[static_cast<std::size_t>(npc_index)];
    current_npc.AreasInfo.AreaID = 0;
    current_npc.AreasInfo.AreaPerteneceX = 0;
    current_npc.AreasInfo.AreaPerteneceY = 0;
    current_npc.AreasInfo.AreaReciveX = 0;
    current_npc.AreasInfo.AreaReciveY = 0;

    CheckUpdateNeededNpc(npc_index, USER_NUEVO);
}

} // namespace ModAreas
