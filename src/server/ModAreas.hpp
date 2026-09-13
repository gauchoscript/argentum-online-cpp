#pragma once

#include <cstdint>
#include <vector>
#include "Declares.hpp"

namespace ModAreas {

// Constante centinela de inicialización de área / login (ModAreas.bas:49)
inline constexpr std::uint8_t USER_NUEVO = 255;

// Aliases a las estructuras canónicas definidas en Declares.hpp
using AreaInfo = ::AreaInfo;
using ConnGroup = ::ConnGroup;

/**
 * @brief Inicializa las tablas precalculadas de áreas y dimensiona ConnGroups.
 *
 * Precalcula:
 * - AreasRecive[12] mediante máscaras de bits con desplazamiento entero (1 << k).
 * - AreasInfo[101][101] con la fórmula literal de VB6 (LoopC \ 9 + 1) * (loopX \ 9 + 1),
 *   preservando las colisiones históricas (Bug #24).
 * - Dimensiona ConnGroups para 1..NumMaps.
 *
 * Excluye explícitamente PosToArea y AreasStats.dat / AreasOptimizacion (Bug #26).
 */
void InitAreas();

/**
 * @brief Obtiene el AreaID precalculado para una coordenada (X, Y) del mapa.
 *
 * @param x Coordenada X (1..100).
 * @param y Coordenada Y (1..100).
 * @return Identificador de área en base a la matriz precalculada (o 0 si está fuera de rango).
 */
[[nodiscard]] std::uint8_t GetAreaID(std::int16_t x, std::int16_t y) noexcept;

/**
 * @brief Obtiene la máscara de recepción de 3 franjas adyacentes para una franja dada.
 *
 * @param franja Índice de franja (0..11).
 * @return Bitmask de 16 bits con los bits (franja-1), (franja) y (franja+1) encendidos.
 */
[[nodiscard]] std::int16_t GetAreaRecive(std::int16_t franja) noexcept;

/**
 * @brief Notifica y actualiza la visibilidad de un usuario tras ingresar a un mapa o cambiar de área.
 *
 * @param user_index Índice del usuario en UserList.
 * @param head Dirección de movimiento o USER_NUEVO si ingresa al mapa.
 * @param but_index Si es true, no envía el paquete MakeUserChar del propio usuario a sí mismo.
 */
void CheckUpdateNeededUser(std::int16_t user_index, std::uint8_t head, bool but_index = false);

/**
 * @brief Enrola a un usuario en el grupo de conexiones de un mapa específico.
 *
 * Previene duplicados con búsqueda lineal, reinicia las variables espaciales de AreasInfo
 * y dispara CheckUpdateNeededUser con USER_NUEVO.
 *
 * @param user_index Índice del usuario en UserList (1-based).
 * @param map Índice del mapa (1..NumMaps).
 * @param but_index Si es true, no envía MakeUserChar del usuario a sí mismo.
 */
void AgregarUser(std::int16_t user_index, std::int16_t map, bool but_index = false);

/**
 * @brief Da de baja a un usuario del grupo de conexiones de su mapa actual.
 *
 * Realiza una búsqueda secuencial y un desplazamiento lineal hacia la izquierda
 * para preservar contigüidad y orden. Si el usuario no está o el mapa es inválido, sale de forma segura.
 *
 * @param user_index Índice del usuario en UserList (1-based).
 * @param map Índice del mapa (1..NumMaps).
 */
void QuitarUser(std::int16_t user_index, std::int16_t map);

/**
 * @brief Notifica y actualiza la visibilidad de una criatura tras moverse o aparecer en el mapa.
 *
 * @param npc_index Índice de la criatura en Npclist.
 * @param head Dirección de movimiento o USER_NUEVO si ingresa al mapa.
 */
void CheckUpdateNeededNpc(std::int16_t npc_index, std::uint8_t head);

/**
 * @brief Inicializa los campos de visibilidad de una criatura y la posiciona en el sistema de áreas.
 *
 * Blanquea los campos espaciales e invoca CheckUpdateNeededNpc con USER_NUEVO.
 *
 * @param npc_index Índice de la criatura en Npclist.
 */
void AgregarNpc(std::int16_t npc_index);

// ============================================================================
// Hooks y Callbacks de Desacoplamiento para Capas Posteriores y Testing
// ============================================================================

using MakeUserCharHook = void (*)(bool to_map, std::int16_t snd_index, std::int16_t user_index,
                                  std::int16_t map, std::int16_t x, std::int16_t y);

using MakeNPCCharHook = void (*)(bool to_map, std::int16_t snd_index, std::int16_t npc_index,
                                 std::int16_t map, std::int16_t x, std::int16_t y);

using BloquearHook = void (*)(bool to_map, std::int16_t snd_index, std::int16_t x, std::int16_t y, bool b);

/**
 * @brief Configura el hook invocado para MakeUserChar (por defecto nullptr).
 */
void SetMakeUserCharHook(MakeUserCharHook hook) noexcept;

/**
 * @brief Configura el hook invocado para MakeNPCChar (por defecto nullptr).
 */
void SetMakeNPCCharHook(MakeNPCCharHook hook) noexcept;

/**
 * @brief Configura el hook invocado para Bloquear (por defecto nullptr).
 */
void SetBloquearHook(BloquearHook hook) noexcept;

} // namespace ModAreas
