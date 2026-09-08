#include <doctest/doctest.h>
#include "server/clsByteQueue.hpp"
#include <vector>
#include <cstdint>

TEST_SUITE("clsByteQueue") {
    TEST_CASE("Serialización byte-exacta de entero positivo") {
        clsByteQueue queue;
        std::int16_t val = 0x1234; // 4660 decimal
        
        std::int32_t bytesWritten = queue.WriteInteger(val);
        CHECK(bytesWritten == 2);
        CHECK(queue.length() == 2);

        // Aserción estricta de la secuencia de bytes Little-Endian {0x34, 0x12}
        std::uint8_t rawBytes[2] = {0, 0};
        queue.PeekBlock(rawBytes, 2);
        CHECK(rawBytes[0] == 0x34);
        CHECK(rawBytes[1] == 0x12);

        // Verificación de lectura
        CHECK(queue.ReadInteger() == 0x1234);
        CHECK(queue.length() == 0);
    }

    TEST_CASE("Serialización byte-exacta de entero negativo (Complemento a dos)") {
        clsByteQueue queue;
        std::int16_t val = -1234; // 0xFB2E en 16-bit complemento a dos
        
        queue.WriteInteger(val);
        CHECK(queue.length() == 2);

        // Aserción estricta de la secuencia de bytes Little-Endian {0x2E, 0xFB}
        std::uint8_t rawBytes[2] = {0, 0};
        queue.PeekBlock(rawBytes, 2);
        CHECK(rawBytes[0] == 0x2E);
        CHECK(rawBytes[1] == 0xFB);

        CHECK(queue.ReadInteger() == -1234);
        CHECK(queue.length() == 0);
    }

    TEST_CASE("Serialización byte-exacta de número flotante Single (IEEE-754)") {
        clsByteQueue queue;
        float val = 1.5f; // En IEEE-754 32-bit es 0x3FC00000. Little-Endian: {0x00, 0x00, 0xC0, 0x3F}

        queue.WriteSingle(val);
        CHECK(queue.length() == 4);

        std::uint8_t rawBytes[4] = {0, 0, 0, 0};
        queue.PeekBlock(rawBytes, 4);
        CHECK(rawBytes[0] == 0x00);
        CHECK(rawBytes[1] == 0x00);
        CHECK(rawBytes[2] == 0xC0);
        CHECK(rawBytes[3] == 0x3F);

        CHECK(queue.ReadSingle() == 1.5f);
        CHECK(queue.length() == 0);
    }

    TEST_CASE("Serialización byte-exacta de cadena con caracteres acentuados (Windows-1252)") {
        clsByteQueue queue;
        std::string text = "ñandú"; // UTF-8: \xC3\xB1 a n d \xC3\xFA

        // "ñandú" en Windows-1252: ñ = 0xF1, a = 0x61, n = 0x6E, d = 0x64, ú = 0xFA (5 bytes)
        // Prefijo de longitud de 2 bytes: 0x0005 (Little-Endian: {0x05, 0x00})
        queue.WriteASCIIString(text);
        CHECK(queue.length() == 7);

        std::uint8_t rawBytes[7] = {0};
        queue.PeekBlock(rawBytes, 7);

        // Prefijo de longitud
        CHECK(rawBytes[0] == 0x05);
        CHECK(rawBytes[1] == 0x00);
        // Payload Windows-1252
        CHECK(rawBytes[2] == 0xF1); // ñ
        CHECK(rawBytes[3] == 0x61); // a
        CHECK(rawBytes[4] == 0x6E); // n
        CHECK(rawBytes[5] == 0x64); // d
        CHECK(rawBytes[6] == 0xFA); // ú

        // Verificación de lectura y reconversión a UTF-8
        CHECK(queue.ReadASCIIString() == "ñandú");
        CHECK(queue.length() == 0);
    }

    TEST_CASE("Serialización byte-exacta de cadena vacía") {
        clsByteQueue queue;

        queue.WriteASCIIString("");
        CHECK(queue.length() == 2);

        std::uint8_t rawBytes[2] = {0xFF, 0xFF};
        queue.PeekBlock(rawBytes, 2);
        CHECK(rawBytes[0] == 0x00);
        CHECK(rawBytes[1] == 0x00);

        CHECK(queue.ReadASCIIString() == "");
        CHECK(queue.length() == 0);
    }

    TEST_CASE("Comportamiento ante lectura más allá del final de los datos (Underflow)") {
        clsByteQueue queue;

        // Intentar leer de una cola totalmente vacía
        CHECK_THROWS_AS(queue.ReadInteger(), NotEnoughDataException);
        CHECK_THROWS_AS(queue.ReadInteger(), ByteQueueException);
        CHECK(queue.length() == 0);

        // Escribir 1 solo byte e intentar leer un Integer de 2 bytes
        queue.WriteByte(0x42);
        CHECK(queue.length() == 1);

        // La lectura debe fallar con NotEnoughDataException / ByteQueueException y mantener el byte inalterado en la cola
        try {
            queue.ReadInteger();
            FAIL("Debería haber lanzado NotEnoughDataException");
        } catch (const ByteQueueException& ex) {
            CHECK(ex.getErrorCode() == NOT_ENOUGH_DATA);
        }

        // El estado del buffer se conserva inalterado
        CHECK(queue.length() == 1);
        CHECK(queue.PeekByte() == 0x42);
        CHECK(queue.ReadByte() == 0x42);
        CHECK(queue.length() == 0);
    }

    TEST_CASE("Comportamiento ante escritura superando capacidad (Overflow)") {
        clsByteQueue queue(5); // Capacidad fija de 5 bytes

        queue.WriteLong(123456); // Consume 4 bytes
        CHECK(queue.length() == 4);

        // Intentar escribir 2 bytes más (excede la capacidad libre de 1 byte)
        try {
            queue.WriteInteger(10);
            FAIL("Debería haber lanzado NotEnoughSpaceException");
        } catch (const ByteQueueException& ex) {
            CHECK(ex.getErrorCode() == NOT_ENOUGH_SPACE);
        }
        CHECK(queue.length() == 4);
    }

    TEST_CASE("Overflow sobre la capacidad por defecto (10.240 bytes)") {
        clsByteQueue queue; // Capacidad por defecto = 10.240 bytes
        CHECK(queue.Capacity() == 10240);

        std::vector<std::uint8_t> block(10240, 0xAA);
        queue.WriteBlock(block);
        CHECK(queue.length() == 10240);

        // Cualquier intento de escribir más bytes debe lanzar NotEnoughSpaceException / ByteQueueException
        CHECK_THROWS_AS(queue.WriteByte(0x01), NotEnoughSpaceException);
        CHECK_THROWS_AS(queue.WriteByte(0x01), ByteQueueException);
        CHECK(queue.length() == 10240);
    }

    TEST_CASE("CopyBuffer e inspección Peek sin consumir datos") {
        clsByteQueue source;
        source.WriteLong(987654321);
        source.WriteASCIIString("Hola AO");

        clsByteQueue target;
        target.CopyBuffer(source);

        CHECK(target.Capacity() == source.Capacity());
        CHECK(target.length() == source.length());

        CHECK(target.PeekLong() == 987654321);
        CHECK(target.length() == source.length()); // Peek no consumió datos

        CHECK(target.ReadLong() == 987654321);
        CHECK(target.ReadASCIIString() == "Hola AO");
        CHECK(target.length() == 0);

        // Comprobar que source no sufrió ningún cambio
        CHECK(source.length() > 0);
    }
}
