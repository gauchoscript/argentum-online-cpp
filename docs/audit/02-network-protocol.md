---
area: network-protocol
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
tags: [networking, protocol, winsock, packets, serialization]
last_updated: 2026-09-05
---

## Summary
The network protocol is a custom binary stream protocol managed by FIFO byte queues (`clsByteQueue`). Opcodes are serialized as single-byte values (`ClientPacketID` and `ServerPacketID`) followed by typed data arguments.

## Findings
Socket I/O is abstracted across Winsock controls and Win32 C-style Winsock API calls.

- **Client Socket Transmission**:
  - `ConnectToMainServer` opens socket connection to host IP/Port via `frmConnect.Winsock1.Connect`.
  > Source: `legacy/client/CODIGO/TCP.bas`, function `ConnectToMainServer`
  - Incoming byte stream is captured by `Listening` and pushed into `incomingData` queue.
  > Source: `legacy/client/CODIGO/TCP.bas`, function `Listening`
  - Outgoing packet buffer `outgoingData` is serialized by `Protocol.bas` and flushed over socket via `FlushBuffer`.
  > Source: `legacy/client/CODIGO/TCP.bas`, function `FlushBuffer`

- **Server Socket Engine**:
  - `AcceptNewUser` accepts client connections via `wskapiAO.accept`.
  > Source: `legacy/server/Codigo/TCP.bas`, function `AcceptNewUser`
  - Socket events are processed asynchronously by `SocketWindowProc` handling `FD_READ`, `FD_WRITE`, `FD_CLOSE`.
  > Source: `legacy/server/Codigo/wsksock.bas`, function `SocketWindowProc`
  - Incoming stream is processed by `HandleIncomingData` which reads opcode byte (`ClientPacketID`) and dispatches to handler routines.
  > Source: `legacy/server/Codigo/Protocol.bas`, function `HandleIncomingData`

## Extracted Logic / Data

### Client-to-Server Opcodes (`ClientPacketID`)

| Command Name | Opcode ID | Trigger / Action | Payload Data Structure | Source Citations |
| :--- | :--- | :--- | :--- | :--- |
| **LoginExistingChar** | `ClientPacketID.LoginExistingChar` | Login form submission | `Name (String)`, `Password (String)`, `Ver (3 Bytes)`, `MD5 (FixedString)` | Client: `Protocol.bas`, `WriteLoginExistingChar`<br>Server: `Protocol.bas`, `HandleLoginExistingChar` |
| **LoginNewChar** | `ClientPacketID.LoginNewChar` | Character creation form | `Name`, `Pass`, `Ver (3B)`, `Race`, `Gender`, `Class`, `Head`, `Mail`, `City` | Client: `Protocol.bas`, `WriteLoginNewChar`<br>Server: `Protocol.bas`, `HandleLoginNewChar` |
| **Walk** | `ClientPacketID.Walk` | Arrow key press | `Heading (Byte)` *(1=N, 2=E, 3=S, 4=W)* | Client: `Protocol.bas`, `WriteWalk`<br>Server: `Protocol.bas`, `HandleWalk` |
| **Attack** | `ClientPacketID.Attack` | CTRL key press | *None* (Opcode byte only) | Client: `Protocol.bas`, `WriteAttack`<br>Server: `Protocol.bas`, `HandleAttack` |
| **PickUp** | `ClientPacketID.PickUp` | `Q` / `A` key press | *None* | Client: `Protocol.bas`, `WritePickUp`<br>Server: `Protocol.bas`, `HandlePickUp` |
| **UseItem** | `ClientPacketID.UseItem` | Double click item | `Slot (Byte)` | Client: `Protocol.bas`, `WriteUseItem`<br>Server: `Protocol.bas`, `HandleUseItem` |
| **CastSpell** | `ClientPacketID.CastSpell` | Launch spell key (`U`) | `SpellSlot (Byte)` | Client: `Protocol.bas`, `WriteCastSpell`<br>Server: `Protocol.bas`, `HandleCastSpell` |
| **LeftClick** | `ClientPacketID.LeftClick` | Click map tile | `X (Byte)`, `Y (Byte)` | Client: `Protocol.bas`, `WriteLeftClick`<br>Server: `Protocol.bas`, `HandleLeftClick` |

### Server-to-Client Opcodes (`ServerPacketID`)

| Message Name | Opcode ID | Trigger / Action | Payload Data Structure | Source Citations |
| :--- | :--- | :--- | :--- | :--- |
| **PosUpdate** | `ServerPacketID.PosUpdate` | Server position sync | `X (Byte)`, `Y (Byte)` | Server: `Protocol.bas`, `WritePosUpdate`<br>Client: `ProtocolCmdParse.bas`, `HandlePosUpdate` |
| **CharacterMove** | `ServerPacketID.CharacterMove` | Entity moves on grid | `CharIndex (Integer)`, `X (Byte)`, `Y (Byte)` | Server: `Protocol.bas`, `WriteCharacterMove`<br>Client: `ProtocolCmdParse.bas`, `HandleCharacterMove` |
| **CharacterCreate** | `ServerPacketID.CharacterCreate` | Entity enters viewport | `CharIndex`, `Body`, `Head`, `Heading`, `X`, `Y`, `Weapon`, `Shield`, `Helmet` | Server: `Protocol.bas`, `WriteCharacterCreate`<br>Client: `ProtocolCmdParse.bas`, `HandleCharacterCreate` |
| **ConsoleMsg** | `ServerPacketID.ConsoleMsg` | Server chat message | `Message (ASCIIString)`, `FontType (Byte)` | Server: `Protocol.bas`, `WriteConsoleMsg`<br>Client: `ProtocolCmdParse.bas`, `HandleConsoleMsg` |
| **UpdateHP** | `ServerPacketID.UpdateHP` | Target HP changes | `CurrentHP (Integer)` | Server: `Protocol.bas`, `WriteUpdateHP`<br>Client: `ProtocolCmdParse.bas`, `HandleUpdateHP` |

## Open Questions
- Some packets retain legacy fixed-size strings (`WriteASCIIStringFixed(32)` vs `WriteASCIIString()`) depending on `#If SeguridadAlkon` conditional compilation flags; need to ensure fixed binary stream framing during C++ migration.
