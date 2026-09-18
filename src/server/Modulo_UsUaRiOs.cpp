#include "Modulo_UsUaRiOs.hpp"
#include "Declares.hpp"
#include "TCP.hpp"
#include "Protocol.hpp"
#include "ModAreas.hpp"
#include "FileIO.hpp"
#include "MODULO_NPCs.hpp"
#include "modSendData.hpp"
#include "modGuilds.hpp"
#include "InvUsuario.hpp"
#include "Matematicas.hpp"
#include "ModFacciones.hpp"
#include "mdParty.hpp"
#include "mdlCOmercioConUsuario.hpp"

#include <cmath>
#include <limits>
#include <array>

namespace Modulo_UsUaRiOs {

using ao::net::protocol::FontTypeNames;

// ==========================================
// Fase G1: Ciclo de Vida y Gestión de Slots
// ==========================================

int NextOpenUser() {
    return TCP::NextOpenUser();
}

int NextOpenCharIndex() {
    for (int i = 1; i <= MAXCHARS; ++i) {
        if (CharList[i] == 0) {
            return i;
        }
    }
    return 0;
}

void Cerrar_Usuario(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) {
        return;
    }
    TCP::CerrarUsuario(static_cast<std::int16_t>(user_index));
}

void CancelExit(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) {
        return;
    }

    auto& user = UserList[user_index];
    if (!user.Counters.Saliendo) {
        return;
    }

    if (user.ConnIDValida) {
        user.Counters.Saliendo = false;
        user.Counters.Salir = 0;
        ao::net::protocol::WriteConsoleMsg(static_cast<std::int16_t>(user_index), "/salir cancelado.", FontTypeNames::FONTTYPE_WARNING);
    } else {
        // Replicación del Bug #52:
        // Si el socket se cayó (!ConnIDValida), se reasigna el temporizador de salida sin cancelar Saliendo ni liberar el slot.
        bool is_user_player = (user.flags.Privilegios & PlayerType::UserPlayer) != 0;
        bool is_pk_map = false;
        if (user.Pos.Map >= 1 && static_cast<std::size_t>(user.Pos.Map) < MapInfoList.size()) {
            is_pk_map = MapInfoList[user.Pos.Map].Pk;
        }
        user.Counters.Salir = (is_user_player && is_pk_map) ? IntervaloCerrarConexion : 0;
    }
}

void CambiarNick(int user_index, int target_user_index, const std::string& new_nick) {
    if (user_index < 1 || user_index > MaxUsers || target_user_index < 1 || target_user_index > MaxUsers) {
        return;
    }

    auto& target_user = UserList[target_user_index];
    if (!target_user.flags.UserLogged) {
        return;
    }

    if (new_nick.empty() || new_nick.length() > 30) {
        return;
    }

    target_user.name = new_nick;
}


// ==========================================
// Fase G2: Cinemática, Transporte y Mascotas
// ==========================================

static inline bool InMapBounds(int map, int x, int y) {
    return (map >= 1 && static_cast<std::size_t>(map) < MapInfoList.size() &&
            x >= 1 && x <= 100 && y >= 1 && y <= 100);
}

static inline std::size_t mapBlockIndex(int map, int x, int y) {
    return (static_cast<std::size_t>(map - 1) * 10000) +
           (static_cast<std::size_t>(y - 1) * 100) +
           static_cast<std::size_t>(x - 1);
}

static inline bool HayAgua(int map, int x, int y) {
    if (!InMapBounds(map, x, y)) return false;
    std::size_t idx = mapBlockIndex(map, x, y);
    if (idx >= MapData.size()) return false;
    const auto& cell = MapData[idx];
    auto g1 = cell.Graphic[1];
    auto g2 = cell.Graphic[2];
    if (((g1 >= 1505 && g1 <= 1520) || (g1 >= 5665 && g1 <= 5680) || (g1 >= 13547 && g1 <= 13562)) && g2 == 0) {
        return true;
    }
    return false;
}

static inline bool MoveToLegalPos(int map, int x, int y, bool puede_agua, bool puede_tierra) {
    if (!InMapBounds(map, x, y)) return false;
    std::size_t idx = mapBlockIndex(map, x, y);
    if (idx >= MapData.size()) return false;
    const auto& cell = MapData[idx];

    if (cell.Blocked == 1 || cell.NpcIndex > 0) return false;

    int user_idx = cell.UserIndex;
    if (user_idx > 0) {
        if (user_idx >= 1 && user_idx <= MaxUsers) {
            bool is_dead = (UserList[user_idx].flags.Muerto == 1);
            bool is_admin_invis = (UserList[user_idx].flags.AdminInvisible == 1);
            if (!is_dead && !is_admin_invis) return false;
        } else {
            return false;
        }
    }

    bool water = HayAgua(map, x, y);
    if (puede_agua && puede_tierra) return true;
    if (puede_tierra && !puede_agua) return !water;
    if (puede_agua && !puede_tierra) return water;
    return false;
}

static inline bool LegalPos(int map, int x, int y, bool puede_agua, bool puede_tierra) {
    if (!InMapBounds(map, x, y)) return false;
    std::size_t idx = mapBlockIndex(map, x, y);
    if (idx >= MapData.size()) return false;
    const auto& cell = MapData[idx];

    if (cell.Blocked == 1 || cell.UserIndex > 0 || cell.NpcIndex > 0) return false;

    bool water = HayAgua(map, x, y);
    if (puede_agua && puede_tierra) return true;
    if (puede_tierra && !puede_agua) return !water;
    if (puede_agua && !puede_tierra) return water;
    return false;
}

uint8_t GetNickColor(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return 4;
    const auto& user = UserList[user_index];
    if ((user.flags.Privilegios & (PlayerType::Admin | PlayerType::Dios | PlayerType::SemiDios | PlayerType::Consejero)) != 0) {
        return 0; // GM
    }
    if (user.Faccion.ArmadaReal == 1) return 1;
    if (user.Faccion.FuerzasCaos == 1) return 2;
    if (FileIO::criminal(user_index)) return 3;
    return 4; // Ciudadano
}

eHeading InvertHeading(eHeading heading) {
    switch (heading) {
        case eHeading::NORTH: return eHeading::SOUTH;
        case eHeading::SOUTH: return eHeading::NORTH;
        case eHeading::EAST:  return eHeading::WEST;
        case eHeading::WEST:  return eHeading::EAST;
        default: return heading;
    }
}

bool PuedeAtravesarAgua(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return false;
    const auto& user = UserList[user_index];
    return (user.flags.Navegando == 1 || user.flags.Muerto == 1);
}

bool BodyIsBoat(int body) {
    return (body == iFragataReal || body == iFragataCaos || body == iBarcaPk ||
            body == iGaleraPk || body == iGaleonPk || body == iBarcaCiuda ||
            body == iGaleraCiuda || body == iGaleonCiuda || body == iFragataFantasmal);
}

void ToogleBoatBody(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];
    user.char_appearance.Head = 0;

    if (user.Faccion.ArmadaReal == 1) {
        user.char_appearance.body = iFragataReal;
    } else if (user.Faccion.FuerzasCaos == 1) {
        user.char_appearance.body = iFragataCaos;
    } else {
        int ropaje = 0;
        if (user.Invent.BarcoObjIndex >= 1 && static_cast<std::size_t>(user.Invent.BarcoObjIndex) < ObjDataList.size()) {
            ropaje = ObjDataList[user.Invent.BarcoObjIndex].Ropaje;
        }

        if (FileIO::criminal(user_index)) {
            switch (ropaje) {
                case iBarca:  user.char_appearance.body = iBarcaPk; break;
                case iGalera: user.char_appearance.body = iGaleraPk; break;
                case iGaleon: user.char_appearance.body = iGaleonPk; break;
                default: user.char_appearance.body = iBarcaPk; break;
            }
        } else {
            switch (ropaje) {
                case iBarca:  user.char_appearance.body = iBarcaCiuda; break;
                case iGalera: user.char_appearance.body = iGaleraCiuda; break;
                case iGaleon: user.char_appearance.body = iGaleonCiuda; break;
                default: user.char_appearance.body = iBarcaCiuda; break;
            }
        }
    }

    user.char_appearance.ShieldAnim = NingunEscudo;
    user.char_appearance.WeaponAnim = NingunArma;
    user.char_appearance.CascoAnim = NingunCasco;
}

void MoveUserChar(int user_index, eHeading heading) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];
    WorldPos nPos = user.Pos;

    switch (heading) {
        case eHeading::NORTH: nPos.Y--; break;
        case eHeading::SOUTH: nPos.Y++; break;
        case eHeading::EAST:  nPos.X++; break;
        case eHeading::WEST:  nPos.X--; break;
    }

    bool sailing = PuedeAtravesarAgua(user_index);

    if (MoveToLegalPos(user.Pos.Map, nPos.X, nPos.Y, sailing, !sailing)) {
        std::int16_t map = user.Pos.Map;
        std::size_t npos_idx = mapBlockIndex(map, nPos.X, nPos.Y);

        if (map >= 1 && static_cast<std::size_t>(map) < MapInfoList.size() && MapInfoList[map].NumUsers > 1) {
            std::int16_t casper_index = MapData[npos_idx].UserIndex;
            if (casper_index > 0 && casper_index <= MaxUsers) {
                // Bug #53: Admins invisibles no patean caspers ni envian mensaje de movimiento
                if (user.flags.AdminInvisible != 1) {
                    eHeading casper_heading = InvertHeading(heading);
                    WorldPos casper_pos = user.Pos; // la posición que deja libre el usuario vivo

                    auto& casper = UserList[casper_index];
                    if (casper.flags.AdminInvisible != 1) {
                        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCAreaButIndex, casper_index,
                            ao::net::protocol::PrepareMessageCharacterMove(casper.char_appearance.CharIndex, static_cast<std::uint8_t>(casper_pos.X), static_cast<std::uint8_t>(casper_pos.Y)));
                    }

                    ao::net::protocol::WriteForceCharMove(casper_index, casper_heading);

                    std::size_t old_casper_idx = mapBlockIndex(map, casper.Pos.X, casper.Pos.Y);
                    if (old_casper_idx < MapData.size() && MapData[old_casper_idx].UserIndex == casper_index) {
                        MapData[old_casper_idx].UserIndex = 0;
                    }

                    casper.Pos = casper_pos;
                    casper.char_appearance.heading = casper_heading;
                    MapData[mapBlockIndex(map, casper_pos.X, casper_pos.Y)].UserIndex = casper_index;

                    ModAreas::CheckUpdateNeededUser(casper_index, static_cast<std::uint8_t>(casper_heading));
                }
            }

            if (user.flags.AdminInvisible != 1) {
                ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCAreaButIndex, user_index,
                    ao::net::protocol::PrepareMessageCharacterMove(user.char_appearance.CharIndex, static_cast<std::uint8_t>(nPos.X), static_cast<std::uint8_t>(nPos.Y)));
            }
        }

        std::int16_t current_casper = MapData[npos_idx].UserIndex;
        if (!(user.flags.AdminInvisible == 1 && current_casper != 0)) {
            std::size_t old_idx = mapBlockIndex(user.Pos.Map, user.Pos.X, user.Pos.Y);
            if (old_idx < MapData.size() && MapData[old_idx].UserIndex == user_index) {
                MapData[old_idx].UserIndex = 0;
            }

            user.Pos = nPos;
            user.char_appearance.heading = heading;
            MapData[npos_idx].UserIndex = static_cast<std::int16_t>(user_index);

            ModAreas::CheckUpdateNeededUser(user_index, static_cast<std::uint8_t>(heading));
        } else {
            ao::net::protocol::WritePosUpdate(user_index);
        }
    } else {
        ao::net::protocol::WritePosUpdate(user_index);
    }
}

void WarpUserChar(int user_index, int map, int x, int y, bool fx, bool teletransported) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];

    std::int16_t old_map = user.Pos.Map;

    EraseUserChar(user_index, user.flags.AdminInvisible == 1);

    if (old_map != map) {
        if (map >= 1 && static_cast<std::size_t>(map) < MapInfoList.size()) {
            MapInfoList[map].NumUsers++;
        }

        if (old_map >= 1 && static_cast<std::size_t>(old_map) < MapInfoList.size()) {
            MapInfoList[old_map].NumUsers--;
            // Bug #51: Underflow defensivo
            if (MapInfoList[old_map].NumUsers < 0) {
                MapInfoList[old_map].NumUsers = 0;
            }
        }

        ModAreas::QuitarUser(user_index, old_map);
        ModAreas::AgregarUser(user_index, map);
    }

    user.Pos.Map = static_cast<std::int16_t>(map);
    user.Pos.X = static_cast<std::int16_t>(x);
    user.Pos.Y = static_cast<std::int16_t>(y);

    MakeUserChar(true, map, user_index, map, x, y);
    ao::net::protocol::WriteUserCharIndexInServer(user_index);

    WarpMascotas(user_index);
}

void Tilelibre(const WorldPos& pos, WorldPos& n_pos, Obj& obj, bool agua, bool tierra) {
    n_pos.Map = pos.Map;
    n_pos.X = 0;
    n_pos.Y = 0;

    int loop_c = 0;
    bool found = false;

    while (loop_c <= 15 && !found) {
        for (int tY = pos.Y - loop_c; tY <= pos.Y + loop_c && !found; ++tY) {
            for (int tX = pos.X - loop_c; tX <= pos.X + loop_c && !found; ++tX) {
                if (LegalPos(pos.Map, tX, tY, agua, tierra)) {
                    std::size_t idx = mapBlockIndex(pos.Map, tX, tY);
                    bool hayobj = (MapData[idx].ObjInfo.ObjIndex > 0 && MapData[idx].ObjInfo.ObjIndex != obj.ObjIndex);
                    if (!hayobj) {
                        hayobj = (MapData[idx].ObjInfo.Amount + obj.Amount > 10000); // MAX_INVENTORY_OBJS
                    }
                    if (!hayobj && MapData[idx].TileExit.Map == 0) {
                        n_pos.X = static_cast<std::int16_t>(tX);
                        n_pos.Y = static_cast<std::int16_t>(tY);
                        found = true;
                    }
                }
            }
        }
        loop_c++;
    }
}

void ChangeUserChar(int user_index, int body, int head, uint8_t heading, int weapon, int shield, int helmet) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];
    user.char_appearance.body = static_cast<std::int16_t>(body);
    user.char_appearance.Head = static_cast<std::int16_t>(head);
    user.char_appearance.heading = static_cast<eHeading>(heading);
    user.char_appearance.WeaponAnim = static_cast<std::int16_t>(weapon);
    user.char_appearance.ShieldAnim = static_cast<std::int16_t>(shield);
    user.char_appearance.CascoAnim = static_cast<std::int16_t>(helmet);

    ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, user_index,
        ao::net::protocol::PrepareMessageCharacterChange(body, head, static_cast<eHeading>(heading), user.char_appearance.CharIndex, weapon, shield, user.char_appearance.FX, user.char_appearance.loops, helmet));
}

void EraseUserChar(int user_index, bool is_admin_invisible) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];

    if (user.char_appearance.CharIndex > 0 && user.char_appearance.CharIndex <= MAXCHARS) {
        CharList[user.char_appearance.CharIndex] = 0;
    }

    auto pkt = ao::net::protocol::PrepareMessageCharacterRemove(user.char_appearance.CharIndex);
    std::string_view pkt_view(reinterpret_cast<const char*>(pkt.data()), pkt.size());

    if (is_admin_invisible) {
        TCP::EnviarDatosASlot(user_index, pkt_view);
    } else {
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, user_index, pkt);
    }

    ModAreas::QuitarUser(user_index, user.Pos.Map);

    if (InMapBounds(user.Pos.Map, user.Pos.X, user.Pos.Y)) {
        std::size_t idx = mapBlockIndex(user.Pos.Map, user.Pos.X, user.Pos.Y);
        if (idx < MapData.size() && MapData[idx].UserIndex == user_index) {
            MapData[idx].UserIndex = 0;
        }
    }

    user.char_appearance.CharIndex = 0;
}

void MakeUserChar(bool to_map, int snd_index, int user_index, int map, int x, int y, bool but_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];

    if (InMapBounds(map, x, y)) {
        if (user.char_appearance.CharIndex == 0) {
            int char_idx = NextOpenCharIndex();
            user.char_appearance.CharIndex = static_cast<std::int16_t>(char_idx);
            if (char_idx > 0 && char_idx <= MAXCHARS) {
                CharList[char_idx] = static_cast<std::int16_t>(user_index);
            }
        }

        std::size_t idx = mapBlockIndex(map, x, y);
        if (to_map && idx < MapData.size()) {
            MapData[idx].UserIndex = static_cast<std::int16_t>(user_index);
        }

        if (!to_map) {
            std::string user_name = user.name;
            uint8_t nick_color = GetNickColor(user_index);
            uint8_t privileges = static_cast<uint8_t>(user.flags.Privilegios);

            ao::net::protocol::WriteCharacterCreate(snd_index, user.char_appearance.body, user.char_appearance.Head, static_cast<eHeading>(user.char_appearance.heading),
                user.char_appearance.CharIndex, static_cast<uint8_t>(x), static_cast<uint8_t>(y),
                user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim, user.char_appearance.FX, 999, user.char_appearance.CascoAnim,
                user_name, nick_color, privileges);
        } else {
            ModAreas::AgregarUser(user_index, user.Pos.Map, but_index);
        }
    }
}

void WarpMascotas(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];

    int nro_pets = user.NroMascotas;
    bool can_warp = false;
    if (user.Pos.Map >= 1 && static_cast<std::size_t>(user.Pos.Map) < MapInfoList.size()) {
        can_warp = MapInfoList[user.Pos.Map].Pk;
    }

    int invocados_matados = 0;

    for (int i = 1; i <= MAXMASCOTAS; ++i) {
        int pet_index = user.MascotasIndex[i];
        int pet_type = 0;
        int min_hp = 0;
        bool pet_respawn = false;
        int pet_tiempo = 0;

        if (pet_index > 0 && static_cast<std::size_t>(pet_index) < Npclist.size()) {
            if (Npclist[pet_index].Contadores.TiempoExistencia > 0) {
                MODULO_NPCs::QuitarNPC(pet_index);
                user.MascotasIndex[i] = 0;
                invocados_matados++;
                nro_pets--;
                pet_type = 0;
            } else {
                pet_type = user.MascotasType[i];
                min_hp = Npclist[pet_index].Stats.MinHp;
                MODULO_NPCs::QuitarNPC(pet_index);
                user.MascotasType[i] = pet_type;
            }
        } else if (user.MascotasType[i] > 0) {
            pet_respawn = true;
            pet_type = user.MascotasType[i];
            pet_tiempo = 0;
        }

        if (pet_type > 0 && can_warp) {
            int new_pet_idx = MODULO_NPCs::SpawnNpc(pet_type, user.Pos, false, pet_respawn);
            if (new_pet_idx == 0) {
                ao::net::protocol::WriteConsoleMsg(user_index, "Tus mascotas no pueden transitar este mapa.", FontTypeNames::FONTTYPE_INFO);
            } else {
                user.MascotasIndex[i] = new_pet_idx;
                if (min_hp > 0) {
                    Npclist[new_pet_idx].Stats.MinHp = min_hp;
                }
                Npclist[new_pet_idx].MaestroUser = user_index;
                Npclist[new_pet_idx].Contadores.TiempoExistencia = pet_tiempo;
                MODULO_NPCs::FollowAmo(new_pet_idx);
            }
        }
    }

    if (invocados_matados > 0) {
        ao::net::protocol::WriteConsoleMsg(user_index, "Pierdes el control de tus mascotas invocadas.", FontTypeNames::FONTTYPE_INFO);
    }
    if (!can_warp && user.NroMascotas > 0) {
        ao::net::protocol::WriteConsoleMsg(user_index, "No se permiten mascotas en zona segura. Éstas te esperarán afuera.", FontTypeNames::FONTTYPE_INFO);
    }

    user.NroMascotas = nro_pets;
}

void WarpMascota(int user_index, int pet_index) {
    if (user_index < 1 || user_index > MaxUsers || pet_index < 1 || pet_index > MAXMASCOTAS) return;
    auto& user = UserList[user_index];

    WorldPos target_pos;
    target_pos.Map = user.flags.TargetMap;
    target_pos.X = user.flags.TargetX;
    target_pos.Y = user.flags.TargetY;

    int npc_index = user.MascotasIndex[pet_index];
    int pet_type = user.MascotasType[pet_index];
    int min_hp = 0;
    if (npc_index > 0 && static_cast<std::size_t>(npc_index) < Npclist.size()) {
        min_hp = Npclist[npc_index].Stats.MinHp;
        MODULO_NPCs::QuitarNPC(npc_index);
    }

    user.MascotasType[pet_index] = pet_type;
    user.NroMascotas++;

    int new_pet_idx = MODULO_NPCs::SpawnNpc(pet_type, target_pos, false, false);
    if (new_pet_idx == 0) {
        ao::net::protocol::WriteConsoleMsg(user_index, "Tu mascota no pueden transitar este sector del mapa, intenta invocarla en otra parte.", FontTypeNames::FONTTYPE_INFO);
    } else {
        user.MascotasIndex[pet_index] = new_pet_idx;
        auto& pet = Npclist[new_pet_idx];
        if (min_hp > 0) {
            pet.Stats.MinHp = min_hp;
        }
        pet.MaestroUser = user_index;
        pet.Movement = TipoAI::SigueAmo;
        pet.Target = 0;
        pet.TargetNPC = 0;

        MODULO_NPCs::FollowAmo(new_pet_idx);
    }
}

std::string GetDireccion(int user_index, int other_user_index) {
    if (user_index < 1 || user_index > MaxUsers || other_user_index < 1 || other_user_index > MaxUsers) return "";
    int dx = UserList[user_index].Pos.X - UserList[other_user_index].Pos.X;
    int dy = UserList[user_index].Pos.Y - UserList[other_user_index].Pos.Y;

    if (dx == 0 && dy > 0) return "Sur";
    if (dx == 0 && dy < 0) return "Norte";
    if (dx > 0 && dy == 0) return "Este";
    if (dx < 0 && dy == 0) return "Oeste";
    if (dx > 0 && dy < 0) return "NorEste";
    if (dx < 0 && dy < 0) return "NorOeste";
    if (dx > 0 && dy > 0) return "SurEste";
    if (dx < 0 && dy > 0) return "SurOeste";
    return "";
}

int FarthestPet(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return 0;
    const auto& user = UserList[user_index];
    if (user.NroMascotas == 0) return 0;

    int farthest_idx = 0;
    int max_dist = -1;

    for (int i = 1; i <= MAXMASCOTAS; ++i) {
        int pet_index = user.MascotasIndex[i];
        if (pet_index > 0 && static_cast<std::size_t>(pet_index) < Npclist.size()) {
            if (Npclist[pet_index].Contadores.TiempoExistencia == 0) {
                int dist = std::abs(user.Pos.X - Npclist[pet_index].Pos.X) +
                           std::abs(user.Pos.Y - Npclist[pet_index].Pos.Y);
                if (dist > max_dist) {
                    max_dist = dist;
                    farthest_idx = i;
                }
            }
        }
    }
    return farthest_idx;
}

// ==========================================
// Fase G3: Progresión e Inspección
// ==========================================

static CharReaderHook g_char_reader_hook = nullptr;

void SetCharReaderHook(CharReaderHook hook) noexcept {
    g_char_reader_hook = std::move(hook);
}

static std::string ReadCharVar(const std::string& char_name, const std::string& section, const std::string& key, CharReaderHook reader) {
    if (reader) {
        return reader(char_name, section, key);
    }
    if (g_char_reader_hook) {
        return g_char_reader_hook(char_name, section, key);
    }
    std::string char_file = CharPath.empty() ? ("charfile/" + char_name + ".chr") : (CharPath + char_name + ".chr");
    return FileIO::GetVar(char_file, section, key);
}

void CheckUserLevel(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];

    std::int16_t was_newbie_elv = user.Stats.ELV;
    int pts = 0;

    while (user.Stats.Exp >= user.Stats.ELU) {
        if (user.Stats.ELV >= STAT_MAXELV) {
            user.Stats.Exp = 0;
            user.Stats.ELU = 0;
            return;
        }

        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, user_index,
            ao::net::protocol::PrepareMessagePlayWave(SND_NIVEL, user.Pos.X, user.Pos.Y));
        ao::net::protocol::WriteConsoleMsg(user_index, "¡Has subido de nivel!", FontTypeNames::FONTTYPE_INFO);

        if (user.Stats.ELV == 1) {
            pts += 10;
        } else {
            pts += 5;
        }

        user.Stats.ELV++;
        user.Stats.Exp -= user.Stats.ELU;

        // Bug #50: Computar next_elu en std::int64_t y validar desbordamiento
        double factor = 1.25;
        if (user.Stats.ELV < 15) {
            factor = 1.4;
        } else if (user.Stats.ELV < 21) {
            factor = 1.35;
        } else if (user.Stats.ELV < 33) {
            factor = 1.3;
        } else if (user.Stats.ELV < 41) {
            factor = 1.225;
        } else {
            factor = 1.25;
        }

        std::int64_t next_elu = static_cast<std::int64_t>(user.Stats.ELU * factor);
        if (next_elu > std::numeric_limits<std::int32_t>::max()) {
            throw ELUOverflowException("ELU overflow en CheckUserLevel para usuario " + std::to_string(user_index));
        }
        user.Stats.ELU = static_cast<std::int32_t>(next_elu);

        // Calculo subida de vida
        double promedio = ModVida[static_cast<std::size_t>(user.clase)] - (21.0 - static_cast<double>(user.Stats.UserAtributos[eAtributos::Constitucion])) * 0.5;
        int aux = RandomNumber(0, 100);
        int aumento_hp = 0;

        if (std::abs((promedio - std::floor(promedio)) - 0.5) < 0.001) {
            std::array<int, 5> dist_vida{};
            dist_vida[1] = DistribucionSemienteraVida[1];
            dist_vida[2] = dist_vida[1] + DistribucionSemienteraVida[2];
            dist_vida[3] = dist_vida[2] + DistribucionSemienteraVida[3];
            dist_vida[4] = dist_vida[3] + DistribucionSemienteraVida[4];

            if (aux <= dist_vida[1]) {
                aumento_hp = static_cast<int>(promedio + 1.5);
            } else if (aux <= dist_vida[2]) {
                aumento_hp = static_cast<int>(promedio + 0.5);
            } else if (aux <= dist_vida[3]) {
                aumento_hp = static_cast<int>(promedio - 0.5);
            } else {
                aumento_hp = static_cast<int>(promedio - 1.5);
            }
        } else {
            std::array<int, 6> dist_vida{};
            dist_vida[1] = DistribucionSemienteraVida[1];
            dist_vida[2] = dist_vida[1] + DistribucionEnteraVida[2];
            dist_vida[3] = dist_vida[2] + DistribucionEnteraVida[3];
            dist_vida[4] = dist_vida[3] + DistribucionEnteraVida[4];
            dist_vida[5] = dist_vida[4] + DistribucionEnteraVida[5];

            if (aux <= dist_vida[1]) {
                aumento_hp = static_cast<int>(promedio + 2.0);
            } else if (aux <= dist_vida[2]) {
                aumento_hp = static_cast<int>(promedio + 1.0);
            } else if (aux <= dist_vida[3]) {
                aumento_hp = static_cast<int>(promedio);
            } else if (aux <= dist_vida[4]) {
                aumento_hp = static_cast<int>(promedio - 1.0);
            } else {
                aumento_hp = static_cast<int>(promedio - 2.0);
            }
        }

        int aumento_hit = 0;
        int aumento_mana = 0;
        int aumento_sta = 0;

        switch (user.clase) {
            case eClass::Warrior:
                aumento_hit = (user.Stats.ELV > 35) ? 2 : 3;
                aumento_sta = AumentoSTDef;
                break;
            case eClass::Hunter:
                aumento_hit = (user.Stats.ELV > 35) ? 2 : 3;
                aumento_sta = AumentoSTDef;
                break;
            case eClass::Pirat:
                aumento_hit = 3;
                aumento_sta = AumentoSTDef;
                break;
            case eClass::Paladin:
                aumento_hit = (user.Stats.ELV > 35) ? 1 : 3;
                aumento_mana = user.Stats.UserAtributos[eAtributos::Inteligencia];
                aumento_sta = AumentoSTDef;
                break;
            case eClass::Thief:
                aumento_hit = 2;
                aumento_sta = AumentoSTLadron;
                break;
            case eClass::Mage:
                aumento_hit = 1;
                aumento_mana = static_cast<int>(2.8 * user.Stats.UserAtributos[eAtributos::Inteligencia]);
                aumento_sta = AumentoSTMago;
                break;
            case eClass::Worker:
                aumento_hit = 2;
                aumento_sta = AumentoSTTrabajador;
                break;
            case eClass::Cleric:
            case eClass::Druid:
            case eClass::Bard:
                aumento_hit = 2;
                aumento_mana = 2 * user.Stats.UserAtributos[eAtributos::Inteligencia];
                aumento_sta = AumentoSTDef;
                break;
            case eClass::Assasin:
                aumento_hit = (user.Stats.ELV > 35) ? 1 : 3;
                aumento_mana = user.Stats.UserAtributos[eAtributos::Inteligencia];
                aumento_sta = AumentoSTDef;
                break;
            case eClass::Bandit:
                aumento_hit = (user.Stats.ELV > 35) ? 1 : 3;
                aumento_mana = (user.Stats.UserAtributos[eAtributos::Inteligencia] / 3) * 2;
                aumento_sta = AumentoStBandido;
                break;
            default:
                aumento_hit = 2;
                aumento_sta = AumentoSTDef;
                break;
        }

        user.Stats.MaxHp += static_cast<std::int16_t>(aumento_hp);
        if (user.Stats.MaxHp > STAT_MAXHP) user.Stats.MaxHp = STAT_MAXHP;

        user.Stats.MaxSta += static_cast<std::int16_t>(aumento_sta);
        if (user.Stats.MaxSta > STAT_MAXSTA) user.Stats.MaxSta = STAT_MAXSTA;

        user.Stats.MaxMAN += static_cast<std::int16_t>(aumento_mana);
        if (user.Stats.MaxMAN > STAT_MAXMAN) user.Stats.MaxMAN = STAT_MAXMAN;

        user.Stats.MaxHIT += static_cast<std::int16_t>(aumento_hit);
        user.Stats.MinHIT += static_cast<std::int16_t>(aumento_hit);

        if (aumento_hp > 0) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Has ganado " + std::to_string(aumento_hp) + " puntos de vida.", FontTypeNames::FONTTYPE_INFO);
        }
        if (aumento_sta > 0) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Has ganado " + std::to_string(aumento_sta) + " puntos de energía.", FontTypeNames::FONTTYPE_INFO);
        }
        if (aumento_mana > 0) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Has ganado " + std::to_string(aumento_mana) + " puntos de maná.", FontTypeNames::FONTTYPE_INFO);
        }
        if (aumento_hit > 0) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Tu golpe máximo aumentó en " + std::to_string(aumento_hit) + " puntos.", FontTypeNames::FONTTYPE_INFO);
            ao::net::protocol::WriteConsoleMsg(user_index, "Tu golpe mínimo aumentó en " + std::to_string(aumento_hit) + " puntos.", FontTypeNames::FONTTYPE_INFO);
        }

        user.Stats.MinHp = user.Stats.MaxHp;

        // Expulsión de clan faccionario si ELV == 25
        if (user.Stats.ELV == 25) {
            std::int16_t gi = user.GuildIndex;
            if (gi > 0) {
                std::string align = modGuilds::GuildAlignment(gi);
                if (align == "Del Mal" || align == "Real") {
                    modGuilds::m_EcharMiembroDeClan(-1, user.name);
                    ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToGuildMembers, gi,
                        ao::net::protocol::PrepareMessageConsoleMsg(user.name + " deja el clan.", FontTypeNames::FONTTYPE_GUILD));
                    ao::net::protocol::WriteConsoleMsg(user_index, "¡Ya tienes la madurez suficiente como para decidir bajo que estandarte pelearás! Por esta razón, hasta tanto no te enlistes en la facción bajo la cual tu clan está alineado, estarás excluído del mismo.", FontTypeNames::FONTTYPE_GUILD);
                }
            }
        }
    }

    // Transición de Novato
    if (was_newbie_elv <= LimiteNewbie && user.Stats.ELV > LimiteNewbie) {
        InvUsuario::QuitarNewbieObj(static_cast<std::int16_t>(user_index));
        if (user.Pos.Map >= 1 && static_cast<std::size_t>(user.Pos.Map) < MapInfoList.size()) {
            if (MapInfoList[user.Pos.Map].Restringir == "NEWBIE") {
                WarpUserChar(user_index, 1, 50, 50, true);
                ao::net::protocol::WriteConsoleMsg(user_index, "Debes abandonar el Dungeon Newbie.", FontTypeNames::FONTTYPE_INFO);
            }
        }
    }

    if (pts > 0) {
        ao::net::protocol::WriteLevelUp(user_index, static_cast<std::int16_t>(pts));
        user.Stats.SkillPts += static_cast<std::int16_t>(pts);
        ao::net::protocol::WriteConsoleMsg(user_index, "Has ganado un total de " + std::to_string(pts) + " skillpoints.", FontTypeNames::FONTTYPE_INFO);
    }
}

void ActStats(int victim_index, int attacker_index) {
    if (victim_index < 1 || victim_index > MaxUsers || attacker_index < 1 || attacker_index > MaxUsers) return;

    std::int32_t da_exp = static_cast<std::int32_t>(UserList[victim_index].Stats.ELV) * 2;
    auto& attacker = UserList[attacker_index];

    attacker.Stats.Exp += da_exp;
    if (attacker.Stats.Exp > MAXEXP) attacker.Stats.Exp = MAXEXP;

    bool era_criminal = FileIO::criminal(attacker_index);

    if (UserList[victim_index].flags.AtacablePor != attacker_index) {
        if (!FileIO::criminal(victim_index)) {
            attacker.Reputacion.AsesinoRep += vlASESINO * 2;
            if (attacker.Reputacion.AsesinoRep > MAXREP) attacker.Reputacion.AsesinoRep = MAXREP;
            attacker.Reputacion.BurguesRep = 0;
            attacker.Reputacion.NobleRep = 0;
            attacker.Reputacion.PlebeRep = 0;
        } else {
            attacker.Reputacion.NobleRep += vlASESINO / 2;
            if (attacker.Reputacion.NobleRep > MAXREP) attacker.Reputacion.NobleRep = MAXREP;
        }
    }

    ao::net::protocol::WriteMultiMessage(attacker_index, eMessages::HaveKilledUser, victim_index, da_exp);
    ao::net::protocol::WriteMultiMessage(victim_index, eMessages::UserKill, attacker_index);
    TCP::FlushBuffer(static_cast<std::int16_t>(victim_index));
}

void SubirSkill(int user_index, int skill, bool acerto) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];

    if (user.flags.Hambre == 1 || user.flags.Sed == 1) return;

    if (user.Counters.AsignedSkills < 10) {
        if (user.flags.UltimoMensaje != 7) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Para poder entrenar un skill debes asignar los 10 skills iniciales.", FontTypeNames::FONTTYPE_INFO);
            user.flags.UltimoMensaje = 7;
        }
        return;
    }

    if (skill < 1 || skill > NUMSKILLS) return;
    if (user.Stats.UserSkills[skill] >= MAXSKILLPOINTS) return;

    int lvl = user.Stats.ELV;
    if (lvl > STAT_MAXELV) lvl = STAT_MAXELV;
    if (static_cast<std::size_t>(lvl) < LevelSkillList.size()) {
        if (user.Stats.UserSkills[skill] >= LevelSkillList[lvl].LevelValue) return;
    }

    if (acerto) {
        user.Stats.ExpSkills[skill] += EXP_ACIERTO_SKILL;
    } else {
        user.Stats.ExpSkills[skill] += EXP_FALLO_SKILL;
    }

    if (user.Stats.ExpSkills[skill] >= user.Stats.EluSkills[skill]) {
        user.Stats.UserSkills[skill]++;
        std::string skill_name = (static_cast<std::size_t>(skill) < SkillsNames.size()) ? SkillsNames[skill] : "";
        ao::net::protocol::WriteConsoleMsg(user_index, "¡Has mejorado tu skill " + skill_name + " en un punto! Ahora tienes " + std::to_string(user.Stats.UserSkills[skill]) + " pts.", FontTypeNames::FONTTYPE_INFO);

        user.Stats.Exp += 50;
        if (user.Stats.Exp > MAXEXP) user.Stats.Exp = MAXEXP;

        ao::net::protocol::WriteConsoleMsg(user_index, "¡Has ganado 50 puntos de experiencia!", FontTypeNames::FONTTYPE_FIGHT);
        ao::net::protocol::WriteUpdateExp(user_index);
        CheckUserLevel(user_index);
        CheckEluSkill(user_index, static_cast<uint8_t>(skill), false);
    }
}

void CheckEluSkill(int user_index, uint8_t skill, bool allocation) {
    if (user_index < 1 || user_index > MaxUsers || skill < 1 || skill > NUMSKILLS) return;
    auto& stats = UserList[user_index].Stats;

    if (stats.UserSkills[skill] < MAXSKILLPOINTS) {
        if (allocation) {
            stats.ExpSkills[skill] = 0;
        } else {
            stats.ExpSkills[skill] -= stats.EluSkills[skill];
        }
        stats.EluSkills[skill] = static_cast<int32_t>(ELU_SKILL_INICIAL * std::pow(1.05, stats.UserSkills[skill]));
    } else {
        stats.ExpSkills[skill] = 0;
        stats.EluSkills[skill] = 0;
    }
}

void EnviarFama(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& rep = UserList[user_index].Reputacion;
    std::int32_t L = (-rep.AsesinoRep) + (-rep.BandidoRep) + rep.BurguesRep + (-rep.LadronesRep) + rep.NobleRep + rep.PlebeRep;
    rep.Promedio = std::round(static_cast<double>(L) / 6.0);
}

void SendUserStatsTxt(int send_index, int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    const auto& user = UserList[user_index];

    ao::net::protocol::WriteConsoleMsg(send_index, "Estadísticas de: " + user.name, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Nivel: " + std::to_string(user.Stats.ELV) + "  EXP: " + std::to_string(user.Stats.Exp) + "/" + std::to_string(user.Stats.ELU), FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Salud: " + std::to_string(user.Stats.MinHp) + "/" + std::to_string(user.Stats.MaxHp) + "  Maná: " + std::to_string(user.Stats.MinMAN) + "/" + std::to_string(user.Stats.MaxMAN) + "  Energía: " + std::to_string(user.Stats.MinSta) + "/" + std::to_string(user.Stats.MaxSta), FontTypeNames::FONTTYPE_INFO);

    if (user.Invent.WeaponEqpObjIndex > 0 && static_cast<std::size_t>(user.Invent.WeaponEqpObjIndex) < ObjDataList.size()) {
        const auto& wpn = ObjDataList[user.Invent.WeaponEqpObjIndex];
        ao::net::protocol::WriteConsoleMsg(send_index, "Menor Golpe/Mayor Golpe: " + std::to_string(user.Stats.MinHIT) + "/" + std::to_string(user.Stats.MaxHIT) + " (" + std::to_string(wpn.MinHIT) + "/" + std::to_string(wpn.MaxHIT) + ")", FontTypeNames::FONTTYPE_INFO);
    } else {
        ao::net::protocol::WriteConsoleMsg(send_index, "Menor Golpe/Mayor Golpe: " + std::to_string(user.Stats.MinHIT) + "/" + std::to_string(user.Stats.MaxHIT), FontTypeNames::FONTTYPE_INFO);
    }

    if (user.Invent.ArmourEqpObjIndex > 0 && static_cast<std::size_t>(user.Invent.ArmourEqpObjIndex) < ObjDataList.size()) {
        const auto& arm = ObjDataList[user.Invent.ArmourEqpObjIndex];
        if (user.Invent.EscudoEqpObjIndex > 0 && static_cast<std::size_t>(user.Invent.EscudoEqpObjIndex) < ObjDataList.size()) {
            const auto& shd = ObjDataList[user.Invent.EscudoEqpObjIndex];
            ao::net::protocol::WriteConsoleMsg(send_index, "(CUERPO) Mín Def/Máx Def: " + std::to_string(arm.MinDef + shd.MinDef) + "/" + std::to_string(arm.MaxDef + shd.MaxDef), FontTypeNames::FONTTYPE_INFO);
        } else {
            ao::net::protocol::WriteConsoleMsg(send_index, "(CUERPO) Mín Def/Máx Def: " + std::to_string(arm.MinDef) + "/" + std::to_string(arm.MaxDef), FontTypeNames::FONTTYPE_INFO);
        }
    } else {
        ao::net::protocol::WriteConsoleMsg(send_index, "(CUERPO) Mín Def/Máx Def: 0", FontTypeNames::FONTTYPE_INFO);
    }

    if (user.Invent.CascoEqpObjIndex > 0 && static_cast<std::size_t>(user.Invent.CascoEqpObjIndex) < ObjDataList.size()) {
        const auto& hlm = ObjDataList[user.Invent.CascoEqpObjIndex];
        ao::net::protocol::WriteConsoleMsg(send_index, "(CABEZA) Mín Def/Máx Def: " + std::to_string(hlm.MinDef) + "/" + std::to_string(hlm.MaxDef), FontTypeNames::FONTTYPE_INFO);
    } else {
        ao::net::protocol::WriteConsoleMsg(send_index, "(CABEZA) Mín Def/Máx Def: 0", FontTypeNames::FONTTYPE_INFO);
    }
}

void SendUserMiniStatsTxt(int send_index, int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    const auto& user = UserList[user_index];

    std::string clase_name = (static_cast<std::size_t>(user.clase) < ListaClases.size()) ? ListaClases[static_cast<std::size_t>(user.clase)] : "";

    ao::net::protocol::WriteConsoleMsg(send_index, "Pj: " + user.name, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Ciudadanos matados: " + std::to_string(user.Faccion.CiudadanosMatados) + " Criminales matados: " + std::to_string(user.Faccion.CriminalesMatados) + " usuarios matados: " + std::to_string(user.Stats.UsuariosMatados), FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "NPCs muertos: " + std::to_string(user.Stats.NPCsMuertos), FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Clase: " + clase_name, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Pena: " + std::to_string(user.Counters.Pena), FontTypeNames::FONTTYPE_INFO);

    if (user.Faccion.ArmadaReal == 1) {
        ao::net::protocol::WriteConsoleMsg(send_index, "Ejército real desde: " + user.Faccion.FechaIngreso, FontTypeNames::FONTTYPE_INFO);
        ao::net::protocol::WriteConsoleMsg(send_index, "Ingresó en nivel: " + std::to_string(user.Faccion.NivelIngreso) + " con " + std::to_string(user.Faccion.MatadosIngreso) + " ciudadanos matados.", FontTypeNames::FONTTYPE_INFO);
        ao::net::protocol::WriteConsoleMsg(send_index, "Veces que ingresó: " + std::to_string(user.Faccion.Reenlistadas), FontTypeNames::FONTTYPE_INFO);
    } else if (user.Faccion.FuerzasCaos == 1) {
        ao::net::protocol::WriteConsoleMsg(send_index, "Legión oscura desde: " + user.Faccion.FechaIngreso, FontTypeNames::FONTTYPE_INFO);
        ao::net::protocol::WriteConsoleMsg(send_index, "Ingresó en nivel: " + std::to_string(user.Faccion.NivelIngreso), FontTypeNames::FONTTYPE_INFO);
        ao::net::protocol::WriteConsoleMsg(send_index, "Veces que ingresó: " + std::to_string(user.Faccion.Reenlistadas), FontTypeNames::FONTTYPE_INFO);
    } else if (user.Faccion.RecibioExpInicialReal == 1) {
        ao::net::protocol::WriteConsoleMsg(send_index, "Fue ejército real", FontTypeNames::FONTTYPE_INFO);
    }
}

void SendUserSkillsTxt(int send_index, int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    const auto& user = UserList[user_index];

    ao::net::protocol::WriteConsoleMsg(send_index, user.name, FontTypeNames::FONTTYPE_INFO);
    for (int j = 1; j <= NUMSKILLS; ++j) {
        std::string skill_name = (static_cast<std::size_t>(j) < SkillsNames.size()) ? SkillsNames[j] : "";
        ao::net::protocol::WriteConsoleMsg(send_index, skill_name + " = " + std::to_string(user.Stats.UserSkills[j]), FontTypeNames::FONTTYPE_INFO);
    }
    ao::net::protocol::WriteConsoleMsg(send_index, "SkillLibres:" + std::to_string(user.Stats.SkillPts), FontTypeNames::FONTTYPE_INFO);
}

void SendUserInvTxt(int send_index, int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    const auto& user = UserList[user_index];

    ao::net::protocol::WriteConsoleMsg(send_index, user.name, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Tiene " + std::to_string(user.Invent.NroItems) + " objetos.", FontTypeNames::FONTTYPE_INFO);

    for (int j = 1; j <= MAX_INVENTORY_SLOTS; ++j) {
        int obj_idx = user.Invent.Object[j].ObjIndex;
        int obj_amt = user.Invent.Object[j].Amount;
        if (obj_idx > 0 && static_cast<std::size_t>(obj_idx) < ObjDataList.size()) {
            ao::net::protocol::WriteConsoleMsg(send_index, "Objeto " + std::to_string(j) + " " + ObjDataList[obj_idx].name + " Cantidad:" + std::to_string(obj_amt), FontTypeNames::FONTTYPE_INFO);
        }
    }
}

void SendUserMiniStatsTxtFromChar(int send_index, const std::string& char_name, CharReaderHook reader) {
    std::string clase_str = ReadCharVar(char_name, "INIT", "Clase", reader);
    if (clase_str.empty()) {
        ao::net::protocol::WriteConsoleMsg(send_index, "Pj Inexistente", FontTypeNames::FONTTYPE_INFO);
        return;
    }

    std::string ciud_mat = ReadCharVar(char_name, "FACCIONES", "CiudMatados", reader);
    std::string crim_mat = ReadCharVar(char_name, "FACCIONES", "CrimMatados", reader);
    std::string user_muertes = ReadCharVar(char_name, "MUERTES", "UserMuertes", reader);
    std::string npcs_muertes = ReadCharVar(char_name, "MUERTES", "NpcsMuertes", reader);
    std::string pena = ReadCharVar(char_name, "COUNTERS", "PENA", reader);

    int clase_idx = std::stoi(clase_str);
    std::string clase_name = (clase_idx >= 1 && static_cast<std::size_t>(clase_idx) < ListaClases.size()) ? ListaClases[clase_idx] : "";

    ao::net::protocol::WriteConsoleMsg(send_index, "Pj: " + char_name, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Ciudadanos matados: " + ciud_mat + " CriminalesMatados: " + crim_mat + " usuarios matados: " + user_muertes, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "NPCs muertos: " + npcs_muertes, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Clase: " + clase_name, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Pena: " + pena, FontTypeNames::FONTTYPE_INFO);

    std::string ejercito_real = ReadCharVar(char_name, "FACCIONES", "EjercitoReal", reader);
    if (!ejercito_real.empty() && std::stoi(ejercito_real) == 1) {
        std::string fecha = ReadCharVar(char_name, "FACCIONES", "FechaIngreso", reader);
        std::string lvl_ing = ReadCharVar(char_name, "FACCIONES", "NivelIngreso", reader);
        std::string mat_ing = ReadCharVar(char_name, "FACCIONES", "MatadosIngreso", reader);
        std::string reenl = ReadCharVar(char_name, "FACCIONES", "Reenlistadas", reader);

        ao::net::protocol::WriteConsoleMsg(send_index, "Ejército real desde: " + fecha, FontTypeNames::FONTTYPE_INFO);
        ao::net::protocol::WriteConsoleMsg(send_index, "Ingresó en nivel: " + lvl_ing + " con " + mat_ing + " ciudadanos matados.", FontTypeNames::FONTTYPE_INFO);
        ao::net::protocol::WriteConsoleMsg(send_index, "Veces que ingresó: " + reenl, FontTypeNames::FONTTYPE_INFO);
    }
}

void SendUserStatsTxtOFF(int send_index, const std::string& nombre, CharReaderHook reader) {
    std::string elv_str = ReadCharVar(nombre, "STATS", "ELV", reader);
    if (elv_str.empty()) {
        elv_str = ReadCharVar(nombre, "stats", "elv", reader);
    }
    if (elv_str.empty()) {
        ao::net::protocol::WriteConsoleMsg(send_index, "Pj Inexistente", FontTypeNames::FONTTYPE_INFO);
        return;
    }

    std::string exp = ReadCharVar(nombre, "STATS", "EXP", reader);
    if (exp.empty()) exp = ReadCharVar(nombre, "stats", "Exp", reader);
    std::string elu = ReadCharVar(nombre, "STATS", "ELU", reader);
    if (elu.empty()) elu = ReadCharVar(nombre, "stats", "elu", reader);

    std::string min_sta = ReadCharVar(nombre, "STATS", "MINSTA", reader);
    if (min_sta.empty()) min_sta = ReadCharVar(nombre, "stats", "minsta", reader);
    std::string max_sta = ReadCharVar(nombre, "STATS", "MAXSTA", reader);
    if (max_sta.empty()) max_sta = ReadCharVar(nombre, "stats", "maxSta", reader);

    std::string min_hp = ReadCharVar(nombre, "STATS", "MINHP", reader);
    if (min_hp.empty()) min_hp = ReadCharVar(nombre, "stats", "MinHP", reader);
    std::string max_hp = ReadCharVar(nombre, "STATS", "MAXHP", reader);
    if (max_hp.empty()) max_hp = ReadCharVar(nombre, "stats", "MaxHP", reader);

    std::string min_man = ReadCharVar(nombre, "STATS", "MINMAN", reader);
    if (min_man.empty()) min_man = ReadCharVar(nombre, "stats", "MinMAN", reader);
    std::string max_man = ReadCharVar(nombre, "STATS", "MAXMAN", reader);
    if (max_man.empty()) max_man = ReadCharVar(nombre, "stats", "MaxMAN", reader);

    std::string max_hit = ReadCharVar(nombre, "STATS", "MAXHIT", reader);
    if (max_hit.empty()) max_hit = ReadCharVar(nombre, "stats", "MaxHIT", reader);

    std::string gld = ReadCharVar(nombre, "STATS", "GLD", reader);
    if (gld.empty()) gld = ReadCharVar(nombre, "stats", "GLD", reader);

    ao::net::protocol::WriteConsoleMsg(send_index, "Estadísticas de: " + nombre, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Nivel: " + elv_str + "  EXP: " + exp + "/" + elu, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Energía: " + min_sta + "/" + max_sta, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Salud: " + min_hp + "/" + max_hp + "  Maná: " + min_man + "/" + max_man, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Menor Golpe/Mayor Golpe: " + max_hit, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Oro: " + gld, FontTypeNames::FONTTYPE_INFO);
}

void SendUserOROTxtFromChar(int send_index, const std::string& char_name, CharReaderHook reader) {
    std::string banco_str = ReadCharVar(char_name, "STATS", "BANCO", reader);
    std::string gld_str = ReadCharVar(char_name, "STATS", "GLD", reader);

    if (banco_str.empty() && gld_str.empty()) {
        ao::net::protocol::WriteConsoleMsg(send_index, "Usuario inexistente: " + char_name, FontTypeNames::FONTTYPE_INFO);
        return;
    }

    int32_t banco = banco_str.empty() ? 0 : std::stoi(banco_str);

    ao::net::protocol::WriteConsoleMsg(send_index, char_name, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Tiene " + std::to_string(banco) + " en el banco.", FontTypeNames::FONTTYPE_INFO);
}

void SendUserInvTxtFromChar(int send_index, const std::string& char_name, CharReaderHook reader) {
    std::string cant_str = ReadCharVar(char_name, "Inventory", "CantidadItems", reader);
    if (cant_str.empty()) {
        ao::net::protocol::WriteConsoleMsg(send_index, "Usuario inexistente: " + char_name, FontTypeNames::FONTTYPE_INFO);
        return;
    }

    ao::net::protocol::WriteConsoleMsg(send_index, char_name, FontTypeNames::FONTTYPE_INFO);
    ao::net::protocol::WriteConsoleMsg(send_index, "Tiene " + cant_str + " objetos.", FontTypeNames::FONTTYPE_INFO);

    for (int j = 1; j <= MAX_INVENTORY_SLOTS; ++j) {
        std::string tmp = ReadCharVar(char_name, "Inventory", "Obj" + std::to_string(j), reader);
        if (!tmp.empty()) {
            auto dash_pos = tmp.find('-');
            if (dash_pos != std::string::npos) {
                int obj_idx = std::stoi(tmp.substr(0, dash_pos));
                int obj_cant = std::stoi(tmp.substr(dash_pos + 1));
                if (obj_idx > 0 && static_cast<std::size_t>(obj_idx) < ObjDataList.size()) {
                    ao::net::protocol::WriteConsoleMsg(send_index, "Objeto " + std::to_string(j) + " " + ObjDataList[obj_idx].name + " Cantidad:" + std::to_string(obj_cant), FontTypeNames::FONTTYPE_INFO);
                }
            }
        }
    }
}

// ==========================================
// Fase G4: Muerte, Resurrección y Estados
// ==========================================

bool IsArena(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return false;
    const auto& user = UserList[user_index];
    std::size_t idx = mapBlockIndex(user.Pos.Map, user.Pos.X, user.Pos.Y);
    if (idx < MapData.size()) {
        return (MapData[idx].trigger == eTrigger::ZONAPELEA);
    }
    return false;
}

void SetInvisible(int user_index, int user_char_index, bool invisible) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];
    user.flags.invisible = invisible ? 1 : 0;

    auto pkt = ao::net::protocol::PrepareMessageSetInvisible(user_char_index, invisible);
    ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToUsersAndRmsAndCounselorsAreaButGMs, user_index, pkt);

    std::string snd_nick = user.name;
    if (invisible) {
        snd_nick += " " + TAG_USER_INVISIBLE;
    } else {
        if (user.GuildIndex > 0) {
            snd_nick += " <" + modGuilds::GuildName(user.GuildIndex) + ">";
        }
    }

    auto nick_pkt = ao::net::protocol::PrepareMessageCharacterChangeNick(user_char_index, snd_nick);
    ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToGMsAreaButRmsOrCounselors, user_index, nick_pkt);
}

void SetConsulatMode(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];

    std::string snd_nick = user.name;
    if (user.flags.EnConsulta) {
        snd_nick += " " + TAG_CONSULT_MODE;
    } else {
        if (user.GuildIndex > 0) {
            snd_nick += " <" + modGuilds::GuildName(user.GuildIndex) + ">";
        }
    }

    auto nick_pkt = ao::net::protocol::PrepareMessageCharacterChangeNick(user.char_appearance.CharIndex, snd_nick);
    ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, user_index, nick_pkt);
}

void RefreshCharStatus(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];

    std::string clan_tag;
    if (user.GuildIndex > 0) {
        clan_tag = " <" + modGuilds::GuildName(user.GuildIndex) + ">";
    }

    uint8_t nick_color = GetNickColor(user_index);

    if (user.showName) {
        auto pkt = ao::net::protocol::PrepareMessageUpdateTagAndStatus(user_index, nick_color, user.name + clan_tag);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, user_index, pkt);
    }

    if (user.flags.Paralizado == 1) {
        user.flags.Paralizado = 0;
        ao::net::protocol::WriteParalizeOK(user_index);
    }

    if (user.flags.Estupidez == 1) {
        user.flags.Estupidez = 0;
        ao::net::protocol::WriteDumbNoMore(user_index);
    }

    if (user.flags.Descansar) {
        user.flags.Descansar = false;
        ao::net::protocol::WriteRestOK(user_index);
    }

    if (user.flags.Navegando == 1) {
        ToogleBoatBody(user_index);
    }
}

void VolverCriminal(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];

    if (IsArena(user_index)) return;

    if ((user.flags.Privilegios & (PlayerType::UserPlayer | PlayerType::Consejero)) != 0) {
        user.Reputacion.BurguesRep = 0;
        user.Reputacion.NobleRep = 0;
        user.Reputacion.PlebeRep = 0;
        user.Reputacion.BandidoRep += vlASALTO;
        if (user.Reputacion.BandidoRep > MAXREP) user.Reputacion.BandidoRep = MAXREP;

        if (user.Faccion.ArmadaReal == 1) {
            ModFacciones::ExpulsarFaccionReal(static_cast<std::int16_t>(user_index));
        }

        if (user.flags.AtacablePor > 0) {
            user.flags.AtacablePor = 0;
        }
    }

    RefreshCharStatus(user_index);
}

void VolverCiudadano(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];

    if (IsArena(user_index)) return;

    user.Reputacion.LadronesRep = 0;
    user.Reputacion.BandidoRep = 0;
    user.Reputacion.AsesinoRep = 0;
    user.Reputacion.PlebeRep += vlASALTO;
    if (user.Reputacion.PlebeRep > MAXREP) user.Reputacion.PlebeRep = MAXREP;

    if (user.Faccion.FuerzasCaos == 1) {
        ModFacciones::ExpulsarFaccionCaos(static_cast<std::int16_t>(user_index));
    }

    RefreshCharStatus(user_index);
}

void ContarMuerte(int muerto_index, int atacante_index) {
    if (muerto_index < 1 || muerto_index > MaxUsers || atacante_index < 1 || atacante_index > MaxUsers) return;

    if (UserList[muerto_index].Stats.ELV <= LimiteNewbie) return;
    if (IsArena(muerto_index)) return;

    // Respetar legítima defensa (AtacablePor == atacante_index)
    if (UserList[muerto_index].flags.AtacablePor == atacante_index) return;

    auto& atacante = UserList[atacante_index];
    const auto& muerto = UserList[muerto_index];

    if (FileIO::criminal(muerto_index)) {
        if (atacante.flags.LastCrimMatado != muerto.name) {
            atacante.flags.LastCrimMatado = muerto.name;
            if (atacante.Faccion.CriminalesMatados < MAXUSERMATADOS) {
                atacante.Faccion.CriminalesMatados++;
            }
        }
        if (atacante.Faccion.RecibioExpInicialCaos == 1 && muerto.Faccion.FuerzasCaos == 1) {
            atacante.Faccion.Reenlistadas = 200;
        }
    } else {
        if (atacante.flags.LastCiudMatado != muerto.name) {
            atacante.flags.LastCiudMatado = muerto.name;
            if (atacante.Faccion.CiudadanosMatados < MAXUSERMATADOS) {
                atacante.Faccion.CiudadanosMatados++;
            }
        }
    }

    if (atacante.Stats.UsuariosMatados < MAXUSERMATADOS) {
        atacante.Stats.UsuariosMatados++;
    }
}

bool EsMascotaCiudadano(int npc_index, int user_index) {
    if (npc_index > 0 && static_cast<std::size_t>(npc_index) < Npclist.size() && Npclist[npc_index].MaestroUser > 0) {
        int maestro = Npclist[npc_index].MaestroUser;
        bool is_ciuda = !FileIO::criminal(maestro);
        if (is_ciuda && user_index >= 1 && user_index <= MaxUsers) {
            ao::net::protocol::WriteConsoleMsg(maestro, "¡" + UserList[user_index].name + " está atacando tu mascota!!", FontTypeNames::FONTTYPE_INFO);
        }
        return is_ciuda;
    }
    return false;
}

void UserDie(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];

    // Reproducir sonido de muerte
    ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, user_index,
        ao::net::protocol::PrepareMessagePlayWave(SND_USERMUERTE, user.Pos.X, user.Pos.Y));

    // Quitar diálogo
    ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, user_index,
        ao::net::protocol::PrepareMessageRemoveCharDialog(user.char_appearance.CharIndex));

    user.Stats.MinHp = 0;
    user.Stats.MinSta = 0;
    user.flags.AtacadoPorUser = 0;
    user.flags.Envenenado = 0;
    user.flags.Muerto = 1;

    if (!IsArena(user_index)) {
        user.flags.SeguroResu = true;
        ao::net::protocol::WriteMultiMessage(user_index, eMessages::ResuscitationSafeOn);

        if (user.Stats.ELV <= LimiteNewbie) {
            InvUsuario::TirarTodosLosItemsNoNewbies(static_cast<std::int16_t>(user_index));
        } else {
            InvUsuario::TirarTodo(static_cast<std::int16_t>(user_index));
        }
    } else {
        user.flags.SeguroResu = false;
        ao::net::protocol::WriteMultiMessage(user_index, eMessages::ResuscitationSafeOff);
    }

    // Desequipar todo
    if (user.Invent.ArmourEqpObjIndex > 0) InvUsuario::Desequipar(static_cast<std::int16_t>(user_index), user.Invent.ArmourEqpSlot);
    if (user.Invent.WeaponEqpObjIndex > 0) InvUsuario::Desequipar(static_cast<std::int16_t>(user_index), user.Invent.WeaponEqpSlot);
    if (user.Invent.CascoEqpObjIndex > 0) InvUsuario::Desequipar(static_cast<std::int16_t>(user_index), user.Invent.CascoEqpSlot);
    if (user.Invent.AnilloEqpSlot > 0) InvUsuario::Desequipar(static_cast<std::int16_t>(user_index), user.Invent.AnilloEqpSlot);
    if (user.Invent.MunicionEqpObjIndex > 0) InvUsuario::Desequipar(static_cast<std::int16_t>(user_index), user.Invent.MunicionEqpSlot);
    if (user.Invent.EscudoEqpObjIndex > 0) InvUsuario::Desequipar(static_cast<std::int16_t>(user_index), user.Invent.EscudoEqpSlot);

    // Purgar estados alterados
    if (user.char_appearance.loops == INFINITE_LOOPS) {
        user.char_appearance.FX = 0;
        user.char_appearance.loops = 0;
    }

    if (user.flags.Mimetizado == 1) {
        user.char_appearance.body = user.CharMimetizado.body;
        user.char_appearance.Head = user.CharMimetizado.Head;
        user.char_appearance.CascoAnim = user.CharMimetizado.CascoAnim;
        user.char_appearance.ShieldAnim = user.CharMimetizado.ShieldAnim;
        user.char_appearance.WeaponAnim = user.CharMimetizado.WeaponAnim;
        user.Counters.Mimetismo = 0;
        user.flags.Mimetizado = 0;
        user.flags.Ignorado = false;
    }

    if (user.flags.AtacablePor > 0) {
        user.flags.AtacablePor = 0;
        RefreshCharStatus(user_index);
    }

    if (user.flags.Paralizado == 1) {
        user.flags.Paralizado = 0;
        ao::net::protocol::WriteParalizeOK(user_index);
    }

    if (user.flags.Estupidez == 1) {
        user.flags.Estupidez = 0;
        ao::net::protocol::WriteDumbNoMore(user_index);
    }

    if (user.flags.Descansar) {
        user.flags.Descansar = false;
        ao::net::protocol::WriteRestOK(user_index);
    }

    if (user.flags.Meditando) {
        user.flags.Meditando = false;
        ao::net::protocol::WriteMeditateToggle(user_index);
    }

    if (user.flags.invisible == 1 || user.flags.Oculto == 1) {
        user.flags.Oculto = 0;
        user.flags.invisible = 0;
        user.Counters.TiempoOculto = 0;
        user.Counters.Invisibilidad = 0;
        SetInvisible(user_index, user.char_appearance.CharIndex, false);
    }

    // Sprite de cuerpo muerto (Preservando user.OrigChar intacto)
    if (user.flags.Navegando == 0) {
        user.char_appearance.body = iCuerpoMuerto;
        user.char_appearance.Head = 0;
        user.char_appearance.ShieldAnim = NingunEscudo;
        user.char_appearance.WeaponAnim = NingunArma;
        user.char_appearance.CascoAnim = NingunCasco;
    } else {
        user.char_appearance.body = iFragataFantasmal;
    }

    user.NroMascotas = 0;

    ChangeUserChar(user_index, user.char_appearance.body, user.char_appearance.Head, user.char_appearance.heading, NingunArma, NingunEscudo, NingunCasco);
    ao::net::protocol::WriteUpdateUserStats(user_index);
    ao::net::protocol::WriteUpdateStrenghtAndDexterity(user_index);

    // Party penalty
    if (user.PartyIndex > 0) {
        std::int16_t miembros = ModParty::CantMiembros(static_cast<std::int16_t>(user_index));
        std::int32_t exp_loss = static_cast<std::int32_t>(user.Stats.ELV) * -10 * miembros;
        ModParty::ObtenerExito(static_cast<std::int16_t>(user_index), exp_loss, user.Pos.Map, user.Pos.X, user.Pos.Y);
    }

    // Limpiar comercio seguro
    ao::FinComerciarUsu(static_cast<std::int16_t>(user_index));
}

void RevivirUsuario(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];

    user.flags.Muerto = 0;
    user.Stats.MinHp = 1;

    if (user.flags.Navegando == 1) {
        ToogleBoatBody(user_index);
    } else {
        user.char_appearance.body = user.OrigChar.body;
        user.char_appearance.Head = user.OrigChar.Head;
        user.char_appearance.WeaponAnim = user.OrigChar.WeaponAnim;
        user.char_appearance.ShieldAnim = user.OrigChar.ShieldAnim;
        user.char_appearance.CascoAnim = user.OrigChar.CascoAnim;
    }

    ChangeUserChar(user_index, user.char_appearance.body, user.char_appearance.Head, user.char_appearance.heading, user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim, user.char_appearance.CascoAnim);
    ao::net::protocol::WriteUpdateUserStats(user_index);
}

// ==========================================
// Fase G5: Combate e Inventario
// ==========================================

bool ToogleToAtackable(int user_index, int owner_index, bool stealing_npc) {
    if (user_index < 1 || user_index > MaxUsers || owner_index < 1 || owner_index > MaxUsers) return false;
    auto& user = UserList[user_index];
    const auto& owner = UserList[owner_index];

    if (user_index == owner_index) return false;
    if (user.flags.Privilegios & (PlayerType::Admin | PlayerType::Dios | PlayerType::SemiDios | PlayerType::Consejero)) return false;

    if (!FileIO::criminal(user_index) && !FileIO::criminal(owner_index)) {
        if (user.flags.AtacablePor != owner_index) {
            user.flags.AtacablePor = owner_index;
            if (stealing_npc) {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡Has atacado a una criatura perteneciente a " + owner.name + "! Te ha considerado un atacante por 3 minutos.", FontTypeNames::FONTTYPE_INFO);
                ao::net::protocol::WriteConsoleMsg(owner_index, "¡" + user.name + " ha atacado a tu criatura! Tienes 3 minutos para atacarlo sin convertirte en criminal.", FontTypeNames::FONTTYPE_INFO);
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡Has atacado a " + owner.name + "! Te ha considerado un atacante por 3 minutos.", FontTypeNames::FONTTYPE_INFO);
                ao::net::protocol::WriteConsoleMsg(owner_index, "¡" + user.name + " te ha atacado! Tienes 3 minutos para atacarlo sin convertirte en criminal.", FontTypeNames::FONTTYPE_INFO);
            }
            return true;
        }
    }
    return false;
}

void setHome(int user_index, eCiudad new_home, int npc_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];
    user.Hogar = new_home;
    if (npc_index > 0) {
        ao::net::protocol::WriteConsoleMsg(user_index, "Tu hogar ha sido configurado.", FontTypeNames::FONTTYPE_INFO);
    }
}

void goHome(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];
    WorldPos pos{1, 50, 50}; // Coordenadas por defecto a Ullathorpe/Ciudad Natal
    WarpUserChar(user_index, pos.Map, pos.X, pos.Y, true, true);
}

void PerdioNpc(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return;
    auto& user = UserList[user_index];
    if (user.flags.TargetNPC > 0 && static_cast<std::size_t>(user.flags.TargetNPC) < Npclist.size()) {
        if (Npclist[user.flags.TargetNPC].Owner == user_index) {
            Npclist[user.flags.TargetNPC].Owner = 0;
        }
    }
    user.flags.TargetNPC = 0;
}

void ApropioNpc(int user_index, int npc_index) {
    if (user_index < 1 || user_index > MaxUsers || npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) return;
    auto& user = UserList[user_index];
    auto& npc = Npclist[npc_index];

    if (npc.Owner == 0) {
        npc.Owner = user_index;
        user.flags.TargetNPC = npc_index;
    }
}

bool SameFaccion(int user_index, int other_user_index) {
    if (user_index < 1 || user_index > MaxUsers || other_user_index < 1 || other_user_index > MaxUsers) return false;
    const auto& u1 = UserList[user_index];
    const auto& u2 = UserList[other_user_index];

    if (u1.Faccion.ArmadaReal == 1 && u2.Faccion.ArmadaReal == 1) return true;
    if (u1.Faccion.FuerzasCaos == 1 && u2.Faccion.FuerzasCaos == 1) return true;
    return false;
}

void NPCAtacado(int npc_index, int user_index) {
    if (user_index < 1 || user_index > MaxUsers || npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) return;
    auto& user = UserList[user_index];
    auto& npc = Npclist[npc_index];

    user.flags.NPCAtacado = npc_index;

    if (npc.MaestroUser > 0) {
        ToogleToAtackable(user_index, npc.MaestroUser, true);
    }

    if (npc.flags.Faccion != 0) {
        if (npc.flags.Faccion == 1 && user.Faccion.ArmadaReal == 1) {
            ModFacciones::ExpulsarFaccionReal(static_cast<std::int16_t>(user_index));
            VolverCriminal(user_index);
        } else if (npc.flags.Faccion == 2 && user.Faccion.FuerzasCaos == 1) {
            ModFacciones::ExpulsarFaccionCaos(static_cast<std::int16_t>(user_index));
            VolverCriminal(user_index);
        }
    }
}

bool PuedeApuñalar(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return false;
    const auto& user = UserList[user_index];
    if (user.clase != eClass::Assasin) return false;

    if (user.Invent.WeaponEqpObjIndex > 0 && static_cast<std::size_t>(user.Invent.WeaponEqpObjIndex) < ObjDataList.size()) {
        if (ObjDataList[user.Invent.WeaponEqpObjIndex].Apuñala == 1) {
            return (user.Stats.UserSkills[eSkill::Apuñalar] >= 10);
        }
    }
    return false;
}

bool PuedeAcuchillar(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return false;
    const auto& user = UserList[user_index];
    if (user.clase != eClass::Pirat) return false;

    if (user.Invent.WeaponEqpObjIndex > 0 && static_cast<std::size_t>(user.Invent.WeaponEqpObjIndex) < ObjDataList.size()) {
        if (ObjDataList[user.Invent.WeaponEqpObjIndex].Apuñala == 1) {
            return (user.Stats.UserSkills[eSkill::Apuñalar] >= 10);
        }
    }
    return false;
}

int GetWeaponAnim(int user_index, int obj_index) {
    if (obj_index > 0 && static_cast<std::size_t>(obj_index) < ObjDataList.size()) {
        return ObjDataList[obj_index].WeaponAnim;
    }
    return NingunArma;
}

void ChangeUserInv(int user_index, uint8_t slot, const UserOBJ& obj) {
    if (user_index < 1 || user_index > MaxUsers || slot < 1 || slot > MAX_INVENTORY_SLOTS) return;
    UserList[user_index].Invent.Object[slot] = obj;
    ao::net::protocol::WriteChangeInventorySlot(user_index, slot);
}

bool HasEnoughItems(int user_index, int obj_index, int32_t amount) {
    if (user_index < 1 || user_index > MaxUsers || amount <= 0) return false;
    int32_t count = 0;
    const auto& user = UserList[user_index];
    uint8_t max_slots = getMaxInventorySlots(user_index);

    for (uint8_t i = 1; i <= max_slots; ++i) {
        if (user.Invent.Object[i].ObjIndex == obj_index) {
            count += user.Invent.Object[i].Amount;
            if (count >= amount) return true;
        }
    }
    return false;
}

int32_t TotalOfferItems(int obj_index, int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return 0;
    int32_t count = 0;
    const auto& user = UserList[user_index];
    uint8_t max_slots = getMaxInventorySlots(user_index);

    for (uint8_t i = 1; i <= max_slots; ++i) {
        if (user.Invent.Object[i].ObjIndex == obj_index) {
            count += user.Invent.Object[i].Amount;
        }
    }
    return count;
}

uint8_t getMaxInventorySlots(int user_index) {
    if (user_index < 1 || user_index > MaxUsers) return MAX_NORMAL_INVENTORY_SLOTS;
    const auto& user = UserList[user_index];
    if (user.Invent.MochilaEqpSlot > 0) {
        return MAX_INVENTORY_SLOTS; // 30
    }
    return MAX_NORMAL_INVENTORY_SLOTS; // 20
}

} // namespace Modulo_UsUaRiOs
