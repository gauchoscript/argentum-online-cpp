---
area: prediction-check
source_files:
  - legacy/client/CODIGO/General.bas
  - legacy/client/CODIGO/Protocol.bas
  - legacy/client/CODIGO/ProtocolCmdParse.bas
  - legacy/client/CODIGO/TileEngine.bas
  - legacy/client/CODIGO/frmMain.frm
  - legacy/server/Codigo/Modulo_UsUaRiOs.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/SistemaCombate.bas
tags: [prediction, movement, combat, sync, extrapolation]
last_updated: 2026-09-05
---

## Summary
Movement uses **Client-Side Prediction (Extrapolation)** — the client updates player grid position and camera viewport immediately upon keypress. Combat actions use **Strict Server-Authority** — no local animations or damage occur until server response packets arrive.

## Findings

- **Movement Execution Flow**:
  - `CheckKeys` detects arrow keypress and calls `MoveTo(Direccion)`.
  > Source: `legacy/client/CODIGO/General.bas`, function `CheckKeys`
  - `MoveTo` evaluates `MoveToLegalPos`. If valid:
    a. Pushes `ClientPacketID.Walk` packet to server via `WriteWalk(Direccion)`.
    > Source: `legacy/client/CODIGO/Protocol.bas`, function `WriteWalk`
    b. **IMMEDIATELY** updates client sprite position via `MoveCharbyHead` and scrolls viewport camera via `MoveScreen`.
    > Source: `legacy/client/CODIGO/General.bas`, function `MoveTo`
  - Server validates move in `HandleWalk`. If invalid, server sends `ServerPacketID.PosUpdate` which forces client to snap back to server position in `HandlePosUpdate`.
  > Source: `legacy/server/Codigo/Protocol.bas`, function `HandleWalk`<br>Source: `legacy/client/CODIGO/ProtocolCmdParse.bas`, function `HandlePosUpdate`

- **Combat Action Flow**:
  - `Form_KeyDown` captures attack key (`mKeyAttack`) and calls `WriteAttack`.
  > Source: `legacy/client/CODIGO/frmMain.frm`, function `Form_KeyDown`
  - `WriteAttack` writes single byte opcode `ClientPacketID.Attack` to network buffer. **Zero local hit/damage calculation or animation occurs.**
  > Source: `legacy/client/CODIGO/Protocol.bas`, function `WriteAttack`
  - Server processes attack in `HandleAttack` -> `UsuarioAtaca`, calculates hit odds and damage, then sends back `CreateFX`, `PlayWave`, `ConsoleMsg`, and `UpdateHP` packets.
  > Source: `legacy/server/Codigo/Protocol.bas`, function `HandleAttack`<br>Source: `legacy/server/Codigo/SistemaCombate.bas`, function `UsuarioAtaca`

## Extracted Logic / Data

### Comparison Matrix

| Feature | Execution Model | Immediate Client Update? | Server Correction Strategy |
| :--- | :--- | :--- | :--- |
| **Movement** | Client-Side Prediction | **YES** (`MoveCharbyHead`, `MoveScreen`) | Server sends `ServerPacketID.PosUpdate` on illegal move; client resets `UserPos` in `HandlePosUpdate`. |
| **Combat** | Server-Authoritative | **NO** (waits for server packets) | Server processes combat math, returning `CreateFX` (sparks), `PlayWave` (sound), and `UpdateHP`. |

## Open Questions
- Client prediction does not extrapolate other players' movements (only the local player); remote players are positioned solely by server `CharacterMove` packets.
