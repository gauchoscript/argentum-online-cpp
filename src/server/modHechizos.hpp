#pragma once

#include "Declares.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <functional>

namespace ao {

// ============================================================================
// Constantes de Magia y Hechizos (modHechizos.bas y Declares.bas)
// ============================================================================

constexpr std::int16_t SUPERANILLO = 700;
constexpr std::int16_t APOCALIPSIS_SPELL_INDEX = 25;

constexpr std::int16_t HELEMENTAL_FUEGO = 26;
constexpr std::int16_t HELEMENTAL_TIERRA = 28;

constexpr std::int16_t LAUDMAGICO = 696;
constexpr std::int16_t FLAUTAMAGICA = 208;
constexpr std::int16_t LAUDELFICO = 1049;
constexpr std::int16_t FLAUTAELFICA = 1050;

constexpr std::int32_t COLOR_CYAN = 0x00FFFF00;

// ============================================================================
// Callbacks de Desacoplamiento (SpellsCallbacks)
// Catálogo de inyección de dependencias para mantener modHechizos aislado en Capa 7.
// ============================================================================

struct SpellsCallbacks {
    // --- Muerte, Resurrección y Ciclo Vital ---
    /// Dispara el proceso completo de muerte del usuario (drops, fantasmas, paquetes).
    std::function<void(std::int16_t user_index)> UserDie;
    /// Revive al usuario objetivo restaurándole el cuerpo original y HP básica.
    std::function<void(std::int16_t target_index)> RevivirUsuario;
    /// Procesa la muerte de una criatura asignando el crédito al asesino o su amo.
    std::function<void(std::int16_t npc_index, std::int16_t user_index)> MuereNpc;

    // --- Combate, Validación y Retaliación ---
    /// Valida si un usuario atacante puede agredir mágicamente a la víctima.
    std::function<bool(std::int16_t user_index, std::int16_t target_index)> PuedeAtacar;
    /// Valida si un usuario puede atacar a un NPC (considerando parálisis y alineación).
    std::function<bool(std::int16_t user_index, std::int16_t npc_index, bool paralisis)> PuedeAtacarNPC;
    /// Notifica y actualiza estados de legítima defensa e historial de agresión PvP.
    std::function<void(std::int16_t attacker_index, std::int16_t victim_index)> UsuarioAtacadoPorUsuario;
    /// Notifica la agresión a un NPC disparando inteligencia reactiva o defensa de amo.
    std::function<void(std::int16_t npc_index, std::int16_t user_index)> NpcAtacado;
    /// Evalúa condiciones de zona de combate/arena (Trigger 6 permite combate irrestricto).
    std::function<std::int16_t(std::int16_t user_index, std::int16_t target_index)> TriggerZonaPelea;
    /// Distribuye la experiencia obtenida por daño infligido a una criatura.
    std::function<void(std::int16_t user_index, std::int16_t npc_index, std::int32_t damage)> CalcularDarExp;

    // --- Facciones, Moralidad y Reputación ---
    /// Consulta si un usuario posee estatus criminal.
    std::function<bool(std::int16_t user_index)> Criminal;
    /// Consulta si un usuario pertenece a las fuerzas de la Armada Real.
    std::function<bool(std::int16_t user_index)> EsArmada;
    /// Consulta si un usuario pertenece a la Legión del Caos.
    std::function<bool(std::int16_t user_index)> EsCaos;
    /// Aplica penalización de criminalidad directa sobre el usuario.
    std::function<void(std::int16_t user_index)> VolverCriminal;
    /// Resta puntos de criminalidad (ej. abatido por un Guardia Real).
    std::function<void(std::int16_t user_index)> RestarCriminalidad;
    /// Expulsa a un miembro de la Armada Real por conductas impropias o criminalidad.
    std::function<void(std::int16_t user_index)> ExpulsarFaccionReal;
    /// Actualiza el estado visual del personaje (color de nick, jerarquía o facción).
    std::function<void(std::int16_t user_index)> RefreshCharStatus;

    // --- Criaturas y Mascotas ---
    /// Invoca una nueva criatura en coordenadas de mapa específicas.
    std::function<std::int16_t(std::int16_t npc_num, const WorldPos& pos, bool backup, bool respawn)> SpawnNpc;
    /// Ordena a la criatura invocada seguir y custodiar a su amo.
    std::function<void(std::int16_t npc_index)> FollowAmo;
    /// Teletransporta a la mascota hacia la posición de su amo (hechizo de Warp).
    std::function<void(std::int16_t user_index, std::int16_t pet_index)> WarpMascota;
    /// Localiza el índice de la mascota más alejada del usuario.
    std::function<std::int16_t(std::int16_t user_index)> FarthestPet;
    /// Obtiene el primer slot de mascota disponible en el perfil del usuario.
    std::function<std::int16_t(std::int16_t user_index)> FreeMascotaIndex;

    // --- Inventario y Progresión ---
    /// Remueve una cantidad determinada de un ítem en un slot de inventario (ej. runas).
    std::function<void(std::int16_t user_index, std::uint8_t slot, std::int16_t amount)> QuitarUserInvItem;
    /// Otorga probabilidad de avance en un skill del usuario tras un lanzamiento exitoso.
    std::function<void(std::int16_t user_index, eSkill skill, bool sube)> SubirSkill;

    // --- Notificaciones, Apariencia y Estadísticas ---
    /// Registra el fragmento/muerte en las estadísticas del usuario ganador.
    std::function<void(std::int16_t user_index, std::int16_t target_index)> StoreFrag;
    /// Contabiliza bajas y muertes en las estadísticas globales del personaje.
    std::function<void(std::int16_t victim_index, std::int16_t killer_index)> ContarMuerte;
    /// Sincroniza y recalcula estadísticas de combate entre víctima y atacante.
    std::function<void(std::int16_t victim_index, std::int16_t killer_index)> ActStats;
    /// Modifica y sincroniza la visibilidad (invisibilidad) del avatar hacia el área.
    std::function<void(std::int16_t user_index, std::int16_t char_index, bool invisible)> SetInvisible;
    /// Altera la apariencia física (cuerpo, cabeza, armas) para efectos de mimetismo.
    std::function<void(std::int16_t user_index, std::int16_t body, std::int16_t head, std::uint8_t heading, std::int16_t weapon, std::int16_t shield, std::int16_t helmet)> ChangeUserChar;
    /// Recupera la animación de ataque del arma equipada.
    std::function<std::int16_t(std::int16_t user_index, std::int16_t weapon_obj_index)> GetWeaponAnim;

    // --- Soporte Legal y Penalizaciones Morales ---
    /// Hook externo opcional para delegar la matriz de validación moral de soporte.
    std::function<bool(std::int16_t caster_index, std::int16_t target_index, bool do_criminal)> CanSupportUser;
    /// Hook externo opcional para delegar penalizaciones de reputación y nobleza.
    std::function<void(std::int16_t user_index, double exp_restada, std::int32_t puntos_bandido)> DisNobAuBan;
};

void SetSpellsCallbacks(const SpellsCallbacks& callbacks) noexcept;
void ResetSpellsCallbacks() noexcept;

// Constante de rango de visión vertical (AI_NPC.bas: Const RANGO_VISION_Y = 6)
inline constexpr std::int16_t RANGO_VISION_Y = 6;

// ============================================================================
// Fase 1: Libro de Hechizos, Utilidades y Validación Preliminar (G1)
// ============================================================================

bool TieneHechizo(std::int16_t i, std::int16_t UserIndex);
void AgregarHechizo(std::int16_t UserIndex, std::int16_t Slot);
void DecirPalabrasMagicas(std::string_view SpellWords, std::int16_t UserIndex);
bool PuedeLanzar(std::int16_t UserIndex, std::int16_t HechizoIndex);
void InfoHechizo(std::int16_t UserIndex);
void UpdateUserHechizos(bool UpdateAll, std::int16_t UserIndex, std::uint8_t Slot);
void ChangeUserHechizo(std::int16_t UserIndex, std::uint8_t Slot, std::int16_t Hechizo);
void DesplazarHechizo(std::int16_t UserIndex, std::int16_t Dire, std::int16_t HechizoDesplazado);

// ============================================================================
// Fase 2: Efectos Cuantitativos y Modificación de Atributos (G2)
// ============================================================================

bool HechizoPropUsuario(std::int16_t user_index);
void HechizoPropNPC(std::int16_t spell_index, std::int16_t npc_index, std::int16_t user_index, bool& hechizo_casteado);

// ============================================================================
// Fase 3: Estados Alterados, Metamorfosis e Invocaciones (G3)
// ============================================================================

void HechizoEstadoUsuario(std::int16_t user_index, bool& hechizo_casteado);
void HechizoEstadoNPC(std::int16_t npc_index, std::int16_t spell_index, bool& hechizo_casteado, std::int16_t user_index);
void HechizoTerrenoEstado(std::int16_t user_index, bool& b);
void HechizoInvocacion(std::int16_t user_index, bool& hechizo_casteado);

// ============================================================================
// Fase 4: Orquestación de Casteo, Soporte Legal, Moralidad y Magia de NPCs (G4)
// ============================================================================

void LanzarHechizo(std::int16_t SpellIndex, std::int16_t UserIndex);
void HandleHechizoUsuario(std::int16_t UserIndex, std::int16_t SpellIndex);
void HandleHechizoNPC(std::int16_t UserIndex, std::int16_t SpellIndex);
void HandleHechizoTerreno(std::int16_t UserIndex, std::int16_t SpellIndex);
bool CanSupportUser(std::int16_t CasterIndex, std::int16_t TargetIndex, bool DoCriminal = false);
void DisNobAuBan(std::int16_t UserIndex, double exp_restada, std::int32_t puntos_bandido = 10000);
void NpcLanzaSpellSobreUser(std::int16_t NpcIndex, std::int16_t UserIndex, std::int16_t Spell);
void NpcLanzaSpellSobreNpc(std::int16_t NpcIndex, std::int16_t TargetNPC, std::int16_t Spell);

} // namespace ao
