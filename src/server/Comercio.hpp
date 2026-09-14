#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include "Declares.hpp"

namespace ao {

enum class eModoComercio : std::uint8_t {
    Compra = 1,
    Venta = 2
};
using TradeMode = eModoComercio;

constexpr std::uint8_t REDUCTOR_PRECIOVENTA = 3;

// Callbacks e inyecciones de dependencias
using KeyPurchaseHook = std::function<void(std::int16_t user_index, std::int16_t npc_index, std::uint8_t slot, std::int16_t obj_index)>;
using BanUserHook = std::function<void(std::int16_t user_index, std::string_view reason)>;
using SubirSkillHook = std::function<void(std::int16_t user_index, eSkill skill, bool sube)>;

void SetSubirSkillHook(SubirSkillHook hook) noexcept;
void SetKeyPurchaseHook(KeyPurchaseHook hook) noexcept;
void SetBanUserHook(BanUserHook hook) noexcept;

/**
 * @brief Computa el factor de descuento en compras según la habilidad de comerciar del usuario.
 * Fórmula: 1.0 + (UserSkills[Comerciar] / 100.0). No interviene el atributo Carisma.
 * Corresponde a Descuento en Comercio.bas:234-240.
 */
float Descuento(std::int16_t user_index);

/**
 * @brief Obtiene el precio base unitario de recompra por parte de mercaderes NPC.
 * Retorna 0 si el ítem es Newbie (Newbie = 1); de lo contrario, Valor / REDUCTOR_PRECIOVENTA.
 * Corresponde a SalePrice en Comercio.bas:279-288.
 */
float SalePrice(std::int16_t obj_index);

/**
 * @brief Inicia la sesión de comercio con el mercader NPC objetivo.
 * Sincroniza las ranuras del mercader, activa flags.Comerciando = true y emite WriteCommerceInit.
 * Corresponde a IniciarComercioNPC en Comercio.bas:193-201.
 */
void IniciarComercioNPC(std::int16_t user_index);

/**
 * @brief Busca una ranura apilable (Amount + Cantidad <= 10000) o la primera ranura libre en el NPC.
 * Recorre las 30 ranuras del NPC (MAX_INVENTORY_SLOTS) e incrementa Invent.NroItems si ocupa una nueva.
 * Corresponde a SlotEnNPCInv en Comercio.bas:203-232.
 */
std::int16_t SlotEnNPCInv(std::int16_t npc_index, std::int16_t obj_index, std::int16_t cantidad);

/**
 * @brief Despacha al cliente la actualización de inventario del mercader NPC.
 * Replicación obligatoria del Bug #33: itera estrictamente los primeros 20 slots
 * (MAX_NORMAL_INVENTORY_SLOTS), dejando invisibles las ranuras 21 a 30 al cliente.
 * Corresponde a EnviarNpcInv en Comercio.bas:248-272.
 */
void EnviarNpcInv(std::int16_t user_index, std::int16_t npc_index);

/**
 * @brief Ejecuta la transacción unitaria de compra o venta con el mercader NPC.
 * Aplica guardianes de cantidad, autoban ante pedidos anómalos (> 10000), verificación
 * de fondos y espacio, restricciones faccionarias ("SR" / "SC") y clamping de oro a MAXORO.
 * Corresponde a Comercio en Comercio.bas:41-191.
 */
void Comercio(eModoComercio modo, std::int16_t user_index, std::int16_t npc_index, std::int16_t slot, std::int16_t cantidad,
              KeyPurchaseHook key_hook = nullptr, BanUserHook ban_hook = nullptr);

} // namespace ao
