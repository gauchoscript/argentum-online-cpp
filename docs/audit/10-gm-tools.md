---
area: gm-tools
source_files:
  - legacy/client/CODIGO/frmCambiaMotd.frm
  - legacy/client/CODIGO/frmPanelGm.frm
  - legacy/client/CODIGO/frmSpawnList.frm
  - legacy/server/Codigo/Admin.bas
  - legacy/server/Codigo/Declares.bas
  - legacy/server/Codigo/FileIO.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/frmAdmin.frm
tags: [admin, gm, commands, privileges, security, dashboard]
last_updated: 2026-09-05
---

## Summary
Gamemaster functionality is gated by a 5-tier rank hierarchy (`Consejero`, `SemiDios`, `Dios`, `Admin`, `User`) mapped in `legacy/server/Server.ini`. GMs issue text commands (`/BAN`, `/TELEP`, `/CI`, `/ACC`) or use client GUI forms (`frmPanelGm.frm`).

## Findings

- **Authorization Mechanics**:
  - `EsAdmin`, `EsDios`, `EsSemiDios`, `EsConsejero` check player usernames against `[Admines]`, `[Dioses]`, `[SemiDioses]`, `[Consejeros]` sections in `Server.ini`.
  > Source: `legacy/server/Codigo/FileIO.bas`, function `EsAdmin`<br>Source: `legacy/server/Codigo/FileIO.bas`, function `EsDios`
  - Rank bitmask flags are set in `UserList(UserIndex).flags.Privilegios`. `HandleGMCommands` enforces permission checks via bitwise AND:
    ```vb
    If (UserList(UserIndex).flags.Privilegios And (PlayerType.Admin Or PlayerType.Dios)) = 0 Then Exit Sub
    ```
  > Source: `legacy/server/Codigo/Protocol.bas`, function `HandleGMCommands`

- **GM Command Hierarchy**:
  - **Consejero (Rank 2)**: Basic support (`/INVISIBLE`, `/SHOW SOS`, `/TRAINING`, `/SUMMON` counselor, `/TELEP`).
  - **SemiDios (Rank 4)**: Moderate GM (`/IRA`, `/SUM`, `/BAN`, `/UNBAN`, `/ECHAR`, `/CARCEL`, `/SILENCIAR`, `/REVIVIR`).
  - **Dios (Rank 8)**: Senior GM (`/CI` item creation, `/ACC` NPC creation, `/KILL`, `/MOD` edit stats, `/DEST`).
  - **Admin (Rank 16)**: Full superuser (`/APAGAR` shutdown, `/HABILITAR` server open/close, `/APASS` alter password, `/DOBACKUP`).

- **GM UI Forms**:
  - `frmPanelGm.frm`: GM shortcut dashboard for teleports, bans, stat checks, and invulnerability.
  > Source: `legacy/client/CODIGO/frmPanelGm.frm`
  - `frmSpawnList.frm`: Searchable NPC database list for real-time monster spawning via `/SPA`.
  > Source: `legacy/client/CODIGO/frmSpawnList.frm`
  - `frmAdmin.frm`: Dedicated server GUI console for managing active sockets and IP ban tables.
  > Source: `legacy/server/Codigo/frmAdmin.frm`

## Extracted Logic / Data

### GM Privilege Ranks (`PlayerType`)

```vb
Public Enum PlayerType
    User = 1
    Consejero = 2
    SemiDios = 4
    Dios = 8
    Admin = 16
    RoyalCouncil = 32
    ChaosCouncil = 64
End Enum
```

## Open Questions
- In C++, GM command authorization should be driven by database role-based access control (RBAC) rather than static usernames listed in an INI file.
