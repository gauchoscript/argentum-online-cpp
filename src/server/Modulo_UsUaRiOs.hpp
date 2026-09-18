#pragma once

#include <cstdint>
#include <stdexcept>
#include <string>
#include <functional>
#include "Declares.hpp"

namespace Modulo_UsUaRiOs {

/**
 * @brief Excepción tipada para desbordamiento aritmético en el cálculo de ELU (G3).
 */
class ELUOverflowException : public std::runtime_error {
public:
    explicit ELUOverflowException(const std::string& message)
        : std::runtime_error(message) {}
};

// ==========================================
// Fase G1: Ciclo de Vida y Gestión de Slots
// ==========================================

/**
 * @brief Busca el primer slot desocupado en UserList delegando en TCP::NextOpenUser().
 * @return Índice de usuario libre (1 a MaxUsers) o MaxUsers + 1 si está lleno.
 */
int NextOpenUser();

/**
 * @brief Busca la primera ranura libre en CharList (1 a MAXCHARS).
 * @return Índice de char disponible (1 a MAXCHARS) o 0 si está saturado.
 */
int NextOpenCharIndex();

/**
 * @brief Inicia la secuencia de desconexión diferida de un usuario (10s en zonas PK/combate).
 * @param user_index Índice de usuario en UserList.
 */
void Cerrar_Usuario(int user_index);

/**
 * @brief Cancela la desconexión diferida en progreso (/salir).
 * Replica el Bug #52 si ConnIDValida es false reasignando el temporizador.
 * @param user_index Índice de usuario en UserList.
 */
void CancelExit(int user_index);

/**
 * @brief Modifica el nombre (nick) de un personaje destino.
 * @param user_index Índice de usuario que solicita el cambio.
 * @param target_user_index Índice del personaje a renombrar.
 * @param new_nick Nuevo nombre para el personaje.
 */
void CambiarNick(int user_index, int target_user_index, const std::string& new_nick);


// ==========================================
// Fase G2: Cinemática, Transporte y Mascotas
// ==========================================

/**
 * @brief Retorna la dirección cardinal opuesta a la provista.
 */
eHeading InvertHeading(eHeading heading);

/**
 * @brief Evalúa si el personaje puede transitar sobre celdas de agua (navegando o muerto).
 */
bool PuedeAtravesarAgua(int user_index);

/**
 * @brief Verifica si un ID de cuerpo corresponde a una embarcación.
 */
bool BodyIsBoat(int body);

/**
 * @brief Alterna la apariencia del personaje entre su cuerpo terrestre y la embarcación.
 */
void ToogleBoatBody(int user_index);

/**
 * @brief Ejecuta el desplazamiento grilla a grilla, gestiona permuta con caspers (Bug #53) y actualiza áreas.
 */
void MoveUserChar(int user_index, eHeading heading);

/**
 * @brief Relocaliza al personaje a coordenadas arbitrarias (Bug #51 en NumUsers y actualización de áreas).
 */
void WarpUserChar(int user_index, int map, int x, int y, bool fx, bool teletransported = false);

/**
 * @brief Busca una celda libre adyacente para depositar objetos o desplazar criaturas.
 */
void Tilelibre(const WorldPos& pos, WorldPos& n_pos, Obj& obj, bool agua, bool tierra);

/**
 * @brief Crea la instancia visual del personaje en el mapa o envía el paquete de creación al cliente.
 */
void MakeUserChar(bool to_map, int snd_index, int user_index, int map, int x, int y, bool but_index = false);

/**
 * @brief Elimina la presencia visual del personaje del mapa.
 */
void EraseUserChar(int user_index, bool is_admin_invisible = false);

/**
 * @brief Cambia los atributos visuales (cuerpo, cabeza, armas, escudo, casco) y notifica al área.
 */
void ChangeUserChar(int user_index, int body, int head, uint8_t heading, int weapon, int shield, int helmet);

/**
 * @brief Reubica iterativamente todas las mascotas convocadas del usuario al cambiar de mapa.
 */
void WarpMascotas(int user_index);

/**
 * @brief Reubica a una mascota específica a la posición deseada.
 */
void WarpMascota(int user_index, int pet_index);

/**
 * @brief Calcula el punto cardinal relativo entre dos usuarios.
 */
std::string GetDireccion(int user_index, int other_user_index);

/**
 * @brief Retorna el índice de la mascota de un jugador ubicada a mayor distancia física.
 */
int FarthestPet(int user_index);


// ==========================================
// Fase G3: Progresión e Inspección
// ==========================================

using CharReaderHook = std::function<std::string(const std::string& char_name, const std::string& section, const std::string& key)>;

void SetCharReaderHook(CharReaderHook hook) noexcept;

void CheckUserLevel(int user_index);
void ActStats(int victim_index, int attacker_index);
void SubirSkill(int user_index, int skill, bool acerto);
void CheckEluSkill(int user_index, uint8_t skill, bool allocation);
void EnviarFama(int user_index);

void SendUserStatsTxt(int send_index, int user_index);
void SendUserMiniStatsTxt(int send_index, int user_index);
void SendUserSkillsTxt(int send_index, int user_index);
void SendUserInvTxt(int send_index, int user_index);

void SendUserMiniStatsTxtFromChar(int send_index, const std::string& char_name, CharReaderHook reader = nullptr);
void SendUserStatsTxtOFF(int send_index, const std::string& nombre, CharReaderHook reader = nullptr);
void SendUserOROTxtFromChar(int send_index, const std::string& char_name, CharReaderHook reader = nullptr);
void SendUserInvTxtFromChar(int send_index, const std::string& char_name, CharReaderHook reader = nullptr);

// ==========================================
// Fase G4: Muerte, Resurrección y Estados
// ==========================================

void UserDie(int user_index);
void RevivirUsuario(int user_index);
void ContarMuerte(int muerto_index, int atacante_index);
void SetInvisible(int user_index, int user_char_index, bool invisible);
void SetConsulatMode(int user_index);
bool IsArena(int user_index);
void VolverCriminal(int user_index);
void VolverCiudadano(int user_index);
void RefreshCharStatus(int user_index);
bool EsMascotaCiudadano(int npc_index, int user_index);

// ==========================================
// Fase G5: Combate e Inventario
// ==========================================

bool ToogleToAtackable(int user_index, int owner_index, bool stealing_npc = true);
void setHome(int user_index, eCiudad new_home, int npc_index);
void goHome(int user_index);
void PerdioNpc(int user_index);
void ApropioNpc(int user_index, int npc_index);
bool SameFaccion(int user_index, int other_user_index);
void NPCAtacado(int npc_index, int user_index);
bool PuedeApuñalar(int user_index);
bool PuedeAcuchillar(int user_index);
int GetWeaponAnim(int user_index, int obj_index);
uint8_t GetNickColor(int user_index);
void ChangeUserInv(int user_index, uint8_t slot, const UserOBJ& obj);
bool HasEnoughItems(int user_index, int obj_index, int32_t amount);
int32_t TotalOfferItems(int obj_index, int user_index);
uint8_t getMaxInventorySlots(int user_index);

} // namespace Modulo_UsUaRiOs
