/*
 * Módulo de Gestión y Ciclo de Vida de Criaturas (MODULO_NPCs)
 * 
 * Este módulo administra la totalidad del ciclo de vida de los NPCs y mascotas:
 * - Resets e infraestructura de slots (1..MAXNPCS) e inventario (1..30) base 1.
 * - Instanciación espacial (CrearNPC, SpawnNpc, ReSpawnNpc) y visual (MakeNPCChar, ChangeNPCChar, EraseNPCChar).
 * - Desplazamiento y desalojo de caspers en MoveNPCChar con barrera de agua (HayAgua).
 * - Muerte (MuereNpc), repartición de experiencia/reputación, barrido cuadrático pretoriano 8 To 90.
 * - Réplica estricta del Bug #27 (descarte de GiveGLD para NPCs estándar) y cota 32.000 para NPCsMuertos.
 * - Rutinas de mascotas y persecución (DoFollow, FollowAmo).
 */

#ifndef MODULO_NPCS_HPP
#define MODULO_NPCS_HPP

#include "Declares.hpp"
#include "modSendData.hpp"
#include <functional>
#include <cstdint>
#include <string>
#include <vector>

namespace MODULO_NPCs {

struct NpcCallbacks {
    std::function<void(std::int16_t user_index, std::int32_t exp, std::int16_t map, std::int16_t x, std::int16_t y)> PartyObtenerExito;
    std::function<void(std::int16_t user_index, const std::string& msg, std::int16_t font_type)> WriteConsoleMsg;
    std::function<void(std::int16_t user_index)> ExpulsarFaccionReal;
    std::function<void(std::int16_t user_index)> ExpulsarFaccionCaos;
    std::function<void(std::int16_t user_index)> RefreshCharStatus;
    std::function<void(std::int16_t user_index)> CheckUserLevel;
    std::function<void(std::int16_t user_index)> PerdioNpc;
    std::function<bool(std::int16_t owner)> IntervaloPerdioNpc;
    std::function<void(std::int16_t x)> CrearClanPretoriano;
    std::function<void(npc& npc_ref, bool is_pretoriano)> NPC_TIRAR_ITEMS;
    std::function<void(const WorldPos& pos, const Obj& item)> TirarItemAlPiso;
    std::function<void(std::int16_t npc_index, eHeading heading)> CheckUpdateNeededNpc;
    std::function<void(std::int16_t npc_index)> AgregarNpc;
    std::function<void(ao::net::send_data::SendTarget target, std::int16_t target_index, const std::vector<std::uint8_t>& data)> SendData;
    std::function<void(std::int16_t socket_or_user)> FlushBuffer;
    std::function<bool(const WorldPos& pos)> HayPCarea;
    std::function<bool(std::int16_t map, std::int16_t x, std::int16_t y, bool puede_agua, bool es_mascota)> LegalPosNPC;
    std::function<bool(std::int16_t map, std::int16_t x, std::int16_t y, bool puede_agua)> LegalPos;
    std::function<void(const WorldPos& pos, WorldPos& new_pos, bool puede_agua, bool puede_tierra)> ClosestLegalPos;
    std::function<void(std::int16_t snd_index, std::int16_t body, std::int16_t head, eHeading heading, std::int16_t char_index, std::int16_t x, std::int16_t y, std::int16_t weapon, std::int16_t shield, std::int16_t helmet, std::int16_t fx, std::int16_t loops, const std::string& name, std::int16_t clan, std::int16_t ciudad)> WriteCharacterCreate;
    std::function<bool(std::int16_t map, std::int16_t x, std::int16_t y)> HayAgua;
    std::function<void(std::int16_t user_index, eHeading heading)> WriteForceCharMove;
    std::function<void(std::int16_t pet_npc_index)> FollowAmo;
    std::function<std::int16_t(const std::string& name)> FindUser;
};

void SetCallbacks(const NpcCallbacks& cb) noexcept;
void ResetCallbacks() noexcept;

void ResetNpcFlags(std::int16_t npc_index) noexcept;
void ResetNpcCounters(std::int16_t npc_index) noexcept;
void ResetNpcCharInfo(std::int16_t npc_index) noexcept;
void ResetNpcCriatures(std::int16_t npc_index) noexcept;
void ResetExpresiones(std::int16_t npc_index) noexcept;
void ResetNpcMainInfo(std::int16_t npc_index) noexcept;

std::int16_t NextOpenNPC() noexcept;
std::int16_t OpenNPC(std::int16_t npc_number, bool respawn = true);
void QuitarNPC(std::int16_t npc_index);

bool TestSpawnTrigger(const WorldPos& pos, bool puede_agua = false) noexcept;
std::int16_t CrearNPC(std::int16_t nro_npc, std::int16_t mapa, const WorldPos& orig_pos, const WorldPos& alt_pos = {});
std::int16_t SpawnNpc(std::int16_t npc_index, const WorldPos& pos, bool fx, bool respawn);
void ReSpawnNpc(npc& mi_npc);
void MakeNPCChar(bool to_map, std::int16_t snd_index, std::int16_t npc_index, std::int16_t map, std::int16_t x, std::int16_t y);
void ChangeNPCChar(std::int16_t npc_index, std::int16_t body, std::int16_t head, eHeading heading);
void EraseNPCChar(std::int16_t npc_index);

void MoveNPCChar(std::int16_t npc_index, eHeading n_heading);

void MuereNpc(std::int16_t npc_index, std::int16_t user_index);
void NpcEnvenenarUser(std::int16_t user_index);
void QuitarMascota(std::int16_t user_index, std::int16_t npc_index);
void QuitarMascotaNpc(std::int16_t maestro_npc_index);
void QuitarPet(std::int16_t user_index, std::int16_t npc_index);
void ValidarPermanenciaNpc(std::int16_t npc_index);

void DoFollow(std::int16_t npc_index, const std::string& user_name);
void FollowAmo(std::int16_t npc_index);

} // namespace MODULO_NPCs

#endif // MODULO_NPCS_HPP
