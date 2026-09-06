---
area: loop-del-juego
source_files:
  - legacy/server/Codigo/frmMain.frm
  - legacy/server/Codigo/General.bas
  - legacy/server/Codigo/NPCs.bas
  - legacy/server/Codigo/Modulo_UsUaRiOs.bas
  - legacy/client/CODIGO/MainTimer.cls
  - legacy/client/CODIGO/General.bas
tags: [loop, game-loop, timers, fps, ticks, ejecucion]
last_updated: 2026-09-06
---

## Resumen
El servidor legacy de Argentum Online utiliza controles `VB.Timer` en el formulario principal oculto (`frmMain.frm`) para disparar eventos periódicos como la IA de NPCs, regeneración de stats, limpieza del mundo y autoguardado.

## Hallazgos

- **Timers del Servidor (`frmMain.frm`)**:
  - `tNPCAI` (Intervalo ~40-100ms): Invoca la IA de todos los NPCs activos en el mundo (`NPC_AI`), gestionando su movimiento, persecución y ataques.
  > Fuente: `legacy/server/Codigo/frmMain.frm`, evento `tNPCAI_Timer`
  - `tGameLoop` (Intervalo ~40ms / 25 Ticks/seg): Procesa la lógica principal del juego: estados temporales, venenos, regeneración de HP/Maná (`RegenerarHP`, `RegenerarMana`), efectos de hechizos y movimiento de barcos.
  > Fuente: `legacy/server/Codigo/frmMain.frm`, evento `tGameLoop_Timer`
  - `tCleanWorld` (Intervalo ~60.000ms / 1 min): Barre el suelo de todos los mapas eliminando ítems tirados no protegidos.
  > Fuente: `legacy/server/Codigo/frmMain.frm`, evento `tCleanWorld_Timer`
  - `tSaveAuto` (Intervalo ~300.000ms / 5 min): Dispara la copia de seguridad global del servidor (`DoBackUp`).
  > Fuente: `legacy/server/Codigo/frmMain.frm`, evento `tSaveAuto_Timer`

- **Loop del Cliente**:
  - El cliente ejecuta un bucle `DoEvents` con sincronización `GetTickCount` en `GameLoop` (`General.bas`) para mantener una tasa constante de renderizado (~40 FPS) mediante la clase `MainTimer.cls`.
  > Fuente: `legacy/client/CODIGO/General.bas`, función `Main`

## Lógica y Datos Extraídos

### Tabla de Timers y Frecuencias del Servidor

| Timer | Intervalo por Defecto | Procedimiento Asociado | Tareas Principales Ejecutadas |
| :--- | :--- | :--- | :--- |
| `tNPCAI` | 40 ms | `NPC_AI` | Pathfinding, agresión a jugadores, ataques y movimiento de NPCs |
| `tGameLoop` | 40 ms | `GameLoop` | Regeneración de stats, expiración de hechizos, veneno, estamina |
| `tCleanWorld` | 60.000 ms (1 min) | `LimpiarMundo` | Eliminación de objetos arrojados en los mapas |
| `tSaveAuto` | 300.000 ms (5 min) | `DoBackUp` | Persistencia en disco de todos los personajes y estados |

## Preguntas Abiertas
- En la migración a C++, la arquitectura basada en controles GUI `VB.Timer` debe reemplazarse por un game loop de alta precisión con ticks fijos (e.g. 40ms / 25 Hz) con hilos dedicados para la simulación del mundo.
