#pragma once

#include "Declares.hpp"
#include <cstdint>
#include <functional>
#include <vector>

namespace Modulo_InventANDobj {

/**
 * @brief Entrada de slot para inicialización y reposición de inventario de NPCs.
 * Representa la plantilla en memoria de un objeto del archivo NPCs.dat (ObjX / CantX).
 */
struct NPCInventorySlot {
    std::int16_t obj_index{0};
    std::int16_t amount{0};
};

/**
 * @brief Hook para consultar la plantilla base de inventario de un NPC en memoria.
 * Erradica las lecturas bloqueantes y síncronas a disco ("NPCs.dat") que realizaba VB6
 * mediante GetVar en CargarInvent y EncontrarCant.
 */
using NPCTemplateLookupHook = std::function<std::vector<NPCInventorySlot>(std::int16_t npc_numero)>;
void SetNPCTemplateLookupHook(NPCTemplateLookupHook hook) noexcept;

/**
 * @brief Hook para generador de números aleatorios (permite pruebas deterministas de drops).
 */
using RandomGeneratorHook = std::function<std::int32_t(std::int32_t min, std::int32_t max)>;
void SetRandomGeneratorHook(RandomGeneratorHook hook) noexcept;

/**
 * @brief Hook para interceptar la llamada completa a TirarItemAlPiso en pruebas unitarias y desacoplamiento de alto nivel.
 */
using TirarItemAlPisoHook = std::function<WorldPos(const WorldPos& pos, const Obj& obj, bool not_pirata)>;
void SetTirarItemAlPisoHook(TirarItemAlPisoHook hook) noexcept;

/**
 * @brief Hook para la búsqueda espacial de celdas libres en el mapa (desacopla de Modulo_UsUaRiOs.bas:Tilelibre).
 */
using TilelibreHook = std::function<void(const WorldPos& pos, WorldPos& n_pos, const Obj& obj, bool agua, bool tierra)>;
void SetTilelibreHook(TilelibreHook hook) noexcept;

/**
 * @brief Hook para la instanciación física de objetos en celdas de mapa (desacopla de InvUsuario.bas:MakeObj).
 */
using MakeObjHook = std::function<void(const Obj& obj, std::int16_t map, std::int16_t x, std::int16_t y)>;
void SetMakeObjHook(MakeObjHook hook) noexcept;

// ============================================================================
// Fase 1 - G1: Núcleo de Inventario de NPCs
// ============================================================================

/**
 * @brief Blanquea por completo el inventario de un NPC y resetea contadores.
 * Preserva la indexación 1-based original de VB6 en Invent.Object (slots 1..MAX_INVENTORY_SLOTS).
 * Corresponde a ResetNpcInv en Modulo_InventANDobj.bas:197-211.
 */
void ResetNpcInv(std::int16_t npc_index) noexcept;

/**
 * @brief Determina si un NPC todavía posee al menos una unidad de un objeto dado.
 * Corresponde a QuedanItems en Modulo_InventANDobj.bas:143-157.
 */
[[nodiscard]] bool QuedanItems(std::int16_t npc_index, std::int16_t obj_index) noexcept;

/**
 * @brief Obtiene la cantidad original configurada de un objeto para un NPC comerciante.
 * Corresponde a EncontrarCant en Modulo_InventANDobj.bas:171-189.
 */
[[nodiscard]] std::int16_t EncontrarCant(std::int16_t npc_index, std::int16_t obj_index) noexcept;

/**
 * @brief Descuenta una cantidad de ítems de un slot del inventario del NPC.
 * Si el ítem es crucial (Crucial = 1), repone automáticamente el stock consultando la plantilla base.
 * Si el inventario se vacía por completo (NroItems == 0) y no tiene InvReSpawn = 1, recarga todo.
 * Corresponde a QuitarNpcInvItem en Modulo_InventANDobj.bas:228-270.
 */
void QuitarNpcInvItem(std::int16_t npc_index, std::uint8_t slot, std::int16_t cantidad);

/**
 * @brief Carga el inventario inicial del NPC desde la plantilla configurada en memoria.
 * Corresponde a CargarInvent en Modulo_InventANDobj.bas:280-297.
 */
void CargarInvent(std::int16_t npc_index);

// ============================================================================
// Fase 2 - G2: Drops Probabilísticos y Fraccionamiento de Oro
// ============================================================================

/**
 * @brief Divide una cantidad arbitraria de oro en fragmentos de hasta MAX_INVENTORY_OBJS (10.000)
 * y los arroja al suelo invocando TirarItemAlPiso.
 * Corresponde a TirarOroNpc en Modulo_InventANDobj.bas:307-345.
 */
void TirarOroNpc(std::int32_t cantidad, const WorldPos& pos);

/**
 * @brief Ejecuta el descarte de botín de una criatura al morir.
 * Si es pretoriano, arroja todo el inventario y evalúa GiveGLD.
 * Si es regular, evalúa la cascada geométrica de drops y descarta GiveGLD (Bug #27).
 * Corresponde a NPC_TIRAR_ITEMS en Modulo_InventANDobj.bas:67-136.
 */
void NPC_TIRAR_ITEMS(npc& current_npc, bool is_pretoriano);

// ============================================================================
// Fase 3 - G3: Despacho Espacial y Puntos de Extensión
// ============================================================================

/**
 * @brief Despacha un ítem hacia una celda del suelo buscando un tile legal adyacente.
 * Invoca TilelibreHook; si la celda devuelta es válida (X != 0 && Y != 0), materializa el objeto vía MakeObjHook.
 * Preserva el Bug #28: si no hay celdas libres disponibles (devuelve 0, 0), omite MakeObjHook y retorna (0, 0) silenciosamente.
 * Corresponde a TirarItemAlPiso en Modulo_InventANDobj.bas:43-63.
 */
WorldPos TirarItemAlPiso(const WorldPos& pos, const Obj& obj, bool not_pirata = true);

} // namespace Modulo_InventANDobj
