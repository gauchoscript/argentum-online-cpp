#include <doctest/doctest.h>
#include "server/ModCola.hpp"
#include <regex>

// ============================================================================
// Pruebas unitarias para cCola (ModCola.cls) según la Filosofía de Testing
// (docs/CONVENTIONS.md)
// ============================================================================

/*
 * CRITERIO ATENDIDO: VB6-specific quirk / Audit finding
 * JUSTIFICACIÓN:
 * En el VB6 original, Push agrega la hora del sistema formateada mediante time$ ("HH:MM:SS")
 * seguida de un espacio y el nombre del usuario en mayúsculas ("HH:MM:SS NOMBRE").
 * Esta prueba verifica que la marca de tiempo cumpla con la expresión regular HH:MM:SS (8 caracteres),
 * que el espacio delimitador exista y que el nombre se almacene en mayúsculas.
 */
TEST_CASE("cCola - Formato exacto de timestamp producido por Push") {
    cCola cola;
    cola.Push("JuanPerez");

    CHECK(cola.Longitud() == 1);
    std::string elem = cola.PopByVal();

    // Debe tener al menos 9 caracteres ("HH:MM:SS " + nombre)
    REQUIRE(elem.length() > 9);

    // Formato de hora de 8 caracteres HH:MM:SS en el índice 0..7
    std::string timePart = elem.substr(0, 8);
    std::regex timeRegex(R"(\d{2}:\d{2}:\d{2})");
    CHECK(std::regex_match(timePart, timeRegex));

    // Espacio separador en la posición 8
    CHECK(elem[8] == ' ');

    // Nombre en mayúsculas a partir de la posición 9
    CHECK(elem.substr(9) == "JUANPEREZ");
}

/*
 * CRITERIO ATENDIDO: VB6-specific quirk / Audit finding
 * JUSTIFICACIÓN:
 * En VB6, la función Existe recorre la cola utilizando Mid$(VerElemento(i), 10)
 * para ignorar el timestamp prefijado ("HH:MM:SS ") y comparar contra el nombre en mayúsculas.
 * Se prueba con PushConTiempo para garantizar determinismo en el timestamp.
 */
TEST_CASE("cCola - Existe ignora correctamente el prefijo de timestamp") {
    cCola cola;
    cola.PushConTiempo("Gamer1", "12:34:56");
    cola.PushConTiempo("AdminJose", "23:59:59");
    cola.PushConTiempo("Carlos_Ao", "00:00:01");

    SUBCASE("Busqueda case-insensitive encuentra usuarios independientemente del timestamp") {
        CHECK(cola.Existe("GAMER1"));
        CHECK(cola.Existe("gamer1"));
        CHECK(cola.Existe("Gamer1"));

        CHECK(cola.Existe("ADMINJOSE"));
        CHECK(cola.Existe("adminjose"));

        CHECK(cola.Existe("CARLOS_AO"));
        CHECK(cola.Existe("carlos_ao"));
    }

    SUBCASE("No coincide con horas ni fragmentos de timestamp") {
        CHECK_FALSE(cola.Existe("12:34:56"));
        CHECK_FALSE(cola.Existe("23:59:59"));
        CHECK_FALSE(cola.Existe("12:34:56 Gamer1"));
    }

    SUBCASE("Usuario inexistente devuelve false") {
        CHECK_FALSE(cola.Existe("UsuarioInexistente"));
    }
}

/*
 * CRITERIO ATENDIDO: Edge case
 * JUSTIFICACIÓN:
 * Requisito explícito de la consigna: probar Quitar y QuitarIndex en casos borde
 * (lista vacía, eliminar usuario inexistente, índices fuera de rango).
 */
TEST_CASE("cCola - Casos borde en Quitar y QuitarIndex") {
    cCola cola;

    SUBCASE("Quitar y QuitarIndex sobre lista vacia no fallan ni corrompen estado") {
        cola.Quitar("Alguien");
        CHECK(cola.Longitud() == 0);

        cola.QuitarIndex(1);
        cola.QuitarIndex(0);
        cola.QuitarIndex(-5);
        cola.QuitarIndex(100);
        CHECK(cola.Longitud() == 0);
    }

    SUBCASE("Quitar un nombre inexistente no altera la cola") {
        cola.PushConTiempo("Pedro", "10:00:00");
        cola.PushConTiempo("Maria", "10:01:00");
        CHECK(cola.Longitud() == 2);

        cola.Quitar("Inexistente");
        CHECK(cola.Longitud() == 2);
        CHECK(cola.Existe("Pedro"));
        CHECK(cola.Existe("Maria"));
    }

    SUBCASE("QuitarIndex con indices fuera de rango (base 1)") {
        cola.PushConTiempo("Uno", "10:00:00");
        cola.PushConTiempo("Dos", "10:01:00");

        // Índices fuera de rango no eliminan nada
        cola.QuitarIndex(0);    // Menor a 1
        cola.QuitarIndex(-1);   // Negativo
        cola.QuitarIndex(3);    // Mayor a Longitud()
        CHECK(cola.Longitud() == 2);

        // Eliminación válida en índice 1 (remueve "Uno")
        cola.QuitarIndex(1);
        CHECK(cola.Longitud() == 1);
        CHECK_FALSE(cola.Existe("Uno"));
        CHECK(cola.Existe("Dos"));
    }

    SUBCASE("Quitar elimina solo la primera ocurrencia encontrada") {
        cola.PushConTiempo("Repetido", "10:00:00");
        cola.PushConTiempo("Otro", "10:01:00");
        cola.PushConTiempo("Repetido", "10:02:00");
        CHECK(cola.Longitud() == 3);

        cola.Quitar("Repetido");
        CHECK(cola.Longitud() == 2);
        // Sigue existiendo la segunda ocurrencia
        CHECK(cola.Existe("Repetido"));

        // Inspecciona que haya quedado la segunda instancia ("10:02:00 REPETIDO")
        CHECK(cola.VerElemento(2).substr(0, 8) == "10:02:00");
    }
}

/*
 * CRITERIO ATENDIDO: VB6-specific quirk / Audit finding
 * JUSTIFICACIÓN:
 * En VB6, al hacer Pop o PopByVal sobre una cola vacía o pedir VerElemento con un índice inválido,
 * el comportamiento debido a On Error Resume Next y retornos por defecto asigna el caracter "0".
 * Esta prueba protege la compatibilidad exacta con esa respuesta por defecto de VB6.
 */
TEST_CASE("cCola - Comportamiento de Pop, PopByVal y VerElemento en cola vacia e indices invalidos") {
    cCola cola;

    SUBCASE("Pop y PopByVal devuelven string '0' cuando la cola esta vacia") {
        CHECK(cola.Pop() == "0");
        CHECK(cola.PopByVal() == "0");
        CHECK(cola.Longitud() == 0);
    }

    SUBCASE("VerElemento devuelve '0' para indices invalidos y el elemento en mayusculas para validos") {
        cola.PushConTiempo("morgolock", "15:30:00");

        CHECK(cola.VerElemento(0) == "0");
        CHECK(cola.VerElemento(-1) == "0");
        CHECK(cola.VerElemento(2) == "0");

        CHECK(cola.VerElemento(1) == "15:30:00 MORGOLOCK");
    }

    SUBCASE("Reset vacia completamente la cola") {
        cola.Push("User1");
        cola.Push("User2");
        CHECK(cola.Longitud() == 2);

        cola.Reset();
        CHECK(cola.Longitud() == 0);
        CHECK(cola.Pop() == "0");
    }
}
