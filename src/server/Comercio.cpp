#include "Comercio.hpp"
#include "Protocol.hpp"
#include "InvUsuario.hpp"
#include "Modulo_InventANDobj.hpp"
#include <cmath>
#include <string>
#include <algorithm>

namespace ao {

using namespace ao::net::protocol;

namespace {
SubirSkillHook g_subir_skill_hook{nullptr};
KeyPurchaseHook g_key_purchase_hook{nullptr};
BanUserHook g_ban_user_hook{nullptr};
} // namespace

void SetSubirSkillHook(SubirSkillHook hook) noexcept {
    g_subir_skill_hook = std::move(hook);
}

void SetKeyPurchaseHook(KeyPurchaseHook hook) noexcept {
    g_key_purchase_hook = std::move(hook);
}

void SetBanUserHook(BanUserHook hook) noexcept {
    g_ban_user_hook = std::move(hook);
}

float Descuento(std::int16_t user_index) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return 1.0f;
    }
    return 1.0f + static_cast<float>(UserList[user_index].Stats.UserSkills[eSkill::Comerciar]) / 100.0f;
}

float SalePrice(std::int16_t obj_index) {
    if (obj_index < 1 || static_cast<std::size_t>(obj_index) >= ObjDataList.size()) {
        return 0.0f;
    }
    if (InvUsuario::ItemNewbie(obj_index)) {
        return 0.0f;
    }
    return static_cast<float>(ObjDataList[obj_index].Valor) / static_cast<float>(REDUCTOR_PRECIOVENTA);
}

void IniciarComercioNPC(std::int16_t user_index) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }
    const auto target_npc = UserList[user_index].flags.TargetNPC;
    EnviarNpcInv(user_index, target_npc);
    UserList[user_index].flags.Comerciando = true;
    WriteCommerceInit(user_index);
}

std::int16_t SlotEnNPCInv(std::int16_t npc_index, std::int16_t obj_index, std::int16_t cantidad) {
    if (npc_index < 1 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return MAX_INVENTORY_SLOTS + 1;
    }

    std::int16_t slot = 1;
    while (slot <= MAX_INVENTORY_SLOTS) {
        const auto& item = Npclist[npc_index].Invent.Object[slot];
        if (item.ObjIndex == obj_index &&
            (static_cast<std::int32_t>(item.Amount) + static_cast<std::int32_t>(cantidad)) <= MAX_INVENTORY_OBJS) {
            return slot;
        }
        ++slot;
    }

    slot = 1;
    while (slot <= MAX_INVENTORY_SLOTS) {
        if (Npclist[npc_index].Invent.Object[slot].ObjIndex == 0) {
            Npclist[npc_index].Invent.NroItems++;
            return slot;
        }
        ++slot;
    }

    return slot;
}

void EnviarNpcInv(std::int16_t user_index, std::int16_t npc_index) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }
    if (npc_index < 1 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    // Replicación obligatoria del Bug #33: itera rígidamente solo hasta MAX_NORMAL_INVENTORY_SLOTS (20)
    for (std::uint8_t slot = 1; slot <= MAX_NORMAL_INVENTORY_SLOTS; ++slot) {
        const auto& npc_obj = Npclist[npc_index].Invent.Object[slot];
        if (npc_obj.ObjIndex > 0) {
            Obj this_obj{npc_obj.ObjIndex, npc_obj.Amount};
            float val = 0.0f;
            if (static_cast<std::size_t>(this_obj.ObjIndex) < ObjDataList.size()) {
                val = static_cast<float>(ObjDataList[this_obj.ObjIndex].Valor) / Descuento(user_index);
            }
            WriteChangeNPCInventorySlot(user_index, slot, this_obj, val);
        } else {
            Obj dummy_obj{};
            WriteChangeNPCInventorySlot(user_index, slot, dummy_obj, 0.0f);
        }
    }
}

void Comercio(eModoComercio modo, std::int16_t user_index, std::int16_t npc_index, std::int16_t slot, std::int16_t cantidad,
              KeyPurchaseHook key_hook, BanUserHook ban_hook) {
    if (cantidad < 1 || slot < 1) {
        return;
    }
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }
    if (npc_index < 1 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto effective_key_hook = key_hook ? key_hook : g_key_purchase_hook;
    auto effective_ban_hook = ban_hook ? ban_hook : g_ban_user_hook;

    if (modo == eModoComercio::Compra) {
        if (slot > MAX_INVENTORY_SLOTS) {
            return;
        }

        if (cantidad > MAX_INVENTORY_OBJS) {
            if (effective_ban_hook) {
                effective_ban_hook(user_index, "Intentar hackear el sistema de comercio. Quiso comprar demasiados ítems:" + std::to_string(cantidad));
            } else {
                UserList[user_index].flags.Ban = 1;
                WriteConsoleMsg(user_index, "Has sido baneado por el Sistema AntiCheat.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }

        if (Npclist[npc_index].Invent.Object[slot].Amount <= 0) {
            return;
        }

        if (cantidad > Npclist[npc_index].Invent.Object[slot].Amount) {
            cantidad = Npclist[npc_index].Invent.Object[slot].Amount;
        }

        const auto obj_index = Npclist[npc_index].Invent.Object[slot].ObjIndex;
        if (obj_index < 1 || static_cast<std::size_t>(obj_index) >= ObjDataList.size()) {
            return;
        }

        // El precio se redondea hacia arriba (+0.5)
        double unit_val = static_cast<double>(ObjDataList[obj_index].Valor) / static_cast<double>(Descuento(user_index));
        std::int64_t precio = static_cast<std::int64_t>((unit_val * static_cast<double>(cantidad)) + 0.5);

        if (static_cast<std::int64_t>(UserList[user_index].Stats.GLD) < precio) {
            WriteConsoleMsg(user_index, "No tienes suficiente dinero.", FontTypeNames::FONTTYPE_INFO);
            return;
        }

        Obj objeto{obj_index, cantidad};
        if (!InvUsuario::MeterItemEnInventario(user_index, objeto)) {
            EnviarNpcInv(user_index, npc_index);
            WriteTradeOK(user_index);
            return;
        }

        UserList[user_index].Stats.GLD -= static_cast<std::int32_t>(precio);
        Modulo_InventANDobj::QuitarNpcInvItem(npc_index, static_cast<std::uint8_t>(slot), cantidad);

        if (ObjDataList[obj_index].OBJType == eOBJType::otLlaves && effective_key_hook) {
            effective_key_hook(user_index, npc_index, static_cast<std::uint8_t>(slot), obj_index);
        }
    } else if (modo == eModoComercio::Venta) {
        if (slot > MAX_INVENTORY_SLOTS) {
            EnviarNpcInv(user_index, npc_index);
            return;
        }

        if (cantidad > UserList[user_index].Invent.Object[slot].Amount) {
            cantidad = UserList[user_index].Invent.Object[slot].Amount;
        }

        const auto obj_index = UserList[user_index].Invent.Object[slot].ObjIndex;
        if (obj_index == 0) {
            return;
        }
        if (static_cast<std::size_t>(obj_index) >= ObjDataList.size()) {
            return;
        }

        const auto npc_tipo_items = Npclist[npc_index].TipoItems;
        const auto obj_type = static_cast<std::int16_t>(ObjDataList[obj_index].OBJType);

        if ((npc_tipo_items != obj_type && npc_tipo_items != static_cast<std::int16_t>(eOBJType::otCualquiera)) || obj_index == iORO) {
            WriteConsoleMsg(user_index, "Lo siento, no estoy interesado en este tipo de objetos.", FontTypeNames::FONTTYPE_INFO);
            EnviarNpcInv(user_index, npc_index);
            WriteTradeOK(user_index);
            return;
        }

        if (ObjDataList[obj_index].Real == 1) {
            if (Npclist[npc_index].name != "SR") {
                WriteConsoleMsg(user_index, "Las armaduras del ejército real sólo pueden ser vendidas a los sastres reales.", FontTypeNames::FONTTYPE_INFO);
                EnviarNpcInv(user_index, npc_index);
                WriteTradeOK(user_index);
                return;
            }
        } else if (ObjDataList[obj_index].Caos == 1) {
            if (Npclist[npc_index].name != "SC") {
                WriteConsoleMsg(user_index, "Las armaduras de la legión oscura sólo pueden ser vendidas a los sastres del demonio.", FontTypeNames::FONTTYPE_INFO);
                EnviarNpcInv(user_index, npc_index);
                WriteTradeOK(user_index);
                return;
            }
        }

        if (UserList[user_index].Invent.Object[slot].Amount < 0 || cantidad == 0) {
            return;
        }

        if ((static_cast<std::int32_t>(UserList[user_index].flags.Privilegios) & PlayerType::Consejero) != 0) {
            WriteConsoleMsg(user_index, "No puedes vender ítems.", FontTypeNames::FONTTYPE_WARNING);
            EnviarNpcInv(user_index, npc_index);
            WriteTradeOK(user_index);
            return;
        }

        InvUsuario::QuitarUserInvItem(user_index, static_cast<std::uint8_t>(slot), cantidad);

        double unit_sale_price = static_cast<double>(SalePrice(obj_index));
        std::int64_t precio = static_cast<std::int64_t>(std::floor(unit_sale_price * static_cast<double>(cantidad)));

        std::int64_t nuevo_oro = static_cast<std::int64_t>(UserList[user_index].Stats.GLD) + precio;
        if (nuevo_oro > MAXORO) {
            nuevo_oro = MAXORO;
        }
        UserList[user_index].Stats.GLD = static_cast<std::int32_t>(nuevo_oro);

        std::int16_t npc_slot = SlotEnNPCInv(npc_index, obj_index, cantidad);
        if (npc_slot <= MAX_INVENTORY_SLOTS) {
            Npclist[npc_index].Invent.Object[npc_slot].ObjIndex = obj_index;
            std::int32_t nuevo_amount = static_cast<std::int32_t>(Npclist[npc_index].Invent.Object[npc_slot].Amount) + cantidad;
            if (nuevo_amount > MAX_INVENTORY_OBJS) {
                nuevo_amount = MAX_INVENTORY_OBJS;
            }
            Npclist[npc_index].Invent.Object[npc_slot].Amount = static_cast<std::int16_t>(nuevo_amount);
        }
    }

    InvUsuario::UpdateUserInv(true, user_index, 0);
    WriteUpdateUserStats(user_index);
    EnviarNpcInv(user_index, npc_index);
    WriteTradeOK(user_index);

    if (g_subir_skill_hook) {
        g_subir_skill_hook(user_index, eSkill::Comerciar, true);
    }
}

} // namespace ao
