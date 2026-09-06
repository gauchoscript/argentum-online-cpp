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
  - legacy/server/Codigo/clsClan.cls
  - tests/fixtures/manifest.md
  - tests/generate_fixtures.ps1
  - tests/fixtures/charfile/PEPE.chr
  - tests/fixtures/charfile/GONZALO.chr
  - tests/fixtures/charfile/NOVATO.chr
  - tests/fixtures/charfile/PODEROSO.chr
  - tests/fixtures/guilds/guildsinfo.inf
  - tests/fixtures/guilds/Armada Real-members.mem
tags: [datos, formatos, ini, chr, guild, map, dat, binario, structs, padding, verificacion-fixtures]
last_updated: 2026-09-06
---

## Resumen
Argentum Online v0.13.0 utiliza dos categorías fundamentales de persistencia de datos:
1. **Archivos Binarios Propietarios**: Formatos binarios en disco escritos y leídos mediante las instrucciones nativas `Put #` y `Get #` de VB6 (con empaquetado de structs de 1 byte sin padding, campos numéricos Little-Endian x86 y cadenas de tamaño fijo). Se utilizan en mapas (`.map`), triggers de servidor (`.inf`) y tablas de índices gráficos del cliente (`.ind` y `.con`).
2. **Archivos de Texto Estructurado INI**: Archivos de texto plano codificados en Windows-1252 (ANSI) organizados en secciones `[SECCION]` y pares `Clave=Valor` con terminaciones de línea CRLF (`\r\n`). Se utilizan para la persistencia individual de personajes (`.chr`), administración de clanes (`guildsinfo.inf`, `.mem`, `.sol`, `.rel`) y configuraciones estáticas del juego (`OBJ.dat`, `NPCs.dat`, `Hechizos.dat`, `Balance.dat`).

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

### 3. Archivos de Foros (`.for`)
Para la auditoría detallada de la estructura en disco de los archivos de foros (`.for`), su patrón de nombres (`<ID>.for`, `<ID><N>.for`, `<ID><N>a.for`) y su formato de texto/INI, consultá la documentación profunda en [11a-modforum-detalle.md](11a-modforum-detalle.md).

### 3. Persistencia de Personajes (`legacy/server/Charfile/<NAME>.chr`)
La rutina principal de guardado en el servidor es `SaveUser` en `legacy/server/Codigo/FileIO.bas`.
- La información en memoria se mapea desde los tipos de datos `User`, `UserStats`, `UserFlags`, `UserCounters`, `tFacciones`, `Inventario` y `BancoInventario` definidos en `legacy/server/Codigo/Declares.bas`.
- La persistencia se efectúa secuencialmente llamando al procedimiento helper `WriteVar`, el cual emite un archivo INI estándar formateado en ANSI (Windows-1252).
- Los nombres de archivo corresponden al nick del usuario en mayúsculas concatenado con la extensión `.chr` (ej. `PEPE.chr`).

### 4. Persistencia de Clanes (`legacy/server/guilds/`)
La administración y persistencia de clanes es operada por `legacy/server/Codigo/modGuilds.bas` y la clase `legacy/server/Codigo/clsClan.cls`. Se divide en 4 familias de archivos de texto INI:
- **`guildsinfo.inf`**: Archivo maestro con metadatos globales (`[INIT] NroGuilds`) y secciones individuales `[GUILD<N>]` para cada clan.
- **`<GuildName>-members.mem`**: Lista de miembros activos (`[INIT] NroMembers` y `[Members] Member<N>=<Nick>`).
- **`<GuildName>-solicitudes.sol`**: Postulaciones pendientes (`[INIT] CantSolicitudes` y `[SOLICITUD<N>] Nombre` / `Detalle`).
- **`<GuildName>-relaciones.rel`**: Relaciones diplomáticas de alianza, paz o guerra con otros clanes.

### 5. Verificación Empírica y Cruzada con Datos Reales (Fixtures)
Se contrastó la especificación del código fuente contra los archivos reales de prueba ubicados en `tests/fixtures/charfile/` y `tests/fixtures/guilds/` (generados por `tests/generate_fixtures.ps1`). Se confirmó la coincidencia exacta entre la estructura del código, los valores esperados de negocio y los offsets a nivel de bytes en disco.

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

### 4. Formato de Archivos de Personaje (`.chr`)

#### 4.1. Estructura Exhaustiva de Secciones y Claves
Los archivos `.chr` se persisten mediante la rutina `SaveUser` (`legacy/server/Codigo/FileIO.bas`) en formato INI con codificación Windows-1252.

| Sección | Clave | Tipo VB6 / C++ Equivalent | Descripción y Formato de Valor |
| :--- | :--- | :---: | :--- |
| `[INIT]` | `Password` | `String` | Contraseña en texto plano o hash MD5. |
| `[INIT]` | `Genero` | `Byte` (`uint8_t`) | Enum `eGenero`: 1=Hombre, 2=Mujer. |
| `[INIT]` | `Raza` | `Byte` (`uint8_t`) | Enum `eRaza`: 1=Humano, 2=Elfo, 3=Alto Elfo, 4=Gnomo, 5=Elfo Oscuro. |
| `[INIT]` | `Hogar` | `Byte` (`uint8_t`) | Enum `eCiudad`: 1=Ullathorpe, 2=Nix, 3=Banderbill, etc. |
| `[INIT]` | `Clase` | `Byte` (`uint8_t`) | Enum `eClase`: 1=Mago, 2=Clérigo, 3=Guerrero, 4=Asesino, 5=Aventurero, 6=Cazador, etc. |
| `[INIT]` | `Desc` | `String` | Descripción libre escrita por el personaje. |
| `[INIT]` | `Heading` | `Byte` (`uint8_t`) | Orientación gráfica (1=Norte, 2=Este, 3=Sur, 4=Oeste). |
| `[INIT]` | `Head`, `Body` | `Integer` (`int16_t`) | ID de sprite de cabeza y cuerpo. |
| `[INIT]` | `Arma`, `Escudo`, `Casco` | `Integer` (`int16_t`) | ID de gráfico de animación equipada (2 = desequipado). |
| `[INIT]` | `UpTime` | `Long` (`int32_t`) | Segundos jugados acumulados. |
| `[INIT]` | `LastIP1` .. `LastIP5` | `String` | Historial de conexiones `"IP - fecha:hora"`. |
| `[INIT]` | `Position` | `String` | Formato `"Mapa-X-Y"` (e.g. `"1-50-50"`). |
| `[STATS]` | `GLD`, `BANCO` | `Long` (`int32_t`) | Monedas de oro en billetera y depositadas en banco. |
| `[STATS]` | `MaxHP`, `MinHP` | `Integer` (`int16_t`) | Puntos de salud máximo y actual. |
| `[STATS]` | `MaxMAN`, `MinMAN` | `Integer` (`int16_t`) | Puntos de maná máximo y actual. |
| `[STATS]` | `MaxSTA`, `MinSTA` | `Integer` (`int16_t`) | Energía / Estamina máxima y actual. |
| `[STATS]` | `MaxHAM`, `MinHAM` | `Integer` (`int16_t`) | Nivel de hambre. |
| `[STATS]` | `MaxAGU`, `MinAGU` | `Integer` (`int16_t`) | Nivel de sed. |
| `[STATS]` | `MaxHIT`, `MinHIT` | `Integer` (`int16_t`) | Daño máximo y mínimo. |
| `[STATS]` | `EXP`, `ELU` | `Long` (`int32_t`) | Experiencia actual y requerida para el siguiente nivel. |
| `[STATS]` | `ELV` | `Byte` (`uint8_t`) | Nivel actual del personaje ($1 \dots 50+$). |
| `[STATS]` | `SkillPtsLibres` | `Integer` (`int16_t`) | Puntos de habilidad pendientes de asignar. |
| `[ATRIBUTOS]`| `AT1` .. `AT5` | `Byte` (`uint8_t`) | Fuerza (AT1), Agilidad (AT2), Inteligencia (AT3), Carisma (AT4), Constitución (AT5) ($1 \dots 21$). |
| `[SKILLS]` | `SK1` .. `SK21` | `Byte` (`uint8_t`) | Puntos asignados en cada habilidad ($0 \dots 100$). |
| `[SKILLS]` | `ELUSK1` .. `ELUSK21` | `Long` (`int32_t`) | Experiencia requerida para subir la habilidad. |
| `[SKILLS]` | `EXPSK1` .. `EXPSK21` | `Long` (`int32_t`) | Experiencia acumulada en la habilidad. |
| `[FLAGS]` | `Muerto`, `Escondido`, etc. | `Byte` (`uint8_t`) | Banderas de estado (`0` = inactivo, `1` = activo). |
| `[FACCIONES]`| `EjercitoReal`, `EjercitoCaos`| `Byte` (`uint8_t`) | Estado de membresía en facciones (`0` o `1`). |
| `[FACCIONES]`| `CiudMatados`, `CrimMatados`| `Long` (`int32_t`) | Conteo de ciudadanos y criminales ejecutados. |
| `[INVENTORY]`| `CantidadItems` | `Integer` (`int16_t`) | Cantidad total de slots ocupados en inventario ($0 \dots 30$). |
| `[INVENTORY]`| `Item<N>` | `String` | Formato `"ObjIndex-Cantidad-Equipado"` (ej. `"101-5-0"`). |
| `[INVENTORY]`| `WeaponEqpSlot`, etc. | `Byte` (`uint8_t`) | Slot de inventario asignado al equipamiento. |
| `[SPELLS]` | `CantidadSpells` | `Integer` (`int16_t`) | Total de hechizos aprendidos ($0 \dots 35$). |
| `[SPELLS]` | `Sp<N>` | `Integer` (`int16_t`) | ID de hechizo en slot $N$. |
| `[BANCO]` / `[BancoInventory]`| `CantidadItems`, `Obj<N>` | `Integer` / `String` | Items guardados en banco con formato `"ObjIndex-Cantidad"`. |
| `[GUILD]` | `GuildIndex`, `Miembro` | `Integer` / `String` | Índice de clan y nombre del clan perteneciente. |

#### 4.2. Matriz de Verificación Cruzada contra Fixtures Reales (`tests/fixtures/charfile/`)

Se realizó la verificación empírica leyendo los archivos de fixtures en disco byte por byte:

| Archivo Fixture | Campo Evaluado | Valor Esperado | Valor Real en Disco | Línea | Offset (Hex) | Offset (Decimal) | Estado de Verificación |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| `PEPE.chr` | `[INIT] Genero` | `1` | `1` | 3 | `0x0019` | 25 B | **Confirmado con datos reales** |
| `PEPE.chr` | `[INIT] Raza` | `1` | `1` | 4 | `0x0022` | 34 B | **Confirmado con datos reales** |
| `PEPE.chr` | `[INIT] Hogar` | `1` | `1` | 5 | `0x0029` | 41 B | **Confirmado con datos reales** |
| `PEPE.chr` | `[INIT] Clase` | `1` | `1` | 6 | `0x0031` | 49 B | **Confirmado con datos reales** |
| `PEPE.chr` | `[STATS] GLD` | `50000` | `50000` | 18 | `0x00D5` | 213 B | **Confirmado con datos reales** |
| `PEPE.chr` | `[STATS] BANCO` | `100000` | `100000` | 19 | `0x00DF` | 223 B | **Confirmado con datos reales** |
| `PEPE.chr` | `[STATS] MaxHP` | `250` | `250` | 20 | `0x00EC` | 236 B | **Confirmado con datos reales** |
| `PEPE.chr` | `[STATS] ELV` | `25` | `25` | 34 | `0x0184` | 388 B | **Confirmado con datos reales** |
| `PEPE.chr` | `[INVENTORY] Item1` | `"101-5-0"` | `"101-5-0"` | 79 | `0x02E5` | 741 B | **Confirmado con datos reales** |
| `PEPE.chr` | `[GUILD] GuildIndex`| `1` | `1` | 99 | `0x03BD` | 957 B | **Confirmado con datos reales** |
| `PEPE.chr` | `[GUILD] Miembro` | `"Legion de Honor"` | `"Legion de Honor"` | 101 | `0x03D7` | 983 B | **Confirmado con datos reales** |
| `GONZALO.chr` | `[INIT] Raza` | `2` | `2` | 4 | `0x0022` | 34 B | **Confirmado con datos reales** |
| `GONZALO.chr` | `[INIT] Clase` | `3` | `3` | 6 | `0x0031` | 49 B | **Confirmado con datos reales** |
| `GONZALO.chr` | `[STATS] GLD` | `250000` | `250000` | 18 | `0x00E8` | 232 B | **Confirmado con datos reales** |
| `GONZALO.chr` | `[STATS] ELV` | `45` | `45` | 34 | `0x019C` | 412 B | **Confirmado con datos reales** |
| `NOVATO.chr` | `[STATS] GLD` | `0` | `0` | 18 | `0x00CE` | 206 B | **Confirmado con datos reales** |
| `NOVATO.chr` | `[STATS] ELV` | `1` | `1` | 34 | `0x0160` | 352 B | **Confirmado con datos reales** |
| `NOVATO.chr` | `[INVENTORY] CantidadItems` | `0` | `0` | 78 | `0x029A` | 666 B | **Confirmado con datos reales** |
| `PODEROSO.chr` | `[STATS] GLD` | `5000000` | `5000000` | 18 | `0x00E0` | 224 B | **Confirmado con datos reales** |
| `PODEROSO.chr` | `[STATS] MaxHP` | `999` | `999` | 20 | `0x00FB` | 251 B | **Confirmado con datos reales** |
| `PODEROSO.chr` | `[STATS] MaxMAN` | `9999` | `9999` | 22 | `0x010F` | 271 B | **Confirmado con datos reales** |
| `PODEROSO.chr` | `[STATS] ELV` | `50` | `50` | 34 | `0x0191` | 401 B | **Confirmado con datos reales** |
| `PODEROSO.chr` | `[ATRIBUTOS] AT1` | `21` | `21` | 37 | `0x01A5` | 421 B | **Confirmado con datos reales** |
| *Cualquier personaje* | `[COUNTERS] Pena` | `Long` | *No emitido en fixture inicial* | N/A | N/A | N/A | **Inferido del código fuente** (`FileIO.bas:1748`) |
| *Cualquier personaje* | `[INIT] LastIP2..5`| `String` | *No emitido en fixture de IP fija* | N/A | N/A | N/A | **Inferido del código fuente** (`FileIO.bas:1820`) |
| *Cualquier personaje* | `[MASCOTAS] MAS1` | `String` | *No emitido sin mascotas* | N/A | N/A | N/A | **Inferido del código fuente** (`FileIO.bas:1890`) |

---

### 5. Formato de Archivos de Clanes (`legacy/server/guilds/`)

#### 5.1. Estructura de la Persistencia de Clanes
- **`guildsinfo.inf`**:
  - `[INIT] NroGuilds=<Count>`: Número entero de clanes registrados.
  - Secciones `[GUILD<N>]`: `Founder`, `GuildName`, `Date`, `Antifaccion`, `Alineacion` (`Neutral`, `Real`, `Caos`), `Leader`, `URL`, `GuildNews`, `Desc`, `Codex1` a `Codex8`, `EleccionesAbiertas`.
- **`<GuildName>-members.mem`**:
  - `[INIT] NroMembers=<Count>`: Cantidad de integrantes.
  - `[Members] Member<N>=<Nick>`: Nickname en mayúsculas de cada miembro ($1 \dots N$).
- **`<GuildName>-solicitudes.sol`**:
  - `[INIT] CantSolicitudes=<Count>`: Cantidad de peticiones de ingreso.
  - `[SOLICITUD<N>] Nombre=<Nick>` y `Detalle=<Texto>`: Petición de aspirantes.
- **`<GuildName>-relaciones.rel`**:
  - `[RELACIONES] <GuildIndex>=<Estado>`: Estado diplomático con otros clanes.

#### 5.2. Matriz de Verificación Cruzada contra Fixtures Reales (`tests/fixtures/guilds/`)

| Archivo Fixture | Campo Evaluado | Valor Esperado | Valor Real en Disco | Línea | Offset (Hex) | Offset (Decimal) | Estado de Verificación |
| :--- | :--- | :---: | :---: | :---: | :---: | :---: | :---: |
| `guildsinfo.inf` | `[INIT] NroGuilds` | `3` | `3` | 2 | `0x0007` | 7 B | **Confirmado con datos reales** |
| `guildsinfo.inf` | `[GUILD1] Founder` | `"Gonzalo"` | `"Gonzalo"` | 5 | `0x001D` | 29 B | **Confirmado con datos reales** |
| `guildsinfo.inf` | `[GUILD1] GuildName` | `"Legion de Honor"` | `"Legion de Honor"` | 6 | `0x002D` | 45 B | **Confirmado con datos reales** |
| `guildsinfo.inf` | `[GUILD1] Alineacion`| `"Neutral"` | `"Neutral"` | 9 | `0x0065` | 101 B | **Confirmado con datos reales** |
| `guildsinfo.inf` | `[GUILD1] Leader` | `"Gonzalo"` | `"Gonzalo"` | 10 | `0x0078` | 120 B | **Confirmado con datos reales** |
| `guildsinfo.inf` | `[GUILD2] GuildName` | `"Armada Real"` | `"Armada Real"` | 20 | `0x01B4` | 436 B | **Confirmado con datos reales** |
| `guildsinfo.inf` | `[GUILD2] Alineacion`| `"Real"` | `"Real"` | 23 | `0x01E8` | 488 B | **Confirmado con datos reales** |
| `guildsinfo.inf` | `[GUILD3] GuildName` | `"Fuerzas del Caos"` | `"Fuerzas del Caos"` | 34 | `0x0332` | 818 B | **Confirmado con datos reales** |
| `guildsinfo.inf` | `[GUILD3] Alineacion`| `"Caos"` | `"Caos"` | 37 | `0x036B` | 875 B | **Confirmado con datos reales** |
| `Armada Real-members.mem` | `[INIT] NroMembers` | `1` | `1` | 2 | `0x0007` | 7 B | **Confirmado con datos reales** |
| `Armada Real-members.mem` | `[Members] Member1` | `"SACERDOTEREAL"` | `"SACERDOTEREAL"` | 5 | `0x001F` | 31 B | **Confirmado con datos reales** |
| *Archivo `.rel`* | `[RELACIONES]` | `<Index>=<Estado>` | *No parseado en fixture mínima* | N/A | N/A | N/A | **Inferido del código fuente** (`clsClan.cls:661`) |

---

## Preguntas Abiertas
- **Nombres de Archivos de Clanes con Espacios en Linux**: Archivos como `Armada Real-members.mem` contienen espacios en blanco en el nombre de archivo. En sistemas de archivos Windows (NTFS) esto no representa inconveniente, pero la implementación en C++ debe garantizar la correcta manipulación de paths multiplataforma evitando problemas de escaping en Linux.
- **Divergencias en Secciones de Inventario de Banco**: En `FileIO.bas`, la rutina `SaveUser` escribe la sección como `[BancoInventory]` con claves `Obj<N>`, mientras que rutinas de versiones anteriores utilizaban `[BANCO]`. El parser C++ debe ofrecer soporte tolerante para ambas claves de sección.
- **Formato de Cadenas en `Inicio.con`**: En `GameIni.bas`, el tipo `tGameIni` contiene miembros dinámicos (`Password As String`, `Name As String`, etc.). En VB6, `Put #` sobre strings dinámicos dentro de un record graba un prefijo de 2 bytes (`int16_t`) con la longitud seguido de los caracteres. No obstante, dado que en C++ `Inicio.ini` reemplaza a `Inicio.con` en las configuraciones modernas, debe verificarse si se mantendrá soporte retrocompatible para leer `Inicio.con` binario o se migrará exclusivamente a formato INI de texto.
