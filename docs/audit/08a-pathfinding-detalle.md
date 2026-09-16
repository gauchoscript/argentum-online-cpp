# Auditoría Técnica: Búsqueda de Caminos (`PathFinding.bas` y `Queue.bas`)

## Resumen Ejecutivo

Este documento contiene la auditoría técnica exhaustiva del módulo de búsqueda de caminos del servidor legacy de Argentum Online (`legacy/server/Codigo/PathFinding.bas`) y su estructura de datos auxiliar de cola FIFO (`legacy/server/Codigo/Queue.bas`).

La investigación cubre el catálogo completo de procedimientos, la estructura de datos BFS, la secuencia determinista de exploración direccional, las condiciones de transitabilidad sobre el mapa, el ciclo de vida de la cola y la identificación de quirks y desfasajes históricos.

---

## 1. Catálogo de Procedimientos y Firmas

El módulo `legacy/server/Codigo/PathFinding.bas` cuenta con un total de 6 rutinas (1 pública y 5 privadas), apoyadas por tipos de datos y constantes de módulo.

### Tipos de Datos y Variables de Módulo (`legacy/server/Codigo/PathFinding.bas`, líneas 24-36)

- `ROWS As Integer = 100` (línea 24): Constante de filas máximas de la grilla.
- `COLUMS As Integer = 100` (línea 25): Constante de columnas máximas de la grilla.
- `MAXINT As Integer = 1000` (línea 26): Distancia máxima o centinela de celda no visitada / inalcanzable.
- `Type tIntermidiateWork` (líneas 28-32):
  - `Known As Boolean`: Marca de celda visitada (no utilizada activamente en la lógica principal).
  - `DistV As Integer`: Distancia mínima en pasos desde el origen hasta el vértice.
  - `PrevV As tVertice`: Vértice predecesor para la reconstrucción del camino inverso.
- `Dim TmpArray(1 To ROWS, 1 To COLUMS) As tIntermidiateWork` (línea 34): Matriz estática global a nivel de módulo que mantiene la tabla de cálculos intermedios de BFS.
- `Dim TilePosY As Integer` (línea 36): Variable a nivel de módulo sin uso en ninguna función (código muerto).

---

### Procedimientos

#### `Public Sub SeekPath(ByVal NpcIndex As Integer, Optional ByVal MaxSteps As Integer = 30)`
- **Ubicación**: `legacy/server/Codigo/PathFinding.bas`, líneas 129-165.
- **Propósito**: Punto de entrada principal invocado por la IA de NPCs (`AI_NPC.bas`, línea 1033) para calcular la ruta más corta desde la posición actual del NPC hasta `Npclist(NpcIndex).PFINFO.Target`.
- **Flujo de Ejecución**:
  1. Obtiene el mapa del NPC (`NpcMap = Npclist(NpcIndex).Pos.map`).
  2. Invierte las coordenadas del NPC y del objetivo para adaptarlas a la convención interna del módulo (ver Sección 5).
  3. Resetea la sub-grilla en `TmpArray` mediante `InitializeTable(TmpArray, cur_npc_pos)`.
  4. Inicializa la cola FIFO global mediante `InitQueue`.
  5. Encola la posición inicial del NPC (`Push(cur_npc_pos)`).
  6. Ejecuta el bucle BFS (`Do While (Not IsEmpty)`):
     - Desencola un vértice `V = Pop`.
     - Si el vértice es el destino (`V.X = tar_npc_pos.X And V.Y = tar_npc_pos.Y`), interrumpe el bucle (`Exit Do`).
     - Expande los vecinos transitables mediante `ProcessAdjacents(NpcMap, TmpArray, V.Y, V.X, NpcIndex)`.
  7. Invoca `MakePath(NpcIndex)` para reconstruir el camino hallado.

#### `Private Sub ProcessAdjacents(ByVal MapIndex As Integer, ByRef T() As tIntermidiateWork, ByRef vfila As Integer, ByRef vcolu As Integer, ByVal NpcIndex As Integer)`
- **Ubicación**: `legacy/server/Codigo/PathFinding.bas`, líneas 62-127.
- **Propósito**: Examina las 4 casilleros adyacentes cardinales de la celda `(vfila, vcolu)`. Para cada celda válida y transitable que posea `DistV = MAXINT`, actualiza la distancia (`DistV + 1`), asigna el puntero de retroceso (`PrevV`) y la encola en la cola BFS.

#### `Private Sub MakePath(ByVal NpcIndex As Integer)`
- **Ubicación**: `legacy/server/Codigo/PathFinding.bas`, líneas 167-200.
- **Propósito**: Reconstruye la ruta trazada desde el destino hacia el origen leyendo los punteros `PrevV` en `TmpArray`.
- **Comportamiento**:
  - Lee los pasos totales desde `TmpArray(Target.Y, Target.X).DistV`.
  - Si `Pasos = MAXINT`, asigna `NoPath = True` y `PathLenght = 0`.
  - Si hay camino, dimensiona `Npclist(NpcIndex).PFINFO.Path(0 To Pasos)`, recorre hacia atrás desde `Pasos` hasta 1 cargando las posiciones `tVertice`, fija `CurPos = 1` y `NoPath = False`.

#### `Private Sub InitializeTable(ByRef T() As tIntermidiateWork, ByRef S As tVertice, Optional ByVal MaxSteps As Integer = 30)`
- **Ubicación**: `legacy/server/Codigo/PathFinding.bas`, líneas 202-225.
- **Propósito**: Inicializa las casillas de `TmpArray` a sus valores por defecto (`Known = False`, `DistV = MAXINT`, `PrevV = (0,0)`).
- **Detalle de Sub-Grilla**: Solo limpia un área centrada en el origen `S` con un radio de `MaxSteps` (`S.Y - MaxSteps` a `S.Y + MaxSteps`, `S.X - MaxSteps` a `S.X + MaxSteps`). La celda inicial `(S.Y, S.X)` se fija con `DistV = 0`.

#### `Private Function IsWalkable(ByVal map As Integer, ByVal row As Integer, ByVal Col As Integer, ByVal NpcIndex As Integer) As Boolean`
- **Ubicación**: `legacy/server/Codigo/PathFinding.bas`, líneas 48-60.
- **Propósito**: Determina si una coordenada `(row, Col)` en el mapa `map` es transitable por el NPC `NpcIndex`.

#### `Private Function Limites(ByVal vfila As Integer, ByVal vcolu As Integer)`
- **Ubicación**: `legacy/server/Codigo/PathFinding.bas`, líneas 38-46.
- **Propósito**: Evalúa si la coordenada está dentro de las cotas físicas `1 <= vcolu <= 100` y `1 <= vfila <= 100`. Devuelve un tipo `Variant` implícito (evaluado como `Boolean`).

---

## 2. Estructuras de Datos y Algoritmo de Búsqueda (BFS)

### Matrices y Buffers Auxiliares

1. **`TmpArray` (`tIntermidiateWork`)**:
   - Buffer estático de `100 x 100` celdas a nivel de módulo.
   - Evita la alocación dinámica de memoria durante la búsqueda BFS de un mapa individual.
   - Mantiene la distancia mínima desde el origen y la coordenada `PrevV` de retroceso.

2. **Cola FIFO (`Queue.bas`)**:
   - Módulo `legacy/server/Codigo/Queue.bas` (líneas 26-84).
   - Utiliza una grilla lineal redimensionable `m_array() As tVertice` inicializada con capacidad estática de `1000` elementos (`MAXELEM = 1000`).
   - Punteros de estado: `m_firstelem`, `m_lastelem`, `m_size`.

### Orden Exacto de Exploración Cardinal (Determinismo 1:1)

En `ProcessAdjacents` (`legacy/server/Codigo/PathFinding.bas`, líneas 71-126), el algoritmo evalúa a los vecinos en el siguiente orden secuencial estricto:

1. **Norte (`vfila - 1, vcolu`)** (líneas 71-84)
2. **Sur (`vfila + 1, vcolu`)** (líneas 85-98)
3. **Oeste (`vfila, vcolu - 1`)** (líneas 99-112)
4. **Este (`vfila, vcolu + 1`)** (líneas 113-126)

> [!IMPORTANT]
> **Secuencia Fija N-S-O-E**: Para garantizar la paridad exacta del camino elegido en C++ frente a VB6 en el caso de múltiples rutas de igual longitud, la exploración debe realizarse respetando exactamente el orden **Norte -> Sur -> Oeste -> Este**.

### Cotas y Umbrales de Corte

- **Distancia Centinela**: `MAXINT = 1000`. Celdas sin visitar poseen `DistV = 1000`.
- **Cota de la Cola**: `MAXELEM = 1000` en `Queue.bas`. Si la cola se llena, `Push` retorna `False` y los nuevos nodos son ignorados.
- **Parámetro `MaxSteps`**:
  - `SeekPath` recibe `MaxSteps` (por defecto `30`).
  - `InitializeTable` limpia únicamente una ventana de `[-MaxSteps, +MaxSteps]` alrededor del NPC.

---

## 3. Interacción con el Entorno y Estado del Mundo

### Condiciones de Transitabilidad (`IsWalkable`)

`IsWalkable` (`legacy/server/Codigo/PathFinding.bas`, líneas 48-60) evalúa las siguientes reglas sobre el estado global del mapa:

```vb
IsWalkable = MapData(map, row, Col).Blocked = 0 And MapData(map, row, Col).NpcIndex = 0

If MapData(map, row, Col).UserIndex <> 0 Then
     If MapData(map, row, Col).UserIndex <> Npclist(NpcIndex).PFINFO.TargetUser Then IsWalkable = False
End If
```

1. **Bloqueo de Mapa**: `MapData(map, row, Col).Blocked == 0`. (No debe haber un bloqueo estático de mapa).
2. **Bloqueo por NPC**: `MapData(map, row, Col).NpcIndex == 0`. (No debe haber ninguna criatura en la celda).
3. **Bloqueo por Usuario Target**:
   - Si `UserIndex == 0`: La celda está libre de usuarios.
   - Si `UserIndex <> 0`: La celda **solo es transitable** si el usuario parado en ella es exactamente el objetivo perseguido por el NPC (`UserIndex == Npclist(NpcIndex).PFINFO.TargetUser`). Si es cualquier otro usuario, se considera bloqueada.

### Invocaciones Externas y Acoplamiento

- **`InMapBounds(map, y, x)`**: Función externa (en `Extra.bas` / `MapData.bas`) llamada dentro de `InitializeTable` (línea 216).
- **Acceso Directo a Estados Globales**:
  - `MapData(map, Y, X)`
  - `Npclist(NpcIndex)`

---

## 4. Tratamiento de `Queue.bas` y Ciclo de Vida

Auditoría de integración de `Queue.bas` (`legacy/server/Codigo/Queue.bas`):

- **Uso Exclusivo**: El módulo `Queue.bas` es consumido **únicamente** por `PathFinding.bas`. No existe ningún otro módulo en el servidor ni cliente que invoque las rutinas `InitQueue`, `Push` o `Pop`.
- **Naturaleza Transitoria Monohilo**:
  - La cola se reinicia en cada llamada a `SeekPath` mediante `Call InitQueue` (`PathFinding.bas`, línea 228).
  - Toda la búsqueda BFS y el vaciado de la cola ocurren de manera sincrónica en el mismo hilo de ejecución dentro del cuerpo de `SeekPath`.
  - La cola finaliza su ciclo de vida al terminar la función `SeekPath`. No se mantiene estado ni memoria viva entre invocaciones ni entre ticks.

---

## 5. Detección de Quirks, Off-by-Ones y Comportamientos Anómalos

### 1. Inversión Histórica de Coordenadas (X <-> Y)
El encabezado de `PathFinding.bas` (líneas 1-19) documenta explícitamente una discrepancia entre la convención del autor original y el motor ORE (Argentum Online):
- ORE/AO indexa los mapas como `MapData(Map, X, Y)`.
- `PathFinding.bas` trata internamente la primera coordenada como `Y` (Fila) y la segunda como `X` (Columna): `TmpArray(Fila, Columna)`.
- Para compensar esto, en `SeekPath` (líneas 221-222) se invierten las coordenadas al asignar:
  `cur_npc_pos.X = Npclist(NpcIndex).Pos.Y`
  `cur_npc_pos.Y = Npclist(NpcIndex).Pos.X`
- Asimismo, al llamar a `SeekPath` en `AI_NPC.bas` (líneas 1030-1031) y al leer la ruta calculada (líneas 990-991), el código invierte explícitamente los componentes `X` e `Y`.

### 2. Variable `steps` sin Incrementar (Bug en Bucle Principal)
En `SeekPath` (`legacy/server/Codigo/PathFinding.bas`, líneas 217-237):
```vb
steps = 0
Do While (Not IsEmpty)
    If steps > MaxSteps Then Exit Do
    V = Pop
    ...
    Call ProcessAdjacents(...)
Loop
```
La variable `steps` se inicializa en `0` y **nunca es incrementada** dentro del bucle (`steps = steps + 1` no existe). Por ende, el control `If steps > MaxSteps Then Exit Do` es inoperante y jamás aborta la búsqueda. El corte real ocurre únicamente si la cola se vacía (`IsEmpty`) o si se alcanza el objetivo (`tar_npc_pos`).

### 3. Limpieza Incompleta de `TmpArray` (Efecto Colateral de Sub-Grilla)
`InitializeTable` (líneas 212-220) limita la limpieza de `TmpArray` a un sub-cuadrante de `[-MaxSteps, +MaxSteps]` alrededor del origen.
Si en una búsqueda BFS anterior la exploración se extendió a casilleros fuera de ese rango, o si la posición inicial de un NPC cambia bruscamente, esas celdas conservan distancias o predecesores obsoletos de la llamada anterior, pudiendo ocasionar corrupción de punteros `PrevV` o rutas defectuosas.

### 4. Firma sin Tipo de Retorno Explicito en `Limites`
`Private Function Limites(ByVal vfila As Integer, ByVal vcolu As Integer)` (línea 38) omite la cláusula `As Boolean`. En VB6 esto provoca que retorne un tipo `Variant` con costo de empaquetado/desempaquetado implícito.

### 5. Variable Residual de Módulo
`Dim TilePosY As Integer` (línea 36) es una variable a nivel de módulo no utilizada en ningún procedimiento.

---

## 6. Conclusiones y Recomendaciones de Portabilidad

1. **Paridad BFS Mono-hilo**: La portabilidad a C++ debe mantener una cola local o buffer temporal `std::vector` / `std::deque` de vértices, encapsulada completamente dentro del servicio de PathFinding sin necesidad de módulos globales como `Queue.bas`.
2. **Determinismo Direccional**: Es imprescindible preservar el orden de evaluación adyacente **Norte -> Sur -> Oeste -> Este** (`Y-1`, `Y+1`, `X-1`, `X+1`) para garantizar paridad 1:1 en las rutas elegidas.
3. **Limpieza Completa de Estado**: En la versión C++, la grilla de distancias/visitados debe limpiarse completamente o rastrearse mediante una estampa de id de búsqueda (*visit token*) para evitar la fuga de estado entre búsquedas descrita en el Quirk #3.
4. **Normalización de Coordenadas**: Se recomienda utilizar estructuras de posición estándar `WorldPos` / `Position` con `(X, Y)` consistentes, eliminando las inversiones manuales de coordenadas de VB6.
