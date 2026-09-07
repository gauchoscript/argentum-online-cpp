#include <doctest/doctest.h>
#include "server/clsIniReader.hpp"
#include <fstream>
#include <cstdio>

// Helper para crear archivos INI temporales para los tests
static std::string CrearArchivoIniTemporal(const std::string& contenido) {
    std::string filename = "temp_test_config.ini";
    std::ofstream out(filename, std::ios::binary);
    out << contenido;
    out.close();
    return filename;
}

static void BorrarArchivoIniTemporal(const std::string& filename) {
    std::remove(filename.c_str());
}

// ============================================================================
// Pruebas unitarias para clsIniReader justificadas según Testing Philosophy
// ============================================================================

/*
 * CRITERIO ATENDIDO: Edge case
 * JUSTIFICACIÓN:
 * 1. Verifica la respuesta ante claves o secciones inexistentes: el código VB6 legacy
 *    no arroja error ni excepción, sino que devuelve una cadena vacía ("").
 * 2. Verifica la respuesta ante un archivo inexistente: Initialize no falla y
 *    deja la estructura de datos vacía.
 */
TEST_CASE("clsIniReader - Manejo de casos borde y claves/secciones inexistentes") {
    std::string iniContent =
        "[INIT]\n"
        "Puerto=7680\n";
    std::string tempFile = CrearArchivoIniTemporal(iniContent);

    clsIniReader reader;
    reader.Initialize(tempFile);

    SUBCASE("Seccion o clave inexistente retorna cadena vacia") {
        CHECK(reader.GetValue("INEXISTENTE", "Puerto") == "");
        CHECK(reader.GetValue("INIT", "CLAVE_INEXISTENTE") == "");
    }

    SUBCASE("Archivo inexistente no provoca fallos y retorna cadena vacia") {
        clsIniReader emptyReader;
        emptyReader.Initialize("archivo_que_no_existe_12345.ini");
        CHECK(emptyReader.GetValue("INIT", "Puerto") == "");
        CHECK_FALSE(emptyReader.KeyExists("INIT"));
    }

    BorrarArchivoIniTemporal(tempFile);
}

/*
 * CRITERIO ATENDIDO: VB6-specific quirk
 * JUSTIFICACIÓN:
 * 1. Quirk de KeyExists: En el VB6 original, la función KeyExists(name) busca únicamente
 *    si la SECCIÓN principal (MainNode) existe, independientemente de su nombre "KeyExists".
 * 2. Case-Insensitivity: Secciones y claves son convertidas a mayúsculas (UCase$) al leer,
 *    por lo que "server", "SERVER" o "sErVeR" deben matchear exactamente el mismo valor.
 * 3. Preservacion de casing del Value: Las claves se normalizan a UCase$, pero el valor (Value)
 *    mantiene intacto su casing original.
 * 4. Modificacion con ChangeValue: Modifica el valor en memoria y es consultable mediante GetValue.
 */
TEST_CASE("clsIniReader - Quirks especificos del comportamiento VB6") {
    std::string iniContent =
        "[Servidor]\n"
        "NombreServidor=Argentum Online Rioplatense\n"
        "Puerto=7680\n"
        "PathData=C:\\Argentum\\DataFile.dat\n";
    std::string tempFile = CrearArchivoIniTemporal(iniContent);

    clsIniReader reader;
    reader.Initialize(tempFile);

    SUBCASE("KeyExists verifica la existencia de la SECCION (MainNode), no de una clave") {
        // En VB6 KeyExists("Servidor") comprueba FindMain(UCase$("Servidor")) >= 0
        CHECK(reader.KeyExists("Servidor"));
        CHECK(reader.KeyExists("SERVIDOR"));
        CHECK(reader.KeyExists("servidor"));

        // Retorna false si se le pasa una clave en lugar de una sección
        CHECK_FALSE(reader.KeyExists("Puerto"));
    }

    SUBCASE("Búsqueda totalmente case-insensitive para seccion y clave") {
        CHECK(reader.GetValue("servidor", "puerto") == "7680");
        CHECK(reader.GetValue("SERVIDOR", "PUERTO") == "7680");
        CHECK(reader.GetValue("sErViDoR", "nOmBrEsErViDoR") == "Argentum Online Rioplatense");
    }

    SUBCASE("El valor (Value) preserva el casing original mientras que la clave es case-insensitive") {
        std::string path = reader.GetValue("SERVIDOR", "PATHDATA");
        CHECK(path == "C:\\Argentum\\DataFile.dat");
    }

    SUBCASE("ChangeValue actualiza el valor almacenado") {
        reader.ChangeValue("servidor", "puerto", 9000);
        CHECK(reader.GetValue("SERVIDOR", "PUERTO") == "9000");

        reader.ChangeValue("Servidor", "NombreServidor", "Nuevo Nombre");
        CHECK(reader.GetValue("servidor", "nombreservidor") == "Nuevo Nombre");
    }

    BorrarArchivoIniTemporal(tempFile);
}

/*
 * CRITERIO ATENDIDO: Translation risk
 * JUSTIFICACIÓN:
 * 1. Codificación ANSI/Windows-1252: Verifica que caracteres de 8 bits (acentos y 'ñ')
 *    se lean y mantengan correctamente en C++.
 * 2. Tolerancia a lineas vacias, comentarios o sintaxis sin '=': En VB6 lineas que no arrancan
 *    con '[' o no contienen '=' dentro de pos >= 2 se ignoran silenciosamente.
 */
TEST_CASE("clsIniReader - Riesgos de traduccion: codificacion ANSI y tolerancia de formato") {
    std::string iniContent =
        "' Comentario inicial ignorado\n"
        "LineaInvalidaSinIgual\n"
        "\n"
        "[CONFIG]\n"
        "Descripcion=San Martín y Años\n"
        "Ciudad=Niño\n"
        "Comentario=Este es un texto con acento: canción\n";
    std::string tempFile = CrearArchivoIniTemporal(iniContent);

    clsIniReader reader;
    reader.Initialize(tempFile);

    SUBCASE("Preservacion de caracteres ANSI/Windows-1252 en valores") {
        CHECK(reader.GetValue("CONFIG", "DESCRIPCION") == "San Martín y Años");
        CHECK(reader.GetValue("config", "ciudad") == "Niño");
        CHECK(reader.GetValue("Config", "Comentario") == "Este es un texto con acento: canción");
    }

    BorrarArchivoIniTemporal(tempFile);
}
