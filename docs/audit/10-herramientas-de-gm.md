---
area: herramientas-de-gm
source_files:
  - legacy/server/Codigo/Admin.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/TCP.bas
  - legacy/server/Codigo/frmAdmin.frm
tags: [gm, game-master, administracion, comandos, privilegios, rangos]
last_updated: 2026-09-06
---

## Resumen
Las herramientas de administración y Game Master (GM) de Argentum Online v0.13.0 están protegidas por una jerarquía de 5 niveles de privilegios definidos en el archivo de cada personaje y en `Server.ini`.

## Hallazgos

- **Niveles de Privilegio y Autorización**:
  - Los niveles se definen mediante la propiedad `UserList(UserIndex).flags.Privilegios`:
    - `PlayerType.User` (0): Jugador normal.
    - `PlayerType.Consejero` (1): Asistente de soporte y orientador.
    - `PlayerType.SemiDios` (2): GM de moderación intermedia.
    - `PlayerType.Dios` (3): Game Master completo con poderes de spawn y modificación de mapa.
    - `PlayerType.Admin` (4): Administrador total del servidor.
  > Fuente: `legacy/server/Codigo/Admin.bas`, función `EsGM`

- **Comandos de Consola de GM**:
  - Se ejecutan escribiendo comandos con prefijo `/` en la barra de chat o mediante opcodes de administración dedicados.
  - Validación: `HandleGMCommand` verifica `EsGM(UserIndex)` antes de interpretar cada comando.
  > Fuente: `legacy/server/Codigo/Protocol.bas`, función `HandleGMCommand`

- **Panel de Administración GUI**:
  - `frmAdmin.frm`: Formulario en el servidor que permite listar usuarios conectados, buscar cuentas por IP, aplicar Baneos o silenciar chat global.
  > Fuente: `legacy/server/Codigo/frmAdmin.frm`

## Lógica y Datos Extraídos

### Lista Principales de Comandos de GM

| Comando | Nivel Mínimo Requerido | Descripción de la Acción | Fuente |
| :--- | :--- | :--- | :--- |
| `/TELEP` / `/SUMMON` | Dios (3) | Teletransporta al GM hacia un jugador o atrae a un jugador hacia el GM. | `Admin.bas` |
| `/BAN` / `/UNBAN` | SemiDios (2) | Banea la cuenta del jugador de forma permanente o revoca el ban. | `Admin.bas` |
| `/BANIP` | Admin (4) | Bloquea la dirección IP de conexión en `Server.ini`. | `TCP.bas` |
| `/CI <OBJ_ID>` | Dios (3) | Crea una cantidad especificada de un objeto en el inventario del GM. | `Admin.bas` |
| `/ACC <NPC_ID>` | Dios (3) | Invoca una criatura o NPC en la casilla frente al GM. | `Admin.bas` |
| `/INVI` | Consejero (1) | Vuelve al personaje invisible para los jugadores normales. | `Admin.bas` |
| `/INVULNERABLE` | Dios (3) | Otorga inmunidad total ante ataques de criaturas y jugadores. | `Admin.bas` |
| `/SILENCIAR` | Consejero (1) | Deshabilita el chat público de un jugador. | `Admin.bas` |

## Preguntas Abiertas
- En la implementación en C++, los niveles de acceso de GM se deben modelar como un enum explícito (`enum class GMRank { None = 0, Counselor = 1, SemiGod = 2, God = 3, Admin = 4 }`) garantizando el chequeo de permisos en cada comando RPC.
