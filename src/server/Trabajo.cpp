#include "Trabajo.hpp"
#include "InvUsuario.hpp"
#include "Modulo_InventANDobj.hpp"
#include "FileIO.hpp"
#include "Matematicas.hpp"
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace Trabajo {

using ao::net::protocol::FontTypeNames;

static TrabajoHooks g_hooks;

void SetHooks(const TrabajoHooks& hooks) {
    g_hooks = hooks;
}

void ResetHooks() {
    g_hooks = TrabajoHooks{};
}

static std::int32_t GetRandomNumber(std::int32_t min, std::int32_t max) {
    if (g_hooks.RandomNumber) {
        return g_hooks.RandomNumber(min, max);
    }
    return ::RandomNumber(min, max);
}

static bool PutItemInInv(std::int16_t user_index, const Obj& item) {
    if (g_hooks.MeterItemEnInventario) {
        return g_hooks.MeterItemEnInventario(user_index, item);
    }
    Obj copy_item = item;
    return InvUsuario::MeterItemEnInventario(user_index, copy_item);
}

static void DropItemToFloor(const WorldPos& pos, const Obj& item) {
    if (g_hooks.TirarItemAlPiso) {
        g_hooks.TirarItemAlPiso(pos, item);
    } else {
        Modulo_InventANDobj::TirarItemAlPiso(pos, item);
    }
}

static void TriggerSubirSkill(std::int16_t user_index, std::int16_t skill_index, bool exito) {
    if (g_hooks.SubirSkill) {
        g_hooks.SubirSkill(user_index, skill_index, exito);
    }
}

float ModNavegacion(eClass clase, std::int16_t user_index) {
    switch (clase) {
        case eClass::Pirat:
            return 1.0f;
        case eClass::Worker:
            if (UserList[user_index].Stats.UserSkills[static_cast<std::size_t>(eSkill::Pesca)] == 100) {
                return 1.71f;
            } else {
                return 2.0f;
            }
        default:
            return 2.0f;
    }
}

float ModFundicion(eClass clase) {
    switch (clase) {
        case eClass::Worker:
            return 1.0f;
        default:
            return 3.0f;
    }
}

std::int16_t ModCarpinteria(eClass clase) {
    switch (clase) {
        case eClass::Worker:
            return 1;
        default:
            return 3;
    }
}

float ModHerreriA(eClass clase) {
    switch (clase) {
        case eClass::Worker:
            return 1.0f;
        default:
            return 3.0f;
    }
}

std::int16_t ModDomar(eClass clase) {
    switch (clase) {
        case eClass::Worker:
        case eClass::Hunter:
        case eClass::Druid:
            return 1;
        default:
            return 2;
    }
}

std::int16_t MaxItemsConstruibles(std::int16_t user_index) {
    auto val = static_cast<std::int16_t>(std::nearbyint((UserList[user_index].Stats.ELV - 4.0) / 5.0));
    return std::max<std::int16_t>(1, val);
}

void QuitarSta(std::int16_t user_index, std::int16_t cantidad) {
    UserList[user_index].Stats.MinSta -= cantidad;
    if (UserList[user_index].Stats.MinSta < 0) {
        UserList[user_index].Stats.MinSta = 0;
    }
    if (g_hooks.WriteUpdateSta) {
        g_hooks.WriteUpdateSta(user_index);
    }
}

bool TieneObjetos(std::int16_t item_index, std::int16_t cant, std::int16_t user_index) {
    std::int32_t total = 0;
    for (std::int16_t i = 1; i <= UserList[user_index].CurrentInventorySlots; ++i) {
        if (UserList[user_index].Invent.Object[i].ObjIndex == item_index) {
            total += UserList[user_index].Invent.Object[i].Amount;
        }
    }
    return static_cast<std::int32_t>(cant) <= total;
}

void QuitarObjetos(std::int16_t item_index, std::int16_t cant, std::int16_t user_index) {
    for (std::int16_t i = 1; i <= UserList[user_index].CurrentInventorySlots; ++i) {
        auto& slotObj = UserList[user_index].Invent.Object[i];
        if (slotObj.ObjIndex == item_index) {
            if (slotObj.Amount <= cant && slotObj.Equipped == 1) {
                if (g_hooks.Desequipar) {
                    g_hooks.Desequipar(user_index, static_cast<std::uint8_t>(i));
                } else {
                    InvUsuario::Desequipar(user_index, static_cast<std::uint8_t>(i));
                }
            }

            slotObj.Amount -= cant;
            if (slotObj.Amount <= 0) {
                cant = static_cast<std::int16_t>(std::abs(slotObj.Amount));
                UserList[user_index].Invent.NroItems--;
                slotObj.Amount = 0;
                slotObj.ObjIndex = 0;
            } else {
                cant = 0;
            }
            InvUsuario::UpdateUserInv(false, user_index, static_cast<std::uint8_t>(i));
            if (cant == 0) {
                return;
            }
        }
    }
}

bool HerreroTieneMateriales(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items) {
    const auto& obj = ObjDataList[item_index];
    if (obj.LingH > 0) {
        if (!TieneObjetos(LingoteHierro, static_cast<std::int16_t>(obj.LingH * cantidad_items), user_index)) return false;
    }
    if (obj.LingP > 0) {
        if (!TieneObjetos(LingotePlata, static_cast<std::int16_t>(obj.LingP * cantidad_items), user_index)) return false;
    }
    if (obj.LingO > 0) {
        if (!TieneObjetos(LingoteOro, static_cast<std::int16_t>(obj.LingO * cantidad_items), user_index)) return false;
    }
    return true;
}

void HerreroQuitarMateriales(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items) {
    const auto& obj = ObjDataList[item_index];
    if (obj.LingH > 0) QuitarObjetos(LingoteHierro, static_cast<std::int16_t>(obj.LingH * cantidad_items), user_index);
    if (obj.LingP > 0) QuitarObjetos(LingotePlata, static_cast<std::int16_t>(obj.LingP * cantidad_items), user_index);
    if (obj.LingO > 0) QuitarObjetos(LingoteOro, static_cast<std::int16_t>(obj.LingO * cantidad_items), user_index);
}

bool PuedeConstruirHerreria(std::int16_t item_index) {
    for (std::size_t i = 1; i < ArmasHerrero.size(); ++i) {
        if (ArmasHerrero[i] == item_index) return true;
    }
    for (std::size_t i = 1; i < ArmadurasHerrero.size(); ++i) {
        if (ArmadurasHerrero[i] == item_index) return true;
    }
    return false;
}

bool PuedeConstruir(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items) {
    if (!HerreroTieneMateriales(user_index, item_index, cantidad_items)) return false;
    float mod = ModHerreriA(UserList[user_index].clase);
    float skillDiv = static_cast<float>(UserList[user_index].Stats.UserSkills[static_cast<std::size_t>(eSkill::Herreria)]) / mod;
    auto skillRound = static_cast<std::int16_t>(std::round(skillDiv));
    return skillRound >= ObjDataList[item_index].SkHerreria;
}

bool CarpinteroTieneMateriales(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad, bool show_msg) {
    const auto& obj = ObjDataList[item_index];
    if (obj.Madera > 0) {
        if (!TieneObjetos(Leña, static_cast<std::int16_t>(obj.Madera * cantidad), user_index)) {
            if (show_msg && g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "No tienes suficiente madera.", FontTypeNames::FONTTYPE_INFO);
            }
            return false;
        }
    }
    if (obj.MaderaElfica > 0) {
        if (!TieneObjetos(LeñaElfica, static_cast<std::int16_t>(obj.MaderaElfica * cantidad), user_index)) {
            if (show_msg && g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "No tienes suficiente madera elfica.", FontTypeNames::FONTTYPE_INFO);
            }
            return false;
        }
    }
    return true;
}

void CarpinteroQuitarMateriales(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items) {
    const auto& obj = ObjDataList[item_index];
    if (obj.Madera > 0) QuitarObjetos(Leña, static_cast<std::int16_t>(obj.Madera * cantidad_items), user_index);
    if (obj.MaderaElfica > 0) QuitarObjetos(LeñaElfica, static_cast<std::int16_t>(obj.MaderaElfica * cantidad_items), user_index);
}

bool PuedeConstruirCarpintero(std::int16_t item_index) {
    for (std::size_t i = 1; i < ObjCarpintero.size(); ++i) {
        if (ObjCarpintero[i] == item_index) return true;
    }
    return false;
}

bool TieneMaterialesUpgrade(std::int16_t user_index, std::int16_t item_index) {
    const auto& obj = ObjDataList[item_index];
    if (obj.LingH > 0 && !TieneObjetos(LingoteHierro, obj.LingH, user_index)) return false;
    if (obj.LingP > 0 && !TieneObjetos(LingotePlata, obj.LingP, user_index)) return false;
    if (obj.LingO > 0 && !TieneObjetos(LingoteOro, obj.LingO, user_index)) return false;
    if (obj.Madera > 0 && !TieneObjetos(Leña, obj.Madera, user_index)) return false;
    if (obj.MaderaElfica > 0 && !TieneObjetos(LeñaElfica, obj.MaderaElfica, user_index)) return false;
    if (!TieneObjetos(item_index, 1, user_index)) return false;
    return true;
}

void QuitarMaterialesUpgrade(std::int16_t user_index, std::int16_t item_index) {
    const auto& obj = ObjDataList[item_index];
    if (obj.LingH > 0) QuitarObjetos(LingoteHierro, obj.LingH, user_index);
    if (obj.LingP > 0) QuitarObjetos(LingotePlata, obj.LingP, user_index);
    if (obj.LingO > 0) QuitarObjetos(LingoteOro, obj.LingO, user_index);
    if (obj.Madera > 0) QuitarObjetos(Leña, obj.Madera, user_index);
    if (obj.MaderaElfica > 0) QuitarObjetos(LeñaElfica, obj.MaderaElfica, user_index);
    QuitarObjetos(item_index, 1, user_index);
}

// G2 — Recolección de Recursos

void DoTalar(std::int16_t user_index, bool dar_madera_elfica) {
    auto& user = UserList[user_index];
    if (user.clase == eClass::Worker) {
        QuitarSta(user_index, EsfuerzoTalarLeñador);
    } else {
        QuitarSta(user_index, EsfuerzoTalarGeneral);
    }

    std::int32_t skill = user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Talar)];
    auto suerte = static_cast<std::int32_t>(-0.00125 * skill * skill - 0.3 * skill + 49);

    std::int32_t res = GetRandomNumber(1, suerte);

    if (res <= 6) {
        Obj mi_obj;
        if (user.clase == eClass::Worker) {
            std::int16_t cantidad_max = static_cast<std::int16_t>(1 + MaxItemsConstruibles(user_index));
            mi_obj.Amount = static_cast<std::int16_t>(GetRandomNumber(1, cantidad_max));
        } else {
            mi_obj.Amount = 1;
        }

        mi_obj.ObjIndex = dar_madera_elfica ? LeñaElfica : Leña;

        if (!PutItemInInv(user_index, mi_obj)) {
            DropItemToFloor(user.Pos, mi_obj);
        }

        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "¡Has conseguido algo de leña!", FontTypeNames::FONTTYPE_INFO);
        }

        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Talar), true);
    } else {
        if (user.flags.UltimoMensaje != 8) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "¡No has obtenido leña!", FontTypeNames::FONTTYPE_INFO);
            }
            user.flags.UltimoMensaje = 8;
        }
        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Talar), false);
    }

    user.Reputacion.PlebeRep += vlProleta;
    if (user.Reputacion.PlebeRep > MAXREP) {
        user.Reputacion.PlebeRep = MAXREP;
    }

    user.Counters.Trabajando++;
}

void DoMineria(std::int16_t user_index) {
    auto& user = UserList[user_index];
    if (user.clase == eClass::Worker) {
        QuitarSta(user_index, EsfuerzoExcavarMinero);
    } else {
        QuitarSta(user_index, EsfuerzoExcavarGeneral);
    }

    std::int32_t skill = user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Mineria)];
    auto suerte = static_cast<std::int32_t>(-0.00125 * skill * skill - 0.3 * skill + 49);

    std::int32_t res = GetRandomNumber(1, suerte);

    if (res <= 5) {
        if (user.flags.TargetObj == 0) return;

        Obj mi_obj;
        mi_obj.ObjIndex = ObjDataList[user.flags.TargetObj].MineralIndex;

        if (user.clase == eClass::Worker) {
            std::int16_t cantidad_max = static_cast<std::int16_t>(1 + MaxItemsConstruibles(user_index));
            mi_obj.Amount = static_cast<std::int16_t>(GetRandomNumber(1, cantidad_max));
        } else {
            mi_obj.Amount = 1;
        }

        if (!PutItemInInv(user_index, mi_obj)) {
            DropItemToFloor(user.Pos, mi_obj);
        }

        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "¡Has extraido algunos minerales!", FontTypeNames::FONTTYPE_INFO);
        }

        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Mineria), true);
    } else {
        if (user.flags.UltimoMensaje != 9) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "¡No has conseguido nada!", FontTypeNames::FONTTYPE_INFO);
            }
            user.flags.UltimoMensaje = 9;
        }
        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Mineria), false);
    }

    user.Reputacion.PlebeRep += vlProleta;
    if (user.Reputacion.PlebeRep > MAXREP) {
        user.Reputacion.PlebeRep = MAXREP;
    }

    user.Counters.Trabajando++;
}

void DoPescar(std::int16_t user_index) {
    auto& user = UserList[user_index];
    if (user.clase == eClass::Worker) {
        QuitarSta(user_index, EsfuerzoPescarPescador);
    } else {
        QuitarSta(user_index, EsfuerzoPescarGeneral);
    }

    std::int32_t skill = user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Pesca)];
    auto suerte = static_cast<std::int32_t>(-0.00125 * skill * skill - 0.3 * skill + 49);

    std::int32_t res = GetRandomNumber(1, suerte);

    if (res <= 6) {
        Obj mi_obj;
        if (user.clase == eClass::Worker) {
            std::int16_t cantidad_max = static_cast<std::int16_t>(1 + MaxItemsConstruibles(user_index));
            mi_obj.Amount = static_cast<std::int16_t>(GetRandomNumber(1, cantidad_max));
        } else {
            mi_obj.Amount = 1;
        }

        mi_obj.ObjIndex = Pescado;

        if (!PutItemInInv(user_index, mi_obj)) {
            DropItemToFloor(user.Pos, mi_obj);
        }

        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "¡Has pescado un lindo pez!", FontTypeNames::FONTTYPE_INFO);
        }

        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Pesca), true);
    } else {
        if (user.flags.UltimoMensaje != 6) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "¡No has pescado nada!", FontTypeNames::FONTTYPE_INFO);
            }
            user.flags.UltimoMensaje = 6;
        }
        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Pesca), false);
    }

    user.Reputacion.PlebeRep += vlProleta;
    if (user.Reputacion.PlebeRep > MAXREP) {
        user.Reputacion.PlebeRep = MAXREP;
    }

    user.Counters.Trabajando++;
}

void DoPescarRed(std::int16_t user_index) {
    auto& user = UserList[user_index];
    bool es_pescador = false;

    if (user.clase == eClass::Worker) {
        QuitarSta(user_index, EsfuerzoPescarPescador);
        es_pescador = true;
    } else {
        QuitarSta(user_index, EsfuerzoPescarGeneral);
        es_pescador = false;
    }

    std::int32_t skill = user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Pesca)];
    auto suerte = static_cast<std::int32_t>(-0.00125 * skill * skill - 0.3 * skill + 49);

    if (suerte > 0) {
        std::int32_t res = GetRandomNumber(1, suerte);

        if (res < 6) {
            Obj mi_obj;
            std::int16_t peces_posibles[4] = {PESCADO1, PESCADO2, PESCADO3, PESCADO4};

            if (es_pescador) {
                mi_obj.Amount = static_cast<std::int16_t>(GetRandomNumber(1, 5));
            } else {
                mi_obj.Amount = 1;
            }

            std::int32_t idx_pez = GetRandomNumber(0, 3);
            mi_obj.ObjIndex = peces_posibles[idx_pez];

            if (!PutItemInInv(user_index, mi_obj)) {
                DropItemToFloor(user.Pos, mi_obj);
            }

            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "¡Has pescado algunos peces!", FontTypeNames::FONTTYPE_INFO);
            }

            TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Pesca), true);
        } else {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "¡No has pescado nada!", FontTypeNames::FONTTYPE_INFO);
            }
            TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Pesca), false);
        }
    }

    user.Reputacion.PlebeRep += vlProleta;
    if (user.Reputacion.PlebeRep > MAXREP) {
        user.Reputacion.PlebeRep = MAXREP;
    }
}

// G3 — Manufactura, Fundición y Upgrades

std::int16_t MineralesParaLingote(iMinerales lingote) {
    switch (lingote) {
        case iMinerales::HierroCrudo:
        case iMinerales::LingoteDeHierro:
            return MineralHierro;
        case iMinerales::PlataCruda:
        case iMinerales::LingoteDePlata:
            return MineralPlata;
        case iMinerales::OroCrudo:
        case iMinerales::LingoteDeOro:
            return MineralOro;
        default:
            return 0;
    }
}

void DoLingotes(std::int16_t user_index) {
    auto& user = UserList[user_index];
    if (user.flags.TargetObjInvIndex <= 0) return;

    std::int16_t target_item = user.flags.TargetObjInvIndex;
    if (ObjDataList[target_item].OBJType != eOBJType::otMinerales) return;

    std::int16_t lingote_index = 0;
    iMinerales lingote_enum = iMinerales::HierroCrudo;

    if (target_item == MineralHierro) {
        lingote_index = LingoteHierro;
        lingote_enum = iMinerales::HierroCrudo;
    } else if (target_item == MineralPlata) {
        lingote_index = LingotePlata;
        lingote_enum = iMinerales::PlataCruda;
    } else if (target_item == MineralOro) {
        lingote_index = LingoteOro;
        lingote_enum = iMinerales::OroCrudo;
    } else {
        return;
    }

    std::int16_t mineral_index = MineralesParaLingote(lingote_enum);
    std::uint8_t slot = user.flags.TargetObjInvSlot;

    if (user.Invent.Object[slot].ObjIndex == mineral_index) {
        if (user.Invent.Object[slot].Amount >= 50) {
            user.Invent.Object[slot].Amount -= 50;
            if (user.Invent.Object[slot].Amount < 1) {
                user.Invent.Object[slot].Amount = 0;
                user.Invent.Object[slot].ObjIndex = 0;
                user.Invent.NroItems--;
            }

            Obj mi_obj;
            mi_obj.Amount = 1;
            mi_obj.ObjIndex = lingote_index;

            if (!PutItemInInv(user_index, mi_obj)) {
                DropItemToFloor(user.Pos, mi_obj);
            }

            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "¡Has fundido el mineral y hecho un lingote!", FontTypeNames::FONTTYPE_INFO);
            }
            InvUsuario::UpdateUserInv(true, user_index, slot);
            TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Mineria), true);
        } else {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "Necesitas por lo menos 50 minerales para hacer un lingote.", FontTypeNames::FONTTYPE_INFO);
            }
        }
    }
}

void FundirMineral(std::int16_t user_index) {
    auto& user = UserList[user_index];
    if (user.flags.TargetObjInvIndex <= 0) return;

    const auto& obj = ObjDataList[user.flags.TargetObjInvIndex];
    if (obj.OBJType == eOBJType::otMinerales) {
        float mod = ModFundicion(user.clase);
        float skill_div = static_cast<float>(user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Mineria)]) / mod;
        if (static_cast<float>(obj.MinSkill) <= skill_div) {
            DoLingotes(user_index);
        } else {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "No tienes conocimientos de minería suficientes para trabajar este mineral.", FontTypeNames::FONTTYPE_INFO);
            }
        }
    }
}

void FundirArmas(std::int16_t user_index) {
    auto& user = UserList[user_index];
    if (user.flags.TargetObjInvIndex <= 0) return;

    const auto& obj = ObjDataList[user.flags.TargetObjInvIndex];
    if (obj.OBJType == eOBJType::otWeapon) {
        float mod = ModHerreriA(user.clase);
        float skill_div = static_cast<float>(user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Herreria)]) / mod;
        if (static_cast<float>(obj.SkHerreria) <= skill_div) {
            DoFundir(user_index);
        } else {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "No tienes los conocimientos suficientes en herrería para fundir este objeto.", FontTypeNames::FONTTYPE_INFO);
            }
        }
    }
}

void DoFundir(std::int16_t user_index) {
    auto& user = UserList[user_index];
    std::uint8_t slot = user.flags.TargetObjInvSlot;
    if (slot < 1 || slot > user.CurrentInventorySlots) return;

    auto& slot_obj = user.Invent.Object[slot];
    if (slot_obj.Equipped == 1) {
        if (g_hooks.Desequipar) {
            g_hooks.Desequipar(user_index, slot);
        } else {
            InvUsuario::Desequipar(user_index, slot);
        }
    }

    slot_obj.Amount--;
    if (slot_obj.Amount < 1) {
        slot_obj.Amount = 0;
        slot_obj.ObjIndex = 0;
        user.Invent.NroItems--;
    }

    std::int32_t num = GetRandomNumber(10, 25);
    const auto& item_obj = ObjDataList[user.flags.TargetObjInvIndex];

    std::int16_t lingotes[3];
    lingotes[0] = static_cast<std::int16_t>((static_cast<std::int32_t>(item_obj.LingH) * num) * 0.01);
    lingotes[1] = static_cast<std::int16_t>((static_cast<std::int32_t>(item_obj.LingP) * num) * 0.01);
    lingotes[2] = static_cast<std::int16_t>((static_cast<std::int32_t>(item_obj.LingO) * num) * 0.01);

    for (int i = 0; i < 3; ++i) {
        if (lingotes[i] > 0) {
            Obj mi_obj;
            mi_obj.Amount = lingotes[i];
            mi_obj.ObjIndex = static_cast<std::int16_t>(LingoteHierro + i);

            if (!PutItemInInv(user_index, mi_obj)) {
                DropItemToFloor(user.Pos, mi_obj);
            }
            InvUsuario::UpdateUserInv(true, user_index, slot);
        }
    }

    if (g_hooks.WriteConsoleMsg) {
        std::string msg = "¡Has obtenido el " + std::to_string(num) + "% de los lingotes utilizados para la construcción del objeto!";
        g_hooks.WriteConsoleMsg(user_index, msg, FontTypeNames::FONTTYPE_INFO);
    }

    user.Counters.Trabajando++;
}

void HerreroConstruirItem(std::int16_t user_index, std::int16_t item_index) {
    auto& user = UserList[user_index];
    std::int16_t cantidad_items = user.Construir.PorCiclo;

    if (user.Construir.Cantidad < cantidad_items) {
        cantidad_items = user.Construir.Cantidad;
    }

    if (user.Construir.Cantidad > 0) {
        user.Construir.Cantidad -= cantidad_items;
    }

    if (cantidad_items == 0) {
        if (g_hooks.WriteStopWorking) {
            g_hooks.WriteStopWorking(user_index);
        }
        return;
    }

    bool tiene_materiales = false;
    if (PuedeConstruirHerreria(item_index)) {
        while (cantidad_items > 0 && !tiene_materiales) {
            if (PuedeConstruir(user_index, item_index, cantidad_items)) {
                tiene_materiales = true;
            } else {
                cantidad_items--;
            }
        }

        if (!tiene_materiales) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "No tienes suficientes materiales.", FontTypeNames::FONTTYPE_INFO);
            }
            if (g_hooks.WriteStopWorking) {
                g_hooks.WriteStopWorking(user_index);
            }
            return;
        }

        std::int16_t gasto_sta = (user.clase == eClass::Worker) ? GASTO_ENERGIA_TRABAJADOR : GASTO_ENERGIA_NO_TRABAJADOR;
        if (user.Stats.MinSta >= gasto_sta) {
            QuitarSta(user_index, gasto_sta);
        } else {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "No tienes suficiente energía.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }

        HerreroQuitarMateriales(user_index, item_index, cantidad_items);

        if (g_hooks.WriteConsoleMsg) {
            std::string msg;
            const auto& item_type = ObjDataList[item_index].OBJType;
            if (item_type == eOBJType::otWeapon) {
                msg = (cantidad_items > 1) ? ("Has construido " + std::to_string(cantidad_items) + " armas!") : "Has construido el arma!";
            } else if (item_type == eOBJType::otESCUDO) {
                msg = (cantidad_items > 1) ? ("Has construido " + std::to_string(cantidad_items) + " escudos!") : "Has construido el escudo!";
            } else if (item_type == eOBJType::otCASCO) {
                msg = (cantidad_items > 1) ? ("Has construido " + std::to_string(cantidad_items) + " cascos!") : "Has construido el casco!";
            } else if (item_type == eOBJType::otArmadura) {
                msg = (cantidad_items > 1) ? ("Has construido " + std::to_string(cantidad_items) + " armaduras") : "Has construido la armadura!";
            }
            if (!msg.empty()) {
                g_hooks.WriteConsoleMsg(user_index, msg, FontTypeNames::FONTTYPE_INFO);
            }
        }

        Obj mi_obj;
        mi_obj.Amount = cantidad_items;
        mi_obj.ObjIndex = item_index;

        if (!PutItemInInv(user_index, mi_obj)) {
            DropItemToFloor(user.Pos, mi_obj);
        }

        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Herreria), true);
        InvUsuario::UpdateUserInv(true, user_index, 0);

        if (g_hooks.SendAreaWave) {
            g_hooks.SendAreaWave(user_index, MARTILLOHERRERO);
        }

        user.Reputacion.PlebeRep += vlProleta;
        if (user.Reputacion.PlebeRep > MAXREP) {
            user.Reputacion.PlebeRep = MAXREP;
        }
        user.Counters.Trabajando++;
    }
}

void CarpinteroConstruirItem(std::int16_t user_index, std::int16_t item_index) {
    auto& user = UserList[user_index];
    std::int16_t cantidad_items = user.Construir.PorCiclo;

    if (user.Construir.Cantidad < cantidad_items) {
        cantidad_items = user.Construir.Cantidad;
    }

    if (user.Construir.Cantidad > 0) {
        user.Construir.Cantidad -= cantidad_items;
    }

    if (cantidad_items == 0) {
        if (g_hooks.WriteStopWorking) {
            g_hooks.WriteStopWorking(user_index);
        }
        return;
    }

    bool tiene_materiales = false;
    if (PuedeConstruirCarpintero(item_index)) {
        while (cantidad_items > 0 && !tiene_materiales) {
            if (CarpinteroTieneMateriales(user_index, item_index, cantidad_items)) {
                float mod = static_cast<float>(ModCarpinteria(user.clase));
                float skill_div = static_cast<float>(user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Carpinteria)]) / mod;
                if (static_cast<std::int16_t>(std::round(skill_div)) >= ObjDataList[item_index].SkCarpinteria) {
                    tiene_materiales = true;
                } else {
                    cantidad_items--;
                }
            } else {
                cantidad_items--;
            }
        }

        if (!tiene_materiales) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "No tienes suficientes materiales.", FontTypeNames::FONTTYPE_INFO);
            }
            if (g_hooks.WriteStopWorking) {
                g_hooks.WriteStopWorking(user_index);
            }
            return;
        }

        std::int16_t gasto_sta = (user.clase == eClass::Worker) ? GASTO_ENERGIA_TRABAJADOR : GASTO_ENERGIA_NO_TRABAJADOR;
        if (user.Stats.MinSta >= gasto_sta) {
            QuitarSta(user_index, gasto_sta);
        } else {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "No tienes suficiente energía.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }

        CarpinteroQuitarMateriales(user_index, item_index, cantidad_items);

        if (g_hooks.WriteConsoleMsg) {
            std::string msg;
            const auto& item_type = ObjDataList[item_index].OBJType;
            if (item_type == eOBJType::otFlechas) {
                msg = (cantidad_items > 1) ? ("Has construido " + std::to_string(cantidad_items) + " flechas!") : "Has construido la flecha!";
            } else if (item_type == eOBJType::otWeapon) {
                msg = (cantidad_items > 1) ? ("Has construido " + std::to_string(cantidad_items) + " armas!") : "Has construido el arma!";
            } else if (item_type == eOBJType::otBarcos) {
                msg = (cantidad_items > 1) ? ("Has construido " + std::to_string(cantidad_items) + " barcos!") : "Has construido el barco!";
            } else {
                msg = (cantidad_items > 1) ? ("Has construido " + std::to_string(cantidad_items) + " objetos!") : "Has construido el objeto!";
            }
            g_hooks.WriteConsoleMsg(user_index, msg, FontTypeNames::FONTTYPE_INFO);
        }

        Obj mi_obj;
        mi_obj.Amount = cantidad_items;
        mi_obj.ObjIndex = item_index;

        if (!PutItemInInv(user_index, mi_obj)) {
            DropItemToFloor(user.Pos, mi_obj);
        }

        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Carpinteria), true);
        InvUsuario::UpdateUserInv(true, user_index, 0);

        if (g_hooks.SendAreaWave) {
            g_hooks.SendAreaWave(user_index, LABUROCARPINTERO);
        }

        user.Reputacion.PlebeRep += vlProleta;
        if (user.Reputacion.PlebeRep > MAXREP) {
            user.Reputacion.PlebeRep = MAXREP;
        }
        user.Counters.Trabajando++;
    }
}

void DoUpgrade(std::int16_t user_index, std::int16_t item_index) {
    auto& user = UserList[user_index];
    std::int16_t item_upgrade = ObjDataList[item_index].Upgrade;

    std::int16_t gasto_sta = (user.clase == eClass::Worker) ? GASTO_ENERGIA_TRABAJADOR : GASTO_ENERGIA_NO_TRABAJADOR;
    if (user.Stats.MinSta >= gasto_sta) {
        QuitarSta(user_index, gasto_sta);
    } else {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "No tienes suficiente energía.", FontTypeNames::FONTTYPE_INFO);
        }
        return;
    }

    if (item_upgrade <= 0) return;
    if (!TieneMaterialesUpgrade(user_index, item_index)) return;

    if (PuedeConstruirHerreria(item_upgrade)) {
        if (user.Invent.WeaponEqpObjIndex != MARTILLO_HERRERO) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "Debes equiparte el martillo de herrero.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }
        float mod = ModHerreriA(user.clase);
        float skill_div = static_cast<float>(user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Herreria)]) / mod;
        if (static_cast<std::int16_t>(std::round(skill_div)) < ObjDataList[item_upgrade].SkHerreria) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "No tienes suficientes skills.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }

        if (g_hooks.WriteConsoleMsg) {
            const auto& item_type = ObjDataList[item_index].OBJType;
            if (item_type == eOBJType::otWeapon) {
                g_hooks.WriteConsoleMsg(user_index, "Has mejorado el arma!", FontTypeNames::FONTTYPE_INFO);
            } else if (item_type == eOBJType::otESCUDO) {
                g_hooks.WriteConsoleMsg(user_index, "Has mejorado el escudo!", FontTypeNames::FONTTYPE_INFO);
            } else if (item_type == eOBJType::otCASCO) {
                g_hooks.WriteConsoleMsg(user_index, "Has mejorado el casco!", FontTypeNames::FONTTYPE_INFO);
            } else if (item_type == eOBJType::otArmadura) {
                g_hooks.WriteConsoleMsg(user_index, "Has mejorado la armadura!", FontTypeNames::FONTTYPE_INFO);
            }
        }

        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Herreria), true);
        if (g_hooks.SendAreaWave) {
            g_hooks.SendAreaWave(user_index, MARTILLOHERRERO);
        }
    } else if (PuedeConstruirCarpintero(item_upgrade)) {
        if (user.Invent.WeaponEqpObjIndex != SERRUCHO_CARPINTERO) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "Debes equiparte el serrucho.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }
        float mod = static_cast<float>(ModCarpinteria(user.clase));
        float skill_div = static_cast<float>(user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Carpinteria)]) / mod;
        if (static_cast<std::int16_t>(std::round(skill_div)) < ObjDataList[item_upgrade].SkCarpinteria) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "No tienes suficientes skills.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }

        if (g_hooks.WriteConsoleMsg) {
            const auto& item_type = ObjDataList[item_index].OBJType;
            if (item_type == eOBJType::otFlechas) {
                g_hooks.WriteConsoleMsg(user_index, "Has mejorado la flecha!", FontTypeNames::FONTTYPE_INFO);
            } else if (item_type == eOBJType::otWeapon) {
                g_hooks.WriteConsoleMsg(user_index, "Has mejorado el arma!", FontTypeNames::FONTTYPE_INFO);
            } else if (item_type == eOBJType::otBarcos) {
                g_hooks.WriteConsoleMsg(user_index, "Has mejorado el barco!", FontTypeNames::FONTTYPE_INFO);
            }
        }

        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Carpinteria), true);
        if (g_hooks.SendAreaWave) {
            g_hooks.SendAreaWave(user_index, LABUROCARPINTERO);
        }
    } else {
        return;
    }

    QuitarMaterialesUpgrade(user_index, item_index);

    Obj mi_obj;
    mi_obj.Amount = 1;
    mi_obj.ObjIndex = item_upgrade;

    if (!PutItemInInv(user_index, mi_obj)) {
        DropItemToFloor(user.Pos, mi_obj);
    }

    InvUsuario::UpdateUserInv(true, user_index, 0);

    user.Reputacion.PlebeRep += vlProleta;
    if (user.Reputacion.PlebeRep > MAXREP) {
        user.Reputacion.PlebeRep = MAXREP;
    }

    user.Counters.Trabajando++;
}

// G4 — Habilidades de Combate, Hurto, Desarme y Domesticación

void DoApuñalar(std::int16_t user_index, std::int16_t victim_npc_index, std::int16_t victim_user_index, std::int16_t daño) {
    auto& user = UserList[user_index];
    std::int16_t skill = user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Apuñalar)];
    std::int32_t suerte = 0;

    switch (user.clase) {
        case eClass::Assasin:
            suerte = static_cast<std::int32_t>(((0.00003 * skill - 0.002) * skill + 0.098) * skill + 4.25);
            break;
        case eClass::Cleric:
        case eClass::Paladin:
        case eClass::Pirat:
            suerte = static_cast<std::int32_t>(((0.000003 * skill + 0.0006) * skill + 0.0107) * skill + 4.93);
            break;
        case eClass::Bard:
            suerte = static_cast<std::int32_t>(((0.000002 * skill + 0.0002) * skill + 0.032) * skill + 4.81);
            break;
        default:
            suerte = static_cast<std::int32_t>(0.0361 * skill + 4.39);
            break;
    }

    if (GetRandomNumber(0, 100) < suerte) {
        if (victim_user_index != 0) {
            auto& victim = UserList[victim_user_index];
            if (user.clase == eClass::Assasin) {
                daño = static_cast<std::int16_t>(std::round(daño * 1.5));
            } else {
                daño = static_cast<std::int16_t>(std::round(daño * 1.4));
            }

            victim.Stats.MinHp -= daño;
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "Has apuñalado a " + victim.name + " por " + std::to_string(daño), FontTypeNames::FONTTYPE_FIGHT);
                g_hooks.WriteConsoleMsg(victim_user_index, "Te ha apuñalado " + user.name + " por " + std::to_string(daño), FontTypeNames::FONTTYPE_FIGHT);
            }

            if (g_hooks.FlushBuffer) {
                g_hooks.FlushBuffer(victim_user_index);
            }
        } else if (victim_npc_index != 0) {
            auto& npc = Npclist[victim_npc_index];
            std::int16_t npc_daño = static_cast<std::int16_t>(daño * 2);
            npc.Stats.MinHp -= npc_daño;
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "Has apuñalado la criatura por " + std::to_string(npc_daño), FontTypeNames::FONTTYPE_FIGHT);
            }
            if (g_hooks.CalcularDarExp) {
                g_hooks.CalcularDarExp(user_index, victim_npc_index, npc_daño);
            }
        }

        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Apuñalar), true);
    } else {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "¡No has logrado apuñalar a tu enemigo!", FontTypeNames::FONTTYPE_FIGHT);
        }
        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Apuñalar), false);
    }
}

void DoAcuchillar(std::int16_t user_index, std::int16_t victim_npc_index, std::int16_t victim_user_index, std::int16_t daño) {
    auto& user = UserList[user_index];
    if (user.clase != eClass::Pirat) return;
    if (user.Invent.WeaponEqpSlot == 0) return;

    if (GetRandomNumber(0, 100) < PROB_ACUCHILLAR) {
        daño = static_cast<std::int16_t>(daño * DAÑO_ACUCHILLAR);

        if (victim_user_index != 0) {
            auto& victim = UserList[victim_user_index];
            victim.Stats.MinHp -= daño;
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "Has acuchillado a " + victim.name + " por " + std::to_string(daño), FontTypeNames::FONTTYPE_FIGHT);
                g_hooks.WriteConsoleMsg(victim_user_index, user.name + " te ha acuchillado por " + std::to_string(daño), FontTypeNames::FONTTYPE_FIGHT);
            }
        } else if (victim_npc_index != 0) {
            auto& npc = Npclist[victim_npc_index];
            npc.Stats.MinHp -= daño;
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "Has acuchillado a la criatura por " + std::to_string(daño), FontTypeNames::FONTTYPE_FIGHT);
            }
            if (g_hooks.CalcularDarExp) {
                g_hooks.CalcularDarExp(user_index, victim_npc_index, daño);
            }
        }
    }
}

void DoGolpeCritico(std::int16_t user_index, std::int16_t victim_npc_index, std::int16_t victim_user_index, std::int16_t daño) {
    auto& user = UserList[user_index];
    if (user.clase != eClass::Bandit) return;
    if (user.Invent.WeaponEqpSlot == 0) return;
    if (user.Invent.WeaponEqpObjIndex <= 0 || ObjDataList[user.Invent.WeaponEqpObjIndex].name != "Espada Vikinga") return;

    std::int16_t skill = user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Wrestling)];
    std::int32_t suerte = static_cast<std::int32_t>((((0.00000003 * skill + 0.000006) * skill + 0.000107) * skill + 0.0893) * 100);

    if (GetRandomNumber(0, 100) < suerte) {
        // Bug #42: Preservación del reductor al 75%
        daño = static_cast<std::int16_t>(daño * 0.75);

        if (victim_user_index != 0) {
            auto& victim = UserList[victim_user_index];
            victim.Stats.MinHp -= daño;
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "Has golpeado críticamente a " + victim.name + " por " + std::to_string(daño) + ".", FontTypeNames::FONTTYPE_FIGHT);
                g_hooks.WriteConsoleMsg(victim_user_index, user.name + " te ha golpeado críticamente por " + std::to_string(daño) + ".", FontTypeNames::FONTTYPE_FIGHT);
            }
        } else if (victim_npc_index != 0) {
            auto& npc = Npclist[victim_npc_index];
            npc.Stats.MinHp -= daño;
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "Has golpeado críticamente a la criatura por " + std::to_string(daño) + ".", FontTypeNames::FONTTYPE_FIGHT);
            }
            if (g_hooks.CalcularDarExp) {
                g_hooks.CalcularDarExp(user_index, victim_npc_index, daño);
            }
        }
    }
}

static bool TieneObjetosRobables(std::int16_t victima_index) {
    const auto& victim = UserList[victima_index];
    for (std::int16_t i = 1; i <= victim.CurrentInventorySlots; ++i) {
        if (victim.Invent.Object[i].ObjIndex > 0) {
            if (ObjEsRobable(victima_index, i)) return true;
        }
    }
    return false;
}

bool ObjEsRobable(std::int16_t victima_index, std::int16_t slot) {
    const auto& victim = UserList[victima_index];
    std::int16_t oi = victim.Invent.Object[slot].ObjIndex;
    if (oi <= 0 || oi >= static_cast<std::int16_t>(ObjDataList.size())) return false;

    const auto& obj = ObjDataList[oi];
    return (obj.OBJType != eOBJType::otLlaves &&
            victim.Invent.Object[slot].Equipped == 0 &&
            obj.Real == 0 &&
            obj.Caos == 0 &&
            obj.OBJType != eOBJType::otBarcos);
}

void RobarObjeto(std::int16_t ladr_on_index, std::int16_t victima_index) {
    auto& victim = UserList[victima_index];
    bool flag = false;
    std::int16_t i = 1;

    if (GetRandomNumber(1, 12) < 6) {
        i = 1;
        while (!flag && i <= victim.CurrentInventorySlots) {
            if (victim.Invent.Object[i].ObjIndex > 0 && ObjEsRobable(victima_index, i)) {
                if (GetRandomNumber(1, 10) < 4) flag = true;
            }
            if (!flag) i++;
        }
    } else {
        i = 20;
        while (!flag && i > 0) {
            if (victim.Invent.Object[i].ObjIndex > 0 && ObjEsRobable(victima_index, i)) {
                if (GetRandomNumber(1, 10) < 4) flag = true;
            }
            if (!flag) i--;
        }
    }

    if (flag) {
        Obj mi_obj{};
        std::int32_t obj_amount = victim.Invent.Object[i].Amount;
        std::int16_t num = std::max<std::int16_t>(1, GetRandomNumber(static_cast<std::int32_t>(obj_amount * 0.05), static_cast<std::int32_t>(obj_amount * 0.1)));

        mi_obj.Amount = num;
        mi_obj.ObjIndex = victim.Invent.Object[i].ObjIndex;

        victim.Invent.Object[i].Amount = obj_amount - num;
        if (victim.Invent.Object[i].Amount <= 0) {
            if (g_hooks.QuitarUserInvItem) {
                g_hooks.QuitarUserInvItem(victima_index, static_cast<std::uint8_t>(i), 1);
            }
        }

        if (g_hooks.UpdateUserInv) {
            g_hooks.UpdateUserInv(false, victima_index, static_cast<std::uint8_t>(i));
        } else {
            InvUsuario::UpdateUserInv(false, victima_index, static_cast<std::uint8_t>(i));
        }

        if (!PutItemInInv(ladr_on_index, mi_obj)) {
            DropItemToFloor(UserList[ladr_on_index].Pos, mi_obj);
        }

        if (g_hooks.WriteConsoleMsg) {
            if (UserList[ladr_on_index].clase == eClass::Thief) {
                g_hooks.WriteConsoleMsg(ladr_on_index, "Has robado " + std::to_string(mi_obj.Amount) + " " + ObjDataList[mi_obj.ObjIndex].name, FontTypeNames::FONTTYPE_INFO);
            } else {
                g_hooks.WriteConsoleMsg(ladr_on_index, "Has hurtado " + std::to_string(mi_obj.Amount) + " " + ObjDataList[mi_obj.ObjIndex].name, FontTypeNames::FONTTYPE_INFO);
            }
        }
    } else {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(ladr_on_index, "No has logrado robar ningún objeto.", FontTypeNames::FONTTYPE_INFO);
        }
    }

    if (g_hooks.CancelExit) {
        g_hooks.CancelExit(victima_index);
    }
}

void DoRobar(std::int16_t ladr_on_index, std::int16_t victima_index) {
    auto& thief = UserList[ladr_on_index];
    auto& victim = UserList[victima_index];

    if (thief.Stats.MinSta < 10) {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(ladr_on_index, "Estás muy cansado.", FontTypeNames::FONTTYPE_INFO);
        }
        return;
    }

    QuitarSta(ladr_on_index, 10);

    bool guantes_hurto = (thief.Invent.AnilloEqpObjIndex == GUANTE_HURTO);

    std::int16_t robar_skill = thief.Stats.UserSkills[static_cast<std::size_t>(eSkill::Robar)];
    std::int32_t suerte = 35;

    if (robar_skill <= 10) suerte = 35;
    else if (robar_skill <= 20) suerte = 30;
    else if (robar_skill <= 30) suerte = 28;
    else if (robar_skill <= 40) suerte = 24;
    else if (robar_skill <= 50) suerte = 22;
    else if (robar_skill <= 60) suerte = 20;
    else if (robar_skill <= 70) suerte = 18;
    else if (robar_skill <= 80) suerte = 15;
    else if (robar_skill <= 90) suerte = 10;
    else if (robar_skill < 100) suerte = 7;
    else if (robar_skill == 100) suerte = 5;

    std::int32_t res = GetRandomNumber(1, suerte);

    if (res < 3) {
        if ((GetRandomNumber(1, 50) < 25) && (thief.clase == eClass::Thief)) {
            if (TieneObjetosRobables(victima_index)) {
                RobarObjeto(ladr_on_index, victima_index);
            } else {
                if (g_hooks.WriteConsoleMsg) {
                    g_hooks.WriteConsoleMsg(ladr_on_index, victim.name + " no tiene objetos.", FontTypeNames::FONTTYPE_INFO);
                }
            }
        } else {
            if (victim.Stats.GLD > 0) {
                std::int32_t n = 0;
                if (thief.clase == eClass::Thief) {
                    if (guantes_hurto) {
                        n = GetRandomNumber(thief.Stats.ELV * 50, thief.Stats.ELV * 100);
                    } else {
                        n = GetRandomNumber(thief.Stats.ELV * 25, thief.Stats.ELV * 50);
                    }
                } else {
                    n = GetRandomNumber(1, 100);
                }

                if (n > victim.Stats.GLD) n = victim.Stats.GLD;
                victim.Stats.GLD -= n;
                thief.Stats.GLD += n;
                if (thief.Stats.GLD > MAXORO) thief.Stats.GLD = MAXORO;

                if (g_hooks.WriteConsoleMsg) {
                    g_hooks.WriteConsoleMsg(ladr_on_index, "Le has robado " + std::to_string(n) + " monedas de oro a " + victim.name, FontTypeNames::FONTTYPE_INFO);
                }

                if (g_hooks.WriteUpdateGold) {
                    g_hooks.WriteUpdateGold(ladr_on_index);
                    g_hooks.WriteUpdateGold(victima_index);
                }

                if (g_hooks.FlushBuffer) {
                    g_hooks.FlushBuffer(victima_index);
                }
            } else {
                if (g_hooks.WriteConsoleMsg) {
                    g_hooks.WriteConsoleMsg(ladr_on_index, victim.name + " no tiene oro.", FontTypeNames::FONTTYPE_INFO);
                }
            }
        }
        TriggerSubirSkill(ladr_on_index, static_cast<std::int16_t>(eSkill::Robar), true);
    } else {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(ladr_on_index, "¡No has logrado robar nada!", FontTypeNames::FONTTYPE_INFO);
            g_hooks.WriteConsoleMsg(victima_index, "¡" + thief.name + " ha intentado robarte!", FontTypeNames::FONTTYPE_INFO);
            g_hooks.WriteConsoleMsg(victima_index, "¡" + thief.name + " es un criminal!", FontTypeNames::FONTTYPE_INFO);
        }
        if (g_hooks.FlushBuffer) {
            g_hooks.FlushBuffer(victima_index);
        }
        TriggerSubirSkill(ladr_on_index, static_cast<std::int16_t>(eSkill::Robar), false);
    }

    if (!FileIO::criminal(ladr_on_index)) {
        if (!FileIO::criminal(victima_index)) {
            if (g_hooks.VolverCriminal) {
                g_hooks.VolverCriminal(ladr_on_index);
            }
        }
    }

    if (FileIO::criminal(ladr_on_index)) {
        thief.Reputacion.LadronesRep += vlLadron;
        if (thief.Reputacion.LadronesRep > MAXREP) thief.Reputacion.LadronesRep = MAXREP;
    }
}

void DoHurtar(std::int16_t user_index, std::int16_t victima_index) {
    if (g_hooks.TriggerZonaPelea) {
        if (g_hooks.TriggerZonaPelea(user_index, victima_index) != TRIGGER6_AUSENTE) return;
    }

    const auto& user = UserList[user_index];
    if (user.clase != eClass::Bandit) return;
    if (user.Invent.AnilloEqpObjIndex != GUANTE_HURTO) return;

    std::int32_t res = GetRandomNumber(1, 100);
    if (res < 20) {
        if (TieneObjetosRobables(victima_index)) {
            RobarObjeto(user_index, victima_index);
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(victima_index, "¡" + user.name + " es un Bandido!", FontTypeNames::FONTTYPE_INFO);
            }
        } else {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, UserList[victima_index].name + " no tiene objetos.", FontTypeNames::FONTTYPE_INFO);
            }
        }
    }
}

void DoHandInmo(std::int16_t user_index, std::int16_t victima_index) {
    auto& victim = UserList[victima_index];
    if (victim.flags.Paralizado == 1) return;

    const auto& user = UserList[user_index];
    if (user.clase != eClass::Thief) return;
    if (user.Invent.AnilloEqpObjIndex != GUANTE_HURTO) return;

    std::int32_t res = GetRandomNumber(0, 100);
    if (res < (user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Wrestling)] / 4)) {
        victim.flags.Paralizado = 1;
        victim.Counters.Paralisis = IntervaloParalizado / 2;
        if (g_hooks.WriteParalizeOK) {
            g_hooks.WriteParalizeOK(victima_index);
        }
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "Tu golpe ha dejado inmóvil a tu oponente", FontTypeNames::FONTTYPE_INFO);
            g_hooks.WriteConsoleMsg(victima_index, "¡El golpe te ha dejado inmóvil!", FontTypeNames::FONTTYPE_INFO);
        }
    }
}

void Desarmar(std::int16_t user_index, std::int16_t victim_index) {
    const auto& user = UserList[user_index];
    auto& victim = UserList[victim_index];

    std::uint8_t wrestling_skill = static_cast<std::uint8_t>(user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Wrestling)]);
    std::int32_t probabilidad = static_cast<std::int32_t>(wrestling_skill * 0.2 + user.Stats.ELV * 0.66);
    std::int32_t resultado = GetRandomNumber(1, 100);

    if (resultado <= probabilidad) {
        if (g_hooks.Desequipar) {
            g_hooks.Desequipar(victim_index, victim.Invent.WeaponEqpSlot);
        } else {
            InvUsuario::Desequipar(victim_index, victim.Invent.WeaponEqpSlot);
        }

        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "¡Has logrado desarmar a tu oponente!", FontTypeNames::FONTTYPE_FIGHT);
            if (victim.Stats.ELV < 20) {
                g_hooks.WriteConsoleMsg(victim_index, "¡Tu oponente te ha desarmado!", FontTypeNames::FONTTYPE_FIGHT);
            }
        }
        if (g_hooks.FlushBuffer) {
            g_hooks.FlushBuffer(victim_index);
        }
    }
}

void DoDesequipar(std::int16_t user_index, std::int16_t victim_index) {
    const auto& user = UserList[user_index];
    auto& victim = UserList[victim_index];

    if (user.Invent.AnilloEqpObjIndex != GUANTE_HURTO) return;
    if (user.Invent.WeaponEqpObjIndex > 0) return;

    std::uint8_t wrestling_skill = static_cast<std::uint8_t>(user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Wrestling)]);
    std::int32_t probabilidad = static_cast<std::int32_t>(wrestling_skill * 0.2 + user.Stats.ELV * 0.66);
    bool algo_equipado = false;

    // Escudo
    if (victim.Invent.EscudoEqpObjIndex > 0) {
        if (GetRandomNumber(1, 100) <= probabilidad) {
            if (g_hooks.Desequipar) {
                g_hooks.Desequipar(victim_index, victim.Invent.EscudoEqpSlot);
            } else {
                InvUsuario::Desequipar(victim_index, victim.Invent.EscudoEqpSlot);
            }
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "¡Has logrado desequipar el escudo de tu oponente!", FontTypeNames::FONTTYPE_FIGHT);
                if (victim.Stats.ELV < 20) {
                    g_hooks.WriteConsoleMsg(victim_index, "¡Tu oponente te ha desequipado el escudo!", FontTypeNames::FONTTYPE_FIGHT);
                }
            }
            if (g_hooks.FlushBuffer) g_hooks.FlushBuffer(victim_index);
            return;
        }
        algo_equipado = true;
    }

    // Arma
    if (victim.Invent.WeaponEqpObjIndex > 0) {
        if (GetRandomNumber(1, 100) <= probabilidad) {
            if (g_hooks.Desequipar) {
                g_hooks.Desequipar(victim_index, victim.Invent.WeaponEqpSlot);
            } else {
                InvUsuario::Desequipar(victim_index, victim.Invent.WeaponEqpSlot);
            }
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "¡Has logrado desarmar a tu oponente!", FontTypeNames::FONTTYPE_FIGHT);
                if (victim.Stats.ELV < 20) {
                    g_hooks.WriteConsoleMsg(victim_index, "¡Tu oponente te ha desarmado!", FontTypeNames::FONTTYPE_FIGHT);
                }
            }
            if (g_hooks.FlushBuffer) g_hooks.FlushBuffer(victim_index);
            return;
        }
        algo_equipado = true;
    }

    // Casco
    if (victim.Invent.CascoEqpObjIndex > 0) {
        if (GetRandomNumber(1, 100) <= probabilidad) {
            if (g_hooks.Desequipar) {
                g_hooks.Desequipar(victim_index, victim.Invent.CascoEqpSlot);
            } else {
                InvUsuario::Desequipar(victim_index, victim.Invent.CascoEqpSlot);
            }
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "¡Has logrado desequipar el casco de tu oponente!", FontTypeNames::FONTTYPE_FIGHT);
                if (victim.Stats.ELV < 20) {
                    g_hooks.WriteConsoleMsg(victim_index, "¡Tu oponente te ha desequipado el casco!", FontTypeNames::FONTTYPE_FIGHT);
                }
            }
            if (g_hooks.FlushBuffer) g_hooks.FlushBuffer(victim_index);
            return;
        }
        algo_equipado = true;
    }

    if (g_hooks.WriteConsoleMsg) {
        if (algo_equipado) {
            g_hooks.WriteConsoleMsg(user_index, "¡No has logrado desequipar ningún item a tu oponente!", FontTypeNames::FONTTYPE_FIGHT);
        } else {
            g_hooks.WriteConsoleMsg(user_index, "Tu oponente no tiene equipado items!", FontTypeNames::FONTTYPE_FIGHT);
        }
    }
}

std::int16_t FreeMascotaIndex(std::int16_t user_index) {
    const auto& user = UserList[user_index];
    for (std::size_t j = 1; j <= MAXMASCOTAS; ++j) {
        if (user.MascotasType[j] == 0) {
            return static_cast<std::int16_t>(j);
        }
    }
    return 0;
}

bool PuedeDomarMascota(std::int16_t user_index, std::int16_t npc_index) {
    const auto& user = UserList[user_index];
    std::int32_t num_mascotas = 0;
    std::int16_t npc_type = Npclist[npc_index].Numero;

    for (std::size_t i = 1; i <= MAXMASCOTAS; ++i) {
        if (user.MascotasType[i] == npc_type) {
            num_mascotas++;
        }
    }
    return (num_mascotas <= 1);
}

void DoDomar(std::int16_t user_index, std::int16_t npc_index) {
    auto& npc = Npclist[npc_index];
    if (npc.MaestroUser == user_index) {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "Ya domaste a esa criatura.", FontTypeNames::FONTTYPE_INFO);
        }
        return;
    }

    auto& user = UserList[user_index];
    if (user.NroMascotas < MAXMASCOTAS) {
        if (npc.MaestroNpc > 0 || npc.MaestroUser > 0) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "La criatura ya tiene amo.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }

        if (!PuedeDomarMascota(user_index, npc_index)) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "No puedes domar más de dos criaturas del mismo tipo.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }

        std::int32_t puntos_domar = static_cast<std::int32_t>(user.Stats.UserAtributos[static_cast<std::size_t>(eAtributos::Carisma)]) *
                                   static_cast<std::int32_t>(user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Domar)]);

        std::int32_t puntos_requeridos = 0;
        if (user.Invent.AnilloEqpObjIndex == FLAUTAELFICA) {
            puntos_requeridos = static_cast<std::int32_t>(npc.flags.Domable * 0.8);
        } else if (user.Invent.AnilloEqpObjIndex == FLAUTAMAGICA) {
            puntos_requeridos = static_cast<std::int32_t>(npc.flags.Domable * 0.89);
        } else {
            puntos_requeridos = npc.flags.Domable;
        }

        if (puntos_requeridos <= puntos_domar && GetRandomNumber(1, 5) == 1) {
            user.NroMascotas++;
            std::int16_t index = FreeMascotaIndex(user_index);
            user.MascotasIndex[index] = npc_index;
            user.MascotasType[index] = npc.Numero;

            npc.MaestroUser = user_index;

            if (g_hooks.FollowAmo) {
                g_hooks.FollowAmo(npc_index);
            }

            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "La criatura te ha aceptado como su amo.", FontTypeNames::FONTTYPE_INFO);
            }

            bool can_stay = false;
            if (static_cast<std::size_t>(user.Pos.Map) < MapInfoList.size()) {
                can_stay = (MapInfoList[user.Pos.Map].Pk == true);
            }

            if (!can_stay) {
                std::int16_t pet_type = npc.Numero;
                std::int16_t nro_pets = user.NroMascotas;

                if (g_hooks.QuitarNPC) {
                    g_hooks.QuitarNPC(npc_index);
                }

                user.MascotasType[index] = pet_type;
                user.NroMascotas = nro_pets;

                if (g_hooks.WriteConsoleMsg) {
                    g_hooks.WriteConsoleMsg(user_index, "No se permiten mascotas en zona segura. Éstas te esperarán afuera.", FontTypeNames::FONTTYPE_INFO);
                }
            }

            TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Domar), true);
        } else {
            if (user.flags.UltimoMensaje != 5) {
                if (g_hooks.WriteConsoleMsg) {
                    g_hooks.WriteConsoleMsg(user_index, "No has logrado domar la criatura.", FontTypeNames::FONTTYPE_INFO);
                }
                user.flags.UltimoMensaje = 5;
            }
            TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Domar), false);
        }
    } else {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "No puedes controlar más criaturas.", FontTypeNames::FONTTYPE_INFO);
        }
    }
}

static inline std::size_t mapBlockIndex(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return static_cast<std::size_t>((map - 1) * 10000 + (y - 1) * 100 + (x - 1));
}

// G5 — Gestión de Estados y Entorno

void DoPermanecerOculto(std::int16_t user_index) {
    auto& user = UserList[user_index];
    user.Counters.TiempoOculto--;
    if (user.Counters.TiempoOculto <= 0) {
        if (user.clase == eClass::Bandit) {
            user.Counters.TiempoOculto = static_cast<std::int32_t>(IntervaloOculto / 2);
        } else {
            user.Counters.TiempoOculto = IntervaloOculto;
        }

        if (user.clase == eClass::Hunter && user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Ocultarse)] > 90) {
            if (user.Invent.ArmourEqpObjIndex == 648 || user.Invent.ArmourEqpObjIndex == 360) {
                return;
            }
        }

        user.Counters.TiempoOculto = 0;
        user.flags.Oculto = 0;

        if (user.flags.Navegando == 1) {
            if (user.clase == eClass::Pirat) {
                if (g_hooks.ToogleBoatBody) {
                    g_hooks.ToogleBoatBody(user_index);
                }
                if (g_hooks.WriteConsoleMsg) {
                    g_hooks.WriteConsoleMsg(user_index, "¡Has recuperado tu apariencia normal!", FontTypeNames::FONTTYPE_INFO);
                }
                if (g_hooks.ChangeUserChar) {
                    g_hooks.ChangeUserChar(user_index, user.char_appearance.body, user.char_appearance.Head, static_cast<std::uint8_t>(user.char_appearance.heading), NingunArma, NingunEscudo, NingunCasco);
                }
            }
        } else {
            if (user.flags.invisible == 0) {
                if (g_hooks.WriteConsoleMsg) {
                    g_hooks.WriteConsoleMsg(user_index, "Has vuelto a ser visible.", FontTypeNames::FONTTYPE_INFO);
                }
                if (g_hooks.SetInvisible) {
                    g_hooks.SetInvisible(user_index, user.char_appearance.CharIndex, false);
                }
            }
        }
    }
}

void DoOcultarse(std::int16_t user_index) {
    auto& user = UserList[user_index];
    std::int16_t skill = user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Ocultarse)];
    double suerte = (((0.000002 * skill - 0.0002) * skill + 0.0064) * skill + 0.1124) * 100.0;

    std::int32_t res = GetRandomNumber(1, 100);
    if (static_cast<double>(res) <= suerte) {
        user.flags.Oculto = 1;
        double tiempo = (-0.000001 * std::pow(100 - skill, 3));
        tiempo += (0.00009229 * std::pow(100 - skill, 2));
        tiempo += (-0.0088 * (100 - skill));
        tiempo += 0.9571;
        tiempo *= IntervaloOculto;

        user.Counters.TiempoOculto = static_cast<std::int32_t>(tiempo);

        if (user.flags.Navegando == 0) {
            if (g_hooks.SetInvisible) {
                g_hooks.SetInvisible(user_index, user.char_appearance.CharIndex, true);
            }
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "¡Te has escondido entre las sombras!", FontTypeNames::FONTTYPE_INFO);
            }
        } else {
            user.char_appearance.body = iFragataFantasmal;
            if (g_hooks.ChangeUserChar) {
                g_hooks.ChangeUserChar(user_index, user.char_appearance.body, user.char_appearance.Head, static_cast<std::uint8_t>(user.char_appearance.heading), NingunArma, NingunEscudo, NingunCasco);
            }
        }

        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Ocultarse), true);
    } else {
        if (user.flags.UltimoMensaje != 4) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "¡No has logrado esconderte!", FontTypeNames::FONTTYPE_INFO);
            }
            user.flags.UltimoMensaje = 4;
        }
        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Ocultarse), false);
    }

    user.Counters.Ocultando++;
}

void DoNavega(std::int16_t user_index, const ObjData& barco, std::int16_t slot) {
    auto& user = UserList[user_index];
    float mod_nave = ModNavegacion(user.clase, user_index);

    if (static_cast<float>(user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Navegacion)]) / mod_nave < static_cast<float>(barco.MinSkill)) {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "No tienes suficientes conocimientos para usar este barco.", FontTypeNames::FONTTYPE_INFO);
            g_hooks.WriteConsoleMsg(user_index, "Para usar este barco necesitas " + std::to_string(static_cast<std::int32_t>(barco.MinSkill * mod_nave)) + " puntos en navegacion.", FontTypeNames::FONTTYPE_INFO);
        }
        return;
    }

    user.Invent.BarcoObjIndex = user.Invent.Object[slot].ObjIndex;
    user.Invent.BarcoSlot = slot;

    if (user.flags.Navegando == 0) {
        user.char_appearance.Head = 0;
        if (user.flags.Muerto == 0) {
            if (g_hooks.ToogleBoatBody) {
                g_hooks.ToogleBoatBody(user_index);
            }
            if (user.clase == eClass::Pirat && user.flags.Oculto == 1) {
                user.flags.Oculto = 0;
                if (g_hooks.SetInvisible) {
                    g_hooks.SetInvisible(user_index, user.char_appearance.CharIndex, false);
                }
                if (g_hooks.WriteConsoleMsg) {
                    g_hooks.WriteConsoleMsg(user_index, "¡Has vuelto a ser visible!", FontTypeNames::FONTTYPE_INFO);
                }
            }
        } else {
            user.char_appearance.body = iFragataFantasmal;
            user.char_appearance.ShieldAnim = NingunEscudo;
            user.char_appearance.WeaponAnim = NingunArma;
            user.char_appearance.CascoAnim = NingunCasco;
        }
        user.flags.Navegando = 1;
    } else {
        if (user.flags.Muerto == 0) {
            user.char_appearance.Head = user.OrigChar.Head;
            if (user.clase == eClass::Pirat && user.flags.Oculto == 1) {
                user.flags.Oculto = 0;
                user.Counters.Ocultando = 0;
                if (g_hooks.WriteConsoleMsg) {
                    g_hooks.WriteConsoleMsg(user_index, "¡Has recuperado tu apariencia normal!", FontTypeNames::FONTTYPE_INFO);
                }
            }

            if (user.Invent.ArmourEqpObjIndex > 0) {
                user.char_appearance.body = ObjDataList[user.Invent.ArmourEqpObjIndex].Ropaje;
            } else {
                user.char_appearance.body = user.OrigChar.body;
            }

            if (user.Invent.EscudoEqpObjIndex > 0) {
                user.char_appearance.ShieldAnim = ObjDataList[user.Invent.EscudoEqpObjIndex].ShieldAnim;
            }
            if (user.Invent.WeaponEqpObjIndex > 0) {
                user.char_appearance.WeaponAnim = ObjDataList[user.Invent.WeaponEqpObjIndex].WeaponAnim;
            }
            if (user.Invent.CascoEqpObjIndex > 0) {
                user.char_appearance.CascoAnim = ObjDataList[user.Invent.CascoEqpObjIndex].CascoAnim;
            }
        } else {
            user.char_appearance.body = iCuerpoMuerto;
            user.char_appearance.Head = iCabezaMuerto;
            user.char_appearance.ShieldAnim = NingunEscudo;
            user.char_appearance.WeaponAnim = NingunArma;
            user.char_appearance.CascoAnim = NingunCasco;
        }
        user.flags.Navegando = 0;
    }

    if (g_hooks.ChangeUserChar) {
        g_hooks.ChangeUserChar(user_index, user.char_appearance.body, user.char_appearance.Head, static_cast<std::uint8_t>(user.char_appearance.heading), user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim, user.char_appearance.CascoAnim);
    }
    if (g_hooks.WriteNavigateToggle) {
        g_hooks.WriteNavigateToggle(user_index);
    }
}

void DoAdminInvisible(std::int16_t user_index) {
    auto& user = UserList[user_index];
    if (user.flags.AdminInvisible == 0) {
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

        user.flags.AdminInvisible = 1;
        user.flags.invisible = 1;
        user.flags.Oculto = 1;
        user.flags.OldBody = user.char_appearance.body;
        user.flags.OldHead = user.char_appearance.Head;
        user.char_appearance.body = 0;
        user.char_appearance.Head = 0;

        if (g_hooks.SetInvisible) {
            g_hooks.SetInvisible(user_index, user.char_appearance.CharIndex, true);
        }
    } else {
        user.flags.AdminInvisible = 0;
        user.flags.invisible = 0;
        user.flags.Oculto = 0;
        user.Counters.TiempoOculto = 0;
        user.char_appearance.body = user.flags.OldBody;
        user.char_appearance.Head = user.flags.OldHead;

        if (g_hooks.ChangeUserChar) {
            g_hooks.ChangeUserChar(user_index, user.char_appearance.body, user.char_appearance.Head, static_cast<std::uint8_t>(user.char_appearance.heading), user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim, user.char_appearance.CascoAnim);
        }
        if (g_hooks.SetInvisible) {
            g_hooks.SetInvisible(user_index, user.char_appearance.CharIndex, false);
        }
    }
}

void TratarDeHacerFogata(std::int16_t map, std::int16_t x, std::int16_t y, std::int16_t user_index) {
    if (map < 1 || x < 1 || y < 1 || x > 100 || y > 100) return;

    auto& user = UserList[user_index];
    const WorldPos pos_madera{map, x, y};

    std::size_t idx = mapBlockIndex(map, x, y);
    if (idx >= MapData.size()) return;

    if (MapData[idx].ObjInfo.ObjIndex != 58) {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "Necesitas clickear sobre leña para hacer ramitas.", FontTypeNames::FONTTYPE_INFO);
        }
        return;
    }

    std::int32_t dist = std::abs(user.Pos.X - x) + std::abs(user.Pos.Y - y);
    if (user.Pos.Map != map || dist > 2) {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "Estás demasiado lejos para prender la fogata.", FontTypeNames::FONTTYPE_INFO);
        }
        return;
    }

    if (user.flags.Muerto == 1) {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "No puedes hacer fogatas estando muerto.", FontTypeNames::FONTTYPE_INFO);
        }
        return;
    }

    if (MapData[idx].ObjInfo.Amount < 3) {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "Necesitas por lo menos tres troncos para hacer una fogata.", FontTypeNames::FONTTYPE_INFO);
        }
        return;
    }

    std::uint8_t supervivencia_skill = static_cast<std::uint8_t>(user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Supervivencia)]);
    std::int32_t suerte = 3;
    if (supervivencia_skill >= 6 && supervivencia_skill <= 34) {
        suerte = 2;
    } else if (supervivencia_skill >= 35) {
        suerte = 1;
    }

    std::int32_t exito = GetRandomNumber(1, suerte);

    if (exito == 1) {
        Obj obj{};
        obj.ObjIndex = FOGATA_APAG;
        obj.Amount = static_cast<std::int16_t>(MapData[idx].ObjInfo.Amount / 3);

        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "Has hecho " + std::to_string(obj.Amount) + " fogatas.", FontTypeNames::FONTTYPE_INFO);
        }

        if (g_hooks.MakeObj) {
            g_hooks.MakeObj(obj, map, x, y);
        } else {
            MapData[idx].ObjInfo = obj;
        }

        user.flags.TargetObj = FOGATA_APAG;
        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Supervivencia), true);
    } else {
        if (user.flags.UltimoMensaje != 10) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "No has podido hacer la fogata.", FontTypeNames::FONTTYPE_INFO);
            }
            user.flags.UltimoMensaje = 10;
        }
        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Supervivencia), false);
    }
}

void DoMeditar(std::int16_t user_index) {
    auto& user = UserList[user_index];
    user.Counters.IdleCount = 0;

    std::uint32_t t_actual = 0;
    if (g_hooks.GetTickCount) {
        t_actual = g_hooks.GetTickCount() & 0x7FFFFFFF;
    } else {
        t_actual = static_cast<std::uint32_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count() & 0x7FFFFFFF);
    }

    if (t_actual - user.Counters.tInicioMeditar < TIEMPO_INICIOMEDITAR) {
        return;
    }

    if (!user.Counters.bPuedeMeditar) {
        user.Counters.bPuedeMeditar = true;
    }

    if (user.Stats.MinMAN >= user.Stats.MaxMAN) {
        if (g_hooks.WriteConsoleMsg) {
            g_hooks.WriteConsoleMsg(user_index, "Has terminado de meditar.", FontTypeNames::FONTTYPE_INFO);
        }
        if (g_hooks.WriteMeditateToggle) {
            g_hooks.WriteMeditateToggle(user_index);
        }
        user.flags.Meditando = false;
        user.char_appearance.FX = 0;
        user.char_appearance.loops = 0;
        return;
    }

    std::uint8_t meditar_skill = static_cast<std::uint8_t>(user.Stats.UserSkills[static_cast<std::size_t>(eSkill::Meditar)]);
    std::int32_t suerte = 35;

    if (meditar_skill <= 10) suerte = 35;
    else if (meditar_skill <= 20) suerte = 30;
    else if (meditar_skill <= 30) suerte = 28;
    else if (meditar_skill <= 40) suerte = 24;
    else if (meditar_skill <= 50) suerte = 22;
    else if (meditar_skill <= 60) suerte = 20;
    else if (meditar_skill <= 70) suerte = 18;
    else if (meditar_skill <= 80) suerte = 15;
    else if (meditar_skill <= 90) suerte = 10;
    else if (meditar_skill < 100) suerte = 7;
    else if (meditar_skill == 100) suerte = 5;

    std::int32_t res = GetRandomNumber(1, suerte);

    if (res == 1) {
        std::int16_t cant = static_cast<std::int16_t>(Porcentaje(user.Stats.MaxMAN, PorcentajeRecuperoMana));
        if (cant <= 0) cant = 1;
        user.Stats.MinMAN += cant;
        if (user.Stats.MinMAN > user.Stats.MaxMAN) {
            user.Stats.MinMAN = user.Stats.MaxMAN;
        }

        if (user.flags.UltimoMensaje != 22) {
            if (g_hooks.WriteConsoleMsg) {
                g_hooks.WriteConsoleMsg(user_index, "¡Has recuperado " + std::to_string(cant) + " puntos de maná!", FontTypeNames::FONTTYPE_INFO);
            }
            user.flags.UltimoMensaje = 22;
        }

        if (g_hooks.WriteUpdateMana) {
            g_hooks.WriteUpdateMana(user_index);
        }
        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Meditar), true);
    } else {
        TriggerSubirSkill(user_index, static_cast<std::int16_t>(eSkill::Meditar), false);
    }
}

} // namespace Trabajo
