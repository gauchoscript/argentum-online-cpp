# Plan de Desglose Modular — Módulo #27: `PathFinding.bas` (Capa 8)

> **Estado**: Planificación Aprobada — Pendiente de Ejecución  
> **Área**: Capa 8 (NPCs e Inteligencia Artificial)  
> **Documentación Relacionada**: [`docs/audit/08a-pathfinding-detalle.md`](../audit/08a-pathfinding-detalle.md), [`docs/implementation/00-port-plan.md`](00-port-plan.md), [`docs/CONVENTIONS.md`](../CONVENTIONS.md), [`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md)  
> **Archivos a Crear**: `src/server/PathFinding.hpp`, `src/server/PathFinding.cpp`, `tests/test_pathfinding.cpp`, `docs/implementation/27-pathfinding.md`

---

## 1. Resumen de Objetivos y Arquitectura

El presente documento establece el plan de implementación modular en **4 fases lógicas secuenciales** para la migración del módulo de búsqueda de caminos `legacy/server/Codigo/PathFinding.bas` (~300 líneas de VB6) y su cola auxiliar `legacy/server/Codigo/Queue.bas` a C++20 (`src/server/PathFinding.hpp` y `src/server/PathFinding.cpp`).

### Decisiones Arquitectónicas Vinculantes

1. **Encapsulamiento del Algoritmo BFS**:
   - `Queue.bas` se absorbe de forma transitoria y encapsulada utilizando primitivas estándar de C++ (`std::queue<tVertice>` o `std::vector<tVertice>`), eliminando dependencias de estructuras globales de cola.
   - `TmpArray` se mantiene en el módulo `PathFinding.cpp` como buffer de cálculo estático/reutilizable de `100 x 100` celdas (`tIntermidiateWork`) para preservar la política de cero alocaciones dinámicas por búsqueda del servidor original.

2. **Preservación Estricta de Firmas y Convención PascalCase**:
   - `void SeekPath(std::int16_t npc_index, std::int16_t max_steps = 30);`
   - `bool IsWalkable(std::int16_t map, std::int16_t row, std::int16_t col, std::int16_t npc_index);`
   - `bool Limites(std::int16_t vfila, std::int16_t vcolu);`
   - `void InitializeTable(tIntermidiateWork (&T)[101][101], const tVertice& S, std::int16_t max_steps = 30);`
   - `void ProcessAdjacents(std::int16_t map_index, tIntermidiateWork (&T)[101][101], std::int16_t vfila, std::int16_t vcolu, std::int16_t npc_index);`
   - `void MakePath(std::int16_t npc_index);`

3. **Replicación 1:1 de Bugs y Quirks Históricos**:
   - **Bug #43 (`steps` sin incrementar)**: Preservación de `steps` inicializado en 0 sin incrementarse dentro del bucle BFS de `SeekPath`.
   - **Bug #44 (Limpieza incompleta de `TmpArray`)**: Preservación del reseteo parcial de la matriz en `InitializeTable` restringido al cuadrante `[-MaxSteps, +MaxSteps]`.
   - **Bug #45 (Inversión de Coordenadas $X \leftrightarrow Y$)**: Preservación del intercambio de ejes entre `MapData` y la lógica interna de `PathFinding.bas`.

4. **Determinismo 1:1 en Exploración Cardinal**:
   - Respeto absoluto a la secuencia fija **Norte -> Sur -> Oeste -> Este** en `ProcessAdjacents`.

---

## 2. Plan de Implementación por Fases

---

### Fase 1: Estructuras Auxiliares y Validaciones de Transitabilidad

#### Componentes
- Definición de tipos de datos en `src/server/PathFinding.hpp`:
  - `struct tVertice { std::int16_t X = 0; std::int16_t Y = 0; };`
  - `struct tIntermidiateWork { bool Known = false; std::int16_t DistV = 1000; tVertice PrevV{}; };`
- Matriz estática en `src/server/PathFinding.cpp`:
  - `static tIntermidiateWork TmpArray[101][101];`
- Funciones auxiliares de límites y transitabilidad:
  - `bool Limites(std::int16_t vfila, std::int16_t vcolu);`
    - Retorna `vcolu >= 1 && vcolu <= 100 && vfila >= 1 && vfila <= 100`.
  - `bool IsWalkable(std::int16_t map, std::int16_t row, std::int16_t col, std::int16_t npc_index);`
    - Evalúa `MapData[map][row][col].Blocked == 0 && MapData[map][row][col].NpcIndex == 0`.
    - Si `UserIndex != 0`, verifica si `UserIndex == Npclist[npc_index].PFINFO.TargetUser`.

---

### Fase 2: Rutinas de Inicialización y Exploración BFS Determinista

#### Componentes
- `void InitializeTable(tIntermidiateWork (&T)[101][101], const tVertice& S, std::int16_t max_steps = 30);`
  - Limpia únicamente la sub-grilla `[S.Y - max_steps, S.Y + max_steps]` y `[S.X - max_steps, S.X + max_steps]` si satisface `InMapBounds`.
  - Asigna `DistV = 1000`, `Known = false`, `PrevV = {0,0}`.
  - Fija la celda origen `T[S.Y][S.X].DistV = 0`.
- `void ProcessAdjacents(std::int16_t map_index, tIntermidiateWork (&T)[101][101], std::int16_t vfila, std::int16_t vcolu, std::int16_t npc_index);`
  - Explora secuencialmente en orden estricto:
    1. **Norte**: `j = vfila - 1, vcolu`
    2. **Sur**: `j = vfila + 1, vcolu`
    3. **Oeste**: `vfila, vcolu - 1`
    4. **Este**: `vfila, vcolu + 1`
  - Para cada dirección válida y transitable con `T[j][k].DistV == 1000`, actualiza `DistV = DistV + 1`, asigna `PrevV = {vcolu, vfila}` y encola en la cola BFS local.

---

### Fase 3: Búsqueda Principal (`SeekPath`) y Reconstrucción de Camino (`MakePath`)

#### Componentes
- `void SeekPath(std::int16_t npc_index, std::int16_t max_steps = 30);`
  - Extrae `cur_npc_pos.X = Npclist[npc_index].Pos.Y` y `cur_npc_pos.Y = Npclist[npc_index].Pos.X` (inversión de ejes Bug #45).
  - Extrae `tar_npc_pos.X = Npclist[npc_index].PFINFO.Target.X` y `tar_npc_pos.Y = Npclist[npc_index].PFINFO.Target.Y`.
  - Llama a `InitializeTable(TmpArray, cur_npc_pos)`.
  - Inicializa la cola local `std::queue<tVertice> queue;` e inserta `cur_npc_pos`.
  - Ejecuta el bucle `while (!queue.empty())`:
    - Evalúa `if (steps > max_steps) break;` (con `steps` fijo en 0, Bug #43).
    - Desencola `V = queue.front(); queue.pop();`.
    - Si `V.X == tar_npc_pos.X && V.Y == tar_npc_pos.Y`, interrumpe con `break`.
    - Llama a `ProcessAdjacents(map, TmpArray, V.Y, V.X, npc_index)`.
  - Invoca `MakePath(npc_index)`.
- `void MakePath(std::int16_t npc_index);`
  - Lee `Pasos = TmpArray[Target.Y][Target.X].DistV`.
  - Si `Pasos == 1000`, fija `NoPath = true`, `PathLenght = 0` y finaliza.
  - De lo contrario, redimensiona el vector `Path` a `Pasos + 1`.
  - Reconstruye hacia atrás desde `Pasos` hasta 1 leyendo `PrevV`.
  - Fija `CurPos = 1` y `NoPath = false`.

---

### Fase 4: Suite de Pruebas Unitarias en doctest (`tests/test_pathfinding.cpp`)

#### Cobertura Requerida
1. **Paridad BFS Básica**:
   - Cálculo de ruta directa en línea recta libre de obstáculos.
   - Verificación de longitud exacta de pasos (`PathLenght`) y coordenadas del camino devuelto.
2. **Sorteo de Obstáculos**:
   - Evasión de bloqueos estáticos (`Blocked = 1`) y otros NPCs (`NpcIndex != 0`).
   - Verificación de transitabilidad sobre la posición del `TargetUser`.
3. **Orden Cardinal Determinista (Desempeño N-S-O-E)**:
   - Caso con dos rutas simétricas idénticas. Confirmación de que el algoritmo elige la rama Norte sobre el resto de las direcciones.
4. **Pruebas de Bugs Replicados**:
   - **Bug #43**: Confirmar que la búsqueda procesa más de `MaxSteps` iteraciones en la cola debido a `steps` no incrementado.
   - **Bug #44**: Validar el efecto colateral de celdas no reseteadas fuera de la sub-grilla `MaxSteps`.
   - **Bug #45**: Validar el mapeo inverso de coordenadas $X \leftrightarrow Y$ frente a `MapData`.

---

## 3. Plan de Verificación

```bash
# Compilación de la suite de pruebas unitarias
cmake --build build --config Debug --target test_pathfinding

# Ejecución de tests de PathFinding
./build/bin/Debug/test_pathfinding.exe
```
