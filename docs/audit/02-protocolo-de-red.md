---
area: protocolo-de-red
source_files:
  - legacy/client/CODIGO/Protocol.bas
  - legacy/client/CODIGO/ProtocolCmdParse.bas
  - legacy/client/CODIGO/TCP.bas
  - legacy/client/CODIGO/clsByteQueue.cls
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/TCP.bas
  - legacy/server/Codigo/clsByteQueue.cls
  - legacy/server/Codigo/modSendData.bas
  - legacy/server/Codigo/wskapiAO.bas
  - legacy/server/Codigo/wsksock.bas
tags: [red, protocolo, winsock, paquetes, serializacion]
last_updated: 2026-09-06
---

## Resumen
El protocolo de red es un sistema binario basado en streams administrado por colas de bytes FIFO (`clsByteQueue`). Los opcodes se serializan como bytes individuales (`ClientPacketID` y `ServerPacketID`) seguidos por los argumentos de datos correspondientes.

## Hallazgos
La entrada y salida de datos del socket se realiza mediante controles Winsock y llamadas C-style a la API de Winsock de Windows.

- **Transmisión del Socket en el Cliente**:
  - `ConnectToMainServer` abre la conexión TCP hacia la IP/Puerto destino usando `frmConnect.Winsock1.Connect`.
  > Fuente: `legacy/client/CODIGO/TCP.bas`, función `ConnectToMainServer`
  - La ráfaga de bytes entrante es capturada por `Listening` y volcada en la cola `incomingData`.
  > Fuente: `legacy/client/CODIGO/TCP.bas`, función `Listening`
  - El buffer de salida `outgoingData` se serializa en `Protocol.bas` y se envía mediante `FlushBuffer`.
  > Fuente: `legacy/client/CODIGO/TCP.bas`, función `FlushBuffer`

- **Motor de Sockets del Servidor**:
  - `AcceptNewUser` acepta conexiones entrantes usando `wskapiAO.accept`.
  > Fuente: `legacy/server/Codigo/TCP.bas`, función `AcceptNewUser`
  - Los eventos de socket se procesan de forma asincrónica mediante `SocketWindowProc` gestionando `FD_READ`, `FD_WRITE`, `FD_CLOSE`.
  > Fuente: `legacy/server/Codigo/wsksock.bas`, función `SocketWindowProc`
  - El flujo de datos entrantes es procesado por `HandleIncomingData`, que lee el byte de opcode (`ClientPacketID`) y delega a las funciones handler.
  > Fuente: `legacy/server/Codigo/Protocol.bas`, función `HandleIncomingData`

## Lógica y Datos Extraídos

### Opcodes Cliente a Servidor (`ClientPacketID`)

| Comando | Opcode ID | Acción Disparadora | Estructura de Payload | Fuentes |
| :--- | :--- | :--- | :--- | :--- |
| **LoginExistingChar** | `ClientPacketID.LoginExistingChar` | Envío del form de login | `Name (String)`, `Password (String)`, `Ver (3 Bytes)`, `MD5 (FixedString)` | Cliente: `Protocol.bas`, `WriteLoginExistingChar`<br>Servidor: `Protocol.bas`, `HandleLoginExistingChar` |
| **LoginNewChar** | `ClientPacketID.LoginNewChar` | Crear personaje nuevo | `Name`, `Pass`, `Ver (3B)`, `Race`, `Gender`, `Class`, `Head`, `Mail`, `City` | Cliente: `Protocol.bas`, `WriteLoginNewChar`<br>Servidor: `Protocol.bas`, `HandleLoginNewChar` |
| **Walk** | `ClientPacketID.Walk` | Flechas de movimiento | `Heading (Byte)` *(1=Norte, 2=Este, 3=Sur, 4=Oeste)* | Cliente: `Protocol.bas`, `WriteWalk`<br>Servidor: `Protocol.bas`, `HandleWalk` |
| **Attack** | `ClientPacketID.Attack` | Tecla CTRL | *Ninguno* (Solo byte de opcode) | Cliente: `Protocol.bas`, `WriteAttack`<br>Servidor: `Protocol.bas`, `HandleAttack` |
| **PickUp** | `ClientPacketID.PickUp` | Tecla `Q` / `A` | *Ninguno* | Cliente: `Protocol.bas`, `WritePickUp`<br>Servidor: `Protocol.bas`, `HandlePickUp` |
| **UseItem** | `ClientPacketID.UseItem` | Doble clic en ítem | `Slot (Byte)` | Cliente: `Protocol.bas`, `WriteUseItem`<br>Servidor: `Protocol.bas`, `HandleUseItem` |
| **CastSpell** | `ClientPacketID.CastSpell` | Tecla de lanzar hechizo (`U`) | `SpellSlot (Byte)` | Cliente: `Protocol.bas`, `WriteCastSpell`<br>Servidor: `Protocol.bas`, `HandleCastSpell` |
| **LeftClick** | `ClientPacketID.LeftClick` | Clic en mapa | `X (Byte)`, `Y (Byte)` | Cliente: `Protocol.bas`, `WriteLeftClick`<br>Servidor: `Protocol.bas`, `HandleLeftClick` |

### Opcodes Servidor a Cliente (`ServerPacketID`)

| Mensaje | Opcode ID | Acción Disparadora | Estructura de Payload | Fuentes |
| :--- | :--- | :--- | :--- | :--- |
| **PosUpdate** | `ServerPacketID.PosUpdate` | Corrección de posición | `X (Byte)`, `Y (Byte)` | Servidor: `Protocol.bas`, `WritePosUpdate`<br>Cliente: `ProtocolCmdParse.bas`, `HandlePosUpdate` |
| **CharacterMove** | `ServerPacketID.CharacterMove` | Movimiento de entidad | `CharIndex (Integer)`, `X (Byte)`, `Y (Byte)` | Servidor: `Protocol.bas`, `WriteCharacterMove`<br>Cliente: `ProtocolCmdParse.bas`, `HandleCharacterMove` |
| **CharacterCreate** | `ServerPacketID.CharacterCreate` | Entidad entra en visión | `CharIndex`, `Body`, `Head`, `Heading`, `X`, `Y`, `Weapon`, `Shield`, `Helmet` | Servidor: `Protocol.bas`, `WriteCharacterCreate`<br>Cliente: `ProtocolCmdParse.bas`, `HandleCharacterCreate` |
| **ConsoleMsg** | `ServerPacketID.ConsoleMsg` | Texto a consola | `Message (ASCIIString)`, `FontType (Byte)` | Servidor: `Protocol.bas`, `WriteConsoleMsg`<br>Cliente: `ProtocolCmdParse.bas`, `HandleConsoleMsg` |
| **UpdateHP** | `ServerPacketID.UpdateHP` | Cambio de vida | `CurrentHP (Integer)` | Servidor: `Protocol.bas`, `WriteUpdateHP`<br>Cliente: `ProtocolCmdParse.bas`, `HandleUpdateHP` |

## Preguntas Abiertas
- Ciertos paquetes utilizan strings de longitud fija legacy (`WriteASCIIStringFixed(32)` vs `WriteASCIIString()`) según la directiva de compilación `#If SeguridadAlkon`; se debe respetar exactamente dicho encuadre binario en C++.
