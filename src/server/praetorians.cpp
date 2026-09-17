#include "server/praetorians.hpp"
#include "server/Declares.hpp"
#include "server/modSendData.hpp"

namespace praetorians {

namespace {
PraetorianCallbacks g_callbacks{};

inline std::size_t mapBlockIndex(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return (static_cast<std::size_t>(map) * 101 + static_cast<std::size_t>(x)) * 101 + static_cast<std::size_t>(y);
}
}

void SetCallbacks(const PraetorianCallbacks& cb) {
    g_callbacks = cb;
}

void ResetCallbacks() {
    g_callbacks = PraetorianCallbacks{};
}

std::int16_t pretorianosVivos{0};

std::int16_t esPretoriano(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) {
        return 0;
    }
    const auto npc_num = Npclist[npc_index].Numero;
    switch (npc_num) {
    case PRCLER_NPC: return 1;
    case PRMAGO_NPC: return 2;
    case PRCAZA_NPC: return 3;
    case PRKING_NPC: return 4;
    case PRGUER_NPC: return 5;
    default: return 0;
    }
}

bool EstoyMuyLejos(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) {
        return false;
    }
    const bool retvalue = Npclist[npc_index].Pos.Y > 39;
    if (Npclist[npc_index].Pos.Map != MAPA_PRETORIANO) {
        return false;
    }
    return retvalue;
}

bool EstoyLejos(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) {
        return false;
    }
    bool retvalue = false;
    if (Npclist[npc_index].Pos.X < 50) {
        retvalue = Npclist[npc_index].Pos.X < 43 && Npclist[npc_index].Pos.X > 27;
    } else {
        retvalue = Npclist[npc_index].Pos.X < 75 && Npclist[npc_index].Pos.X > 59;
    }
    retvalue = retvalue && Npclist[npc_index].Pos.Y > 19 && Npclist[npc_index].Pos.Y < 31;
    if (Npclist[npc_index].Pos.Map != MAPA_PRETORIANO) {
        return false;
    }
    return !retvalue;
}

bool EsAlcanzable(std::int16_t npc_index, std::int16_t user_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS || user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return false;
    }
    const auto pj_pos_x = UserList[user_index].Pos.X;
    const auto pj_pos_y = UserList[user_index].Pos.Y;
    const auto npc_pos_x = Npclist[npc_index].Pos.X;
    if (Npclist[npc_index].Pos.Map == MAPA_PRETORIANO && UserList[user_index].Pos.Map == MAPA_PRETORIANO) {
        bool retvalue = pj_pos_x > 18 && pj_pos_x < 49 && npc_pos_x <= 51;
        retvalue = retvalue && (pj_pos_y > 14 && pj_pos_y < 40);
        bool retvalue2 = pj_pos_x > 52 && pj_pos_x < 81 && npc_pos_x > 51;
        retvalue2 = retvalue2 && (pj_pos_y > 14 && pj_pos_y < 40);
        return retvalue || retvalue2;
    }
    return false;
}

bool CasperBlock(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) {
        return false;
    }
    const auto npc_pos_x = Npclist[npc_index].Pos.X;
    const auto npc_pos_y = Npclist[npc_index].Pos.Y;
    const auto npc_pos_m = Npclist[npc_index].Pos.Map;
    auto legal_pos_cb = g_callbacks.LegalPos;
    if (!legal_pos_cb) {
        return false;
    }
    bool retvalue = !(legal_pos_cb(npc_pos_m, npc_pos_x + 1, npc_pos_y) ||
                      legal_pos_cb(npc_pos_m, npc_pos_x - 1, npc_pos_y) ||
                      legal_pos_cb(npc_pos_m, npc_pos_x, npc_pos_y + 1) ||
                      legal_pos_cb(npc_pos_m, npc_pos_x, npc_pos_y - 1));
    if (retvalue) {
        retvalue = false;
        std::int16_t pj = MapData[mapBlockIndex(npc_pos_m, npc_pos_x + 1, npc_pos_y)].UserIndex;
        if (pj > 0 && static_cast<std::size_t>(pj) < UserList.size()) {
            retvalue = (UserList[pj].flags.Muerto == 1);
        }
        pj = MapData[mapBlockIndex(npc_pos_m, npc_pos_x - 1, npc_pos_y)].UserIndex;
        if (pj > 0 && static_cast<std::size_t>(pj) < UserList.size()) {
            retvalue = retvalue || (UserList[pj].flags.Muerto == 1);
        }
        pj = MapData[mapBlockIndex(npc_pos_m, npc_pos_x, npc_pos_y + 1)].UserIndex;
        if (pj > 0 && static_cast<std::size_t>(pj) < UserList.size()) {
            retvalue = retvalue || (UserList[pj].flags.Muerto == 1);
        }
        pj = MapData[mapBlockIndex(npc_pos_m, npc_pos_x, npc_pos_y - 1)].UserIndex;
        if (pj > 0 && static_cast<std::size_t>(pj) < UserList.size()) {
            retvalue = retvalue || (UserList[pj].flags.Muerto == 1);
        }
    } else {
        retvalue = false;
    }
    return retvalue;
}

void LiberarCasperBlock(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) {
        return;
    }
    const auto npc_pos_x = Npclist[npc_index].Pos.X;
    const auto npc_pos_y = Npclist[npc_index].Pos.Y;
    const auto npc_pos_m = Npclist[npc_index].Pos.Map;
    auto legal_pos_cb = g_callbacks.LegalPos;
    auto send_data_cb = g_callbacks.SendData;
    auto prep_move_cb = g_callbacks.PrepareMessageCharacterMove;
    auto prep_chat_cb = g_callbacks.PrepareMessageChatOverHead;
    if (!legal_pos_cb) {
        return;
    }
    constexpr std::int32_t vbYellow = 0x00FFFF;
    const auto to_npc_area = static_cast<std::int16_t>(ao::net::send_data::SendTarget::ToNPCArea);
    if (legal_pos_cb(npc_pos_m, npc_pos_x + 1, npc_pos_y + 1)) {
        if (send_data_cb && prep_move_cb) {
            send_data_cb(to_npc_area, npc_index, prep_move_cb(Npclist[npc_index].char_appearance.CharIndex, npc_pos_x + 1, npc_pos_y + 1));
        }
        MapData[mapBlockIndex(npc_pos_m, npc_pos_x, npc_pos_y)].NpcIndex = 0;
        Npclist[npc_index].Pos.Y = npc_pos_y + 1;
        Npclist[npc_index].Pos.X = npc_pos_x + 1;
        Npclist[npc_index].char_appearance.heading = eHeading::SOUTH;
        MapData[mapBlockIndex(npc_pos_m, npc_pos_x + 1, npc_pos_y + 1)].NpcIndex = npc_index;
        if (send_data_cb && prep_chat_cb) {
            send_data_cb(to_npc_area, npc_index, prep_chat_cb("JA JA JA JA!!", Npclist[npc_index].char_appearance.CharIndex, vbYellow));
        }
        return;
    }
    if (legal_pos_cb(npc_pos_m, npc_pos_x - 1, npc_pos_y - 1)) {
        if (send_data_cb && prep_move_cb) {
            send_data_cb(to_npc_area, npc_index, prep_move_cb(Npclist[npc_index].char_appearance.CharIndex, npc_pos_x - 1, npc_pos_y - 1));
        }
        MapData[mapBlockIndex(npc_pos_m, npc_pos_x, npc_pos_y)].NpcIndex = 0;
        Npclist[npc_index].Pos.Y = npc_pos_y - 1;
        Npclist[npc_index].Pos.X = npc_pos_x - 1;
        Npclist[npc_index].char_appearance.heading = eHeading::NORTH;
        MapData[mapBlockIndex(npc_pos_m, npc_pos_x - 1, npc_pos_y - 1)].NpcIndex = npc_index;
        if (send_data_cb && prep_chat_cb) {
            send_data_cb(to_npc_area, npc_index, prep_chat_cb("JA JA JA JA!!", Npclist[npc_index].char_appearance.CharIndex, vbYellow));
        }
        return;
    }
    if (legal_pos_cb(npc_pos_m, npc_pos_x + 1, npc_pos_y - 1)) {
        if (send_data_cb && prep_move_cb) {
            send_data_cb(to_npc_area, npc_index, prep_move_cb(Npclist[npc_index].char_appearance.CharIndex, npc_pos_x + 1, npc_pos_y - 1));
        }
        MapData[mapBlockIndex(npc_pos_m, npc_pos_x, npc_pos_y)].NpcIndex = 0;
        Npclist[npc_index].Pos.Y = npc_pos_y - 1;
        Npclist[npc_index].Pos.X = npc_pos_x + 1;
        Npclist[npc_index].char_appearance.heading = eHeading::EAST;
        MapData[mapBlockIndex(npc_pos_m, npc_pos_x + 1, npc_pos_y - 1)].NpcIndex = npc_index;
        if (send_data_cb && prep_chat_cb) {
            send_data_cb(to_npc_area, npc_index, prep_chat_cb("JA JA JA JA!!", Npclist[npc_index].char_appearance.CharIndex, vbYellow));
        }
        return;
    }
    if (legal_pos_cb(npc_pos_m, npc_pos_x - 1, npc_pos_y + 1)) {
        if (send_data_cb && prep_move_cb) {
            send_data_cb(to_npc_area, npc_index, prep_move_cb(Npclist[npc_index].char_appearance.CharIndex, npc_pos_x - 1, npc_pos_y + 1));
        }
        MapData[mapBlockIndex(npc_pos_m, npc_pos_x, npc_pos_y)].NpcIndex = 0;
        Npclist[npc_index].Pos.Y = npc_pos_y + 1;
        Npclist[npc_index].Pos.X = npc_pos_x - 1;
        Npclist[npc_index].char_appearance.heading = eHeading::WEST;
        MapData[mapBlockIndex(npc_pos_m, npc_pos_x - 1, npc_pos_y + 1)].NpcIndex = npc_index;
        if (send_data_cb && prep_chat_cb) {
            send_data_cb(to_npc_area, npc_index, prep_chat_cb("JA JA JA JA!!", Npclist[npc_index].char_appearance.CharIndex, vbYellow));
        }
        return;
    }
    if (Npclist[npc_index].CanAttack == 1) {
        if (send_data_cb && prep_chat_cb) {
            send_data_cb(to_npc_area, npc_index, prep_chat_cb("Por las barbas de los antiguos reyes! Alejaos endemoniados espectros o sufrireis la furia de los dioses!", Npclist[npc_index].char_appearance.CharIndex, vbYellow));
        }
        Npclist[npc_index].CanAttack = 0;
    }
}

void MoverAba(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;
    if (CasperBlock(npc_index)) {
        LiberarCasperBlock(npc_index);
    } else {
        if (g_callbacks.MoveNPCChar) {
            g_callbacks.MoveNPCChar(npc_index, static_cast<std::uint8_t>(eHeading::SOUTH));
        }
    }
}

void MoverArr(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;
    if (CasperBlock(npc_index)) {
        LiberarCasperBlock(npc_index);
    } else {
        if (g_callbacks.MoveNPCChar) {
            g_callbacks.MoveNPCChar(npc_index, static_cast<std::uint8_t>(eHeading::NORTH));
        }
    }
}

void MoverIzq(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;
    if (CasperBlock(npc_index)) {
        LiberarCasperBlock(npc_index);
    } else {
        if (g_callbacks.MoveNPCChar) {
            g_callbacks.MoveNPCChar(npc_index, static_cast<std::uint8_t>(eHeading::WEST));
        }
    }
}

void MoverDer(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;
    if (CasperBlock(npc_index)) {
        LiberarCasperBlock(npc_index);
    } else {
        if (g_callbacks.MoveNPCChar) {
            g_callbacks.MoveNPCChar(npc_index, static_cast<std::uint8_t>(eHeading::EAST));
        }
    }
}

void GreedyWalkTo(std::int16_t npc_index, std::int16_t map, std::int16_t x, std::int16_t y) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;
    const auto legal_pos_cb = g_callbacks.LegalPos;
    if (!legal_pos_cb) return;

    const std::int16_t orig_x = Npclist[npc_index].Pos.X;
    const std::int16_t orig_y = Npclist[npc_index].Pos.Y;
    const std::int16_t orig_m = Npclist[npc_index].Pos.Map;

    if (orig_m != map) return;

    const std::int16_t d_x = x - orig_x;
    const std::int16_t d_y = y - orig_y;

    if (std::abs(d_x) >= std::abs(d_y)) {
        if (d_x > 0) {
            if (legal_pos_cb(map, orig_x + 1, orig_y)) {
                MoverDer(npc_index);
            } else if (d_y > 0) {
                if (legal_pos_cb(map, orig_x, orig_y + 1)) {
                    MoverAba(npc_index);
                }
            } else if (d_y < 0) {
                if (legal_pos_cb(map, orig_x, orig_y - 1)) {
                    MoverArr(npc_index);
                }
            }
        } else if (d_x < 0) {
            if (legal_pos_cb(map, orig_x - 1, orig_y)) {
                MoverIzq(npc_index);
            } else if (d_y > 0) {
                if (legal_pos_cb(map, orig_x, orig_y + 1)) {
                    MoverAba(npc_index);
                }
            } else if (d_y < 0) {
                if (legal_pos_cb(map, orig_x, orig_y - 1)) {
                    MoverArr(npc_index);
                }
            }
        }
    } else {
        if (d_y > 0) {
            if (legal_pos_cb(map, orig_x, orig_y + 1)) {
                MoverAba(npc_index);
            } else if (d_x > 0) {
                if (legal_pos_cb(map, orig_x + 1, orig_y)) {
                    MoverDer(npc_index);
                }
            } else if (d_x < 0) {
                if (legal_pos_cb(map, orig_x - 1, orig_y)) {
                    MoverIzq(npc_index);
                }
            }
        } else if (d_y < 0) {
            if (legal_pos_cb(map, orig_x, orig_y - 1)) {
                MoverArr(npc_index);
            } else if (d_x > 0) {
                if (legal_pos_cb(map, orig_x + 1, orig_y)) {
                    MoverDer(npc_index);
                }
            } else if (d_x < 0) {
                if (legal_pos_cb(map, orig_x - 1, orig_y)) {
                    MoverIzq(npc_index);
                }
            }
        }
    }
}

void VolverAlCentro(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;
    if (Npclist[npc_index].Pos.X < 50) {
        GreedyWalkTo(npc_index, MAPA_PRETORIANO, ALCOBA1_X, ALCOBA1_Y);
    } else {
        GreedyWalkTo(npc_index, MAPA_PRETORIANO, ALCOBA2_X, ALCOBA2_Y);
    }
}

void CambiarAlcoba(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;

    if (Npclist[npc_index].Invent.ArmourEqpSlot == 0) {
        Npclist[npc_index].Invent.ArmourEqpSlot = 1;
    }

    struct Waypoint {
        std::int16_t x;
        std::int16_t y;
    };

    constexpr Waypoint waypoints[9] = {
        {0, 0},
        {35, 14}, // Waypoint 1
        {50, 14}, // Waypoint 2
        {67, 14}, // Waypoint 3
        {67, 25}, // Waypoint 4
        {67, 14}, // Waypoint 5
        {50, 14}, // Waypoint 6
        {35, 14}, // Waypoint 7
        {35, 25}  // Waypoint 8
    };

    const auto slot = Npclist[npc_index].Invent.ArmourEqpSlot;
    if (slot < 1 || slot > 8) {
        Npclist[npc_index].Invent.ArmourEqpSlot = 1;
        return;
    }

    const Waypoint target = waypoints[slot];
    if (Npclist[npc_index].Pos.X == target.x && Npclist[npc_index].Pos.Y == target.y) {
        if (Npclist[npc_index].Invent.ArmourEqpSlot == 8) {
            Npclist[npc_index].Invent.ArmourEqpSlot = 1;
        } else {
            Npclist[npc_index].Invent.ArmourEqpSlot++;
        }
    } else {
        GreedyWalkTo(npc_index, MAPA_PRETORIANO, target.x, target.y);
    }
}

bool EsMagoOClerigo(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return false;
    }
    const auto user_class = UserList[user_index].clase;
    return (user_class == eClass::Mage || user_class == eClass::Cleric);
}

void NPCRemueveVenenoNPC(std::int16_t npc_index, std::int16_t ally_npc_index, std::int16_t spell_slot) {
    if (npc_index <= 0 || npc_index > MAXNPCS || ally_npc_index <= 0 || ally_npc_index > MAXNPCS) return;
    if (spell_slot < 0 || static_cast<std::size_t>(spell_slot) >= Npclist[npc_index].Spells.size()) return;

    const auto spell_index = Npclist[npc_index].Spells[spell_slot];
    if (spell_index <= 0 || static_cast<std::size_t>(spell_index) >= Hechizos.size()) return;

    const auto mana_req = Hechizos[spell_index].ManaRequerido;
    if (Npclist[npc_index].Stats.MinMAN < mana_req) return;

    Npclist[npc_index].Stats.MinMAN -= mana_req;
    Npclist[ally_npc_index].Veneno = 0;

    const auto send_data_cb = g_callbacks.SendData;
    const auto prep_fx_cb = g_callbacks.PrepareMessageCreateFX;
    if (send_data_cb && prep_fx_cb) {
        const auto to_npc_area = static_cast<std::int16_t>(ao::net::send_data::SendTarget::ToNPCArea);
        send_data_cb(to_npc_area, ally_npc_index, prep_fx_cb(Npclist[ally_npc_index].char_appearance.CharIndex, Hechizos[spell_index].FXgrh, Hechizos[spell_index].loops));
    }
}

void NPCCuraLevesNPC(std::int16_t npc_index, std::int16_t ally_npc_index, std::int16_t spell_slot) {
    if (npc_index <= 0 || npc_index > MAXNPCS || ally_npc_index <= 0 || ally_npc_index > MAXNPCS) return;
    if (spell_slot < 0 || static_cast<std::size_t>(spell_slot) >= Npclist[npc_index].Spells.size()) return;

    const auto spell_index = Npclist[npc_index].Spells[spell_slot];
    if (spell_index <= 0 || static_cast<std::size_t>(spell_index) >= Hechizos.size()) return;

    const auto mana_req = Hechizos[spell_index].ManaRequerido;
    if (Npclist[npc_index].Stats.MinMAN < mana_req) return;

    Npclist[npc_index].Stats.MinMAN -= mana_req;

    std::int32_t heal_amount = 0;
    if (g_callbacks.RandomNumber) {
        heal_amount = g_callbacks.RandomNumber(Hechizos[spell_index].MinHp, Hechizos[spell_index].MaxHp);
    } else {
        heal_amount = Hechizos[spell_index].MinHp;
    }

    Npclist[ally_npc_index].Stats.MinHp += static_cast<std::int16_t>(heal_amount);
    if (Npclist[ally_npc_index].Stats.MinHp > Npclist[ally_npc_index].Stats.MaxHp) {
        Npclist[ally_npc_index].Stats.MinHp = Npclist[ally_npc_index].Stats.MaxHp;
    }

    const auto send_data_cb = g_callbacks.SendData;
    const auto prep_fx_cb = g_callbacks.PrepareMessageCreateFX;
    if (send_data_cb && prep_fx_cb) {
        const auto to_npc_area = static_cast<std::int16_t>(ao::net::send_data::SendTarget::ToNPCArea);
        send_data_cb(to_npc_area, ally_npc_index, prep_fx_cb(Npclist[ally_npc_index].char_appearance.CharIndex, Hechizos[spell_index].FXgrh, Hechizos[spell_index].loops));
    }
}

void NPCRemueveParalisisNPC(std::int16_t npc_index, std::int16_t ally_npc_index, std::int16_t spell_slot) {
    if (npc_index <= 0 || npc_index > MAXNPCS || ally_npc_index <= 0 || ally_npc_index > MAXNPCS) return;
    if (spell_slot < 0 || static_cast<std::size_t>(spell_slot) >= Npclist[npc_index].Spells.size()) return;

    const auto spell_index = Npclist[npc_index].Spells[spell_slot];
    if (spell_index <= 0 || static_cast<std::size_t>(spell_index) >= Hechizos.size()) return;

    const auto mana_req = Hechizos[spell_index].ManaRequerido;
    if (Npclist[npc_index].Stats.MinMAN < mana_req) return;

    Npclist[npc_index].Stats.MinMAN -= mana_req;
    Npclist[ally_npc_index].flags.Paralizado = 0;
    Npclist[ally_npc_index].flags.Inmovilizado = 0;

    const auto send_data_cb = g_callbacks.SendData;
    const auto prep_fx_cb = g_callbacks.PrepareMessageCreateFX;
    if (send_data_cb && prep_fx_cb) {
        const auto to_npc_area = static_cast<std::int16_t>(ao::net::send_data::SendTarget::ToNPCArea);
        send_data_cb(to_npc_area, ally_npc_index, prep_fx_cb(Npclist[ally_npc_index].char_appearance.CharIndex, Hechizos[spell_index].FXgrh, Hechizos[spell_index].loops));
    }
}

void NPCcuraNPC(std::int16_t healer_npc, std::int16_t target_npc, std::int16_t spell_slot) {
    NPCCuraLevesNPC(healer_npc, target_npc, spell_slot);
}

void NPCparalizaNPC(std::int16_t caster_npc, std::int16_t target_index, std::int16_t spell_slot) {
    if (caster_npc <= 0 || caster_npc > MAXNPCS || target_index <= 0 || target_index > MAXNPCS) return;
    if (spell_slot < 0 || static_cast<std::size_t>(spell_slot) >= Npclist[caster_npc].Spells.size()) return;

    const auto spell_index = Npclist[caster_npc].Spells[spell_slot];
    if (spell_index <= 0 || static_cast<std::size_t>(spell_index) >= Hechizos.size()) return;

    const auto mana_req = Hechizos[spell_index].ManaRequerido;
    if (Npclist[caster_npc].Stats.MinMAN < mana_req) return;

    Npclist[caster_npc].Stats.MinMAN -= mana_req;
    Npclist[target_index].flags.Paralizado = 1;

    const auto send_data_cb = g_callbacks.SendData;
    const auto prep_fx_cb = g_callbacks.PrepareMessageCreateFX;
    if (send_data_cb && prep_fx_cb) {
        const auto to_npc_area = static_cast<std::int16_t>(ao::net::send_data::SendTarget::ToNPCArea);
        send_data_cb(to_npc_area, target_index, prep_fx_cb(Npclist[target_index].char_appearance.CharIndex, Hechizos[spell_index].FXgrh, Hechizos[spell_index].loops));
    }
}

void NPCLanzaCegueraPJ(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_slot) {
    if (npc_index <= 0 || npc_index > MAXNPCS || user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) return;
    if (spell_slot < 0 || static_cast<std::size_t>(spell_slot) >= Npclist[npc_index].Spells.size()) return;

    const auto spell_index = Npclist[npc_index].Spells[spell_slot];
    if (spell_index <= 0 || static_cast<std::size_t>(spell_index) >= Hechizos.size()) return;

    const auto mana_req = Hechizos[spell_index].ManaRequerido;
    if (Npclist[npc_index].Stats.MinMAN < mana_req) return;

    Npclist[npc_index].Stats.MinMAN -= mana_req;
    UserList[user_index].flags.Ceguera = 1;

    const auto send_data_cb = g_callbacks.SendData;
    const auto prep_fx_cb = g_callbacks.PrepareMessageCreateFX;
    if (send_data_cb && prep_fx_cb) {
        const auto to_pc_area = static_cast<std::int16_t>(ao::net::send_data::SendTarget::ToPCArea);
        send_data_cb(to_pc_area, user_index, prep_fx_cb(UserList[user_index].char_appearance.CharIndex, Hechizos[spell_index].FXgrh, Hechizos[spell_index].loops));
    }
}

void NPCLanzaEstupidezPJ(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_slot) {
    if (npc_index <= 0 || npc_index > MAXNPCS || user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) return;
    if (spell_slot < 0 || static_cast<std::size_t>(spell_slot) >= Npclist[npc_index].Spells.size()) return;

    const auto spell_index = Npclist[npc_index].Spells[spell_slot];
    if (spell_index <= 0 || static_cast<std::size_t>(spell_index) >= Hechizos.size()) return;

    const auto mana_req = Hechizos[spell_index].ManaRequerido;
    if (Npclist[npc_index].Stats.MinMAN < mana_req) return;

    Npclist[npc_index].Stats.MinMAN -= mana_req;
    UserList[user_index].flags.Estupidez = 1;

    const auto send_data_cb = g_callbacks.SendData;
    const auto prep_fx_cb = g_callbacks.PrepareMessageCreateFX;
    if (send_data_cb && prep_fx_cb) {
        const auto to_pc_area = static_cast<std::int16_t>(ao::net::send_data::SendTarget::ToPCArea);
        send_data_cb(to_pc_area, user_index, prep_fx_cb(UserList[user_index].char_appearance.CharIndex, Hechizos[spell_index].FXgrh, Hechizos[spell_index].loops));
    }
}

void NPCRemueveInvisibilidad(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_slot) {
    if (npc_index <= 0 || npc_index > MAXNPCS || user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) return;
    if (spell_slot < 0 || static_cast<std::size_t>(spell_slot) >= Npclist[npc_index].Spells.size()) return;

    const auto spell_index = Npclist[npc_index].Spells[spell_slot];
    if (spell_index <= 0 || static_cast<std::size_t>(spell_index) >= Hechizos.size()) return;

    const auto mana_req = Hechizos[spell_index].ManaRequerido;
    if (Npclist[npc_index].Stats.MinMAN < mana_req) return;

    Npclist[npc_index].Stats.MinMAN -= mana_req;
    UserList[user_index].flags.invisible = 0;
    UserList[user_index].flags.Oculto = 0;

    const auto send_data_cb = g_callbacks.SendData;
    const auto prep_fx_cb = g_callbacks.PrepareMessageCreateFX;
    const auto prep_chat_cb = g_callbacks.PrepareMessageChatOverHead;
    constexpr std::int32_t vbYellow = 0x00FFFF;

    if (send_data_cb) {
        const auto to_pc_area = static_cast<std::int16_t>(ao::net::send_data::SendTarget::ToPCArea);
        if (prep_fx_cb) {
            send_data_cb(to_pc_area, user_index, prep_fx_cb(UserList[user_index].char_appearance.CharIndex, Hechizos[spell_index].FXgrh, Hechizos[spell_index].loops));
        }
        if (prep_chat_cb) {
            send_data_cb(static_cast<std::int16_t>(ao::net::send_data::SendTarget::ToNPCArea), npc_index, prep_chat_cb("Revelate villano!", Npclist[npc_index].char_appearance.CharIndex, vbYellow));
        }
    }
}

void NpcLanzaSpellSobreUser2(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_index) {
    if (g_callbacks.NpcLanzaSpellSobreUser) {
        g_callbacks.NpcLanzaSpellSobreUser(npc_index, user_index, spell_index);
    }
}

void MagoDestruyeWand(std::int16_t npc_index, std::uint8_t wand_counter, std::int16_t spell_slot) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;

    if (Npclist[npc_index].Invent.BarcoSlot == 0) {
        Npclist[npc_index].Invent.BarcoSlot = wand_counter;
    } else {
        Npclist[npc_index].Invent.BarcoSlot--;
    }

    if (Npclist[npc_index].Invent.BarcoSlot <= 1) {
        const auto send_data_cb = g_callbacks.SendData;
        const auto prep_fx_cb = g_callbacks.PrepareMessageCreateFX;
        const auto prep_chat_cb = g_callbacks.PrepareMessageChatOverHead;
        constexpr std::int32_t vbYellow = 0x00FFFF;

        if (send_data_cb && prep_chat_cb) {
            send_data_cb(static_cast<std::int16_t>(ao::net::send_data::SendTarget::ToNPCArea), npc_index, prep_chat_cb("HA HA HA! Muere junto a mi misera escoria!", Npclist[npc_index].char_appearance.CharIndex, vbYellow));
        }

        const auto m = Npclist[npc_index].Pos.Map;
        const auto x = Npclist[npc_index].Pos.X;
        const auto y = Npclist[npc_index].Pos.Y;

        for (std::int16_t loop_x = x - 2; loop_x <= x + 2; ++loop_x) {
            for (std::int16_t loop_y = y - 2; loop_y <= y + 2; ++loop_y) {
                if (loop_x >= 1 && loop_x <= 100 && loop_y >= 1 && loop_y <= 100) {
                    const auto idx = mapBlockIndex(m, loop_x, loop_y);
                    if (idx < MapData.size()) {
                        const std::int16_t user_idx = MapData[idx].UserIndex;
                        if (user_idx > 0 && static_cast<std::size_t>(user_idx) < UserList.size()) {
                            if (send_data_cb && prep_fx_cb) {
                                send_data_cb(static_cast<std::int16_t>(ao::net::send_data::SendTarget::ToPCArea), user_idx, prep_fx_cb(UserList[user_idx].char_appearance.CharIndex, 5, 0));
                            }
                            if (UserList[user_idx].flags.Muerto == 0) {
                                UserList[user_idx].Stats.MinHp -= 300;
                                if (UserList[user_idx].Stats.MinHp < 0) {
                                    UserList[user_idx].Stats.MinHp = 0;
                                }
                            }
                        }
                    }
                }
            }
        }

        if (g_callbacks.MuereNpc) {
            g_callbacks.MuereNpc(npc_index, 0);
        }
    }
}

void PRREY_AI(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;
    if (EstoyLejos(npc_index)) {
        VolverAlCentro(npc_index);
    }
}

void PRGUER_AI(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;
    if (EstoyMuyLejos(npc_index)) {
        VolverAlCentro(npc_index);
        return;
    }

    const auto npc_x = Npclist[npc_index].Pos.X;
    const auto npc_y = Npclist[npc_index].Pos.Y;
    const auto npc_m = Npclist[npc_index].Pos.Map;

    std::int16_t closest_user = 0;
    std::int32_t min_dist = 999999;

    for (std::size_t u = 1; u < UserList.size(); ++u) {
        if (UserList[u].flags.Muerto == 0 && UserList[u].Pos.Map == npc_m) {
            const auto u_x = UserList[u].Pos.X;
            const auto u_y = UserList[u].Pos.Y;
            if (EsAlcanzable(npc_index, static_cast<std::int16_t>(u))) {
                const std::int32_t dist = std::max(std::abs(u_x - npc_x), std::abs(u_y - npc_y));
                if (dist < min_dist) {
                    min_dist = dist;
                    closest_user = static_cast<std::int16_t>(u);
                }
            }
        }
    }

    if (closest_user > 0) {
        if (min_dist <= 1) {
            if (g_callbacks.NpcAtacaUser) {
                g_callbacks.NpcAtacaUser(npc_index, closest_user);
            }
        } else {
            GreedyWalkTo(npc_index, npc_m, UserList[closest_user].Pos.X, UserList[closest_user].Pos.Y);
        }
    } else {
        CambiarAlcoba(npc_index);
    }
}

void PRCAZA_AI(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;
    if (EstoyMuyLejos(npc_index)) {
        VolverAlCentro(npc_index);
        return;
    }

    const auto npc_x = Npclist[npc_index].Pos.X;
    const auto npc_y = Npclist[npc_index].Pos.Y;
    const auto npc_m = Npclist[npc_index].Pos.Map;

    std::int16_t closest_user = 0;
    std::int32_t min_dist = 999999;

    for (std::size_t u = 1; u < UserList.size(); ++u) {
        if (UserList[u].flags.Muerto == 0 && UserList[u].Pos.Map == npc_m) {
            if (EsAlcanzable(npc_index, static_cast<std::int16_t>(u))) {
                const std::int32_t dist = std::max(std::abs(UserList[u].Pos.X - npc_x), std::abs(UserList[u].Pos.Y - npc_y));
                if (dist < min_dist) {
                    min_dist = dist;
                    closest_user = static_cast<std::int16_t>(u);
                }
            }
        }
    }

    if (closest_user > 0) {
        if (min_dist <= 1) {
            const std::int16_t opp_x = npc_x + (npc_x - UserList[closest_user].Pos.X);
            const std::int16_t opp_y = npc_y + (npc_y - UserList[closest_user].Pos.Y);
            GreedyWalkTo(npc_index, npc_m, opp_x, opp_y);
        } else {
            if (g_callbacks.NpcAtacaUser) {
                g_callbacks.NpcAtacaUser(npc_index, closest_user);
            }
        }
    } else {
        CambiarAlcoba(npc_index);
    }
}

void PRMAGO_AI(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;

    if (Npclist[npc_index].Stats.MinHp < 750) {
        if (Npclist[npc_index].Invent.BarcoSlot == 0) {
            MagoDestruyeWand(npc_index, 6, 0);
        } else {
            MagoDestruyeWand(npc_index, Npclist[npc_index].Invent.BarcoSlot, 0);
        }
        return;
    }

    if (EstoyMuyLejos(npc_index)) {
        VolverAlCentro(npc_index);
        return;
    }

    const auto npc_m = Npclist[npc_index].Pos.Map;
    const auto npc_x = Npclist[npc_index].Pos.X;
    const auto npc_y = Npclist[npc_index].Pos.Y;

    for (std::size_t u = 1; u < UserList.size(); ++u) {
        if (UserList[u].flags.Muerto == 0 && UserList[u].Pos.Map == npc_m) {
            if (UserList[u].flags.invisible == 1 || UserList[u].flags.Oculto == 1) {
                if (g_callbacks.RandomNumber && g_callbacks.RandomNumber(1, 100) <= 35) {
                    NPCRemueveInvisibilidad(npc_index, static_cast<std::int16_t>(u), 0);
                    return;
                }
            }
        }
    }

    std::int16_t target_user = 0;
    std::int32_t min_dist = 999999;

    for (std::size_t u = 1; u < UserList.size(); ++u) {
        if (UserList[u].flags.Muerto == 0 && UserList[u].Pos.Map == npc_m) {
            if (EsAlcanzable(npc_index, static_cast<std::int16_t>(u))) {
                const std::int32_t dist = std::max(std::abs(UserList[u].Pos.X - npc_x), std::abs(UserList[u].Pos.Y - npc_y));
                if (dist < min_dist) {
                    min_dist = dist;
                    target_user = static_cast<std::int16_t>(u);
                }
            }
        }
    }

    if (target_user > 0) {
        if (!Npclist[npc_index].Spells.empty()) {
            NpcLanzaSpellSobreUser2(npc_index, target_user, Npclist[npc_index].Spells[0]);
        }
    } else {
        CambiarAlcoba(npc_index);
    }
}

void PRCLER_AI(std::int16_t npc_index) {
    if (npc_index <= 0 || npc_index > MAXNPCS) return;

    const auto npc_m = Npclist[npc_index].Pos.Map;
    const auto npc_x = Npclist[npc_index].Pos.X;
    const auto npc_y = Npclist[npc_index].Pos.Y;

    // 1. Check allies for Poison
    for (std::int16_t ally = 1; ally <= MAXNPCS; ++ally) {
        if (ally != npc_index && Npclist[ally].Pos.Map == npc_m && esPretoriano(ally) > 0) {
            const std::int32_t dist = std::max(std::abs(Npclist[ally].Pos.X - npc_x), std::abs(Npclist[ally].Pos.Y - npc_y));
            if (dist <= 12 && Npclist[ally].Veneno == 1) {
                NPCRemueveVenenoNPC(npc_index, ally, 0);
                return;
            }
        }
    }

    // 2. Check allies for Paralysis
    for (std::int16_t ally = 1; ally <= MAXNPCS; ++ally) {
        if (ally != npc_index && Npclist[ally].Pos.Map == npc_m && esPretoriano(ally) > 0) {
            const std::int32_t dist = std::max(std::abs(Npclist[ally].Pos.X - npc_x), std::abs(Npclist[ally].Pos.Y - npc_y));
            if (dist <= 12 && (Npclist[ally].flags.Paralizado == 1 || Npclist[ally].flags.Inmovilizado == 1)) {
                NPCRemueveParalisisNPC(npc_index, ally, 0);
                return;
            }
        }
    }

    // 3. Check allies for Healing
    for (std::int16_t ally = 1; ally <= MAXNPCS; ++ally) {
        if (ally != npc_index && Npclist[ally].Pos.Map == npc_m && esPretoriano(ally) > 0) {
            const std::int32_t dist = std::max(std::abs(Npclist[ally].Pos.X - npc_x), std::abs(Npclist[ally].Pos.Y - npc_y));
            if (dist <= 12 && Npclist[ally].Stats.MinHp < Npclist[ally].Stats.MaxHp) {
                NPCCuraLevesNPC(npc_index, ally, 0);
                return;
            }
        }
    }

    if (EstoyMuyLejos(npc_index)) {
        VolverAlCentro(npc_index);
        return;
    }

    // 4. CC / Attack enemies
    std::int16_t target_user = 0;
    std::int32_t min_dist = 999999;

    for (std::size_t u = 1; u < UserList.size(); ++u) {
        if (UserList[u].flags.Muerto == 0 && UserList[u].Pos.Map == npc_m) {
            if (EsAlcanzable(npc_index, static_cast<std::int16_t>(u))) {
                const std::int32_t dist = std::max(std::abs(UserList[u].Pos.X - npc_x), std::abs(UserList[u].Pos.Y - npc_y));
                if (dist < min_dist) {
                    min_dist = dist;
                    target_user = static_cast<std::int16_t>(u);
                }
            }
        }
    }

    if (target_user > 0) {
        if (EsMagoOClerigo(target_user) && UserList[target_user].flags.Paralizado == 0) {
            if (!Npclist[npc_index].Spells.empty()) {
                NpcLanzaSpellSobreUser2(npc_index, target_user, Npclist[npc_index].Spells[0]);
            }
        } else {
            if (g_callbacks.NpcAtacaUser) {
                g_callbacks.NpcAtacaUser(npc_index, target_user);
            }
        }
    } else {
        CambiarAlcoba(npc_index);
    }
}

void CrearClanPretoriano(std::int16_t previous_king_x) {
    if (!g_callbacks.CrearNPC) return;

    std::int16_t base_x = 0;
    std::int16_t base_y = 0;

    if (previous_king_x >= 50) {
        base_x = ALCOBA1_X;
        base_y = ALCOBA1_Y;
    } else {
        base_x = ALCOBA2_X;
        base_y = ALCOBA2_Y;
    }

    const std::int16_t map = MAPA_PRETORIANO;

    // 1 King
    g_callbacks.CrearNPC(PRKING_NPC, map, WorldPos{map, base_x, base_y});

    // 2 Clerics
    g_callbacks.CrearNPC(PRCLER_NPC, map, WorldPos{map, static_cast<std::int16_t>(base_x - 2), static_cast<std::int16_t>(base_y + 1)});
    g_callbacks.CrearNPC(PRCLER_NPC, map, WorldPos{map, static_cast<std::int16_t>(base_x + 2), static_cast<std::int16_t>(base_y + 1)});

    // 3 Warriors
    g_callbacks.CrearNPC(PRGUER_NPC, map, WorldPos{map, static_cast<std::int16_t>(base_x - 1), static_cast<std::int16_t>(base_y + 2)});
    g_callbacks.CrearNPC(PRGUER_NPC, map, WorldPos{map, base_x, static_cast<std::int16_t>(base_y + 2)});
    g_callbacks.CrearNPC(PRGUER_NPC, map, WorldPos{map, static_cast<std::int16_t>(base_x + 1), static_cast<std::int16_t>(base_y + 2)});

    // 1 Hunter
    g_callbacks.CrearNPC(PRCAZA_NPC, map, WorldPos{map, static_cast<std::int16_t>(base_x - 1), static_cast<std::int16_t>(base_y + 1)});

    // 1 Mage
    g_callbacks.CrearNPC(PRMAGO_NPC, map, WorldPos{map, static_cast<std::int16_t>(base_x + 1), static_cast<std::int16_t>(base_y + 1)});

    pretorianosVivos = 7;
}

} // namespace praetorians