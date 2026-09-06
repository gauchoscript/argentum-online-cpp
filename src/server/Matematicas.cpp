#include "Matematicas.hpp"
#include <cmath>
#include <cstdlib>

std::int32_t Porcentaje(std::int32_t Total, std::int32_t Porc) {
    return (Total * Porc) / 100;
}

std::int32_t Distancia(const WorldPos& wp1, const WorldPos& wp2) {
    return std::abs(wp1.X - wp2.X) + std::abs(wp1.Y - wp2.Y) + (std::abs(wp1.Map - wp2.Map) * 100);
}

double Distance(std::int16_t X1, std::int16_t Y1, std::int16_t X2, std::int16_t Y2) {
    return std::sqrt(std::pow(Y1 - Y2, 2) + std::pow(X1 - X2, 2));
}

std::int32_t RandomNumber(std::int32_t LowerBound, std::int32_t UpperBound) {
    float Rnd = static_cast<float>(std::rand()) / (static_cast<float>(RAND_MAX) + 1.0f);
    return static_cast<std::int32_t>(Rnd * static_cast<float>(UpperBound - LowerBound + 1)) + LowerBound;
}
