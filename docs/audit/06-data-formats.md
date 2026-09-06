---
area: data-formats
source_files:
  - legacy/server/Codigo/Declares.bas
  - legacy/server/Codigo/FileIO.bas
  - legacy/server/Codigo/General.bas
  - legacy/server/Codigo/clsClan.cls
  - legacy/server/Codigo/clsIniReader.cls
tags: [storage, ini, binary, maps, databases, persistence]
last_updated: 2026-09-05
---

## Summary
All game data persistence relies on **flat Windows INI text files** (`.chr`, `.dat`, `.ini`, `.guild`) and **custom packed binary streams** (`.map`, `.inf`). There are zero SQL database or ODBC/ADO connections.

## Findings

- **Player Character Files (`.chr`)**:
  - Saved in `legacy/server/Charfile/<Username>.chr` as standard Windows INI files.
  > Source: `legacy/server/Codigo/FileIO.bas`, function `SaveUser`
  - Grouped into INI headers (`[INIT]`, `[FLAGS]`, `[STATS]`, `[ATRIBUTOS]`, `[INVENTORY]`, `[SPELLS]`, `[FACCIONES]`, `[REPUTACION]`).

- **World Maps (`.map` and `.inf`)**:
  - Binary files in `legacy/server/Maps/` read via `Open ... For Binary` and `Get #`.
  > Source: `legacy/server/Codigo/FileIO.bas`, function `CargarMapa`
  - Header: `MapVersion (Integer)` + `tMapHeader` (273 reserved bytes) + 4 dimension ints.
  - Grid: 100x100 array reading a 1-byte bitmask (`ByFlags`) for blocked tiles and graphic layers 1-4.

- **Game Databases (`OBJ.dat`, `NPCs.dat`, `Hechizos.dat`)**:
  - INI text databases in `legacy/server/Dat/` parsed into array structures via `clsIniReader`.
  > Source: `legacy/server/Codigo/FileIO.bas`, function `LoadOBJData`<br>Source: `legacy/server/Codigo/FileIO.bas`, function `CargarHechizos`

- **Guild Storage (`legacy/server/guilds/`)**:
  - Multi-file INI text database (`guildsinfo.inf`, `<GuildName>-members.mem`, `<GuildName>-relaciones.rel`, `<GuildName>-propositions.pro`).
  > Source: `legacy/server/Codigo/clsClan.cls`, function `Class_Initialize`

## Extracted Logic / Data

### Summary Persistence Schema Table

| Data Type | Extension | Location | Storage Engine | Citation |
| :--- | :--- | :--- | :--- | :--- |
| **Player Characters** | `.chr` | `legacy/server/Charfile/` | INI Text (`WriteVar` / `clsIniReader`) | `FileIO.bas`, `SaveUser` |
| **World Maps** | `.map` / `.inf` | `legacy/server/Maps/` | Binary Stream (`Open ... For Binary`) | `FileIO.bas`, `CargarMapa` |
| **Items Database** | `.dat` | `legacy/server/Dat/OBJ.dat` | INI Text Database (`clsIniReader`) | `FileIO.bas`, `LoadOBJData` |
| **NPCs Database** | `.dat` | `legacy/server/Dat/NPCs.dat` | INI Text Database (`clsIniReader`) | `General.bas`, `CargaNpcsDat` |
| **Spells Database** | `.dat` | `legacy/server/Dat/Hechizos.dat` | INI Text Database (`clsIniReader`) | `FileIO.bas`, `CargarHechizos` |
| **Guild Roster & Diplomacy** | `.inf` / `.mem` | `legacy/server/guilds/` | INI Text Files (`GetVar` / `WriteVar`) | `clsClan.cls`, `Class_Initialize` |

## Open Questions
- Flat INI files incur high disk I/O parsing overhead on server boot; migrating to SQLite or JSON/binary formats in C++ will improve startup performance.
