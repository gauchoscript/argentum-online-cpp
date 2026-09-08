#include "clsByteQueue.hpp"
#include <cstring>

#ifdef _WIN32
#include <windows.h>

std::string utf8_to_cp1252(const std::string& utf8Str) {
    if (utf8Str.empty()) return "";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>(utf8Str.length()), NULL, 0);
    if (wlen <= 0) return "";
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>(utf8Str.length()), &wstr[0], wlen);

    int len1252 = WideCharToMultiByte(1252, 0, wstr.c_str(), wlen, NULL, 0, NULL, NULL);
    if (len1252 <= 0) return "";
    std::string str1252(len1252, 0);
    WideCharToMultiByte(1252, 0, wstr.c_str(), wlen, &str1252[0], len1252, NULL, NULL);
    return str1252;
}

std::string cp1252_to_utf8(const std::string& cp1252Str) {
    if (cp1252Str.empty()) return "";
    int wlen = MultiByteToWideChar(1252, 0, cp1252Str.c_str(), static_cast<int>(cp1252Str.length()), NULL, 0);
    if (wlen <= 0) return "";
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(1252, 0, cp1252Str.c_str(), static_cast<int>(cp1252Str.length()), &wstr[0], wlen);

    int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, NULL, 0, NULL, NULL);
    if (u8len <= 0) return "";
    std::string utf8Str(u8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), wlen, &utf8Str[0], u8len, NULL, NULL);
    return utf8Str;
}

std::string utf8_to_utf16le_bytes(const std::string& utf8Str) {
    if (utf8Str.empty()) return "";
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>(utf8Str.length()), NULL, 0);
    if (wlen <= 0) return "";
    std::wstring wstr(wlen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Str.c_str(), static_cast<int>(utf8Str.length()), &wstr[0], wlen);

    std::string bytes;
    bytes.resize(wstr.size() * sizeof(wchar_t));
    std::memcpy(&bytes[0], wstr.data(), bytes.size());
    return bytes;
}

std::string utf16le_bytes_to_utf8(const char* bytes, size_t byteLength) {
    if (byteLength == 0 || bytes == nullptr) return "";
    size_t wlen = byteLength / sizeof(wchar_t);
    std::wstring wstr(wlen, 0);
    std::memcpy(&wstr[0], bytes, wlen * sizeof(wchar_t));

    int u8len = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wlen), NULL, 0, NULL, NULL);
    if (u8len <= 0) return "";
    std::string utf8Str(u8len, 0);
    WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wlen), &utf8Str[0], u8len, NULL, NULL);
    return utf8Str;
}
#else
// Fallback portable implementations for non-Windows builds if required in future
std::string utf8_to_cp1252(const std::string& utf8Str) { return utf8Str; }
std::string cp1252_to_utf8(const std::string& cp1252Str) { return cp1252Str; }
std::string utf8_to_utf16le_bytes(const std::string& utf8Str) { return utf8Str; }
std::string utf16le_bytes_to_utf8(const char* bytes, size_t byteLength) { return std::string(bytes, byteLength); }
#endif

constexpr std::int32_t DATA_BUFFER = 10240;

clsByteQueue::clsByteQueue()
    : clsByteQueue(DATA_BUFFER) {
}

clsByteQueue::clsByteQueue(std::int32_t initialCapacity)
    : m_queueCapacity(initialCapacity), m_queueLength(0) {
    m_data.resize(m_queueCapacity, 0);
}

std::int32_t clsByteQueue::Capacity() const {
    return m_queueCapacity;
}

void clsByteQueue::Capacity(std::int32_t value) {
    m_queueCapacity = value;
    if (m_queueLength > value) {
        m_queueLength = value;
    }
    m_data.resize(m_queueCapacity, 0);
}

std::int32_t clsByteQueue::length() const {
    return m_queueLength;
}

void clsByteQueue::CopyBuffer(const clsByteQueue& source) {
    if (source.length() == 0) {
        RemoveData(m_queueLength);
        return;
    }

    m_queueCapacity = source.Capacity();
    m_data.resize(m_queueCapacity, 0);
    m_queueLength = 0;

    WriteBlock(source.m_data.data(), source.length());
}

std::int32_t clsByteQueue::minVal(std::int32_t val1, std::int32_t val2) {
    return (val1 < val2) ? val1 : val2;
}

std::int32_t clsByteQueue::WriteData(const std::uint8_t* buf, std::int32_t dataLength) {
    if (dataLength <= 0) return 0;
    if (m_queueCapacity - m_queueLength - dataLength < 0) {
        throw NotEnoughSpaceException();
    }
    std::memcpy(&m_data[m_queueLength], buf, dataLength);
    m_queueLength += dataLength;
    return dataLength;
}

std::int32_t clsByteQueue::ReadData(std::uint8_t* buf, std::int32_t dataLength) const {
    if (dataLength > m_queueLength) {
        throw NotEnoughDataException();
    }
    std::memcpy(buf, &m_data[0], dataLength);
    return dataLength;
}

std::int32_t clsByteQueue::RemoveData(std::int32_t dataLength) {
    std::int32_t removed = minVal(dataLength, m_queueLength);
    if (removed != m_queueCapacity && m_queueLength > removed) {
        std::memcpy(&m_data[0], &m_data[removed], m_queueLength - removed);
    }
    m_queueLength -= removed;
    return removed;
}

std::int32_t clsByteQueue::WriteByte(std::uint8_t value) {
    return WriteData(&value, 1);
}

std::int32_t clsByteQueue::WriteInteger(std::int16_t value) {
    return WriteData(reinterpret_cast<const std::uint8_t*>(&value), 2);
}

std::int32_t clsByteQueue::WriteLong(std::int32_t value) {
    return WriteData(reinterpret_cast<const std::uint8_t*>(&value), 4);
}

std::int32_t clsByteQueue::WriteSingle(float value) {
    return WriteData(reinterpret_cast<const std::uint8_t*>(&value), 4);
}

std::int32_t clsByteQueue::WriteDouble(double value) {
    return WriteData(reinterpret_cast<const std::uint8_t*>(&value), 8);
}

std::int32_t clsByteQueue::WriteBoolean(bool value) {
    std::uint8_t buf = value ? 1 : 0;
    return WriteData(&buf, 1);
}

std::int32_t clsByteQueue::WriteASCIIStringFixed(const std::string& value) {
    std::string str1252 = utf8_to_cp1252(value);
    if (str1252.empty()) return 0;
    return WriteData(reinterpret_cast<const std::uint8_t*>(str1252.data()), static_cast<std::int32_t>(str1252.length()));
}

std::int32_t clsByteQueue::WriteUnicodeStringFixed(const std::string& value) {
    std::string u16bytes = utf8_to_utf16le_bytes(value);
    if (u16bytes.empty()) return 0;
    return WriteData(reinterpret_cast<const std::uint8_t*>(u16bytes.data()), static_cast<std::int32_t>(u16bytes.length()));
}

std::int32_t clsByteQueue::WriteASCIIString(const std::string& value) {
    std::string str1252 = utf8_to_cp1252(value);
    std::int16_t len = static_cast<std::int16_t>(str1252.length());
    std::vector<std::uint8_t> buf(len + 2);
    std::memcpy(&buf[0], &len, 2);
    if (len > 0) {
        std::memcpy(&buf[2], str1252.data(), len);
    }
    return WriteData(buf.data(), len + 2);
}

std::int32_t clsByteQueue::WriteUnicodeString(const std::string& value) {
    std::string u16bytes = utf8_to_utf16le_bytes(value);
    std::int16_t charCount = static_cast<std::int16_t>(u16bytes.length() / sizeof(wchar_t));
    std::vector<std::uint8_t> buf(u16bytes.length() + 2);
    std::memcpy(&buf[0], &charCount, 2);
    if (!u16bytes.empty()) {
        std::memcpy(&buf[2], u16bytes.data(), u16bytes.length());
    }
    return WriteData(buf.data(), static_cast<std::int32_t>(buf.size()));
}

std::int32_t clsByteQueue::WriteBlock(const std::vector<std::uint8_t>& buf, std::int32_t dataLength) {
    std::int32_t len = (dataLength < 0) ? static_cast<std::int32_t>(buf.size()) : dataLength;
    return WriteData(buf.data(), len);
}

std::int32_t clsByteQueue::WriteBlock(const std::uint8_t* buf, std::int32_t dataLength) {
    return WriteData(buf, dataLength);
}

std::uint8_t clsByteQueue::ReadByte() {
    std::uint8_t val;
    ReadData(&val, 1);
    RemoveData(1);
    return val;
}

std::int16_t clsByteQueue::ReadInteger() {
    std::int16_t val;
    ReadData(reinterpret_cast<std::uint8_t*>(&val), 2);
    RemoveData(2);
    return val;
}

std::int32_t clsByteQueue::ReadLong() {
    std::int32_t val;
    ReadData(reinterpret_cast<std::uint8_t*>(&val), 4);
    RemoveData(4);
    return val;
}

float clsByteQueue::ReadSingle() {
    float val;
    ReadData(reinterpret_cast<std::uint8_t*>(&val), 4);
    RemoveData(4);
    return val;
}

double clsByteQueue::ReadDouble() {
    double val;
    ReadData(reinterpret_cast<std::uint8_t*>(&val), 8);
    RemoveData(8);
    return val;
}

bool clsByteQueue::ReadBoolean() {
    std::uint8_t val;
    ReadData(&val, 1);
    RemoveData(1);
    return (val == 1);
}

std::string clsByteQueue::ReadASCIIStringFixed(std::int32_t length) {
    if (length <= 0) return "";
    if (m_queueLength < length) throw NotEnoughDataException();
    std::string str1252(length, 0);
    ReadData(reinterpret_cast<std::uint8_t*>(&str1252[0]), length);
    RemoveData(length);
    return cp1252_to_utf8(str1252);
}

std::string clsByteQueue::ReadUnicodeStringFixed(std::int32_t length) {
    if (length <= 0) return "";
    std::int32_t byteLen = length * static_cast<std::int32_t>(sizeof(wchar_t));
    if (m_queueLength < byteLen) throw NotEnoughDataException();
    std::string bytes(byteLen, 0);
    ReadData(reinterpret_cast<std::uint8_t*>(&bytes[0]), byteLen);
    RemoveData(byteLen);
    return utf16le_bytes_to_utf8(bytes.data(), byteLen);
}

std::string clsByteQueue::ReadASCIIString() {
    if (m_queueLength <= 1) throw NotEnoughDataException();
    std::int16_t length = PeekInteger();
    if (m_queueLength < length + 2) throw NotEnoughDataException();
    RemoveData(2);
    if (length > 0) {
        std::string str1252(length, 0);
        ReadData(reinterpret_cast<std::uint8_t*>(&str1252[0]), length);
        RemoveData(length);
        return cp1252_to_utf8(str1252);
    }
    return "";
}

std::string clsByteQueue::ReadUnicodeString() {
    if (m_queueLength <= 1) throw NotEnoughDataException();
    std::int16_t charCount = PeekInteger();
    std::int32_t byteLen = charCount * static_cast<std::int32_t>(sizeof(wchar_t));
    if (m_queueLength < byteLen + 2) throw NotEnoughDataException();
    RemoveData(2);
    if (byteLen > 0) {
        std::string bytes(byteLen, 0);
        ReadData(reinterpret_cast<std::uint8_t*>(&bytes[0]), byteLen);
        RemoveData(byteLen);
        return utf16le_bytes_to_utf8(bytes.data(), byteLen);
    }
    return "";
}

std::int32_t clsByteQueue::ReadBlock(std::vector<std::uint8_t>& block, std::int32_t dataLength) {
    if (block.size() < static_cast<size_t>(dataLength)) {
        block.resize(dataLength);
    }
    ReadData(block.data(), dataLength);
    RemoveData(dataLength);
    return dataLength;
}

std::int32_t clsByteQueue::ReadBlock(std::uint8_t* block, std::int32_t dataLength) {
    ReadData(block, dataLength);
    RemoveData(dataLength);
    return dataLength;
}

std::uint8_t clsByteQueue::PeekByte() const {
    std::uint8_t val;
    ReadData(&val, 1);
    return val;
}

std::int16_t clsByteQueue::PeekInteger() const {
    std::int16_t val;
    ReadData(reinterpret_cast<std::uint8_t*>(&val), 2);
    return val;
}

std::int32_t clsByteQueue::PeekLong() const {
    std::int32_t val;
    ReadData(reinterpret_cast<std::uint8_t*>(&val), 4);
    return val;
}

float clsByteQueue::PeekSingle() const {
    float val;
    ReadData(reinterpret_cast<std::uint8_t*>(&val), 4);
    return val;
}

double clsByteQueue::PeekDouble() const {
    double val;
    ReadData(reinterpret_cast<std::uint8_t*>(&val), 8);
    return val;
}

bool clsByteQueue::PeekBoolean() const {
    std::uint8_t val;
    ReadData(&val, 1);
    return (val == 1);
}

std::string clsByteQueue::PeekASCIIStringFixed(std::int32_t length) const {
    if (length <= 0) return "";
    if (m_queueLength < length) throw NotEnoughDataException();
    std::string str1252(length, 0);
    ReadData(reinterpret_cast<std::uint8_t*>(&str1252[0]), length);
    return cp1252_to_utf8(str1252);
}

std::string clsByteQueue::PeekUnicodeStringFixed(std::int32_t length) const {
    if (length <= 0) return "";
    std::int32_t byteLen = length * static_cast<std::int32_t>(sizeof(wchar_t));
    if (m_queueLength < byteLen) throw NotEnoughDataException();
    std::string bytes(byteLen, 0);
    ReadData(reinterpret_cast<std::uint8_t*>(&bytes[0]), byteLen);
    return utf16le_bytes_to_utf8(bytes.data(), byteLen);
}

std::string clsByteQueue::PeekASCIIString() const {
    if (m_queueLength <= 1) throw NotEnoughDataException();
    std::int16_t length = PeekInteger();
    if (m_queueLength < length + 2) throw NotEnoughDataException();
    if (length > 0) {
        std::string str1252(length, 0);
        std::memcpy(&str1252[0], &m_data[2], length);
        return cp1252_to_utf8(str1252);
    }
    return "";
}

std::string clsByteQueue::PeekUnicodeString() const {
    if (m_queueLength <= 1) throw NotEnoughDataException();
    std::int16_t charCount = PeekInteger();
    std::int32_t byteLen = charCount * static_cast<std::int32_t>(sizeof(wchar_t));
    if (m_queueLength < byteLen + 2) throw NotEnoughDataException();
    if (byteLen > 0) {
        std::string bytes(byteLen, 0);
        std::memcpy(&bytes[0], &m_data[2], byteLen);
        return utf16le_bytes_to_utf8(bytes.data(), byteLen);
    }
    return "";
}

std::int32_t clsByteQueue::PeekBlock(std::vector<std::uint8_t>& block, std::int32_t dataLength) const {
    if (block.size() < static_cast<size_t>(dataLength)) {
        block.resize(dataLength);
    }
    ReadData(block.data(), dataLength);
    return dataLength;
}

std::int32_t clsByteQueue::PeekBlock(std::uint8_t* block, std::int32_t dataLength) const {
    ReadData(block, dataLength);
    return dataLength;
}
