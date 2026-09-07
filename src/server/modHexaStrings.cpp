#include "modHexaStrings.hpp"
#include <cctype>

std::int32_t hexHex2Dec(const std::string& hex) {
    std::uint32_t result = 0;

    for (char c : hex) {
        if (c == ' ') {
            continue; // VB6 Val omite espacios
        }

        int digit = -1;
        if (c >= '0' && c <= '9') {
            digit = c - '0';
        } else if (c >= 'a' && c <= 'f') {
            digit = c - 'a' + 10;
        } else if (c >= 'A' && c <= 'F') {
            digit = c - 'A' + 10;
        } else {
            // Primer carácter no hexadecimal: VB6 Val detiene la conversión inmediatamente
            break;
        }

        result = (result << 4) | static_cast<std::uint32_t>(digit);
    }

    return static_cast<std::int32_t>(result);
}

std::string hexMd52Asc(const std::string& MD5) {
    std::string workingMd5 = MD5;

    // Quirk de VB6: si la longitud de la cadena es impar, antepone '0'
    if ((workingMd5.length() & 1) != 0) {
        workingMd5.insert(0, "0");
    }

    std::string result;
    result.reserve(workingMd5.length() / 2);

    for (std::size_t i = 0; i < workingMd5.length(); i += 2) {
        std::string chunk = workingMd5.substr(i, 2);
        std::int32_t decVal = hexHex2Dec(chunk);
        result.push_back(static_cast<char>(static_cast<std::uint8_t>(decVal & 0xFF)));
    }

    return result;
}

std::string txtOffset(const std::string& Text, std::int16_t off) {
    std::string result;
    result.reserve(Text.length());

    for (char c : Text) {
        std::uint8_t asciiVal = static_cast<std::uint8_t>(c);
        std::uint8_t shifted = static_cast<std::uint8_t>((asciiVal + off) & 0xFF);
        result.push_back(static_cast<char>(shifted));
    }

    return result;
}
