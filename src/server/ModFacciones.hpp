#ifndef MODFACCIONES_HPP
#define MODFACCIONES_HPP

#include "Declares.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace ModFacciones {

struct FactionCallbacks {
    std::function<bool(std::int16_t user_index)> IsCriminal;
    std::function<void(std::int16_t user_index, std::string_view msg, std::int16_t target_npc_char_index, std::uint32_t color)> WriteChatOverHead;
    std::function<void(std::int16_t user_index, std::string_view msg, std::uint8_t font_type)> WriteConsoleMsg;
    std::function<void(std::int16_t user_index)> RefreshCharStatus;
    std::function<void(std::int16_t user_index)> CheckUserLevel;
    std::function<bool(std::int16_t user_index, const Obj& item)> MeterItemEnInventario;
    std::function<void(const WorldPos& pos, const Obj& item)> TirarItemAlPiso;
    std::function<void(std::int16_t user_index, std::uint8_t slot)> Desequipar;
    std::function<std::string(std::int16_t guild_index)> GetGuildAlignment;
    std::function<void(std::string_view text)> LogEjercitoReal;
    std::function<void(std::string_view text)> LogEjercitoCaos;
};

void SetFactionCallbacks(const FactionCallbacks& callbacks);
void ResetFactionCallbacks();
void InitDefaultFactionCallbacks();

std::int16_t GetArmourAmount(std::int16_t rango, eTipoDefArmors tipo_def);
std::string TituloReal(std::int16_t user_index);
std::string TituloCaos(std::int16_t user_index);

void GiveFactionArmours(std::int16_t user_index, bool is_caos);
void GiveExpReward(std::int16_t user_index, std::int32_t rango);
void RecompensaArmadaReal(std::int16_t user_index);
void RecompensaCaos(std::int16_t user_index);

void EnlistarArmadaReal(std::int16_t user_index);
void EnlistarCaos(std::int16_t user_index);
void ExpulsarFaccionReal(std::int16_t user_index, bool expulsado = true);
void ExpulsarFaccionCaos(std::int16_t user_index, bool expulsado = true);

} // namespace ModFacciones

#endif // MODFACCIONES_HPP
