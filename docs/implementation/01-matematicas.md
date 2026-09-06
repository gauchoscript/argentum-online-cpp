---
area: implementacion
status: completed
module: Matematicas
layer: 0
legacy_source: legacy/server/Codigo/Matematicas.bas
target_header: src/server/Matematicas.hpp
target_source: src/server/Matematicas.cpp
last_updated: 2026-09-07
---

# Módulo #1: Matematicas

## Resumen

Se completó el puerto transliterado 1:1 del módulo `legacy/server/Codigo/Matematicas.bas` a C++ en `src/server/Matematicas.hpp` y `src/server/Matematicas.cpp`.

La migración preservó rigurosamente la firma, nombres de parámetros y algoritmos internos sin refactorizaciones ni optimizaciones.

## Estructuras y Funciones Migradas

### `WorldPos` (Estructura Base)
- **Definición**: Se definió en `Matematicas.hpp` para permitir la independencia de compilación de la Capa 0:
  ```cpp
  struct WorldPos {
      std::int16_t Map{0};
      std::int16_t X{0};
      std::int16_t Y{0};
  };
  ```

### 1. `Porcentaje`
- **Firma**: `std::int32_t Porcentaje(std::int32_t Total, std::int32_t Porc)`
- **Lógica**: División entera `(Total * Porc) / 100`.

### 2. `Distancia`
- **Firma**: `std::int32_t Distancia(const WorldPos& wp1, const WorldPos& wp2)`
- **Lógica**: Distancia Manhattan en grilla `|wp1.X - wp2.X| + |wp1.Y - wp2.Y| + (|wp1.Map - wp2.Map| * 100)`.

### 3. `Distance`
- **Firma**: `double Distance(std::int16_t X1, std::int16_t Y1, std::int16_t X2, std::int16_t Y2)`
- **Lógica**: Distancia euclidiana geométrica 2D `sqrt((Y1 - Y2)^2 + (X1 - X2)^2)`.

### 4. `RandomNumber`
- **Firma**: `std::int32_t RandomNumber(std::int32_t LowerBound, std::int32_t UpperBound)`
- **Lógica**: Preserva exactamente la fórmula de VB6: `Fix(Rnd * (UpperBound - LowerBound + 1)) + LowerBound`.

## Pruebas Unitarias

Siguiendo la política de pruebas orientadas al valor (Escuela de Detroit / Estilo Clásico) documentada en [`docs/CONVENTIONS.md`](../CONVENTIONS.md), no se mantienen pruebas unitarias artificiales para funciones aritméticas puras de 1 línea. El esfuerzo de testing se reservará para los módulos críticos de red (`clsByteQueue`), persistencia (`FileIO`) y mecánicas complejas con estado del juego.
