---
area: game-loop
source_files:
  - legacy/server/Codigo/Admin.bas
  - legacy/server/Codigo/FileIO.bas
  - legacy/server/Codigo/General.bas
  - legacy/server/Codigo/MODULO_NPCs.bas
  - legacy/server/Codigo/ModAreas.bas
  - legacy/server/Codigo/Modulo_UsUaRiOs.bas
  - legacy/server/Codigo/frmMain.frm
  - legacy/server/Codigo/modCentinela.bas
tags: [timers, game-loop, server, async, intervals]
last_updated: 2026-09-05
---

## Summary
The server does not run an infinite `While-Sleep` thread. It uses an **asynchronous event-driven architecture** driven by 12 Visual Basic 6 `VB.Timer` controls hosted on background form `frmMain.frm`.

## Findings

- **Primary Game Tick Loop (`GameTimer`)**:
  - Runs every **40 ms** (25 Hz tick rate).
  > Source: `legacy/server/Codigo/frmMain.frm`, subroutine `GameTimer_Timer`
  - Iterates connected players (`1 To MaxUsers`), evaluating tile events (`DoTileEvents`), status effects (poison, paralysis, blindness, invisibility, mimetism, lava, cold), meditation (`DoMeditar`), hunger/thirst, health regen (`Sanar`), and stamina regen (`RecStamina`).

- **NPC AI Loop (`TIMER_AI`)**:
  - Runs every **100 ms** (10 Hz tick rate).
  > Source: `legacy/server/Codigo/frmMain.frm`, subroutine `TIMER_AI_Timer`
  - Iterates active NPCs (`1 To LastNPC`), validating pet ownership (`ValidarPermanenciaNpc`), checking paralysis, processing Praetorian boss AI, and running core pathfinding (`NPCAI`).

- **Network Socket Buffer Flush (`packetResend`)**:
  - Runs every **10 ms** (100 Hz rate).
  > Source: `legacy/server/Codigo/frmMain.frm`, subroutine `packetResend_Timer`
  - Flushes queued byte buffers to connected sockets via `EnviarDatosASlot`.

- **World Persistence Loop (`AutoSave`)**:
  - Runs every **60,000 ms** (1 minute rate).
  > Source: `legacy/server/Codigo/frmMain.frm`, subroutine `AutoSave_Timer`
  - Runs grid area optimization (`AreasOptimizacion`), ticks centinel anti-bot timers, executes world backup (`DoBackUp`), and every 15 minutes sweeps dropped ground items (`LimpiarMundo`) and respawns guards (`ReSpawnOrigPosNpcs`).

## Extracted Logic / Data

### Server Timer Registry Table

| Timer Name | Interval (ms) | Frequency / Tick Rate | Primary Responsibility | Source Citation |
| :--- | :--- | :--- | :--- | :--- |
| **`packetResend`** | **10 ms** | 100 Hz | Flushes queued binary socket streams. | `frmMain.frm`, `packetResend_Timer` |
| **`GameTimer`** | **40 ms** | 25 Hz *(Primary Tick)* | Player status effects, environmental damage, HP/Mana/Stamina regen. | `frmMain.frm`, `GameTimer_Timer` |
| **`TIMER_AI`** | **100 ms** | 10 Hz | NPC AI decision making, pathfinding, target acquisition. | `frmMain.frm`, `TIMER_AI_Timer` |
| **`Auditoria`** | **1,000 ms** | 1 Hz | Flushes GM auditing logs to disk. | `frmMain.frm`, `Auditoria_Timer` |
| **`tLluvia`** | **500 ms** | 2 Hz | Animated rain particle effects & tile weather states. | `frmMain.frm`, `tLluvia_Timer` |
| **`npcataca`** | **4,000 ms** | 0.25 Hz *(4 sec)* | Resets global NPC attack permissions (`CanAttack = 1`). | `frmMain.frm`, `npcataca_Timer` |
| **`securityTimer`**| **10,000 ms** | 0.1 Hz *(10 sec)* | Anti-cheat anti-mass-login security checks. | `frmMain.frm`, `securityTimer_Timer` |
| **`AutoSave`** | **60,000 ms**| 1/60 Hz *(1 min)* | World state backups, ground item sweeps (15 min), jail timeouts. | `frmMain.frm`, `AutoSave_Timer` |
| **`tLluviaEvent`** | **60,000 ms**| 1/60 Hz *(1 min)* | Weather event duration timer (starts/stops rain). | `frmMain.frm`, `tLluviaEvent_Timer` |

## Open Questions
- In C++, VB6 `VB.Timer` controls will be replaced by a modern high-precision main loop thread tick executor (e.g. 60 FPS or 20 Hz fixed tick rate thread).
