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

bool IsValidMapIndex(int map) noexcept {
    return map > 0 && map <= NumMaps;
}

// ============================================================================
// Paso 2: Ruteo Geográfico (Áreas y Mapas)
// ============================================================================

void SendToUserArea(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendToUserAreaButIndex(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendToDeadUserArea(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendToAreaByPos(int map, int area_x, int area_y, std::span<const std::uint8_t> data) {
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendToMap(int map, std::span<const std::uint8_t> data) {
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendToMapButIndex(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendToNpcArea(int npc_index, std::span<const std::uint8_t> data) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    const int map = Npclist[npc_index].Pos.Map;
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendToGuildMembers(int guild_index, std::span<const std::uint8_t> data) {
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

void SendToDiosesYclan(int guild_index, std::span<const std::uint8_t> data) {
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

void SendToUserGuildArea(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const auto guild_index = UserList[user_index].GuildIndex;
    if (guild_index == 0) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendToUserPartyArea(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const auto party_index = UserList[user_index].PartyIndex;
    if (party_index == 0) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendToAll(std::span<const std::uint8_t> data) {
    const auto view = to_string_view(data);
    const int limit = std::min<int>(LastUser, static_cast<int>(UserList.size()) - 1);

    for (int i = 1; i <= limit; ++i) {
        const auto& u = UserList[i];
        if (u.ConnID != -1 && u.flags.UserLogged) {
            TCP::EnviarDatosASlot(static_cast<std::int16_t>(i), view);
        }
    }
}

void SendToAllButIndex(int user_index, std::span<const std::uint8_t> data) {
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

void SendToAdmins(std::span<const std::uint8_t> data) {
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

void SendToHigherAdmins(std::span<const std::uint8_t> data) {
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

void SendToConsejo(std::span<const std::uint8_t> data) {
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

void SendToConsejoCaos(std::span<const std::uint8_t> data) {
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

void SendToRolesMasters(std::span<const std::uint8_t> data) {
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

void SendToCiudadanos(std::span<const std::uint8_t> data) {
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

void SendToCriminales(std::span<const std::uint8_t> data) {
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

void SendToReal(std::span<const std::uint8_t> data) {
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

void SendToCaos(std::span<const std::uint8_t> data) {
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

void SendToCiudadanosYRMs(std::span<const std::uint8_t> data) {
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

void SendToCriminalesYRMs(std::span<const std::uint8_t> data) {
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

void SendToRealYRMs(std::span<const std::uint8_t> data) {
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

void SendToCaosYRMs(std::span<const std::uint8_t> data) {
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

void SendToAdminsButConsejerosArea(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendToGMsAreaButRmsOrCounselors(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendToUsersAreaButGMs(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendToUsersAndRmsAndCounselorsAreaButGMs(int user_index, std::span<const std::uint8_t> data) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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

void SendData(SendTarget route, int index, std::span<const std::uint8_t> data) {
    switch (route) {
        case SendTarget::ToAll:
            SendToAll(data);
            break;

        case SendTarget::ToMap:
            SendToMap(index, data);
            break;

        case SendTarget::ToPCArea:
            SendToUserArea(index, data);
            break;

        case SendTarget::ToAllButIndex:
            SendToAllButIndex(index, data);
            break;

        case SendTarget::ToMapButIndex:
            SendToMapButIndex(index, data);
            break;

        case SendTarget::ToNPCArea:
            SendToNpcArea(index, data);
            break;

        case SendTarget::ToGuildMembers:
            SendToGuildMembers(index, data);
            break;

        case SendTarget::ToAdmins:
            SendToAdmins(data);
            break;

        case SendTarget::ToPCAreaButIndex:
            SendToUserAreaButIndex(index, data);
            break;

        case SendTarget::ToAdminsAreaButConsejeros:
            SendToAdminsButConsejerosArea(index, data);
            break;

        case SendTarget::ToDiosesYclan:
            SendToDiosesYclan(index, data);
            break;

        case SendTarget::ToConsejo:
            SendToConsejo(data);
            break;

        case SendTarget::ToClanArea:
            SendToUserGuildArea(index, data);
            break;

        case SendTarget::ToConsejoCaos:
            SendToConsejoCaos(data);
            break;

        case SendTarget::ToRolesMasters:
            SendToRolesMasters(data);
            break;

        case SendTarget::ToDeadArea:
            SendToDeadUserArea(index, data);
            break;

        case SendTarget::ToCiudadanos:
            SendToCiudadanos(data);
            break;

        case SendTarget::ToCriminales:
            SendToCriminales(data);
            break;

        case SendTarget::ToPartyArea:
            SendToUserPartyArea(index, data);
            break;

        case SendTarget::ToReal:
            SendToReal(data);
            break;

        case SendTarget::ToCaos:
            SendToCaos(data);
            break;

        case SendTarget::ToCiudadanosYRMs:
            SendToCiudadanosYRMs(data);
            break;

        case SendTarget::ToCriminalesYRMs:
            SendToCriminalesYRMs(data);
            break;

        case SendTarget::ToRealYRMs:
            SendToRealYRMs(data);
            break;

        case SendTarget::ToCaosYRMs:
            SendToCaosYRMs(data);
            break;

        case SendTarget::ToHigherAdmins:
            SendToHigherAdmins(data);
            break;

        case SendTarget::ToGMsAreaButRmsOrCounselors:
            SendToGMsAreaButRmsOrCounselors(index, data);
            break;

        case SendTarget::ToUsersAreaButGMs:
            SendToUsersAreaButGMs(index, data);
            break;

        case SendTarget::ToUsersAndRmsAndCounselorsAreaButGMs:
            SendToUsersAndRmsAndCounselorsAreaButGMs(index, data);
            break;
    }
}

void AlertarFaccionarios(int user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const int map = UserList[user_index].Pos.Map;
    if (!IsValidMapIndex(map) || static_cast<std::size_t>(map) >= ConnGroups.size()) {
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
