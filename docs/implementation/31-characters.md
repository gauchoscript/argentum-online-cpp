---
area: implementacion
modulo: Characters
capa: 9
estado: completado-autonomo
last_updated: 2026-09-18
---

# Módulo Characters (Capa 9) — Mapeo Bidireccional de Personajes

## 1. Resumen Ejecutivo
- **Archivo Legacy**: `legacy/server/Codigo/Characters.bas` (54 líneas).
- **Archivos C++**: `src/server/Characters.hpp` y `src/server/Characters.cpp`.
- **Estado**: **`✅ COMPLETADO (Autónomo)`**.
- **Clasificación por Tamaño**: **Chico** (< 500 líneas). Exento de desglose formal en fases (`-breakdown.md`).
- **Pruebas Unitarias**: `tests/test_characters.cpp` (suite doctest con cobertura del 100% de escenarios).

---

## 2. Descripción Funcional y Declaraciones Exportadas

El módulo `Characters` provee la constante `INVALID_INDEX` y la función utilitaria `CharIndexToUserIndex`, cuya responsabilidad es mapear de forma bidireccional y segura el `CharIndex` (representación visual del personaje en el mundo/mapa) hacia el `UserIndex` en `UserList`.

### Interfaz C++ (`src/server/Characters.hpp`)

```cpp
#pragma once

#include <cstdint>

constexpr std::int16_t INVALID_INDEX = 0;

/**
 * @brief Obtiene el UserIndex correspondiente a un CharIndex del mapa.
 *
 * @param char_index El índice de personaje en el mapa.
 * @return std::int16_t El UserIndex asignado en UserList, o INVALID_INDEX (0) si no es un usuario o el índice es inválido.
 */
std::int16_t CharIndexToUserIndex(std::int16_t char_index) noexcept;
```

---

## 3. Algoritmo de Mapeo y Validaciones

La función `CharIndexToUserIndex` aplica una secuencia estricta de validaciones en 3 pasos:

1. **Rango de CharIndex**: Verifica que $1 \le \text{char\_index} \le \text{MAXCHARS}$. Si está fuera de rango, retorna `INVALID_INDEX`.
2. **Rango de UserIndex**: Obtiene `user_index = CharList[char_index]`. Verifica que $1 \le \text{user\_index} \le \text{MaxUsers}$. Si está fuera de rango (ej. ranura vacía o NPC), retorna `INVALID_INDEX`.
3. **Correspondencia Bidireccional**: Comprueba que `UserList[user_index].Char.CharIndex == char_index`. Si el valor difiere (ej. slot desactualizado o desincronizado), retorna `INVALID_INDEX`.

---

## 4. Estrategia de Verificación y Cobertura de Pruebas

Las pruebas unitarias implementadas en `tests/test_characters.cpp` mediante `doctest` certifiquen:

- **Casos fuera de rango para `CharIndex`**: `0`, `-1`, `-32768`, y `MAXCHARS + 1`.
- **Casos de `CharList` inválidos**: `CharList[i] = 0`, `CharList[i] < 0`, o `CharList[i] > MaxUsers`.
- **Desincronización y NPCs**: Slots donde `UserList[user_index].Char.CharIndex` es `0` o no coincide con `char_index`.
- **Caso Exitoso**: Mapeos válidos con correspondencia bidireccional exacta.
