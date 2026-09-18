#pragma once

#include <cstdint>

constexpr std::int16_t INVALID_INDEX = 0;

/**
 * @brief Obtiene el UserIndex correspondiente a un CharIndex del mapa.
 *
 * @param char_index El índice de personaje en el mapa.
 * @return std::int16_t El UserIndex asignado en UserList, o INVALID_INDEX (0) si no es un usuario o el índice es inválido.
 */
std::int16_t CharIndexToUserIndex(std::int16_t char_index) noexcept;
