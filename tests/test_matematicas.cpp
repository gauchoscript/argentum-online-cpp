#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include "server/Matematicas.hpp"

// ==========================================
// Prueba de valor para RandomNumber
// ==========================================

/*
 * Traza manual de la lógica legacy VB6:
 * Public Function RandomNumber(ByVal LowerBound As Long, ByVal UpperBound As Long) As Long
 *     RandomNumber = Fix(Rnd * (UpperBound - LowerBound + 1)) + LowerBound
 * End Function
 *
 * Caso 1: Rango de 1 sólo valor (LowerBound = UpperBound = 5)
 * Traza: (5 - 5 + 1) = 1. Rnd * 1 < 1.0. Fix(Rnd * 1) = 0. Resultado = 0 + 5 = 5.
 * Resultado esperado: 5 siempre.
 *
 * Caso 2: Rango cerrado (LowerBound = 1, UpperBound = 10)
 * Traza: Rnd * 10 produce valores en [0.0, 10.0). Fix produce enteros en [0, 9].
 * Sumando LowerBound (1), el resultado debe estar estrictamente acotado en [1, 10].
 * Resultado esperado: cada iteración devuelve un entero en el rango cerrado [1, 10].
 */
TEST_CASE("RandomNumber respeta los limites del rango inclusive") {
    SUBCASE("Mismo limite inferior y superior devuelve el valor exacto") {
        CHECK(RandomNumber(5, 5) == 5);
        CHECK(RandomNumber(-3, -3) == -3);
    }

    SUBCASE("Generacion en rango valido acotada inclusive") {
        for (int i = 0; i < 1000; ++i) {
            std::int32_t val = RandomNumber(1, 10);
            CHECK(val >= 1);
            CHECK(val <= 10);
        }
    }
}
