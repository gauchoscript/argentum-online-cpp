/*
 * Módulo de Inteligencia Artificial de Criaturas (AI_NPC)
 *
 * Este módulo contiene la lógica de toma de decisiones, escaneo espacial,
 * persecución, magia y búsqueda de caminos (BFS / Pathfinding) para NPCs.
 *
 * Desacoplamiento plano: Toda interacción externa con UserList, SistemaCombate,
 * modHechizos, ModAreas y subsistemas de red/log se efectúa mediante la
 * estructura AiNpcCallbacks con miembrosstd::function en PascalCase estricto.
 */

#ifndef AI_NPC_HPP
#define AI_NPC_HPP

#include "Declares.hpp"
#include <functional>
#include <cstdint>
#include <string>
#include <vector>

namespace AI_NPC {

constexpr std::uint8_t RANGO_VISION_X = 8;
constexpr std::uint8_t RANGO_VISION_Y = 6;

constexpr std::int16_t ELEMENTALFUEGO = 93;
constexpr std::int16_t ELEMENTALTIERRA = 94;
constexpr std::int16_t ELEMENTALAGUA = 92;

/**
 * @brief Estructura de callbacks desacoplados para la IA de NPCs.
 * Nombres de miembros estrictamente en PascalCase.
 */
struct AiNpcCallbacks {
    std::function<bool(std::int16_t user_index)> IntervaloPermiteSerAtacado;
    std::function<bool(std::int16_t user_index)> EsCriminal;
    std::function<bool(std::int16_t npc_index, std::int16_t user_index)> NpcAtacaUser;
    std::function<void(std::int16_t npc_index, std::int16_t target_npc, bool is_pet)> NpcAtacaNpc;
    std::function<void(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_id)> NpcLanzaSpellSobreUser;
    std::function<void(std::int16_t npc_index, std::int16_t target_npc, std::int16_t spell_id)> NpcLanzaSpellSobreNpc;
    std::function<void(std::int16_t user_index, const std::string& msg, std::int16_t font_type)> WriteConsoleMsg;
    std::function<void(std::int16_t user_index)> FlushBuffer;
    std::function<void(const std::string& error_msg)> LogError;
    std::function<std::int32_t(std::int32_t min, std::int32_t max)> RandomNumber;
    std::function<std::uint8_t(const WorldPos& from, const WorldPos& to)> FindDirection;
    std::function<void(std::uint8_t heading, WorldPos& pos)> HeadtoPos;
    std::function<bool(std::int16_t map, std::int16_t x, std::int16_t y)> InMapBounds;
    std::function<double(const WorldPos& pos1, const WorldPos& pos2)> Distancia;
    std::function<const std::vector<std::int16_t>&(std::int16_t map)> GetConnGroupUserEntries;
    std::function<std::int16_t(std::int16_t map, std::int16_t x, std::int16_t y)> GetMapUserIndex;
    std::function<std::int16_t(std::int16_t map, std::int16_t x, std::int16_t y)> GetMapNpcIndex;
    std::function<void(std::int16_t npc_index, std::int16_t body, std::int16_t head, std::uint8_t heading)> ChangeNPCChar;
    std::function<void(std::int16_t npc_index, std::uint8_t heading)> MoveNPCChar;
    std::function<void(std::int16_t npc_index)> QuitarNPC;
    std::function<void(const npc& npc_data)> ReSpawnNpc;
    std::function<void(std::int16_t npc_index)> FollowAmo;
    std::function<void(std::int16_t npc_index)> SeekPath;
    std::function<const User&(std::int16_t user_index)> GetUser;
};

void SetCallbacks(const AiNpcCallbacks& cb) noexcept;
void ResetCallbacks() noexcept;

// Fase 1 (G1)
void NpcLanzaUnSpell(std::int16_t npc_index, std::int16_t user_index);
void NpcLanzaUnSpellSobreNpc(std::int16_t npc_index, std::int16_t target_npc);

/**
 * @brief Evalúa si el usuario objetivo está adyacente (distancia <= 1).
 * Nota de paridad: Preserva la semántica de VB6 mediante `!(static_cast<int>(dist) > 1)`
 * para evitar inversiones de precedencia con el operador NOT.
 */
bool UserNear(std::int16_t npc_index);
bool ReCalculatePath(std::int16_t npc_index);
bool PathEnd(std::int16_t npc_index);

/**
 * @brief Avanza un paso en la ruta BFS.
 * Nota de quirk (Bug #45): Lee desinvirtiendo las coordenadas X/Y desde PFINFO.Path.
 */
bool FollowPath(std::int16_t npc_index);

/**
 * @brief Invoca a SeekPath para calcular la ruta BFS.
 * Nota de quirk (Bug #45): Escribe invirtiendo deliberadamente Target.X = Pos.Y y Target.Y = Pos.X.
 */
bool PathFindingAI(std::int16_t npc_index);

// Fase 2 (G2)
void GuardiasAI(std::int16_t npc_index, bool del_caos);
void HostilMalvadoAI(std::int16_t npc_index);
void HostilBuenoAI(std::int16_t npc_index);
void RestoreOldMovement(std::int16_t npc_index);
void AiNpcObjeto(std::int16_t npc_index);

// Fase 3 (G3)
void IrUsuarioCercano(std::int16_t npc_index);

/**
 * @brief Lógica de agresión a perseguidores.
 * Nota de quirk: El Elemental de Agua (Numero 92) queda excluido del ataque melee directo.
 */
void SeguirAgresor(std::int16_t npc_index);
void PersigueCiudadano(std::int16_t npc_index);
void PersigueCriminal(std::int16_t npc_index);
void SeguirAmo(std::int16_t npc_index);

/**
 * @brief Lógica de ataque inter-NPC.
 * Nota de quirk: El Elemental de Fuego (Numero 93) spamea magia contra el Dragón (13),
 * forzando CanAttack = 1 en el Dragón para contraataque.
 */
void AiNpcAtacaNpc(std::int16_t npc_index);

// Fase 4 (G4)
/**
 * @brief Orquestador central del tick de IA de un NPC.
 * Nota de error handler: Encapsulado en try / catch recuperativo que invoca
 * LogError, QuitarNPC y ReSpawnNpc en caso de excepción no capturada.
 */
void NPCAI(std::int16_t npc_index);

} // namespace AI_NPC

#endif // AI_NPC_HPP
