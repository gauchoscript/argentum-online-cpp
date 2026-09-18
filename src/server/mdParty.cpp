#include "mdParty.hpp"
#include "Declares.hpp"
#include "Protocol.hpp"
#include <iostream>

using ao::net::protocol::FontTypeNames;

namespace {
ModParty::PartyCallbacks g_party_callbacks{};
}

namespace ModParty {

void SetPartyCallbacks(const PartyCallbacks& callbacks) {
    g_party_callbacks = callbacks;
}

const PartyCallbacks& GetPartyCallbacks() {
    return g_party_callbacks;
}

std::int16_t NextParty() {
    for (std::int16_t i = 1; i <= MAX_PARTIES; ++i) {
        if (!Parties[i]) {
            return i;
        }
    }
    return -1;
}

bool PuedeCrearParty(std::int16_t user_index) {
    if (user_index <= 0 || user_index > MaxUsers) return false;

    const auto& cb = GetPartyCallbacks();

    auto carisma = static_cast<std::int32_t>(UserList[user_index].Stats.UserAtributos[eAtributos::Carisma]);
    auto liderazgo = static_cast<std::int32_t>(UserList[user_index].Stats.UserSkills[eSkill::Liderazgo]);

    if (carisma * liderazgo < 100) {
        if (cb.WriteConsoleMsg) {
            cb.WriteConsoleMsg(user_index, "Tu carisma y liderazgo no son suficientes para liderar una party.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Tu carisma y liderazgo no son suficientes para liderar una party.", FontTypeNames::FONTTYPE_PARTY);
        }
        return false;
    } else if (UserList[user_index].flags.Muerto == 1) {
        if (cb.WriteConsoleMsg) {
            cb.WriteConsoleMsg(user_index, "¡¡Estás muerto!!", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "¡¡Estás muerto!!", FontTypeNames::FONTTYPE_PARTY);
        }
        return false;
    }

    return true;
}

void CrearParty(std::int16_t user_index) {
    if (user_index <= 0 || user_index > MaxUsers) return;

    const auto& cb = GetPartyCallbacks();
    auto& user = UserList[user_index];

    if (user.PartyIndex == 0) {
        if (user.flags.Muerto == 0) {
            if (user.Stats.UserSkills[eSkill::Liderazgo] >= 5) {
                std::int16_t tInt = NextParty();
                if (tInt == -1) {
                    if (cb.WriteConsoleMsg) {
                        cb.WriteConsoleMsg(user_index, "Por el momento no se pueden crear más parties.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
                    } else {
                        ao::net::protocol::WriteConsoleMsg(user_index, "Por el momento no se pueden crear más parties.", FontTypeNames::FONTTYPE_PARTY);
                    }
                    return;
                } else {
                    Parties[tInt] = std::make_shared<clsParty>();
                    if (!Parties[tInt]->NuevoMiembro(user_index)) {
                        if (cb.WriteConsoleMsg) {
                            cb.WriteConsoleMsg(user_index, "La party está llena, no puedes entrar.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
                        } else {
                            ao::net::protocol::WriteConsoleMsg(user_index, "La party está llena, no puedes entrar.", FontTypeNames::FONTTYPE_PARTY);
                        }
                        Parties[tInt] = nullptr;
                        return;
                    } else {
                        if (cb.WriteConsoleMsg) {
                            cb.WriteConsoleMsg(user_index, "¡Has formado una party!", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
                        } else {
                            ao::net::protocol::WriteConsoleMsg(user_index, "¡Has formado una party!", FontTypeNames::FONTTYPE_PARTY);
                        }
                        user.PartyIndex = tInt;
                        user.PartySolicitud = 0;
                        if (!Parties[tInt]->HacerLeader(user_index)) {
                            if (cb.WriteConsoleMsg) {
                                cb.WriteConsoleMsg(user_index, "No puedes hacerte líder.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
                            } else {
                                ao::net::protocol::WriteConsoleMsg(user_index, "No puedes hacerte líder.", FontTypeNames::FONTTYPE_PARTY);
                            }
                        } else {
                            if (cb.WriteConsoleMsg) {
                                cb.WriteConsoleMsg(user_index, "¡Te has convertido en líder de la party!", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
                            } else {
                                ao::net::protocol::WriteConsoleMsg(user_index, "¡Te has convertido en líder de la party!", FontTypeNames::FONTTYPE_PARTY);
                            }
                        }
                    }
                }
            } else {
                if (cb.WriteConsoleMsg) {
                    cb.WriteConsoleMsg(user_index, "No tienes suficientes puntos de liderazgo para liderar una party.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
                } else {
                    ao::net::protocol::WriteConsoleMsg(user_index, "No tienes suficientes puntos de liderazgo para liderar una party.", FontTypeNames::FONTTYPE_PARTY);
                }
            }
        } else {
            if (cb.WriteConsoleMsg) {
                cb.WriteConsoleMsg(user_index, "¡¡Estás muerto!!", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡¡Estás muerto!!", FontTypeNames::FONTTYPE_PARTY);
            }
        }
    } else {
        if (cb.WriteConsoleMsg) {
            cb.WriteConsoleMsg(user_index, "Ya perteneces a una party.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Ya perteneces a una party.", FontTypeNames::FONTTYPE_PARTY);
        }
    }
}

void SolicitarIngresoAParty(std::int16_t user_index) {
    if (user_index <= 0 || user_index > MaxUsers) return;

    const auto& cb = GetPartyCallbacks();
    auto& user = UserList[user_index];

    if (user.PartyIndex > 0) {
        if (cb.WriteConsoleMsg) {
            cb.WriteConsoleMsg(user_index, "Ya perteneces a una party, escribe /SALIRPARTY para abandonarla", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Ya perteneces a una party, escribe /SALIRPARTY para abandonarla", FontTypeNames::FONTTYPE_PARTY);
        }
        user.PartySolicitud = 0;
        return;
    }

    if (user.flags.Muerto == 1) {
        if (cb.WriteConsoleMsg) {
            cb.WriteConsoleMsg(user_index, "¡¡Estás muerto!!", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_INFO));
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "¡¡Estás muerto!!", FontTypeNames::FONTTYPE_INFO);
        }
        user.PartySolicitud = 0;
        return;
    }

    auto tInt = user.flags.TargetUser;
    if (tInt > 0 && tInt <= MaxUsers) {
        if (UserList[tInt].PartyIndex > 0) {
            user.PartySolicitud = UserList[tInt].PartyIndex;
            if (cb.WriteConsoleMsg) {
                cb.WriteConsoleMsg(user_index, "El fundador decidirá si te acepta en la party.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, "El fundador decidirá si te acepta en la party.", FontTypeNames::FONTTYPE_PARTY);
            }
        } else {
            if (cb.WriteConsoleMsg) {
                cb.WriteConsoleMsg(user_index, std::string(UserList[tInt].name) + " no es fundador de ninguna party.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_INFO));
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, std::string(UserList[tInt].name) + " no es fundador de ninguna party.", FontTypeNames::FONTTYPE_INFO);
            }
            user.PartySolicitud = 0;
        }
    } else {
        if (cb.WriteConsoleMsg) {
            cb.WriteConsoleMsg(user_index, "Para ingresar a una party debes hacer click sobre el fundador y luego escribir /PARTY", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Para ingresar a una party debes hacer click sobre el fundador y luego escribir /PARTY", FontTypeNames::FONTTYPE_PARTY);
        }
        user.PartySolicitud = 0;
    }
}

void SalirDeParty(std::int16_t user_index) {
    if (user_index <= 0 || user_index > MaxUsers) return;

    const auto& cb = GetPartyCallbacks();
    auto PI = UserList[user_index].PartyIndex;

    if (PI > 0 && PI <= MAX_PARTIES && Parties[PI]) {
        if (Parties[PI]->SaleMiembro(user_index)) {
            Parties[PI].reset();
        }
    } else {
        if (cb.WriteConsoleMsg) {
            cb.WriteConsoleMsg(user_index, "No eres miembro de ninguna party.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_INFO));
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "No eres miembro de ninguna party.", FontTypeNames::FONTTYPE_INFO);
        }
    }
}

void ExpulsarDeParty(std::int16_t leader, std::int16_t old_member) {
    if (leader <= 0 || leader > MaxUsers || old_member <= 0 || old_member > MaxUsers) return;

    if (!UserPuedeEjecutarComandos(leader)) return;

    const auto& cb = GetPartyCallbacks();
    auto PI = UserList[leader].PartyIndex;

    if (PI > 0 && PI <= MAX_PARTIES && PI == UserList[old_member].PartyIndex && Parties[PI]) {
        if (Parties[PI]->SaleMiembro(old_member)) {
            Parties[PI].reset();
        }
    } else {
        std::string msg = std::string(UserList[old_member].name) + " no pertenece a tu party.";
        if (cb.WriteConsoleMsg) {
            cb.WriteConsoleMsg(leader, msg, static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_INFO));
        } else {
            ao::net::protocol::WriteConsoleMsg(leader, msg, FontTypeNames::FONTTYPE_INFO);
        }
    }
}

bool UserPuedeEjecutarComandos(std::int16_t user_index) {
    if (user_index <= 0 || user_index > MaxUsers) return false;

    const auto& cb = GetPartyCallbacks();
    auto PI = UserList[user_index].PartyIndex;

    if (PI > 0 && PI <= MAX_PARTIES && Parties[PI]) {
        if (Parties[PI]->EsPartyLeader(user_index)) {
            return true;
        } else {
            if (cb.WriteConsoleMsg) {
                cb.WriteConsoleMsg(user_index, "¡No eres el líder de tu party!", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡No eres el líder de tu party!", FontTypeNames::FONTTYPE_PARTY);
            }
            return false;
        }
    } else {
        if (cb.WriteConsoleMsg) {
            cb.WriteConsoleMsg(user_index, "No eres miembro de ninguna party.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_INFO));
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "No eres miembro de ninguna party.", FontTypeNames::FONTTYPE_INFO);
        }
        return false;
    }
}

void AprobarIngresoAParty(std::int16_t leader, std::int16_t new_member) {
    if (leader <= 0 || leader > MaxUsers || new_member <= 0 || new_member > MaxUsers) return;

    if (!UserPuedeEjecutarComandos(leader)) return;

    const auto& cb = GetPartyCallbacks();
    auto PI = UserList[leader].PartyIndex;
    std::string razon;

    if (PI <= 0 || PI > MAX_PARTIES || !Parties[PI]) return;

    auto& target_user = UserList[new_member];

    if (target_user.PartySolicitud == PI) {
        if (target_user.flags.Muerto != 1) {
            if (target_user.PartyIndex == 0) {
                if (Parties[PI]->PuedeEntrar(new_member, razon)) {
                    if (Parties[PI]->NuevoMiembro(new_member)) {
                        Parties[PI]->MandarMensajeAConsola(std::string(UserList[leader].name) + " ha aceptado a " + std::string(target_user.name) + " en la party.", "Servidor");
                        target_user.PartyIndex = PI;
                        target_user.PartySolicitud = 0;
                        if (cb.WriteUpdateUserStats) cb.WriteUpdateUserStats(new_member);
                    }
                } else {
                    if (cb.WriteConsoleMsg) {
                        cb.WriteConsoleMsg(leader, razon, static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
                    } else {
                        ao::net::protocol::WriteConsoleMsg(leader, razon, FontTypeNames::FONTTYPE_PARTY);
                    }
                }
            } else {
                if (target_user.PartyIndex == PI) {
                    if (cb.WriteConsoleMsg) {
                        cb.WriteConsoleMsg(leader, std::string(target_user.name) + " ya es miembro de la party.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
                    } else {
                        ao::net::protocol::WriteConsoleMsg(leader, std::string(target_user.name) + " ya es miembro de la party.", FontTypeNames::FONTTYPE_PARTY);
                    }
                } else {
                    if (cb.WriteConsoleMsg) {
                        cb.WriteConsoleMsg(leader, std::string(target_user.name) + " ya es miembro de otra party.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
                    } else {
                        ao::net::protocol::WriteConsoleMsg(leader, std::string(target_user.name) + " ya es miembro de otra party.", FontTypeNames::FONTTYPE_PARTY);
                    }
                }
            }
        } else {
            if (cb.WriteConsoleMsg) {
                cb.WriteConsoleMsg(leader, "¡Está muerto, no puedes aceptar miembros en ese estado!", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
            } else {
                ao::net::protocol::WriteConsoleMsg(leader, "¡Está muerto, no puedes aceptar miembros en ese estado!", FontTypeNames::FONTTYPE_PARTY);
            }
        }
    } else {
        if (target_user.PartyIndex == PI) {
            if (cb.WriteConsoleMsg) {
                cb.WriteConsoleMsg(leader, std::string(target_user.name) + " ya es miembro de la party.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
            } else {
                ao::net::protocol::WriteConsoleMsg(leader, std::string(target_user.name) + " ya es miembro de la party.", FontTypeNames::FONTTYPE_PARTY);
            }
        } else {
            if (cb.WriteConsoleMsg) {
                cb.WriteConsoleMsg(leader, std::string(target_user.name) + " no ha solicitado ingresar a tu party.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
            } else {
                ao::net::protocol::WriteConsoleMsg(leader, std::string(target_user.name) + " no ha solicitado ingresar a tu party.", FontTypeNames::FONTTYPE_PARTY);
            }
        }
    }
}

void BroadCastParty(std::int16_t user_index, std::string_view texto) {
    if (user_index <= 0 || user_index > MaxUsers) return;

    auto PI = UserList[user_index].PartyIndex;
    if (PI > 0 && PI <= MAX_PARTIES && Parties[PI]) {
        Parties[PI]->MandarMensajeAConsola(texto, UserList[user_index].name);
    }
}

void OnlineParty(std::int16_t user_index) {
    if (user_index <= 0 || user_index > MaxUsers) return;

    const auto& cb = GetPartyCallbacks();
    auto PI = UserList[user_index].PartyIndex;

    if (PI > 0 && PI <= MAX_PARTIES && Parties[PI]) {
        std::array<std::int16_t, ModParty::PARTY_MAXMEMBERS> members_online{};
        Parties[PI]->ObtenerMiembrosOnline(members_online);

        std::string text = "Nombre(Exp): ";
        for (std::size_t i = 0; i < ModParty::PARTY_MAXMEMBERS; ++i) {
            if (members_online[i] > 0 && members_online[i] <= MaxUsers) {
                text += " - " + std::string(UserList[members_online[i]].name) + " (" + std::to_string(Parties[PI]->MiExperiencia(members_online[i])) + ")";
            }
        }
        text += ". Experiencia total: " + std::to_string(Parties[PI]->ObtenerExperienciaTotal());

        if (cb.WriteConsoleMsg) {
            cb.WriteConsoleMsg(user_index, text, static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, text, FontTypeNames::FONTTYPE_PARTY);
        }
    }
}

void TransformarEnLider(std::int16_t old_leader, std::int16_t new_leader) {
    if (old_leader <= 0 || old_leader > MaxUsers || new_leader <= 0 || new_leader > MaxUsers) return;
    if (old_leader == new_leader) return;

    if (!UserPuedeEjecutarComandos(old_leader)) return;

    const auto& cb = GetPartyCallbacks();
    auto PI = UserList[old_leader].PartyIndex;

    if (PI > 0 && PI <= MAX_PARTIES && PI == UserList[new_leader].PartyIndex && Parties[PI]) {
        if (UserList[new_leader].flags.Muerto == 0) {
            if (Parties[PI]->HacerLeader(new_leader)) {
                Parties[PI]->MandarMensajeAConsola("El nuevo líder de la party es " + std::string(UserList[new_leader].name), UserList[old_leader].name);
            } else {
                if (cb.WriteConsoleMsg) {
                    cb.WriteConsoleMsg(old_leader, "¡No se ha hecho el cambio de mando!", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_PARTY));
                } else {
                    ao::net::protocol::WriteConsoleMsg(old_leader, "¡No se ha hecho el cambio de mando!", FontTypeNames::FONTTYPE_PARTY);
                }
            }
        } else {
            if (cb.WriteConsoleMsg) {
                cb.WriteConsoleMsg(old_leader, "¡Está muerto!", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_INFO));
            } else {
                ao::net::protocol::WriteConsoleMsg(old_leader, "¡Está muerto!", FontTypeNames::FONTTYPE_INFO);
            }
        }
    } else {
        if (cb.WriteConsoleMsg) {
            cb.WriteConsoleMsg(old_leader, std::string(UserList[new_leader].name) + " no pertenece a tu party.", static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_INFO));
        } else {
            ao::net::protocol::WriteConsoleMsg(old_leader, std::string(UserList[new_leader].name) + " no pertenece a tu party.", FontTypeNames::FONTTYPE_INFO);
        }
    }
}

void ActualizaExperiencias() {
    if (!PARTY_EXPERIENCIAPORGOLPE) {
        for (std::int16_t i = 1; i <= MAX_PARTIES; ++i) {
            if (Parties[i]) {
                Parties[i]->FlushExperiencia();
            }
        }
    }
}

void ObtenerExito(std::int16_t user_index, std::int32_t exp_ganada, std::int16_t mapa, std::int16_t x, std::int16_t y) {
    if (user_index <= 0 || user_index > MaxUsers) return;

    if (exp_ganada <= 0) {
        if (!CASTIGOS) return;
    }

    auto PI = UserList[user_index].PartyIndex;
    if (PI > 0 && PI <= MAX_PARTIES && Parties[PI]) {
        Parties[PI]->ObtenerExito(exp_ganada, mapa, x, y);
    }
}

std::int16_t CantMiembros(std::int16_t user_index) {
    if (user_index <= 0 || user_index > MaxUsers) return 0;

    auto PI = UserList[user_index].PartyIndex;
    if (PI > 0 && PI <= MAX_PARTIES && Parties[PI]) {
        return Parties[PI]->CantMiembros();
    }
    return 0;
}

void ActualizarSumaNivelesElevados(std::int16_t user_index) {
    if (user_index <= 0 || user_index > MaxUsers) return;

    auto PI = UserList[user_index].PartyIndex;
    if (PI > 0 && PI <= MAX_PARTIES && Parties[PI]) {
        Parties[PI]->UpdateSumaNivelesElevados(UserList[user_index].Stats.ELV);
    }
}

} // namespace ModParty
