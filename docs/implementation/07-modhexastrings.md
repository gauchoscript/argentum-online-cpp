---
area: implementacion
status: completed
module: modHexaStrings
layer: 0
legacy_source: legacy/server/Codigo/modHexaStrings.bas
target_header: src/server/modHexaStrings.hpp
target_source: src/server/modHexaStrings.cpp
test_suite: tests/test_modhexastrings.cpp
last_updated: 2026-09-08
---

# Módulo #7: modHexaStrings

## Resumen

Se completó la migración transliterada 1:1 del módulo `legacy/server/Codigo/modHexaStrings.bas` a C++ en `src/server/modHexaStrings.hpp` y `src/server/modHexaStrings.cpp`.

El módulo proporciona utilidades para la conversión entre representación hexadecimal y cadenas de bytes ASCII, utilizadas principalmente en la verificación de hash MD5 de ejecutables cliente en la autenticación del servidor, así como un cifrado por desplazamiento de caracteres (offset) de tabla ASCII.

## Decisiones de Diseño y Quirks de VB6

### 1. Ausencia de Salida Hexadecimal en las Funciones
A diferencia de librerías modernas de conversión hexadecimal, las funciones en `modHexaStrings.bas` **NO generan salida en formato texto hexadecimal**. Sus funciones procesan entradas hexadecimales para producir valores numéricos o secuencias de bytes ASCII crudos (`std::string`).

### 2. Conversión Insensible a Mayúsculas/Minúsculas (Case-Insensitive)
La función `hexHex2Dec` (y por extensión `hexMd52Asc`) evalúa los dígitos hexadecimales ('0'-'9', 'a'-'f', 'A'-'F') de forma insensible a mayúsculas/minúsculas. Ambas representaciones `"4a"` y `"4A"` producen exactamente el mismo valor decimal (`74`) y el mismo carácter ASCII (`'J'`).

> [!IMPORTANT]
> ### Requisito Rígido de Autenticación (`Admin.bas`)
> En `Admin.bas` (`MD5sCarga`), la lista de hashes habilitados `MD5AceptadoX` leída desde `Server.ini` se procesa mediante `txtOffset(hexMd52Asc(...), 55)` y se guarda en el arreglo `MD5s()`.
> Luego, `MD5ok` realiza una comparación exacta de cadenas de 16 bytes (`If md5formateado = MD5s(i)`) contra el buffer recibido del cliente por red.
> Dado que la comparación final es case-sensitive sobre los bytes resultantes, la conversión de `hexMd52Asc` DEBE ser case-insensitive para que hashes definidos en `Server.ini` en minúsculas o mayúsculas produzcan la misma secuencia de bytes que el cliente.

### 3. Relleno de Ceros a la Izquierda (Zero-Padding) para Longitud Impar
En `hexMd52Asc`, si la cadena de entrada `MD5` posee una longitud impar (`Len(MD5) And &H1`), VB6 le antepone automáticamente un carácter `"0"` al comienzo antes de agrupar en pares de dos dígitos hex.
- Ejemplo: `"A"` pasa a ser `"0A"` (evaluado como `10` / `'\n'`).
- Ejemplo: `"123"` pasa a ser `"0123"` (evaluado como `0x01` y `0x23`).

### 4. Tolerancia a Caracteres No-Hexadecimales y Espacios (`Val("&H" & hex)`)
`hexHex2Dec` emula la función `Val("&H" & hex)` de VB6:
- Omite espacios en blanco presentes en la cadena.
- Escanea dígitos hex de izquierda a derecha.
- **Detiene el parseo inmediatamente** al encontrar el primer carácter que no sea un dígito hexadecimal válido o espacio, retornando el valor acumulado hasta ese punto.
- Si la cadena comienza con un carácter inválido o está vacía, retorna `0` sin lanzar excepciones.

### 5. Desplazamiento de Texto (`txtOffset`)
`txtOffset` aplica una suma del argumento `off` sobre cada byte de la cadena de entrada `Text`, truncando a 8 bits (`& 0xFF`). No realiza transformaciones multi-byte ni fallos en caracteres fuera de rango imprimible ASCII.

## Funciones Migradas

### Header: `src/server/modHexaStrings.hpp`
### Source: `src/server/modHexaStrings.cpp`

```cpp
std::string hexMd52Asc(const std::string& MD5);
std::int32_t hexHex2Dec(const std::string& hex);
std::string txtOffset(const std::string& Text, std::int16_t off);
```

## Pruebas Unitarias (`doctest`)

La suite `tests/test_modhexastrings.cpp` valida minuciosamente los quirks y comportamientos límite identificados:

1. **Insensibilidad a Mayúsculas/Minúsculas**: Valida que `"4a"` y `"4A"` devuelvan `74`, y que `"4a4b"` y `"4A4B"` produzcan la misma cadena `"JK"`.
2. **Zero-Padding de Entrada Impar**: Valida que `"A"` se interprete como `"0A"` (`"\n"`) y `"123"` como `"0123"`.
3. **Casos Límite y Errores**:
   - Cadenas vacías (`""`) retornan `""` o `0`.
   - Cadenas con no-hex (`"G1"` -> `0`, `"4G"` -> `4`) verifican la detención limpia sin excepciones.
4. **Desplazamiento ASCII (`txtOffset`)**: Valida el offset de 55 utilizado por `Admin.bas` (`'A'` + 55 = `'x'`), así como el desbordamiento circular de byte a 256 (`\xFF` + 1 -> `\0`).
