/*
 * Módulo de Inteligencia Artificial de Criaturas (AI_NPC) - Fases 1, 2, 3 y 4 (Completo)
 */

#include "AI_NPC.hpp"
#include "Matematicas.hpp"
#include <cmath>

namespace AI_NPC {

namespace {
AiNpcCallbacks g_callbacks{};

bool HelperInMapBounds(std::int16_t map, std::int16_t x, std::int16_t y) {
    if (g_callbacks.InMapBounds) {
        return g_callbacks.InMapBounds(map, x, y);
    }
    std::uint8_t min_x = (MinXBorder > 0) ? MinXBorder : 1;
    std::uint8_t max_x = (MaxXBorder > 0) ? MaxXBorder : 100;
    std::uint8_t min_y = (MinYBorder > 0) ? MinYBorder : 1;
    std::uint8_t max_y = (MaxYBorder > 0) ? MaxYBorder : 100;
    return (map > 0 && x >= min_x && x <= max_x && y >= min_y && y <= max_y);
}

void HelperHeadtoPos(std::uint8_t heading, WorldPos& pos) {
    if (g_callbacks.HeadtoPos) {
        g_callbacks.HeadtoPos(heading, pos);
    } else {
        switch (static_cast<eHeading>(heading)) {
            case eHeading::NORTH: pos.Y--; break;
            case eHeading::EAST:  pos.X++; break;
            case eHeading::SOUTH: pos.Y++; break;
            case eHeading::WEST:  pos.X--; break;
        }
    }
}

inline std::size_t MapBlockIndex(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return static_cast<std::size_t>(map) * 101 * 101 + static_cast<std::size_t>(x) * 101 + static_cast<std::size_t>(y);
}

std::int16_t HelperGetMapUserIndex(std::int16_t map, std::int16_t x, std::int16_t y) {
    if (g_callbacks.GetMapUserIndex) {
        return g_callbacks.GetMapUserIndex(map, x, y);
    }
    std::size_t idx = MapBlockIndex(map, x, y);
    if (idx < MapData.size()) {
        return MapData[idx].UserIndex;
    }
    return 0;
}

std::int16_t HelperGetMapNpcIndex(std::int16_t map, std::int16_t x, std::int16_t y) {
    if (g_callbacks.GetMapNpcIndex) {
        return g_callbacks.GetMapNpcIndex(map, x, y);
    }
    std::size_t idx = MapBlockIndex(map, x, y);
    if (idx < MapData.size()) {
        return MapData[idx].NpcIndex;
    }
    return 0;
}

bool IsUserProtected(std::int16_t user_index) {
    if (!g_callbacks.GetUser) {
        return true;
    }
    const auto& user = g_callbacks.GetUser(user_index);

    bool protected_by_interval = false;
    if (g_callbacks.IntervaloPermiteSerAtacado) {
        protected_by_interval = !g_callbacks.IntervaloPermiteSerAtacado(user_index);
    }

    bool user_protected = protected_by_interval && user.flags.NoPuedeSerAtacado;
    user_protected = user_protected || user.flags.Ignorado || user.flags.EnConsulta;
    return user_protected;
}

} // namespace

void SetCallbacks(const AiNpcCallbacks& cb) noexcept {
    g_callbacks = cb;
}

void ResetCallbacks() noexcept {
    g_callbacks = AiNpcCallbacks{};
}

// ============================================================================
// FASE 1 (G1)
// ============================================================================

void NpcLanzaUnSpell(std::int16_t npc_index, std::int16_t user_index) {
    if (g_callbacks.GetUser) {
        const auto& user = g_callbacks.GetUser(user_index);
        if (user.flags.invisible == 1 || user.flags.Oculto == 1) {
            return;
        }
    }

    std::int16_t max_spells = Npclist[npc_index].flags.LanzaSpells;
    if (max_spells <= 0) {
        return;
    }

    std::int32_t k = 1;
    if (g_callbacks.RandomNumber) {
        k = g_callbacks.RandomNumber(1, max_spells);
    }

    if (k >= 1 && static_cast<std::size_t>(k) < Npclist[npc_index].Spells.size()) {
        std::int16_t spell_id = Npclist[npc_index].Spells[k];
        if (g_callbacks.NpcLanzaSpellSobreUser) {
            g_callbacks.NpcLanzaSpellSobreUser(npc_index, user_index, spell_id);
        }
    }
}

void NpcLanzaUnSpellSobreNpc(std::int16_t npc_index, std::int16_t target_npc) {
    std::int16_t max_spells = Npclist[npc_index].flags.LanzaSpells;
    if (max_spells <= 0) {
        return;
    }

    std::int32_t k = 1;
    if (g_callbacks.RandomNumber) {
        k = g_callbacks.RandomNumber(1, max_spells);
    }

    if (k >= 1 && static_cast<std::size_t>(k) < Npclist[npc_index].Spells.size()) {
        std::int16_t spell_id = Npclist[npc_index].Spells[k];
        if (g_callbacks.NpcLanzaSpellSobreNpc) {
            g_callbacks.NpcLanzaSpellSobreNpc(npc_index, target_npc, spell_id);
        }
    }
}

bool UserNear(std::int16_t npc_index) {
    const auto& npc_ref = Npclist[npc_index];
    std::int16_t target_user = npc_ref.PFINFO.TargetUser;
    if (target_user <= 0) {
        return false;
    }

    if (!g_callbacks.GetUser) {
        return false;
    }

    const auto& user = g_callbacks.GetUser(target_user);
    double dist = 0.0;
    if (g_callbacks.Distancia) {
        dist = g_callbacks.Distancia(npc_ref.Pos, user.Pos);
    } else {
        dist = Distance(npc_ref.Pos.X, npc_ref.Pos.Y, user.Pos.X, user.Pos.Y);
    }

    // Paridad estricta con VB6: Not Int(Distance(...)) > 1
    return !(static_cast<int>(dist) > 1);
}

bool ReCalculatePath(std::int16_t npc_index) {
    const auto& pf = Npclist[npc_index].PFINFO;
    if (pf.PathLenght == 0) {
        return true;
    }
    
    if (!UserNear(npc_index) && pf.PathLenght == (pf.CurPos - 1)) {
        return true;
    }

    return false;
}

bool PathEnd(std::int16_t npc_index) {
    return Npclist[npc_index].PFINFO.CurPos == Npclist[npc_index].PFINFO.PathLenght;
}

bool FollowPath(std::int16_t npc_index) {
    auto& npc_ref = Npclist[npc_index];
    if (npc_ref.PFINFO.CurPos <= 0 || 
        static_cast<std::size_t>(npc_ref.PFINFO.CurPos) >= npc_ref.PFINFO.Path.size()) {
        return false;
    }

    WorldPos tmpPos;
    tmpPos.Map = npc_ref.Pos.Map;

    // Inversión de coordenadas histórica al leer del path
    tmpPos.X = npc_ref.PFINFO.Path[npc_ref.PFINFO.CurPos].Y;
    tmpPos.Y = npc_ref.PFINFO.Path[npc_ref.PFINFO.CurPos].X;

    std::uint8_t tHeading = 0;
    if (g_callbacks.FindDirection) {
        tHeading = g_callbacks.FindDirection(npc_ref.Pos, tmpPos);
    }

    if (g_callbacks.MoveNPCChar) {
        g_callbacks.MoveNPCChar(npc_index, tHeading);
    }

    npc_ref.PFINFO.CurPos++;
    return true;
}

bool PathFindingAI(std::int16_t npc_index) {
    auto& npc_ref = Npclist[npc_index];

    for (std::int16_t y = npc_ref.Pos.Y - 10; y <= npc_ref.Pos.Y + 10; ++y) {
        for (std::int16_t x = npc_ref.Pos.X - 10; x <= npc_ref.Pos.X + 10; ++x) {
            if (HelperInMapBounds(npc_ref.Pos.Map, x, y)) {
                std::int16_t user_index = HelperGetMapUserIndex(npc_ref.Pos.Map, x, y);

                if (user_index > 0 && g_callbacks.GetUser) {
                    const auto& user = g_callbacks.GetUser(user_index);
                    if (user.flags.Muerto == 0 && user.flags.invisible == 0 &&
                        user.flags.Oculto == 0 && user.flags.AdminPerseguible) {

                        // Inversión deliberada X <-> Y (Bug #45)
                        npc_ref.PFINFO.Target.X = user.Pos.Y;
                        npc_ref.PFINFO.Target.Y = user.Pos.X; // ops!
                        npc_ref.PFINFO.TargetUser = user_index;

                        if (g_callbacks.SeekPath) {
                            g_callbacks.SeekPath(npc_index);
                        }

                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// ============================================================================
// FASE 2 (G2)
// ============================================================================

void GuardiasAI(std::int16_t npc_index, bool del_caos) {
    auto& npc_ref = Npclist[npc_index];

    for (std::uint8_t headingloop = static_cast<std::uint8_t>(eHeading::NORTH);
         headingloop <= static_cast<std::uint8_t>(eHeading::WEST); ++headingloop) {

        WorldPos nPos = npc_ref.Pos;
        if (npc_ref.flags.Inmovilizado == 0 || headingloop == static_cast<std::uint8_t>(npc_ref.char_appearance.heading)) {
            HelperHeadtoPos(headingloop, nPos);

            if (HelperInMapBounds(nPos.Map, nPos.X, nPos.Y)) {
                std::int16_t ui = HelperGetMapUserIndex(nPos.Map, nPos.X, nPos.Y);

                if (ui > 0 && g_callbacks.GetUser) {
                    const auto& user = g_callbacks.GetUser(ui);

                    bool user_protected = IsUserProtected(ui);

                    if (user.flags.Muerto == 0 && user.flags.AdminPerseguible && !user_protected) {
                        bool es_crim = false;
                        if (g_callbacks.EsCriminal) {
                            es_crim = g_callbacks.EsCriminal(ui);
                        }

                        if (!del_caos) {
                            // Guardias Reales atacan criminales
                            if (es_crim) {
                                if (g_callbacks.NpcAtacaUser && g_callbacks.NpcAtacaUser(npc_index, ui)) {
                                    if (g_callbacks.ChangeNPCChar) {
                                        g_callbacks.ChangeNPCChar(npc_index, npc_ref.char_appearance.body,
                                                                   npc_ref.char_appearance.Head, headingloop);
                                    }
                                }
                                return;
                            } else if (npc_ref.flags.AttackedBy == user.name && !npc_ref.flags.Follow) {
                                if (g_callbacks.NpcAtacaUser && g_callbacks.NpcAtacaUser(npc_index, ui)) {
                                    if (g_callbacks.ChangeNPCChar) {
                                        g_callbacks.ChangeNPCChar(npc_index, npc_ref.char_appearance.body,
                                                                   npc_ref.char_appearance.Head, headingloop);
                                    }
                                }
                                return;
                            }
                        } else {
                            // Guardias del Caos atacan ciudadanos (no criminales)
                            if (!es_crim) {
                                if (g_callbacks.NpcAtacaUser && g_callbacks.NpcAtacaUser(npc_index, ui)) {
                                    if (g_callbacks.ChangeNPCChar) {
                                        g_callbacks.ChangeNPCChar(npc_index, npc_ref.char_appearance.body,
                                                                   npc_ref.char_appearance.Head, headingloop);
                                    }
                                }
                                return;
                            } else if (npc_ref.flags.AttackedBy == user.name && !npc_ref.flags.Follow) {
                                if (g_callbacks.NpcAtacaUser && g_callbacks.NpcAtacaUser(npc_index, ui)) {
                                    if (g_callbacks.ChangeNPCChar) {
                                        g_callbacks.ChangeNPCChar(npc_index, npc_ref.char_appearance.body,
                                                                   npc_ref.char_appearance.Head, headingloop);
                                    }
                                }
                                return;
                            }
                        }
                    }
                }
            }
        }
    }

    RestoreOldMovement(npc_index);
}

void HostilMalvadoAI(std::int16_t npc_index) {
    auto& npc_ref = Npclist[npc_index];

    for (std::uint8_t headingloop = static_cast<std::uint8_t>(eHeading::NORTH);
         headingloop <= static_cast<std::uint8_t>(eHeading::WEST); ++headingloop) {

        WorldPos nPos = npc_ref.Pos;
        if (npc_ref.flags.Inmovilizado == 0 || headingloop == static_cast<std::uint8_t>(npc_ref.char_appearance.heading)) {
            HelperHeadtoPos(headingloop, nPos);

            if (HelperInMapBounds(nPos.Map, nPos.X, nPos.Y)) {
                std::int16_t ui = HelperGetMapUserIndex(nPos.Map, nPos.X, nPos.Y);
                std::int16_t npci = HelperGetMapNpcIndex(nPos.Map, nPos.X, nPos.Y);

                if (ui > 0 && g_callbacks.GetUser) {
                    const auto& user = g_callbacks.GetUser(ui);
                    bool user_protected = IsUserProtected(ui);

                    if (user.flags.Muerto == 0 && user.flags.AdminPerseguible && !user_protected) {
                        if (npc_ref.flags.LanzaSpells > 0) {
                            std::int32_t rnd = 1;
                            if (g_callbacks.RandomNumber) {
                                rnd = g_callbacks.RandomNumber(1, 2);
                            }

                            if (rnd == 1) {
                                if (g_callbacks.NpcAtacaUser && g_callbacks.NpcAtacaUser(npc_index, ui)) {
                                    if (g_callbacks.ChangeNPCChar) {
                                        g_callbacks.ChangeNPCChar(npc_index, npc_ref.char_appearance.body,
                                                                   npc_ref.char_appearance.Head, headingloop);
                                    }
                                    if (npc_ref.flags.LanzaSpells > 0) {
                                        NpcLanzaUnSpell(npc_index, ui);
                                    }
                                    return;
                                }
                            } else {
                                if (g_callbacks.ChangeNPCChar) {
                                    g_callbacks.ChangeNPCChar(npc_index, npc_ref.char_appearance.body,
                                                               npc_ref.char_appearance.Head, headingloop);
                                }
                                NpcLanzaUnSpell(npc_index, ui);
                                return;
                            }
                        }

                        if (g_callbacks.NpcAtacaUser && g_callbacks.NpcAtacaUser(npc_index, ui)) {
                            if (g_callbacks.ChangeNPCChar) {
                                g_callbacks.ChangeNPCChar(npc_index, npc_ref.char_appearance.body,
                                                           npc_ref.char_appearance.Head, headingloop);
                            }
                        }
                        return;
                    }
                } else if (npci > 0) {
                    const auto& target_npc = Npclist[npci];
                    if (target_npc.MaestroUser > 0 && target_npc.flags.Paralizado == 0) {
                        if (g_callbacks.ChangeNPCChar) {
                            g_callbacks.ChangeNPCChar(npc_index, npc_ref.char_appearance.body,
                                                       npc_ref.char_appearance.Head, headingloop);
                        }
                        if (g_callbacks.NpcAtacaNpc) {
                            g_callbacks.NpcAtacaNpc(npc_index, npci, false);
                        }
                        return;
                    }
                }
            }
        }
    }

    RestoreOldMovement(npc_index);
}

void HostilBuenoAI(std::int16_t npc_index) {
    auto& npc_ref = Npclist[npc_index];

    for (std::uint8_t headingloop = static_cast<std::uint8_t>(eHeading::NORTH);
         headingloop <= static_cast<std::uint8_t>(eHeading::WEST); ++headingloop) {

        WorldPos nPos = npc_ref.Pos;
        if (npc_ref.flags.Inmovilizado == 0 || headingloop == static_cast<std::uint8_t>(npc_ref.char_appearance.heading)) {
            HelperHeadtoPos(headingloop, nPos);

            if (HelperInMapBounds(nPos.Map, nPos.X, nPos.Y)) {
                std::int16_t ui = HelperGetMapUserIndex(nPos.Map, nPos.X, nPos.Y);

                if (ui > 0 && g_callbacks.GetUser) {
                    const auto& user = g_callbacks.GetUser(ui);

                    if (user.name == npc_ref.flags.AttackedBy) {
                        bool user_protected = IsUserProtected(ui);

                        if (user.flags.Muerto == 0 && user.flags.AdminPerseguible && !user_protected) {
                            if (npc_ref.flags.LanzaSpells > 0) {
                                NpcLanzaUnSpell(npc_index, ui);
                            }

                            if (g_callbacks.NpcAtacaUser && g_callbacks.NpcAtacaUser(npc_index, ui)) {
                                if (g_callbacks.ChangeNPCChar) {
                                    g_callbacks.ChangeNPCChar(npc_index, npc_ref.char_appearance.body,
                                                               npc_ref.char_appearance.Head, headingloop);
                                }
                            }
                            return;
                        }
                    }
                }
            }
        }
    }

    RestoreOldMovement(npc_index);
}

void RestoreOldMovement(std::int16_t npc_index) {
    auto& npc_ref = Npclist[npc_index];
    if (npc_ref.MaestroUser == 0) {
        npc_ref.Movement = npc_ref.flags.OldMovement;
        npc_ref.Hostile = npc_ref.flags.OldHostil;
        npc_ref.flags.AttackedBy.clear();
    }
}

void AiNpcObjeto(std::int16_t npc_index) {
    const auto& npc_ref = Npclist[npc_index];
    std::int16_t map = npc_ref.Pos.Map;

    if (!g_callbacks.GetConnGroupUserEntries || !g_callbacks.GetUser) {
        return;
    }

    const auto& user_entries = g_callbacks.GetConnGroupUserEntries(map);

    for (std::int16_t user_index : user_entries) {
        if (user_index <= 0) {
            continue;
        }

        const auto& user = g_callbacks.GetUser(user_index);

        if (std::abs(user.Pos.X - npc_ref.Pos.X) <= RANGO_VISION_X &&
            std::abs(user.Pos.Y - npc_ref.Pos.Y) <= RANGO_VISION_Y) {

            bool user_protected = false;
            if (g_callbacks.IntervaloPermiteSerAtacado) {
                user_protected = !g_callbacks.IntervaloPermiteSerAtacado(user_index) && user.flags.NoPuedeSerAtacado;
            }

            if (user.flags.Muerto == 0 && user.flags.invisible == 0 &&
                user.flags.Oculto == 0 && user.flags.AdminPerseguible && !user_protected) {

                // Probabilidad 2/3 (RandomNumber(1, 3) < 3)
                std::int32_t rnd = 1;
                if (g_callbacks.RandomNumber) {
                    rnd = g_callbacks.RandomNumber(1, 3);
                }

                if (rnd < 3) {
                    if (npc_ref.flags.LanzaSpells > 0) {
                        NpcLanzaUnSpell(npc_index, user_index);
                    }
                    return;
                }
            }
        }
    }
}

// ============================================================================
// FASE 3 (G3)
// ============================================================================

void IrUsuarioCercano(std::int16_t npc_index) {
    auto& npc_ref = Npclist[npc_index];

    if (npc_ref.flags.Inmovilizado == 1) {
        int signo_ns = 0;
        int signo_eo = 0;
        switch (npc_ref.char_appearance.heading) {
            case eHeading::NORTH: signo_ns = -1; signo_eo = 0; break;
            case eHeading::EAST:  signo_ns = 0;  signo_eo = 1; break;
            case eHeading::SOUTH: signo_ns = 1;  signo_eo = 0; break;
            case eHeading::WEST:  signo_ns = 0;  signo_eo = -1; break;
        }

        if (g_callbacks.GetConnGroupUserEntries && g_callbacks.GetUser) {
            const auto& user_entries = g_callbacks.GetConnGroupUserEntries(npc_ref.Pos.Map);
            for (std::int16_t user_index : user_entries) {
                if (user_index <= 0) continue;
                const auto& user = g_callbacks.GetUser(user_index);

                int dx = user.Pos.X - npc_ref.Pos.X;
                int dy = user.Pos.Y - npc_ref.Pos.Y;
                int sgn_x = (dx > 0) - (dx < 0);
                int sgn_y = (dy > 0) - (dy < 0);

                if (std::abs(dx) <= RANGO_VISION_X && sgn_x == signo_eo) {
                    if (std::abs(dy) <= RANGO_VISION_Y && sgn_y == signo_ns) {
                        if (user.flags.Muerto == 0 && !IsUserProtected(user_index)) {
                            if (npc_ref.flags.LanzaSpells != 0) {
                                NpcLanzaUnSpell(npc_index, user_index);
                            }
                            return;
                        }
                    }
                }
            }
        }
    } else {
        std::int16_t owner_index = npc_ref.Owner;
        if (owner_index > 0 && g_callbacks.GetUser) {
            const auto& owner = g_callbacks.GetUser(owner_index);
            if (std::abs(owner.Pos.X - npc_ref.Pos.X) <= RANGO_VISION_X &&
                std::abs(owner.Pos.Y - npc_ref.Pos.Y) <= RANGO_VISION_Y) {
                if (owner.flags.invisible == 0 && owner.flags.Oculto == 0 &&
                    !owner.flags.EnConsulta && !owner.flags.Ignorado) {
                    if (npc_ref.flags.LanzaSpells != 0) {
                        NpcLanzaUnSpell(npc_index, owner_index);
                    }
                    if (g_callbacks.FindDirection && g_callbacks.MoveNPCChar) {
                        std::uint8_t t_heading = g_callbacks.FindDirection(npc_ref.Pos, owner.Pos);
                        g_callbacks.MoveNPCChar(npc_index, t_heading);
                    }
                    return;
                }
            }
        }

        if (g_callbacks.GetConnGroupUserEntries && g_callbacks.GetUser) {
            const auto& user_entries = g_callbacks.GetConnGroupUserEntries(npc_ref.Pos.Map);
            for (std::int16_t user_index : user_entries) {
                if (user_index <= 0) continue;
                const auto& user = g_callbacks.GetUser(user_index);

                if (std::abs(user.Pos.X - npc_ref.Pos.X) <= RANGO_VISION_X &&
                    std::abs(user.Pos.Y - npc_ref.Pos.Y) <= RANGO_VISION_Y) {
                    if (user.flags.Muerto == 0 && user.flags.invisible == 0 &&
                        user.flags.Oculto == 0 && user.flags.AdminPerseguible &&
                        !IsUserProtected(user_index)) {

                        if (npc_ref.flags.LanzaSpells != 0) {
                            NpcLanzaUnSpell(npc_index, user_index);
                        }
                        if (g_callbacks.FindDirection && g_callbacks.MoveNPCChar) {
                            std::uint8_t t_heading = g_callbacks.FindDirection(npc_ref.Pos, user.Pos);
                            g_callbacks.MoveNPCChar(npc_index, t_heading);
                        }
                        return;
                    }
                }
            }
        }
    }
}

void SeguirAgresor(std::int16_t npc_index) {
    auto& npc_ref = Npclist[npc_index];

    if (npc_ref.flags.Paralizado == 1 || npc_ref.flags.Inmovilizado == 1) {
        int signo_ns = 0;
        int signo_eo = 0;
        switch (npc_ref.char_appearance.heading) {
            case eHeading::NORTH: signo_ns = -1; signo_eo = 0; break;
            case eHeading::EAST:  signo_ns = 0;  signo_eo = 1; break;
            case eHeading::SOUTH: signo_ns = 1;  signo_eo = 0; break;
            case eHeading::WEST:  signo_ns = 0;  signo_eo = -1; break;
        }

        if (g_callbacks.GetConnGroupUserEntries && g_callbacks.GetUser) {
            const auto& user_entries = g_callbacks.GetConnGroupUserEntries(npc_ref.Pos.Map);
            for (std::int16_t ui : user_entries) {
                if (ui <= 0) continue;
                const auto& user = g_callbacks.GetUser(ui);

                int dx = user.Pos.X - npc_ref.Pos.X;
                int dy = user.Pos.Y - npc_ref.Pos.Y;
                int sgn_x = (dx > 0) - (dx < 0);
                int sgn_y = (dy > 0) - (dy < 0);

                if (std::abs(dx) <= RANGO_VISION_X && sgn_x == signo_eo &&
                    std::abs(dy) <= RANGO_VISION_Y && sgn_y == signo_ns) {

                    if (user.name == npc_ref.flags.AttackedBy) {
                        if (npc_ref.MaestroUser > 0) {
                            bool master_crim = g_callbacks.EsCriminal && g_callbacks.EsCriminal(npc_ref.MaestroUser);
                            bool ui_crim = g_callbacks.EsCriminal && g_callbacks.EsCriminal(ui);
                            const auto& master = g_callbacks.GetUser(npc_ref.MaestroUser);

                            if (!master_crim && !ui_crim && (master.flags.Seguro || master.Faccion.ArmadaReal == 1)) {
                                if (g_callbacks.WriteConsoleMsg) {
                                    g_callbacks.WriteConsoleMsg(npc_ref.MaestroUser, "La mascota no atacará a ciudadanos si eres miembro del ejército real o tienes el seguro activado.", 15);
                                }
                                if (g_callbacks.FlushBuffer) {
                                    g_callbacks.FlushBuffer(npc_ref.MaestroUser);
                                }
                                npc_ref.flags.AttackedBy.clear();
                                return;
                            }
                        }

                        if (user.flags.Muerto == 0 && user.flags.invisible == 0 && user.flags.Oculto == 0) {
                            if (npc_ref.flags.LanzaSpells > 0) {
                                NpcLanzaUnSpell(npc_index, ui);
                            } else {
                                double dist = g_callbacks.Distancia ? g_callbacks.Distancia(user.Pos, npc_ref.Pos) : Distance(user.Pos.X, user.Pos.Y, npc_ref.Pos.X, npc_ref.Pos.Y);
                                if (dist <= 1.0) {
                                    if (npc_ref.Numero != ELEMENTALAGUA) {
                                        if (g_callbacks.NpcAtacaUser) {
                                            g_callbacks.NpcAtacaUser(npc_index, ui);
                                        }
                                    }
                                }
                            }
                            return;
                        }
                    }
                }
            }
        }
    } else {
        if (g_callbacks.GetConnGroupUserEntries && g_callbacks.GetUser) {
            const auto& user_entries = g_callbacks.GetConnGroupUserEntries(npc_ref.Pos.Map);
            for (std::int16_t ui : user_entries) {
                if (ui <= 0) continue;
                const auto& user = g_callbacks.GetUser(ui);

                if (std::abs(user.Pos.X - npc_ref.Pos.X) <= RANGO_VISION_X &&
                    std::abs(user.Pos.Y - npc_ref.Pos.Y) <= RANGO_VISION_Y) {

                    if (user.name == npc_ref.flags.AttackedBy) {
                        if (npc_ref.MaestroUser > 0) {
                            bool master_crim = g_callbacks.EsCriminal && g_callbacks.EsCriminal(npc_ref.MaestroUser);
                            bool ui_crim = g_callbacks.EsCriminal && g_callbacks.EsCriminal(ui);
                            const auto& master = g_callbacks.GetUser(npc_ref.MaestroUser);

                            if (!master_crim && !ui_crim && (master.flags.Seguro || master.Faccion.ArmadaReal == 1)) {
                                if (g_callbacks.WriteConsoleMsg) {
                                    g_callbacks.WriteConsoleMsg(npc_ref.MaestroUser, "La mascota no atacará a ciudadanos si eres miembro del ejército real o tienes el seguro activado.", 15);
                                }
                                if (g_callbacks.FlushBuffer) {
                                    g_callbacks.FlushBuffer(npc_ref.MaestroUser);
                                }
                                npc_ref.flags.AttackedBy.clear();
                                if (g_callbacks.FollowAmo) {
                                    g_callbacks.FollowAmo(npc_index);
                                }
                                return;
                            }
                        }

                        if (user.flags.Muerto == 0 && user.flags.invisible == 0 && user.flags.Oculto == 0) {
                            if (npc_ref.flags.LanzaSpells > 0) {
                                NpcLanzaUnSpell(npc_index, ui);
                            } else {
                                double dist = g_callbacks.Distancia ? g_callbacks.Distancia(user.Pos, npc_ref.Pos) : Distance(user.Pos.X, user.Pos.Y, npc_ref.Pos.X, npc_ref.Pos.Y);
                                if (dist <= 1.0) {
                                    if (npc_ref.Numero != ELEMENTALAGUA) {
                                        if (g_callbacks.NpcAtacaUser) {
                                            g_callbacks.NpcAtacaUser(npc_index, ui);
                                        }
                                    }
                                }
                            }

                            if (g_callbacks.FindDirection && g_callbacks.MoveNPCChar) {
                                std::uint8_t t_heading = g_callbacks.FindDirection(npc_ref.Pos, user.Pos);
                                g_callbacks.MoveNPCChar(npc_index, t_heading);
                            }
                            return;
                        }
                    }
                }
            }
        }
    }

    RestoreOldMovement(npc_index);
}

void PersigueCiudadano(std::int16_t npc_index) {
    auto& npc_ref = Npclist[npc_index];

    if (g_callbacks.GetConnGroupUserEntries && g_callbacks.GetUser) {
        const auto& user_entries = g_callbacks.GetConnGroupUserEntries(npc_ref.Pos.Map);
        for (std::int16_t user_index : user_entries) {
            if (user_index <= 0) continue;
            const auto& user = g_callbacks.GetUser(user_index);

            if (std::abs(user.Pos.X - npc_ref.Pos.X) <= RANGO_VISION_X &&
                std::abs(user.Pos.Y - npc_ref.Pos.Y) <= RANGO_VISION_Y) {

                bool es_crim = g_callbacks.EsCriminal && g_callbacks.EsCriminal(user_index);
                if (!es_crim) {
                    if (user.flags.Muerto == 0 && user.flags.invisible == 0 &&
                        user.flags.Oculto == 0 && user.flags.AdminPerseguible &&
                        !IsUserProtected(user_index)) {

                        if (npc_ref.flags.LanzaSpells > 0) {
                            NpcLanzaUnSpell(npc_index, user_index);
                        }
                        if (g_callbacks.FindDirection && g_callbacks.MoveNPCChar) {
                            std::uint8_t t_heading = g_callbacks.FindDirection(npc_ref.Pos, user.Pos);
                            g_callbacks.MoveNPCChar(npc_index, t_heading);
                        }
                        return;
                    }
                }
            }
        }
    }

    RestoreOldMovement(npc_index);
}

void PersigueCriminal(std::int16_t npc_index) {
    auto& npc_ref = Npclist[npc_index];

    if (npc_ref.flags.Inmovilizado == 1) {
        int signo_ns = 0;
        int signo_eo = 0;
        switch (npc_ref.char_appearance.heading) {
            case eHeading::NORTH: signo_ns = -1; signo_eo = 0; break;
            case eHeading::EAST:  signo_ns = 0;  signo_eo = 1; break;
            case eHeading::SOUTH: signo_ns = 1;  signo_eo = 0; break;
            case eHeading::WEST:  signo_ns = 0;  signo_eo = -1; break;
        }

        if (g_callbacks.GetConnGroupUserEntries && g_callbacks.GetUser) {
            const auto& user_entries = g_callbacks.GetConnGroupUserEntries(npc_ref.Pos.Map);
            for (std::int16_t user_index : user_entries) {
                if (user_index <= 0) continue;
                const auto& user = g_callbacks.GetUser(user_index);

                int dx = user.Pos.X - npc_ref.Pos.X;
                int dy = user.Pos.Y - npc_ref.Pos.Y;
                int sgn_x = (dx > 0) - (dx < 0);
                int sgn_y = (dy > 0) - (dy < 0);

                if (std::abs(dx) <= RANGO_VISION_X && sgn_x == signo_eo &&
                    std::abs(dy) <= RANGO_VISION_Y && sgn_y == signo_ns) {

                    bool es_crim = g_callbacks.EsCriminal && g_callbacks.EsCriminal(user_index);
                    if (es_crim) {
                        if (user.flags.Muerto == 0 && user.flags.invisible == 0 &&
                            user.flags.Oculto == 0 && user.flags.AdminPerseguible &&
                            !IsUserProtected(user_index)) {

                            if (npc_ref.flags.LanzaSpells > 0) {
                                NpcLanzaUnSpell(npc_index, user_index);
                            }
                            return;
                        }
                    }
                }
            }
        }
    } else {
        if (g_callbacks.GetConnGroupUserEntries && g_callbacks.GetUser) {
            const auto& user_entries = g_callbacks.GetConnGroupUserEntries(npc_ref.Pos.Map);
            for (std::int16_t user_index : user_entries) {
                if (user_index <= 0) continue;
                const auto& user = g_callbacks.GetUser(user_index);

                if (std::abs(user.Pos.X - npc_ref.Pos.X) <= RANGO_VISION_X &&
                    std::abs(user.Pos.Y - npc_ref.Pos.Y) <= RANGO_VISION_Y) {

                    bool es_crim = g_callbacks.EsCriminal && g_callbacks.EsCriminal(user_index);
                    if (es_crim) {
                        if (user.flags.Muerto == 0 && user.flags.invisible == 0 &&
                            user.flags.Oculto == 0 && user.flags.AdminPerseguible &&
                            !IsUserProtected(user_index)) {

                            if (npc_ref.flags.LanzaSpells > 0) {
                                NpcLanzaUnSpell(npc_index, user_index);
                            }
                            if (npc_ref.flags.Inmovilizado == 1) return;

                            if (g_callbacks.FindDirection && g_callbacks.MoveNPCChar) {
                                std::uint8_t t_heading = g_callbacks.FindDirection(npc_ref.Pos, user.Pos);
                                g_callbacks.MoveNPCChar(npc_index, t_heading);
                            }
                            return;
                        }
                    }
                }
            }
        }
    }

    RestoreOldMovement(npc_index);
}

void SeguirAmo(std::int16_t npc_index) {
    auto& npc_ref = Npclist[npc_index];

    if (npc_ref.Target == 0 && npc_ref.TargetNPC == 0) {
        std::int16_t ui = npc_ref.MaestroUser;
        if (ui > 0 && g_callbacks.GetUser) {
            const auto& user = g_callbacks.GetUser(ui);

            if (std::abs(user.Pos.X - npc_ref.Pos.X) <= RANGO_VISION_X &&
                std::abs(user.Pos.Y - npc_ref.Pos.Y) <= RANGO_VISION_Y) {

                double dist = g_callbacks.Distancia ? g_callbacks.Distancia(npc_ref.Pos, user.Pos) : Distance(npc_ref.Pos.X, npc_ref.Pos.Y, user.Pos.X, user.Pos.Y);

                if (user.flags.Muerto == 0 && user.flags.invisible == 0 &&
                    user.flags.Oculto == 0 && dist > 3.0) {

                    if (g_callbacks.FindDirection && g_callbacks.MoveNPCChar) {
                        std::uint8_t t_heading = g_callbacks.FindDirection(npc_ref.Pos, user.Pos);
                        g_callbacks.MoveNPCChar(npc_index, t_heading);
                    }
                    return;
                }
            }
        }
    }

    RestoreOldMovement(npc_index);
}

void AiNpcAtacaNpc(std::int16_t npc_index) {
    auto& npc_ref = Npclist[npc_index];
    bool b_no_esta = false;

    if (npc_ref.flags.Inmovilizado == 1) {
        int signo_ns = 0;
        int signo_eo = 0;
        switch (npc_ref.char_appearance.heading) {
            case eHeading::NORTH: signo_ns = -1; signo_eo = 0; break;
            case eHeading::EAST:  signo_ns = 0;  signo_eo = 1; break;
            case eHeading::SOUTH: signo_ns = 1;  signo_eo = 0; break;
            case eHeading::WEST:  signo_ns = 0;  signo_eo = -1; break;
        }

        std::int16_t end_y = npc_ref.Pos.Y + signo_ns * RANGO_VISION_Y;
        std::int16_t end_x = npc_ref.Pos.X + signo_eo * RANGO_VISION_X;
        std::int16_t step_y = (signo_ns == 0) ? 1 : signo_ns;
        std::int16_t step_x = (signo_eo == 0) ? 1 : signo_eo;

        for (std::int16_t y = npc_ref.Pos.Y; (step_y > 0 ? y <= end_y : y >= end_y); y += step_y) {
            for (std::int16_t x = npc_ref.Pos.X; (step_x > 0 ? x <= end_x : x >= end_x); x += step_x) {
                if (HelperInMapBounds(npc_ref.Pos.Map, x, y)) {
                    std::int16_t ni = HelperGetMapNpcIndex(npc_ref.Pos.Map, x, y);
                    if (ni > 0 && npc_ref.TargetNPC == ni) {
                        b_no_esta = true;
                        if (npc_ref.Numero == ELEMENTALFUEGO) {
                            NpcLanzaUnSpellSobreNpc(npc_index, ni);
                            if (Npclist[ni].NPCtype == eNPCType::DRAGON) {
                                Npclist[ni].CanAttack = 1;
                                NpcLanzaUnSpellSobreNpc(ni, npc_index);
                            }
                        } else {
                            double dist = g_callbacks.Distancia ? g_callbacks.Distancia(npc_ref.Pos, Npclist[ni].Pos) : Distance(npc_ref.Pos.X, npc_ref.Pos.Y, Npclist[ni].Pos.X, Npclist[ni].Pos.Y);
                            if (dist <= 1.0) {
                                if (g_callbacks.NpcAtacaNpc) {
                                    g_callbacks.NpcAtacaNpc(npc_index, ni, false);
                                }
                            }
                        }
                        return;
                    }
                }
            }
        }
    } else {
        for (std::int16_t y = npc_ref.Pos.Y - RANGO_VISION_Y; y <= npc_ref.Pos.Y + RANGO_VISION_Y; ++y) {
            for (std::int16_t x = npc_ref.Pos.X - RANGO_VISION_Y; x <= npc_ref.Pos.X + RANGO_VISION_Y; ++x) {
                if (HelperInMapBounds(npc_ref.Pos.Map, x, y)) {
                    std::int16_t ni = HelperGetMapNpcIndex(npc_ref.Pos.Map, x, y);
                    if (ni > 0 && npc_ref.TargetNPC == ni) {
                        b_no_esta = true;
                        if (npc_ref.Numero == ELEMENTALFUEGO) {
                            NpcLanzaUnSpellSobreNpc(npc_index, ni);
                            if (Npclist[ni].NPCtype == eNPCType::DRAGON) {
                                Npclist[ni].CanAttack = 1;
                                NpcLanzaUnSpellSobreNpc(ni, npc_index);
                            }
                        } else {
                            double dist = g_callbacks.Distancia ? g_callbacks.Distancia(npc_ref.Pos, Npclist[ni].Pos) : Distance(npc_ref.Pos.X, npc_ref.Pos.Y, Npclist[ni].Pos.X, Npclist[ni].Pos.Y);
                            if (dist <= 1.0) {
                                if (g_callbacks.NpcAtacaNpc) {
                                    g_callbacks.NpcAtacaNpc(npc_index, ni, false);
                                }
                            }
                        }

                        if (npc_ref.flags.Inmovilizado == 1 || npc_ref.TargetNPC == 0) return;

                        if (g_callbacks.FindDirection && g_callbacks.MoveNPCChar) {
                            std::uint8_t t_heading = g_callbacks.FindDirection(npc_ref.Pos, Npclist[ni].Pos);
                            g_callbacks.MoveNPCChar(npc_index, t_heading);
                        }
                        return;
                    }
                }
            }
        }
    }

    if (!b_no_esta) {
        if (npc_ref.MaestroUser > 0) {
            if (g_callbacks.FollowAmo) {
                g_callbacks.FollowAmo(npc_index);
            }
        } else {
            npc_ref.Movement = npc_ref.flags.OldMovement;
            npc_ref.Hostile = npc_ref.flags.OldHostil;
        }
    }
}

void NPCAI(std::int16_t npc_index) {
    try {
        auto& npc_ref = Npclist[npc_index];

        // <<<<<<<<<<< Ataques >>>>>>>>>>>>>>>>
        if (npc_ref.MaestroUser == 0) {
            if (npc_ref.NPCtype == eNPCType::GuardiaReal) {
                GuardiasAI(npc_index, false);
            } else if (npc_ref.NPCtype == eNPCType::Guardiascaos) {
                GuardiasAI(npc_index, true);
            } else if (npc_ref.Hostile && npc_ref.Stats.Alineacion != 0) {
                HostilMalvadoAI(npc_index);
            } else if (npc_ref.Hostile && npc_ref.Stats.Alineacion == 0) {
                HostilBuenoAI(npc_index);
            }
        }

        // <<<<<<<<<<< Movimiento >>>>>>>>>>>>>>>>
        switch (static_cast<TipoAI>(npc_ref.Movement)) {
            case TipoAI::MueveAlAzar: {
                if (npc_ref.flags.Inmovilizado == 1) return;

                if (npc_ref.NPCtype == eNPCType::GuardiaReal) {
                    if (g_callbacks.RandomNumber && g_callbacks.RandomNumber(1, 12) == 3) {
                        if (g_callbacks.MoveNPCChar) {
                            std::uint8_t heading = static_cast<std::uint8_t>(g_callbacks.RandomNumber(static_cast<std::int32_t>(eHeading::NORTH), static_cast<std::int32_t>(eHeading::WEST)));
                            g_callbacks.MoveNPCChar(npc_index, heading);
                        }
                    }
                    PersigueCriminal(npc_index);
                } else if (npc_ref.NPCtype == eNPCType::Guardiascaos) {
                    if (g_callbacks.RandomNumber && g_callbacks.RandomNumber(1, 12) == 3) {
                        if (g_callbacks.MoveNPCChar) {
                            std::uint8_t heading = static_cast<std::uint8_t>(g_callbacks.RandomNumber(static_cast<std::int32_t>(eHeading::NORTH), static_cast<std::int32_t>(eHeading::WEST)));
                            g_callbacks.MoveNPCChar(npc_index, heading);
                        }
                    }
                    PersigueCiudadano(npc_index);
                } else {
                    if (g_callbacks.RandomNumber && g_callbacks.RandomNumber(1, 12) == 3) {
                        if (g_callbacks.MoveNPCChar) {
                            std::uint8_t heading = static_cast<std::uint8_t>(g_callbacks.RandomNumber(static_cast<std::int32_t>(eHeading::NORTH), static_cast<std::int32_t>(eHeading::WEST)));
                            g_callbacks.MoveNPCChar(npc_index, heading);
                        }
                    }
                }
                break;
            }

            case TipoAI::NpcMaloAtacaUsersBuenos:
                IrUsuarioCercano(npc_index);
                break;

            case TipoAI::NPCDEFENSA:
                SeguirAgresor(npc_index);
                break;

            case TipoAI::GuardiasAtacanCriminales:
                PersigueCriminal(npc_index);
                break;

            case TipoAI::SigueAmo:
                if (npc_ref.flags.Inmovilizado == 1) return;
                SeguirAmo(npc_index);
                if (g_callbacks.RandomNumber && g_callbacks.RandomNumber(1, 12) == 3) {
                    if (g_callbacks.MoveNPCChar) {
                        std::uint8_t heading = static_cast<std::uint8_t>(g_callbacks.RandomNumber(static_cast<std::int32_t>(eHeading::NORTH), static_cast<std::int32_t>(eHeading::WEST)));
                        g_callbacks.MoveNPCChar(npc_index, heading);
                    }
                }
                break;

            case TipoAI::NpcAtacaNpc:
                AiNpcAtacaNpc(npc_index);
                break;

            case TipoAI::NpcObjeto:
                AiNpcObjeto(npc_index);
                break;

            case TipoAI::NpcPathfinding:
                if (npc_ref.flags.Inmovilizado == 1) return;
                if (ReCalculatePath(npc_index)) {
                    PathFindingAI(npc_index);
                    if (npc_ref.PFINFO.NoPath) {
                        if (g_callbacks.RandomNumber && g_callbacks.MoveNPCChar) {
                            std::uint8_t heading = static_cast<std::uint8_t>(g_callbacks.RandomNumber(static_cast<std::int32_t>(eHeading::NORTH), static_cast<std::int32_t>(eHeading::WEST)));
                            g_callbacks.MoveNPCChar(npc_index, heading);
                        }
                    }
                } else {
                    if (!PathEnd(npc_index)) {
                        FollowPath(npc_index);
                    } else {
                        npc_ref.PFINFO.PathLenght = 0;
                    }
                }
                break;

            default:
                break;
        }
    } catch (...) {
        const auto& npc_ref = Npclist[npc_index];
        std::string err = "NPCAI " + std::string(npc_ref.name.data()) + " " +
                          std::to_string(npc_ref.MaestroUser) + " " +
                          std::to_string(npc_ref.MaestroNpc) + " mapa:" +
                          std::to_string(npc_ref.Pos.Map) + " x:" +
                          std::to_string(npc_ref.Pos.X) + " y:" +
                          std::to_string(npc_ref.Pos.Y) + " Mov:" +
                          std::to_string(static_cast<int>(npc_ref.Movement)) + " TargU:" +
                          std::to_string(npc_ref.Target) + " TargN:" +
                          std::to_string(npc_ref.TargetNPC);

        if (g_callbacks.LogError) {
            g_callbacks.LogError(err);
        }

        npc copy_npc = npc_ref;

        if (g_callbacks.QuitarNPC) {
            g_callbacks.QuitarNPC(npc_index);
        }

        if (g_callbacks.ReSpawnNpc) {
            g_callbacks.ReSpawnNpc(copy_npc);
        }
    }
}

} // namespace AI_NPC
