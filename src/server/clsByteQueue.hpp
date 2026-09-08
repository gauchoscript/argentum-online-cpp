#ifndef CLSBYTEQUEUE_HPP
#define CLSBYTEQUEUE_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <stdexcept>

/*
 * clsByteQueue - Adaptación C++ de la cola FIFO de bytes de VB6 (legacy/server/Codigo/clsByteQueue.cls)
 *
 * NOTAS DE COMPORTAMIENTO Y COMPATIBILIDAD LEGACY:
 *
 * 1. Representación en memoria de tipos numéricos (Little-Endian):
 *    Al igual que en VB6, donde se utiliza CopyMemory (RtlMoveMemory), la lectura y escritura
 *    de Integer (int16_t), Long (int32_t), Single (float) y Double (double) copia la representación
 *    directa de bytes en formato Little-Endian nativo x86/x64 mediante std::memcpy.
 *
 * 2. Estructura del Búfer y Comportamiento de Desplazamiento (Shift-on-Remove):
 *    No es una cola circular. Al remover datos (RemoveData), los elementos remanentes se desplazan
 *    hacia el índice 0 del búfer interno mediante std::memcpy.
 *
 * 3. Gestión de Errores y Límites (Underflow / Overflow):
 *    - Si no hay suficientes datos para leer (Underflow), se lanza NotEnoughDataException
 *      (código legacy vbObjectError + 9 = -2147221495). La cola no se modifica en absoluto.
 *    - Si una escritura supera la capacidad libre (Overflow), se lanza NotEnoughSpaceException
 *      (código legacy vbObjectError + 10 = -2147221494).
 *
 * 4. Serialización de Cadenas de Caracteres:
 *    - WriteASCIIString / ReadASCIIString: Escribe/lee un prefijo de 2 bytes (int16_t Little-Endian)
 *      con la longitud en bytes, seguido inmediatamente por los bytes codificados en Windows-1252.
 *    - WriteASCIIStringFixed / ReadASCIIStringFixed: Escribe/lee N bytes crudos en Windows-1252
 *      sin prefijo de longitud ni terminador nulo.
 */

// Código de error devuelto cuando no hay suficientes datos para leer (vbObjectError + 9)
constexpr std::int32_t NOT_ENOUGH_DATA = -2147221495;

// Código de error devuelto cuando no hay suficiente espacio para escribir (vbObjectError + 10)
constexpr std::int32_t NOT_ENOUGH_SPACE = -2147221494;

class ByteQueueException : public std::runtime_error {
public:
    ByteQueueException(const std::string& message, std::int32_t errorCode)
        : std::runtime_error(message), m_errorCode(errorCode) {}
    std::int32_t getErrorCode() const { return m_errorCode; }
private:
    std::int32_t m_errorCode;
};

class NotEnoughDataException : public ByteQueueException {
public:
    NotEnoughDataException() 
        : ByteQueueException("Not enough data in queue", NOT_ENOUGH_DATA) {}
};

class NotEnoughSpaceException : public ByteQueueException {
public:
    NotEnoughSpaceException() 
        : ByteQueueException("Not enough space in queue", NOT_ENOUGH_SPACE) {}
};

class clsByteQueue {
public:
    clsByteQueue();
    explicit clsByteQueue(std::int32_t initialCapacity);
    ~clsByteQueue() = default;

    // Propiedades de Capacidad y Longitud
    std::int32_t Capacity() const;
    void Capacity(std::int32_t value);
    std::int32_t length() const;

    std::int32_t NotEnoughDataErrCode() const { return NOT_ENOUGH_DATA; }
    std::int32_t NotEnoughSpaceErrCode() const { return NOT_ENOUGH_SPACE; }

    // Copia el búfer de otra cola a esta instancia
    void CopyBuffer(const clsByteQueue& source);

    // Métodos de Escritura (Write)
    std::int32_t WriteByte(std::uint8_t value);
    std::int32_t WriteInteger(std::int16_t value);
    std::int32_t WriteLong(std::int32_t value);
    std::int32_t WriteSingle(float value);
    std::int32_t WriteDouble(double value);
    std::int32_t WriteBoolean(bool value);
    std::int32_t WriteASCIIStringFixed(const std::string& value);
    std::int32_t WriteUnicodeStringFixed(const std::string& value);
    std::int32_t WriteASCIIString(const std::string& value);
    std::int32_t WriteUnicodeString(const std::string& value);
    std::int32_t WriteBlock(const std::vector<std::uint8_t>& buf, std::int32_t dataLength = -1);
    std::int32_t WriteBlock(const std::uint8_t* buf, std::int32_t dataLength);

    // Métodos de Lectura (Read) - Remueven los datos del búfer
    std::uint8_t ReadByte();
    std::int16_t ReadInteger();
    std::int32_t ReadLong();
    float ReadSingle();
    double ReadDouble();
    bool ReadBoolean();
    std::string ReadASCIIStringFixed(std::int32_t length);
    std::string ReadUnicodeStringFixed(std::int32_t length);
    std::string ReadASCIIString();
    std::string ReadUnicodeString();
    std::int32_t ReadBlock(std::vector<std::uint8_t>& block, std::int32_t dataLength);
    std::int32_t ReadBlock(std::uint8_t* block, std::int32_t dataLength);

    // Métodos de Inspección (Peek) - No remueven los datos del búfer
    std::uint8_t PeekByte() const;
    std::int16_t PeekInteger() const;
    std::int32_t PeekLong() const;
    float PeekSingle() const;
    double PeekDouble() const;
    bool PeekBoolean() const;
    std::string PeekASCIIStringFixed(std::int32_t length) const;
    std::string PeekUnicodeStringFixed(std::int32_t length) const;
    std::string PeekASCIIString() const;
    std::string PeekUnicodeString() const;
    std::int32_t PeekBlock(std::vector<std::uint8_t>& block, std::int32_t dataLength) const;
    std::int32_t PeekBlock(std::uint8_t* block, std::int32_t dataLength) const;

private:
    std::int32_t WriteData(const std::uint8_t* buf, std::int32_t dataLength);
    std::int32_t ReadData(std::uint8_t* buf, std::int32_t dataLength) const;
    std::int32_t RemoveData(std::int32_t dataLength);

    static std::int32_t minVal(std::int32_t val1, std::int32_t val2);

    std::vector<std::uint8_t> m_data;
    std::int32_t m_queueCapacity;
    std::int32_t m_queueLength;
};

// Funciones auxiliares de conversión de codificación (Windows-1252 / UTF-16LE <-> UTF-8)
std::string utf8_to_cp1252(const std::string& utf8Str);
std::string cp1252_to_utf8(const std::string& cp1252Str);
std::string utf8_to_utf16le_bytes(const std::string& utf8Str);
std::string utf16le_bytes_to_utf8(const char* bytes, size_t byteLength);

#endif // CLSBYTEQUEUE_HPP
