#ifndef MDPARTY_HPP
#define MDPARTY_HPP

#include "clsParty.hpp"
#include <array>
#include <memory>
#include <string>
#include <string_view>
#include <functional>

namespace ModParty {

struct PartyCallbacks {
    std::function<void(std::int16_t user_index, std::string_view msg, std::uint8_t font_type)> WriteConsoleMsg;
    std::function<void(std::int16_t user_index)> WriteUpdateUserStats;
    std::function<void(std::int16_t user_index)> CheckUserLevel;
};

void SetPartyCallbacks(const PartyCallbacks& callbacks);
const PartyCallbacks& GetPartyCallbacks();

std::int16_t NextParty();
bool PuedeCrearParty(std::int16_t user_index);
void CrearParty(std::int16_t user_index);
void SolicitarIngresoAParty(std::int16_t user_index);
void SalirDeParty(std::int16_t user_index);
void ExpulsarDeParty(std::int16_t leader, std::int16_t old_member);
bool UserPuedeEjecutarComandos(std::int16_t user_index);
void AprobarIngresoAParty(std::int16_t leader, std::int16_t new_member);
void BroadCastParty(std::int16_t user_index, std::string_view texto);
void OnlineParty(std::int16_t user_index);
void TransformarEnLider(std::int16_t old_leader, std::int16_t new_leader);
void ActualizaExperiencias();
void ObtenerExito(std::int16_t user_index, std::int32_t exp_ganada, std::int16_t mapa, std::int16_t x, std::int16_t y);
std::int16_t CantMiembros(std::int16_t user_index);
void ActualizarSumaNivelesElevados(std::int16_t user_index);

} // namespace ModParty

// Contenedor Global de Parties (1..300)
extern std::array<std::shared_ptr<clsParty>, ModParty::MAX_PARTIES + 1> Parties;

#endif // MDPARTY_HPP
