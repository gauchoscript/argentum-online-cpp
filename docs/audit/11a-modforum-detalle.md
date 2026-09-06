---
area: protocolo-de-red
source_files:
  - legacy/server/Codigo/modForum.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/Declares.bas
  - legacy/server/Codigo/Acciones.bas
tags: [auditoria, modforum, red, datos, foros, protocol, vb6, cpp]
last_updated: 2026-09-06
---

# Auditoría Profunda de Módulo de Foros: modForum.bas (Persistencia y Protocolo)

Este documento presenta la auditoría técnica detallada y la especificación a nivel de bytes del módulo `legacy/server/Codigo/modForum.bas`, cubriendo tanto el formato de persistencia en disco de archivos de foros (`.for`) como la especificación binaria de los paquetes de red asociados.

---

## Resumen

- **Formato de Archivos en Disco (`.for`)**: Los foros del juego se persisten en el directorio `App.Path & "\Foros\"` mediante dos categorías de archivos en texto plano ANSI (Windows-1252) con finales de línea CRLF (`\r\n`):
  1. **Archivo de Índice del Foro (`ID.for`)**: Archivo de formato INI estructurado con una sección `[INFO]` que registra la cantidad de mensajes generales (`CantMSG`) y de anuncios fijados (`CantAnuncios`).
  2. **Archivos de Publicaciones Individuales (`ID<N>.for` y `ID<N>a.for`)**: Archivos de texto secuencial de 3 líneas donde cada línea contiene en orden estricto el Título (`sTitulo`), el Autor (`Autor`) y el Contenido (`sPost`).
- **Protocolo de Red**: El subsistema utiliza tres paquetes binarios de red: `ClientPacketID.ForumPost` (ID `45`, Cliente $\to$ Servidor), `ServerPacketID.ShowForumForm` (ID `63`, Servidor $\to$ Cliente) y `ServerPacketID.AddForumMsg` (ID `62`, Servidor $\to$ Cliente).
- **Estado de Fixtures en Disco**: **No existen archivos `.for` reales en el repositorio ni en `tests/fixtures/`**. Se requiere crear una suite de generación de datos de prueba para foros (`tests/fixtures/foros/`) para validar los parsers C++ contra disco mediante **doctest**.

---

## Hallazgos

### 1. Auditoría del Formato de Archivos de Foros (`.for`)

- **Ubicación en Disco**: Los archivos de foros residen en la carpeta `App.Path & "\Foros\"` (por ejemplo, `legacy/server/Foros/`).
- **Patrón de Nombres de Archivos y Variantes**:
  - `sForoID.for`: Archivo de cabecera/índice en formato INI para un foro específico (ej. `Ullathorpe.for`, `REAL.for`, `CAOS.for` o el ID de clan de un foro privado).
  - `sForoID<N>.for`: Archivo secuencial del post general número $N$ ($1 \le N \le 30$). Ej: `Ullathorpe1.for`, `Ullathorpe2.for`.
  - `sForoID<N>a.for`: Archivo secuencial del anuncio/post fijado (*sticky*) número $N$ ($1 \le N \le 5$). Ej: `Ullathorpe1a.for`.
- **Límites Internos Estáticos**:
  - `MAX_MENSAJES_FORO = 30`: Máximo de 30 mensajes generales por foro.
  - `MAX_ANUNCIOS_FORO = 5`: Máximo de 5 anuncios fijados por foro.
- **Rutina de Guardado y Limpieza**: Al ejecutar `SaveForum`, el servidor invoca `CleanForum`, eliminando del disco todos los archivos `sForoID<N>.for` y `sForoID<N>a.for` existentes y el archivo índice `.for`, y reescribe secuencialmente el índice e ítems activos desde 1 hasta `CantPosts` y `CantAnuncios`.

### 2. Auditoría del Protocolo de Red de Foros

- **Interacción con el Mundo**: Cuando un personaje hace doble clic en un cartel o tableros de mensajes de foro en el mapa (`Acciones.bas`), el servidor responde enviando el paquete `ServerPacketID.ShowForumForm` con las banderas de visibilidad del jugador y permisos para fijar mensajes, seguido inmediatamente de la invocación a `modForum.SendPosts`, que envía cada mensaje registrado usando paquetes `ServerPacketID.AddForumMsg`.
- **Creación de Mensajes (`ForumPost`)**: Cuando el cliente envía un post desde la interfaz `frmForo.frm`, la petición llega al servidor mediante el paquete `ClientPacketID.ForumPost`. El procedimiento `HandleForumPost` desglosa la petición, valida la alineación de facción del jugador (`eForumAlignment`) y agrega la entrada a la estructura en memoria mediante `AddPost`.

---

## Lógica y Datos Extraídos

### Estructura en Disco de Archivos `.for`

#### 1. Archivo de Índice (`<ID>.for`)
Format: Archivo de texto estructurado INI (Windows-1252 / CRLF).

```ini
[INFO]
CantMSG=<CantPosts>
CantAnuncios=<CantAnuncios>
```

#### 2. Archivo de Post General (`<ID><N>.for`) y Anuncio (`<ID><N>a.for`)
Format: Archivo de texto plano secuencial (Windows-1252 / CRLF), 3 líneas exactas.

```text
<Título del mensaje>
<Nickname del Autor>
<Cuerpo/Contenido del mensaje>
```

---

### Especificación Byte a Byte del Protocolo de Red de Foros

#### 1. `ClientPacketID.ForumPost` (Cliente $\to$ Servidor)
- **ID Opcode**: `45` (`0x2D`)
- **Despachador Servidor**: `Protocol.bas:HandleForumPost`
- **Estructura del Payload**:

| Campo | Tipo C++ | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `PacketID` | `uint8_t` | 1 B | N/A | Valor constante `45` (`ClientPacketID.ForumPost`). |
| `ForumMsgType` | `uint8_t` | 1 B | N/A | Enum `eForumMsgType`: `0` (`ieGeneral`), `1` (`ieGENERAL_STICKY`), `2` (`ieREAL`), `3` (`ieREAL_STICKY`), `4` (`ieCAOS`), `5` (`ieCAOS_STICKY`). |
| `Title` | `std::string` | 2 B + Var | Little-Endian | Prefijo de longitud de 2 bytes (`uint16_t`) seguido de los caracteres ASCII del título. |
| `Post` | `std::string` | 2 B + Var | Little-Endian | Prefijo de longitud de 2 bytes (`uint16_t`) seguido de los caracteres ASCII del contenido. |

#### 2. `ServerPacketID.ShowForumForm` (Servidor $\to$ Cliente)
- **ID Opcode**: `63` (`0x3F`)
- **Despachador Servidor**: `Protocol.bas:WriteShowForumForm`
- **Estructura del Payload**:

| Campo | Tipo C++ | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `PacketID` | `uint8_t` | 1 B | N/A | Valor constante `63` (`ServerPacketID.ShowForumForm`). |
| `Visibilidad` | `uint8_t` | 1 B | N/A | Máscara de bits `eForumVisibility`: `0x01` (`ieGENERAL_MEMBER`), `0x02` (`ieREAL_MEMBER` si es Armada Real o GM), `0x04` (`ieCAOS_MEMBER` si es Fuerzas del Caos o GM). |
| `CanMakeSticky` | `uint8_t` | 1 B | N/A | Nivel de permiso para fijar mensajes: `0` (usuario común), `1` (miembro del Consejo Real/Caos), `2` (Game Master). |

#### 3. `ServerPacketID.AddForumMsg` (Servidor $\to$ Cliente)
- **ID Opcode**: `62` (`0x3E`)
- **Despachador Servidor**: `Protocol.bas:WriteAddForumMsg`
- **Estructura del Payload**:

| Campo | Tipo C++ | Tamaño (Bytes) | Endianness | Descripción |
| :--- | :--- | :---: | :---: | :--- |
| `PacketID` | `uint8_t` | 1 B | N/A | Valor constante `62` (`ServerPacketID.AddForumMsg`). |
| `ForumType` | `uint8_t` | 1 B | N/A | Tipo/Sección del mensaje (`eForumType` / `eForumMsgType`). |
| `Title` | `std::string` | 2 B + Var | Little-Endian | Prefijo de longitud de 2 bytes (`uint16_t`) seguido de los caracteres ASCII del título. |
| `Author` | `std::string` | 2 B + Var | Little-Endian | Prefijo de longitud de 2 bytes (`uint16_t`) seguido del nickname del autor. |
| `Message` | `std::string` | 2 B + Var | Little-Endian | Prefijo de longitud de 2 bytes (`uint16_t`) seguido del texto del post. |

---

## Preguntas Abiertas

- **Soporte de Salto de Línea en Mensajes Multilínea**: En VB6, `Input #` lee hasta la primera ocurrencia de retorno de carro/salto de línea. Si un jugador envía un mensaje de foro con saltos de línea `\r\n`, la lectura VB6 se trunca en la primera línea. En C++, el parser debe definir explícitamente si se sanitizan las cadenas reemplazando `\r\n` por un carácter especial (ej. `~` o `\n`) antes de guardar a disco.
- **Generación de Fixtures en Disco**: Dado que no existen archivos `.for` de muestra en la distribución original, debe crearse un script generador en `tests/generate_fixtures.ps1` que cree fixtures representativas en `tests/fixtures/foros/` para su verificación unitaria con **doctest**.
