#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include "Declares.hpp"

namespace modBanco {

// Hooks inyectables para pruebas unitarias y desacoplamiento transaccional y de E/S
using QuitarUserInvItemHook = std::function<void(std::int16_t user_index, std::uint8_t slot, std::int16_t cantidad)>;
using QuitarBancoInvItemHook = std::function<void(std::int16_t user_index, std::uint8_t slot, std::int16_t cantidad)>;
using CharReaderHook = std::function<std::string(const std::string& file_path, const std::string& section, const std::string& key)>;

void SetQuitarUserInvItemHook(QuitarUserInvItemHook hook) noexcept;
void SetQuitarBancoInvItemHook(QuitarBancoInvItemHook hook) noexcept;
void SetCharReaderHook(CharReaderHook hook) noexcept;

// ==========================================
// Fase 1: G1 — Apertura, GUI y Sincronización
// ==========================================

/**
 * @brief Inicia la interacción bancaria con el NPC banquero.
 * Sincroniza los 40 slots del banco, actualiza las estadísticas/oro en el cliente,
 * despacha el mensaje de apertura de GUI (BankInit) y activa el flag Comerciando.
 *
 * @param user_index Índice de usuario válido (1..MaxUsers).
 */
void IniciarDeposito(std::int16_t user_index);

/**
 * @brief Asigna un objeto en el slot de bóveda y despacha el paquete ChangeBankSlot al cliente.
 *
 * @param user_index Índice de usuario válido (1..MaxUsers).
 * @param slot Slot bancario base-1 (1..MAX_BANCOINVENTORY_SLOTS).
 * @param object Objeto a asignar en el slot del banco.
 */
void SendBanObj(std::int16_t user_index, std::uint8_t slot, const UserOBJ& object);

/**
 * @brief Actualiza uno o todos los slots de la bóveda bancaria del usuario hacia el cliente.
 *
 * @param update_all Si es true, recorre del slot 1 al 40 despachando SendBanObj.
 * @param user_index Índice de usuario válido (1..MaxUsers).
 * @param slot Slot específico a actualizar cuando update_all es false (1..MAX_BANCOINVENTORY_SLOTS).
 */
void UpdateBanUserInv(bool update_all, std::int16_t user_index, std::uint8_t slot);

/**
 * @brief Emite la confirmación de operación bancaria completada (BankOK) hacia el cliente.
 *
 * @param user_index Índice de usuario válido (1..MaxUsers).
 */
void UpdateVentanaBanco(std::int16_t user_index);

// ==========================================
// Fase 2: G2 — Retiro y Depósito de Objetos
// ==========================================

/**
 * @brief Punto de entrada para retirar un ítem de la bóveda bancaria.
 * Valida la cantidad, acota al saldo disponible en el slot de bóveda, convoca a UserReciveObj,
 * y sincroniza la mochila, la bóveda y la ventana bancaria.
 *
 * @param user_index Índice de usuario válido (1..MaxUsers).
 * @param i Slot de bóveda origen base-1 (1..MAX_BANCOINVENTORY_SLOTS).
 * @param cantidad Unidades a retirar.
 */
void UserRetiraItem(std::int16_t user_index, std::int16_t i, std::int16_t cantidad);

/**
 * @brief Transfiere un objeto desde la ranura de bóveda hacia la mochila del usuario.
 * Replicación de Bug #31: Acredita en destino antes de debitar en origen.
 * Nota: Se preserva la errata histórica de tipeo UserReciveObj de VB6.
 *
 * @param user_index Índice de usuario válido (1..MaxUsers).
 * @param obj_index Slot de bóveda origen base-1 (1..MAX_BANCOINVENTORY_SLOTS).
 * @param cantidad Unidades a transferir.
 */
void UserReciveObj(std::int16_t user_index, std::int16_t obj_index, std::int16_t cantidad);

/**
 * @brief Descuenta cantidad del slot de bóveda bancaria especificado.
 * Si la cantidad llega a <= 0, decrementa NroItems y blanquea el slot preservando
 * el modelo de grilla fija dispersa (sin compactar ranuras subsiguientes).
 *
 * @param user_index Índice de usuario válido (1..MaxUsers).
 * @param slot Slot de bóveda base-1 (1..MAX_BANCOINVENTORY_SLOTS).
 * @param cantidad Unidades a descontar.
 */
void QuitarBancoInvItem(std::int16_t user_index, std::uint8_t slot, std::int16_t cantidad);

/**
 * @brief Punto de entrada para depositar un ítem del inventario del usuario en la bóveda.
 * Valida disponibilidad en mochila, acota la cantidad, convoca a UserDejaObj,
 * y sincroniza la mochila, la bóveda y la ventana bancaria.
 *
 * @param user_index Índice de usuario válido (1..MaxUsers).
 * @param item Slot de mochila origen base-1 (1..CurrentInventorySlots).
 * @param cantidad Unidades a depositar.
 */
void UserDepositaItem(std::int16_t user_index, std::int16_t item, std::int16_t cantidad);

/**
 * @brief Transfiere un objeto desde la ranura de la mochila del usuario hacia la bóveda.
 * Replicación de Bug #31 (acreditación previa al débito) y Bug #32 (almacenamiento irrestricto).
 *
 * @param user_index Índice de usuario válido (1..MaxUsers).
 * @param obj_index Slot de mochila origen base-1 (1..CurrentInventorySlots).
 * @param cantidad Unidades a transferir.
 */
void UserDejaObj(std::int16_t user_index, std::int16_t obj_index, std::int16_t cantidad);

// ==========================================
// Fase 3: G3 — Inspección Administrativa GM
// ==========================================

/**
 * @brief Inspección administrativa GM: lista en consola el inventario bancario de un usuario conectado.
 *
 * @param send_index Índice del GM que recibe los mensajes de consola.
 * @param user_index Índice del usuario conectado a inspeccionar.
 */
void SendUserBovedaTxt(std::int16_t send_index, std::int16_t user_index);

/**
 * @brief Inspección administrativa GM: lista en consola el inventario bancario leyendo el archivo .chr.
 *
 * @param send_index Índice del GM que recibe los mensajes de consola.
 * @param char_name Nombre del personaje offline a consultar.
 */
void SendUserBovedaTxtFromChar(std::int16_t send_index, const std::string& char_name);

} // namespace modBanco
