#pragma once

#include <cstdint>
#include <string>

/**
 * @brief Convierte una cadena hexadecimal (por ejemplo, hash MD5 en formato hex) a sus bytes ASCII correspondientes.
 * 
 * Quirk VB6: Si la longitud de la cadena recibida es impar, antepone un '0' al principio.
 * La conversión de pares hexadecimales a byte es insensible a mayúsculas/minúsculas (A-F vs a-f).
 */
std::string hexMd52Asc(const std::string& MD5);

/**
 * @brief Convierte una cadena de dígitos hexadecimales a un entero signado de 32 bits (Long en VB6).
 * 
 * Emula la semántica exacta de Val("&H" & hex) en VB6:
 * - Insensible a mayúsculas/minúsculas.
 * - Omite espacios en blanco.
 * - Detiene el parseo al encontrar el primer carácter no-hexadecimal, retornando la acumulación previa.
 */
std::int32_t hexHex2Dec(const std::string& hex);

/**
 * @brief Aplica un desplazamiento (offset) al valor ASCII de cada carácter de la cadena Text (módulo 256).
 */
std::string txtOffset(const std::string& Text, std::int16_t off);
