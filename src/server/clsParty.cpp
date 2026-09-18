#include "clsParty.hpp"
#include "mdParty.hpp"
#include "Declares.hpp"
#include "FileIO.hpp"
#include "Matematicas.hpp"
#include "Protocol.hpp"
#include <cmath>
#include <algorithm>

using ao::net::protocol::FontTypeNames;

clsParty::clsParty() {
    Class_Initialize();
}

void clsParty::Class_Initialize() {
    p_expTotal = 0;
    p_CantMiembros = 0;
    p_SumaNivelesElevados = 0.0f;
    p_Fundador = 0;
    p_members.fill(ModParty::tPartyMember{});
}

void clsParty::Class_Terminate() {
    // Destructor helper
}

void clsParty::UpdateSumaNivelesElevados(std::int16_t lvl) {
    p_SumaNivelesElevados = p_SumaNivelesElevados 
        - std::pow(static_cast<float>(lvl - 1), ExponenteNivelParty) 
        + std::pow(static_cast<float>(lvl), ExponenteNivelParty);
}

std::int32_t clsParty::MiExperiencia(std::int16_t user_index) const {
    for (std::size_t i = 0; i < ModParty::PARTY_MAXMEMBERS; ++i) {
        if (p_members[i].UserIndex == user_index) {
            return static_cast<std::int32_t>(std::trunc(p_members[i].Experiencia));
        }
    }
    return -1;
}

void clsParty::ObtenerExito(std::int32_t exp_ganada, std::int16_t mapa, std::int16_t x, std::int16_t y) {
    p_expTotal += exp_ganada;

    const auto& cb = ModParty::GetPartyCallbacks();

    for (std::size_t i = 0; i < ModParty::PARTY_MAXMEMBERS; ++i) {
        const auto UI = p_members[i].UserIndex;
        if (UI > 0 && UI <= MaxUsers) {
            double expThisUser = static_cast<double>(exp_ganada) 
                * (std::pow(static_cast<float>(UserList[UI].Stats.ELV), ExponenteNivelParty) / p_SumaNivelesElevados);

            if (mapa == UserList[UI].Pos.Map && UserList[UI].flags.Muerto == 0) {
                if (Distance(UserList[UI].Pos.X, UserList[UI].Pos.Y, x, y) <= ModParty::PARTY_MAXDISTANCIA) {
                    if (ModParty::PARTY_EXPERIENCIAPORGOLPE) {
                        auto add_exp = static_cast<std::int32_t>(std::trunc(expThisUser));
                        UserList[UI].Stats.Exp += add_exp;
                        if (UserList[UI].Stats.Exp > MAXEXP) {
                            UserList[UI].Stats.Exp = MAXEXP;
                        }
                        if (cb.CheckUserLevel) cb.CheckUserLevel(UI);
                        if (cb.WriteUpdateUserStats) cb.WriteUpdateUserStats(UI);
                    } else {
                        p_members[i].Experiencia += expThisUser;
                        if (p_members[i].Experiencia < 0.0) {
                            p_members[i].Experiencia = 0.0;
                        }
                    }
                }
            }
        }
    }
}

void clsParty::MandarMensajeAConsola(std::string_view texto, std::string_view sender) {
    const auto& cb = ModParty::GetPartyCallbacks();
    std::string msg = " [" + std::string(sender) + "] " + std::string(texto);

    for (std::size_t i = 0; i < ModParty::PARTY_MAXMEMBERS; ++i) {
        if (p_members[i].UserIndex > 0) {
            if (cb.WriteConsoleMsg) {
                cb.WriteConsoleMsg(p_members[i].UserIndex, msg, static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
            } else {
                ao::net::protocol::WriteConsoleMsg(p_members[i].UserIndex, msg, FontTypeNames::FONTTYPE_PARTY);
            }
        }
    }
}

bool clsParty::EsPartyLeader(std::int16_t user_index) const {
    return (user_index == p_Fundador);
}

bool clsParty::NuevoMiembro(std::int16_t user_index) {
    for (std::size_t i = 0; i < ModParty::PARTY_MAXMEMBERS; ++i) {
        if (p_members[i].UserIndex == 0) {
            p_members[i].Experiencia = 0.0;
            p_members[i].UserIndex = user_index;
            p_CantMiembros++;
            p_SumaNivelesElevados += std::pow(static_cast<float>(UserList[user_index].Stats.ELV), ExponenteNivelParty);
            return true;
        }
    }
    return false;
}

void clsParty::CompactMemberList() {
    int freeIndex = -1;
    for (int i = 0; i < static_cast<int>(ModParty::PARTY_MAXMEMBERS); ++i) {
        if (p_members[i].UserIndex == 0 && freeIndex == -1) {
            freeIndex = i;
        } else if (p_members[i].UserIndex > 0 && freeIndex != -1) {
            p_members[freeIndex] = p_members[i];
            p_members[i] = ModParty::tPartyMember{};
            i = freeIndex;
            freeIndex = -1;
        }
    }
}

bool clsParty::SaleMiembro(std::int16_t user_index) {
    std::size_t found_idx = ModParty::PARTY_MAXMEMBERS;
    for (std::size_t i = 0; i < ModParty::PARTY_MAXMEMBERS; ++i) {
        if (p_members[i].UserIndex == user_index) {
            found_idx = i;
            break;
        }
    }

    const auto& cb = ModParty::GetPartyCallbacks();

    if (found_idx == 0) {
        // Leader leaves, party dissolves
        MandarMensajeAConsola("El líder disuelve la party.", "Servidor");

        for (int j = static_cast<int>(ModParty::PARTY_MAXMEMBERS) - 1; j >= 0; --j) {
            if (p_members[j].UserIndex > 0) {
                const auto UI = p_members[j].UserIndex;
                if (j != 0) {
                    std::string msg = "Abandonas la party liderada por " + std::string(UserList[p_members[0].UserIndex].name) + ".";
                    if (cb.WriteConsoleMsg) {
                        cb.WriteConsoleMsg(UI, msg, static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
                    } else {
                        ao::net::protocol::WriteConsoleMsg(UI, msg, FontTypeNames::FONTTYPE_PARTY);
                    }
                }

                std::string msg_exp = "Durante la misma has conseguido " + std::to_string(static_cast<std::int32_t>(std::trunc(p_members[j].Experiencia))) + " puntos de experiencia.";
                if (cb.WriteConsoleMsg) {
                    cb.WriteConsoleMsg(UI, msg_exp, static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
                } else {
                    ao::net::protocol::WriteConsoleMsg(UI, msg_exp, FontTypeNames::FONTTYPE_PARTY);
                }

                if (!ModParty::PARTY_EXPERIENCIAPORGOLPE) {
                    auto add_exp = static_cast<std::int32_t>(std::trunc(p_members[j].Experiencia));
                    UserList[UI].Stats.Exp += add_exp;
                    if (UserList[UI].Stats.Exp > MAXEXP) {
                        UserList[UI].Stats.Exp = MAXEXP;
                    }
                    if (cb.CheckUserLevel) cb.CheckUserLevel(UI);
                    if (cb.WriteUpdateUserStats) cb.WriteUpdateUserStats(UI);
                }

                MandarMensajeAConsola(std::string(UserList[UI].name) + " abandona la party.", "Servidor");

                UserList[UI].PartyIndex = 0;
                p_CantMiembros--;

                // KNOWN LEGACY BUG #46: Replicating subtraction of leader's level (user_index) instead of p_members[j].UserIndex
                p_SumaNivelesElevados -= std::pow(static_cast<float>(UserList[user_index].Stats.ELV), ExponenteNivelParty);
                p_members[j] = ModParty::tPartyMember{};
            }
        }
        return true;
    } else if (found_idx < ModParty::PARTY_MAXMEMBERS) {
        const auto UI = p_members[found_idx].UserIndex;
        if (!ModParty::PARTY_EXPERIENCIAPORGOLPE) {
            auto add_exp = static_cast<std::int32_t>(std::trunc(p_members[found_idx].Experiencia));
            UserList[UI].Stats.Exp += add_exp;
            if (UserList[UI].Stats.Exp > MAXEXP) {
                UserList[UI].Stats.Exp = MAXEXP;
            }
            if (cb.CheckUserLevel) cb.CheckUserLevel(UI);
            if (cb.WriteUpdateUserStats) cb.WriteUpdateUserStats(UI);
        }

        MandarMensajeAConsola(std::string(UserList[UI].name) + " abandona la party.", "Servidor");
        std::string msg_exp = "Durante la misma has conseguido " + std::to_string(static_cast<std::int32_t>(std::trunc(p_members[found_idx].Experiencia))) + " puntos de experiencia.";
        if (cb.WriteConsoleMsg) {
            cb.WriteConsoleMsg(UI, msg_exp, static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
        } else {
            ao::net::protocol::WriteConsoleMsg(UI, msg_exp, FontTypeNames::FONTTYPE_PARTY);
        }

        p_CantMiembros--;
        p_SumaNivelesElevados -= std::pow(static_cast<float>(UserList[user_index].Stats.ELV), ExponenteNivelParty);
        UserList[UI].PartyIndex = 0;
        p_members[found_idx] = ModParty::tPartyMember{};
        CompactMemberList();
        return false;
    }

    return false;
}

bool clsParty::HacerLeader(std::int16_t user_index) {
    std::size_t user_idx_idx = ModParty::PARTY_MAXMEMBERS;
    for (std::size_t i = 0; i < ModParty::PARTY_MAXMEMBERS; ++i) {
        if (p_members[i].UserIndex == user_index) {
            user_idx_idx = i;
            break;
        }
    }

    if (user_idx_idx == ModParty::PARTY_MAXMEMBERS) {
        return false;
    }

    auto oldLeader = p_members[0].UserIndex;
    auto oldExp = p_members[0].Experiencia;

    p_members[0].UserIndex = p_members[user_idx_idx].UserIndex;
    p_members[0].Experiencia = p_members[user_idx_idx].Experiencia;

    p_members[user_idx_idx].UserIndex = oldLeader;
    p_members[user_idx_idx].Experiencia = oldExp;

    p_Fundador = p_members[0].UserIndex;
    return true;
}

void clsParty::ObtenerMiembrosOnline(std::array<std::int16_t, ModParty::PARTY_MAXMEMBERS>& member_list) const {
    member_list.fill(0);
    for (std::size_t i = 0; i < ModParty::PARTY_MAXMEMBERS; ++i) {
        if (p_members[i].UserIndex > 0) {
            member_list[i] = p_members[i].UserIndex;
        }
    }
}

std::int32_t clsParty::ObtenerExperienciaTotal() const {
    return p_expTotal;
}

bool clsParty::PuedeEntrar(std::int16_t user_index, std::string& razon) const {
    if (user_index <= 0 || user_index > MaxUsers) return false;

    if (p_CantMiembros >= ModParty::PARTY_MAXMEMBERS || p_members[ModParty::PARTY_MAXMEMBERS - 1].UserIndex > 0) {
        razon = "La party está llena.";
        return false;
    }

    if (p_members[0].UserIndex <= 0) return false;

    bool dist_ok = (Distance(UserList[p_members[0].UserIndex].Pos.X, UserList[p_members[0].UserIndex].Pos.Y, UserList[user_index].Pos.X, UserList[user_index].Pos.Y) <= ModParty::MAXDISTANCIAINGRESOPARTY);
    if (!dist_ok) {
        razon = "El usuario " + std::string(UserList[user_index].name) + " se encuentra muy lejos.";
        return false;
    }

    bool esArmada = (UserList[user_index].Faccion.ArmadaReal == 1);
    bool esCaos = (UserList[user_index].Faccion.FuerzasCaos == 1);

    for (std::size_t i = 0; i < ModParty::PARTY_MAXMEMBERS; ++i) {
        const auto UI = p_members[i].UserIndex;
        if (UI > 0) {
            if (esArmada && FileIO::criminal(UI)) {
                razon = "Los miembros del ejército real no entran a una party con criminales.";
                return false;
            }
            if (esCaos && !FileIO::criminal(UI)) {
                razon = "Los miembros de la legión oscura no entran a una party con ciudadanos.";
                return false;
            }
            if (UserList[UI].Faccion.ArmadaReal == 1 && FileIO::criminal(user_index)) {
                razon = "Los criminales no entran a parties con miembros del ejército real.";
                return false;
            }
            if (UserList[UI].Faccion.FuerzasCaos == 1 && !FileIO::criminal(user_index)) {
                razon = "Los ciudadanos no entran a parties con miembros de la legión oscura.";
                return false;
            }
        }
    }

    return true;
}

void clsParty::FlushExperiencia() {
    if (ModParty::PARTY_EXPERIENCIAPORGOLPE) return;

    const auto& cb = ModParty::GetPartyCallbacks();

    for (std::size_t i = 0; i < ModParty::PARTY_MAXMEMBERS; ++i) {
        const auto UI = p_members[i].UserIndex;
        if (UI > 0) {
            if (p_members[i].Experiencia > 0.0) {
                auto add_exp = static_cast<std::int32_t>(std::trunc(p_members[i].Experiencia));
                UserList[UI].Stats.Exp += add_exp;
                if (UserList[UI].Stats.Exp > MAXEXP) {
                    UserList[UI].Stats.Exp = MAXEXP;
                }
                if (cb.CheckUserLevel) cb.CheckUserLevel(UI);
                p_members[i].Experiencia -= std::trunc(p_members[i].Experiencia);
            } else {
                if (std::abs(static_cast<double>(UserList[UI].Stats.Exp)) > std::abs(std::trunc(p_members[i].Experiencia))) {
                    UserList[UI].Stats.Exp += static_cast<std::int32_t>(std::trunc(p_members[i].Experiencia));
                } else {
                    UserList[UI].Stats.Exp = 0;
                }
                p_members[i].Experiencia = 0.0;
            }
            if (cb.WriteUpdateUserStats) cb.WriteUpdateUserStats(UI);
        }
    }
}

std::int16_t clsParty::CantMiembros() const {
    return p_CantMiembros;
}
