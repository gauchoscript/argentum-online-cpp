---
area: implementacion
status: completed
module: clsIniReader
layer: 0
legacy_source: legacy/server/Codigo/clsIniReader.cls
target_header: src/server/clsIniReader.hpp
target_source: src/server/clsIniReader.cpp
test_suite: tests/test_clsinireader.cpp
last_updated: 2026-09-07
---

# Módulo #3: clsIniReader

## Resumen

Se completó el puerto transliterado 1:1 del módulo `legacy/server/Codigo/clsIniReader.cls` a C++ en `src/server/clsIniReader.hpp` y `src/server/clsIniReader.cpp`.

La migración preservó estrictamente la estructura interna de datos, algoritmos de Quicksort y búsqueda binaria, firmas y comportamientos del lenguaje VB6 sin refactorizaciones ni optimizaciones.

## Decisiones de Diseño

### Codificación ANSI / Windows-1252

Se confirmó y preservó la suposición de que los archivos INI están guardados en codificación mono-byte ANSI (Windows-1252). La lectura de archivos mediante `std::ifstream` en modo binario procesa los bytes de 8 bits preservando exactamente la representación de caracteres del entorno Windows original.

### Búsqueda Insensible a Mayúsculas/Minúsculas (Case-Insensitive) con Preservación de Valores

Tanto los nombres de las secciones principales (`MainNode`) como las claves de cada valor (`ChildNode`) se convierten internamente a mayúsculas utilizando `UCase$` al momento del parseo e indexación en `Initialize`. Las búsquedas en `GetValue`, `ChangeValue` y `KeyExists` aplican `UCase$` sobre los argumentos de entrada.

Sin embargo, los valores (`Value`) asignados a las claves preservan su formato, espacios y mayúsculas/minúsculas originales tal como fueron leídos del archivo.

> [!WARNING]
> ### Quirk Prominente de `KeyExists`
> A pesar de su nombre (`KeyExists`), en el código legacy VB6 esta función **NO comprueba la existencia de una clave individual** dentro de una sección. Internamente ejecuta `FindMain(UCase$(name)) >= 0`, lo que significa que **comprueba únicamente si existe la SECCIÓN principal (`MainNode`)** dada por `[name]`.
> 
> Se destaca esta advertencia de manera prioritaria para evitar que otros módulos o desarrolladores utilicen por error `KeyExists` creyendo que valida la presencia de un par clave-valor individual.

### Tolerancia a Líneas Mal Formateadas

Durante la lectura paso a paso en `Initialize`, cualquier línea que no sea un encabezado de sección (no empiece con `[`) y que no posea un signo `=` a partir de la segunda posición (`InStr(2, Text, "=")` en VB6 / `Text.find('=', 1)` en C++) es ignorada silenciosamente sin lanzar errores ni alterar el estado de la sección activa actual.

## Estructuras y Clase Migrada

### `clsIniReader`

- **Header**: `src/server/clsIniReader.hpp`
- **Source**: `src/server/clsIniReader.cpp`

```cpp
struct ChildNode {
    std::string Key;
    std::string Value;
};

struct MainNode {
    std::string name;
    std::vector<ChildNode> values;
    std::int32_t numValues{0};
};
```

### Métodos Públicos
- `void Initialize(const std::string& file)`: Carga el archivo INI en memoria, parsea secciones y pares clave-valor, y ordena las secciones y claves usando Quicksort.
- `std::string GetValue(const std::string& Main, const std::string& Key)`: Búsqueda binaria en secciones y claves. Retorna `""` si no existe.
- `void ChangeValue(const std::string& Main, const std::string& Key, std::int32_t Value)` y `void ChangeValue(const std::string& Main, const std::string& Key, const std::string& Value)`: Modifica en memoria el valor de una clave existente.
- `bool KeyExists(const std::string& name)`: Retorna `true` si la sección `name` existe.

## Pruebas Unitarias (`doctest`)

Suite implementada en `tests/test_clsinireader.cpp` justificando cada `TEST_CASE` según la política de pruebas (Escuela Clásica / Detroit):

1. **Edge Case**: Verificación de retorno de cadena vacía (`""`) para claves o secciones inexistentes y comportamiento seguro ante archivos que no existen.
2. **VB6-Specific Quirk**: Cobertura del comportamiento case-insensitive, verificación del quirk de `KeyExists` (que busca la sección `MainNode`), preservación del casing en `Value` y actualización en memoria con `ChangeValue`.
3. **Translation Risk**: Verificación de lectura de caracteres ANSI/Windows-1252 (acentos y `ñ`) y resistencia ante líneas mal formateadas o comentarios sin `=`.
