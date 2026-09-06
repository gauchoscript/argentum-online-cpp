---
area: herramientas-de-gm
status: not-started
audit_reference: docs/audit/10-herramientas-de-gm.md
tags: [gm, game-master, administracion, comandos, privilegios, rangos]
last_updated: 2026-09-06
---

## Decisiones de Diseño
- **Sistema de Permisos de GM en C++**: Implementar `GMRankManager` con enums de rango estricto (`GMRank`) y decoradores/guards en el despachador de comandos para validar los permisos de administración antes de ejecutar cualquier acción RPC.

## Preguntas Abiertas / Riesgos
- *Heredado de la auditoría*: Asegurar que la tabla de comandos mantenga la paridad exacta de comandos `/` con el servidor legacy de VB6.

## Tareas
- [ ] Implementar la clase `GMCommandDispatcher` en C++.
- [ ] Recrear los comandos principales (`/TELEP`, `/SUMMON`, `/BAN`, `/CI`, `/ACC`, `/INVI`).

## Archivos de Código Relacionados
- `legacy/server/Codigo/Admin.bas`
- `legacy/server/Codigo/Protocol.bas`
