---
area: implementacion
status: completed
module: Matematicas
layer: 0
legacy_source: legacy/server/Codigo/Matematicas.bas
target_header: src/server/Matematicas.hpp
target_source: src/server/Matematicas.cpp
test_suite: tests/test_matematicas.cpp
last_updated: 2026-09-07
---

# Módulo #1: Matematicas

## Resumen

Se completó el puerto transliterado 1:1 del módulo `legacy/server/Codigo/Matematicas.bas` a C++ en `src/server/Matematicas.hpp` y `src/server/Matematicas.cpp`.

La migración preservó rigurosamente la firma, nombres de parámetros y algoritmos internos sin refactorizaciones ni optimizaciones.

## Decisiones de Diseño

### Diferencia entre `Distancia` y `Distance`

A partir de la investigación del código fuente original de VB6, se confirmó que `Distancia` y `Distance` son dos funciones genuinamente distintas que responden a necesidades diferentes dentro del juego:

- **`Distancia(wp1, wp2)`**: Recibe dos estructuras `WorldPos` y calcula la **distancia Manhattan ponderada por mapa** en la grilla:
  `|wp1.X - wp2.X| + |wp1.Y - wp2.Y| + (|wp1.Map - wp2.Map| * 100)`
  Se utiliza principalmente para la comprobación de proximidad y visibilidad entre posiciones del mundo en la grilla del juego.

- **`Distance(X1, Y1, X2, Y2)`**: Recibe coordenadas de grilla individuales `(X, Y)` y calcula la **distancia euclidiana geométrica 2D**:
  `sqrt((Y1 - Y2)^2 + (X1 - X2)^2)`
  Se utiliza para cálculos continuos de rango, radio de hechizos o trayectorias euclidianas.

Ambas funciones coexisten en el port porque sustituirlas por una sola o unificar sus comportamientos alteraría la lógica interna del servidor.

### Comportamiento Confirmado de `RandomNumber`

La función `RandomNumber(LowerBound, UpperBound)` preserva exactamente la fórmula del legacy VB6:
`Fix(Rnd * (UpperBound - LowerBound + 1)) + LowerBound`

Durante la verificación con pruebas unitarias se ratificaron dos aspectos clave:
1. **Rango Inclusivo**: El rango devuelto es estrictamente cerrado e inclusivo en ambos extremos `[LowerBound, UpperBound]`.
2. **Generación con Limite Único**: Cuando `LowerBound == UpperBound`, devuelve exactamente ese valor sin divisiones por cero ni errores de rango.

Una reescritura ilusoria en C++ (usando el operador `%` sobre enteros o variaciones con `std::uniform_int_distribution` sin considerar los límites exactos de truncamiento con `Fix(Rnd * ...)`) podría haber provocado sesgos de distribución o fallos en los extremos inclusivos.

### Poda de la Suite de Pruebas

Siguiendo la [Testing Philosophy](docs/CONVENTIONS.md#testing-philosophy) detallada en `docs/CONVENTIONS.md`, no se escribieron pruebas unitarias para funciones aritméticas triviales de una sola línea (como `Porcentaje`), dado que no presentaban riesgos reales de mala traducción. La suite de pruebas se centró exclusivamente en verificar los límites inclusivos y la distribución de `RandomNumber`.

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

## Pruebas Unitarias (`doctest`)

Siguiendo la política de testing orientada al valor (Escuela Clásica / Detroit), se mantuvo la suite de pruebas unitarias enfocada específicamente en `RandomNumber` (`tests/test_matematicas.cpp`), verificando el correcto comportamiento del truncamiento y acotamiento inclusivo del rango.

### Resultados de la Ejecución

```text
[doctest] doctest version is "2.5.3"
===============================================================================
[doctest] test cases:    1 |    1 passed | 0 failed | 0 skipped
[doctest] assertions: 2002 | 2002 passed | 0 failed |
[doctest] Status: SUCCESS!
```
