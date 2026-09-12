#include "modSendData.hpp"
#include "Declares.hpp"
#include "FileIO.hpp"
#include "modGuilds.hpp"
#include "TCP.hpp"

#include <algorithm>
#include <string>

namespace ao::net::send_data {

namespace {

[[nodiscard]] inline std::string_view to_string_view(std::span<const std::uint8_t> data) noexcept {
    return std::string_view(reinterpret_cast<const char*>(data.data()), data.size());
}

} // namespace

// ============================================================================
// Paso 1: Helpers de Validación de Mapa
// ============================================================================

bool is_valid_map_index(int map) noexcept {
    return map > 0 && map <= NumMaps;
}

// ============================================================================
// Paso 2: Ruteo Geográfico (Áreas y Mapas)
// ============================================================================

void send_to_user_area(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const int area_x = UserList[user_index].AreasInfo.AreaPerteneceX;
    const int area_y = UserList[user_index].AreasInfo.AreaPerteneceY;
    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        const auto& target = UserList[temp_index];
        if ((target.AreasInfo.AreaReciveX & area_x) && (target.AreasInfo.AreaReciveY & area_y)) {
            if (target.ConnIDValida) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
            }
        }
    }
}

void send_to_user_area_but_index(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const int area_x = UserList[user_index].AreasInfo.AreaPerteneceX;
    const int area_y = UserList[user_index].AreasInfo.AreaPerteneceY;
    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        if (temp_index != user_index) {
            const auto& target = UserList[temp_index];
            if ((target.AreasInfo.AreaReciveX & area_x) && (target.AreasInfo.AreaReciveY & area_y)) {
                if (target.ConnIDValida) {
                    TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
                }
            }
        }
    }
}

void send_to_dead_user_area(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const int area_x = UserList[user_index].AreasInfo.AreaPerteneceX;
    const int area_y = UserList[user_index].AreasInfo.AreaPerteneceY;
    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        const auto& target = UserList[temp_index];
        if ((target.AreasInfo.AreaReciveX & area_x) && (target.AreasInfo.AreaReciveY & area_y)) {
            if (target.ConnIDValida) {
                const bool es_staff = (target.flags.Privilegios & (PlayerType::Admin | PlayerType::Dios | PlayerType::SemiDios | PlayerType::Consejero)) != 0;
                if (target.flags.Muerto == 1 || es_staff) {
                    TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
                }
            }
        }
    }
}

void send_to_area_by_pos(int map, int area_x, int area_y, std::span<const std::uint8_t> data) {
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const int mask_x = area_pertenece_mask(area_x);
    const int mask_y = area_pertenece_mask(area_y);
    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        const auto& target = UserList[temp_index];
        if ((target.AreasInfo.AreaReciveX & mask_x) && (target.AreasInfo.AreaReciveY & mask_y)) {
            if (target.ConnIDValida) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
            }
        }
    }
}

void send_to_map(int map, std::span<const std::uint8_t> data) {
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        if (UserList[temp_index].ConnIDValida) {
            TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
        }
    }
}

void send_to_map_but_index(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        if (temp_index != user_index && UserList[temp_index].ConnIDValida) {
            TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
        }
    }
}

void send_to_npc_area(int npc_index, std::span<const std::uint8_t> data) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    const int map = Npclist[npc_index].Pos.Map;
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const int area_x = Npclist[npc_index].AreasInfo.AreaPerteneceX;
    const int area_y = Npclist[npc_index].AreasInfo.AreaPerteneceY;
    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        const auto& target = UserList[temp_index];
        if ((target.AreasInfo.AreaReciveX & area_x) && (target.AreasInfo.AreaReciveY & area_y)) {
            if (target.ConnIDValida) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
            }
        }
    }
}

// ============================================================================
// Paso 3: Ruteo Social e Iteradores de Clanes y Parties
// ============================================================================

void send_to_guild_members(int guild_index, std::span<const std::uint8_t> data) {
    if (guild_index <= 0) {
        return;
    }

    const auto view = to_string_view(data);
    std::int16_t loop_c = modGuilds::m_Iterador_ProximoUserIndex(static_cast<std::int16_t>(guild_index));
    while (loop_c > 0) {
        if (static_cast<std::size_t>(loop_c) < UserList.size()) {
            if (UserList[loop_c].ConnID != -1 && UserList[loop_c].flags.UserLogged) {
                TCP::EnviarDatosASlot(loop_c, view);
            }
        }
        loop_c = modGuilds::m_Iterador_ProximoUserIndex(static_cast<std::int16_t>(guild_index));
    }
}

void send_to_dioses_y_clan(int guild_index, std::span<const std::uint8_t> data) {
    if (guild_index <= 0) {
        return;
    }

    const auto view = to_string_view(data);
    std::int16_t loop_c = modGuilds::m_Iterador_ProximoUserIndex(static_cast<std::int16_t>(guild_index));
    while (loop_c > 0) {
        if (static_cast<std::size_t>(loop_c) < UserList.size()) {
            if (UserList[loop_c].ConnID != -1 && UserList[loop_c].flags.UserLogged) {
                TCP::EnviarDatosASlot(loop_c, view);
            }
        }
        loop_c = modGuilds::m_Iterador_ProximoUserIndex(static_cast<std::int16_t>(guild_index));
    }

    loop_c = modGuilds::Iterador_ProximoGM(static_cast<std::int16_t>(guild_index));
    while (loop_c > 0) {
        if (static_cast<std::size_t>(loop_c) < UserList.size()) {
            if (UserList[loop_c].ConnID != -1 && UserList[loop_c].flags.UserLogged) {
                TCP::EnviarDatosASlot(loop_c, view);
            }
        }
        loop_c = modGuilds::Iterador_ProximoGM(static_cast<std::int16_t>(guild_index));
    }
}

void send_to_user_guild_area(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const auto guild_index = UserList[user_index].GuildIndex;
    if (guild_index == 0) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const int area_x = UserList[user_index].AreasInfo.AreaPerteneceX;
    const int area_y = UserList[user_index].AreasInfo.AreaPerteneceY;
    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        const auto& target = UserList[temp_index];
        if ((target.AreasInfo.AreaReciveX & area_x) && (target.AreasInfo.AreaReciveY & area_y)) {
            const bool es_dios_sin_rm = (target.flags.Privilegios & PlayerType::Dios) && !(target.flags.Privilegios & PlayerType::RoleMaster);
            if (target.ConnIDValida && (target.GuildIndex == guild_index || es_dios_sin_rm)) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
            }
        }
    }
}

void send_to_user_party_area(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const auto party_index = UserList[user_index].PartyIndex;
    if (party_index == 0) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const int area_x = UserList[user_index].AreasInfo.AreaPerteneceX;
    const int area_y = UserList[user_index].AreasInfo.AreaPerteneceY;
    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        const auto& target = UserList[temp_index];
        if ((target.AreasInfo.AreaReciveX & area_x) && (target.AreasInfo.AreaReciveY & area_y)) {
            if (target.ConnIDValida && target.PartyIndex == party_index) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
            }
        }
    }
}

// ============================================================================
// Paso 4: Ruteo de Privilegios y Facciones (con Parche Pre-Login Bug #22)
// ============================================================================

void send_to_all(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        if (u.ConnID != -1 && u.flags.UserLogged) {
            TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
        }
    }
}

void send_to_all_but_index(int user_index, std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        if (i != user_index) {
            const auto& u = UserList[i];
            if (u.ConnID != -1 && u.flags.UserLogged) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_admins(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22: verificación explícita de UserLogged
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (u.flags.Privilegios & (PlayerType::Admin | PlayerType::Dios | PlayerType::SemiDios | PlayerType::Consejero)) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_higher_admins(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22: verificación explícita de UserLogged
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (u.flags.Privilegios & (PlayerType::Admin | PlayerType::Dios)) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_consejo(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (u.flags.Privilegios & PlayerType::RoyalCouncil) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_consejo_caos(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (u.flags.Privilegios & PlayerType::ChaosCouncil) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_roles_masters(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (u.flags.Privilegios & PlayerType::RoleMaster) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_ciudadanos(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (!FileIO::criminal(i)) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_criminales(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (FileIO::criminal(i)) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_real(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (u.Faccion.ArmadaReal == 1) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_caos(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (u.Faccion.FuerzasCaos == 1) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_ciudadanos_y_rms(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (!FileIO::criminal(i) || (u.flags.Privilegios & PlayerType::RoleMaster) != 0) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_criminales_y_rms(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (FileIO::criminal(i) || (u.flags.Privilegios & PlayerType::RoleMaster) != 0) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_real_y_rms(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (u.Faccion.ArmadaReal == 1 || (u.flags.Privilegios & PlayerType::RoleMaster) != 0) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_caos_y_rms(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        // Mitigación Bug #22
        if (u.ConnID != -1 && u.flags.UserLogged) {
            if (u.Faccion.FuerzasCaos == 1 || (u.flags.Privilegios & PlayerType::RoleMaster) != 0) {
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
            }
        }
    }
}

void send_to_admins_but_consejeros_area(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const int area_x = UserList[user_index].AreasInfo.AreaPerteneceX;
    const int area_y = UserList[user_index].AreasInfo.AreaPerteneceY;
    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        const auto& target = UserList[temp_index];
        if ((target.AreasInfo.AreaReciveX & area_x) && (target.AreasInfo.AreaReciveY & area_y)) {
            if (target.ConnIDValida) {
                if (target.flags.Privilegios & (PlayerType::SemiDios | PlayerType::Dios | PlayerType::Admin)) {
                    TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
                }
            }
        }
    }
}

void send_to_gms_area_but_rms_or_counselors(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const int area_x = UserList[user_index].AreasInfo.AreaPerteneceX;
    const int area_y = UserList[user_index].AreasInfo.AreaPerteneceY;
    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        const auto& target = UserList[temp_index];
        if ((target.AreasInfo.AreaReciveX & area_x) && (target.AreasInfo.AreaReciveY & area_y)) {
            if (target.ConnIDValida) {
                const auto priv = target.flags.Privilegios;
                // Exclusivo para dioses, admins y gms (sin bits de User, Consejero ni RoleMaster)
                if ((priv & ~PlayerType::UserPlayer & ~PlayerType::Consejero & ~PlayerType::RoleMaster) == priv) {
                    TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
                }
            }
        }
    }
}

void send_to_users_area_but_gms(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const int area_x = UserList[user_index].AreasInfo.AreaPerteneceX;
    const int area_y = UserList[user_index].AreasInfo.AreaPerteneceY;
    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        const auto& target = UserList[temp_index];
        if ((target.AreasInfo.AreaReciveX & area_x) && (target.AreasInfo.AreaReciveY & area_y)) {
            if (target.ConnIDValida) {
                if (target.flags.Privilegios & PlayerType::UserPlayer) {
                    TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
                }
            }
        }
    }
}

void send_to_users_and_rms_and_counselors_area_but_gms(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const int area_x = UserList[user_index].AreasInfo.AreaPerteneceX;
    const int area_y = UserList[user_index].AreasInfo.AreaPerteneceY;
    const auto& cg = ConnGroups[map];
    const auto view = to_string_view(data);

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        const auto& target = UserList[temp_index];
        if ((target.AreasInfo.AreaReciveX & area_x) && (target.AreasInfo.AreaReciveY & area_y)) {
            if (target.ConnIDValida) {
                if (target.flags.Privilegios & (PlayerType::UserPlayer | PlayerType::Consejero | PlayerType::RoleMaster)) {
                    TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), view);
                }
            }
        }
    }
}

// ============================================================================
// Paso 5: Dispatcher Maestro SendData y AlertarFaccionarios
// ============================================================================

void send_data(SendTarget route, int index, std::span<const std::uint8_t> data) {
    switch (route) {
        case SendTarget::ToAll:
            send_to_all(data);
            break;

        case SendTarget::ToMap:
            send_to_map(index, data);
            break;

        case SendTarget::ToPCArea:
            send_to_user_area(index, data);
            break;

        case SendTarget::ToAllButIndex:
            send_to_all_but_index(index, data);
            break;

        case SendTarget::ToMapButIndex:
            send_to_map_but_index(index, data);
            break;

        case SendTarget::ToNPCArea:
            send_to_npc_area(index, data);
            break;

        case SendTarget::ToGuildMembers:
            send_to_guild_members(index, data);
            break;

        case SendTarget::ToAdmins:
            send_to_admins(data);
            break;

        case SendTarget::ToPCAreaButIndex:
            send_to_user_area_but_index(index, data);
            break;

        case SendTarget::ToAdminsAreaButConsejeros:
            send_to_admins_but_consejeros_area(index, data);
            break;

        case SendTarget::ToDiosesYclan:
            send_to_dioses_y_clan(index, data);
            break;

        case SendTarget::ToConsejo:
            send_to_consejo(data);
            break;

        case SendTarget::ToClanArea:
            send_to_user_guild_area(index, data);
            break;

        case SendTarget::ToConsejoCaos:
            send_to_consejo_caos(data);
            break;

        case SendTarget::ToRolesMasters:
            send_to_roles_masters(data);
            break;

        case SendTarget::ToDeadArea:
            send_to_dead_user_area(index, data);
            break;

        case SendTarget::ToCiudadanos:
            send_to_ciudadanos(data);
            break;

        case SendTarget::ToCriminales:
            send_to_criminales(data);
            break;

        case SendTarget::ToPartyArea:
            send_to_user_party_area(index, data);
            break;

        case SendTarget::ToReal:
            send_to_real(data);
            break;

        case SendTarget::ToCaos:
            send_to_caos(data);
            break;

        case SendTarget::ToCiudadanosYRMs:
            send_to_ciudadanos_y_rms(data);
            break;

        case SendTarget::ToCriminalesYRMs:
            send_to_criminales_y_rms(data);
            break;

        case SendTarget::ToRealYRMs:
            send_to_real_y_rms(data);
            break;

        case SendTarget::ToCaosYRMs:
            send_to_caos_y_rms(data);
            break;

        case SendTarget::ToHigherAdmins:
            send_to_higher_admins(data);
            break;

        case SendTarget::ToGMsAreaButRmsOrCounselors:
            send_to_gms_area_but_rms_or_counselors(index, data);
            break;

        case SendTarget::ToUsersAreaButGMs:
            send_to_users_area_but_gms(index, data);
            break;

        case SendTarget::ToUsersAndRmsAndCounselorsAreaButGMs:
            send_to_users_and_rms_and_counselors_area_but_gms(index, data);
            break;
    }
}

void alertar_faccionarios(int user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!is_valid_map_index(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
        return;
    }

    const auto& sender = UserList[user_index];
    const auto& cg = ConnGroups[map];

    for (std::int32_t i = 1; i <= cg.CountEntrys; ++i) {
        if (static_cast<std::size_t>(i) >= cg.UserEntrys.size()) {
            break;
        }
        const auto temp_index = cg.UserEntrys[i];
        if (temp_index <= 0 || static_cast<std::size_t>(temp_index) >= UserList.size()) {
            continue;
        }

        if (temp_index == user_index) {
            continue;
        }

        const auto& target = UserList[temp_index];
        if (target.ConnIDValida) {
            const bool same_faccion = 
                (sender.Faccion.ArmadaReal == 1 && target.Faccion.ArmadaReal == 1) ||
                (sender.Faccion.FuerzasCaos == 1 && target.Faccion.FuerzasCaos == 1);

            if (same_faccion) {
                // Cálculo de orientación cardinal (GetDireccion)
                const int dx = sender.Pos.X - target.Pos.X;
                const int dy = sender.Pos.Y - target.Pos.Y;
                std::string direccion;

                if (dy < 0) {
                    direccion = "Norte";
                } else if (dy > 0) {
                    direccion = "Sur";
                }

                if (dx < 0) {
                    direccion += "Oeste";
                } else if (dx > 0) {
                    direccion += "Este";
                }

                const std::string mensaje = "Escuchas el llamado de un compañero que proviene del " + direccion;
                TCP::EnviarDatosASlot(static_cast<std::int16_t>(temp_index), mensaje);
            }
        }
    }
}

} // namespace ao::net::send_data
