#include <doctest/doctest.h>
#include "server/clsdicc.hpp"

// ============================================================================
// Pruebas unitarias para clsdicc justificadas según la Filosofía de Testing
// (docs/CONVENTIONS.md)
// ============================================================================

/*
 * CRITERIO ATENDIDO: VB6-specific quirk / Audit finding
 * JUSTIFICACIÓN:
 * Inspección y prueba exhaustiva del comportamiento del método MayorValor en empates.
 * El código legacy VB6 en clsdicc.cls concatena TODAS las claves empatadas en el valor
 * máximo separadas por comas, respetando el orden de inserción original (no alfabético),
 * en mayúsculas, y devuelve en 'cant' la cantidad exacta de claves empatadas.
 */
TEST_CASE("clsdicc - Comportamiento de desempate y concatenacion en MayorValor") {

    SUBCASE("Sin empate (un solo ganador claro con el valor mas alto)") {
        clsdicc dicc;
        dicc.AtPut("Perez", 5);
        dicc.AtPut("Gomez", 12);
        dicc.AtPut("Lopez", 8);

        int cant = 0;
        std::string resultado = dicc.MayorValor(cant);

        CHECK(resultado == "GOMEZ");
        CHECK(cant == 1);
    }

    SUBCASE("Empate doble (dos claves comparten el valor maximo)") {
        clsdicc dicc;
        dicc.AtPut("Perez", 10);
        dicc.AtPut("Gomez", 10);
        dicc.AtPut("Lopez", 3);

        int cant = 0;
        std::string resultado = dicc.MayorValor(cant);

        // Retorna ambas claves en orden de insercion separadas por coma, cant = 2
        CHECK(resultado == "PEREZ,GOMEZ");
        CHECK(cant == 2);
    }

    SUBCASE("Empate triple o múltiple (mas de dos claves comparten el valor maximo)") {
        clsdicc dicc;
        dicc.AtPut("Perez", 15);
        dicc.AtPut("Gomez", 15);
        dicc.AtPut("Lopez", 15);
        dicc.AtPut("Diaz", 15);
        dicc.AtPut("Sanchez", 2);

        int cant = 0;
        std::string resultado = dicc.MayorValor(cant);

        // Confirma que no esta hardcodeado a exactamente dos empates
        CHECK(resultado == "PEREZ,GOMEZ,LOPEZ,DIAZ");
        CHECK(cant == 4);
    }

    SUBCASE("El orden de insercion (no alfabetico) determina la secuencia de la coma") {
        clsdicc dicc;
        // Insertamos intencionalmente en orden no alfabético: Zeballos -> Alvarez -> Brizuela
        dicc.AtPut("Zeballos", 100);
        dicc.AtPut("Alvarez", 100);
        dicc.AtPut("Brizuela", 100);

        int cant = 0;
        std::string resultado = dicc.MayorValor(cant);

        // Debe coincidir con el orden de inserción ZEBALLOS,ALVAREZ,BRIZUELA y NO con el orden alfabético
        CHECK(resultado == "ZEBALLOS,ALVAREZ,BRIZUELA");
        CHECK(resultado != "ALVAREZ,BRIZUELA,ZEBALLOS");
        CHECK(cant == 3);
    }
}

/*
 * CRITERIO ATENDIDO: VB6-specific quirk / Audit finding
 * JUSTIFICACIÓN:
 * En el VB6 original, UCase$ normaliza todas las claves a mayúsculas en inserciones y búsquedas.
 * La prueba confirma que la búsqueda es insensible a mayúsculas/minúsculas y que las claves
 * se devuelven en mayúsculas en AtIndex y MayorValor.
 */
TEST_CASE("clsdicc - Normalizacion case-insensitive de claves") {
    clsdicc dicc;

    SUBCASE("Insercion con diferente casing actualiza la misma entrada") {
        CHECK(dicc.AtPut("perez", 10));
        CHECK(dicc.CantElem() == 1);
        CHECK(dicc.At("perez") == "10");
        CHECK(dicc.At("PEREZ") == "10");

        // Actualización con casing distinto
        CHECK(dicc.AtPut("PeReZ", 25));
        CHECK(dicc.CantElem() == 1); // No incrementa la cantidad de elementos
        CHECK(dicc.At("PEREZ") == "25");
        CHECK(dicc.At("perez") == "25");
    }

    SUBCASE("AtIndex y MayorValor entregan las claves normalizadas en MAYUSCULAS") {
        dicc.AtPut("juan", 10);
        dicc.AtPut("pedro", 20);

        CHECK(dicc.AtIndex(1) == "JUAN");
        CHECK(dicc.AtIndex(2) == "PEDRO");

        int cant = 0;
        CHECK(dicc.MayorValor(cant) == "PEDRO");
    }
}

/*
 * CRITERIO ATENDIDO: Edge case
 * JUSTIFICACIÓN:
 * Verifica el comportamiento del módulo ante diccionarios vacíos, índices fuera de rango (1-based),
 * inserción de claves vacías y el vaciado completo con DumpAll.
 */
TEST_CASE("clsdicc - Casos borde: vacio, limites de AtIndex, clave vacia y DumpAll") {
    clsdicc dicc;

    SUBCASE("Diccionario vacio") {
        CHECK(dicc.CantElem() == 0);
        CHECK(dicc.At("cualquiera") == "");
        CHECK(dicc.AtIndex(1) == "");

        int cant = 99;
        CHECK(dicc.MayorValor(cant) == "");
        CHECK(cant == 0);
    }

    SUBCASE("AtIndex valida rango 1-based") {
        dicc.AtPut("Uno", 1);
        dicc.AtPut("Dos", 2);

        CHECK(dicc.AtIndex(0) == "");   // Fuera de rango (base 1)
        CHECK(dicc.AtIndex(1) == "UNO");
        CHECK(dicc.AtIndex(2) == "DOS");
        CHECK(dicc.AtIndex(3) == "");   // Fuera de rango
    }

    SUBCASE("AtPut rechaza clave vacia") {
        CHECK_FALSE(dicc.AtPut("", 100));
        CHECK(dicc.CantElem() == 0);
    }

    SUBCASE("DumpAll limpia completamente la estructura") {
        dicc.AtPut("A", 1);
        dicc.AtPut("B", 2);
        CHECK(dicc.CantElem() == 2);

        dicc.DumpAll();
        CHECK(dicc.CantElem() == 0);
        CHECK(dicc.At("A") == "");
        CHECK(dicc.AtIndex(1) == "");
    }
}
