---
area: ui-screens
source_files:
  - legacy/client/CODIGO/frmBancoObj.frm
  - legacy/client/CODIGO/frmCarp.frm
  - legacy/client/CODIGO/frmCharInfo.frm
  - legacy/client/CODIGO/frmComerciar.frm
  - legacy/client/CODIGO/frmComerciarUsu.frm
  - legacy/client/CODIGO/frmConnect.frm
  - legacy/client/CODIGO/frmCrearPersonaje.frm
  - legacy/client/CODIGO/frmForo.frm
  - legacy/client/CODIGO/frmGuildFoundation.frm
  - legacy/client/CODIGO/frmGuildLeader.frm
  - legacy/client/CODIGO/frmHerrero.frm
  - legacy/client/CODIGO/frmMain.frm
  - legacy/client/CODIGO/frmOpciones.frm
  - legacy/client/CODIGO/frmPanelGm.frm
  - legacy/client/CODIGO/frmParty.frm
  - legacy/client/CODIGO/frmSkills3.frm
tags: [ui, forms, screens, gui, dialogs]
last_updated: 2026-09-05
---

## Summary
The client UI consists of over 45 distinct forms (`.frm`) handling main rendering, login, character creation, crafting, trade, banking, party, guild management, and GM panels.

## Findings

- **Main HUD**: Main game canvas viewport, chat console, inventory grid, spellbook, hotbars.
  > Source: `legacy/client/CODIGO/frmMain.frm`
- **Login & Character Creation**: Server connection, credentials entry, 5-race and 12-class character creation wizard.
  > Source: `legacy/client/CODIGO/frmConnect.frm`<br>Source: `legacy/client/CODIGO/frmCrearPersonaje.frm`
- **Economy & Banking**: Merchant shop trading (`frmComerciar.frm`), player-to-player trade (`frmComerciarUsu.frm`), bank vault (`frmBancoObj.frm`).
  > Source: `legacy/client/CODIGO/frmComerciar.frm`<br>Source: `legacy/client/CODIGO/frmComerciarUsu.frm`<br>Source: `legacy/client/CODIGO/frmBancoObj.frm`
- **Crafting & Skills**: Blacksmithing (`frmHerrero.frm`), Carpentry (`frmCarp.frm`), Skill point allocation (`frmSkills3.frm`).
  > Source: `legacy/client/CODIGO/frmHerrero.frm`<br>Source: `legacy/client/CODIGO/frmCarp.frm`<br>Source: `legacy/client/CODIGO/frmSkills3.frm`
- **Guilds & Social**: Party management (`frmParty.frm`), Guild foundation (`frmGuildFoundation.frm`), Guild leader panel (`frmGuildLeader.frm`), Bulletin board forum (`frmForo.frm`).
  > Source: `legacy/client/CODIGO/frmParty.frm`<br>Source: `legacy/client/CODIGO/frmGuildLeader.frm`<br>Source: `legacy/client/CODIGO/frmForo.frm`

## Extracted Logic / Data

### Key UI Form Inventory

| Screen Name | File Reference | Primary Display Content | Player Actions |
| :--- | :--- | :--- | :--- |
| **Main HUD** | `frmMain.frm` | Viewport canvas, status bars, inventory grid, spellbook, chat | Move, attack, cast, talk, use items, macros |
| **Login Screen** | `frmConnect.frm` | Server selector, username/password fields, news banner | Enter credentials, select server, click login/create |
| **Char Creation** | `frmCrearPersonaje.frm` | Race/class/gender options, head preview, dice roll stats | Roll stats, select race/class/city, create character |
| **NPC Commerce** | `frmComerciar.frm` | Merchant catalog, buy/sell prices, player inventory | Buy/sell items, enter quantities |
| **Player Trade** | `frmComerciarUsu.frm` | Dual trade slots, gold offer input, lock indicators | Drag items, offer gold, accept/cancel trade |
| **Bank Vault** | `frmBancoObj.frm` | Bank storage grid (40 slots), wallet gold balance | Deposit/withdraw items and gold |
| **Blacksmithing** | `frmHerrero.frm` | Craftable metal weapons/armors, ingot recipes | Select recipe, craft items |
| **Carpentry** | `frmCarp.frm` | Craftable wooden staves/bows/shields, wood recipes | Select recipe, craft items |
| **Guild Panel** | `frmGuildLeader.frm` | Member roster, applicant list, war/peace controls | Accept members, kick, declare war/peace |
| **GM Panel** | `frmPanelGm.frm` | GM shortcut buttons (teleport, ban, spawn, info) | Execute administrative GM actions |

## Open Questions
- Client GUI relies on native VB6 control layouts; converting UI to C++ requires modern GUI frameworks (e.g. ImGui, RmlUi, or custom DirectX/OpenGL UI system).
