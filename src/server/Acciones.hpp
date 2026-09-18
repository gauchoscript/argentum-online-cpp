#ifndef ACCIONES_HPP
#define ACCIONES_HPP

#include <cstdint>
#include <functional>
#include <string>

#include "Declares.hpp"
#include "Protocol.hpp"

namespace Acciones {

constexpr int16_t RANGO_VISION_X = 8;
constexpr int16_t RANGO_VISION_Y = 6;

struct AccionesCallbacks {
    // Red y Notificaciones
    std::function<void(int16_t user_index, const std::string& msg, uint8_t font_type)> WriteConsoleMsg;
    std::function<void(int16_t user_index, int16_t obj_index)> WriteShowSignal;
    std::function<void(int16_t user_index)> WriteShowForumForm;
    std::function<void(int16_t user_index)> WriteUpdateUserStats;
    std::function<void(int16_t map, int16_t x, int16_t y, uint16_t grh_index)> SendObjectCreateToArea;
    std::function<void(int16_t map, int16_t x, int16_t y, uint16_t wave_index)> SendPlayWaveToArea;

    // Negocio y Usuarios
    std::function<void(int16_t user_index)> IniciarComercioNPC;
    std::function<void(int16_t user_index)> IniciarDepositoBanco;
    std::function<void(int16_t user_index)> RevivirUsuario;
    std::function<bool(int16_t user_index)> EsNewbie;
    std::function<void(int16_t user_index, eSkill skill, bool exito)> SubirSkill;
    std::function<bool(int16_t user_index, int16_t foro_id)> SendPosts;

    // Entorno, Objetos y Basura
    std::function<void(const Obj& obj, int16_t map, int16_t x, int16_t y)> MakeObj;
    std::function<void(bool bloquear, int16_t map, int16_t x, int16_t y, uint8_t flag)> SetTileBlocked;
    std::function<void(int16_t map, int16_t x, int16_t y)> RegisterGarbageFire;
};

void SetAccionesCallbacks(const AccionesCallbacks& callbacks);
void InitDefaultAccionesCallbacks();

// Las 5 rutinas del módulo legacy Acciones.bas
void Accion(int16_t user_index, int16_t map, int16_t x, int16_t y);
void AccionParaForo(int16_t map, int16_t x, int16_t y, int16_t user_index);
void AccionParaPuerta(int16_t map, int16_t x, int16_t y, int16_t user_index);
void AccionParaCartel(int16_t map, int16_t x, int16_t y, int16_t user_index);
void AccionParaRamita(int16_t map, int16_t x, int16_t y, int16_t user_index);

} // namespace Acciones

#endif // ACCIONES_HPP
