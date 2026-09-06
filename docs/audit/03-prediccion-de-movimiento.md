---
area: prediccion-de-movimiento
source_files:
  - legacy/client/CODIGO/General.bas
  - legacy/client/CODIGO/Protocol.bas
  - legacy/client/CODIGO/ProtocolCmdParse.bas
  - legacy/client/CODIGO/TileEngine.bas
  - legacy/client/CODIGO/frmMain.frm
  - legacy/server/Codigo/Modulo_UsUaRiOs.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/SistemaCombate.bas
tags: [prediccion, movimiento, combate, sincronizacion, extrapolacion]
last_updated: 2026-09-06
---

## Resumen
El movimiento utiliza **Predicción del Cliente (Extrapolación)** — el cliente actualiza la posición del personaje en la grilla y la cámara inmediatamente al presionar la tecla. El combate utiliza **Autoridad Estricta del Servidor** — no ocurre ninguna animación local ni cálculo de daño hasta que retornan los paquetes de respuesta del servidor.

## Hallazgos

- **Flujo del Movimiento**:
  - `CheckKeys` detecta la tecla de dirección y llama a `MoveTo(Direccion)`.
  > Fuente: `legacy/client/CODIGO/General.bas`, función `CheckKeys`
  - `MoveTo` evalúa `MoveToLegalPos`. Si es válido:
    a. Envía el paquete `ClientPacketID.Walk` mediante `WriteWalk(Direccion)`.
    > Fuente: `legacy/client/CODIGO/Protocol.bas`, función `WriteWalk`
    b. **INMEDIATAMENTE** actualiza la posición gráfica del personaje mediante `MoveCharbyHead` y desplaza la cámara con `MoveScreen`.
    > Fuente: `legacy/client/CODIGO/General.bas`, función `MoveTo`
  - El servidor valida el paso en `HandleWalk`. Si es inválido, envía `ServerPacketID.PosUpdate`, forzando al cliente a retroceder a la posición válida en `HandlePosUpdate`.
  > Fuente: `legacy/server/Codigo/Protocol.bas`, función `HandleWalk`<br>Fuente: `legacy/client/CODIGO/ProtocolCmdParse.bas`, función `HandlePosUpdate`

- **Flujo del Combate**:
  - `Form_KeyDown` captura la tecla de golpe (`mKeyAttack`) y llama a `WriteAttack`.
  > Fuente: `legacy/client/CODIGO/frmMain.frm`, función `Form_KeyDown`
  - `WriteAttack` escribe el byte de opcode `ClientPacketID.Attack`. **No se realiza ningún cálculo local de daño ni animación.**
  > Fuente: `legacy/client/CODIGO/Protocol.bas`, función `WriteAttack`
  - El servidor procesa el ataque en `HandleAttack` -> `UsuarioAtaca`, calcula evasión y daño, y responde con los paquetes `CreateFX`, `PlayWave`, `ConsoleMsg` y `UpdateHP`.
  > Fuente: `legacy/server/Codigo/Protocol.bas`, función `HandleAttack`<br>Fuente: `legacy/server/Codigo/SistemaCombate.bas`, función `UsuarioAtaca`

## Lógica y Datos Extraídos

### Matriz Comparativa

| Subsistema | Modelo de Ejecución | ¿Actualización Inmediata en Cliente? | Estrategia de Corrección del Servidor |
| :--- | :--- | :--- | :--- |
| **Movimiento** | Predicción del Cliente | **SÍ** (`MoveCharbyHead`, `MoveScreen`) | El servidor envía `ServerPacketID.PosUpdate` en caso de movimiento ilegal; el cliente resetea `UserPos` en `HandlePosUpdate`. |
| **Combate** | Autoridad del Servidor | **NO** (Espera paquetes del servidor) | El servidor calcula probabilidades y daño, retornando `CreateFX` (chispas), `PlayWave` (sonido) y `UpdateHP`. |

## Preguntas Abiertas
- La predicción del cliente no extrapola el movimiento de otros jugadores (solo el jugador local); los demás personajes se posicionan únicamente por los paquetes `CharacterMove` recibidos del servidor.
