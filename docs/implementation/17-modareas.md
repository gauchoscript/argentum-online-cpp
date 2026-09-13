---
area: sistema-de-areas
status: completed
module: ModAreas
layer: 5
legacy_source: legacy/server/Codigo/ModAreas.bas
target_header: src/server/ModAreas.hpp
target_source: src/server/ModAreas.cpp
test_suite: tests/test_modareas.cpp
last_updated: 2026-09-13
---

# Módulo #17: ModAreas — Particionamiento Espacial y Ciclo de Visibilidad

## Resumen del Módulo

Este documento formaliza la arquitectura, decisiones de diseño de bajo nivel y suite de verificación del módulo de Capa 5 **`ModAreas`** ([`src/server/ModAreas.hpp`](../../src/server/ModAreas.hpp) y [`src/server/ModAreas.cpp`](../../src/server/ModAreas.cpp)), transliterado a partir de [`legacy/server/Codigo/ModAreas.bas`](../../legacy/server/Codigo/ModAreas.bas) (~500 líneas en Visual Basic 6.0).

`ModAreas` es el núcleo de particionamiento espacial de Argentum Online v0.13.0. Su misión fundamental es mitigar la complejidad de red de un entorno multijugador masivo en 2D:
1. Divide el plano de cada mapa (100x100 tiles) en franjas cardinales de 9 tiles (12 franjas horizontales y 12 verticales).
2. Determina el campo de visión de 9 cuadrantes (27x27 tiles) que rodea a cada entidad.
3. Notifica selectivamente la aparición y actualización de jugadores, criaturas y objetos cuando cruzan fronteras de área.
4. Mantiene los grupos de conexiones de usuarios activos por mapa (`ConnGroups`) consumidos por el sistema de difusión multicasting [`modSendData`](15-modsenddata.md).

### Estado de Cierre Formal: `Completado (Aislado / Cableado Pendiente)`

Conforme a la taxonomía definida en [`docs/CONVENTIONS.md`](../CONVENTIONS.md#definición-formal-de-los-tres-estados-de-cierre-de-módulo-module-closure-states), este módulo se encuentra en estado **`Completado (Aislado / Cableado Pendiente)`**:
- **Código C++ Cerrado**: La transliteración de [`ModAreas.hpp`](../../src/server/ModAreas.hpp) y [`ModAreas.cpp`](../../src/server/ModAreas.cpp) está 100% finalizada y testeada (25 subcasos doctest). No requiere ediciones internas futuras.
- **Cableado Pendiente en Capas Superiores**: Su integración productiva en el bucle de juego depende de conectar los hooks de desacoplamiento (`SetMakeUserCharHook`, `SetMakeNPCCharHook` y `SetBloquearHook`) a sus implementaciones definitivas en `Modulo_UsUaRiOs` (Capa 9) y `MODULO_NPCs` (Capa 8).

---

## Decisiones Estratégicas de Arquitectura y Paridad en C++20

### 1. Replicación Estricta de la Fórmula de `AreaID` (Bug #24)

- **Diagnóstico en VB6**: En `ModAreas.bas:84`, la matriz precalculada `AreasInfo` computa el identificador de cuadrante mediante una multiplicación escalar no inyectiva:
  `AreaID(X, Y) = ((X \ 9) + 1) * ((Y \ 9) + 1)`
  Esta operación genera colisiones numéricas masivas (por ejemplo, pares de franjas (1, 5), (2, 3), (3, 2) y (5, 1) producen todos AreaID = 12).
- **Decisión de Porting**: En observancia estricta de la Regla #4 de [`docs/CONVENTIONS.md`](../CONVENTIONS.md), **la fórmula original fue replicada de forma literal**:
  ```cpp
  s_areas_info[x][y] = static_cast<std::uint8_t>((x / 9 + 1) * (y / 9 + 1));
  ```
  No se modificó por una función inyectiva (como Y * 12 + X) para preservar el comportamiento idéntico del control de salida temprana ante teletransportaciones o movimientos espaciales. Ver entrada del Master Bug Ledger: [`KNOWN-LEGACY-BUGS.md` (Entrada #24)](KNOWN-LEGACY-BUGS.md#entrada-24--modareas-colisión-de-areaid-por-producto-no-inyectivo).

### 2. Aritmética de Viewport y Persistencia de Coordenadas Negativas (Bug #25)

- **Diagnóstico en VB6**: Al conectarse o cambiar de mapa (`head = USER_NUEVO`), cuando X < 9 o Y < 9, la fórmula `MinX = ((Pos.X \ 9) - 1) * 9` produce `-9`. En VB6, este valor negativo se asigna a la estructura del usuario (`AreasInfo.MinX = CInt(MinX)`) antes de que las variables locales del bucle de barrido se acoten a 1. En desplazamientos subsiguientes hacia el Este (`EAST`), el motor toma este `-9` base y le suma 27 para obtener la franja emergente [18, 26].
- **Decisión de Porting**: Los campos `MinX` y `MinY` en `AreaInfo` ([`src/server/Declares.hpp`](../../src/server/Declares.hpp)) están estrictamente tipados como enteros con signo (`std::int16_t`). Se preserva el almacenamiento de `-9` y la aritmética de offsets original sin alterar los supuestos espaciales asumidos por los movimientos cardinales subsecuentes. Ver entrada del Master Bug Ledger: [`KNOWN-LEGACY-BUGS.md` (Entrada #25)](KNOWN-LEGACY-BUGS.md#entrada-25--modareas-persistencia-de-coordenadas-negativas-en-minx--miny).

### 3. Membresía Contigua en `ConnGroups` y Orden Determinista de Red

- **Diagnóstico en VB6**: `ConnGroups(Map).UserEntrys` almacena secuencialmente los índices de los usuarios presentes en cada mapa. Al ingresar un jugador (`AgregarUser`), se realiza una búsqueda lineal O(N) para prevenir duplicados. Al salir (`QuitarUser`), se desplazan todos los elementos posteriores un lugar hacia la izquierda mediante un bucle `For`, compactando el arreglo.
- **Implementación en C++20**:
  - `UserEntrys` utiliza un vector dinámico plano `std::vector<std::int16_t>` con base 1 (posición 0 reservada con centinela `0`) para garantizar compatibilidad con los bucles `for (int i = 1; i <= cg.CountEntrys; ++i)` de [`modSendData.cpp`](../../src/server/modSendData.cpp).
  - La baja ejecuta `group.UserEntrys.erase(group.UserEntrys.begin() + found_idx)`. Esto preserva el desplazamiento contiguo a la izquierda y el orden determinista exacto de inserción de los clientes (quedando estrictamente prohibido el uso de técnicas como *swap-and-pop* o `std::unordered_set`, conforme a la Regla #4 de convenciones).

### 4. Asimetría de Red: `AreaChanged` vs. `CharacterRemove`

- En el motor de Argentum Online v0.13.0, cuando un usuario cruza la frontera de un área hacia un nuevo cuadrante:
  - El servidor despacha `WriteAreaChanged(user_index)`.
  - El servidor **NUNCA emite paquetes de remoción `CharacterRemove` (BP)** a los clientes que quedaron fuera de visión.
  - Es responsabilidad del motor gráfico del cliente VB6 purgar de su memoria los personajes e ítems cuyas coordenadas caigan fuera del nuevo radio de visión recibido tras el aviso de `AreaChanged`.
  - Se confirmó y preservó esta asimetría tanto en la lógica de `CheckUpdateNeededUser` como en las pruebas unitarias.

### 5. Asimetría de Búfer: Jugadores vs. Criaturas

- En `CheckUpdateNeededUser`, al notificar a otro cliente presente en la franja emergente, el servidor invoca de inmediato `TCP::FlushBuffer(other_user)` para forzar la transmisión inmediata de los datos acumulados.
- En `CheckUpdateNeededNpc`, el barrido de criaturas ante usuarios (`MapInfoList[map].NumUsers != 0`) emite `MakeNPCChar` pero **NO invoca `FlushBuffer`**, dejando que los bytes se acumulen para la siguiente ráfaga de red. Esta asimetría histórica fue replicada al 100%.

### 6. Exclusiones por Código Muerto e I/O Obsoleta (Bug #26)

- **`PosToArea`**: Matriz de 100 bytes que en VB6 se inicializaba en `InitAreas` pero jamás se consultaba en ningún archivo del servidor ni del cliente. Excluida formalmente por código muerto.
- **`AreasStats.dat` / `AreasOptimizacion`**: Módulo que grababa y leía un archivo INI en disco para computar el promedio de usuarios por franja horaria a fin de optimizar el tamaño de `ReDim Preserve`. Excluido bajo la *Única Excepción* de [`docs/CONVENTIONS.md`](../CONVENTIONS.md) al tratarse de una optimización arcaica de gestión de memoria basada en I/O síncrona a disco. Ver entrada del Master Bug Ledger: [`KNOWN-LEGACY-BUGS.md` (Entrada #26)](KNOWN-LEGACY-BUGS.md#entrada-26--modareas-auto-optimización-obsoleta-areasstatsdat-y-arreglo-ocioso-postoarea).

### 7. Desacoplamiento de Capas Posteriores Mediante Inyección de Hooks

Para evitar el acoplamiento prematuro con subsistemas aún no portados de las Capas 8 y 9 (`Modulo_UsUaRiOs.bas`, `MODULO_NPCs.bas` y `General.bas`), se implementaron hooks funcionales en `ModAreas`:
- `SetMakeUserCharHook(MakeUserCharHook)`
- `SetMakeNPCCharHook(MakeNPCCharHook)`
- `SetBloquearHook(BloquearHook)`

Estos hooks permiten la verificación unitaria total de los despachos hacia observadores sin requerir la implementación completa de las criaturas y usuarios de alto nivel.

---

## Interfaz Pública y Prototipos Canónicos

```cpp
namespace ModAreas {

inline constexpr std::uint8_t USER_NUEVO = 255;

using AreaInfo = ::AreaInfo;
using ConnGroup = ::ConnGroup;

// Inicialización de tablas precalculadas y dimensionamiento de mapas
void InitAreas();

// Consultas de matrices precalculadas
[[nodiscard]] std::uint8_t GetAreaID(std::int16_t x, std::int16_t y) noexcept;
[[nodiscard]] std::int16_t GetAreaRecive(std::int16_t franja) noexcept;

// Ciclo de visibilidad y presencia
void CheckUpdateNeededUser(std::int16_t user_index, std::uint8_t head, bool but_index = false);
void CheckUpdateNeededNpc(std::int16_t npc_index, std::uint8_t head);

// Gestión de membresía en mapas
void AgregarUser(std::int16_t user_index, std::int16_t map, bool but_index = false);
void QuitarUser(std::int16_t user_index, std::int16_t map);
void AgregarNpc(std::int16_t npc_index);

// Hooks de desacoplamiento para pruebas e integración
void SetMakeUserCharHook(MakeUserCharHook hook) noexcept;
void SetMakeNPCCharHook(MakeNPCCharHook hook) noexcept;
void SetBloquearHook(BloquearHook hook) noexcept;

} // namespace ModAreas
```

---

## Cobertura de Pruebas Unitarias (`tests/test_modareas.cpp`)

La suite de pruebas en [`tests/test_modareas.cpp`](../../tests/test_modareas.cpp) consta de 4 grandes casos de prueba con subcasos exhaustivos:

1. **Fase 1: Tablas Precalculadas y Estructuras Base**:
   - Validación de las 12 máscaras de franjas adyacentes de 3 bits en `AreasRecive`.
   - Validación de `AreasInfo` en esquinas y fronteras [1, 100].
   - Verificación de la preservación literal de las colisiones matemáticas de `AreaID` (Bug #24).
   - Verificación de dimensionamiento limpio de `ConnGroups` con elemento base 1-based.
   - Retención de enteros con signo en `AreaInfo.MinX` y `MinY` (Bug #25).
2. **Fase 2: Gestión de Membresía de Mapas**:
   - Inserción secuencial contigua en `ConnGroups`.
   - Prevención lineal de usuarios duplicados (idempotencia).
   - Remoción intermedia con corrimiento estricto a la izquierda preservando el orden relativo.
   - Remoción en extremos (primer y último usuario) y vaciado completo.
   - Remoción idempotente de usuarios inexistentes y validación de cotas de mapas.
   - Blanqueo integral de `UserList[u].AreasInfo` al enrolar.
3. **Fase 3: Ciclo de Visibilidad de Jugadores**:
   - No-op: salida temprana si `AreaID` coincide con el cuadrante actual.
   - Login con `USER_NUEVO` en esquina extrema (5, 5): persistencia de `-9` en `MinX`/`MinY`, acotamiento del barrido a [1, 17] x [1, 17] y emisión de `AreaChanged` y `MakeUserChar` propio.
   - Desplazamiento cardinal `EAST`: verificación de barrido en franja emergente [18, 26] y actualización de `MinX` a 0.
   - Interacción recíproca entre jugadores y vaciado de cola `FlushBuffer`.
   - Detección de criaturas (`MakeNPCChar`), objetos (`WriteObjectCreate`) y bloqueo de puertas en (X, Y) y (X-1, Y).
   - Preservación de asimetría legacy: constatar que nunca se emite `CharacterRemove`.
4. **Fase 4: Ciclo de Visibilidad de Criaturas**:
   - `AgregarNpc`: reseteo de campos espaciales y ejecución con `USER_NUEVO`.
   - Movimiento de criatura en mapa vacío (`NumUsers == 0`): omisión de barrido pero actualización correcta de bitmasks y `AreaID`.
   - Movimiento de criatura con observadores (`NumUsers > 0`): emisión de `MakeNPCChar` hacia el jugador destinatario.
   - Salida temprana en criatura por coincidencia de `AreaID`.
   - Verificación de ausencia de `FlushBuffer` en el flujo de NPCs.
