# Módulo #27: `PathFinding.bas` y `Queue.bas` (Capa 8)

> **Estado**: Completado — Autónomo  
> **Área**: Capa 8 (NPCs e Inteligencia Artificial)  
> **Documentación Relacionada**: [`docs/audit/08a-pathfinding-detalle.md`](../audit/08a-pathfinding-detalle.md), [`27-pathfinding-breakdown.md`](27-pathfinding-breakdown.md), [`00-port-plan.md`](00-port-plan.md), [`KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md)  
> **Archivos C++**: `src/server/PathFinding.hpp`, `src/server/PathFinding.cpp`, `tests/test_pathfinding.cpp`

---

## 1. Resumen y Alcance

El módulo `PathFinding.bas` y su cola auxiliar `Queue.bas` constituyen el motor de navegación espacial por búsqueda en anchura (BFS) utilizado por la Inteligencia Artificial del servidor para trazar rutas óptimas entre NPCs y jugadores objetivos.

La portación a C++20 reestructura el subsistema encapsulando la cola FIFO de `Queue.bas` dentro de las rutinas de búsqueda (`std::queue<tVertice>`), eliminando dependencias de estado global de cola y preservando el buffer estático `TmpArray` para evitar alocaciones de memoria dinámicas por tick en tiempo de ejecución.

---

## 2. Decisiones de Diseño y Arquitectura

1. **Absorción de `Queue.bas`**:
   - La cola FIFO global de VB6 fue sustituida por una instancia local `std::queue<tVertice>` en el cuerpo de `SeekPath`. No requiere mantención de estado ni limpieza entre llamadas.

2. **Buffer Estático `TmpArray` (Base 1)**:
   - Se mantiene la matriz estática de módulo `static tIntermidiateWork TmpArray[101][101];` en `src/server/PathFinding.cpp` para conservar la paridad de memoria y la política de cero alocaciones durante la ejecución del servidor.

3. **Determinismo Espacial (N-S-O-E)**:
   - La rutina `ProcessAdjacents` evalúa los 4 casilleros adyacentes cardinales respetando de forma estricta la secuencia determinista:
     1. **Norte**: `(vfila - 1, vcolu)`
     2. **Sur**: `(vfila + 1, vcolu)`
     3. **Oeste**: `(vfila, vcolu - 1)`
     4. **Este**: `(vfila, vcolu + 1)`

---

## 3. Bugs Históricos Replicados (1:1)

- **Bug #43 — Bucle BFS con `steps` Fijo en 0**:
  En `SeekPath`, la variable local `steps` se inicializa en `0` y nunca se incrementa dentro del bucle `while (!queue.empty())`. Por lo tanto, la condición `if (steps > max_steps)` permanece inoperante y la búsqueda no finaliza por cota de pasos. Replicado en `src/server/PathFinding.cpp:177` y verificado en `tests/test_pathfinding.cpp:281`.

- **Bug #44 — Limpieza Parcial de `TmpArray`**:
  `InitializeTable` limita la inicialización del buffer únicamente a la ventana `[-max_steps, +max_steps]` alrededor de la posición inicial. Celdas fuera de ese radio conservan valores basuras de búsquedas anteriores. Replicado en `src/server/PathFinding.cpp:52` y verificado en `tests/test_pathfinding.cpp:115`.

- **Bug #45 — Inversión Histórica de Coordenadas $X \leftrightarrow Y$**:
  Debido a la discrepancia entre la convención de `PathFinding.bas` (`Fila, Columna`) y ORE/AO (`X, Y`), `SeekPath` invierte las coordenadas del NPC al ingresar (`cur_npc_pos.X = Pos.Y`, `cur_npc_pos.Y = Pos.X`) y `MakePath` realiza la desinversión correspondiente al escribir `PFINFO.Path`. Replicado en `src/server/PathFinding.cpp:144, 169` y verificado en `tests/test_pathfinding.cpp:202`.

---

## 4. Métricas de Cobertura y Pruebas Unitarias

La suite de pruebas unitarias `tests/test_pathfinding.cpp` cubre 12 escenarios de doctest organizados en 3 suites principales:
1. **Fase 1: Validaciones y Tránsito**: Pruebas de límites de grilla (`Limites`) y transitabilidad de `MapData` (`IsWalkable`), verificando celdas libres, bloqueos estáticos, NPCs y usuarios.
2. **Fase 2: Inicialización y Adyacencias**: Prueba de reseteo de tabla, replicación exacta del Bug #44, secuencia determinista N-S-O-E y filtrado de obstáculos en `ProcessAdjacents`.
3. **Fase 3: Búsqueda y Reconstrucción**: Búsqueda en línea recta, evasión de obstáculos estáticos, objetivos inalcanzables, y replicación de los Bugs #43 y #45 en `SeekPath` y `MakePath`.

---

## 5. Contrato de Consumo

Punto de entrada canónico del módulo para su interacción con `AI_NPC.bas` (Módulo #29):

```cpp
// src/server/PathFinding.hpp
void SeekPath(std::int16_t npc_index, std::int16_t max_steps = 30);
```

- **Entrada**: `npc_index` (índice del NPC en `Npclist`), `max_steps` (cota opcional de pasos, por defecto 30).
- **Salida**: Actualización directa de la estructura `Npclist[npc_index].PFINFO`:
  - `PFINFO.Path`: Vector de vértices `tVertice` con coordenadas $(X, Y)$ del mundo.
  - `PFINFO.PathLenght`: Cantidad total de pasos de la ruta.
  - `PFINFO.CurPos`: Reseteado a 1.
  - `PFINFO.NoPath`: `false` si se halló camino, `true` si es inalcanzable.
