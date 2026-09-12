#ifndef MOD_SEND_DATA_HPP
#define MOD_SEND_DATA_HPP

#include <cstdint>
#include <cstddef>
#include <span>
#include <string_view>

namespace ao::net::send_data {

/**
 * @brief Rutas y estrategias de difusión de paquetes salientes.
 *
 * Mapeo strongly-typed del Enum SendTarget de VB6 (modSendData.bas:34-64).
 * Se excluye la constante huérfana ToGM (Bug #21) para garantizar que
 * el compilador fuerce la exhaustividad en el switch de despacho.
 */
enum class SendTarget : std::uint8_t {
    ToAll = 1,
    ToMap = 2,
    ToPCArea = 3,
    ToAllButIndex = 4,
    ToMapButIndex = 5,
    // ToGM = 6 excluido por código muerto (Bug #21)
    ToNPCArea = 7,
    ToGuildMembers = 8,
    ToAdmins = 9,
    ToPCAreaButIndex = 10,
    ToAdminsAreaButConsejeros = 11,
    ToDiosesYclan = 12,
    ToConsejo = 13,
    ToClanArea = 14,
    ToConsejoCaos = 15,
    ToRolesMasters = 16,
    ToDeadArea = 17,
    ToCiudadanos = 18,
    ToCriminales = 19,
    ToPartyArea = 20,
    ToReal = 21,
    ToCaos = 22,
    ToCiudadanosYRMs = 23,
    ToCriminalesYRMs = 24,
    ToRealYRMs = 25,
    ToCaosYRMs = 26,
    ToHigherAdmins = 27,
    ToGMsAreaButRmsOrCounselors = 28,
    ToUsersAreaButGMs = 29,
    ToUsersAndRmsAndCounselorsAreaButGMs = 30
};

// ============================================================================
// Paso 1: Helpers inline constexpr de Áreas y Validación
// ============================================================================

/**
 * @brief Calcula la máscara de bit de pertenencia a un área (1 << (pos / 9)).
 *
 * En VB6 se usaba 2 ^ (pos \ 9).
 *
 * @param pos Coordenada de tile X o Y (1..100).
 * @return Máscara entera con un único bit encendido.
 */
[[nodiscard]] constexpr int area_pertenece_mask(int pos) noexcept {
    return 1 << (pos / 9);
}

/**
 * @brief Calcula la máscara de recepción de área que cubre la propia y sus dos adyacentes.
 *
 * @param area_index Índice de área en la grilla (0..11).
 * @return Máscara entera con hasta 3 bits contiguos encendidos.
 */
[[nodiscard]] constexpr int area_recive_mask(int area_index) noexcept {
    int mask = (1 << area_index);
    if (area_index > 0) {
        mask |= (1 << (area_index - 1));
    }
    if (area_index < 11) {
        mask |= (1 << (area_index + 1));
    }
    return mask;
}

/**
 * @brief Valida si un índice de mapa es estrictamente válido dentro de los límites del servidor.
 *
 * @param map Índice del mapa a evaluar.
 * @return true si 1 <= map <= NumMaps, false en caso contrario.
 */
[[nodiscard]] bool is_valid_map_index(int map) noexcept;

// ============================================================================
// Paso 2: Ruteo Geográfico (Áreas y Mapas)
// ============================================================================

void send_to_user_area(int user_index, std::span<const std::uint8_t> data);
void send_to_user_area_but_index(int user_index, std::span<const std::uint8_t> data);
void send_to_dead_user_area(int user_index, std::span<const std::uint8_t> data);
void send_to_area_by_pos(int map, int area_x, int area_y, std::span<const std::uint8_t> data);
void send_to_map(int map, std::span<const std::uint8_t> data);
void send_to_map_but_index(int user_index, std::span<const std::uint8_t> data);
void send_to_npc_area(int npc_index, std::span<const std::uint8_t> data);

// ============================================================================
// Paso 3: Ruteo Social e Iteradores de Clanes y Parties
// ============================================================================

void send_to_guild_members(int guild_index, std::span<const std::uint8_t> data);
void send_to_dioses_y_clan(int guild_index, std::span<const std::uint8_t> data);
void send_to_user_guild_area(int user_index, std::span<const std::uint8_t> data);
void send_to_user_party_area(int user_index, std::span<const std::uint8_t> data);

// ============================================================================
// Paso 4: Ruteo de Privilegios y Facciones (con Parche Pre-Login Bug #22)
// ============================================================================

void send_to_all(std::span<const std::uint8_t> data);
void send_to_all_but_index(int user_index, std::span<const std::uint8_t> data);
void send_to_admins(std::span<const std::uint8_t> data);
void send_to_higher_admins(std::span<const std::uint8_t> data);
void send_to_consejo(std::span<const std::uint8_t> data);
void send_to_consejo_caos(std::span<const std::uint8_t> data);
void send_to_roles_masters(std::span<const std::uint8_t> data);
void send_to_ciudadanos(std::span<const std::uint8_t> data);
void send_to_criminales(std::span<const std::uint8_t> data);
void send_to_real(std::span<const std::uint8_t> data);
void send_to_caos(std::span<const std::uint8_t> data);
void send_to_ciudadanos_y_rms(std::span<const std::uint8_t> data);
void send_to_criminales_y_rms(std::span<const std::uint8_t> data);
void send_to_real_y_rms(std::span<const std::uint8_t> data);
void send_to_caos_y_rms(std::span<const std::uint8_t> data);

void send_to_admins_but_consejeros_area(int user_index, std::span<const std::uint8_t> data);
void send_to_gms_area_but_rms_or_counselors(int user_index, std::span<const std::uint8_t> data);
void send_to_users_area_but_gms(int user_index, std::span<const std::uint8_t> data);
void send_to_users_and_rms_and_counselors_area_but_gms(int user_index, std::span<const std::uint8_t> data);

// ============================================================================
// Paso 5: Dispatcher Maestro SendData y Anomalía AlertarFaccionarios
// ============================================================================

/**
 * @brief Despacha un mensaje a los destinatarios especificados por la ruta.
 *
 * @param route Estrategia de destino de SendTarget.
 * @param index Identificador contextual (UserIndex, Map, GuildIndex, etc.).
 * @param data Carga útil a transmitir (vista inmutable zero-copy).
 */
void send_data(SendTarget route, int index, std::span<const std::uint8_t> data);

/**
 * @brief Sobrecarga de conveniencia que acepta std::string_view sin realizar copias.
 */
inline void send_data(SendTarget route, int index, std::string_view data) {
    send_data(route, index, std::span<const std::uint8_t>(reinterpret_cast<const std::uint8_t*>(data.data()), data.size()));
}

/**
 * @brief Alerta a los compañeros de facción en el mapa la dirección desde donde proviene el auxilio.
 *
 * @param user_index Usuario que emite la alerta faccionaria.
 */
void alertar_faccionarios(int user_index);

} // namespace ao::net::send_data

#endif // MOD_SEND_DATA_HPP
