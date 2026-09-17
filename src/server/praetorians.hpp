#ifndef AO_PRAETORIANS_HPP
#define AO_PRAETORIANS_HPP

#include <cstdint>
#include <functional>
#include <string>
#include "server/Declares.hpp"

namespace praetorians {

constexpr std::int16_t PRCLER_NPC = 900;
constexpr std::int16_t PRGUER_NPC = 901;
constexpr std::int16_t PRMAGO_NPC = 902;
constexpr std::int16_t PRCAZA_NPC = 903;
constexpr std::int16_t PRKING_NPC = 904;

constexpr std::int16_t ALCOBA1_X = 35;
constexpr std::int16_t ALCOBA1_Y = 25;
constexpr std::int16_t ALCOBA2_X = 67;
constexpr std::int16_t ALCOBA2_Y = 25;

extern std::int16_t pretorianosVivos;

/**
 * @brief Estructura de callbacks plana para desacoplar el módulo praetorians
 * de los subsistemas externos del servidor (red, mapa, movimiento, etc.).
 * Todos los miembros utilizan la convención PascalCase estricta.
 */
struct PraetorianCallbacks {
    std::function<void(std::int16_t user_index, std::int16_t map, std::int16_t x, std::int16_t y)> WarpUserChar;
    std::function<void(std::int16_t npc_index, std::uint8_t heading)> MoveNPCChar;
    std::function<void(std::int16_t npc_index)> QuitarNPC;
    std::function<void(std::int16_t npc_number, std::int16_t map, const WorldPos& pos)> CrearNPC;
    std::function<void(const WorldPos& pos, WorldPos& npos, bool check_water, bool check_land)> ClosestLegalPos;
    std::function<bool(std::int16_t map, std::int16_t x, std::int16_t y)> LegalPos;
    std::function<void(std::int16_t npc_index, std::int16_t target_user)> NpcAtacaUser;
    std::function<void(std::int16_t npc_index, std::int16_t target_user, std::int16_t spell_index)> NpcLanzaSpellSobreUser;
    std::function<void(std::int16_t send_target, std::int16_t target_index, const std::string& msg)> SendData;
    std::function<std::string(std::int16_t char_index, std::int16_t x, std::int16_t y)> PrepareMessageCharacterMove;
    std::function<std::string(const std::string& text, std::int16_t char_index, std::int32_t color)> PrepareMessageChatOverHead;
    std::function<std::string(std::int16_t char_index, std::int16_t fx, std::int16_t loops)> PrepareMessageCreateFX;
    std::function<void(std::int16_t user_index, const std::string& msg, std::int16_t font_type)> WriteConsoleMsg;
    std::function<void(std::int16_t npc_index, std::int16_t attacker_user)> MuereNpc;
    std::function<void(const std::string& error_msg)> LogError;
    std::function<std::int32_t(std::int32_t min, std::int32_t max)> RandomNumber;
};

void SetCallbacks(const PraetorianCallbacks& cb);
void ResetCallbacks();

std::int16_t esPretoriano(std::int16_t npc_index);
bool EstoyMuyLejos(std::int16_t npc_index);
bool EstoyLejos(std::int16_t npc_index);
bool EsAlcanzable(std::int16_t npc_index, std::int16_t user_index);
bool CasperBlock(std::int16_t npc_index);
void LiberarCasperBlock(std::int16_t npc_index);

void MoverAba(std::int16_t npc_index);
void MoverArr(std::int16_t npc_index);
void MoverIzq(std::int16_t npc_index);
void MoverDer(std::int16_t npc_index);

/**
 * @brief Navegación voraz celda a celda (greedy movement).
 * NOTA: Preserva intencionalmente el comportamiento legacy VB6, incluyendo
 * oscilaciones y bloqueos ante esquinas en "L", sin reemplazar por BFS/A*.
 */
void GreedyWalkTo(std::int16_t npc_index, std::int16_t map, std::int16_t x, std::int16_t y);
void VolverAlCentro(std::int16_t npc_index);

/**
 * @brief Patrulla de waypoints 1 a 8.
 * NOTA: Reutiliza el campo Npclist[npc].Invent.ArmourEqpSlot para almacenar
 * el waypoint activo (1..8) según la paridad legacy de VB6.
 */
void CambiarAlcoba(std::int16_t npc_index);

bool EsMagoOClerigo(std::int16_t user_index);
void NPCRemueveVenenoNPC(std::int16_t npc_index, std::int16_t ally_npc_index, std::int16_t spell_slot);
void NPCCuraLevesNPC(std::int16_t npc_index, std::int16_t ally_npc_index, std::int16_t spell_slot);
void NPCRemueveParalisisNPC(std::int16_t npc_index, std::int16_t ally_npc_index, std::int16_t spell_slot);
void NPCcuraNPC(std::int16_t healer_npc, std::int16_t target_npc, std::int16_t spell_slot);
void NPCparalizaNPC(std::int16_t caster_npc, std::int16_t target_index, std::int16_t spell_slot);
void NPCLanzaCegueraPJ(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_slot);
void NPCLanzaEstupidezPJ(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_slot);
void NPCRemueveInvisibilidad(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_slot);
void NpcLanzaSpellSobreUser2(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_index);

/**
 * @brief Inmolación kamikaze del Mago Pretoriano.
 * NOTA: Reutiliza Npclist[npc].Invent.BarcoSlot como contador regresivo.
 * Al llegar a <= 1, inflige daño masivo en área y convoca MuereNpc(npc, 0).
 */
void MagoDestruyeWand(std::int16_t npc_index, std::uint8_t wand_counter, std::int16_t spell_slot);

void PRREY_AI(std::int16_t npc_index);
void PRGUER_AI(std::int16_t npc_index);
void PRCAZA_AI(std::int16_t npc_index);
void PRMAGO_AI(std::int16_t npc_index);

/**
 * @brief IA del Clérigo Pretoriano.
 * NOTA: Aplica la cascada de prioridad de soporte aliado:
 * 1. Remoción de Veneno -> 2. Remoción de Parálisis -> 3. Curación de Vida.
 */
void PRCLER_AI(std::int16_t npc_index);

void CrearClanPretoriano(std::int16_t previous_king_x);

} // namespace praetorians

#endif // AO_PRAETORIANS_HPP
