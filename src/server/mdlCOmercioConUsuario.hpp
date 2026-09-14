#pragma once

#include <cstdint>
#include <functional>
#include <string_view>
#include "Declares.hpp"

namespace ao {

constexpr std::int32_t MAX_ORO_LOGUEABLE = 50000;
constexpr std::int32_t MAX_OBJ_LOGUEABLE = 1000;

using QuitarObjetosHook = std::function<void(std::int16_t obj_index, std::int32_t amount, std::int16_t user_index)>;
using TirarItemAlPisoHook = std::function<WorldPos(const WorldPos& pos, const Obj& obj)>;

void SetQuitarObjetosHook(QuitarObjetosHook hook) noexcept;
void SetTirarItemAlPisoHook(TirarItemAlPisoHook hook) noexcept;

/**
 * @brief Inicia o progresa en la solicitud bilateral de comercio entre usuarios.
 * Si ambos usuarios se apuntan recíprocamente (/COMERCIAR), sincroniza inventarios,
 * activa flags.Comerciando = true y despacha WriteUserCommerceInit a ambos.
 * De lo contrario, notifica al destinatario con invitación a comerciar.
 * Corresponde a IniciarComercioConUsuario en mdlCOmercioConUsuario.bas:44-80.
 */
void IniciarComercioConUsuario(std::int16_t origen, std::int16_t destino);

/**
 * @brief Despacha al usuario destino la actualización de un slot de oferta específico.
 * Traduce el slot especial GOLD_OFFER_SLOT (31) a iORO y cantidad de oro del interlocutor.
 * Corresponde a EnviarOferta en mdlCOmercioConUsuario.bas:82-105.
 */
void EnviarOferta(std::int16_t user_index, std::uint8_t offer_slot);

/**
 * @brief Restablece a cero el estado del comercio seguro bilateral.
 * Despacha WriteUserCommerceEnd si DestUsu > 0, blanquea Acepto, Confirmo, DestUsu,
 * limpia arrays de ítems y cantidades, vacía GoldAmount, DestNick y flags.Comerciando.
 * Corresponde a FinComerciarUsu en mdlCOmercioConUsuario.bas:107-133.
 */
void FinComerciarUsu(std::int16_t user_index);

/**
 * @brief Agrega o quita ítems u oro de la mesa de ofertas propia.
 * Si Confirmo es true, la oferta se encuentra bloqueada y cualquier intento es rechazado.
 * Promueve sumas a std::int64_t para prevenir desbordamientos aritméticos con signo.
 * Corresponde a AgregarOferta en mdlCOmercioConUsuario.bas:250-284.
 */
void AgregarOferta(std::int16_t user_index, std::uint8_t offer_slot, std::int16_t obj_index, std::int64_t amount, bool is_gold);

/**
 * @brief Valida si las precondiciones de la sesión comercial segura siguen vigentes.
 * Comprueba: índices en rango, conexión activa, reciprocidad en DestUsu, coincidencia
 * mutua de DestNick y estado con vida. Si alguna falla, invoca FinComerciarUsu.
 * Quirk histórico: NO valida distancia espacial física entre los usuarios.
 * Corresponde a PuedeSeguirComerciando en mdlCOmercioConUsuario.bas:286-352.
 */
bool PuedeSeguirComerciando(std::int16_t user_index);

/**
 * @brief Ejecuta el intercambio bilateral pactado entre ambos usuarios al pulsar Aceptar.
 * Si la contraparte aún no aceptó, marca Acepto = true y espera.
 * Al estar ambos aceptados, ejecuta la transferencia de los 31 slots en ambas direcciones:
 * 1. Acredita oro sin clampleo contra MAXORO (Bug #36) mediante aritmética de 64 bits.
 * 2. Entrega ítems acreditando en receptor antes de debitar en emisor (Bug #35).
 * 3. Si la mochila receptora está colmada y el piso saturado (pos 0,0), el ítem se evapora en el limbo (Bug #34).
 * Concluye invocando FinComerciarUsu para ambas partes.
 * Corresponde a AceptarComercioUsu en mdlCOmercioConUsuario.bas:135-248.
 */
void AceptarComercioUsu(std::int16_t user_index,
                        QuitarObjetosHook quitar_obj_hook = nullptr,
                        TirarItemAlPisoHook tirar_piso_hook = nullptr);

} // namespace ao
