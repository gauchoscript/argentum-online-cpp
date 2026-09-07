#include <doctest/doctest.h>
#include "server/modHexaStrings.hpp"

TEST_SUITE("modHexaStrings") {

    TEST_CASE("Insensibilidad a Mayúsculas/Minúsculas en Entrada Hexadecimal") {
        // Audit quirk: hexHex2Dec y hexMd52Asc deben producir los mismos bytes ASCII sin importar
        // si los caracteres hexadecimales están en mayúsculas o minúsculas (a-f vs A-F).
        CHECK(hexHex2Dec("4a") == 74);
        CHECK(hexHex2Dec("4A") == 74);
        CHECK(hexHex2Dec("ff") == 255);
        CHECK(hexHex2Dec("FF") == 255);

        CHECK(hexMd52Asc("4a4b") == "JK");
        CHECK(hexMd52Asc("4A4B") == "JK");
        CHECK(hexMd52Asc("4a4B") == "JK");
    }

    TEST_CASE("Comportamiento de Zero-Padding para Cadenas de Longitud Impar") {
        // Audit quirk: si la cadena MD5 tiene longitud impar, VB6 antepone '0' al principio.
        // "A" se convierte en "0A" (decimal 10, LF '\n')
        CHECK(hexMd52Asc("A") == "\n");
        CHECK(hexMd52Asc("a") == "\n");
        // "123" se convierte en "0123" -> "01" (ASCII 1 '\x01') y "23" (ASCII 35 '#')
        CHECK(hexMd52Asc("123") == "\x01#");
    }

    TEST_CASE("Casos Límite: Entrada Vacía y Caracteres No-Hexadecimales") {
        // Entrada vacía
        CHECK(hexMd52Asc("") == "");
        CHECK(hexHex2Dec("") == 0);
        CHECK(txtOffset("", 55) == "");

        // Caracteres no-hexadecimales en hexHex2Dec (emulación de Val("&H" & hex) de VB6)
        // Si comienza con no-hex, se detiene inmediatamente y retorna 0
        CHECK(hexHex2Dec("G1") == 0);
        CHECK(hexHex2Dec("XYZ") == 0);
        // Si comienza con dígitos hex válidos y luego no-hex, retorna la acumulación previa
        CHECK(hexHex2Dec("4G") == 4);
        CHECK(hexHex2Dec("10Z") == 16);

        // Impacto en hexMd52Asc con caracteres no-hex
        // "G1" -> 0 -> byte nulo '\0'
        CHECK(hexMd52Asc("G1") == std::string("\0", 1));
    }

    TEST_CASE("Transformación de Desplazamiento de Texto (txtOffset)") {
        // Desplazamiento normal
        CHECK(txtOffset("ABC", 1) == "BCD");
        
        // Offset de 55 utilizado en autenticación MD5 (Admin.bas)
        CHECK(txtOffset("A", 55) == "x"); // 'A' (65) + 55 = 120 ('x')

        // Desbordamiento (wraparound) de byte (módulo 256)
        std::string maxByte = "\xFF";
        CHECK(txtOffset(maxByte, 1) == std::string("\0", 1));
        CHECK(txtOffset("A", 256) == "A");
    }
}
