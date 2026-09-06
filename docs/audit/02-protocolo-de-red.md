---
area: protocolo-de-red
source_files:
  - legacy/client/Client.vbp
  - legacy/client/CODIGO/Declares.bas
  - legacy/client/CODIGO/Protocol.bas
  - legacy/client/CODIGO/ProtocolCmdParse.bas
  - legacy/client/CODIGO/TCP.bas
  - legacy/client/CODIGO/clsByteQueue.cls
  - legacy/server/Codigo/Declares.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/SecurityIp.bas
  - legacy/server/Codigo/TCP.bas
  - legacy/server/Codigo/clsByteQueue.cls
  - legacy/server/Codigo/modSendData.bas
  - legacy/server/Codigo/wskapiAO.bas
  - legacy/server/Codigo/wsksock.bas
tags: [red, protocolo, winsock, paquetes, serializacion, binario, layout, fuente-solo]
last_updated: 2026-09-06
---

## Resumen
El protocolo de red de Argentum Online v0.13.0 es un protocolo binario orientado a streams TCP sin delimitadores de fin de mensaje (como `\r\n` o bytes nulos). La serialización y el parseo son gestionados en ambos extremos por una cola circular/FIFO de bytes (`clsByteQueue`). Cada mensaje se compone de un identificador de paquete (opcode) de 1 byte (`ClientPacketID` o `ServerPacketID`) seguido inmediatamente por sus argumentos serializados de forma contigua.

> [!IMPORTANT]
> **Estado de Verificación de Datos (Source-Only / Not Data-Verified)**:
> La totalidad de las especificaciones de paquetes, opcodes, encuadre y serialización documentadas en esta auditoría han sido obtenidas **única y exclusivamente mediante la inspección del código fuente VB6**. No se cuenta aún con archivos de fixture ni capturas de tráfico `.pcap` para validación empírica en red, por lo que todos los layouts se clasifican como **inferidos del código fuente (source-only)**.

---

## Hallazgos

### 1. Representación Cruda de Tipos Numéricos en Memoria (Raw VB6 Memory)
La clase `clsByteQueue` emplea llamadas directas a la API de Windows `RtlMoveMemory` (`CopyMemory`) para copiar tipos de datos desde y hacia su buffer interno `data() As Byte`. Por lo tanto, todos los campos numéricos se transmiten exactamente con la **representación nativa de memoria de VB6 en arquitecturas x86 (Little-Endian)**:

- **`Byte`**: 1 byte sin signo (`uint8_t`), valores de 0 a 255.
- **`Integer`**: 2 bytes con signo en complemento a dos, Little-Endian (`int16_t`), valores de -32.768 a 32.767.
- **`Long`**: 4 bytes con signo en complemento a dos, Little-Endian (`int32_t`), valores de -2.147.483.648 a 2.147.483.647.
- **`Single`**: 4 bytes en punto flotante estándar IEEE-754 de precisión simple Little-Endian (`float`).
- **`Double`**: 8 bytes en punto flotante estándar IEEE-754 de precisión doble Little-Endian (`double`).
- **`Boolean`**: Aunque en la memoria interna de VB6 un `Boolean` ocupa 2 bytes (`0xFFFF` para True y `0x0000` para False), en la red `clsByteQueue.WriteBoolean` lo empaqueta obligatoriamente como **1 byte (`uint8_t`)**:
  - `0x01` para `True`.
  - `0x00` para `False`.

### 2. Codificación y Terminación de Cadenas de Caracteres (`Strings`)
- **Página de códigos (Codepage)**: Las cadenas no se transmiten en UTF-8 ni en UTF-16. VB6 almacena internamente strings BSTR en UTF-16LE, pero antes de enviarlas al socket invoca `StrConv(value, vbFromUnicode)`, convirtiéndolas a la página de códigos ANSI activa del sistema Windows en Argentina/Latinoamérica: **Windows-1252 (CP1252 / ISO-8859-1)**.
- **Cadenas de longitud variable (`WriteASCIIString` / `ReadASCIIString`)**:
  - Se transmiten con un **prefijo de longitud de 2 bytes** (`Integer` con signo Little-Endian, `int16_t`) que especifica el número exacto de bytes $L$ de la cadena.
  - Inmediatamente a continuación se envían los $L$ bytes de texto codificados en Windows-1252.
  - **No contienen terminador nulo (`\0`)**.
- **Protocolo de Foros (`modForum.bas`)**: Para la especificación byte a byte de los opcodes de red relacionados con el sistema de foros (`ClientPacketID.ForumPost` [ID 45], `ServerPacketID.ShowForumForm` [ID 63] y `ServerPacketID.AddForumMsg` [ID 62]), consultá la auditoría detallada en [11a-modforum-detalle.md](11a-modforum-detalle.md).
  - Si la cadena está vacía (`""`), el prefijo de longitud es `0x0000` (2 bytes) y no se transmite ningún byte adicional.
- **Cadenas de longitud fija (`WriteASCIIStringFixed` / `ReadASCIIStringFixed`)**:
  - Se transmiten como una secuencia pura de $N$ bytes (según la constante fija definida en el mensaje).
  - **No tienen prefijo de longitud y no tienen terminador nulo**.
  - Si el contenido es menor al tamaño fijo, el emisor rellena o trunca según la lógica del emisor en VB6.

### 3. Mecanismo de Encuadre y Límites de Mensajes (Message Framing)
Dado que TCP es un flujo continuo de bytes y los mensajes no poseen bytes centinela ni delimitadores:
- **Mensajes de Tamaño Fijo**:
  - El handler inspecciona la cola: `If incomingData.length < TAMANIO_FIJO Then Err.Raise NOT_ENOUGH_DATA: Exit Sub`.
  - Si no hay suficientes bytes recibidos en el buffer del socket, la función aborta silenciosamente sin extraer ningún byte y espera la siguiente ráfaga TCP.
  - Si hay suficientes datos, extrae el opcode con `ReadByte()` y consume directamente los campos con `ReadInteger()`, `ReadByte()`, etc.
- **Mensajes de Tamaño Variable (con Cadenas de Caracteres)**:
  - Para evitar pérdida de datos o corrupción ante paquetes partidos en varios segmentos TCP, el handler hace una copia transaccional de la cola: `buffer.CopyBuffer(incomingData)`.
  - Intenta leer el opcode y todos los campos del paquete desde `buffer`.
  - Si durante la lectura de cualquier cadena o campo salta el error `NOT_ENOUGH_DATA` (porque aún no llegó el payload completo), el bloque de manejo de errores captura la excepción, aborta el procedimiento y **deja la cola principal `incomingData` inalterada**.
  - Si la lectura concluye con éxito, sincroniza el estado: `incomingData.CopyBuffer(buffer)`, consumiendo de forma atómica exactamente los bytes que componen el paquete procesado.

---

## Lógica y Datos Extraídos

> [!WARNING]
> Todos los layouts a continuación han sido auditados directamente del código fuente (`legacy/client/CODIGO/Protocol.bas` y `legacy/server/Codigo/Protocol.bas`). **Estado: Source-Only / Not Data-Verified**.

A continuación se documenta el layout binario exacto a nivel de bytes para los mensajes representativos del protocolo.

### Opcodes Cliente a Servidor (`ClientPacketID`)

#### 1. `LoginExistingChar` (Opcode: `ClientPacketID.LoginExistingChar = 1`)
- **Tipo de Tamaño**: Dinámico (Prefijado por longitud de strings).
- **Manejador Servidor**: `Protocol.bas`, `HandleLoginExistingChar`.
- **Emisor Cliente**: `Protocol.bas`, `WriteLoginExistingChar`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Identificador de paquete (`0x01`). |
| `UserName_Len` | `int16_t` | 2 | Little-Endian | Cantidad de bytes $L_1$ del nombre de usuario. |
| `UserName_Data` | `char[L1]` | $L_1$ | - | Nombre de usuario en Windows-1252 (sin `\0`). |
| `Password_Len` * | `int16_t` | 2 | Little-Endian | Longitud $L_2$ de la contraseña (solo si `SeguridadAlkon = 0`). |
| `Password_Data` * | `char[L2]` | $L_2$ / 32 | - | Si `SeguridadAlkon = 0`: $L_2$ bytes ANSI.<br>Si `SeguridadAlkon = 1`: 32 bytes fijos de hash MD5. |
| `AppMajor` | `uint8_t` | 1 | - | Versión mayor del cliente (`App.Major`). |
| `AppMinor` | `uint8_t` | 1 | - | Versión menor del cliente (`App.Minor`). |
| `AppRevision` | `uint8_t` | 1 | - | Número de revisión del cliente (`App.Revision`). |
| `MD5_Client` * | `char[16]` | 0 / 16 | - | Presente únicamente si `SeguridadAlkon = 1` (16 bytes fijos). |

* *Nota sobre tamaño mínimo*: Con `SeguridadAlkon = 0`, tamaño mínimo absoluto es de 8 bytes (Opcode 1B + Len 2B + 0B + Len 2B + 0B + Ver 3B). Con `SeguridadAlkon = 1`, tamaño mínimo es de 53 bytes.

---

#### 2. `LoginNewChar` (Opcode: `ClientPacketID.LoginNewChar = 2`)
- **Tipo de Tamaño**: Dinámico.
- **Manejador Servidor**: `Protocol.bas`, `HandleLoginNewChar`.
- **Emisor Cliente**: `Protocol.bas`, `WriteLoginNewChar`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Identificador de paquete (`0x02`). |
| `UserName_Len` | `int16_t` | 2 | Little-Endian | Longitud $L_1$ del nombre del nuevo personaje. |
| `UserName_Data` | `char[L1]` | $L_1$ | - | Nombre en texto Windows-1252. |
| `Password_Len` * | `int16_t` | 2 | Little-Endian | Longitud $L_2$ de contraseña (solo si `SeguridadAlkon = 0`). |
| `Password_Data` * | `char[L2]` | $L_2$ / 32 | - | Texto o hash MD5 (32 bytes fijos si `SeguridadAlkon = 1`). |
| `AppMajor` | `uint8_t` | 1 | - | Versión mayor. |
| `AppMinor` | `uint8_t` | 1 | - | Versión menor. |
| `AppRevision` | `uint8_t` | 1 | - | Revisión. |
| `MD5_Client` * | `char[16]` | 0 / 16 | - | Solo con `SeguridadAlkon = 1`. |
| `UserRaza` | `uint8_t` | 1 | - | Enum de raza (`eRaza`: 1=Humano, 2=Elfo, etc.). |
| `UserSexo` | `uint8_t` | 1 | - | Enum de género (`eGenero`: 1=Hombre, 2=Mujer). |
| `UserClase` | `uint8_t` | 1 | - | Enum de clase (`eClase`: 1=Mago, 2=Clérigo, etc.). |
| `UserHead` | `int16_t` | 2 | Little-Endian | Índice de cabeza visual elegida. |
| `UserEmail_Len` | `int16_t` | 2 | Little-Endian | Longitud $L_3$ del correo electrónico. |
| `UserEmail_Data`| `char[L3]` | $L_3$ | - | Dirección de email en Windows-1252. |
| `UserHogar` | `uint8_t` | 1 | - | Índice de la ciudad de origen (`eCiudad`). |

---

#### 3. `Walk` (Opcode: `ClientPacketID.Walk = 6`)
- **Tipo de Tamaño**: **Fijo (2 bytes)**.
- **Manejador Servidor**: `Protocol.bas`, `HandleWalk`.
- **Emisor Cliente**: `Protocol.bas`, `WriteWalk`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Valor constante `0x06`. |
| `Heading` | `uint8_t` | 1 | - | Dirección cardinal: 1=Norte, 2=Este, 3=Sur, 4=Oeste. |

---

#### 4. `Attack` (Opcode: `ClientPacketID.Attack = 8`)
- **Tipo de Tamaño**: **Fijo (1 byte)**.
- **Manejador Servidor**: `Protocol.bas`, `HandleAttack`.
- **Emisor Cliente**: `Protocol.bas`, `WriteAttack`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Valor constante `0x08`. Sin payload adicional. |

---

#### 5. `PickUp` (Opcode: `ClientPacketID.PickUp = 9`)
- **Tipo de Tamaño**: **Fijo (1 byte)**.
- **Manejador Servidor**: `Protocol.bas`, `HandlePickUp`.
- **Emisor Cliente**: `Protocol.bas`, `WritePickUp`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Valor constante `0x09`. Sin payload adicional. |

---

#### 6. `UseItem` (Opcode: `ClientPacketID.UseItem = 19`)
- **Tipo de Tamaño**: **Fijo (2 bytes)**.
- **Manejador Servidor**: `Protocol.bas`, `HandleUseItem`.
- **Emisor Cliente**: `Protocol.bas`, `WriteUseItem`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Valor constante `0x13`. |
| `Slot` | `uint8_t` | 1 | - | Número de slot de inventario (1 a 30). |

---

#### 7. `CastSpell` (Opcode: `ClientPacketID.CastSpell = 15`)
- **Tipo de Tamaño**: **Fijo (2 bytes)**.
- **Manejador Servidor**: `Protocol.bas`, `HandleCastSpell`.
- **Emisor Cliente**: `Protocol.bas`, `WriteCastSpell`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Valor constante `0x0F`. |
| `SpellSlot` | `uint8_t` | 1 | - | Número de slot de hechizo seleccionado (1 a 35). |

---

#### 8. `LeftClick` (Opcode: `ClientPacketID.LeftClick = 16`)
- **Tipo de Tamaño**: **Fijo (3 bytes)**.
- **Manejador Servidor**: `Protocol.bas`, `HandleLeftClick`.
- **Emisor Cliente**: `Protocol.bas`, `WriteLeftClick`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Valor constante `0x10`. |
| `X` | `uint8_t` | 1 | - | Coordenada X del mapa clickeada (1 a 100). |
| `Y` | `uint8_t` | 1 | - | Coordenada Y del mapa clickeada (1 a 100). |

---

### Opcodes Servidor a Cliente (`ServerPacketID`)

#### 9. `PosUpdate` (Opcode: `ServerPacketID.PosUpdate = 3`)
- **Tipo de Tamaño**: **Fijo (3 bytes)**.
- **Manejador Cliente**: `legacy/client/CODIGO/Protocol.bas`, `HandlePosUpdate`.
- **Emisor Servidor**: `legacy/server/Codigo/Protocol.bas`, `WritePosUpdate`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Valor constante `0x03`. |
| `X` | `uint8_t` | 1 | - | Coordenada X real y validada en el servidor (1 a 100). |
| `Y` | `uint8_t` | 1 | - | Coordenada Y real y validada en el servidor (1 a 100). |

---

#### 10. `CharacterMove` (Opcode: `ServerPacketID.CharacterMove = 13`)
- **Tipo de Tamaño**: **Fijo (5 bytes)**.
- **Manejador Cliente**: `legacy/client/CODIGO/Protocol.bas`, `HandleCharacterMove`.
- **Emisor Servidor**: `legacy/server/Codigo/Protocol.bas`, `WriteCharacterMove` / `PrepareMessageCharacterMove`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Valor constante `0x0D`. |
| `CharIndex` | `int16_t` | 2 | Little-Endian | Índice del personaje en la lista del cliente (`charlist`). |
| `X` | `uint8_t` | 1 | - | Nueva coordenada X de destino de la entidad. |
| `Y` | `uint8_t` | 1 | - | Nueva coordenada Y de destino de la entidad. |

---

#### 11. `CharacterCreate` (Opcode: `ServerPacketID.CharacterCreate = 10`)
- **Tipo de Tamaño**: Dinámico (Prefijado por longitud del nombre).
- **Manejador Cliente**: `legacy/client/CODIGO/Protocol.bas`, `HandleCharacterCreate`.
- **Emisor Servidor**: `legacy/server/Codigo/Protocol.bas`, `PrepareMessageCharacterCreate`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Valor constante `0x0A`. |
| `CharIndex` | `int16_t` | 2 | Little-Endian | Índice local asignado al personaje en el cliente. |
| `Body` | `int16_t` | 2 | Little-Endian | Índice del cuerpo/gráfico del personaje. |
| `Head` | `int16_t` | 2 | Little-Endian | Índice de cabeza visual. |
| `Heading` | `uint8_t` | 1 | - | Orientación (1=Norte, 2=Este, 3=Sur, 4=Oeste). |
| `X` | `uint8_t` | 1 | - | Posición inicial X en el mapa. |
| `Y` | `uint8_t` | 1 | - | Posición inicial Y en el mapa. |
| `Weapon` | `int16_t` | 2 | Little-Endian | Gráfico de arma equipada (o 2 si no tiene). |
| `Shield` | `int16_t` | 2 | Little-Endian | Gráfico de escudo equipado (o 2 si no tiene). |
| `Helmet` | `int16_t` | 2 | Little-Endian | Gráfico de casco equipado (o 2 si no tiene). |
| `FX` | `int16_t` | 2 | Little-Endian | Índice de efecto visual activo (animación de hechizo, etc.). |
| `FXLoops` | `int16_t` | 2 | Little-Endian | Cantidad de repeticiones del efecto FX. |
| `Name_Len` | `int16_t` | 2 | Little-Endian | Longitud $L$ del nombre del personaje visible. |
| `Name_Data` | `char[L]` | $L$ | - | Nombre en texto Windows-1252. |
| `NickColor` | `uint8_t` | 1 | - | Color del nick (según alineación o estado criminal). |
| `Privileges`| `uint8_t` | 1 | - | Rango/privilegios de GM o usuario estándar. |

---

#### 12. `ConsoleMsg` (Opcode: `ServerPacketID.ConsoleMsg = 5`)
- **Tipo de Tamaño**: Dinámico (Prefijado por longitud del texto).
- **Manejador Cliente**: `legacy/client/CODIGO/Protocol.bas`, `HandleConsoleMessage`.
- **Emisor Servidor**: `legacy/server/Codigo/Protocol.bas`, `WriteConsoleMsg` / `PrepareMessageConsoleMsg`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Valor constante `0x05`. |
| `Chat_Len` | `int16_t` | 2 | Little-Endian | Longitud $L$ del mensaje de consola a imprimir. |
| `Chat_Data` | `char[L]` | $L$ | - | Texto del mensaje en codificación Windows-1252. |
| `FontIndex` | `uint8_t` | 1 | - | Enum de color y tipografía (`FontTypeNames`). |

---

#### 13. `UpdateHP` (Opcode: `ServerPacketID.UpdateHP = 20`)
- **Tipo de Tamaño**: **Fijo (3 bytes)**.
- **Manejador Cliente**: `legacy/client/CODIGO/Protocol.bas`, `HandleUpdateHP`.
- **Emisor Servidor**: `legacy/server/Codigo/Protocol.bas`, `WriteUpdateHP`.
- **Estado de Verificación**: Inferido de código fuente (source-only).
- **Estructura Binaria en el Socket**:

| Campo | Tipo C++ equivalente | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `Opcode` | `uint8_t` | 1 | - | Valor constante `0x14`. |
| `CurrentHP` | `int16_t` | 2 | Little-Endian | Puntos de vida actuales del jugador (`Stats.MinHp`). |

---

## Preguntas Abiertas
- **Generación de Fixtures Binarias de Red**: Para elevar el nivel de madurez del protocolo de `source-only` a `data-verified`, se requiere capturar sesiones TCP o implementar un generador de paquetes en los tests C++ (standalone Asio / doctest) que aserte los bytes exactos recibidos ante cada comando.
- **Directiva `#If SeguridadAlkon`**: En el archivo de proyecto `legacy/client/Client.vbp` la constante condicional `SeguridadAlkon` no está definida dentro del parámetro `CondComp`, por lo que el compilador VB6 la evalúa por omisión como `0` (False). No obstante, el servidor legacy posee código compilado condicionalmente que espera hashes MD5 de 16 y 32 bytes. En C++, la implementación de red debe parametrizar esta bandera para garantizar compatibilidad con binarios oficiales según la versión requerida.
- **Ubicación de los Manejadores de Paquetes en el Cliente**: La tabla original de la auditoría indicaba erróneamente que los handlers del cliente residían en `ProtocolCmdParse.bas`. Se comprobó que `ProtocolCmdParse.bas` sólo procesa comandos de texto tipeados por el usuario (e.g. `/online`, `/whisper`), mientras que la recepción, decodificación binaria y despacho de paquetes del socket reside íntegramente en `legacy/client/CODIGO/Protocol.bas`.
