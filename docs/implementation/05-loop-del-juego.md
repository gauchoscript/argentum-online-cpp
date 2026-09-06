---
area: loop-del-juego
status: not-started
audit_reference: docs/audit/05-loop-del-juego.md
tags: [loop, game-loop, timers, fps, ticks, ejecucion]
last_updated: 2026-09-06
---

## Decisiones de Diseño
- **Game Loop Principal en C++**: Diseñar un bucle continuo de simulación a 25 Hz (tick fijo de 40 ms) utilizando `std::chrono::steady_clock` en un hilo de trabajo dedicado para independizar el servidor de la GUI de Windows.

## Preguntas Abiertas / Riesgos
- *Heredado de la auditoría*: Evitar deriva de tiempo (*timer drift*) comparado con la precisión de `VB.Timer` en Windows.

## Tareas
- [ ] Implementar la clase `GameLoop` con soporte para timers programados (`TaskScheduler`).
- [ ] Migrar las llamadas de `NPC_AI`, `RegenerarHP`, `RegenerarMana` y `LimpiarMundo` al planificador de tareas en C++.

## Archivos de Código Relacionados
- `legacy/server/Codigo/frmMain.frm`
- `legacy/server/Codigo/General.bas`
