#include "modBanco.hpp"
#include "Protocol.hpp"
#include "InvUsuario.hpp"
#include "FileIO.hpp"
#include <filesystem>
#include <string>

namespace modBanco {

namespace Protocol = ao::net::protocol;

static QuitarUserInvItemHook s_quitar_inv_hook{nullptr};
static QuitarBancoInvItemHook s_quitar_banco_hook{nullptr};
static CharReaderHook s_char_reader_hook{nullptr};

void SetQuitarUserInvItemHook(QuitarUserInvItemHook hook) noexcept {
    s_quitar_inv_hook = hook;
}

void SetQuitarBancoInvItemHook(QuitarBancoInvItemHook hook) noexcept {
    s_quitar_banco_hook = hook;
}

void SetCharReaderHook(CharReaderHook hook) noexcept {
    s_char_reader_hook = hook;
}

// ==========================================
// Fase 1: G1 — Apertura, GUI y Sincronización
// ==========================================

void IniciarDeposito(std::int16_t user_index) {
    if (user_index < 1 || user_index > MaxUsers) {
        return;
    }

    // Hacemos un Update del inventario del banco del usuario
    UpdateBanUserInv(true, user_index, 0);

    // Actualizamos el dinero y estadísticas
    Protocol::WriteUpdateUserStats(user_index);

    // Mostramos la ventana pa' comerciar
    Protocol::WriteBankInit(user_index);
    UserList[user_index].flags.Comerciando = true;
}

void SendBanObj(std::int16_t user_index, std::uint8_t slot, const UserOBJ& object) {
    if (user_index < 1 || user_index > MaxUsers || slot < 1 || slot > MAX_BANCOINVENTORY_SLOTS) {
        return;
    }

    UserList[user_index].BancoInvent.Object[slot] = object;
    Protocol::WriteChangeBankSlot(user_index, slot);
}

void UpdateBanUserInv(bool update_all, std::int16_t user_index, std::uint8_t slot) {
    if (user_index < 1 || user_index > MaxUsers) {
        return;
    }

    if (!update_all) {
        // Actualiza un solo slot
        if (slot >= 1 && slot <= MAX_BANCOINVENTORY_SLOTS) {
            if (UserList[user_index].BancoInvent.Object[slot].ObjIndex > 0) {
                SendBanObj(user_index, slot, UserList[user_index].BancoInvent.Object[slot]);
            } else {
                SendBanObj(user_index, slot, UserOBJ{});
            }
        }
    } else {
        // Actualiza todos los slots (1 a 40)
        for (std::uint8_t loop_c = 1; loop_c <= MAX_BANCOINVENTORY_SLOTS; ++loop_c) {
            if (UserList[user_index].BancoInvent.Object[loop_c].ObjIndex > 0) {
                SendBanObj(user_index, loop_c, UserList[user_index].BancoInvent.Object[loop_c]);
            } else {
                SendBanObj(user_index, loop_c, UserOBJ{});
            }
        }
    }
}

void UpdateVentanaBanco(std::int16_t user_index) {
    if (user_index < 1 || user_index > MaxUsers) {
        return;
    }

    Protocol::WriteBankOK(user_index);
}

// ==========================================
// Fase 2: G2 — Retiro y Depósito de Objetos
// ==========================================

void UserRetiraItem(std::int16_t user_index, std::int16_t i, std::int16_t cantidad) {
    if (user_index < 1 || user_index > MaxUsers) {
        return;
    }

    if (cantidad < 1) {
        return;
    }

    if (i < 1 || i > MAX_BANCOINVENTORY_SLOTS) {
        return;
    }

    Protocol::WriteUpdateUserStats(user_index);

    if (UserList[user_index].BancoInvent.Object[i].Amount > 0) {
        if (cantidad > UserList[user_index].BancoInvent.Object[i].Amount) {
            cantidad = UserList[user_index].BancoInvent.Object[i].Amount;
        }

        // Agregamos el obj que compra al inventario
        UserReciveObj(user_index, i, cantidad);

        // Actualizamos el inventario del usuario
        InvUsuario::UpdateUserInv(true, user_index, 0);

        // Actualizamos el banco
        UpdateBanUserInv(true, user_index, 0);
    }

    // Actualizamos la ventana de comercio
    UpdateVentanaBanco(user_index);
}

void UserReciveObj(std::int16_t user_index, std::int16_t obj_index, std::int16_t cantidad) {
    if (user_index < 1 || user_index > MaxUsers) {
        return;
    }

    if (obj_index < 1 || obj_index > MAX_BANCOINVENTORY_SLOTS) {
        return;
    }

    if (UserList[user_index].BancoInvent.Object[obj_index].Amount <= 0) {
        return;
    }

    if (cantidad < 1) {
        return;
    }

    const std::int16_t obji = UserList[user_index].BancoInvent.Object[obj_index].ObjIndex;
    if (obji <= 0) {
        return;
    }

    const std::uint8_t current_slots = UserList[user_index].CurrentInventorySlots;

    // ¿Ya tiene un objeto de este tipo?
    std::int16_t slot = 1;
    while (slot <= current_slots) {
        if (UserList[user_index].Invent.Object[slot].ObjIndex == obji &&
            UserList[user_index].Invent.Object[slot].Amount + cantidad <= MAX_INVENTORY_OBJS) {
            break;
        }
        ++slot;
    }

    // Sino se fija por un slot vacío
    if (slot > current_slots) {
        slot = 1;
        while (slot <= current_slots) {
            if (UserList[user_index].Invent.Object[slot].ObjIndex == 0) {
                break;
            }
            ++slot;
        }

        if (slot > current_slots) {
            Protocol::WriteConsoleMsg(user_index, "No podés tener mas objetos.", Protocol::FontTypeNames::FONTTYPE_INFO);
            return;
        }

        UserList[user_index].Invent.NroItems++;
    }

    // Mete el obj en el slot (Bug #31: Acredita en destino antes de debitar en origen)
    if (UserList[user_index].Invent.Object[slot].Amount + cantidad <= MAX_INVENTORY_OBJS) {
        UserList[user_index].Invent.Object[slot].ObjIndex = obji;
        UserList[user_index].Invent.Object[slot].Amount += cantidad;

        if (s_quitar_banco_hook) {
            s_quitar_banco_hook(user_index, static_cast<std::uint8_t>(obj_index), cantidad);
        } else {
            QuitarBancoInvItem(user_index, static_cast<std::uint8_t>(obj_index), cantidad);
        }
    } else {
        Protocol::WriteConsoleMsg(user_index, "No podés tener mas objetos.", Protocol::FontTypeNames::FONTTYPE_INFO);
    }
}

void QuitarBancoInvItem(std::int16_t user_index, std::uint8_t slot, std::int16_t cantidad) {
    if (user_index < 1 || user_index > MaxUsers) {
        return;
    }

    if (slot < 1 || slot > MAX_BANCOINVENTORY_SLOTS) {
        return;
    }

    auto& banco_slot = UserList[user_index].BancoInvent.Object[slot];

    // Quita un Obj
    banco_slot.Amount -= cantidad;

    // Modelo de Grilla Fija Dispersa (Sparse Grid de 40 ranuras fijas):
    // Cuando una ranura llega a Amount <= 0, se decrementa NroItems y se blanquea en su
    // posición exacta (ObjIndex = 0, Amount = 0). Está terminantemente prohibido compactar
    // o correr los slots hacia la izquierda, preservando los índices fijos (1..40).
    if (banco_slot.Amount <= 0) {
        UserList[user_index].BancoInvent.NroItems--;
        banco_slot.ObjIndex = 0;
        banco_slot.Amount = 0;
    }
}

void UserDepositaItem(std::int16_t user_index, std::int16_t item, std::int16_t cantidad) {
    if (user_index < 1 || user_index > MaxUsers) {
        return;
    }

    if (item < 1 || item > UserList[user_index].CurrentInventorySlots) {
        return;
    }

    if (UserList[user_index].Invent.Object[item].Amount > 0 && cantidad > 0) {
        if (cantidad > UserList[user_index].Invent.Object[item].Amount) {
            cantidad = UserList[user_index].Invent.Object[item].Amount;
        }

        // Agregamos el obj que deposita al banco
        UserDejaObj(user_index, item, cantidad);

        // Actualizamos el inventario del usuario
        InvUsuario::UpdateUserInv(true, user_index, 0);

        // Actualizamos el banco
        UpdateBanUserInv(true, user_index, 0);
    }

    // Actualizamos la ventana del banco
    UpdateVentanaBanco(user_index);
}

void UserDejaObj(std::int16_t user_index, std::int16_t obj_index, std::int16_t cantidad) {
    if (user_index < 1 || user_index > MaxUsers) {
        return;
    }

    if (cantidad < 1) {
        return;
    }

    if (obj_index < 1 || obj_index > UserList[user_index].CurrentInventorySlots) {
        return;
    }

    // Bug #32: Se toma el objeto sin validar si es barco, faccionario o newbie
    const std::int16_t obji = UserList[user_index].Invent.Object[obj_index].ObjIndex;
    if (obji <= 0) {
        return;
    }

    // ¿Ya tiene un objeto de este tipo?
    std::int16_t slot = 1;
    while (slot <= MAX_BANCOINVENTORY_SLOTS) {
        if (UserList[user_index].BancoInvent.Object[slot].ObjIndex == obji &&
            UserList[user_index].BancoInvent.Object[slot].Amount + cantidad <= MAX_INVENTORY_OBJS) {
            break;
        }
        ++slot;
    }

    // Sino se fija por un slot vacío antes del slot devuelto
    if (slot > MAX_BANCOINVENTORY_SLOTS) {
        slot = 1;
        while (slot <= MAX_BANCOINVENTORY_SLOTS) {
            if (UserList[user_index].BancoInvent.Object[slot].ObjIndex == 0) {
                break;
            }
            ++slot;
        }

        if (slot > MAX_BANCOINVENTORY_SLOTS) {
            Protocol::WriteConsoleMsg(user_index, "No tienes mas espacio en el banco!!", Protocol::FontTypeNames::FONTTYPE_INFO);
            return;
        }

        UserList[user_index].BancoInvent.NroItems++;
    }

    if (slot <= MAX_BANCOINVENTORY_SLOTS) { // Slot válido
        // Mete el obj en el slot (Bug #31: Acredita en destino antes de debitar en origen)
        if (UserList[user_index].BancoInvent.Object[slot].Amount + cantidad <= MAX_INVENTORY_OBJS) {
            UserList[user_index].BancoInvent.Object[slot].ObjIndex = obji;
            UserList[user_index].BancoInvent.Object[slot].Amount += cantidad;

            if (s_quitar_inv_hook) {
                s_quitar_inv_hook(user_index, static_cast<std::uint8_t>(obj_index), cantidad);
            } else {
                InvUsuario::QuitarUserInvItem(user_index, static_cast<std::uint8_t>(obj_index), cantidad);
            }
        } else {
            Protocol::WriteConsoleMsg(user_index, "El banco no puede cargar tantos objetos.", Protocol::FontTypeNames::FONTTYPE_INFO);
        }
    }
}

// ==========================================
// Fase 3: G3 — Inspección Administrativa GM
// ==========================================

void SendUserBovedaTxt(std::int16_t send_index, std::int16_t user_index) {
    if (send_index < 1 || send_index > MaxUsers || user_index < 1 || user_index > MaxUsers) {
        return;
    }

    Protocol::WriteConsoleMsg(send_index, UserList[user_index].name, Protocol::FontTypeNames::FONTTYPE_INFO);
    Protocol::WriteConsoleMsg(send_index, "Tiene " + std::to_string(UserList[user_index].BancoInvent.NroItems) + " objetos.", Protocol::FontTypeNames::FONTTYPE_INFO);

    for (std::uint8_t j = 1; j <= MAX_BANCOINVENTORY_SLOTS; ++j) {
        const auto obj_index = UserList[user_index].BancoInvent.Object[j].ObjIndex;
        if (obj_index > 0) {
            std::string item_name;
            if (static_cast<std::size_t>(obj_index) < ObjDataList.size()) {
                item_name = ObjDataList[obj_index].name;
            }
            const auto msg = "Objeto " + std::to_string(j) + " " + item_name + " Cantidad:" + std::to_string(UserList[user_index].BancoInvent.Object[j].Amount);
            Protocol::WriteConsoleMsg(send_index, msg, Protocol::FontTypeNames::FONTTYPE_INFO);
        }
    }
}

void SendUserBovedaTxtFromChar(std::int16_t send_index, const std::string& char_name) {
    if (send_index < 1 || send_index > MaxUsers) {
        return;
    }

    const std::string char_file = CharPath.empty() ? ("charfile/" + char_name + ".chr") : (CharPath + char_name + ".chr");

    auto get_ini_val = [&](const std::string& section, const std::string& key) -> std::string {
        if (s_char_reader_hook) {
            return s_char_reader_hook(char_file, section, key);
        }
        return FileIO::GetVar(char_file, section, key);
    };

    bool file_exists = false;
    if (s_char_reader_hook) {
        const std::string cant_test = s_char_reader_hook(char_file, "BancoInventory", "CantidadItems");
        file_exists = !cant_test.empty();
    } else {
        std::error_code ec;
        file_exists = std::filesystem::exists(char_file, ec);
    }

    if (file_exists) {
        Protocol::WriteConsoleMsg(send_index, char_name, Protocol::FontTypeNames::FONTTYPE_INFO);
        const std::string cant_items = get_ini_val("BancoInventory", "CantidadItems");
        Protocol::WriteConsoleMsg(send_index, "Tiene " + cant_items + " objetos.", Protocol::FontTypeNames::FONTTYPE_INFO);

        for (std::uint8_t j = 1; j <= MAX_BANCOINVENTORY_SLOTS; ++j) {
            const std::string tmp = get_ini_val("BancoInventory", "Obj" + std::to_string(j));
            if (tmp.empty()) {
                continue;
            }

            const auto dash_pos = tmp.find('-');
            if (dash_pos != std::string::npos) {
                const std::string idx_str = tmp.substr(0, dash_pos);
                const std::string amt_str = tmp.substr(dash_pos + 1);

                std::int32_t obj_ind = 0;
                std::int32_t obj_cant = 0;
                try {
                    obj_ind = std::stol(idx_str);
                    obj_cant = std::stol(amt_str);
                } catch (...) {
                    continue;
                }

                if (obj_ind > 0) {
                    std::string item_name;
                    if (static_cast<std::size_t>(obj_ind) < ObjDataList.size()) {
                        item_name = ObjDataList[obj_ind].name;
                    }
                    const auto msg = "Objeto " + std::to_string(j) + " " + item_name + " Cantidad:" + std::to_string(obj_cant);
                    Protocol::WriteConsoleMsg(send_index, msg, Protocol::FontTypeNames::FONTTYPE_INFO);
                }
            }
        }
    } else {
        Protocol::WriteConsoleMsg(send_index, "Usuario inexistente: " + char_name, Protocol::FontTypeNames::FONTTYPE_INFO);
    }
}

} // namespace modBanco
