#pragma once

#include "Declares.hpp"
#include "Matematicas.hpp"

#include <cstdint>
#include <functional>

namespace ao {

// Constantes de alcance de combate (SistemaCombate.bas:38-39)
constexpr std::uint8_t MAXDISTANCIAARCO = 18;
constexpr std::uint8_t MAXDISTANCIAMAGIA = 18;

// ============================================================================
// Funciones Matemáticas y Utilidades Básicas (Fase 1)
// ============================================================================

std::int16_t MinimoInt(std::int16_t a, std::int16_t b) noexcept;
std::int16_t MaximoInt(std::int16_t a, std::int16_t b) noexcept;

// ============================================================================
// Evasión y Poder de Ataque (Fase 1)
// ============================================================================

std::int32_t PoderEvasionEscudo(std::int16_t UserIndex);
std::int32_t PoderEvasion(std::int16_t UserIndex);

std::int32_t PoderAtaqueArma(std::int16_t UserIndex);
std::int32_t PoderAtaqueProyectil(std::int16_t UserIndex);
std::int32_t PoderAtaqueWrestling(std::int16_t UserIndex);

std::int32_t PoderAtaqueModificado(std::int16_t UserIndex);

// ============================================================================
// Validaciones y Consultas Auxiliares (Fase 1)
// ============================================================================

bool CheckResistencia(std::int16_t UserIndex);
bool ArcoYFlecha(std::int16_t UserIndex);
std::int32_t AlcanzaEspacio(std::int16_t UserIndex, std::int16_t Arma);
bool CheckArmasMuniciones(std::int16_t UserIndex, std::int16_t Arma, std::int16_t Municion);

bool ArmaParaApuñalar(std::int16_t ArmaIndex);
bool SameClan(std::int16_t UserIndex, std::int16_t OtherUserIndex);
bool SameParty(std::int16_t UserIndex, std::int16_t OtherUserIndex);

eTrigger6 TriggerZonaPelea(std::int16_t Origen, std::int16_t Destino);

// Control determinista y verificación de intervalo de ataque
using CombatTimeProvider = std::function<std::int32_t()>;
void SetCombatTimeProvider(CombatTimeProvider provider) noexcept;

bool IntervaloPermiteAtacar(std::int16_t UserIndex, bool Avisar = true);

// ============================================================================
// Daño Bruto y Acierto RNG (Fase 2: G2)
// ============================================================================

// Inyección de proveedor de números aleatorios para determinismo de tests
using CombatRandomProvider = std::function<std::int32_t(std::int32_t lower, std::int32_t upper)>;
void SetCombatRandomProvider(CombatRandomProvider provider) noexcept;

std::int32_t CalcularDaño(std::int16_t UserIndex, std::int16_t NpcIndex = 0);
std::int32_t ProbExito(std::int32_t PoderAtacante, std::int32_t PoderDefensor);

bool UserImpactoUser(std::int16_t AtacanteIndex, std::int16_t VictimaIndex);
bool UserImpactoNpc(std::int16_t UserIndex, std::int16_t NpcIndex);

bool NpcImpactoUser(std::int16_t NpcIndex, std::int16_t UserIndex);
bool NpcImpactoNpc(std::int16_t Atacante, std::int16_t Victima);

std::int32_t NpcDaño(std::int16_t NpcIndex);

// Métodos complementarios de consulta de impacto y evasión
std::int32_t UserImpacto(std::int16_t UserIndex);
std::int32_t NpcImpacto(std::int16_t NpcIndex);
std::int32_t NpcEvasion(std::int16_t NpcIndex);

// ============================================================================
// Deducción de Daño, Absorciones y Peculiaridades (Fase 3: G3)
// ============================================================================

using QuitarObjetosHook = std::function<void(std::int16_t obj_index, std::int32_t amount, std::int16_t user_index)>;
using DoApuñalarHook = std::function<void(std::int16_t user_index, std::int16_t victim_user, std::int16_t victim_npc, std::int32_t daño)>;
using DoGolpeCriticoHook = std::function<void(std::int16_t user_index, std::int16_t victim_user, std::int16_t victim_npc, std::int32_t daño)>;
using DoAcuchillarHook = std::function<void(std::int16_t user_index, std::int16_t victim_user, std::int16_t victim_npc, std::int32_t daño)>;
using UserDieHook = std::function<void(std::int16_t user_index)>;
using MuereNpcHook = std::function<void(std::int16_t npc_index, std::int16_t user_index)>;
using SubirSkillHook = std::function<void(std::int16_t user_index, eSkill skill, bool exito)>;
using CheckUserLevelHook = std::function<void(std::int16_t user_index)>;
using PartyExpHook = std::function<void(std::int16_t user_index, std::int32_t exp, std::int16_t map, std::int16_t x, std::int16_t y)>;
using RefreshCharStatusHook = std::function<void(std::int16_t user_index)>;

void SetCombatQuitarObjetosHook(QuitarObjetosHook hook) noexcept;
void SetDoApuñalarHook(DoApuñalarHook hook) noexcept;
void SetDoGolpeCriticoHook(DoGolpeCriticoHook hook) noexcept;
void SetDoAcuchillarHook(DoAcuchillarHook hook) noexcept;
void SetUserDieHook(UserDieHook hook) noexcept;
void SetMuereNpcHook(MuereNpcHook hook) noexcept;
void SetCombatSubirSkillHook(SubirSkillHook hook) noexcept;
void SetCheckUserLevelHook(CheckUserLevelHook hook) noexcept;
void SetPartyExpHook(PartyExpHook hook) noexcept;
void SetRefreshCharStatusHook(RefreshCharStatusHook hook) noexcept;
void ResetCombatHooks() noexcept;

bool PuedeApuñalar(std::int16_t user_index);
bool PuedeAcuchillar(std::int16_t user_index);

std::int32_t UserDañoNpc(std::int16_t user_index, std::int16_t npc_index);
std::int32_t UserDañoUser(std::int16_t atacante_index, std::int16_t victima_index);
std::int32_t NpcDañoUser(std::int16_t npc_index, std::int16_t user_index);
std::int32_t NpcDaño(std::int16_t npc_index, std::int16_t user_index);
std::int32_t NpcDañoNpc(std::int16_t atacante_index, std::int16_t victima_index);

void CalcularDarExp(std::int16_t user_index, std::int16_t npc_index, std::int32_t daño);
void RestarCriminalidad(std::int16_t user_index);
void UserEnvenena(std::int16_t user_index, std::int16_t victima_index);

// ============================================================================
// Orquestación de Flujo de Ataque, Protocolo y Hooks (Fase 4: G4)
// ============================================================================

using VolverCriminalHook = std::function<void(std::int16_t user_index)>;
using CancelExitHook = std::function<void(std::int16_t user_index)>;
using StoreFragHook = std::function<void(std::int16_t killer, std::int16_t victim)>;
using ContarMuerteHook = std::function<void(std::int16_t victim, std::int16_t killer)>;

void SetVolverCriminalHook(VolverCriminalHook hook) noexcept;
void SetCancelExitHook(CancelExitHook hook) noexcept;
void SetStoreFragHook(StoreFragHook hook) noexcept;
void SetContarMuerteHook(ContarMuerteHook hook) noexcept;

void UsuarioAtaca(std::int16_t user_index);
bool UsuarioAtacaUsuario(std::int16_t atacante_index, std::int16_t victima_index);
void UsuarioAtacadoPorUsuario(std::int16_t atacante_index, std::int16_t victima_index);
bool UsuarioAtacaNpc(std::int16_t user_index, std::int16_t npc_index);
bool NpcAtacaUser(std::int16_t npc_index, std::int16_t user_index);
void NpcAtacaNpc(std::int16_t atacante_index, std::int16_t victima_index, bool cambiarMovimiento = true);

bool PuedeAtacar(std::int16_t atacante_index, std::int16_t victima_index);
bool PuedeAtacarNPC(std::int16_t user_index, std::int16_t npc_index, bool paraliza = false, bool is_pet = false);

void MuereNpc(std::int16_t npc_index, std::int16_t user_index);
void RestarCriaturasEntrenador(std::int16_t user_index, std::int16_t npc_index);
void CheckPets(std::int16_t npc_index, std::int16_t user_index, bool check_elementales = true);
void AllFollowAmo(std::int16_t user_index);
void AllMascotasAtacanUser(std::int16_t victim, std::int16_t maestro);

} // namespace ao
