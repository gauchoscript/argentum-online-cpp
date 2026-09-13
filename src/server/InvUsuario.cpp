#include "InvUsuario.hpp"
#include "Modulo_InventANDobj.hpp"
#include "Declares.hpp"
#include "Protocol.hpp"
#include "modSendData.hpp"
#include <string>

namespace InvUsuario {

namespace {

using ao::net::protocol::FontTypeNames;

QuitarUserInvItemHook s_quitar_user_inv_item_hook{nullptr};
UpdateUserInvHook s_update_user_inv_hook{nullptr};
MeterItemEnInventarioHook s_meter_item_en_inventario_hook{nullptr};
ChangeUserInvHook s_change_user_inv_hook{nullptr};
DesequiparHook s_desequipar_hook{Desequipar};
TilelibreHook s_tilelibre_hook{nullptr};
ChangeUserCharHook s_change_user_char_hook{nullptr};
DarCuerpoDesnudoHook s_dar_cuerpo_desnudo_hook{nullptr};
LearnSpellHook s_learn_spell_hook{nullptr};
NavegaHook s_navega_hook{nullptr};
WorkRequestTargetHook s_work_request_target_hook{nullptr};

inline std::size_t get_map_block_index(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return (static_cast<std::size_t>(map) * 101 + static_cast<std::size_t>(x)) * 101 + static_cast<std::size_t>(y);
}

inline bool is_valid_position(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    if (map <= 0 || x < 1 || x > 100 || y < 1 || y > 100) {
        return false;
    }
    const std::size_t idx = get_map_block_index(map, x, y);
    return idx < MapData.size();
}

} // namespace

void SetQuitarUserInvItemHook(QuitarUserInvItemHook hook) noexcept {
    s_quitar_user_inv_item_hook = hook;
}

void SetUpdateUserInvHook(UpdateUserInvHook hook) noexcept {
    s_update_user_inv_hook = hook;
}

void SetMeterItemEnInventarioHook(MeterItemEnInventarioHook hook) noexcept {
    s_meter_item_en_inventario_hook = hook;
}

void SetChangeUserInvHook(ChangeUserInvHook hook) noexcept {
    s_change_user_inv_hook = hook;
}

void SetDesequiparHook(DesequiparHook hook) noexcept {
    s_desequipar_hook = hook ? hook : Desequipar;
}

void SetTilelibreHook(TilelibreHook hook) noexcept {
    s_tilelibre_hook = hook;
}

void SetChangeUserCharHook(ChangeUserCharHook hook) noexcept {
    s_change_user_char_hook = hook;
}

void SetDarCuerpoDesnudoHook(DarCuerpoDesnudoHook hook) noexcept {
    s_dar_cuerpo_desnudo_hook = hook;
}

void SetLearnSpellHook(LearnSpellHook hook) noexcept {
    s_learn_spell_hook = hook;
}

void SetNavegaHook(NavegaHook hook) noexcept {
    s_navega_hook = hook;
}

void SetWorkRequestTargetHook(WorkRequestTargetHook hook) noexcept {
    s_work_request_target_hook = hook;
}


// ==========================================
// Fase 1: G1 — Mutaciones en el Mundo y Suelo
// ==========================================

void MakeObj(const Obj& obj, std::int16_t map, std::int16_t x, std::int16_t y) {
    if (obj.ObjIndex <= 0 || static_cast<std::size_t>(obj.ObjIndex) >= ObjDataList.size()) {
        return;
    }
    if (!is_valid_position(map, x, y)) {
        return;
    }

    const std::size_t block_idx = get_map_block_index(map, x, y);
    auto& cell = MapData[block_idx];

    if (cell.ObjInfo.ObjIndex == obj.ObjIndex) {
        // Promoción a 32 bits con signo para prevenir integer overflow / UB en C++
        const std::int32_t total = static_cast<std::int32_t>(cell.ObjInfo.Amount) +
                                   static_cast<std::int32_t>(obj.Amount);
        cell.ObjInfo.Amount = static_cast<std::int16_t>(total);
    } else {
        cell.ObjInfo = obj;
        const std::int16_t grh_index = ObjDataList[static_cast<std::size_t>(obj.ObjIndex)].GrhIndex;
        auto msg = ao::net::protocol::PrepareMessageObjectCreate(grh_index,
                                                                 static_cast<std::uint8_t>(x),
                                                                 static_cast<std::uint8_t>(y));
        ao::net::send_data::SendToAreaByPos(map, x, y, msg);
    }
}

void EraseObj(std::int16_t num, std::int16_t map, std::int16_t x, std::int16_t y) {
    if (!is_valid_position(map, x, y)) {
        return;
    }

    const std::size_t block_idx = get_map_block_index(map, x, y);
    auto& cell = MapData[block_idx];

    const std::int32_t remanente = static_cast<std::int32_t>(cell.ObjInfo.Amount) -
                                   static_cast<std::int32_t>(num);
    cell.ObjInfo.Amount = static_cast<std::int16_t>(remanente);

    if (cell.ObjInfo.Amount <= 0) {
        cell.ObjInfo.ObjIndex = 0;
        cell.ObjInfo.Amount = 0;

        auto msg = ao::net::protocol::PrepareMessageObjectDelete(static_cast<std::uint8_t>(x),
                                                                 static_cast<std::uint8_t>(y));
        ao::net::send_data::SendToAreaByPos(map, x, y, msg);
    }
}

void DropObj(std::int16_t user_index, std::uint8_t slot, std::int16_t num, std::int16_t map, std::int16_t x, std::int16_t y) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }
    if (slot < 1 || slot > 30) {
        return;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];
    if (num <= 0) {
        return;
    }

    if (num > user.Invent.Object[slot].Amount) {
        num = user.Invent.Object[slot].Amount;
    }

    Obj obj{};
    obj.ObjIndex = user.Invent.Object[slot].ObjIndex;
    obj.Amount = num;

    if (obj.ObjIndex <= 0 || static_cast<std::size_t>(obj.ObjIndex) >= ObjDataList.size()) {
        return;
    }

    const auto& item_data = ObjDataList[static_cast<std::size_t>(obj.ObjIndex)];

    // Validación de objeto no caíble
    if (item_data.NoSeCae == 1) {
        ao::net::protocol::WriteConsoleMsg(user_index, "No puedes tirar este objeto.", FontTypeNames::FONTTYPE_INFO);
        return;
    }

    // Validación de objeto newbie para usuarios comunes
    if (item_data.Newbie == 1 && (user.flags.Privilegios & PlayerType::UserPlayer)) {
        ao::net::protocol::WriteConsoleMsg(user_index, "No puedes tirar objetos newbie.", FontTypeNames::FONTTYPE_INFO);
        return;
    }

    // Comprobación de ocupación en la celda del mapa del usuario (incoherencia de parámetro legacy: MapData(.Pos.Map, X, Y))
    const std::int16_t check_map = user.Pos.Map;
    if (!is_valid_position(check_map, x, y)) {
        return;
    }
    const std::size_t check_idx = get_map_block_index(check_map, x, y);
    const auto& ground_cell = MapData[check_idx];

    if (ground_cell.ObjInfo.ObjIndex == 0 || ground_cell.ObjInfo.ObjIndex == obj.ObjIndex) {
        // REPLICACIÓN ESTRICTA DEL BUG #29:
        // Si el total acumulado en el suelo supera MAX_INVENTORY_OBJS (10.000), solo se recorta la variable local `num`.
        // La estructura `obj` retiene su `obj.Amount` original completo.
        if (static_cast<std::int32_t>(num) + static_cast<std::int32_t>(ground_cell.ObjInfo.Amount) > MAX_INVENTORY_OBJS) {
            num = static_cast<std::int16_t>(MAX_INVENTORY_OBJS - ground_cell.ObjInfo.Amount);
        }

        // MakeObj recibe la estructura `obj` con el monto original pretendido
        MakeObj(obj, map, x, y);

        // QuitarUserInvItem descuenta únicamente la variable `num` recortada
        if (s_quitar_user_inv_item_hook) {
            s_quitar_user_inv_item_hook(user_index, slot, num);
        } else {
            QuitarUserInvItem(user_index, slot, num);
        }

        if (s_update_user_inv_hook) {
            s_update_user_inv_hook(false, user_index, slot);
        } else {
            UpdateUserInv(false, user_index, slot);
        }

        if (item_data.OBJType == eOBJType::otBarcos) {
            ao::net::protocol::WriteConsoleMsg(user_index, "¡¡ATENCIÓN!! ¡ACABAS DE TIRAR TU BARCA!", FontTypeNames::FONTTYPE_TALK);
        }
    } else {
        ao::net::protocol::WriteConsoleMsg(user_index, "No hay espacio en el piso.", FontTypeNames::FONTTYPE_INFO);
    }
}

void GetObj(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];
    const std::int16_t map = user.Pos.Map;
    const std::int16_t x = user.Pos.X;
    const std::int16_t y = user.Pos.Y;

    if (!is_valid_position(map, x, y)) {
        return;
    }

    const std::size_t block_idx = get_map_block_index(map, x, y);
    auto& cell = MapData[block_idx];

    if (cell.ObjInfo.ObjIndex > 0) {
        const std::int16_t obj_index = cell.ObjInfo.ObjIndex;
        if (obj_index <= 0 || static_cast<std::size_t>(obj_index) >= ObjDataList.size()) {
            return;
        }

        const auto& item_data = ObjDataList[static_cast<std::size_t>(obj_index)];

        // En VB6: If ObjData(...).Agarrable <> 1 Then (si Agarrable == 1, no se puede agarrar)
        if (item_data.Agarrable == 1) {
            ao::net::protocol::WriteConsoleMsg(user_index, "No puedes agarrar este objeto.", FontTypeNames::FONTTYPE_INFO);
            return;
        }

        Obj item_on_ground = cell.ObjInfo;

        // Oro directo a la billetera
        if (item_data.OBJType == eOBJType::otGuita) {
            user.Stats.GLD += item_on_ground.Amount;
            EraseObj(item_on_ground.Amount, map, x, y);
            ao::net::protocol::WriteUpdateGold(user_index);
            ao::net::protocol::WriteConsoleMsg(user_index, "Has recogido " + std::to_string(item_on_ground.Amount) + " monedas de oro.", FontTypeNames::FONTTYPE_INFO);
        } else {
            bool metido = false;
            if (s_meter_item_en_inventario_hook) {
                metido = s_meter_item_en_inventario_hook(user_index, item_on_ground);
            } else {
                metido = MeterItemEnInventario(user_index, item_on_ground);
            }

            if (metido) {
                EraseObj(item_on_ground.Amount, map, x, y);
            }
        }
    } else {
        ao::net::protocol::WriteConsoleMsg(user_index, "No hay nada aquí.", FontTypeNames::FONTTYPE_INFO);
    }
}

// ====================================================
// Fase 2: G2 — Gestión Base de Inventario y Descarte
// ====================================================

bool MeterItemEnInventario(std::int16_t user_index, Obj& mi_obj) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return false;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];

    // 1. ¿El usuario ya tiene un objeto del mismo tipo donde entre la cantidad?
    std::uint8_t slot = 1;
    while (slot <= user.CurrentInventorySlots) {
        if (user.Invent.Object[slot].ObjIndex == mi_obj.ObjIndex &&
            static_cast<std::int32_t>(user.Invent.Object[slot].Amount) + static_cast<std::int32_t>(mi_obj.Amount) <= MAX_INVENTORY_OBJS) {
            break;
        }
        slot++;
    }

    // 2. Si no encontró, busca un slot vacío
    if (slot > user.CurrentInventorySlots) {
        slot = 1;
        while (slot <= user.CurrentInventorySlots && user.Invent.Object[slot].ObjIndex != 0) {
            slot++;
        }

        if (slot > user.CurrentInventorySlots) {
            ao::net::protocol::WriteConsoleMsg(user_index, "No puedes cargar más objetos.", FontTypeNames::FONTTYPE_FIGHT);
            return false;
        }

        user.Invent.NroItems++;
    }

    // 3. Validación de mochila expandida (slots 21 a 30)
    if (slot > MAX_NORMAL_INVENTORY_SLOTS && slot <= MAX_INVENTORY_SLOTS) {
        if (!ItemSeCae(mi_obj.ObjIndex)) {
            std::string bag_name = "mochila";
            if (user.Invent.MochilaEqpObjIndex > 0 &&
                static_cast<std::size_t>(user.Invent.MochilaEqpObjIndex) < ObjDataList.size()) {
                bag_name = ObjDataList[static_cast<std::size_t>(user.Invent.MochilaEqpObjIndex)].name;
            }
            ao::net::protocol::WriteConsoleMsg(user_index, "No puedes contener objetos especiales en tu " + bag_name + ".", FontTypeNames::FONTTYPE_FIGHT);
            return false;
        }
    }

    // 4. Inserción de la cantidad en el slot
    if (static_cast<std::int32_t>(user.Invent.Object[slot].Amount) + static_cast<std::int32_t>(mi_obj.Amount) <= MAX_INVENTORY_OBJS) {
        user.Invent.Object[slot].ObjIndex = mi_obj.ObjIndex;
        user.Invent.Object[slot].Amount = static_cast<std::int16_t>(user.Invent.Object[slot].Amount + mi_obj.Amount);
    } else {
        user.Invent.Object[slot].ObjIndex = mi_obj.ObjIndex;
        user.Invent.Object[slot].Amount = MAX_INVENTORY_OBJS;
    }

    UpdateUserInv(false, user_index, slot);
    return true;
}

/**
 * @brief Descuenta cantidad de un slot del usuario y desequipa si llega a 0.
 *
 * NOTA DE ARQUITECTURA (Grilla Fija y Dispersa):
 * En Argentum Online, el inventario opera estrictamente como una grilla fija y dispersa
 * de 30 ranuras. Está terminantemente PROHIBIDO compactar la lista o correr los slots
 * a la izquierda con corrimientos o vector::erase. Cuando una ranura llega a Amount <= 0,
 * se blanquea en su posición exacta, se decrementa NroItems y se emite UpdateUserInv
 * para esa ranura específica, preservando los índices del resto de las ranuras.
 */
void QuitarUserInvItem(std::int16_t user_index, std::uint8_t slot, std::int16_t cantidad) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];
    if (slot < 1 || slot > user.CurrentInventorySlots) {
        return;
    }

    auto& item = user.Invent.Object[slot];

    if (item.Amount <= cantidad && item.Equipped == 1) {
        if (s_desequipar_hook) {
            s_desequipar_hook(user_index, slot);
        } else {
            item.Equipped = 0;
        }
    }

    item.Amount = static_cast<std::int16_t>(item.Amount - cantidad);

    if (item.Amount <= 0) {
        user.Invent.NroItems--;
        item.ObjIndex = 0;
        item.Amount = 0;
        item.Equipped = 0;
    }

    UpdateUserInv(false, user_index, slot);
}

void UpdateUserInv(bool update_all, std::int16_t user_index, std::uint8_t slot) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const auto& user = UserList[static_cast<std::size_t>(user_index)];
    UserOBJ null_obj{};

    if (!update_all) {
        if (slot >= 1 && slot <= user.CurrentInventorySlots) {
            const auto& item = (user.Invent.Object[slot].ObjIndex > 0) ? user.Invent.Object[slot] : null_obj;
            if (s_change_user_inv_hook) {
                s_change_user_inv_hook(user_index, slot, item);
            } else {
                ao::net::protocol::WriteChangeInventorySlot(user_index, slot);
            }
        }
    } else {
        for (std::uint8_t loop_c = 1; loop_c <= user.CurrentInventorySlots; ++loop_c) {
            const auto& item = (user.Invent.Object[loop_c].ObjIndex > 0) ? user.Invent.Object[loop_c] : null_obj;
            if (s_change_user_inv_hook) {
                s_change_user_inv_hook(user_index, loop_c, item);
            } else {
                ao::net::protocol::WriteChangeInventorySlot(user_index, loop_c);
            }
        }
    }
}

void LimpiarInventario(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];

    for (std::size_t i = 1; i <= MAX_INVENTORY_SLOTS; ++i) {
        user.Invent.Object[i] = UserOBJ{};
    }

    user.Invent.NroItems = 0;

    user.Invent.ArmourEqpObjIndex = 0;
    user.Invent.ArmourEqpSlot = 0;

    user.Invent.WeaponEqpObjIndex = 0;
    user.Invent.WeaponEqpSlot = 0;

    user.Invent.CascoEqpObjIndex = 0;
    user.Invent.CascoEqpSlot = 0;

    user.Invent.EscudoEqpObjIndex = 0;
    user.Invent.EscudoEqpSlot = 0;

    user.Invent.AnilloEqpObjIndex = 0;
    user.Invent.AnilloEqpSlot = 0;

    user.Invent.MunicionEqpObjIndex = 0;
    user.Invent.MunicionEqpSlot = 0;

    user.Invent.BarcoObjIndex = 0;
    user.Invent.BarcoSlot = 0;

    user.Invent.MochilaEqpObjIndex = 0;
    user.Invent.MochilaEqpSlot = 0;
}

bool TieneObjetosRobables(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return false;
    }

    const auto& user = UserList[static_cast<std::size_t>(user_index)];

    for (std::uint8_t i = 1; i <= user.CurrentInventorySlots; ++i) {
        const std::int16_t obj_index = user.Invent.Object[i].ObjIndex;
        if (obj_index > 0 && static_cast<std::size_t>(obj_index) < ObjDataList.size()) {
            const auto& data = ObjDataList[static_cast<std::size_t>(obj_index)];
            if (data.OBJType != eOBJType::otLlaves && data.OBJType != eOBJType::otBarcos) {
                return true;
            }
        }
    }
    return false;
}

void QuitarNewbieObj(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const auto& user = UserList[static_cast<std::size_t>(user_index)];

    for (std::uint8_t j = 1; j <= user.CurrentInventorySlots; ++j) {
        if (user.Invent.Object[j].ObjIndex > 0) {
            if (ItemNewbie(user.Invent.Object[j].ObjIndex)) {
                QuitarUserInvItem(user_index, j, MAX_INVENTORY_OBJS);
            }
            // Quirk histórico de VB6: UpdateUserInv se ejecuta INCONDICIONALMENTE
            // para todo slot con ObjIndex > 0 (debido al salto de línea con guión bajo en el legacy).
            UpdateUserInv(false, user_index, j);
        }
    }
}

bool ItemSeCae(std::int16_t index) {
    // Transliteración fiel de InvUsuario.bas:1579-1594
    if (index <= 0) {
        return false;
    }
    if (NumObjDatas > 0 && index > NumObjDatas) {
        return false;
    }
    if (static_cast<std::size_t>(index) >= ObjDataList.size()) {
        return false;
    }

    const auto& data = ObjDataList[static_cast<std::size_t>(index)];
    if (data.NoSeCae == 1) {
        return false;
    }
    if (data.Real == 1 || data.Caos == 1) {
        return false;
    }
    if (data.OBJType == eOBJType::otLlaves || data.OBJType == eOBJType::otBarcos) {
        return false;
    }
    return true;
}

bool ItemNewbie(std::int16_t item_index) {
    if (item_index <= 0 || static_cast<std::size_t>(item_index) >= ObjDataList.size()) {
        return false;
    }
    return ObjDataList[static_cast<std::size_t>(item_index)].Newbie == 1;
}

eOBJType getObjType(std::int16_t obj_index) {
    if (obj_index <= 0 || static_cast<std::size_t>(obj_index) >= ObjDataList.size()) {
        return eOBJType::otUseOnce;
    }
    return ObjDataList[static_cast<std::size_t>(obj_index)].OBJType;
}

void TirarTodo(std::int16_t user_index) {
    // Transliteración fiel de InvUsuario.bas:1556-1577
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];

    if (is_valid_position(user.Pos.Map, user.Pos.X, user.Pos.Y)) {
        const std::size_t idx = get_map_block_index(user.Pos.Map, user.Pos.X, user.Pos.Y);
        if (static_cast<std::int32_t>(MapData[idx].trigger) == 6 || MapData[idx].trigger == eTrigger::ZONAPELEA) {
            return;
        }
    }

    TirarTodosLosItems(user_index);

    const std::int32_t oro_protegido = static_cast<std::int32_t>(user.Stats.ELV) * 10000;
    if (user.Stats.GLD > oro_protegido) {
        TirarOro(user.Stats.GLD - oro_protegido, user_index);
    }
}

void TirarTodosLosItems(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];

    for (std::uint8_t i = 1; i <= user.CurrentInventorySlots; ++i) {
        const std::int16_t item_index = user.Invent.Object[i].ObjIndex;
        if (item_index > 0 && ItemSeCae(item_index)) {
            WorldPos nueva_pos{0, 0, 0};
            Obj mi_obj{item_index, user.Invent.Object[i].Amount};

            bool drop_agua = true;
            if (user.clase == eClass::Pirat && user.Invent.BarcoObjIndex == 476) {
                if (user.Stats.ELV >= 20 && user.Stats.ELV <= 25) {
                    drop_agua = false;
                }
            }

            if (s_tilelibre_hook) {
                s_tilelibre_hook(user.Pos, nueva_pos, mi_obj, drop_agua, true);
            } else {
                nueva_pos = Modulo_InventANDobj::TirarItemAlPiso(user.Pos, mi_obj, drop_agua);
            }

            if (nueva_pos.X != 0 && nueva_pos.Y != 0) {
                // Quirk de VB6: pasa MAX_INVENTORY_OBJS como parámetro num hacia DropObj
                DropObj(user_index, i, MAX_INVENTORY_OBJS, nueva_pos.Map, nueva_pos.X, nueva_pos.Y);
            }
        }
    }
}

void TirarTodosLosItemsNoNewbies(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];

    if (is_valid_position(user.Pos.Map, user.Pos.X, user.Pos.Y)) {
        const std::size_t idx = get_map_block_index(user.Pos.Map, user.Pos.X, user.Pos.Y);
        if (MapData[idx].trigger == eTrigger::ZONAPELEA) {
            return;
        }
    }

    for (std::uint8_t i = 1; i <= user.CurrentInventorySlots; ++i) {
        const std::int16_t item_index = user.Invent.Object[i].ObjIndex;
        if (item_index > 0 && ItemSeCae(item_index) && !ItemNewbie(item_index)) {
            WorldPos nueva_pos{0, 0, 0};
            Obj mi_obj{item_index, user.Invent.Object[i].Amount};

            if (s_tilelibre_hook) {
                s_tilelibre_hook(user.Pos, nueva_pos, mi_obj, true, true);
            } else {
                nueva_pos = Modulo_InventANDobj::TirarItemAlPiso(user.Pos, mi_obj, true);
            }

            if (nueva_pos.X != 0 && nueva_pos.Y != 0) {
                DropObj(user_index, i, MAX_INVENTORY_OBJS, nueva_pos.Map, nueva_pos.X, nueva_pos.Y);
            }
        }
    }
}

void TirarTodosLosItemsEnMochila(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];

    if (is_valid_position(user.Pos.Map, user.Pos.X, user.Pos.Y)) {
        const std::size_t idx = get_map_block_index(user.Pos.Map, user.Pos.X, user.Pos.Y);
        if (MapData[idx].trigger == eTrigger::ZONAPELEA) {
            return;
        }
    }

    for (std::uint8_t i = MAX_NORMAL_INVENTORY_SLOTS + 1; i <= user.CurrentInventorySlots; ++i) {
        const std::int16_t item_index = user.Invent.Object[i].ObjIndex;
        if (item_index > 0 && ItemSeCae(item_index)) {
            WorldPos nueva_pos{0, 0, 0};
            Obj mi_obj{item_index, user.Invent.Object[i].Amount};

            if (s_tilelibre_hook) {
                s_tilelibre_hook(user.Pos, nueva_pos, mi_obj, true, true);
            } else {
                nueva_pos = Modulo_InventANDobj::TirarItemAlPiso(user.Pos, mi_obj, true);
            }

            if (nueva_pos.X != 0 && nueva_pos.Y != 0) {
                DropObj(user_index, i, MAX_INVENTORY_OBJS, nueva_pos.Map, nueva_pos.X, nueva_pos.Y);
            }
        }
    }
}

void TirarOro(std::int32_t cantidad, std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];
    if (cantidad <= 0 || cantidad > user.Stats.GLD) {
        return;
    }

    std::int32_t extra = 0;
    const std::int32_t tenia_oro = user.Stats.GLD;

    // Salvaguarda histórica contra desbordes (> 500k)
    if (cantidad > 500000) {
        extra = cantidad - 500000;
        cantidad = 500000;
    }

    std::int32_t loops = 0;
    while (cantidad > 0) {
        Obj mi_obj{};
        mi_obj.ObjIndex = iORO;

        if (cantidad > MAX_INVENTORY_OBJS && user.Stats.GLD > MAX_INVENTORY_OBJS) {
            mi_obj.Amount = MAX_INVENTORY_OBJS;
        } else {
            mi_obj.Amount = static_cast<std::int16_t>(cantidad);
        }

        cantidad -= mi_obj.Amount;

        // Quirk de barco pirata: not_pirata = false únicamente si BarcoObjIndex == 476
        bool drop_agua = true;
        if (user.clase == eClass::Pirat && user.Invent.BarcoObjIndex == 476) {
            drop_agua = false;
        }

        WorldPos aux_pos = Modulo_InventANDobj::TirarItemAlPiso(user.Pos, mi_obj, drop_agua);
        if (aux_pos.X != 0 && aux_pos.Y != 0) {
            user.Stats.GLD -= mi_obj.Amount;
        }

        loops++;
        if (loops > 100) {
            break;
        }
    }

    // REPLICACIÓN ESTRICTA DEL BUG #30:
    // Si la billetera cambió (se tiró al menos una pila de oro),
    // el saldo `extra` (> 500k) se deduce directamente de la billetera sin arrojarse al suelo.
    if (tenia_oro == user.Stats.GLD) {
        extra = 0;
    }
    if (extra > 0) {
        user.Stats.GLD -= extra;
    }
}

// ====================================================
// Fase 3: G3 — Restricciones y Sistema de Equipamiento
// ====================================================

namespace {

inline bool esArmada(std::int16_t user_index) noexcept {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) return false;
    return UserList[static_cast<std::size_t>(user_index)].Faccion.ArmadaReal == 1;
}

inline bool esCaos(std::int16_t user_index) noexcept {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) return false;
    return UserList[static_cast<std::size_t>(user_index)].Faccion.FuerzasCaos == 1;
}

inline bool EsGM(std::int16_t user_index) noexcept {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) return false;
    return (UserList[static_cast<std::size_t>(user_index)].flags.Privilegios &
            (PlayerType::Admin | PlayerType::Dios | PlayerType::SemiDios | PlayerType::Consejero)) != 0;
}

inline bool criminal_check(std::int16_t user_index) noexcept {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) return false;
    const auto& rep = UserList[static_cast<std::size_t>(user_index)].Reputacion;
    long l = (-rep.AsesinoRep) +
             (-rep.BandidoRep) +
             rep.BurguesRep +
             (-rep.LadronesRep) +
             rep.NobleRep +
             rep.PlebeRep;
    l = l / 6;
    return (l < 0);
}

inline bool EsNewbie(std::int16_t user_index) noexcept {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) return false;
    return UserList[static_cast<std::size_t>(user_index)].Stats.ELV <= LimiteNewbie;
}

inline std::int16_t GetWeaponAnim(std::int16_t user_index, std::int16_t obj_index) noexcept {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) return 0;
    if (obj_index <= 0 || static_cast<std::size_t>(obj_index) >= ObjDataList.size()) return 0;

    const auto& user = UserList[static_cast<std::size_t>(user_index)];
    const auto& obj = ObjDataList[static_cast<std::size_t>(obj_index)];

    if (obj.WeaponRazaEnanaAnim > 0) {
        if (user.raza == eRaza::Enano || user.raza == eRaza::Gnomo) {
            return obj.WeaponRazaEnanaAnim;
        }
    }
    return obj.WeaponAnim;
}

} // namespace

bool ClasePuedeUsarItem(std::int16_t user_index, std::int16_t obj_index, std::string* motivo) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return false;
    }
    if (obj_index <= 0 || static_cast<std::size_t>(obj_index) >= ObjDataList.size()) {
        return false;
    }

    const auto& user = UserList[static_cast<std::size_t>(user_index)];
    const auto& obj = ObjDataList[static_cast<std::size_t>(obj_index)];

    // Administradores y GMs pueden usar cualquier ítem
    if ((user.flags.Privilegios & PlayerType::UserPlayer) != 0) {
        if (static_cast<std::int32_t>(obj.ClaseProhibida[1]) != 0) {
            for (int i = 1; i <= NUMCLASES; ++i) {
                if (obj.ClaseProhibida[i] == user.clase) {
                    if (motivo) {
                        *motivo = "Tu clase no puede usar este objeto.";
                    }
                    return false;
                }
            }
        }
    }

    return true;
}

bool SexoPuedeUsarItem(std::int16_t user_index, std::int16_t obj_index, std::string* motivo) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return false;
    }
    if (obj_index <= 0 || static_cast<std::size_t>(obj_index) >= ObjDataList.size()) {
        return false;
    }

    const auto& user = UserList[static_cast<std::size_t>(user_index)];
    const auto& obj = ObjDataList[static_cast<std::size_t>(obj_index)];

    bool ok = true;
    if (obj.Mujer == 1) {
        ok = (user.Genero != eGenero::Hombre);
    } else if (obj.Hombre == 1) {
        ok = (user.Genero != eGenero::Mujer);
    }

    if (!ok) {
        if (motivo) {
            *motivo = "Tu género no puede usar este objeto.";
        }
        return false;
    }
    return true;
}

bool FaccionPuedeUsarItem(std::int16_t user_index, std::int16_t obj_index, std::string* motivo) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return false;
    }
    if (obj_index <= 0 || static_cast<std::size_t>(obj_index) >= ObjDataList.size()) {
        return false;
    }

    const auto& obj = ObjDataList[static_cast<std::size_t>(obj_index)];
    bool ok = true;

    if (obj.Real == 1) {
        if (!criminal_check(user_index)) {
            ok = esArmada(user_index);
        } else {
            ok = false;
        }
    } else if (obj.Caos == 1) {
        if (criminal_check(user_index)) {
            ok = esCaos(user_index);
        } else {
            ok = false;
        }
    }

    if (!ok) {
        if (motivo) {
            *motivo = "Tu alineación no puede usar este objeto.";
        }
        return false;
    }
    return true;
}

bool CheckRazaUsaRopa(std::int16_t user_index, std::int16_t item_index, std::string* motivo) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return false;
    }
    if (item_index <= 0 || static_cast<std::size_t>(item_index) >= ObjDataList.size()) {
        return false;
    }

    const auto& user = UserList[static_cast<std::size_t>(user_index)];
    const auto& obj = ObjDataList[static_cast<std::size_t>(item_index)];

    bool ok = true;
    if (user.raza == eRaza::Humano || user.raza == eRaza::Elfo || user.raza == eRaza::Drow) {
        ok = (obj.RazaEnana == 0);
    } else {
        ok = (obj.RazaEnana == 1);
    }

    if (user.raza != eRaza::Drow && obj.RazaDrow == 1) {
        ok = false;
    }

    if (!ok) {
        if (motivo) {
            *motivo = "Tu raza no puede usar este objeto.";
        }
        return false;
    }
    return true;
}

void EquiparInvItem(std::int16_t user_index, std::uint8_t slot) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];
    if (slot < 1 || slot > user.CurrentInventorySlots) {
        return;
    }

    const std::int16_t obj_index = user.Invent.Object[slot].ObjIndex;
    if (obj_index <= 0 || static_cast<std::size_t>(obj_index) >= ObjDataList.size()) {
        return;
    }

    const auto& obj = ObjDataList[static_cast<std::size_t>(obj_index)];

    if (obj.Newbie == 1 && !EsNewbie(user_index)) {
        ao::net::protocol::WriteConsoleMsg(user_index, "Sólo los newbies pueden usar este objeto.", FontTypeNames::FONTTYPE_INFO);
        return;
    }

    // Validación de atributos mínimos (Fuerza y Agilidad)
    if (obj.MinFuerza > 0 && user.Stats.UserAtributos[eAtributos::Fuerza] < obj.MinFuerza) {
        ao::net::protocol::WriteConsoleMsg(user_index, "No tienes suficiente fuerza para usar este objeto.", FontTypeNames::FONTTYPE_INFO);
        return;
    }
    if (obj.MinAgilidad > 0 && user.Stats.UserAtributos[eAtributos::Agilidad] < obj.MinAgilidad) {
        ao::net::protocol::WriteConsoleMsg(user_index, "No tienes suficiente agilidad para usar este objeto.", FontTypeNames::FONTTYPE_INFO);
        return;
    }

    std::string motivo;

    switch (obj.OBJType) {
        case eOBJType::otWeapon: {
            if (ClasePuedeUsarItem(user_index, obj_index, &motivo) &&
                FaccionPuedeUsarItem(user_index, obj_index, &motivo)) {
                
                // Toggle: si ya está equipado, lo quita
                if (user.Invent.Object[slot].Equipped == 1) {
                    Desequipar(user_index, slot);
                    if (user.flags.Mimetizado == 1) {
                        user.CharMimetizado.WeaponAnim = NingunArma;
                    } else {
                        user.char_appearance.WeaponAnim = NingunArma;
                        if (s_change_user_char_hook) {
                            s_change_user_char_hook(user_index, user.char_appearance.body, user.char_appearance.Head,
                                                    static_cast<std::int16_t>(user.char_appearance.heading),
                                                    user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim,
                                                    user.char_appearance.CascoAnim);
                        }
                    }
                    return;
                }

                // Si tiene escudo equipado y el arma es a dos manos, desequipa el escudo
                if (obj.DosManos == 1 && user.Invent.EscudoEqpSlot > 0) {
                    Desequipar(user_index, user.Invent.EscudoEqpSlot);
                }

                // Desequipa el arma anterior si existía
                if (user.Invent.WeaponEqpObjIndex > 0 && user.Invent.WeaponEqpSlot > 0) {
                    Desequipar(user_index, user.Invent.WeaponEqpSlot);
                }

                user.Invent.Object[slot].Equipped = 1;
                user.Invent.WeaponEqpObjIndex = obj_index;
                user.Invent.WeaponEqpSlot = slot;

                if (user.flags.AdminInvisible != 1) {
                    auto wave = ao::net::protocol::PrepareMessagePlayWave(SND_SACARARMA,
                                                                         static_cast<std::uint8_t>(user.Pos.X),
                                                                         static_cast<std::uint8_t>(user.Pos.Y));
                    ao::net::send_data::SendToAreaByPos(user.Pos.Map, user.Pos.X, user.Pos.Y, wave);
                }

                const std::int16_t weapon_anim = GetWeaponAnim(user_index, obj_index);
                if (user.flags.Mimetizado == 1) {
                    user.CharMimetizado.WeaponAnim = weapon_anim;
                } else {
                    user.char_appearance.WeaponAnim = weapon_anim;
                    if (s_change_user_char_hook) {
                        s_change_user_char_hook(user_index, user.char_appearance.body, user.char_appearance.Head,
                                                static_cast<std::int16_t>(user.char_appearance.heading),
                                                user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim,
                                                user.char_appearance.CascoAnim);
                    }
                }
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, motivo, FontTypeNames::FONTTYPE_INFO);
                return;
            }
            break;
        }

        case eOBJType::otAnillo: {
            if (ClasePuedeUsarItem(user_index, obj_index, &motivo) &&
                FaccionPuedeUsarItem(user_index, obj_index, &motivo)) {
                
                if (user.Invent.Object[slot].Equipped == 1) {
                    Desequipar(user_index, slot);
                    return;
                }

                if (user.Invent.AnilloEqpObjIndex > 0 && user.Invent.AnilloEqpSlot > 0) {
                    Desequipar(user_index, user.Invent.AnilloEqpSlot);
                }

                user.Invent.Object[slot].Equipped = 1;
                user.Invent.AnilloEqpObjIndex = obj_index;
                user.Invent.AnilloEqpSlot = slot;
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, motivo, FontTypeNames::FONTTYPE_INFO);
                return;
            }
            break;
        }

        case eOBJType::otFlechas: {
            if (ClasePuedeUsarItem(user_index, obj_index, &motivo) &&
                FaccionPuedeUsarItem(user_index, obj_index, &motivo)) {
                
                if (user.Invent.Object[slot].Equipped == 1) {
                    Desequipar(user_index, slot);
                    return;
                }

                if (user.Invent.MunicionEqpObjIndex > 0 && user.Invent.MunicionEqpSlot > 0) {
                    Desequipar(user_index, user.Invent.MunicionEqpSlot);
                }

                user.Invent.Object[slot].Equipped = 1;
                user.Invent.MunicionEqpObjIndex = obj_index;
                user.Invent.MunicionEqpSlot = slot;
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, motivo, FontTypeNames::FONTTYPE_INFO);
                return;
            }
            break;
        }

        case eOBJType::otArmadura: {
            if (user.flags.Navegando == 1) {
                return;
            }

            if (ClasePuedeUsarItem(user_index, obj_index, &motivo) &&
                SexoPuedeUsarItem(user_index, obj_index, &motivo) &&
                CheckRazaUsaRopa(user_index, obj_index, &motivo) &&
                FaccionPuedeUsarItem(user_index, obj_index, &motivo)) {
                
                if (user.Invent.Object[slot].Equipped == 1) {
                    Desequipar(user_index, slot);
                    if (s_dar_cuerpo_desnudo_hook) {
                        user.char_appearance.body = s_dar_cuerpo_desnudo_hook(user_index, user.flags.Mimetizado == 1);
                    }
                    if (user.flags.Mimetizado != 1) {
                        if (s_change_user_char_hook) {
                            s_change_user_char_hook(user_index, user.char_appearance.body, user.char_appearance.Head,
                                                    static_cast<std::int16_t>(user.char_appearance.heading),
                                                    user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim,
                                                    user.char_appearance.CascoAnim);
                        }
                    }
                    return;
                }

                if (user.Invent.ArmourEqpObjIndex > 0 && user.Invent.ArmourEqpSlot > 0) {
                    Desequipar(user_index, user.Invent.ArmourEqpSlot);
                }

                user.Invent.Object[slot].Equipped = 1;
                user.Invent.ArmourEqpObjIndex = obj_index;
                user.Invent.ArmourEqpSlot = slot;

                if (user.flags.Mimetizado == 1) {
                    user.CharMimetizado.body = obj.Ropaje;
                } else {
                    user.char_appearance.body = obj.Ropaje;
                    if (s_change_user_char_hook) {
                        s_change_user_char_hook(user_index, user.char_appearance.body, user.char_appearance.Head,
                                                static_cast<std::int16_t>(user.char_appearance.heading),
                                                user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim,
                                                user.char_appearance.CascoAnim);
                    }
                }
                user.flags.Desnudo = 0;
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, motivo, FontTypeNames::FONTTYPE_INFO);
                return;
            }
            break;
        }

        case eOBJType::otCASCO: {
            if (user.flags.Navegando == 1) {
                return;
            }

            if (ClasePuedeUsarItem(user_index, obj_index, &motivo)) {
                if (user.Invent.Object[slot].Equipped == 1) {
                    Desequipar(user_index, slot);
                    if (user.flags.Mimetizado == 1) {
                        user.CharMimetizado.CascoAnim = NingunCasco;
                    } else {
                        user.char_appearance.CascoAnim = NingunCasco;
                        if (s_change_user_char_hook) {
                            s_change_user_char_hook(user_index, user.char_appearance.body, user.char_appearance.Head,
                                                    static_cast<std::int16_t>(user.char_appearance.heading),
                                                    user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim,
                                                    user.char_appearance.CascoAnim);
                        }
                    }
                    return;
                }

                if (user.Invent.CascoEqpObjIndex > 0 && user.Invent.CascoEqpSlot > 0) {
                    Desequipar(user_index, user.Invent.CascoEqpSlot);
                }

                user.Invent.Object[slot].Equipped = 1;
                user.Invent.CascoEqpObjIndex = obj_index;
                user.Invent.CascoEqpSlot = slot;

                if (user.flags.Mimetizado == 1) {
                    user.CharMimetizado.CascoAnim = obj.CascoAnim;
                } else {
                    user.char_appearance.CascoAnim = obj.CascoAnim;
                    if (s_change_user_char_hook) {
                        s_change_user_char_hook(user_index, user.char_appearance.body, user.char_appearance.Head,
                                                static_cast<std::int16_t>(user.char_appearance.heading),
                                                user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim,
                                                user.char_appearance.CascoAnim);
                    }
                }
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, motivo, FontTypeNames::FONTTYPE_INFO);
                return;
            }
            break;
        }

        case eOBJType::otESCUDO: {
            if (user.flags.Navegando == 1) {
                return;
            }

            // Regla dos manos vs escudo: bloquea equipar escudo si hay arma a dos manos en uso
            if (user.Invent.WeaponEqpObjIndex > 0 &&
                static_cast<std::size_t>(user.Invent.WeaponEqpObjIndex) < ObjDataList.size()) {
                if (ObjDataList[static_cast<std::size_t>(user.Invent.WeaponEqpObjIndex)].DosManos == 1) {
                    ao::net::protocol::WriteConsoleMsg(user_index, "No puedes usar un escudo con un arma a dos manos.", FontTypeNames::FONTTYPE_INFO);
                    return;
                }
            }

            if (ClasePuedeUsarItem(user_index, obj_index, &motivo) &&
                FaccionPuedeUsarItem(user_index, obj_index, &motivo)) {
                
                if (user.Invent.Object[slot].Equipped == 1) {
                    Desequipar(user_index, slot);
                    if (user.flags.Mimetizado == 1) {
                        user.CharMimetizado.ShieldAnim = NingunEscudo;
                    } else {
                        user.char_appearance.ShieldAnim = NingunEscudo;
                        if (s_change_user_char_hook) {
                            s_change_user_char_hook(user_index, user.char_appearance.body, user.char_appearance.Head,
                                                    static_cast<std::int16_t>(user.char_appearance.heading),
                                                    user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim,
                                                    user.char_appearance.CascoAnim);
                        }
                    }
                    return;
                }

                if (user.Invent.EscudoEqpObjIndex > 0 && user.Invent.EscudoEqpSlot > 0) {
                    Desequipar(user_index, user.Invent.EscudoEqpSlot);
                }

                user.Invent.Object[slot].Equipped = 1;
                user.Invent.EscudoEqpObjIndex = obj_index;
                user.Invent.EscudoEqpSlot = slot;

                if (user.flags.Mimetizado == 1) {
                    user.CharMimetizado.ShieldAnim = obj.ShieldAnim;
                } else {
                    user.char_appearance.ShieldAnim = obj.ShieldAnim;
                    if (s_change_user_char_hook) {
                        s_change_user_char_hook(user_index, user.char_appearance.body, user.char_appearance.Head,
                                                static_cast<std::int16_t>(user.char_appearance.heading),
                                                user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim,
                                                user.char_appearance.CascoAnim);
                    }
                }
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, motivo, FontTypeNames::FONTTYPE_INFO);
                return;
            }
            break;
        }

        case eOBJType::otBarcos: {
            if (user.Invent.Object[slot].Equipped == 1) {
                Desequipar(user_index, slot);
                return;
            }

            if (user.Invent.BarcoObjIndex > 0 && user.Invent.BarcoSlot > 0) {
                Desequipar(user_index, user.Invent.BarcoSlot);
            }

            user.Invent.Object[slot].Equipped = 1;
            user.Invent.BarcoObjIndex = obj_index;
            user.Invent.BarcoSlot = slot;
            break;
        }

        case eOBJType::otMochilas: {
            if (user.flags.Muerto == 1) {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡¡Estas muerto!! Solo podes usar items cuando estas vivo. ", FontTypeNames::FONTTYPE_INFO);
                return;
            }

            if (user.Invent.Object[slot].Equipped == 1) {
                Desequipar(user_index, slot);
                return;
            }

            if (user.Invent.MochilaEqpObjIndex > 0 && user.Invent.MochilaEqpSlot > 0) {
                Desequipar(user_index, user.Invent.MochilaEqpSlot);
            }

            user.Invent.Object[slot].Equipped = 1;
            user.Invent.MochilaEqpObjIndex = obj_index;
            user.Invent.MochilaEqpSlot = slot;
            user.CurrentInventorySlots = static_cast<std::uint8_t>(MAX_NORMAL_INVENTORY_SLOTS + obj.MochilaType * 5);
            ao::net::protocol::WriteAddSlots(user_index, static_cast<eMochilas>(obj.MochilaType));
            break;
        }

        default:
            break;
    }

    UpdateUserInv(false, user_index, slot);
}

void Desequipar(std::int16_t user_index, std::uint8_t slot) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[static_cast<std::size_t>(user_index)];
    if (slot < 1 || slot > MAX_INVENTORY_SLOTS) {
        return;
    }

    const std::int16_t obj_index = user.Invent.Object[slot].ObjIndex;
    if (obj_index <= 0 || static_cast<std::size_t>(obj_index) >= ObjDataList.size()) {
        return;
    }

    const auto& obj = ObjDataList[static_cast<std::size_t>(obj_index)];

    switch (obj.OBJType) {
        case eOBJType::otWeapon: {
            user.Invent.Object[slot].Equipped = 0;
            user.Invent.WeaponEqpObjIndex = 0;
            user.Invent.WeaponEqpSlot = 0;

            if (user.flags.Mimetizado != 1) {
                user.char_appearance.WeaponAnim = NingunArma;
                if (s_change_user_char_hook) {
                    s_change_user_char_hook(user_index, user.char_appearance.body, user.char_appearance.Head,
                                            static_cast<std::int16_t>(user.char_appearance.heading),
                                            user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim,
                                            user.char_appearance.CascoAnim);
                }
            } else {
                user.CharMimetizado.WeaponAnim = NingunArma;
            }
            break;
        }

        case eOBJType::otFlechas: {
            user.Invent.Object[slot].Equipped = 0;
            user.Invent.MunicionEqpObjIndex = 0;
            user.Invent.MunicionEqpSlot = 0;
            break;
        }

        case eOBJType::otAnillo: {
            user.Invent.Object[slot].Equipped = 0;
            user.Invent.AnilloEqpObjIndex = 0;
            user.Invent.AnilloEqpSlot = 0;
            break;
        }

        case eOBJType::otArmadura: {
            user.Invent.Object[slot].Equipped = 0;
            user.Invent.ArmourEqpObjIndex = 0;
            user.Invent.ArmourEqpSlot = 0;

            if (s_dar_cuerpo_desnudo_hook) {
                user.char_appearance.body = s_dar_cuerpo_desnudo_hook(user_index, user.flags.Mimetizado == 1);
            }
            if (user.flags.Mimetizado != 1) {
                if (s_change_user_char_hook) {
                    s_change_user_char_hook(user_index, user.char_appearance.body, user.char_appearance.Head,
                                            static_cast<std::int16_t>(user.char_appearance.heading),
                                            user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim,
                                            user.char_appearance.CascoAnim);
                }
            }
            user.flags.Desnudo = 1;
            break;
        }

        case eOBJType::otCASCO: {
            user.Invent.Object[slot].Equipped = 0;
            user.Invent.CascoEqpObjIndex = 0;
            user.Invent.CascoEqpSlot = 0;

            if (user.flags.Mimetizado != 1) {
                user.char_appearance.CascoAnim = NingunCasco;
                if (s_change_user_char_hook) {
                    s_change_user_char_hook(user_index, user.char_appearance.body, user.char_appearance.Head,
                                            static_cast<std::int16_t>(user.char_appearance.heading),
                                            user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim,
                                            user.char_appearance.CascoAnim);
                }
            } else {
                user.CharMimetizado.CascoAnim = NingunCasco;
            }
            break;
        }

        case eOBJType::otESCUDO: {
            user.Invent.Object[slot].Equipped = 0;
            user.Invent.EscudoEqpObjIndex = 0;
            user.Invent.EscudoEqpSlot = 0;

            if (user.flags.Mimetizado != 1) {
                user.char_appearance.ShieldAnim = NingunEscudo;
                if (s_change_user_char_hook) {
                    s_change_user_char_hook(user_index, user.char_appearance.body, user.char_appearance.Head,
                                            static_cast<std::int16_t>(user.char_appearance.heading),
                                            user.char_appearance.WeaponAnim, user.char_appearance.ShieldAnim,
                                            user.char_appearance.CascoAnim);
                }
            } else {
                user.CharMimetizado.ShieldAnim = NingunEscudo;
            }
            break;
        }

        case eOBJType::otBarcos: {
            user.Invent.Object[slot].Equipped = 0;
            user.Invent.BarcoObjIndex = 0;
            user.Invent.BarcoSlot = 0;
            break;
        }

        case eOBJType::otMochilas: {
            user.Invent.Object[slot].Equipped = 0;
            user.Invent.MochilaEqpObjIndex = 0;
            user.Invent.MochilaEqpSlot = 0;

            TirarTodosLosItemsEnMochila(user_index);
            user.CurrentInventorySlots = MAX_NORMAL_INVENTORY_SLOTS;
            break;
        }

        default:
            user.Invent.Object[slot].Equipped = 0;
            break;
    }

    UpdateUserInv(false, user_index, slot);
}

// ====================================================
// Fase 4: G4 — Uso de Ítems, Interacción y Verificación
// ====================================================

void UseInvItem(std::int16_t user_index, std::uint8_t slot) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    User& user = UserList[user_index];

    if (slot < 1 || slot > user.CurrentInventorySlots) {
        return;
    }

    if (user.Invent.Object[slot].Amount <= 0) {
        return;
    }

    const std::int16_t obj_index = user.Invent.Object[slot].ObjIndex;
    if (obj_index <= 0 || static_cast<std::size_t>(obj_index) >= ObjDataList.size()) {
        return;
    }

    const ObjData& obj = ObjDataList[obj_index];

    if (obj.Newbie == 1 && !EsNewbie(user_index)) {
        ao::net::protocol::WriteConsoleMsg(user_index, "Sólo los newbies pueden usar estos objetos.", FontTypeNames::FONTTYPE_INFO);
        return;
    }

    user.flags.TargetObjInvIndex = obj_index;
    user.flags.TargetObjInvSlot = slot;

    // Ítems equipables de armaduras, accesorios, defensas y mochilas
    if (obj.OBJType == eOBJType::otArmadura ||
        obj.OBJType == eOBJType::otCASCO ||
        obj.OBJType == eOBJType::otESCUDO ||
        obj.OBJType == eOBJType::otAnillo ||
        obj.OBJType == eOBJType::otFlechas ||
        obj.OBJType == eOBJType::otMochilas) {
        EquiparInvItem(user_index, slot);
        return;
    }

    // Armas y herramientas de trabajo
    if (obj.OBJType == eOBJType::otWeapon) {
        const bool es_herramienta = (obj.proyectil == 1) ||
                                    (obj_index == CAÑA_PESCA || obj_index == RED_PESCA) ||
                                    (obj_index == HACHA_LEÑADOR || obj_index == HACHA_LEÑA_ELFICA) ||
                                    (obj_index == PIQUETE_MINERO) ||
                                    (obj_index == MARTILLO_HERRERO) ||
                                    (obj_index == SERRUCHO_CARPINTERO) ||
                                    ((user.flags.TargetObj == Leña || user.flags.TargetObj == LeñaElfica) && obj_index == DAGA) ||
                                    (obj.SkHerreria > 0);

        if (!es_herramienta) {
            EquiparInvItem(user_index, slot);
            return;
        }

        if (user.flags.Muerto == 1) {
            ao::net::protocol::WriteConsoleMsg(user_index, "¡¡Estás muerto!! Sólo puedes usar ítems cuando estás vivo.", FontTypeNames::FONTTYPE_INFO);
            return;
        }

        if (obj.proyectil == 1) {
            if (user.Invent.Object[slot].Equipped == 0) {
                ao::net::protocol::WriteConsoleMsg(user_index, "Antes de usar la herramienta deberías equipartela.", FontTypeNames::FONTTYPE_INFO);
                return;
            }
            if (s_work_request_target_hook) {
                s_work_request_target_hook(user_index, eSkill::Proyectiles);
            } else {
                ao::net::protocol::WriteWorkRequestTarget(user_index, eSkill::Proyectiles);
            }
            return;
        }

        if (obj_index == CAÑA_PESCA || obj_index == RED_PESCA) {
            if (user.Invent.WeaponEqpObjIndex == CAÑA_PESCA || user.Invent.WeaponEqpObjIndex == RED_PESCA) {
                if (s_work_request_target_hook) {
                    s_work_request_target_hook(user_index, eSkill::Pesca);
                } else {
                    ao::net::protocol::WriteWorkRequestTarget(user_index, eSkill::Pesca);
                }
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, "Debes tener equipada la herramienta para trabajar.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }

        if (obj_index == HACHA_LEÑADOR || obj_index == HACHA_LEÑA_ELFICA) {
            if (user.Invent.WeaponEqpObjIndex == HACHA_LEÑADOR || user.Invent.WeaponEqpObjIndex == HACHA_LEÑA_ELFICA) {
                if (s_work_request_target_hook) {
                    s_work_request_target_hook(user_index, eSkill::Talar);
                } else {
                    ao::net::protocol::WriteWorkRequestTarget(user_index, eSkill::Talar);
                }
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, "Debes tener equipada la herramienta para trabajar.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }

        if (obj_index == PIQUETE_MINERO) {
            if (user.Invent.WeaponEqpObjIndex == PIQUETE_MINERO) {
                if (s_work_request_target_hook) {
                    s_work_request_target_hook(user_index, eSkill::Mineria);
                } else {
                    ao::net::protocol::WriteWorkRequestTarget(user_index, eSkill::Mineria);
                }
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, "Debes tener equipada la herramienta para trabajar.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }

        if (obj_index == MARTILLO_HERRERO) {
            if (user.Invent.WeaponEqpObjIndex == MARTILLO_HERRERO) {
                if (s_work_request_target_hook) {
                    s_work_request_target_hook(user_index, eSkill::Herreria);
                } else {
                    ao::net::protocol::WriteWorkRequestTarget(user_index, eSkill::Herreria);
                }
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, "Debes tener equipada la herramienta para trabajar.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }

        if (obj_index == SERRUCHO_CARPINTERO) {
            if (user.Invent.WeaponEqpObjIndex == SERRUCHO_CARPINTERO) {
                EnivarObjConstruibles(user_index);
                ao::net::protocol::WriteShowCarpenterForm(user_index);
            } else {
                ao::net::protocol::WriteConsoleMsg(user_index, "Debes tener equipada la herramienta para trabajar.", FontTypeNames::FONTTYPE_INFO);
            }
            return;
        }

        if (obj.SkHerreria > 0) {
            if (s_work_request_target_hook) {
                s_work_request_target_hook(user_index, static_cast<eSkill>(FundirMetal));
            } else {
                ao::net::protocol::WriteWorkRequestTarget(user_index, static_cast<eSkill>(FundirMetal));
            }
            return;
        }

        return;
    }

    switch (obj.OBJType) {
        case eOBJType::otUseOnce: { // Comida
            if (user.flags.Muerto == 1) {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡¡Estás muerto!! Sólo puedes usar ítems cuando estás vivo.", FontTypeNames::FONTTYPE_INFO);
                return;
            }

            user.Stats.MinHam += obj.MinHam;
            if (user.Stats.MinHam > user.Stats.MaxHam) {
                user.Stats.MinHam = user.Stats.MaxHam;
            }
            user.flags.Hambre = 0;

            ao::net::protocol::WriteUpdateHungerAndThirst(user_index);
            ao::net::protocol::WriteConsoleMsg(user_index, "Has comido una ración.", FontTypeNames::FONTTYPE_INFO);

            QuitarUserInvItem(user_index, slot, 1);
            UpdateUserInv(false, user_index, slot);
            break;
        }

        case eOBJType::otBebidas: { // Bebida
            if (user.flags.Muerto == 1) {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡¡Estás muerto!! Sólo puedes usar ítems cuando estás vivo.", FontTypeNames::FONTTYPE_INFO);
                return;
            }

            user.Stats.MinAGU += obj.MinSed;
            if (user.Stats.MinAGU > user.Stats.MaxAGU) {
                user.Stats.MinAGU = user.Stats.MaxAGU;
            }
            user.flags.Sed = 0;

            ao::net::protocol::WriteUpdateHungerAndThirst(user_index);
            ao::net::protocol::WriteConsoleMsg(user_index, "Has bebido un trago.", FontTypeNames::FONTTYPE_INFO);

            QuitarUserInvItem(user_index, slot, 1);
            UpdateUserInv(false, user_index, slot);
            break;
        }

        case eOBJType::otPociones: {
            if (user.flags.Muerto == 1) {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡¡Estás muerto!! Sólo puedes usar ítems cuando estás vivo.", FontTypeNames::FONTTYPE_INFO);
                return;
            }

            user.flags.TomoPocion = 1;
            user.flags.TipoPocion = obj.TipoPocion;

            if (obj.TipoPocion == 5) {
                if (user.flags.Envenenado == 1) {
                    user.flags.Envenenado = 0;
                    ao::net::protocol::WriteConsoleMsg(user_index, "Te has curado del envenenamiento.", FontTypeNames::FONTTYPE_INFO);
                }
            } else if (obj.TipoPocion == 6) {
                user.flags.Muerto = 1;
                user.Stats.MinHp = 0;
                ao::net::protocol::WriteConsoleMsg(user_index, "Sientes un gran mareo y pierdes el conocimiento.", FontTypeNames::FONTTYPE_FIGHT);
            } else if (obj.DuracionEfecto > 0) {
                user.flags.DuracionEfecto = obj.DuracionEfecto;
                const std::int16_t mod = obj.MinModificador > 0 ? obj.MinModificador : 5;
                if (obj.TipoPocion == 1 || obj.TipoPocion == 4) {
                    auto& agi = user.Stats.UserAtributos[static_cast<std::size_t>(eAtributos::Agilidad)];
                    agi += mod;
                    if (agi > MAXATRIBUTOS) agi = MAXATRIBUTOS;
                    ao::net::protocol::WriteUpdateDexterity(user_index);
                } else {
                    auto& str = user.Stats.UserAtributos[static_cast<std::size_t>(eAtributos::Fuerza)];
                    str += mod;
                    if (str > MAXATRIBUTOS) str = MAXATRIBUTOS;
                    ao::net::protocol::WriteUpdateStrenght(user_index);
                }
                ao::net::protocol::WriteUpdateUserStats(user_index);
            } else {
                if (obj.TipoPocion == 1 || obj.TipoPocion == 3) {
                    std::int16_t mod = obj.MinModificador > 0 ? obj.MinModificador : 30;
                    user.Stats.MinHp += mod;
                    if (user.Stats.MinHp > user.Stats.MaxHp) {
                        user.Stats.MinHp = user.Stats.MaxHp;
                    }
                    ao::net::protocol::WriteUpdateHP(user_index);
                } else {
                    std::int16_t mod = obj.MinModificador;
                    if (mod <= 0) {
                        const std::int16_t elv = user.Stats.ELV > 0 ? user.Stats.ELV : 1;
                        mod = static_cast<std::int16_t>((user.Stats.MaxMAN * 4) / 100 + elv / 2 + 40 / elv);
                    }
                    user.Stats.MinMAN += mod;
                    if (user.Stats.MinMAN > user.Stats.MaxMAN) {
                        user.Stats.MinMAN = user.Stats.MaxMAN;
                    }
                    ao::net::protocol::WriteUpdateMana(user_index);
                }
                ao::net::protocol::WriteUpdateUserStats(user_index);
            }

            QuitarUserInvItem(user_index, slot, 1);
            UpdateUserInv(false, user_index, slot);
            break;
        }

        case eOBJType::otGuita: {
            if (user.flags.Muerto == 1) {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡¡Estás muerto!! Sólo puedes usar ítems cuando estás vivo.", FontTypeNames::FONTTYPE_INFO);
                return;
            }

            user.Stats.GLD += user.Invent.Object[slot].Amount;
            QuitarUserInvItem(user_index, slot, user.Invent.Object[slot].Amount);
            ao::net::protocol::WriteUpdateGold(user_index);
            UpdateUserInv(false, user_index, slot);
            break;
        }

        case eOBJType::otPergaminos: {
            if (user.flags.Muerto == 1) {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡¡Estás muerto!! Sólo puedes usar ítems cuando estás vivo.", FontTypeNames::FONTTYPE_INFO);
                return;
            }

            if (user.Stats.MaxMAN <= 0) {
                ao::net::protocol::WriteConsoleMsg(user_index, "No tienes conocimientos de las Artes Arcanas.", FontTypeNames::FONTTYPE_INFO);
                return;
            }

            if (user.flags.Hambre != 0 || user.flags.Sed != 0) {
                ao::net::protocol::WriteConsoleMsg(user_index, "Estás demasiado hambriento y sediento.", FontTypeNames::FONTTYPE_INFO);
                return;
            }

            if (s_learn_spell_hook) {
                if (s_learn_spell_hook(user_index, obj.HechizoIndex)) {
                    QuitarUserInvItem(user_index, slot, 1);
                    UpdateUserInv(false, user_index, slot);
                }
            }
            break;
        }

        case eOBJType::otBarcos: {
            if (s_navega_hook) {
                Obj barco_obj;
                barco_obj.ObjIndex = user.Invent.Object[slot].ObjIndex;
                barco_obj.Amount = user.Invent.Object[slot].Amount;
                s_navega_hook(user_index, barco_obj);
            }
            break;
        }

        case eOBJType::otMinerales: {
            if (user.flags.Muerto == 1) {
                ao::net::protocol::WriteConsoleMsg(user_index, "¡¡Estás muerto!! Sólo puedes usar ítems cuando estás vivo.", FontTypeNames::FONTTYPE_INFO);
                return;
            }
            if (s_work_request_target_hook) {
                s_work_request_target_hook(user_index, static_cast<eSkill>(FundirMetal));
            } else {
                ao::net::protocol::WriteWorkRequestTarget(user_index, static_cast<eSkill>(FundirMetal));
            }
            break;
        }

        default:
            break;
    }
}

void EnivarArmasConstruibles(std::int16_t user_index) {
    ao::net::protocol::WriteBlacksmithWeapons(user_index);
}

void EnivarObjConstruibles(std::int16_t user_index) {
    ao::net::protocol::WriteCarpenterObjects(user_index);
}

void EnivarArmadurasConstruibles(std::int16_t user_index) {
    ao::net::protocol::WriteBlacksmithArmors(user_index);
}

} // namespace InvUsuario

