---
area: formatos-de-datos
source_files:
  - legacy/client/CODIGO/Declares.bas
  - legacy/client/CODIGO/GameIni.bas
  - legacy/client/CODIGO/General.bas
  - legacy/client/CODIGO/TileEngine.bas
  - legacy/server/Codigo/Declares.bas
  - legacy/server/Codigo/FileIO.bas
  - legacy/server/Codigo/General.bas
  - legacy/server/Codigo/modGuilds.bas
tags: [datos, formatos, ini, chr, map, dat, binario, structs, padding]
last_updated: 2026-09-06
---

## Resumen
Argentum Online v0.13.0 utiliza dos categorías fundamentales de persistencia de datos:
1. **Archivos Binarios Propietarios**: Formatos binarios en disco escritos y leídos mediante las instrucciones nativas `Put #` y `Get #` de VB6 (con empaquetado de structs de 1 byte sin padding, campos numéricos Little-Endian x86 y cadenas de tamaño fijo). Se utilizan en mapas (`.map`), triggers de servidor (`.inf`) y tablas de índices gráficos del cliente (`.ind` y `.con`).
2. **Archivos de Texto Estructurado INI**: Archivos de texto plano codificados en Windows-1252 (ANSI) organizados en secciones `[SECCION]` y pares `Clave=Valor`. Se utilizan para la persistencia de personajes (`.chr`), datos de clanes (`.guild`, `.inf`, `.mem`, `.sol`) y configuraciones estáticas del juego (`OBJ.dat`, `NPCs.dat`, `Hechizos.dat`, `Balance.dat`).

---

## Hallazgos

### 1. Convenciones de Empaquetamiento y Representación Binaria en Disco
Al utilizar `Put #` y `Get #` sobre tipos definidos por el usuario (`Type ... End Type`), VB6 vuelca la memoria directamente al archivo sin compresión ni serialización abstracta:
- **Alineación de memoria (Packing)**: En disco, las estructuras no poseen padding entre campos (`#pragma pack(push, 1)` en C/C++).
- **Endianness**: Todos los enteros y números de punto flotante se almacenan en formato **Little-Endian** nativo de la arquitectura x86.
- **Tipos nativos de VB6**:
  - `Byte`: `uint8_t` (1 byte).
  - `Integer`: `int16_t` (2 bytes, complemento a dos Little-Endian).
  - `Long`: `int32_t` (4 bytes, complemento a dos Little-Endian).
  - `Single`: `float` (4 bytes, formato IEEE-754 Little-Endian).
  - `Double`: `double` (8 bytes, formato IEEE-754 Little-Endian).
  - `String * N` (Cadenas de longitud fija): Exactamente $N$ bytes de caracteres ANSI codificados en Windows-1252, sin prefijo de longitud y sin terminador nulo (`\0`).
  - `String` (Cadenas dinámicas dentro de un `Type`): VB6 escribe un prefijo de 2 bytes (`int16_t` Little-Endian) con la cantidad de caracteres, seguido de los bytes del texto.

### 2. Corrección sobre el Módulo de Mapas
La versión anterior de esta auditoría mencionaba un supuesto archivo `ModMapIO.bas`. Se verificó en el código fuente que dicho archivo no existe:
- En el **servidor**, la carga y persistencia binaria de mapas y triggers se realiza en `legacy/server/Codigo/FileIO.bas`, procedimiento `CargarMapa`.
- En el **cliente**, la lectura del archivo `.map` reside en `legacy/client/CODIGO/General.bas`, procedimiento `CargarMapa`.

---

## Lógica y Datos Extraídos

A continuación se detallan las estructuras exactas campo por campo y las especificaciones a nivel de byte necesarias para implementar parsers compatibles en C++.

### 1. Cabecera Estándar de Archivos Binarios (`tCabecera`)

Tanto los mapas binarios (`.map`) como los archivos de índices gráficos (`.ind`, `.con`) comienzan con el registro de firma `tCabecera` declarado en `Declares.bas`:

```cpp
#pragma pack(push, 1)
struct tCabecera {
    char desc[255];     // 255 bytes: Texto descriptivo / copyright en Windows-1252 (sin \0 garantizado)
    int32_t crc;        // 4 bytes: Checksum CRC (Long Little-Endian)
    int32_t magicWord;  // 4 bytes: Identificador de comprobación (Long Little-Endian)
};
#pragma pack(pop)
static_assert(sizeof(tCabecera) == 263, "tCabecera debe medir exactamente 263 bytes");
```

---

### 2. Formato de Archivos de Mapa (`.map` y `.inf`)

Los mapas representan cuadrículas fijas de **100x100 tiles** (índices $X \in [1, 100]$, $Y \in [1, 100]$).

#### 2.1. Archivo de Geometría y Capas Gráficas (`Mapa<N>.map`)
Compartido entre el cliente y el servidor. El orden de iteración en disco es por filas y luego columnas: primero todo el bucle $Y = 1 \dots 100$, y para cada $Y$ se itera $X = 1 \dots 100$.

- **Cabecera del archivo `.map`**:
  1. `MapVersion`: 2 bytes (`int16_t` Little-Endian).
  2. `MiCabecera`: 263 bytes (`tCabecera`).
  3. 4 campos enteros reservados (`tempint`): 4 $\times$ 2 bytes = 8 bytes (`int16_t`).
  - **Tamaño total de la cabecera**: $2 + 263 + 8 = \mathbf{273\text{ bytes}}$.

- **Registro de cada Tile ($10.000$ tiles por mapa)**:
  Los tiles tienen longitud variable condicionada por un byte de banderas (`ByFlags`):
  1. `ByFlags` (`uint8_t`, 1 byte):
     - Bit 0 (`0x01`): `Blocked` (1 = Bloqueado/Inaccesible, 0 = Transitable).
     - Bit 1 (`0x02`): Capa Gráfica 2 presente.
     - Bit 2 (`0x04`): Capa Gráfica 3 presente.
     - Bit 3 (`0x08`): Capa Gráfica 4 presente.
     - Bit 4 (`0x10`): Disparador (`Trigger`) presente.
  2. `Graphic(1)` (`int16_t`, 2 bytes): **Siempre presente**. GrhIndex de la capa base del suelo.
  3. `Graphic(2)` (`int16_t`, 2 bytes): **Presente solo si** `ByFlags & 0x02`.
  4. `Graphic(3)` (`int16_t`, 2 bytes): **Presente solo si** `ByFlags & 0x04`.
  5. `Graphic(4)` (`int16_t`, 2 bytes): **Presente solo si** `ByFlags & 0x08`.
  6. `Trigger` (`int16_t`, 2 bytes): **Presente solo si** `ByFlags & 0x10` (Triggers de mapa: zona segura, teletransporte, lava, etc.).

#### 2.2. Archivo de Triggers e Información de Servidor (`Mapa<N>.inf`)
Exclusivo del servidor dedicado (`legacy/server/Maps/`).

- **Cabecera del archivo `.inf`**:
  - 5 campos enteros reservados (`TempInt`): 5 $\times$ 2 bytes = $\mathbf{10\text{ bytes}}$ (`int16_t`).

- **Registro de cada Tile en `.inf` ($10.000$ tiles, orden $Y=1\dots 100, X=1\dots 100$)**:
  1. `ByFlags` (`uint8_t`, 1 byte):
     - Bit 0 (`0x01`): `TileExit` presente (transición entre mapas).
     - Bit 1 (`0x02`): `NpcIndex` presente (spawn inicial de NPC).
  2. Si `ByFlags & 0x01` está activo, siguen los datos de teletransporte:
     - `TileExit.Map`: `int16_t` (2 bytes, ID del mapa destino).
     - `TileExit.X`: `int16_t` (2 bytes, coordenada X destino).
     - `TileExit.Y`: `int16_t` (2 bytes, coordenada Y destino).
  3. Si `ByFlags & 0x02` está activo, siguen los datos del NPC:
     - `NpcIndex`: `int16_t` (2 bytes, índice del NPC según `NPCs.dat`).

---

### 3. Formato de Índices Gráficos del Cliente (`.ind`)

#### 3.1. Metadatos de Sprites y Animaciones (`Graficos3.ind` / `Graficos.ind`)
Ubicado en `legacy/client/INIT/`. Gestiona la tabla maestra `GrhData`.

- **Cabecera**:
  1. `fileVersion`: 4 bytes (`int32_t` Little-Endian).
  2. `grhCount`: 4 bytes (`int32_t` Little-Endian, cantidad de gráficos indexados).
- **Entradas secuenciales (hasta alcanzar EOF)**:
  1. `GrhIndex`: 4 bytes (`int32_t` Little-Endian).
  2. `NumFrames`: 2 bytes (`int16_t` Little-Endian).
  3. Si `NumFrames > 1` (Animación compuesta):
     - `Frames`: Array dinámico de `NumFrames` $\times$ 4 bytes (`int32_t` Little-Endian, cada uno referencia a un `GrhIndex` atómico).
     - `Speed`: 4 bytes (`float` IEEE-754 Little-Endian, velocidad de reproducción).
  4. Si `NumFrames == 1` (Sprite atómico recortado):
     - `FileNum`: 4 bytes (`int32_t` Little-Endian, número de mapa de bits `Graficos/<FileNum>.bmp`).
     - `sX`: 2 bytes (`int16_t` Little-Endian, coordenada X inicial del recorte dentro del BMP).
     - `sY`: 2 bytes (`int16_t` Little-Endian, coordenada Y inicial del recorte).
     - `pixelWidth`: 2 bytes (`int16_t` Little-Endian, ancho del sprite en píxeles).
     - `pixelHeight`: 2 bytes (`int16_t` Little-Endian, alto del sprite en píxeles).

#### 3.2. Cuerpos (`Cuerpos.ind`), Cabezas (`Cabezas.ind`) y Cascos (`Cascos.ind`)
Estructuras declaradas en `legacy/client/CODIGO/Declares.bas`:

```cpp
#pragma pack(push, 1)

// Utilizado en Cabezas.ind y Cascos.ind (exactamente 8 bytes por registro)
struct tIndiceCabeza {
    int16_t Head[4]; // GrhIndex en 4 direcciones cardinales (1=Norte, 2=Este, 3=Sur, 4=Oeste)
};

// Utilizado en Cuerpos.ind (exactamente 12 bytes por registro)
struct tIndiceCuerpo {
    int16_t Body[4];     // 8 bytes: GrhIndex de caminata en las 4 direcciones
    int16_t HeadOffsetX; // 2 bytes: Desplazamiento X del anclaje de la cabeza
    int16_t HeadOffsetY; // 2 bytes: Desplazamiento Y del anclaje de la cabeza
};

// Utilizado en Fxs.ind (exactamente 6 bytes por registro)
struct tIndiceFx {
    int16_t Animacion;   // 2 bytes: GrhIndex de la animación del efecto
    int16_t OffsetX;     // 2 bytes: Offset horizontal
    int16_t OffsetY;     // 2 bytes: Offset vertical
};

#pragma pack(pop)
```

- **Estructura en disco de estos archivos**:
  1. `MiCabecera`: 263 bytes (`tCabecera`).
  2. `Count`: 2 bytes (`int16_t`, cantidad de elementos).
  3. Array de `Count` elementos de la estructura correspondiente (`tIndiceCabeza`, `tIndiceCuerpo` o `tIndiceFx`).

#### 3.3. Tips (`Tips.dat`) y Lluvia (`Lluvia.dat`)
- **`Tips.dat`**:
  - `MiCabecera`: 263 bytes (`tCabecera`).
  - `NumTips`: 2 bytes (`int16_t`).
  - Array de `NumTips` elementos de `char[255]` (Windows-1252 ANSI).
- **`Lluvia.dat`**:
  - `MiCabecera`: 263 bytes (`tCabecera`).
  - `Nu`: 2 bytes (`int16_t`).
  - Array de `Nu` elementos de 1 byte (`uint8_t`).

---

### 4. Formatos de Texto Estructurado (Archivos INI)

#### 4.1. Archivos de Personaje (`legacy/server/Charfile/<NAME>.chr`)
Persistencia individual de cada jugador en texto INI (Windows-1252). Los campos clave que debe mapear el parser son:

| Sección | Clave | Tipo VB6 / C++ | Formato / Valores Permitidos |
| :--- | :--- | :---: | :--- |
| `[INIT]` | `Password` | `String` | Contraseña en texto plano o hash MD5 según configuración. |
| `[INIT]` | `Genero`, `Raza`, `Clase`, `Hogar` | `Byte` (`uint8_t`) | Enums correspondientes (`eGenero`, `eRaza`, `eClase`, `eCiudad`). |
| `[INIT]` | `Heading` | `Byte` (`uint8_t`) | 1 a 4. |
| `[INIT]` | `Head`, `Body` | `Integer` (`int16_t`) | Índices de gráficos corporales. |
| `[INIT]` | `Arma`, `Escudo`, `Casco` | `Integer` (`int16_t`) | Índices de items equipados (o `2` para vacío). |
| `[INIT]` | `Position` | `String` | Texto con formato `"Mapa-X-Y"` (e.g. `"1-50-50"`). |
| `[INIT]` | `UpTime` | `Long` (`int32_t`) | Segundos acumulados en juego. |
| `[STATS]` | `GLD`, `BANCO` | `Long` (`int32_t`) | Oro en billetera y depositado en banco. |
| `[STATS]` | `MaxHP`, `MinHP`, `MaxMAN`, `MinMAN` | `Integer` (`int16_t`) | Vida y Maná máximo/actual. |
| `[STATS]` | `MaxSTA`, `MinSTA`, `MaxAGU`, `MinAGU` | `Integer` (`int16_t`) | Energía y Sed. |
| `[STATS]` | `MaxHAM`, `MinHAM` | `Integer` (`int16_t`) | Hambre. |
| `[STATS]` | `Exp`, `ELU` | `Long` (`int32_t`) | Experiencia actual y requerida para subir de nivel. |
| `[STATS]` | `ELV` | `Integer` (`int16_t`) | Nivel del personaje (1 a 50+). |
| `[ATRIBUTOS]` | `Fuerza`, `Agilidad`, `Inteligencia`, `Carisma`, `Constitucion` | `Byte` (`uint8_t`) | Valores numéricos del personaje (1 a 21). |
| `[FLAGS]` | `Muerto`, `Escondido`, `Navegando`, `Envenenado`, `Paralizado` | `Byte` (`uint8_t`) | Banderas booleanas (1 = Activo, 0 = Inactivo). |
| `[INVENTORY]`| `CantidadItems` | `Integer` (`int16_t`) | Cantidad total de slots ocupados. |
| `[INVENTORY]`| `Item<Slot>` | `String` | Formato `"ObjIndex-Amount-Equipped"` (e.g. `"128-1-1"` para slot 1 a 30). |
| `[SPELLS]` | `CantidadSpells` | `Integer` (`int16_t`) | Total de hechizos aprendidos. |
| `[SPELLS]` | `Sp<Slot>` | `Integer` (`int16_t`) | ID de hechizo en `Hechizos.dat` para cada slot 1 a 35. |
| `[SKILLS]` | `Sk<N>` | `Byte` (`uint8_t`) | Asignación de puntos en cada una de las 21 habilidades. |
| `[FACCIONES]`| `EjercitoReal`, `EjercitoCaos`, `CiudMatados`, `CrimMatados` | `Byte` / `Long` | Estadísticas y rangos de las facciones armada y caos. |
| `[BANCO]` | `CantidadItems`, `Item<Slot>` | `Integer` / `String` | Items almacenados en el banco (`ObjIndex-Amount`). |

#### 4.2. Tablas de Datos del Juego (`legacy/server/Dat/`)
- **`OBJ.dat`**: Define los ítems del juego bajo secciones `[OBJ<N>]` (`Name`, `GrhIndex`, `ObjType`, `MinHIT`, `MaxHIT`, `MinDEF`, `MaxDEF`, `Valor`, `Crucial`, `Real`, `Caos`, etc.).
- **`NPCs.dat`**: Define criaturas bajo `[NPC<N>]` (`Name`, `Desc`, `Body`, `Head`, `Heading`, `MaxHP`, `MinHP`, `GiveEXP`, `GiveGLD`, `Hostil`, `Domable`, `Alineacion`, `Spells`, `Drops`, etc.).
- **`Hechizos.dat`**: Define magia bajo `[HECHIZO<N>]` (`Nombre`, `Desc`, `PalabrasMagicas`, `HechizeroMsg`, `TargetMsg`, `Tipo`, `MinHP`, `MaxHP`, `ManaRequerido`, `StaRequerido`, `WAV`, `FXgrh`, etc.).

#### 4.3. Archivos de Clanes (`legacy/server/guilds/`)
- **`guildsinfo.inf`**: Encabezado `[INIT] NroGuilds=N`, y secciones `[GUILD<N>]` con datos de clan (`Founder`, `GuildName`, `Date`, `Alineacion`, `Leader`, `Desc`, `Codex1` a `Codex8`).
- **`<GuildName>-members.mem`**: Encabezado `[INIT] NroMembers=N`, y sección `[Members]` con claves `Member1=Nick`, `Member2=...`.
- **`<GuildName>-solicitudes.sol`**: Encabezado `[INIT] NroSolicitudes=N`, y sección `[Solicitudes]` con postulantes.

---

## Preguntas Abiertas
- **Formato de Cadenas en `Inicio.con`**: En `GameIni.bas`, el tipo `tGameIni` contiene miembros dinámicos (`Password As String`, `Name As String`, etc.). En VB6, `Put #` sobre strings dinámicos dentro de un record graba un prefijo de 2 bytes (`int16_t`) con la longitud seguido de los caracteres. No obstante, dado que en C++ `Inicio.ini` reemplaza a `Inicio.con` en las configuraciones modernas, debe verificarse si se mantendrá soporte retrocompatible para leer `Inicio.con` binario o se migrará exclusivamente a formato INI de texto.
- **Normalización de Salto de Línea en Archivos INI**: En Windows, los archivos `.chr` y `.dat` utilizan `\r\n` (CRLF). Los parsers en C++ deben aceptar de manera agnóstica tanto terminaciones `\r\n` como `\n` (LF) para garantizar portabilidad transparente en servidores Linux.
