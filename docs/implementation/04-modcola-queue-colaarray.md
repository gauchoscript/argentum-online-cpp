---
module: ModCola_Queue_cColaArray
source_files:
  - legacy/server/Codigo/ModCola.cls
  - legacy/server/Codigo/Queue.bas
  - legacy/server/Codigo/cColaArray.cls
  - src/server/ModCola.hpp
  - src/server/ModCola.cpp
  - tests/test_modcola.cpp
tags: [modcola, ccola, queue, ccolaarray, capa-0, cpp, doctest, ayuda, pathfinding]
last_updated: 2026-09-08
---

# Decisiones de Diseño e Implementación: ModCola.cls (`cCola`), Queue.bas (`Queue`) y cColaArray.cls (`CColaArray`)

Este documento registra las decisiones de diseño, auditoría de concurrencia, quirks de VB6 preservados y detalles de implementación C++ para los tres módulos de cola de Capa 0 (`ModCola.cls`, `Queue.bas` y `cColaArray.cls`), de acuerdo con la lista de chequeo de finalización de módulos del proyecto (`docs/CONVENTIONS.md`).

---

## 1. Auditoría e Implementación de `ModCola.cls` (`cCola`)

### Naturaleza y Propósito del Módulo
A pesar del nombre genérico `ModCola` / `cCola`, este módulo en el servidor original de Argentum Online no es un contenedor de cola FIFO genérico, sino la lista/gestor especializado de la cola de soporte y peticiones de Game Masters (`/AYUDA`), instanciado globalmente como `Public Ayuda As New cCola` en `Declares.bas`.

### Estructura de Datos Subyacente
En VB6 la clase utilizaba un objeto nativo `Collection` (`Private Cola As Collection`). Dado que el módulo requiere acceso directo por índice 1-based (`VerElemento`), inserciones al final, borrado por frente (`Pop`) y eliminación de elementos intermedios por valor o por índice (`Quitar`, `QuitarIndex`), se eligió **`std::deque<std::string>`** como contenedor subyacente en C++, ofreciendo rendimiento $O(1)$ en extremos y acceso eficiente por índice.

### Preservación del Formato de Marca de Tiempo (Timestamp)
Al llamar a `Push(Nombre)`, VB6 concatena la hora actual devuelta por la función de sistema `time$` (formato `"HH:MM:SS"`, 8 caracteres), un espacio delimitador, y el nombre del usuario en mayúsculas: `"HH:MM:SS NOMBRE"`.
- En C++ se implementó `ObtenerHoraActual()` utilizando `std::strftime("%H:%M:%S")` para garantizar exactamente el mismo formato de 8 caracteres.
- Se proveyó además la función auxiliar `PushConTiempo(Nombre, TimeStr)` para posibilitar pruebas unitarias deterministas sin depender del reloj del sistema.

### Búsqueda y Eliminación Ignorando el Timestamp (`Existe`, `Quitar`)
Las funciones `Existe(Nombre)` y `Quitar(Nombre)` operan ignorando los primeros 9 caracteres del elemento (`"HH:MM:SS "`) mediante `elem.substr(9)` (correspondiente a `Mid$(VerElemento(i), 10)` en VB6) y comparando el resto contra el nombre normalizado a mayúsculas (`AMayusculas`).
- `Quitar` remueve únicamente la primera ocurrencia encontrada que coincida con el nombre.

### Respuestas por Defecto en Casos de Error o Cola Vacía
Fiel al comportamiento de VB6 impulsado por `On Error Resume Next` y conversiones automáticas de tipos a `String`:
- `Pop()` y `PopByVal()` sobre una cola vacía devuelven la cadena `"0"`.
- `VerElemento(index)` con un `index` fuera de rango (base 1) devuelve la cadena `"0"`.
- `QuitarIndex(index)` ignora silenciadamente solicitudes con índices fuera de rango ($\le 0$ o $> \text{Longitud()}$).

---

## 2. Auditoría y Disposición Final de `Queue.bas` (`Queue`)

### Investigación del Modelo de Ejecución y Concurrencia
Se analizó minuciosamente el código fuente del servidor legacy para determinar si `Queue.bas` requiere soporte para búsquedas concurrentes/superpuestas o si el modelo del juego garantiza ejecuciones estrictamente secuenciales de a una búsqueda a la vez.

* **Puntos de invocación**: `Queue.bas` (y sus procedimientos `InitQueue`, `Push`, `Pop`, `IsEmpty`, `IsFull`) se utiliza **exclusivamente** dentro de `legacy/server/Codigo/PathFinding.bas`, en la rutina `SeekPath(NpcIndex)`.
* **Invocación en el juego**: `SeekPath` es llamada únicamente desde el bucle de IA de NPCs (`AI_NPC.bas:L1033`).
* **Modelo de ejecucion**: El servidor opera sobre un loop monohilo sincrónico. Durante cada ciclo de timer de IA, el servidor itera secuencialmente sobre la lista de NPCs (`For i = 1 To LastNPC`), procesando la IA de un solo NPC a la vez. Cuando un NPC requiere calcular un camino, invoca `SeekPath(NpcIndex)` en forma síncrona: inicializa la cola con `InitQueue`, ejecuta la búsqueda por anchura (BFS) consumiendo la cola, construye el camino (`MakePath`) y finaliza completamente antes de pasar al siguiente NPC.
* **Conclusión de concurrencia**: Se confirma de forma absoluta que **NO existen búsquedas de caminos concurrentes ni superpuestas** en todo el servidor.

### Disposición Final y Estrategia de Porting para `Queue.bas`
1. **Sin módulo global en C++**: No se creará una clase ni módulo estático/global `Queue.hpp`/`Queue.cpp` para evitar perpetuar el detalle de implementación del código legacy de VB6 (estado global a nivel de módulo con arreglos estáticos).
2. **Cola local en PathFinding**: Al portar el módulo de Pathfinding (`PathFinding.bas`, Capa 8), el comportamiento de la cola se implementará directamente como un objeto local **`std::queue<tVertice>`** dentro del ámbito de la función de búsqueda (`SeekPath`).
3. **Ubicación de `tVertice`**: La estructura plana `tVertice` se encuentra definida centralizadamente en `src/server/Declares.hpp` (líneas 52-55):
   ```cpp
   struct tVertice {
       std::int16_t X{0};
       std::int16_t Y{0};
   };
   ```
   Esta ubicación será reevaluada cuando se realice el porting integral de `PathFinding.bas`.

---

## 3. Disposición de `cColaArray.cls` (`CColaArray`)

- **Estado**: **EXCLUIDO (Código Muerto)**.
- **Fundamento**: Se confirmó que `cColaArray.cls` es 100% código inalcanzable. Está encerrada en un bloque `#If UsarQueSocket = 3` deshabilitado en `SERVER.VBP` (`UsarQueSocket = 1`), su única referencia (`CommandsBuffer`) fue borrada del `Type User` en `Declares.bas` (por lo que ni siquiera compilaría), y el changelog de 2006-2007 confirma que fue reemplazada por `clsByteQueue`.
- **Registro de auditoría**: Ver detalle completo en [`docs/audit/06a-colaarray-dead-code.md`](../audit/06a-colaarray-dead-code.md).

---

## Resumen del Estado de los Tres Módulos

| Módulo Legacy | Estado en el Port | Archivo C++ Resultante / Estrategia |
| :--- | :--- | :--- |
| `ModCola.cls` (`cCola`) | **Completado** | `src/server/ModCola.hpp` / `src/server/ModCola.cpp` (Probado con `test_modcola.cpp`) |
| `Queue.bas` (`Queue`) | **Refactorizado a local** | Se usará `std::queue<tVertice>` local dentro de `SeekPath` en `PathFinding`. No requiere módulo global. |
| `cColaArray.cls` (`CColaArray`) | **Excluido** | Ninguno (Código muerto registrado en `docs/audit/06a-colaarray-dead-code.md`). |
