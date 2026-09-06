# Argentum Online v0.13.0 Codebase Audit Index

This directory contains a frozen historical record of the legacy Visual Basic 6 Argentum Online v0.13.0 codebase architecture, protocol specifications, math formulas, and subsystem implementations.

## Audit Documents Index

| # | Document | Short Description | Area Slug | Last Updated |
| :-: | :--- | :--- | :--- | :-: |
| 01 | [01-project-structure.md](01-project-structure.md) | Top-level folder structure and VB6 code module organization. | `project-structure` | 2026-09-05 |
| 02 | [02-network-protocol.md](02-network-protocol.md) | Custom binary socket protocol opcodes, serialization, and packet handlers. | `network-protocol` | 2026-09-05 |
| 03 | [03-prediction-check.md](03-prediction-check.md) | Client-side movement prediction vs. server-authoritative combat execution. | `prediction-check` | 2026-09-05 |
| 04 | [04-combat-formulas.md](04-combat-formulas.md) | Exact formulas for damage, hit/miss odds, EXP allocation, and HP/Mana regen. | `combat-formulas` | 2026-09-05 |
| 05 | [05-game-loop.md](05-game-loop.md) | Async VB.Timer control event loop frequency and tick callbacks. | `game-loop` | 2026-09-05 |
| 06 | [06-data-formats.md](06-data-formats.md) | Flat INI text files (`.chr`, `.dat`, `.guild`) and binary map stream format (`.map`). | `data-formats` | 2026-09-05 |
| 07 | [07-ui-screens.md](07-ui-screens.md) | Inventory of 45+ client UI forms (`.frm`) and interactive player screens. | `ui-screens` | 2026-09-05 |
| 08 | [08-assets.md](08-assets.md) | Bitmap sprites (`.bmp`), `.ind` animation indices, WAV audio, MIDI, and MP3. | `assets` | 2026-09-05 |
| 09 | [09-login-security.md](09-login-security.md) | Authentication flow, plaintext INI password storage, and socket vulnerabilities. | `login-security` | 2026-09-05 |
| 10 | [10-gm-tools.md](10-gm-tools.md) | 5-tier GM privilege hierarchy, command opcodes (`/BAN`, `/CI`), and admin GUI forms. | `gm-tools` | 2026-09-05 |
