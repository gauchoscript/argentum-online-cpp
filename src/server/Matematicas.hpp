#pragma once

#include <cstdint>

// Estructura WorldPos utilizada en Matematicas
// (Definida originalmente en Declares.bas)
struct WorldPos {
    std::int16_t Map{0};
    std::int16_t X{0};
    std::int16_t Y{0};
};

std::int32_t Porcentaje(std::int32_t Total, std::int32_t Porc);
std::int32_t Distancia(const WorldPos& wp1, const WorldPos& wp2);
double Distance(std::int16_t X1, std::int16_t Y1, std::int16_t X2, std::int16_t Y2);
std::int32_t RandomNumber(std::int32_t LowerBound, std::int32_t UpperBound);
