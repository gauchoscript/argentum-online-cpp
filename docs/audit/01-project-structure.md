---
area: project-structure
source_files:
  - legacy/client/CODIGO/Application.bas
  - legacy/client/CODIGO/Declares.bas
  - legacy/client/CODIGO/GameIni.bas
  - legacy/client/CODIGO/General.bas
  - legacy/client/CODIGO/TileEngine.bas
  - legacy/server/Codigo/Declares.bas
  - legacy/server/Codigo/FileIO.bas
  - legacy/server/Codigo/GameLogic.bas
  - legacy/server/Codigo/General.bas
tags: [architecture, codebase, vb6, modules]
last_updated: 2026-09-05
---

## Summary
The codebase is structured into two main applications: `legacy/client/` and `legacy/server/`. Source code resides in `legacy/client/CODIGO/` and `legacy/server/Codigo/` consisting of Visual Basic 6 standard modules (`.bas`), class modules (`.cls`), and user interface forms (`.frm`).

## Findings
The top-level structure cleanly isolates client runtime code and assets from server binaries and world state files.

- **Client Code Structure**:
  - `legacy/client/CODIGO/`: Contains 117 VB6 source files (`.bas`, `.cls`, `.frm`, `.frx`).
  > Source: `legacy/client/CODIGO/Application.bas`, subroutine `Main`
  - Graphics, sound, map, and configuration directories (`Graficos/`, `WAV/`, `MIDI/`, `MP3/`, `Mapas/`, `INIT/`).
  > Source: `legacy/client/CODIGO/GameIni.bas`, function `LeerGameIni`

- **Server Code Structure**:
  - `legacy/server/Codigo/`: Contains 64 VB6 source files (`.bas`, `.cls`, `.frm`).
  > Source: `legacy/server/Codigo/General.bas`, subroutine `Main`
  - Data storage directories for saved characters (`Charfile/`), world map files (`Maps/`), static configuration DATs (`Dat/`), guild files (`guilds/`), and server logs (`Logs/`).
  > Source: `legacy/server/Codigo/FileIO.bas`, subroutine `DoBackUp`

## Extracted Logic / Data

### Top-Level Folders

| Directory Path | Description & Responsibility | Category |
| :--- | :--- | :--- |
| `legacy/client/` | Client root binary and assets | Client Root |
| `legacy/client/CODIGO/` | VB6 client source code (`.bas`, `.cls`, `.frm`) | Client Source |
| `legacy/client/Graficos/` | Sprite sheets and tileset textures (`.bmp`) | Assets |
| `legacy/client/INIT/` | Configuration files (`.ini`, `.ind`) | Config / Data |
| `legacy/client/Mapas/` | Client map binary files (`.map`) | Maps |
| `legacy/client/WAV/` | Sound effects audio files (`.wav`) | Audio |
| `legacy/client/MIDI/` & `legacy/client/MP3/` | Ambient background music tracks | Audio |
| `legacy/server/` | Dedicated server root application | Server Root |
| `legacy/server/Charfile/` | Saved player character profiles (`.chr`) | Database |
| `legacy/server/Codigo/` | VB6 server source code (`.bas`, `.cls`, `.frm`) | Server Source |
| `legacy/server/Dat/` | Server database files (`OBJ.dat`, `NPCs.dat`, `Hechizos.dat`) | Data |
| `legacy/server/guilds/` | Guild data storage files (`.guild`, `.inf`, `.mem`) | Guilds |
| `legacy/server/Logs/` | Server runtime logs (`GMs.log`, `Hack.log`, `Errores.log`) | Logs |
| `legacy/server/Maps/` | Server binary map and trigger files (`.map`, `.inf`) | Maps |

## Open Questions
- Some `.frx` binary form resource files in `legacy/client/CODIGO/` contain legacy graphical embeds; need to ensure all UI elements can be rendered cleanly without legacy VB6 binary resources.
