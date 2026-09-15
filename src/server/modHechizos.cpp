#include "modHechizos.hpp"
#include "Protocol.hpp"
#include "modSendData.hpp"
#include "TCP.hpp"
#include "Matematicas.hpp"

#include <algorithm>
#include <string>
#include <string_view>

namespace ao {

namespace {
SpellsCallbacks s_callbacks;
}

void SetSpellsCallbacks(const SpellsCallbacks& callbacks) noexcept {
    s_callbacks = callbacks;
}

void ResetSpellsCallbacks() noexcept {
    s_callbacks = SpellsCallbacks{};
}

// ============================================================================
// Fase 1: Procedimientos de Gestión del Libro, Utilidades y Validación
// ============================================================================

bool TieneHechizo(std::int16_t i, std::int16_t UserIndex) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return false;
    }

    const auto& user = UserList[UserIndex];
    for (int j = 1; j <= MAXUSERHECHIZOS; ++j) {
        if (user.Stats.UserHechizos[j] == i) {
            return true;
        }
    }

    return false;
}

void AgregarHechizo(std::int16_t UserIndex, std::int16_t Slot) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return;
    }
    if (Slot < 1 || Slot > MAX_INVENTORY_SLOTS) {
        return;
    }

    auto& user = UserList[UserIndex];
    const std::int16_t obj_index = user.Invent.Object[Slot].ObjIndex;
    if (obj_index < 1 || static_cast<std::size_t>(obj_index) >= ObjDataList.size()) {
        return;
    }

    const std::int16_t hIndex = ObjDataList[obj_index].HechizoIndex;
    if (!TieneHechizo(hIndex, UserIndex)) {
        int empty_slot = 0;
        for (int j = 1; j <= MAXUSERHECHIZOS; ++j) {
            if (user.Stats.UserHechizos[j] == 0) {
                empty_slot = j;
                break;
            }
        }

        if (empty_slot == 0) {
            ao::net::protocol::WriteConsoleMsg(UserIndex, "No tienes espacio para más hechizos.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
        } else {
            user.Stats.UserHechizos[empty_slot] = hIndex;
            UpdateUserHechizos(false, UserIndex, static_cast<std::uint8_t>(empty_slot));
            if (s_callbacks.QuitarUserInvItem) {
                s_callbacks.QuitarUserInvItem(UserIndex, static_cast<std::uint8_t>(Slot), 1);
            }
        }
    } else {
        ao::net::protocol::WriteConsoleMsg(UserIndex, "Ya tienes ese hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
    }
}

void DecirPalabrasMagicas(std::string_view SpellWords, std::int16_t UserIndex) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return;
    }

    auto& user = UserList[UserIndex];
    if (user.flags.AdminInvisible != 1) {
        auto msg = ao::net::protocol::PrepareMessageChatOverHead(SpellWords, user.char_appearance.CharIndex, COLOR_CYAN);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, UserIndex, msg);

        if (user.flags.Oculto == 1) {
            user.flags.Oculto = 0;
            user.Counters.TiempoOculto = 0;

            if (user.flags.invisible == 0) {
                ao::net::protocol::WriteConsoleMsg(UserIndex, "Has vuelto a ser visible.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                if (s_callbacks.SetInvisible) {
                    s_callbacks.SetInvisible(UserIndex, user.char_appearance.CharIndex, false);
                } else {
                    ao::net::protocol::WriteSetInvisible(UserIndex, user.char_appearance.CharIndex, false);
                }
            }
        }
    }
}

bool PuedeLanzar(std::int16_t UserIndex, std::int16_t HechizoIndex) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return false;
    }
    if (HechizoIndex < 1 || static_cast<std::size_t>(HechizoIndex) >= Hechizos.size()) {
        return false;
    }

    const auto& hechizo = Hechizos[HechizoIndex];
    const auto& user = UserList[UserIndex];

    if (user.flags.Muerto) {
        ao::net::protocol::WriteConsoleMsg(UserIndex, "No puedes lanzar hechizos estando muerto.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
        return false;
    }

    if (hechizo.NeedStaff > 0) {
        if (user.clase == eClass::Mage) {
            if (user.Invent.WeaponEqpObjIndex > 0) {
                const auto weapon_idx = user.Invent.WeaponEqpObjIndex;
                if (weapon_idx >= 1 && static_cast<std::size_t>(weapon_idx) < ObjDataList.size()) {
                    if (ObjDataList[weapon_idx].StaffPower < hechizo.NeedStaff) {
                        ao::net::protocol::WriteConsoleMsg(UserIndex, "No posees un báculo lo suficientemente poderoso para poder lanzar el conjuro.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                        return false;
                    }
                } else {
                    ao::net::protocol::WriteConsoleMsg(UserIndex, "No puedes lanzar este conjuro sin la ayuda de un báculo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                    return false;
                }
            } else {
                ao::net::protocol::WriteConsoleMsg(UserIndex, "No puedes lanzar este conjuro sin la ayuda de un báculo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                return false;
            }
        }
    }

    const auto magia_skill_idx = static_cast<std::size_t>(eSkill::Magia);
    if (magia_skill_idx < std::size(user.Stats.UserSkills)) {
        if (user.Stats.UserSkills[magia_skill_idx] < hechizo.MinSkill) {
            ao::net::protocol::WriteConsoleMsg(UserIndex, "No tienes suficientes puntos de magia para lanzar este hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            return false;
        }
    }

    if (user.Stats.MinSta < hechizo.StaRequerido) {
        if (user.Genero == eGenero::Hombre) {
            ao::net::protocol::WriteConsoleMsg(UserIndex, "Estás muy cansado para lanzar este hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
        } else {
            ao::net::protocol::WriteConsoleMsg(UserIndex, "Estás muy cansada para lanzar este hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
        }
        return false;
    }

    // Quirk 7.4: Bonificaciones de coste de maná para Druida con Flauta Élfica
    // (50% en mimetismo, 70% en invocaciones, 90% en magia general excepto Apocalipsis)
    float DruidManaBonus = 1.0f;
    if (user.clase == eClass::Druid) {
        if (user.Invent.AnilloEqpObjIndex == FLAUTAELFICA) {
            if (hechizo.Mimetiza == 1) {
                DruidManaBonus = 0.5f;
            } else if (hechizo.Tipo == TipoHechizo::uInvocacion) {
                DruidManaBonus = 0.7f;
            } else if (HechizoIndex != APOCALIPSIS_SPELL_INDEX) {
                DruidManaBonus = 0.9f;
            }
        }

        if (hechizo.Warp == 1) {
            if (user.Stats.MinMAN != user.Stats.MaxMAN) {
                ao::net::protocol::WriteConsoleMsg(UserIndex, "Debes poseer toda tu maná para poder lanzar este hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                return false;
            } else if (user.NroMascotas == 0) {
                ao::net::protocol::WriteConsoleMsg(UserIndex, "Debes poseer alguna mascota para poder lanzar este hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                return false;
            }
        }
    }

    const auto required_mana = static_cast<std::int32_t>(static_cast<float>(hechizo.ManaRequerido) * DruidManaBonus);
    if (user.Stats.MinMAN < required_mana) {
        ao::net::protocol::WriteConsoleMsg(UserIndex, "No tienes suficiente maná.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
        return false;
    }

    return true;
}

void InfoHechizo(std::int16_t UserIndex) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return;
    }

    const auto& user = UserList[UserIndex];
    const std::int16_t SpellIndex = user.flags.Hechizo;
    if (SpellIndex < 1 || static_cast<std::size_t>(SpellIndex) >= Hechizos.size()) {
        return;
    }

    const auto& hechizo = Hechizos[SpellIndex];
    const std::int16_t tUser = user.flags.TargetUser;
    const std::int16_t tNPC = user.flags.TargetNPC;

    DecirPalabrasMagicas(hechizo.PalabrasMagicas, UserIndex);

    if (tUser > 0 && static_cast<std::size_t>(tUser) < UserList.size()) {
        if (user.flags.AdminInvisible == 1 && UserIndex == tUser) {
            auto fx = ao::net::protocol::PrepareMessageCreateFX(UserList[tUser].char_appearance.CharIndex, hechizo.FXgrh, hechizo.loops);
            TCP::EnviarDatosASlot(UserIndex, std::string_view(reinterpret_cast<const char*>(fx.data()), fx.size()));

            auto wav = ao::net::protocol::PrepareMessagePlayWave(static_cast<std::uint8_t>(hechizo.WAV), UserList[tUser].Pos.X, UserList[tUser].Pos.Y);
            TCP::EnviarDatosASlot(UserIndex, std::string_view(reinterpret_cast<const char*>(wav.data()), wav.size()));
        } else {
            auto fx = ao::net::protocol::PrepareMessageCreateFX(UserList[tUser].char_appearance.CharIndex, hechizo.FXgrh, hechizo.loops);
            ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, tUser, fx);

            auto wav = ao::net::protocol::PrepareMessagePlayWave(static_cast<std::uint8_t>(hechizo.WAV), UserList[tUser].Pos.X, UserList[tUser].Pos.Y);
            ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, tUser, wav);
        }
    } else if (tNPC > 0 && static_cast<std::size_t>(tNPC) < Npclist.size()) {
        auto fx = ao::net::protocol::PrepareMessageCreateFX(Npclist[tNPC].char_appearance.CharIndex, hechizo.FXgrh, hechizo.loops);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToNPCArea, tNPC, fx);

        auto wav = ao::net::protocol::PrepareMessagePlayWave(static_cast<std::uint8_t>(hechizo.WAV), Npclist[tNPC].Pos.X, Npclist[tNPC].Pos.Y);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToNPCArea, tNPC, wav);
    }

    if (tUser > 0 && static_cast<std::size_t>(tUser) < UserList.size()) {
        if (UserIndex != tUser) {
            if (user.showName) {
                ao::net::protocol::WriteConsoleMsg(UserIndex, hechizo.HechizeroMsg + " " + UserList[tUser].name, ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            } else {
                ao::net::protocol::WriteConsoleMsg(UserIndex, hechizo.HechizeroMsg + " alguien.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            }
            ao::net::protocol::WriteConsoleMsg(tUser, user.name + " " + hechizo.TargetMsg, ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        } else {
            ao::net::protocol::WriteConsoleMsg(UserIndex, hechizo.PropioMsg, ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        }
    } else if (tNPC > 0 && static_cast<std::size_t>(tNPC) < Npclist.size()) {
        ao::net::protocol::WriteConsoleMsg(UserIndex, hechizo.HechizeroMsg + " la criatura.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
    }
}

void UpdateUserHechizos(bool UpdateAll, std::int16_t UserIndex, std::uint8_t Slot) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return;
    }

    auto& user = UserList[UserIndex];
    if (!UpdateAll) {
        if (Slot >= 1 && Slot <= MAXUSERHECHIZOS) {
            ChangeUserHechizo(UserIndex, Slot, user.Stats.UserHechizos[Slot]);
        }
    } else {
        for (std::uint8_t loopC = 1; loopC <= MAXUSERHECHIZOS; ++loopC) {
            ChangeUserHechizo(UserIndex, loopC, user.Stats.UserHechizos[loopC]);
        }
    }
}

void ChangeUserHechizo(std::int16_t UserIndex, std::uint8_t Slot, std::int16_t Hechizo) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return;
    }
    if (Slot < 1 || Slot > MAXUSERHECHIZOS) {
        return;
    }

    UserList[UserIndex].Stats.UserHechizos[Slot] = Hechizo;
    ao::net::protocol::WriteChangeSpellSlot(UserIndex, Slot);
}

void DesplazarHechizo(std::int16_t UserIndex, std::int16_t Dire, std::int16_t HechizoDesplazado) {
    if (Dire != 1 && Dire != -1) {
        return;
    }
    if (HechizoDesplazado < 1 || HechizoDesplazado > MAXUSERHECHIZOS) {
        return;
    }
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return;
    }

    auto& user = UserList[UserIndex];

    if (Dire == 1) { // Mover arriba
        if (HechizoDesplazado == 1) {
            ao::net::protocol::WriteConsoleMsg(UserIndex, "No puedes mover el hechizo en esa dirección.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            return;
        }

        std::swap(user.Stats.UserHechizos[HechizoDesplazado], user.Stats.UserHechizos[HechizoDesplazado - 1]);
    } else { // Mover abajo
        if (HechizoDesplazado == MAXUSERHECHIZOS) {
            ao::net::protocol::WriteConsoleMsg(UserIndex, "No puedes mover el hechizo en esa dirección.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            return;
        }

        std::swap(user.Stats.UserHechizos[HechizoDesplazado], user.Stats.UserHechizos[HechizoDesplazado + 1]);
    }
}

// ============================================================================
// Fase 2: Efectos Cuantitativos y Modificación de Atributos (G2)
// ============================================================================

bool CanSupportUser(std::int16_t CasterIndex, std::int16_t TargetIndex, bool DoCriminal) {
    if (s_callbacks.CanSupportUser) {
        return s_callbacks.CanSupportUser(CasterIndex, TargetIndex, DoCriminal);
    }

    if (CasterIndex == TargetIndex) {
        return true;
    }

    if (CasterIndex < 1 || static_cast<std::size_t>(CasterIndex) >= UserList.size() ||
        TargetIndex < 1 || static_cast<std::size_t>(TargetIndex) >= UserList.size()) {
        return false;
    }

    const auto& caster = UserList[CasterIndex];
    const auto& target = UserList[TargetIndex];

    if (caster.flags.EnConsulta) {
        ao::net::protocol::WriteConsoleMsg(CasterIndex, "No puedes ayudar usuarios mientras estas en consulta.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
        return false;
    }

    if (s_callbacks.TriggerZonaPelea && s_callbacks.TriggerZonaPelea(CasterIndex, TargetIndex) == static_cast<std::int16_t>(eTrigger6::TRIGGER6_PERMITE)) {
        return true;
    }

    const bool target_criminal = s_callbacks.Criminal ? s_callbacks.Criminal(TargetIndex) : false;
    const bool caster_criminal = s_callbacks.Criminal ? s_callbacks.Criminal(CasterIndex) : false;
    const bool caster_es_armada = s_callbacks.EsArmada ? s_callbacks.EsArmada(CasterIndex) : false;
    const bool caster_es_caos = s_callbacks.EsCaos ? s_callbacks.EsCaos(CasterIndex) : false;

    if (target_criminal) {
        if (!caster_criminal) {
            if (caster_es_armada) {
                ao::net::protocol::WriteConsoleMsg(CasterIndex, "Los miembros del ejército real no pueden ayudar a los criminales.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                return false;
            }

            if (caster.flags.Seguro) {
                ao::net::protocol::WriteConsoleMsg(CasterIndex, "Para ayudar criminales debes sacarte el seguro ya que te volverás criminal como ellos.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                return false;
            } else {
                if (DoCriminal) {
                    if (s_callbacks.VolverCriminal) {
                        s_callbacks.VolverCriminal(CasterIndex);
                    }
                } else {
                    DisNobAuBan(CasterIndex, static_cast<double>(caster.Reputacion.NobleRep) * 0.5, 10000);
                }
            }
        }
    } else {
        if (caster_es_caos) {
            ao::net::protocol::WriteConsoleMsg(CasterIndex, "Los miembros de la legión oscura no pueden ayudar a los ciudadanos.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            return false;
        } else if (!caster_criminal) {
            if (target.flags.AtacablePor > 0) {
                if (target.flags.AtacablePor != CasterIndex) {
                    if (caster_es_armada) {
                        ao::net::protocol::WriteConsoleMsg(CasterIndex, "Los miembros del ejército real no pueden ayudar a ciudadanos en estado atacable.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                        return false;
                    }

                    if (caster.flags.Seguro) {
                        ao::net::protocol::WriteConsoleMsg(CasterIndex, "Para ayudar ciudadanos en estado atacable debes sacarte el seguro, pero te puedes volver criminal.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                        return false;
                    } else {
                        DisNobAuBan(CasterIndex, static_cast<double>(caster.Reputacion.NobleRep) * 0.5, 10000);
                    }
                }
            }
        }
    }

    return true;
}

bool HechizoPropUsuario(std::int16_t user_index) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return false;
    }

    const std::int16_t SpellIndex = UserList[user_index].flags.Hechizo;
    if (SpellIndex < 1 || static_cast<std::size_t>(SpellIndex) >= Hechizos.size()) {
        return false;
    }

    const std::int16_t TargetIndex = UserList[user_index].flags.TargetUser;
    if (TargetIndex < 1 || static_cast<std::size_t>(TargetIndex) >= UserList.size()) {
        return false;
    }

    auto& target = UserList[TargetIndex];
    const auto& caster = UserList[user_index];
    const auto& hechizo = Hechizos[SpellIndex];

    if (target.flags.Muerto) {
        ao::net::protocol::WriteConsoleMsg(user_index, "No puedes lanzar este hechizo a un muerto.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
        return false;
    }

    std::int32_t daño = 0; // BUG #38: variable residual

    // <-------- Aumenta Hambre ---------->
    if (hechizo.SubeHam == 1) {
        InfoHechizo(user_index);
        daño = RandomNumber(hechizo.MinHam, hechizo.MaxHam);
        target.Stats.MinHam += static_cast<std::int16_t>(daño);
        if (target.Stats.MinHam > target.Stats.MaxHam) {
            target.Stats.MinHam = target.Stats.MaxHam;
        }

        if (user_index != TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Le has restaurado " + std::to_string(daño) + " puntos de hambre a " + target.name + ".", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            ao::net::protocol::WriteConsoleMsg(TargetIndex, caster.name + " te ha restaurado " + std::to_string(daño) + " puntos de hambre.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Te has restaurado " + std::to_string(daño) + " puntos de hambre.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        }

        ao::net::protocol::WriteUpdateHungerAndThirst(TargetIndex);
    }
    // <-------- Quita Hambre ---------->
    else if (hechizo.SubeHam == 2) {
        if (s_callbacks.PuedeAtacar && !s_callbacks.PuedeAtacar(user_index, TargetIndex)) {
            return false;
        }

        if (user_index != TargetIndex) {
            if (s_callbacks.UsuarioAtacadoPorUsuario) {
                s_callbacks.UsuarioAtacadoPorUsuario(user_index, TargetIndex);
            }
        } else {
            return false;
        }

        InfoHechizo(user_index);
        daño = RandomNumber(hechizo.MinHam, hechizo.MaxHam);
        target.Stats.MinHam -= static_cast<std::int16_t>(daño);

        if (user_index != TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Le has quitado " + std::to_string(daño) + " puntos de hambre a " + target.name + ".", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            ao::net::protocol::WriteConsoleMsg(TargetIndex, caster.name + " te ha quitado " + std::to_string(daño) + " puntos de hambre.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Te has quitado " + std::to_string(daño) + " puntos de hambre.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        }

        if (target.Stats.MinHam < 1) {
            target.Stats.MinHam = 0;
            target.flags.Hambre = 1;
        }

        ao::net::protocol::WriteUpdateHungerAndThirst(TargetIndex);
    }

    // <-------- Aumenta Sed ---------->
    if (hechizo.SubeSed == 1) {
        InfoHechizo(user_index);
        daño = RandomNumber(hechizo.MinSed, hechizo.MaxSed);
        target.Stats.MinAGU += static_cast<std::int16_t>(daño);
        if (target.Stats.MinAGU > target.Stats.MaxAGU) {
            target.Stats.MinAGU = target.Stats.MaxAGU;
        }

        ao::net::protocol::WriteUpdateHungerAndThirst(TargetIndex);

        if (user_index != TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Le has restaurado " + std::to_string(daño) + " puntos de sed a " + target.name + ".", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            ao::net::protocol::WriteConsoleMsg(TargetIndex, caster.name + " te ha restaurado " + std::to_string(daño) + " puntos de sed.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Te has restaurado " + std::to_string(daño) + " puntos de sed.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        }
    }
    // <-------- Quita Sed ---------->
    else if (hechizo.SubeSed == 2) {
        if (s_callbacks.PuedeAtacar && !s_callbacks.PuedeAtacar(user_index, TargetIndex)) {
            return false;
        }

        if (user_index != TargetIndex) {
            if (s_callbacks.UsuarioAtacadoPorUsuario) {
                s_callbacks.UsuarioAtacadoPorUsuario(user_index, TargetIndex);
            }
        }

        InfoHechizo(user_index);
        daño = RandomNumber(hechizo.MinSed, hechizo.MaxSed);
        target.Stats.MinAGU -= static_cast<std::int16_t>(daño);

        if (user_index != TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Le has quitado " + std::to_string(daño) + " puntos de sed a " + target.name + ".", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            ao::net::protocol::WriteConsoleMsg(TargetIndex, caster.name + " te ha quitado " + std::to_string(daño) + " puntos de sed.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Te has quitado " + std::to_string(daño) + " puntos de sed.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        }

        if (target.Stats.MinAGU < 1) {
            target.Stats.MinAGU = 0;
            target.flags.Sed = 1;
        }

        ao::net::protocol::WriteUpdateHungerAndThirst(TargetIndex);
    }

    // <-------- Aumenta Agilidad ---------->
    if (hechizo.SubeAgilidad == 1) {
        if (!CanSupportUser(user_index, TargetIndex)) {
            return false;
        }

        InfoHechizo(user_index);
        daño = RandomNumber(hechizo.MinAgilidad, hechizo.MaxAgilidad);
        target.flags.DuracionEfecto = 1200;

        std::int32_t val = static_cast<std::int32_t>(target.Stats.UserAtributos[eAtributos::Agilidad]) + daño;
        const std::int32_t max_agil = std::min(static_cast<std::int32_t>(MAXATRIBUTOS), static_cast<std::int32_t>(target.Stats.UserAtributosBackUP[eAtributos::Agilidad] * 2));
        if (val > max_agil) {
            val = max_agil;
        }
        target.Stats.UserAtributos[eAtributos::Agilidad] = static_cast<std::uint8_t>(val);

        target.flags.TomoPocion = true;
        ao::net::protocol::WriteUpdateDexterity(TargetIndex);
    }
    // <-------- Quita Agilidad ---------->
    else if (hechizo.SubeAgilidad == 2) {
        if (s_callbacks.PuedeAtacar && !s_callbacks.PuedeAtacar(user_index, TargetIndex)) {
            return false;
        }

        if (user_index != TargetIndex) {
            if (s_callbacks.UsuarioAtacadoPorUsuario) {
                s_callbacks.UsuarioAtacadoPorUsuario(user_index, TargetIndex);
            }
        }

        InfoHechizo(user_index);
        target.flags.TomoPocion = true;
        daño = RandomNumber(hechizo.MinAgilidad, hechizo.MaxAgilidad);
        target.flags.DuracionEfecto = 700;

        std::int32_t val = static_cast<std::int32_t>(target.Stats.UserAtributos[eAtributos::Agilidad]) - daño;
        if (val < static_cast<std::int32_t>(MINATRIBUTOS)) {
            val = static_cast<std::int32_t>(MINATRIBUTOS);
        }
        target.Stats.UserAtributos[eAtributos::Agilidad] = static_cast<std::uint8_t>(val);

        ao::net::protocol::WriteUpdateDexterity(TargetIndex);
    }

    // <-------- Aumenta Fuerza ---------->
    if (hechizo.SubeFuerza == 1) {
        if (!CanSupportUser(user_index, TargetIndex)) {
            return false;
        }

        InfoHechizo(user_index);
        daño = RandomNumber(hechizo.MinFuerza, hechizo.MaxFuerza);
        target.flags.DuracionEfecto = 1200;

        std::int32_t val = static_cast<std::int32_t>(target.Stats.UserAtributos[eAtributos::Fuerza]) + daño;
        const std::int32_t max_fuerza = std::min(static_cast<std::int32_t>(MAXATRIBUTOS), static_cast<std::int32_t>(target.Stats.UserAtributosBackUP[eAtributos::Fuerza] * 2));
        if (val > max_fuerza) {
            val = max_fuerza;
        }
        target.Stats.UserAtributos[eAtributos::Fuerza] = static_cast<std::uint8_t>(val);

        target.flags.TomoPocion = true;
        ao::net::protocol::WriteUpdateStrenght(TargetIndex);
    }
    // <-------- Quita Fuerza ---------->
    else if (hechizo.SubeFuerza == 2) {
        if (s_callbacks.PuedeAtacar && !s_callbacks.PuedeAtacar(user_index, TargetIndex)) {
            return false;
        }

        if (user_index != TargetIndex) {
            if (s_callbacks.UsuarioAtacadoPorUsuario) {
                s_callbacks.UsuarioAtacadoPorUsuario(user_index, TargetIndex);
            }
        }

        InfoHechizo(user_index);
        target.flags.TomoPocion = true;
        daño = RandomNumber(hechizo.MinFuerza, hechizo.MaxFuerza);
        target.flags.DuracionEfecto = 700;

        std::int32_t val = static_cast<std::int32_t>(target.Stats.UserAtributos[eAtributos::Fuerza]) - daño;
        if (val < static_cast<std::int32_t>(MINATRIBUTOS)) {
            val = static_cast<std::int32_t>(MINATRIBUTOS);
        }
        target.Stats.UserAtributos[eAtributos::Fuerza] = static_cast<std::uint8_t>(val);

        ao::net::protocol::WriteUpdateStrenght(TargetIndex);
    }

    // <-------- Cura salud ---------->
    if (hechizo.SubeHP == 1) {
        if (!CanSupportUser(user_index, TargetIndex)) {
            return false;
        }

        daño = RandomNumber(hechizo.MinHp, hechizo.MaxHp);
        daño = daño + Porcentaje(daño, 3 * caster.Stats.ELV);

        InfoHechizo(user_index);

        target.Stats.MinHp += static_cast<std::int16_t>(daño);
        if (target.Stats.MinHp > target.Stats.MaxHp) {
            target.Stats.MinHp = target.Stats.MaxHp;
        }

        ao::net::protocol::WriteUpdateHP(TargetIndex);

        if (user_index != TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Le has restaurado " + std::to_string(daño) + " puntos de vida a " + target.name + ".", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            ao::net::protocol::WriteConsoleMsg(TargetIndex, caster.name + " te ha restaurado " + std::to_string(daño) + " puntos de vida.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Te has restaurado " + std::to_string(daño) + " puntos de vida.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        }
    }
    // <-------- Quita salud (Daña) ---------->
    else if (hechizo.SubeHP == 2) {
        if (user_index == TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "No puedes atacarte a vos mismo.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            return false;
        }

        daño = RandomNumber(hechizo.MinHp, hechizo.MaxHp);
        daño = daño + Porcentaje(daño, 3 * caster.Stats.ELV);

        // Quirk 7.5: Multiplicadores canónicos de báculo (Mago desarmado 0.7x vs equipado (Bonus+70)/100)
        // y bonificación de bardo (1.04x con LAUDELFICO o FLAUTAELFICA)
        if (hechizo.StaffAffected) {
            if (caster.clase == eClass::Mage) {
                const auto weapon_idx = caster.Invent.WeaponEqpObjIndex;
                if (weapon_idx > 0 && static_cast<std::size_t>(weapon_idx) < ObjDataList.size()) {
                    daño = (daño * (ObjDataList[weapon_idx].StaffDamageBonus + 70)) / 100;
                } else {
                    daño = static_cast<std::int32_t>(daño * 0.7);
                }
            }
        }

        const auto anillo_idx = caster.Invent.AnilloEqpObjIndex;
        if (anillo_idx == LAUDELFICO || anillo_idx == FLAUTAELFICA) {
            daño = static_cast<std::int32_t>(daño * 1.04);
        }

        const auto casco_target = target.Invent.CascoEqpObjIndex;
        if (casco_target > 0 && static_cast<std::size_t>(casco_target) < ObjDataList.size()) {
            daño -= RandomNumber(ObjDataList[casco_target].DefensaMagicaMin, ObjDataList[casco_target].DefensaMagicaMax);
        }

        const auto anillo_target = target.Invent.AnilloEqpObjIndex;
        if (anillo_target > 0 && static_cast<std::size_t>(anillo_target) < ObjDataList.size()) {
            daño -= RandomNumber(ObjDataList[anillo_target].DefensaMagicaMin, ObjDataList[anillo_target].DefensaMagicaMax);
        }

        if (daño < 0) {
            daño = 0;
        }

        if (s_callbacks.PuedeAtacar && !s_callbacks.PuedeAtacar(user_index, TargetIndex)) {
            return false;
        }

        if (user_index != TargetIndex) {
            if (s_callbacks.UsuarioAtacadoPorUsuario) {
                s_callbacks.UsuarioAtacadoPorUsuario(user_index, TargetIndex);
            }
        }

        InfoHechizo(user_index);

        target.Stats.MinHp -= static_cast<std::int16_t>(daño);

        ao::net::protocol::WriteUpdateHP(TargetIndex);

        ao::net::protocol::WriteConsoleMsg(user_index, "Le has quitado " + std::to_string(daño) + " puntos de vida a " + target.name + ".", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        ao::net::protocol::WriteConsoleMsg(TargetIndex, caster.name + " te ha quitado " + std::to_string(daño) + " puntos de vida.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);

        if (target.Stats.MinHp < 1) {
            if (target.flags.AtacablePor != user_index) {
                if (s_callbacks.StoreFrag) {
                    s_callbacks.StoreFrag(user_index, TargetIndex);
                }
                if (s_callbacks.ContarMuerte) {
                    s_callbacks.ContarMuerte(TargetIndex, user_index);
                }
            }

            target.Stats.MinHp = 0;
            if (s_callbacks.ActStats) {
                s_callbacks.ActStats(TargetIndex, user_index);
            }
            if (s_callbacks.UserDie) {
                s_callbacks.UserDie(TargetIndex);
            }
        }
    }

    // <-------- Aumenta Mana ---------->
    // REPLICACIÓN BUG #38: NO calcula daño con RandomNumber, usa daño residual acumulado previo
    if (hechizo.SubeMana == 1) {
        InfoHechizo(user_index);
        target.Stats.MinMAN += static_cast<std::int16_t>(daño);
        if (target.Stats.MinMAN > target.Stats.MaxMAN) {
            target.Stats.MinMAN = target.Stats.MaxMAN;
        }

        ao::net::protocol::WriteUpdateMana(TargetIndex);

        if (user_index != TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Le has restaurado " + std::to_string(daño) + " puntos de maná a " + target.name + ".", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            ao::net::protocol::WriteConsoleMsg(TargetIndex, caster.name + " te ha restaurado " + std::to_string(daño) + " puntos de maná.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Te has restaurado " + std::to_string(daño) + " puntos de maná.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        }
    }
    // <-------- Quita Mana ---------->
    // REPLICACIÓN BUG #38: NO calcula daño con RandomNumber, usa daño residual acumulado previo
    else if (hechizo.SubeMana == 2) {
        if (s_callbacks.PuedeAtacar && !s_callbacks.PuedeAtacar(user_index, TargetIndex)) {
            return false;
        }

        if (user_index != TargetIndex) {
            if (s_callbacks.UsuarioAtacadoPorUsuario) {
                s_callbacks.UsuarioAtacadoPorUsuario(user_index, TargetIndex);
            }
        }

        InfoHechizo(user_index);

        if (user_index != TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Le has quitado " + std::to_string(daño) + " puntos de maná a " + target.name + ".", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            ao::net::protocol::WriteConsoleMsg(TargetIndex, caster.name + " te ha quitado " + std::to_string(daño) + " puntos de maná.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Te has quitado " + std::to_string(daño) + " puntos de maná.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        }

        target.Stats.MinMAN -= static_cast<std::int16_t>(daño);
        if (target.Stats.MinMAN < 1) {
            target.Stats.MinMAN = 0;
        }

        ao::net::protocol::WriteUpdateMana(TargetIndex);
    }

    // <-------- Aumenta Stamina ---------->
    // REPLICACIÓN BUG #38: NO calcula daño con RandomNumber, usa daño residual acumulado previo
    if (hechizo.SubeSta == 1) {
        InfoHechizo(user_index);
        target.Stats.MinSta += static_cast<std::int16_t>(daño);
        if (target.Stats.MinSta > target.Stats.MaxSta) {
            target.Stats.MinSta = target.Stats.MaxSta;
        }

        ao::net::protocol::WriteUpdateSta(TargetIndex);

        if (user_index != TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Le has restaurado " + std::to_string(daño) + " puntos de energía a " + target.name + ".", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            ao::net::protocol::WriteConsoleMsg(TargetIndex, caster.name + " te ha restaurado " + std::to_string(daño) + " puntos de energía.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Te has restaurado " + std::to_string(daño) + " puntos de energía.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        }
    }
    // <-------- Quita Stamina ---------->
    // REPLICACIÓN BUG #38: NO calcula daño con RandomNumber, usa daño residual acumulado previo
    else if (hechizo.SubeSta == 2) {
        if (s_callbacks.PuedeAtacar && !s_callbacks.PuedeAtacar(user_index, TargetIndex)) {
            return false;
        }

        if (user_index != TargetIndex) {
            if (s_callbacks.UsuarioAtacadoPorUsuario) {
                s_callbacks.UsuarioAtacadoPorUsuario(user_index, TargetIndex);
            }
        }

        InfoHechizo(user_index);

        if (user_index != TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Le has quitado " + std::to_string(daño) + " puntos de energía a " + target.name + ".", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            ao::net::protocol::WriteConsoleMsg(TargetIndex, caster.name + " te ha quitado " + std::to_string(daño) + " puntos de energía.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Te has quitado " + std::to_string(daño) + " puntos de energía.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        }

        target.Stats.MinSta -= static_cast<std::int16_t>(daño);
        if (target.Stats.MinSta < 1) {
            target.Stats.MinSta = 0;
        }

        ao::net::protocol::WriteUpdateSta(TargetIndex);
    }

    ao::net::protocol::FlushBuffer(TargetIndex);
    return true;
}

void HechizoPropNPC(std::int16_t spell_index, std::int16_t npc_index, std::int16_t user_index, bool& hechizo_casteado) {
    if (spell_index < 1 || static_cast<std::size_t>(spell_index) >= Hechizos.size()) {
        return;
    }
    if (npc_index < 1 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& npc = Npclist[npc_index];
    const auto& user = UserList[user_index];
    const auto& hechizo = Hechizos[spell_index];

    std::int32_t daño = 0;

    // Salud
    if (hechizo.SubeHP == 1) {
        daño = RandomNumber(hechizo.MinHp, hechizo.MaxHp);
        daño = daño + Porcentaje(daño, 3 * user.Stats.ELV);

        InfoHechizo(user_index);
        npc.Stats.MinHp += static_cast<std::int16_t>(daño);
        if (npc.Stats.MinHp > npc.Stats.MaxHp) {
            npc.Stats.MinHp = npc.Stats.MaxHp;
        }

        ao::net::protocol::WriteConsoleMsg(user_index, "Has curado " + std::to_string(daño) + " puntos de vida a la criatura.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        hechizo_casteado = true;
    } else if (hechizo.SubeHP == 2) {
        if (s_callbacks.PuedeAtacarNPC && !s_callbacks.PuedeAtacarNPC(user_index, npc_index, false)) {
            hechizo_casteado = false;
            return;
        }

        if (s_callbacks.NpcAtacado) {
            s_callbacks.NpcAtacado(npc_index, user_index);
        }

        daño = RandomNumber(hechizo.MinHp, hechizo.MaxHp);
        daño = daño + Porcentaje(daño, 3 * user.Stats.ELV);

        if (hechizo.StaffAffected) {
            if (user.clase == eClass::Mage) {
                const auto weapon_idx = user.Invent.WeaponEqpObjIndex;
                if (weapon_idx > 0 && static_cast<std::size_t>(weapon_idx) < ObjDataList.size()) {
                    daño = (daño * (ObjDataList[weapon_idx].StaffDamageBonus + 70)) / 100;
                } else {
                    daño = static_cast<std::int32_t>(daño * 0.7);
                }
            }
        }

        const auto anillo_idx = user.Invent.AnilloEqpObjIndex;
        if (anillo_idx == LAUDELFICO || anillo_idx == FLAUTAELFICA) {
            daño = static_cast<std::int32_t>(daño * 1.04);
        }

        InfoHechizo(user_index);
        hechizo_casteado = true;

        if (npc.flags.Snd2 > 0) {
            auto wav = ao::net::protocol::PrepareMessagePlayWave(static_cast<std::uint8_t>(npc.flags.Snd2), npc.Pos.X, npc.Pos.Y);
            ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToNPCArea, npc_index, wav);
        }

        // Quizás tenga defensa mágica el NPC
        daño -= npc.Stats.defM;
        if (daño < 0) {
            daño = 0;
        }

        npc.Stats.MinHp -= static_cast<std::int16_t>(daño);
        ao::net::protocol::WriteConsoleMsg(user_index, "¡Le has quitado " + std::to_string(daño) + " puntos de vida a la criatura!", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);

        if (s_callbacks.CalcularDarExp) {
            s_callbacks.CalcularDarExp(user_index, npc_index, daño);
        }

        if (npc.Stats.MinHp < 1) {
            npc.Stats.MinHp = 0;
            if (s_callbacks.MuereNpc) {
                s_callbacks.MuereNpc(npc_index, user_index);
            }
        }
    }
}

// ============================================================================
// Fase 3: Estados Alterados, Metamorfosis e Invocaciones (G3)
// ============================================================================

void HechizoEstadoUsuario(std::int16_t user_index, bool& hechizo_casteado) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const std::int16_t HechizoIndex = UserList[user_index].flags.Hechizo;
    if (HechizoIndex < 1 || static_cast<std::size_t>(HechizoIndex) >= Hechizos.size()) {
        return;
    }

    const std::int16_t TargetIndex = UserList[user_index].flags.TargetUser;
    if (TargetIndex < 1 || static_cast<std::size_t>(TargetIndex) >= UserList.size()) {
        return;
    }

    auto& caster = UserList[user_index];
    auto& target = UserList[TargetIndex];
    const auto& hechizo = Hechizos[HechizoIndex];

    // <-------- Agrega Invisibilidad ---------->
    if (hechizo.Invisibilidad == 1) {
        if (target.flags.Muerto == 1) {
            ao::net::protocol::WriteConsoleMsg(user_index, "¡El usuario está muerto!", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            hechizo_casteado = false;
            return;
        }

        if (target.Counters.Saliendo) {
            if (user_index != TargetIndex) {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡El hechizo no tiene efecto!", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡No puedes hacerte invisible mientras te encuentras saliendo!", ao::net::protocol::FontTypeNames::FONTTYPE_WARNING);
            }
            hechizo_casteado = false;
            return;
        }

        // No usar invi en mapas InviSinEfecto
        if (target.Pos.Map > 0 && static_cast<std::size_t>(target.Pos.Map) < MapInfoList.size()) {
            if (MapInfoList[target.Pos.Map].InviSinEfecto > 0) {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡La invisibilidad no funciona aquí!", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                hechizo_casteado = false;
                return;
            }
        }

        // Chequea si el status permite ayudar al otro usuario
        hechizo_casteado = CanSupportUser(user_index, TargetIndex, true);
        if (!hechizo_casteado) {
            return;
        }

        // Si sos user, no uses este hechizo con GMs
        if ((caster.flags.Privilegios & PlayerType::UserPlayer) && !(target.flags.Privilegios & PlayerType::UserPlayer)) {
            hechizo_casteado = false;
            return;
        }

        target.flags.invisible = 1;
        if (s_callbacks.SetInvisible) {
            s_callbacks.SetInvisible(TargetIndex, target.char_appearance.CharIndex, true);
        } else {
            ao::net::protocol::WriteSetInvisible(TargetIndex, target.char_appearance.CharIndex, true);
        }

        InfoHechizo(user_index);
        hechizo_casteado = true;
    }

    // <-------- Agrega Mimetismo ---------->
    if (hechizo.Mimetiza == 1) {
        if (target.flags.Muerto == 1 || target.flags.Navegando == 1 || caster.flags.Navegando == 1) {
            return;
        }

        if ((caster.flags.Privilegios & PlayerType::UserPlayer) && !(target.flags.Privilegios & PlayerType::UserPlayer)) {
            return;
        }

        if (caster.flags.Mimetizado == 1) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Ya te encuentras mimetizado. El hechizo no ha tenido efecto.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            return;
        }

        if (caster.flags.AdminInvisible == 1) {
            return;
        }

        // Copio el char original al mimetizado
        caster.CharMimetizado.body = caster.char_appearance.body;
        caster.CharMimetizado.Head = caster.char_appearance.Head;
        caster.CharMimetizado.CascoAnim = caster.char_appearance.CascoAnim;
        caster.CharMimetizado.ShieldAnim = caster.char_appearance.ShieldAnim;
        caster.CharMimetizado.WeaponAnim = caster.char_appearance.WeaponAnim;

        caster.flags.Mimetizado = 1;

        // Ahora pongo local el del enemigo
        caster.char_appearance.body = target.char_appearance.body;
        caster.char_appearance.Head = target.char_appearance.Head;
        caster.char_appearance.CascoAnim = target.char_appearance.CascoAnim;
        caster.char_appearance.ShieldAnim = target.char_appearance.ShieldAnim;

        std::int16_t weapon_anim = 0;
        if (s_callbacks.GetWeaponAnim) {
            weapon_anim = s_callbacks.GetWeaponAnim(user_index, target.Invent.WeaponEqpObjIndex);
        } else if (target.Invent.WeaponEqpObjIndex > 0 && static_cast<std::size_t>(target.Invent.WeaponEqpObjIndex) < ObjDataList.size()) {
            weapon_anim = ObjDataList[target.Invent.WeaponEqpObjIndex].WeaponAnim;
        }
        caster.char_appearance.WeaponAnim = weapon_anim;

        if (s_callbacks.ChangeUserChar) {
            s_callbacks.ChangeUserChar(user_index, caster.char_appearance.body, caster.char_appearance.Head, caster.char_appearance.heading, caster.char_appearance.WeaponAnim, caster.char_appearance.ShieldAnim, caster.char_appearance.CascoAnim);
        }

        InfoHechizo(user_index);
        hechizo_casteado = true;
    }

    // <-------- Agrega Envenenamiento ---------->
    if (hechizo.Envenena == 1) {
        if (user_index == TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "No puedes atacarte a vos mismo.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            return;
        }

        if (s_callbacks.PuedeAtacar && !s_callbacks.PuedeAtacar(user_index, TargetIndex)) {
            return;
        }

        if (user_index != TargetIndex && s_callbacks.UsuarioAtacadoPorUsuario) {
            s_callbacks.UsuarioAtacadoPorUsuario(user_index, TargetIndex);
        }

        target.flags.Envenenado = 1;
        InfoHechizo(user_index);
        hechizo_casteado = true;
    }

    // <-------- Cura Envenenamiento ---------->
    if (hechizo.CuraVeneno == 1) {
        if (target.flags.Muerto == 1) {
            ao::net::protocol::WriteConsoleMsg(user_index, "¡El usuario está muerto!", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            hechizo_casteado = false;
            return;
        }

        hechizo_casteado = CanSupportUser(user_index, TargetIndex);
        if (!hechizo_casteado) {
            return;
        }

        if ((caster.flags.Privilegios & PlayerType::UserPlayer) && !(target.flags.Privilegios & PlayerType::UserPlayer)) {
            return;
        }

        target.flags.Envenenado = 0;
        InfoHechizo(user_index);
        hechizo_casteado = true;
    }

    // <-------- Agrega Maldicion ---------->
    if (hechizo.Maldicion == 1) {
        if (user_index == TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "No puedes atacarte a vos mismo.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            return;
        }

        if (s_callbacks.PuedeAtacar && !s_callbacks.PuedeAtacar(user_index, TargetIndex)) {
            return;
        }

        if (user_index != TargetIndex && s_callbacks.UsuarioAtacadoPorUsuario) {
            s_callbacks.UsuarioAtacadoPorUsuario(user_index, TargetIndex);
        }

        target.flags.Maldicion = 1;
        InfoHechizo(user_index);
        hechizo_casteado = true;
    }

    // <-------- Remueve Maldicion ---------->
    if (hechizo.RemoverMaldicion == 1) {
        target.flags.Maldicion = 0;
        InfoHechizo(user_index);
        hechizo_casteado = true;
    }

    // <-------- Agrega Bendicion ---------->
    if (hechizo.Bendicion == 1) {
        target.flags.Bendicion = 1;
        InfoHechizo(user_index);
        hechizo_casteado = true;
    }

    // <-------- Agrega Paralisis/Inmobilidad ---------->
    if (hechizo.Paraliza == 1 || hechizo.Inmoviliza == 1) {
        if (user_index == TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "No puedes atacarte a vos mismo.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            return;
        }

        if (target.flags.Paralizado == 0) {
            if (s_callbacks.PuedeAtacar && !s_callbacks.PuedeAtacar(user_index, TargetIndex)) {
                return;
            }

            if (user_index != TargetIndex && s_callbacks.UsuarioAtacadoPorUsuario) {
                s_callbacks.UsuarioAtacadoPorUsuario(user_index, TargetIndex);
            }

            InfoHechizo(user_index);
            hechizo_casteado = true;

            // Inmunidad absoluta de SUPERANILLO
            if (target.Invent.AnilloEqpObjIndex == SUPERANILLO) {
                ao::net::protocol::WriteConsoleMsg(TargetIndex, " Tu anillo rechaza los efectos del hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
                ao::net::protocol::WriteConsoleMsg(user_index, " ¡El hechizo no tiene efecto!", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
                ao::net::protocol::FlushBuffer(TargetIndex);
                return;
            }

            if (hechizo.Inmoviliza == 1) {
                target.flags.Inmovilizado = 1;
            }
            target.flags.Paralizado = 1;
            target.Counters.Paralisis = IntervaloParalizado;

            ao::net::protocol::WriteParalizeOK(TargetIndex);
            ao::net::protocol::FlushBuffer(TargetIndex);
        }
    }

    // <-------- Remueve Paralisis/Inmobilidad ---------->
    if (hechizo.RemoverParalisis == 1) {
        if (target.flags.Paralizado == 1) {
            hechizo_casteado = CanSupportUser(user_index, TargetIndex, true);
            if (!hechizo_casteado) {
                return;
            }

            target.flags.Inmovilizado = 0;
            target.flags.Paralizado = 0;

            ao::net::protocol::WriteParalizeOK(TargetIndex);
            InfoHechizo(user_index);
        }
    }

    // <-------- Remueve Estupidez (Aturdimiento) ---------->
    if (hechizo.RemoverEstupidez == 1) {
        if (target.flags.Estupidez == 1) {
            hechizo_casteado = CanSupportUser(user_index, TargetIndex);
            if (!hechizo_casteado) {
                return;
            }

            target.flags.Estupidez = 0;

            ao::net::protocol::WriteDumbNoMore(TargetIndex);
            ao::net::protocol::FlushBuffer(TargetIndex);
            InfoHechizo(user_index);
        }
    }

    // <-------- Revive ---------->
    if (hechizo.Revivir == 1) {
        if (target.flags.Muerto == 1) {
            if (target.flags.SeguroResu) {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡El espíritu no tiene intenciones de regresar al mundo de los vivos!", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                hechizo_casteado = false;
                return;
            }

            if (target.Pos.Map > 0 && static_cast<std::size_t>(target.Pos.Map) < MapInfoList.size()) {
                if (MapInfoList[target.Pos.Map].ResuSinEfecto > 0) {
                    ao::net::protocol::WriteConsoleMsg(user_index, "¡Revivir no está permitido aquí! Retirate de la Zona si deseas utilizar el Hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                    hechizo_casteado = false;
                    return;
                }
            }

            if (caster.Stats.MaxSta != caster.Stats.MinSta) {
                ao::net::protocol::WriteConsoleMsg(user_index, "No puedes resucitar si no tienes tu barra de energía llena.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                hechizo_casteado = false;
                return;
            }

            // Validaciones de catalizador según clase
            if (caster.clase == eClass::Mage) {
                if (caster.Invent.WeaponEqpObjIndex > 0 && static_cast<std::size_t>(caster.Invent.WeaponEqpObjIndex) < ObjDataList.size()) {
                    if (ObjDataList[caster.Invent.WeaponEqpObjIndex].StaffPower < hechizo.NeedStaff) {
                        ao::net::protocol::WriteConsoleMsg(user_index, "Necesitas un báculo mejor para lanzar este hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                        hechizo_casteado = false;
                        return;
                    }
                }
            } else if (caster.clase == eClass::Bard) {
                if (caster.Invent.AnilloEqpObjIndex != LAUDELFICO && caster.Invent.AnilloEqpObjIndex != LAUDMAGICO) {
                    ao::net::protocol::WriteConsoleMsg(user_index, "Necesitas un instrumento mágico para devolver la vida.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                    hechizo_casteado = false;
                    return;
                }
            } else if (caster.clase == eClass::Druid) {
                if (caster.Invent.AnilloEqpObjIndex != FLAUTAELFICA && caster.Invent.AnilloEqpObjIndex != FLAUTAMAGICA) {
                    ao::net::protocol::WriteConsoleMsg(user_index, "Necesitas un instrumento mágico para devolver la vida.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                    hechizo_casteado = false;
                    return;
                }
            }

            hechizo_casteado = CanSupportUser(user_index, TargetIndex, true);
            if (!hechizo_casteado) {
                return;
            }

            const bool era_criminal = s_callbacks.Criminal ? s_callbacks.Criminal(user_index) : false;
            const bool target_criminal = s_callbacks.Criminal ? s_callbacks.Criminal(TargetIndex) : false;

            if (!target_criminal && TargetIndex != user_index) {
                caster.Reputacion.NobleRep += 500;
                if (caster.Reputacion.NobleRep > MAXREP) {
                    caster.Reputacion.NobleRep = MAXREP;
                }
                ao::net::protocol::WriteConsoleMsg(user_index, "¡Los Dioses te sonríen, has ganado 500 puntos de nobleza!", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            }

            if (era_criminal && (s_callbacks.Criminal && !s_callbacks.Criminal(user_index))) {
                if (s_callbacks.RefreshCharStatus) {
                    s_callbacks.RefreshCharStatus(user_index);
                }
            }

            target.Stats.MinAGU = 0;
            target.flags.Sed = 1;
            target.Stats.MinHam = 0;
            target.flags.Hambre = 1;
            ao::net::protocol::WriteUpdateHungerAndThirst(TargetIndex);
            InfoHechizo(user_index);
            target.Stats.MinMAN = 0;
            target.Stats.MinSta = 0;

            const bool en_arena = s_callbacks.TriggerZonaPelea ? (s_callbacks.TriggerZonaPelea(user_index, TargetIndex) == static_cast<std::int16_t>(eTrigger6::TRIGGER6_PERMITE)) : false;
            if (!en_arena) {
                if (caster.flags.Privilegios & PlayerType::UserPlayer) {
                    caster.Stats.MinHp = static_cast<std::int16_t>(caster.Stats.MinHp * (1.0 - target.Stats.ELV * 0.015));
                }
            }

            // REPLICACIÓN DEL BUG #39:
            // Si la vida cae <= 0, el lanzador muere y hechizo_casteado es false,
            // pero se OMITE el return, completando de todos modos la resurrección del target.
            if (caster.Stats.MinHp <= 0) {
                if (s_callbacks.UserDie) {
                    s_callbacks.UserDie(user_index);
                }
                ao::net::protocol::WriteConsoleMsg(user_index, "El esfuerzo de resucitar fue demasiado grande.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                hechizo_casteado = false;
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, "El esfuerzo de resucitar te ha debilitado.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                hechizo_casteado = true;
            }

            if (target.flags.Traveling == 1) {
                target.Counters.goHome = 0;
                target.flags.Traveling = 0;
                ao::net::protocol::WriteMultiMessage(TargetIndex, static_cast<std::int16_t>(eMessages::CancelHome));
            }

            if (s_callbacks.RevivirUsuario) {
                s_callbacks.RevivirUsuario(TargetIndex);
            }
        } else {
            hechizo_casteado = false;
        }
    }

    // <-------- Agrega Ceguera ---------->
    if (hechizo.Ceguera == 1) {
        if (user_index == TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "No puedes atacarte a vos mismo.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            return;
        }

        if (s_callbacks.PuedeAtacar && !s_callbacks.PuedeAtacar(user_index, TargetIndex)) {
            return;
        }

        if (user_index != TargetIndex && s_callbacks.UsuarioAtacadoPorUsuario) {
            s_callbacks.UsuarioAtacadoPorUsuario(user_index, TargetIndex);
        }

        target.flags.Ceguera = 1;
        // REPLICACIÓN BUG #40: sobreescritura de Counters.Ceguera
        target.Counters.Ceguera = IntervaloParalizado / 3;

        ao::net::protocol::WriteBlind(TargetIndex);
        ao::net::protocol::FlushBuffer(TargetIndex);
        InfoHechizo(user_index);
        hechizo_casteado = true;
    }

    // <-------- Agrega Estupidez (Aturdimiento) ---------->
    if (hechizo.Estupidez == 1) {
        if (user_index == TargetIndex) {
            ao::net::protocol::WriteConsoleMsg(user_index, "No puedes atacarte a vos mismo.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            return;
        }

        if (target.Invent.AnilloEqpObjIndex == SUPERANILLO) {
            ao::net::protocol::WriteConsoleMsg(TargetIndex, " Tu anillo rechaza los efectos del hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            ao::net::protocol::WriteConsoleMsg(user_index, " ¡El hechizo no tiene efecto!", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            ao::net::protocol::FlushBuffer(TargetIndex);
            return;
        }

        if (s_callbacks.PuedeAtacar && !s_callbacks.PuedeAtacar(user_index, TargetIndex)) {
            return;
        }

        if (user_index != TargetIndex && s_callbacks.UsuarioAtacadoPorUsuario) {
            s_callbacks.UsuarioAtacadoPorUsuario(user_index, TargetIndex);
        }

        if (target.flags.Estupidez == 0) {
            target.flags.Estupidez = 1;
            // REPLICACIÓN BUG #40: sobreescritura de Counters.Ceguera con IntervaloParalizado
            target.Counters.Ceguera = IntervaloParalizado;
        }

        ao::net::protocol::WriteDumb(TargetIndex);
        ao::net::protocol::FlushBuffer(TargetIndex);

        InfoHechizo(user_index);
        hechizo_casteado = true;
    }
}

void HechizoEstadoNPC(std::int16_t npc_index, std::int16_t spell_index, bool& hechizo_casteado, std::int16_t user_index) {
    if (npc_index < 1 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }
    if (spell_index < 1 || static_cast<std::size_t>(spell_index) >= Hechizos.size()) {
        return;
    }
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& npc = Npclist[npc_index];
    auto& user = UserList[user_index];
    const auto& hechizo = Hechizos[spell_index];

    if (hechizo.Invisibilidad == 1) {
        InfoHechizo(user_index);
        npc.flags.invisible = 1;
        hechizo_casteado = true;
    }

    if (hechizo.Envenena == 1) {
        if (s_callbacks.PuedeAtacarNPC && !s_callbacks.PuedeAtacarNPC(user_index, npc_index, false)) {
            hechizo_casteado = false;
            return;
        }
        if (s_callbacks.NpcAtacado) {
            s_callbacks.NpcAtacado(npc_index, user_index);
        }
        InfoHechizo(user_index);
        npc.flags.Envenenado = 1;
        hechizo_casteado = true;
    }

    if (hechizo.CuraVeneno == 1) {
        InfoHechizo(user_index);
        npc.flags.Envenenado = 0;
        hechizo_casteado = true;
    }

    if (hechizo.Maldicion == 1) {
        if (s_callbacks.PuedeAtacarNPC && !s_callbacks.PuedeAtacarNPC(user_index, npc_index, false)) {
            hechizo_casteado = false;
            return;
        }
        if (s_callbacks.NpcAtacado) {
            s_callbacks.NpcAtacado(npc_index, user_index);
        }
        InfoHechizo(user_index);
        npc.flags.Maldicion = 1;
        hechizo_casteado = true;
    }

    if (hechizo.RemoverMaldicion == 1) {
        InfoHechizo(user_index);
        npc.flags.Maldicion = 0;
        hechizo_casteado = true;
    }

    if (hechizo.Bendicion == 1) {
        InfoHechizo(user_index);
        npc.flags.Bendicion = 1;
        hechizo_casteado = true;
    }

    if (hechizo.Paraliza == 1) {
        if (npc.flags.AfectaParalisis == 0) {
            if (s_callbacks.PuedeAtacarNPC && !s_callbacks.PuedeAtacarNPC(user_index, npc_index, true)) {
                hechizo_casteado = false;
                return;
            }
            if (s_callbacks.NpcAtacado) {
                s_callbacks.NpcAtacado(npc_index, user_index);
            }
            InfoHechizo(user_index);
            npc.flags.Paralizado = 1;
            npc.flags.Inmovilizado = 0;
            npc.Contadores.Paralisis = IntervaloParalizado;
            hechizo_casteado = true;
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "El NPC es inmune a este hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            hechizo_casteado = false;
            return;
        }
    }

    if (hechizo.RemoverParalisis == 1) {
        if (npc.flags.Paralizado == 1 || npc.flags.Inmovilizado == 1) {
            if (npc.MaestroUser == user_index) {
                InfoHechizo(user_index);
                npc.flags.Paralizado = 0;
                npc.flags.Inmovilizado = 0;
                npc.Contadores.Paralisis = 0;
                hechizo_casteado = true;
            } else {
                if (npc.NPCtype == eNPCType::GuardiaReal) {
                    if (s_callbacks.EsArmada && s_callbacks.EsArmada(user_index)) {
                        InfoHechizo(user_index);
                        npc.flags.Paralizado = 0;
                        npc.flags.Inmovilizado = 0;
                        npc.Contadores.Paralisis = 0;
                        hechizo_casteado = true;
                        return;
                    } else {
                        ao::net::protocol::WriteConsoleMsg(user_index, "Sólo puedes remover la parálisis de los Guardias si perteneces a su facción.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                        hechizo_casteado = false;
                        return;
                    }
                } else if (npc.NPCtype == eNPCType::Guardiascaos) {
                    if (s_callbacks.EsCaos && s_callbacks.EsCaos(user_index)) {
                        InfoHechizo(user_index);
                        npc.flags.Paralizado = 0;
                        npc.flags.Inmovilizado = 0;
                        npc.Contadores.Paralisis = 0;
                        hechizo_casteado = true;
                        return;
                    } else {
                        ao::net::protocol::WriteConsoleMsg(user_index, "Solo puedes remover la parálisis de los Guardias si perteneces a su facción.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                        hechizo_casteado = false;
                        return;
                    }
                } else {
                    ao::net::protocol::WriteConsoleMsg(user_index, "Solo puedes remover la parálisis de los NPCs que te consideren su amo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                    hechizo_casteado = false;
                    return;
                }
            }
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Este NPC no está paralizado", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            hechizo_casteado = false;
            return;
        }
    }

    if (hechizo.Inmoviliza == 1) {
        if (npc.flags.AfectaParalisis == 0) {
            if (s_callbacks.PuedeAtacarNPC && !s_callbacks.PuedeAtacarNPC(user_index, npc_index, true)) {
                hechizo_casteado = false;
                return;
            }
            if (s_callbacks.NpcAtacado) {
                s_callbacks.NpcAtacado(npc_index, user_index);
            }
            npc.flags.Inmovilizado = 1;
            npc.flags.Paralizado = 0;
            npc.Contadores.Paralisis = IntervaloParalizado;
            InfoHechizo(user_index);
            hechizo_casteado = true;
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "El NPC es inmune al hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
        }
    }

    if (hechizo.Mimetiza == 1) {
        if (user.flags.Mimetizado == 1) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Ya te encuentras mimetizado. El hechizo no ha tenido efecto.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            return;
        }

        if (user.flags.AdminInvisible == 1) {
            return;
        }

        if (user.clase == eClass::Druid) {
            user.CharMimetizado.body = user.char_appearance.body;
            user.CharMimetizado.Head = user.char_appearance.Head;
            user.CharMimetizado.CascoAnim = user.char_appearance.CascoAnim;
            user.CharMimetizado.ShieldAnim = user.char_appearance.ShieldAnim;
            user.CharMimetizado.WeaponAnim = user.char_appearance.WeaponAnim;

            user.flags.Mimetizado = 1;

            user.char_appearance.body = npc.char_appearance.body;
            user.char_appearance.Head = npc.char_appearance.Head;
            user.char_appearance.CascoAnim = NingunCasco;
            user.char_appearance.ShieldAnim = NingunEscudo;
            user.char_appearance.WeaponAnim = NingunArma;

            if (user.Invent.AnilloEqpObjIndex == FLAUTAELFICA) {
                user.flags.Ignorado = true;
            }

            if (s_callbacks.ChangeUserChar) {
                s_callbacks.ChangeUserChar(user_index, user.char_appearance.body, user.char_appearance.Head, user.char_appearance.heading, NingunArma, NingunEscudo, NingunCasco);
            }

            InfoHechizo(user_index);
            hechizo_casteado = true;
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Sólo los druidas pueden mimetizarse con criaturas.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
            return;
        }
    }
}

void HechizoTerrenoEstado(std::int16_t user_index, bool& b) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const auto& caster = UserList[user_index];
    const std::int16_t PosCasteadaX = caster.flags.TargetX;
    const std::int16_t PosCasteadaY = caster.flags.TargetY;
    const std::int16_t PosCasteadaM = caster.flags.TargetMap;
    const std::int16_t H = caster.flags.Hechizo;

    if (H < 1 || static_cast<std::size_t>(H) >= Hechizos.size()) {
        return;
    }

    const auto& hechizo = Hechizos[H];
    if (hechizo.RemueveInvisibilidadParcial == 1) {
        b = true;
        for (int temp_x = PosCasteadaX - 8; temp_x <= PosCasteadaX + 8; ++temp_x) {
            for (int temp_y = PosCasteadaY - 8; temp_y <= PosCasteadaY + 8; ++temp_y) {
                if (PosCasteadaM > 0 && temp_x >= 1 && temp_x <= 100 && temp_y >= 1 && temp_y <= 100) {
                    const std::size_t idx = (static_cast<std::size_t>(PosCasteadaM) * 101 + static_cast<std::size_t>(temp_x)) * 101 + static_cast<std::size_t>(temp_y);
                    if (idx < MapData.size()) {
                        const std::int16_t target_user = MapData[idx].UserIndex;
                        if (target_user > 0 && static_cast<std::size_t>(target_user) < UserList.size()) {
                            const auto& target = UserList[target_user];
                            if (target.flags.invisible == 1 && target.flags.AdminInvisible == 0) {
                                auto fx = ao::net::protocol::PrepareMessageCreateFX(target.char_appearance.CharIndex, hechizo.FXgrh, hechizo.loops);
                                ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, user_index, fx);
                            }
                        }
                    }
                }
            }
        }
        InfoHechizo(user_index);
    }
}

void HechizoInvocacion(std::int16_t user_index, bool& hechizo_casteado) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& caster = UserList[user_index];
    const std::int16_t SpellIndex = caster.flags.Hechizo;
    if (SpellIndex < 1 || static_cast<std::size_t>(SpellIndex) >= Hechizos.size()) {
        return;
    }

    const auto& hechizo = Hechizos[SpellIndex];

    // No permitimos se invoquen criaturas en zonas seguras
    bool en_zona_segura = false;
    if (caster.Pos.Map > 0 && static_cast<std::size_t>(caster.Pos.Map) < MapInfoList.size()) {
        if (!MapInfoList[caster.Pos.Map].Pk) {
            en_zona_segura = true;
        }
    }
    const std::size_t b_idx = (static_cast<std::size_t>(caster.Pos.Map) * 101 + static_cast<std::size_t>(caster.Pos.X)) * 101 + static_cast<std::size_t>(caster.Pos.Y);
    if (b_idx < MapData.size() && MapData[b_idx].trigger == eTrigger::ZONASEGURA) {
        en_zona_segura = true;
    }

    if (en_zona_segura) {
        ao::net::protocol::WriteConsoleMsg(user_index, "No puedes invocar criaturas en zona segura.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
        return;
    }

    WorldPos target_pos{};
    target_pos.Map = caster.flags.TargetMap;
    target_pos.X = caster.flags.TargetX;
    target_pos.Y = caster.flags.TargetY;

    // Warp de mascotas
    if (hechizo.Warp == 1) {
        std::int16_t pet_index = s_callbacks.FarthestPet ? s_callbacks.FarthestPet(user_index) : 0;
        if (pet_index > 0 && s_callbacks.WarpMascota) {
            s_callbacks.WarpMascota(user_index, pet_index);
        }
    } else {
        // Invocacion normal
        if (caster.NroMascotas >= MAXMASCOTAS) {
            return;
        }

        for (int i = 1; i <= hechizo.cant; ++i) {
            if (caster.NroMascotas < MAXMASCOTAS) {
                std::int16_t npc_index = s_callbacks.SpawnNpc ? s_callbacks.SpawnNpc(hechizo.NumNpc, target_pos, true, false) : 0;
                if (npc_index > 0 && static_cast<std::size_t>(npc_index) < Npclist.size()) {
                    caster.NroMascotas++;
                    std::int16_t pet_slot = s_callbacks.FreeMascotaIndex ? s_callbacks.FreeMascotaIndex(user_index) : 0;
                    if (pet_slot <= 0) {
                        for (int s = 1; s <= MAXMASCOTAS; ++s) {
                            if (caster.MascotasIndex[s] == 0) {
                                pet_slot = static_cast<std::int16_t>(s);
                                break;
                            }
                        }
                    }

                    if (pet_slot > 0 && pet_slot <= MAXMASCOTAS) {
                        caster.MascotasIndex[pet_slot] = npc_index;
                        caster.MascotasType[pet_slot] = Npclist[npc_index].Numero;
                    }

                    auto& npc_inst = Npclist[npc_index];
                    npc_inst.MaestroUser = user_index;
                    npc_inst.Contadores.TiempoExistencia = IntervaloInvocacion;
                    npc_inst.GiveGLD = 0;

                    if (s_callbacks.FollowAmo) {
                        s_callbacks.FollowAmo(npc_index);
                    }
                } else {
                    return;
                }
            } else {
                break;
            }
        }
    }

    InfoHechizo(user_index);
    hechizo_casteado = true;
}

// ============================================================================
// Fase 4: Orquestación de Casteo, Soporte Legal, Moralidad y Magia de NPCs (G4)
// ============================================================================

void DisNobAuBan(std::int16_t UserIndex, double exp_restada, std::int32_t puntos_bandido) {
    if (s_callbacks.DisNobAuBan) {
        s_callbacks.DisNobAuBan(UserIndex, exp_restada, puntos_bandido);
        return;
    }
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return;
    }

    auto& user = UserList[UserIndex];
    const bool era_criminal = s_callbacks.Criminal ? s_callbacks.Criminal(UserIndex) : (user.Reputacion.NobleRep == 0);

    // Si estamos en la arena no hacemos nada
    const std::size_t b_idx = (static_cast<std::size_t>(user.Pos.Map) * 101 + static_cast<std::size_t>(user.Pos.X)) * 101 + static_cast<std::size_t>(user.Pos.Y);
    if (b_idx < MapData.size() && (MapData[b_idx].trigger == eTrigger::ZONAPELEA || static_cast<std::int32_t>(MapData[b_idx].trigger) == 6)) {
        return;
    }

    const std::uint32_t user_privs = static_cast<std::uint32_t>(user.flags.Privilegios);
    const std::uint32_t mask = static_cast<std::uint32_t>(PlayerType::UserPlayer) | static_cast<std::uint32_t>(PlayerType::Consejero);

    if (user_privs & mask) {
        // Pierdo nobleza...
        const auto noble_pts = static_cast<std::int32_t>(exp_restada);
        user.Reputacion.NobleRep -= noble_pts;
        if (user.Reputacion.NobleRep < 0) {
            user.Reputacion.NobleRep = 0;
        }

        // Gano bandido...
        user.Reputacion.BandidoRep += puntos_bandido;
        if (user.Reputacion.BandidoRep > MAXREP) {
            user.Reputacion.BandidoRep = MAXREP;
        }

        ao::net::protocol::WriteMultiMessage(UserIndex, static_cast<std::int16_t>(eMessages::NobilityLost));

        const bool es_crim = s_callbacks.Criminal ? s_callbacks.Criminal(UserIndex) : (user.Reputacion.NobleRep == 0);
        if (es_crim) {
            if (user.Faccion.ArmadaReal == 1 && s_callbacks.ExpulsarFaccionReal) {
                s_callbacks.ExpulsarFaccionReal(UserIndex);
            }
        }
    }

    const bool ahora_criminal = s_callbacks.Criminal ? s_callbacks.Criminal(UserIndex) : (user.Reputacion.NobleRep == 0);
    if (!era_criminal && ahora_criminal) {
        if (s_callbacks.RefreshCharStatus) {
            s_callbacks.RefreshCharStatus(UserIndex);
        }
    }
}

void HandleHechizoTerreno(std::int16_t UserIndex, std::int16_t SpellIndex) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return;
    }
    if (SpellIndex < 1 || static_cast<std::size_t>(SpellIndex) >= Hechizos.size()) {
        return;
    }

    auto& user = UserList[UserIndex];
    user.flags.Hechizo = SpellIndex;

    bool hechizo_casteado = false;
    const auto& hechizo = Hechizos[SpellIndex];

    switch (hechizo.Tipo) {
        case TipoHechizo::uInvocacion:
            HechizoInvocacion(UserIndex, hechizo_casteado);
            break;
        case TipoHechizo::uEstado:
            HechizoTerrenoEstado(UserIndex, hechizo_casteado);
            break;
        default:
            break;
    }

    if (hechizo_casteado) {
        if (s_callbacks.SubirSkill) {
            s_callbacks.SubirSkill(UserIndex, eSkill::Magia, true);
        }

        std::int32_t mana_requerida = hechizo.ManaRequerido;
        if (hechizo.Warp == 1) {
            // Invocó una mascota -> consume toda la mana
            mana_requerida = user.Stats.MinMAN;
        } else {
            // Quirk 7.4: Bonificación de Druida con Flauta Élfica (30% menos de maná en invocaciones)
            if (user.clase == eClass::Druid) {
                if (user.Invent.AnilloEqpObjIndex == FLAUTAELFICA) {
                    mana_requerida = static_cast<std::int32_t>(mana_requerida * 0.7);
                }
            }
        }

        user.Stats.MinMAN -= static_cast<std::int16_t>(mana_requerida);
        if (user.Stats.MinMAN < 0) {
            user.Stats.MinMAN = 0;
        }

        user.Stats.MinSta -= hechizo.StaRequerido;
        if (user.Stats.MinSta < 0) {
            user.Stats.MinSta = 0;
        }

        ao::net::protocol::WriteUpdateUserStats(UserIndex);
    }
}

void HandleHechizoUsuario(std::int16_t UserIndex, std::int16_t SpellIndex) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return;
    }
    if (SpellIndex < 1 || static_cast<std::size_t>(SpellIndex) >= Hechizos.size()) {
        return;
    }

    auto& user = UserList[UserIndex];
    user.flags.Hechizo = SpellIndex;

    bool hechizo_casteado = false;
    const auto& hechizo = Hechizos[SpellIndex];

    switch (hechizo.Tipo) {
        case TipoHechizo::uEstado:
            HechizoEstadoUsuario(UserIndex, hechizo_casteado);
            break;
        case TipoHechizo::uPropiedades:
            hechizo_casteado = HechizoPropUsuario(UserIndex);
            break;
        default:
            break;
    }

    if (hechizo_casteado) {
        if (s_callbacks.SubirSkill) {
            s_callbacks.SubirSkill(UserIndex, eSkill::Magia, true);
        }

        double mana_requerida = hechizo.ManaRequerido;

        // Quirk 7.4: Bonificaciones para Druida con Flauta Élfica (50% en mimetismo, 10% en magia general excepto Apocalipsis)
        if (user.clase == eClass::Druid) {
            if (user.Invent.AnilloEqpObjIndex == FLAUTAELFICA) {
                if (hechizo.Mimetiza == 1) {
                    mana_requerida *= 0.5;
                } else if (SpellIndex != APOCALIPSIS_SPELL_INDEX) {
                    mana_requerida *= 0.9;
                }
            }
        }

        user.Stats.MinMAN -= static_cast<std::int16_t>(mana_requerida);
        if (user.Stats.MinMAN < 0) {
            user.Stats.MinMAN = 0;
        }

        user.Stats.MinSta -= hechizo.StaRequerido;
        if (user.Stats.MinSta < 0) {
            user.Stats.MinSta = 0;
        }

        ao::net::protocol::WriteUpdateUserStats(UserIndex);
        if (user.flags.TargetUser > 0) {
            ao::net::protocol::WriteUpdateUserStats(user.flags.TargetUser);
            user.flags.TargetUser = 0;
        }
    }
}

void HandleHechizoNPC(std::int16_t UserIndex, std::int16_t SpellIndex) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return;
    }
    if (SpellIndex < 1 || static_cast<std::size_t>(SpellIndex) >= Hechizos.size()) {
        return;
    }

    auto& user = UserList[UserIndex];
    user.flags.Hechizo = SpellIndex;

    bool hechizo_casteado = false;
    const auto target_npc = user.flags.TargetNPC;
    const auto& hechizo = Hechizos[SpellIndex];

    switch (hechizo.Tipo) {
        case TipoHechizo::uEstado:
            HechizoEstadoNPC(target_npc, SpellIndex, hechizo_casteado, UserIndex);
            break;
        case TipoHechizo::uPropiedades:
            HechizoPropNPC(SpellIndex, target_npc, UserIndex, hechizo_casteado);
            break;
        default:
            break;
    }

    if (hechizo_casteado) {
        if (s_callbacks.SubirSkill) {
            s_callbacks.SubirSkill(UserIndex, eSkill::Magia, true);
        }

        double mana_requerida = hechizo.ManaRequerido;

        // Quirk 7.4: Bonificación para Druidas con Flauta Élfica y estado Ignorado
        if (user.clase == eClass::Druid) {
            user.flags.Ignorado = false;
            if (user.Invent.AnilloEqpObjIndex == FLAUTAELFICA) {
                if (hechizo.Mimetiza == 1) {
                    mana_requerida *= 0.5;
                    user.flags.Ignorado = true;
                } else if (SpellIndex != APOCALIPSIS_SPELL_INDEX) {
                    mana_requerida *= 0.9;
                }
            }
        }

        user.Stats.MinMAN -= static_cast<std::int16_t>(mana_requerida);
        if (user.Stats.MinMAN < 0) {
            user.Stats.MinMAN = 0;
        }

        user.Stats.MinSta -= hechizo.StaRequerido;
        if (user.Stats.MinSta < 0) {
            user.Stats.MinSta = 0;
        }

        ao::net::protocol::WriteUpdateUserStats(UserIndex);
        user.flags.TargetNPC = 0;
    }
}

void LanzarHechizo(std::int16_t SpellIndex, std::int16_t UserIndex) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return;
    }
    if (SpellIndex < 1 || static_cast<std::size_t>(SpellIndex) >= Hechizos.size()) {
        return;
    }

    auto& user = UserList[UserIndex];
    user.flags.Hechizo = SpellIndex;

    if (user.flags.EnConsulta) {
        ao::net::protocol::WriteConsoleMsg(UserIndex, "No puedes lanzar hechizos si estás en consulta.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
        return;
    }

    if (PuedeLanzar(UserIndex, SpellIndex)) {
        const auto& hechizo = Hechizos[SpellIndex];
        switch (hechizo.Target) {
            case TargetType::uUsuarios:
                if (user.flags.TargetUser > 0 && static_cast<std::size_t>(user.flags.TargetUser) < UserList.size()) {
                    // Quirk 7.3: Rango evaluado exclusivamente sobre el eje vertical Y
                    if (std::abs(UserList[user.flags.TargetUser].Pos.Y - user.Pos.Y) <= RANGO_VISION_Y) {
                        HandleHechizoUsuario(UserIndex, SpellIndex);
                    } else {
                        ao::net::protocol::WriteConsoleMsg(UserIndex, "Estás demasiado lejos para lanzar este hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_WARNING);
                    }
                } else {
                    ao::net::protocol::WriteConsoleMsg(UserIndex, "Este hechizo actúa sólo sobre usuarios.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                }
                break;

            case TargetType::uNPC:
                if (user.flags.TargetNPC > 0 && static_cast<std::size_t>(user.flags.TargetNPC) < Npclist.size()) {
                    // Quirk 7.3: Rango evaluado exclusivamente sobre el eje vertical Y
                    if (std::abs(Npclist[user.flags.TargetNPC].Pos.Y - user.Pos.Y) <= RANGO_VISION_Y) {
                        HandleHechizoNPC(UserIndex, SpellIndex);
                    } else {
                        ao::net::protocol::WriteConsoleMsg(UserIndex, "Estás demasiado lejos para lanzar este hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_WARNING);
                    }
                } else {
                    ao::net::protocol::WriteConsoleMsg(UserIndex, "Este hechizo sólo afecta a los npcs.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                }
                break;

            case TargetType::uUsuariosYnpc:
                if (user.flags.TargetUser > 0 && static_cast<std::size_t>(user.flags.TargetUser) < UserList.size()) {
                    if (std::abs(UserList[user.flags.TargetUser].Pos.Y - user.Pos.Y) <= RANGO_VISION_Y) {
                        HandleHechizoUsuario(UserIndex, SpellIndex);
                    } else {
                        ao::net::protocol::WriteConsoleMsg(UserIndex, "Estás demasiado lejos para lanzar este hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_WARNING);
                    }
                } else if (user.flags.TargetNPC > 0 && static_cast<std::size_t>(user.flags.TargetNPC) < Npclist.size()) {
                    if (std::abs(Npclist[user.flags.TargetNPC].Pos.Y - user.Pos.Y) <= RANGO_VISION_Y) {
                        HandleHechizoNPC(UserIndex, SpellIndex);
                    } else {
                        ao::net::protocol::WriteConsoleMsg(UserIndex, "Estás demasiado lejos para lanzar este hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_WARNING);
                    }
                } else {
                    ao::net::protocol::WriteConsoleMsg(UserIndex, "Target inválido.", ao::net::protocol::FontTypeNames::FONTTYPE_INFO);
                }
                break;

            case TargetType::uTerreno:
                HandleHechizoTerreno(UserIndex, SpellIndex);
                break;

            default:
                break;
        }
    }

    if (user.Counters.Trabajando > 0) {
        user.Counters.Trabajando--;
    }

    if (user.Counters.Ocultando > 0) {
        user.Counters.Ocultando--;
    }
}

void NpcLanzaSpellSobreUser(std::int16_t NpcIndex, std::int16_t UserIndex, std::int16_t Spell) {
    if (NpcIndex < 1 || static_cast<std::size_t>(NpcIndex) >= Npclist.size() ||
        UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size() ||
        Spell < 1 || static_cast<std::size_t>(Spell) >= Hechizos.size()) {
        return;
    }

    auto& npc = Npclist[NpcIndex];
    auto& user = UserList[UserIndex];

    if (npc.CanAttack == 0) {
        return;
    }
    if (user.flags.invisible == 1 || user.flags.Oculto == 1) {
        return;
    }
    if (user.Pos.Map > 0 && static_cast<std::size_t>(user.Pos.Map) < MapInfoList.size()) {
        if (MapInfoList[user.Pos.Map].MagiaSinEfecto > 0) {
            return;
        }
    }

    npc.CanAttack = 0;
    const auto& hechizo = Hechizos[Spell];
    std::int32_t daño = 0;

    if (hechizo.SubeHP == 1) {
        daño = RandomNumber(hechizo.MinHp, hechizo.MaxHp);

        auto wav = ao::net::protocol::PrepareMessagePlayWave(static_cast<std::uint8_t>(hechizo.WAV), user.Pos.X, user.Pos.Y);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, UserIndex, wav);

        auto fx = ao::net::protocol::PrepareMessageCreateFX(user.char_appearance.CharIndex, hechizo.FXgrh, hechizo.loops);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, UserIndex, fx);

        user.Stats.MinHp += static_cast<std::int16_t>(daño);
        if (user.Stats.MinHp > user.Stats.MaxHp) {
            user.Stats.MinHp = user.Stats.MaxHp;
        }

        // Quirk 7.1: El mensaje legacy indica "te ha quitado X puntos de vida" incluso al curar
        ao::net::protocol::WriteConsoleMsg(UserIndex, npc.name + " te ha quitado " + std::to_string(daño) + " puntos de vida.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
        ao::net::protocol::WriteUpdateUserStats(UserIndex);
    } else if (hechizo.SubeHP == 2) {
        if (user.flags.Privilegios & PlayerType::UserPlayer) {
            daño = RandomNumber(hechizo.MinHp, hechizo.MaxHp);

            if (user.Invent.CascoEqpObjIndex > 0 && static_cast<std::size_t>(user.Invent.CascoEqpObjIndex) < ObjDataList.size()) {
                const auto& casco = ObjDataList[user.Invent.CascoEqpObjIndex];
                daño -= RandomNumber(casco.DefensaMagicaMin, casco.DefensaMagicaMax);
            }

            if (user.Invent.AnilloEqpObjIndex > 0 && static_cast<std::size_t>(user.Invent.AnilloEqpObjIndex) < ObjDataList.size()) {
                const auto& anillo = ObjDataList[user.Invent.AnilloEqpObjIndex];
                daño -= RandomNumber(anillo.DefensaMagicaMin, anillo.DefensaMagicaMax);
            }

            if (daño < 0) {
                daño = 0;
            }

            auto wav = ao::net::protocol::PrepareMessagePlayWave(static_cast<std::uint8_t>(hechizo.WAV), user.Pos.X, user.Pos.Y);
            ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, UserIndex, wav);

            auto fx = ao::net::protocol::PrepareMessageCreateFX(user.char_appearance.CharIndex, hechizo.FXgrh, hechizo.loops);
            ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, UserIndex, fx);

            user.Stats.MinHp -= static_cast<std::int16_t>(daño);

            ao::net::protocol::WriteConsoleMsg(UserIndex, npc.name + " te ha quitado " + std::to_string(daño) + " puntos de vida.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
            ao::net::protocol::WriteUpdateUserStats(UserIndex);

            if (user.Stats.MinHp < 1) {
                user.Stats.MinHp = 0;
                if (npc.NPCtype == eNPCType::GuardiaReal) {
                    if (s_callbacks.RestarCriminalidad) {
                        s_callbacks.RestarCriminalidad(UserIndex);
                    }
                }
                if (s_callbacks.UserDie) {
                    s_callbacks.UserDie(UserIndex);
                }
                if (npc.MaestroUser > 0) {
                    if (s_callbacks.StoreFrag) {
                        s_callbacks.StoreFrag(npc.MaestroUser, UserIndex);
                    }
                    if (s_callbacks.ContarMuerte) {
                        s_callbacks.ContarMuerte(UserIndex, npc.MaestroUser);
                    }
                    if (s_callbacks.ActStats) {
                        s_callbacks.ActStats(UserIndex, npc.MaestroUser);
                    }
                }
            }
        }
    }

    if (hechizo.Paraliza == 1 || hechizo.Inmoviliza == 1) {
        if (user.flags.Paralizado == 0) {
            auto wav = ao::net::protocol::PrepareMessagePlayWave(static_cast<std::uint8_t>(hechizo.WAV), user.Pos.X, user.Pos.Y);
            ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, UserIndex, wav);

            auto fx = ao::net::protocol::PrepareMessageCreateFX(user.char_appearance.CharIndex, hechizo.FXgrh, hechizo.loops);
            ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, UserIndex, fx);

            if (user.Invent.AnilloEqpObjIndex == SUPERANILLO) {
                ao::net::protocol::WriteConsoleMsg(UserIndex, " Tu anillo rechaza los efectos del hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
                return;
            }

            if (hechizo.Inmoviliza == 1) {
                user.flags.Inmovilizado = 1;
            }

            user.flags.Paralizado = 1;
            user.Counters.Paralisis = IntervaloParalizado;

            ao::net::protocol::WriteParalizeOK(UserIndex);
        }
    }

    if (hechizo.Estupidez == 1) {
        if (user.flags.Estupidez == 0) {
            auto wav = ao::net::protocol::PrepareMessagePlayWave(static_cast<std::uint8_t>(hechizo.WAV), user.Pos.X, user.Pos.Y);
            ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, UserIndex, wav);

            auto fx = ao::net::protocol::PrepareMessageCreateFX(user.char_appearance.CharIndex, hechizo.FXgrh, hechizo.loops);
            ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, UserIndex, fx);

            if (user.Invent.AnilloEqpObjIndex == SUPERANILLO) {
                ao::net::protocol::WriteConsoleMsg(UserIndex, " Tu anillo rechaza los efectos del hechizo.", ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT);
                return;
            }

            user.flags.Estupidez = 1;
            // Quirk 7.2: Estupidez asigna IntervaloInvisible en Counters.Ceguera
            user.Counters.Ceguera = IntervaloInvisible;

            ao::net::protocol::WriteDumb(UserIndex);
        }
    }
}

void NpcLanzaSpellSobreNpc(std::int16_t NpcIndex, std::int16_t TargetNPC, std::int16_t Spell) {
    if (NpcIndex < 1 || static_cast<std::size_t>(NpcIndex) >= Npclist.size() ||
        TargetNPC < 1 || static_cast<std::size_t>(TargetNPC) >= Npclist.size() ||
        Spell < 1 || static_cast<std::size_t>(Spell) >= Hechizos.size()) {
        return;
    }

    auto& npc = Npclist[NpcIndex];
    auto& target = Npclist[TargetNPC];

    if (npc.CanAttack == 0) {
        return;
    }
    npc.CanAttack = 0;

    const auto& hechizo = Hechizos[Spell];
    if (hechizo.SubeHP == 2) {
        std::int32_t daño = RandomNumber(hechizo.MinHp, hechizo.MaxHp);

        auto wav = ao::net::protocol::PrepareMessagePlayWave(static_cast<std::uint8_t>(hechizo.WAV), target.Pos.X, target.Pos.Y);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToNPCArea, TargetNPC, wav);

        auto fx = ao::net::protocol::PrepareMessageCreateFX(target.char_appearance.CharIndex, hechizo.FXgrh, hechizo.loops);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToNPCArea, TargetNPC, fx);

        target.Stats.MinHp -= static_cast<std::int16_t>(daño);

        if (target.Stats.MinHp < 1) {
            target.Stats.MinHp = 0;
            const std::int16_t killer = npc.MaestroUser > 0 ? npc.MaestroUser : 0;
            if (s_callbacks.MuereNpc) {
                s_callbacks.MuereNpc(TargetNPC, killer);
            }
        }
    }
}

} // namespace ao

