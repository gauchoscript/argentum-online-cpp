---
area: plan-de-port
status: in-progress
audit_reference:
  - docs/audit/01-estructura-del-proyecto.md
  - docs/audit/01a-clsdicc-cgarbage.md
  - docs/audit/02-protocolo-de-red.md
  - docs/audit/02a-securityip-detalle.md
  - docs/audit/06-formatos-de-datos.md
tags: [plan, arquitectura, port, dependencias, modulos, servidor, cpp]
last_updated: 2026-09-06
---

# Plan de Port Secuencial del Servidor — Argentum Online v0.13.0 (VB6 a C++)

Este documento establece el orden estricto de migración a C++ de **todos los módulos del servidor legacy** (`legacy/server/Codigo/`) identificados en la auditoría del proyecto. Se excluyen expresamente los módulos exclusivos del cliente y las interfaces gráficas del servidor (formularios `.frm` de monitoreo/UI), los cuales se abordarán en fases posteriores.

El criterio de ordenamiento es **secuencial por árbol de dependencias**: los módulos base o sin dependencias se migran primero, garantizando que cada paso de porting sólo dependa de código C++ ya migrado y verificado.

---

> [!IMPORTANT]
> ### Sistemas de Numeración de la Documentación del Proyecto
> Es fundamental distinguir los tres sistemas de numeración independientes utilizados en la documentación de este proyecto para evitar confusiones y no asumir erróneamente que se corresponden entre sí:
> 
> 1. **`docs/audit/` (01 al 10, más profundizaciones como 01a, 06a, 11a)**: Numerados por **ÁREA TEMÁTICA DE AUDITORÍA**, fijos desde la fase de auditoría inicial. Esta numeración responde únicamente a los temas auditados y **no tiene relación alguna con el orden de implementación** ni debe intentarse alinear con nada más.
> 2. **`docs/implementation/` (todos los documentos de implementación, breakdowns y anexos)**: Los documentos en `docs/implementation/` utilizan SIEMPRE como prefijo el ID de Módulo oficial asignado en este plan, agrupando visual y lógicamente todos los archivos (implementación, breakdowns, anexos) de un mismo componente físico heredado del legacy (ej. `10-fileio-*.md`, `11-clsclan-*.md`, `12-securityip.md`, `14-tcp.md`, `15-modsenddata-*.md`). Se elimina por completo el orden cronológico para evitar divergencias y dispersión documental.

---

> [!CAUTION]
> ## 1. Requerimientos Críticos de Compatibilidad de Servidor (Banderas Prominentes)
> 
> Existen dos categorías fundamentales del servidor que conllevan **requisitos estrictos de compatibilidad byte a byte** confirmados en la auditoría. Estas categorías deben diseñarse con prioridad absoluta en cuanto a sus especificaciones de diseño:
> 
> ### Categoría 1: Módulos de Persistencia de Datos (Personajes y Clanes)
> - **Módulos involucrados**: `FileIO.bas`, `modGuilds.bas`, `clsClan.cls`, `cSolicitud.cls`.
> - **Requisito de Compatibilidad**: **Salida idéntica byte a byte**. La lectura y guardado de archivos de personaje (`.chr`) y archivos de clanes (`guildsinfo.inf`, `.mem`, `.sol`, `.rel`) deben producir exactamente el mismo formato de texto INI en codificación Windows-1252 (ANSI) y con finales de línea CRLF (`\r\n`).
> - **Estrategia de Verificación**: **No se valida únicamente con el cliente**, sino contra los archivos de fixture reales ubicados en `tests/fixtures/charfile/` y `tests/fixtures/guilds/` mediante suites de pruebas unitarias en C++ usando **doctest**.
> 
> ### Categoría 2: Módulos de Protocolo de Red y Múltiples Conexiones Simultáneas
> - **Módulos involucrados**: `clsByteQueue.cls`, `SecurityIp.bas`, `TCP.bas` (reemplazando `wsksock.bas`/`wskapiAO.bas` con **standalone Asio**), `modSendData.bas`, `Protocol.bas` (con `clsAntiMassClon.cls` formalmente excluido por código muerto).
> - **Requisito de Compatibilidad**: **Compatibilidad binaria de socket a nivel de byte y soporte multijugador**. El empaquetamiento little-endian de enteros, floats, cadenas con prefijo de 2 bytes y booleans de 1 byte debe encajar de forma exacta con la especificación de `ClientPacketID` y `ServerPacketID`. Además, se debe preservar la capacidad del servidor de gestionar múltiples conexiones simultáneas mapeadas a su `UserIndex` independiente.
> - **Estrategia de Verificación**: Verificación binaria con pruebas unitarias en **doctest** sobre `clsByteQueue` y validación de integración conectando el **cliente ejecutable real en VB6** contra nuestro servidor C++.

---

> [!WARNING]
> ## 2. Módulos con Cobertura Incompleta en la Auditoría (Bandera de Advertencia)
> 
> La auditoría de la estructura del proyecto (`01-estructura-del-proyecto.md`) cubrió únicamente un resumen general. Los siguientes módulos del servidor **no poseen un desglose exhaustivo en los documentos de auditoría existentes** o están cubiertos solo parcialmente. Acordate de inspeccionar detenidamente el archivo de código fuente VB6 original antes de iniciar su migración:
> 
> - **`modForum.bas`**: Sistema de foros / tableros de mensajes dentro del juego *(Auditoría completada en [11a-modforum-detalle.md](../audit/11a-modforum-detalle.md))*.
> - **`modCentinela.bas`**: Sistema anti-bot interactivo con preguntas y respuestas.
> - **`praetorians.bas`**: Inteligencia artificial de guardias pretorianos y protección de ciudades.
> - **`ConsultasPopulares.cls`**: Sistema de encuestas y votaciones dentro del juego.
> - **`clsEstadisticasIPC.cls`** y **`Statistics.bas`**: Recolección de estadísticas de rendimiento y tráfico IPC del servidor.
> - **`History.bas`**: Registro de historial y eventos del mundo.
> - **`clsMapSoundManager.cls`**: Gestor de sonidos ambientales del mapa disparados por el servidor.
> - **`Trabajo.bas`**: Oficios y recolección de recursos (herrería, carpintería, pesca, minería, tala) — cubierto solo parcialmente en auditorías de datos.
> - **`clsdicc.cls`**: Estructura de datos auxiliar de diccionario / tabla hash *(Capa 0 — Auditoría completada en [01a-clsdicc-cgarbage.md](../audit/01a-clsdicc-cgarbage.md))*.
> - **`cGarbage.cls`**: Estructura plana de coordenadas para limpieza de objetos temporales del mapa *(Capa 0 — Auditoría completada en [01a-clsdicc-cgarbage.md](../audit/01a-clsdicc-cgarbage.md))*.
> 
> > [!NOTE]
> > **Nota sobre la Capa 0**: Los módulos **`clsdicc.cls`** y **`cGarbage.cls`** ya cuentan con su pase de auditoría profunda completado en [`01a-clsdicc-cgarbage.md`](../audit/01a-clsdicc-cgarbage.md). Todos los módulos de la Capa 0 están listos para avanzar directamente con su migración a C++.

---

### 2.1. Matriz de Triaje Rápido de Auditoría (Persistencia y Red)

Se realizó un pase de triaje rápido (*triage pass*) sobre los 9 módulos restantes identificados con cobertura incompleta para evaluar si leen/escriben archivos directamente o envían/reciben paquetes de red, determinando si introducen formatos o paquetes no documentados en `docs/audit/06-formatos-de-datos.md` y `docs/audit/02-protocolo-de-red.md`:

| Módulo Legacy | ¿Toca Archivos? (Persistencia) | ¿Toca Red? (Sockets / Paquetes) | Flag de Prioridad Recomendada |
| :--- | :--- | :--- | :--- |
| `modForum.bas` | **Sí** — Lee y escribe archivos `.for` (`App.Path & "\Foros\" & ID & ".for"`, `.for` y `a.for`). Formato no cubierto en `06-formatos-de-datos.md`. | **Sí** — Envía paquetes de foro con `WriteAddForumMsg` / `eForumMsgType`. Opcodes no cubiertos en `02-protocolo-de-red.md`. | **Auditoría completada** ([`11a-modforum-detalle.md`](../audit/11a-modforum-detalle.md)) |
| `modCentinela.bas` | **Sí** — Modifica archivos de personaje `.chr` (sección `[FLAGS]` Ban y `[PENAS]`), cubiertos en `06-formatos-de-datos.md`. Escribe logs en `logs\Centinela.log`. | **Sí** — Envía mensajes de consola y overhead de chat (`WriteConsoleMsg`, `WriteChatOverHead`). Paquetes cubiertos en `02-protocolo-de-red.md`. | **defer, audit before its own port step as planned** |
| `praetorians.bas` | **No** — Lógica pura de IA y combate de guardias pretorianos (delegada a `MODULO_NPCs` / `modHechizos`). | **No** — La comunicación de red se delega a los subsistemas de magia y combate. | **defer, audit before its own port step as planned** |
| `ConsultasPopulares.cls` | **Sí** — Lee/escribe `\dat\consultas.dat` (INI), `.chr` (sección `[CONSULTAS]`) y appendea emails a `\logs\votaron.dat`. Formatos INI / `.chr` cubiertos. | **Sí** — Envía mensajes de consola (`WriteConsoleMsg`). Paquetes cubiertos en `02-protocolo-de-red.md`. | **defer, audit before its own port step as planned** |
| `History.bas` | **No** — Archivo de comentarios con el historial de versiones del desarrollo original VB6. Sin código ejecutable. | **No** — Sin llamadas a red. | **defer, audit before its own port step as planned** |
| `Statistics.bas` | **No** *(Los archivos producidos `\logs\stats.log` y `\logs\huffman.log` son exportaciones de diagnóstico para GNU Octave/Huffman, no persistencia del juego)*. | **No** — Sin llamadas a red. | **defer, audit before its own port step as planned** |
| `clsEstadisticasIPC.cls` | **No** — Sin operaciones de E/S de archivos. | **No** — Usa Win32 API (`SendMessageA`) para IPC local con ventana GUI externa de estadísticas. | **defer, audit before its own port step as planned** |
| `clsMapSoundManager.cls` | **Sí** — Lee archivos INI `MAPA<N>.dat` en `Maps/` (sección `[SONIDOS]`), cubiertos por especificaciones INI estándar. | **Sí** — Envía paquetes de sonido (`PrepareMessagePlayWave` / `WritePlayWave`). Opcode de sonido estándar cubierto. | **defer, audit before its own port step as planned** |
| `Trabajo.bas` | **No** — Opera sobre estructuras en memoria de habilidades (minería, tala, apuñalar, robar, pescar). | **Sí** — Envía actualizaciones del usuario (`WriteUpdateGold`, `WriteUpdateSta`, `WriteConsoleMsg`). Paquetes cubiertos. | **defer, audit before its own port step as planned** |

---

## 3. Política de Nombres y Estructura (docs/CONVENTIONS.md)

Para cada módulo se aplica estrictamente la política de nombres definida en `docs/CONVENTIONS.md`:
1. **Preservar el identificador legacy de VB6** siempre que sea un identificador válido en C++ (ej. `FileIO` -> `FileIO.hpp` / `FileIO.cpp`, `clsByteQueue` -> `clsByteQueue.hpp` / `clsByteQueue.cpp`).
2. Si el identificador choca con una palabra reservada de C++ (ej. `New`, `Class`), usar el término en **Español Rioplatense**.
3. Mantener la correspondencia 1 a 1 entre módulos `.bas` / `.cls` y pares de archivos C++ (`.hpp` / `.cpp`).
4. **Framework de Pruebas**: Todas las pruebas unitarias se implementan exclusivamente con **doctest** (paquete `"doctest"` en `vcpkg.json` / CMake).
5. **Capa de Red**: La librería de red del servidor es **standalone Asio** (header-only, paquete `"asio"` en `vcpkg.json`), evitando dependencias pesadas del ecosistema Boost.

*Nota*: Se excluyen de esta lista los archivos de formularios GUI de administración del servidor (`frmAdmin.frm`, `frmServidor.frm`, `frmUserList.frm`, `frmTrafic.frm`, `frmCargando.frm`, `frmConID.frm`, `frmDebugNpc.frm`, `FrmInterv.frm`, `FrmStat.frm`) y la integración directa con la bandeja del sistema de Windows (`Modulo_SysTray.bas`), ya que representan la interfaz gráfica del ejecutable legacy. La lógica del bucle de timers contenida originalmente en `frmMain.frm` se extrae hacia `GameLogic.hpp` / `modNuevoTimer.hpp`.

---

## 4. Secuencia Ordenada de Porting por Capas de Dependencia

### Capa 0: Utilidades Autónomas y Clases Base (Sin Dependencias)

#### 1. `Matematicas` (COMPLETADO)
- **Archivos Legacy**: `legacy/server/Codigo/Matematicas.bas`
- **Propósito**: Funciones matemáticas puras, generación de números aleatorios (`RandomNumber`), cálculo de distancia entre coordenadas (`Distance`) y límites min/max.
- **Archivo C++ Propuesto**: `src/server/Matematicas.hpp` / `src/server/Matematicas.cpp`
- **Estado**: **Completado** (Documentación en [`01-matematicas.md`](01-matematicas.md)).
- **Dependencias**: *Ninguna*.
- **Estimación**: **Chico** (~80 líneas).
- **Estrategia de Verificación**: Pruebas unitarias en C++ con **doctest** (4/4 test cases pasados).

#### 2. `clsIniReader`
- **Archivos Legacy**: `legacy/server/Codigo/clsIniReader.cls`
- **Propósito**: Parser de archivos de texto INI para lectura de configuraciones, secciones y claves.
- **Archivo C++ Propuesto**: `src/server/clsIniReader.hpp` / `src/server/clsIniReader.cpp`
- **Dependencias**: *Ninguna*.
- **Estimación**: **Chico** (~300 líneas).
- **Estrategia de Verificación**: Pruebas unitarias con **doctest** de lectura sobre `Server.ini` y tablas de `Dat/`.

#### 3. `clsdicc`
- **Archivos Legacy**: `legacy/server/Codigo/clsdicc.cls`
- **Propósito**: Estructura de datos de diccionario / tabla asociativa clave-valor (usada en conteo de votos de clanes).
- **Archivo C++ Propuesto**: `src/server/clsdicc.hpp` / `src/server/clsdicc.cpp` (adaptador sobre `std::unordered_map`).
- **Dependencias**: *Ninguna*.
- **Estimación**: **Chico** (~150 líneas).
- **Estrategia de Verificación**: Pruebas unitarias en C++ con **doctest**.
- **Nota de Auditoría / Migración**: Cobertura completada en [`docs/audit/01a-clsdicc-cgarbage.md`](../audit/01a-clsdicc-cgarbage.md). Especificaciones C++ en [`docs/implementation/03-clsdicc.md`](03-clsdicc.md).

#### 4. `ModCola` y `Queue` (`cColaArray` EXCLUIDO)
- **Archivos Legacy**: `legacy/server/Codigo/ModCola.cls`, `Queue.bas` (`legacy/server/Codigo/cColaArray.cls` **EXCLUIDO**)
- **Propósito**: Implementaciones de colas FIFO para mensajes (`ModCola` / `/AYUDA`) y datos de navegación de NPCs (`Queue` / Pathfinding BFS).
- **Archivo C++ Propuesto**: `src/server/ModCola.hpp` / `src/server/ModCola.cpp`, `src/server/Queue.hpp` (`cColaArray.hpp` **NO SE PORTA**)
- **Estado de `cColaArray.cls`**: **EXCLUIDO (Código Muerto)**. Ver auditoría en [`docs/audit/06a-colaarray-dead-code.md`](../audit/06a-colaarray-dead-code.md). La clase está aislada dentro de un bloque `#If UsarQueSocket = 3` deshabilitado en `SERVER.VBP` (`UsarQueSocket = 1`), su única referencia (`CommandsBuffer`) fue eliminada del `Type User` en `Declares.bas` y no compilaría jamás. El changelog de 2006-2007 confirma que fue reemplazada definitivamente por `clsByteQueue`.
- **Dependencias**: *Ninguna*.
- **Estimación**: **Chico** (~200 líneas combinadas).
- **Estrategia de Verificación**: Pruebas unitarias en C++ con **doctest**.

#### 5. `cGarbage` (PARCIAL)
- **Archivos Legacy**: `legacy/server/Codigo/cGarbage.cls`
- **Propósito**: Estructura plana DTO de coordenadas (`map`, `X`, `Y`) para encolar la limpieza de objetos temporales del mundo (fogatas).
- **Archivo C++ Propuesto**: `src/server/cGarbage.hpp` (`struct cGarbage`)
- **Estado**: **Parcial** (Verificación diferida a `Acciones.bas` y `General.bas`, ver [`05-cgarbage.md`](05-cgarbage.md)).
- **Dependencias**: *Ninguna*.
- **Estimación**: **Chico** (~20 líneas).
- **Estrategia de Verificación**: Verificación de compilación (verificación de integración diferida a `Acciones` y `General`).
- **Nota de Auditoría / Migración**: Cobertura completada en [`docs/audit/01a-clsdicc-cgarbage.md`](../audit/01a-clsdicc-cgarbage.md). Especificaciones C++ en [`docs/implementation/05-cgarbage.md`](05-cgarbage.md).

#### 6. `modHexaStrings`
- **Archivos Legacy**: `legacy/server/Codigo/modHexaStrings.bas`
- **Propósito**: Conversiones de cadenas a formato hexadecimal y viceversa (utilizado en seguridad y hashes).
- **Archivo C++ Propuesto**: `src/server/modHexaStrings.hpp` / `src/server/modHexaStrings.cpp`
- **Estado**: **Completado** (Ver [`06-modhexastrings.md`](06-modhexastrings.md)).
- **Dependencias**: *Ninguna*.
- **Estimación**: **Chico** (~80 líneas).
- **Estrategia de Verificación**: Pruebas unitarias en C++ con **doctest**.

#### 7. `cSolicitud`
- **Archivos Legacy**: `legacy/server/Codigo/cSolicitud.cls`
- **Propósito**: Clase contenedora de datos para las solicitudes de ingreso a clanes (`UserName`, `desc`).
- **Archivo C++ Propuesto**: `src/server/cSolicitud.hpp` (`struct cSolicitud`)
- **Estado**: **Completado (Parcial / DTO)** (Ver [`07-csolicitud.md`](07-csolicitud.md)).
- **Dependencias**: *Ninguna*.
- **Estimación**: **Chico** (~15 líneas).
- **Estrategia de Verificación**: Verificación de compilación limpia (verificación de integración diferida a `clsClan` / `modGuilds`).

---

### Capa 1: Serialización de Red (Categoría Crítica 2)

#### 8. `clsByteQueue` *(CATEGORÍA CRÍTICA 2 - PROTOCOLO DE RED - COMPLETADO)*
- **Archivos Legacy**: `legacy/server/Codigo/clsByteQueue.cls`
- **Propósito**: Cola circular de bytes FIFO responsable del empaquetado binario little-endian, lectura/escritura de enteros, floats, cadenas con prefijo de longitud de 2 bytes y booleans de 1 byte.
- **Archivo C++ Propuesto**: `src/server/clsByteQueue.hpp` / `src/server/clsByteQueue.cpp`
- **Estado**: **Completado** (Documentación en [`08-clsbytequeue.md`](08-clsbytequeue.md)).
- **Dependencias**: *Ninguna* (manipulación pura de buffer de bytes).
- **Estimación**: **Mediano** (~600 líneas).
- **Estrategia de Verificación**: **Pruebas unitarias de alineación binaria de bytes con doctest y pruebas de sockets contra cliente VB6 real**. Pass 8/8 test cases.

---

### Capa 2: Declaraciones de Estructuras y Estado Global

#### 9. `Declares` (COMPLETADO - Reubicado a Capa 0)
- **Archivos Legacy**: `legacy/server/Codigo/Declares.bas`
- **Propósito**: Cabecera maestra que define todos los tipos globales (`User`, `UserStats`, `WorldPos`, `tCabecera`, `OBJDAT`, `NPCs`, `MapData`), arrays globales (`UserList`, `NpcList`, `MapData`) y constantes del mundo (`MaxUsers`, `MAXMAPS`).
- **Archivo C++ Propuesto**: `src/server/Declares.hpp` / `src/server/Declares.cpp`
- **Estado**: **Completado** (Documentación en [`09-declares.md`](09-declares.md)).
- **Dependencias**: *Ninguna* (Al confirmarse que no posee código ejecutable `Sub`/`Function`, se clasificó como módulo declarativo base).
- **Estimación**: **Grande** (~1.594 líneas).
- **Estrategia de Verificación**: Compilación limpia en C++ (`server_core`).
- **Nota de Auditoría / Migración (`cGarbage` / `TrashCollector`)**: Se incluyó la declaración de la colección global `TrashCollector` (para encolar objetos temporales del mapa como fogatas), según lo especificado en [`05-cgarbage.md`](05-cgarbage.md) y [`docs/audit/01a-clsdicc-cgarbage.md`](../audit/01a-clsdicc-cgarbage.md).

---

### Capa 3: Persistencia de Datos e I/O de Disco (Categoría Crítica 1)

#### 10. `FileIO` *(CATEGORÍA CRÍTICA 1 - PERSISTENCIA DE PERSONAJES - COMPLETADO)*
- **Archivos Legacy**: `legacy/server/Codigo/FileIO.bas`
- **Propósito**: Persistencia de archivos de personaje (`.chr`), mapas binarios (`.map`, `.inf`), tablas de datos (`OBJ.dat`, `NPCs.dat`, `Hechizos.dat`), configuración `Server.ini` y backups `DoBackUp`.
- **Archivo C++ Propuesto**: `src/server/FileIO.hpp` / `src/server/FileIO.cpp`
- **Estado**: **Completado** (Documentación en [`10-fileio-persistencia-personajes.md`](10-fileio-persistencia-personajes.md), [`10-fileio-configuracion-servidor.md`](10-fileio-configuracion-servidor.md), [`10-fileio-tablas-datos.md`](10-fileio-tablas-datos.md), [`10-fileio-mapas.md`](10-fileio-mapas.md) y [`10-fileio-backup-logging.md`](10-fileio-backup-logging.md)).
- **Dependencias**: `Declares`, `clsIniReader`, `Matematicas`.
- **Estimación**: **Grande** (~2.246 líneas, 38 rutinas).
- **Estrategia de Desglose y Verificación**: **Desglosado en 7 grupos lógicos secuenciales** (ver especificación detallada en [`10-fileio-breakdown.md`](10-fileio-breakdown.md)):
  1. *Paso 1 (G1 — Base)*: Utilidades base de archivos e INI (`GetVar`, `WriteVar`, `TxtDimension`, `ReadField`) — **✅ COMPLETADO**.
  2. *Paso 2 (G2 — Crítico)*: Persistencia de personajes `.chr` (`SaveUser`, `LoadUserInit`, etc.) — **✅ COMPLETADO** (Verificación byte a byte contra fixtures en `tests/fixtures/charfile/` con doctest, 7/7 casos pasados).
  3. *Paso 3 (G3 — Config)*: Configuración del servidor (`LoadSini`, `EsAdmin`, etc. en `Server.ini`) — **✅ COMPLETADO**.
  4. *Paso 4 (G5 — Tablas)*: Carga de tablas de datos e inicialización de juego (`Dat/*.dat`) — **✅ COMPLETADO**.
  5. *Paso 5 (G4 — Mapas)*: Carga y guardado de mapas binarios e INI (`.map`, `.inf`, `.dat`) — **✅ COMPLETADO** (Verificación byte a byte contra mapas reales 1, 4, 8 y 15 con doctest).
  6. *Paso 6 (G7 — Logs)*: Logging administrativo y sanciones (`LogBan`, `LogBanFromName`, `Ban`) — **✅ COMPLETADO** (3/3 casos pasados, 15 aserciones).
  7. *Paso 7 (G6 — Backup)*: Sistema de respaldos de mundo (`DoBackUp`, `CargarBackUp`, `BackUPnPc`, `CargarNpcBackUp`) — **✅ COMPLETADO** (3/3 casos pasados, 35 aserciones, fixtures reales de WorldBackup).

#### 11. `clsClan` y `modGuilds` *(CATEGORÍA CRÍTICA 1 - PERSISTENCIA DE CLANES)* — **✅ COMPLETADO**
- **Archivos Legacy**: `legacy/server/Codigo/clsClan.cls`, `legacy/server/Codigo/modGuilds.bas`
- **Propósito**: Administración de clanes, lista global de clanes y persistencia en disco de archivos de clanes (`guildsinfo.inf`, `.mem`, `.sol`, `.rel`).
- **Archivo C++ Implementado**: `src/server/clsClan.hpp` / `src/server/clsClan.cpp`, `src/server/modGuilds.hpp` / `src/server/modGuilds.cpp`
- **Estado**: **Completado** (Documentación en [`11-clsclan-breakdown.md`](11-clsclan-breakdown.md) y [`11-clsclan-modguilds.md`](11-clsclan-modguilds.md)).
- **Dependencias**: `Declares`, `FileIO`, `clsIniReader`, `cSolicitud`, `clsdicc`.
- **Estimación**: **Grande** (~2.503 líneas combinadas, desglosado en 7 grupos lógicos).
- **Estrategia de Verificación**: **Verificación byte a byte contra fixtures reales en `tests/fixtures/guilds/real/` (`guildsinfo.inf`, `Game Masters-members.mem`, `Game Masters-solicitudes.sol`) con pruebas en doctest** (6/6 casos dedicados en `tests/test_clsclan.cpp`, 69/69 casos totales pasados).
- **Resolución de Empates en `ContarVotos`**: Verificado end-to-end con reproducción verbatim del bug legacy original: no se asigna nuevo líder, el líder actual se mantiene intacto, se elimina el archivo temporal `.vot` y el anuncio en `GuildNews` reproduce el texto original donde la cantidad de empatados se reporta erróneamente como cantidad de votos.
- **Frontera de Serialización de `cSolicitud`**: Documentado y testeado el mapeo explícito e inmutable entre claves INI en disco (`Nombre` / `Detalle`) y miembros del struct C++ (`UserName` / `desc`).

---

### Capa 4: Infraestructura de Red y Múltiples Conexiones (Categoría Crítica 2)

#### 12. `SecurityIp` (PARCIAL)
- **Archivos Legacy**: `legacy/server/Codigo/SecurityIp.bas`
- **Propósito**: Filtrado de IPs, control anti-flood, límites de conexiones por IP (`MaxConnectionsPerIP`) y baneo de IP.
- **Archivo C++ Implementado**: `src/server/SecurityIp.hpp` / `src/server/SecurityIp.cpp`
- **Estado**: **Parcial** (Detalle completo en [`docs/implementation/12-securityip.md`](12-securityip.md)).
- **Dependencias**: *Ninguna* (en el subconjunto anti-flood implementado; las dependencias de red y logging corresponden a las rutinas diferidas).
- **Estimación**: **Mediano** (~350 líneas).
- **Implementado**:
  - `InitIpTables(OptCountersValue)`: Únicamente la porción de `IpTables` (capacidad base, dimensionamiento y reseteo de contadores).
  - `IpSecurityMantenimientoLista()`: Limpieza horaria y reescalado dinámico preservando el quirk de asimetría histórica.
  - `IpSecurityAceptarNuevaConexion(ip)`: Control de intervalo mínimo (1000 ms) y marcas temporales.
  - Ayudantes privados transliterados literalmente: `FindTableIp` y `AddNewIpIntervalo` (reproduciendo fielmente la cota inicial `Last = MaxValue` y la inserción desordenada por `~(Middle * 2)`).
  - Desbordamiento aritmético de ticks: Salvaguardado mediante [`SecurityIp::TickCountOverflowException`](src/server/SecurityIp.hpp) (reproduciendo el Error 6 "Overflow" del binario legacy compilado con `OverflowCheck=0` en `SERVER.VBP`) en lugar de incurrir en comportamiento indefinido (*Undefined Behavior*) por desbordamiento de enteros con signo en C++.
- **Diferido y Justificación**:
  - `IPSecuritySuperaLimiteConexiones` e `IpRestarConexion`: Ambas rutinas constituyen código muerto en el servidor de producción 0.13.0 (comentadas con apóstrofe en todos sus puntos de invocación en `wskapiAO.bas:433`, `wskapiAO.bas:466` y `TCP.bas:626` según la auditoría [`docs/audit/02a-securityip-detalle.md`](../audit/02a-securityip-detalle.md) §7 y [`02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md)). Quedaron formalmente **excluidas por código muerto** en la etapa de TCP.
  - `DumpTables`: Comando administrativo de diagnóstico (`SecurityIp.bas:314-327`). **Completado** en el Paso 7 de TCP mediante `SecurityIp::DumpTables(log_sink)`, consumiendo `TCP::GetAscIP` para formatear las direcciones IP de `IpTables` y ofreciendo un sink inyectable para el logging.
- **Registro de Bugs y Quirks Históricos**:
  - Consultar [`docs/implementation/12-securityip.md`](12-securityip.md) para la documentación exhaustiva del módulo.
  - Entradas del ledger maestro (#11 a #16) documentadas en [`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md).
- **Estrategia de Verificación**: Pruebas unitarias en C++ con **doctest** en [`tests/test_securityip.cpp`](../../tests/test_securityip.cpp) (traza manual del bug de inserción desordenada, validación de anti-flood con reloj determinista, y reproducción fiel del Error 6 con invariancia absoluta de estado).

#### 13. `clsAntiMassClon` (EXCLUIDO - Código Muerto)
- **Archivos Legacy**: `legacy/server/Codigo/clsAntiMassClon.cls`
- **Diagnóstico de Auditoría**: **100% código muerto, inoperante e incompilable** en producción 0.13.0 (`SeguridadAlkon` nunca estuvo definido en `SERVER.VBP:76`, la inserción de IPs nunca compilaba y la clase `UserIpAdress` no existe en el código fuente).
- **Decisión de Porting**: **EXCLUIDO.** No se generará ningún archivo C++ equivalente (`src/server/clsAntiMassClon.hpp` / `src/server/clsAntiMassClon.cpp`). Se retiran stubs y variables de `Declares` y se omite en módulos consumidores.
- **Documentación de Auditoría**: [`docs/audit/02b-antimassclon-detalle.md`](../audit/02b-antimassclon-detalle.md) y [`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md) (Entrada #18).

#### 14. `TCP` (con standalone Asio) *(CATEGORÍA CRÍTICA 2 - CONEXIONES SIMULTÁNEAS)*
- **Archivos Legacy**: `legacy/server/Codigo/TCP.bas` (absorbe la totalidad de `wsksock.bas` y `wskapiAO.bas`)
- **Propósito**: Capa de red multijugador basada en **standalone Asio Monohilo** (`io_context.poll()`). Maneja la asignación y blanqueo de slots en `UserList`, escucha y aceptación asíncrona, filtrado anti-flood síncrono (Bug #20 mitigado), recepción y despacho seguro con cola saliente (Bug #19 mitigado), ciclo de desconexión y retención de entidad Anti-CombatLog por 10 segundos en zonas PK.
- **Archivo C++ Implementado**: `src/server/TCP.hpp` / `src/server/TCP.cpp`
- **Estado**: **✅ COMPLETADO** (Ver documentación técnica en [`14-tcp.md`](14-tcp.md) y desglose por fases en [`14-tcp-breakdown.md`](14-tcp-breakdown.md)).
- **Dependencias**: `Declares`, `clsByteQueue`, `SecurityIp`.
- **Estimación**: **Grande** (~3.276 líneas legacy unificadas).
- **Estrategia de Verificación**: 24 tests unitarios específicos con **doctest** en [`tests/test_tcp.cpp`](../../tests/test_tcp.cpp) cubriendo los 7 grupos funcionales (G1 a G7) con sockets de loopback y avance temporal determinista.
- **Notas de Diseño y Mitigación**:
  - *Modelo Monohilo*: `TCP::PollRed()` se ejecuta en el mismo hilo que el Game Loop, preservando la fidelidad al STA de VB6 y la coherencia del estado global sin locks ni condiciones de carrera.
  - *Mitigación de Backpressure (Bug #19)*: Se erradica el busy-loop infinito de `NOT_ENOUGH_SPACE` + `Resume` mediante colas salientes asíncronas y corte preventivo a los 64 KB (`MAX_OUTGOING_BUFFER_SIZE`).
  - *Mitigación de Socket Leak (Bug #20)*: El cierre RAII explícito de `asio::ip::tcp::socket` ante rechazos de `SecurityIp` previene descriptores huérfanos.
  - *Mecánica Anti-CombatLog*: `CloseSocketSL` destruye el socket TCP pero retiene la entidad en mapa durante 10 segundos antes del volcado a disco y reseteo definitivo del slot con `CloseSocket`.

#### 15. `modSendData` *(CATEGORÍA CRÍTICA 2 - PROTOCOLO DE RED)* — **✅ COMPLETADO**
- **Archivos Legacy**: `legacy/server/Codigo/modSendData.bas`
- **Propósito**: Despacho y broadcast de paquetes de red (`SendData`, `SendToAll`, `SendToArea`, `SendToUserArea`).
- **Archivo C++ Implementado**: `src/server/modSendData.hpp` / `src/server/modSendData.cpp`
- **Estado**: **Completado** (Documentación en [`15-modsenddata.md`](15-modsenddata.md) y desglose en [`15-modsenddata-breakdown.md`](15-modsenddata-breakdown.md)).
- **Dependencias**: `Declares`, `clsByteQueue`, `TCP`.
- **Estimación**: **Mediano** (~650 líneas, 22 rutinas).
- **Estrategia de Verificación**: 12 pruebas unitarias y 54 assertions en `tests/test_modsenddata.cpp` bajo **doctest**.
- **Aspectos Clave de Implementación**:
  - *Zero-Copy Broadcasting*: Despacho de paquetes mediante `std::span<const uint8_t>` y sobrecargas de `std::string_view` sin copias redundantes.
  - *Bypass de `outgoingData`*: Despacho directo a `TCP::EnviarDatosASlot`, evitando sobrecargar las colas individuales de usuario.
  - *Corrección de Bitmasks*: Exponenciaciones de punto flotante `2 ^ (X \ 9)` reemplazadas por desplazamientos de bits enteros `1 << (X / 9)`.
  - *Mitigación de Bugs*: Erradicación de `SendTarget.ToGM` (código muerto huérfano, Bug #21) y verificación obligatoria de `flags.UserLogged` en difusiones globales y faccionarias para prevenir fugas de paquetes en pre-login (Bug #22).

#### 16. `Protocol` *(CATEGORÍA CRÍTICA 2 - DECODIFICADOR Y ENCODIFICADOR)*
- **Archivos Legacy**: `legacy/server/Codigo/Protocol.bas`
- **Propósito**: Decodificación binaria de paquetes entrantes (`ClientPacketID`) y serialización de paquetes salientes (`ServerPacketID`).
- **Archivo C++ Propuesto**: `src/server/Protocol.hpp` / `src/server/Protocol.cpp`
- **Dependencias**: `Declares`, `clsByteQueue`, `modSendData`, `TCP`.
- **Estimación**: **Grande** (~8.500 líneas).
- **Estrategia de Verificación**: **Pruebas binarias con cliente VB6 autenticando, caminando y enviando comandos al servidor C++**.
- **Nota de Migración (`clsByteQueue` y transacciones con `CopyBuffer`)**: `Protocol.bas` utiliza `buffer.CopyBuffer(incomingData)` para simular lectura transaccional de paquetes con strings variables. Si salta la excepción `NotEnoughDataException` (`NOT_ENOUGH_DATA`), el paquete está incompleto y la cola `incomingData` principal permanece inalterada hasta recibir el paquete completo TCP. Ver [`08-clsbytequeue.md`](08-clsbytequeue.md).
- **Nota de Auditoría / Migración (`FlushBuffer` y Erradicación del Bug #19)**: La función `FlushBuffer(UserIndex)` delega directamente en `TCP::FlushBuffer(user_index)`. En C++ **no debe reproducirse el patrón legacy de captura de `NOT_ENOUGH_SPACE` con `Resume`**, dado que la protección contra saturación fue resuelta a nivel de transporte en `TCP::EnviarDatosASlot` con búferes salientes asíncronos y backpressure seguro (ver [`14-tcp.md`](14-tcp.md) y [`KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md) Entrada #19).
- **Nota de Auditoría / Migración (`clsAntiMassClon` / Anti-Clon)**: La comprobación `aClon.MaxPersonajes(UserList(UserIndex).ip)` en `HandleLoginNewChar` (`Protocol.bas:1502`) no debe invocarse por tratarse de código muerto omitido (ver [`docs/audit/02b-antimassclon-detalle.md`](../audit/02b-antimassclon-detalle.md)).

---

### Capa 5: Sistema de Áreas de Mapa

#### 17. `ModAreas`
- **Archivos Legacy**: `legacy/server/Codigo/ModAreas.bas`
- **Propósito**: Partición del mapa en grillas de 9 áreas para optimizar el envío de paquetes de red solo a jugadores visibles.
- **Archivo C++ Propuesto**: `src/server/ModAreas.hpp` / `src/server/ModAreas.cpp`
- **Dependencias**: `Declares`, `modSendData`.
- **Estimación**: **Mediano** (~450 líneas).
- **Estrategia de Verificación**: Pruebas con múltiples clientes VB6 observando el ingreso/egreso de personajes al área de visión.

---

### Capa 6: Subsistemas de Objetos, Inventario y Bóveda

#### 18. `Modulo_InventANDobj`
- **Archivos Legacy**: `legacy/server/Codigo/Modulo_InventANDobj.bas`
- **Propósito**: Carga de la tabla `OBJ.dat`, instanciación de objetos en el mapa y tirado de ítems al suelo.
- **Archivo C++ Propuesto**: `src/server/Modulo_InventANDobj.hpp` / `src/server/Modulo_InventANDobj.cpp`
- **Dependencias**: `Declares`, `FileIO`, `ModAreas`, `modSendData`.
- **Estimación**: **Mediano** (~300 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 (agarrar y tirar ítems al suelo).

#### 19. `InvUsuario`
- **Archivos Legacy**: `legacy/server/Codigo/InvUsuario.bas`
- **Propósito**: Gestión del inventario de cada jugador: equipar armas/armaduras/cascos/escudos, mover ítems de slot y usar objetos.
- **Archivo C++ Propuesto**: `src/server/InvUsuario.hpp` / `src/server/InvUsuario.cpp`
- **Dependencias**: `Declares`, `Modulo_InventANDobj`, `modSendData`, `Protocol`.
- **Estimación**: **Grande** (~1.600 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 (equipamiento e interacción con inventario).

#### 20. `modBanco`
- **Archivos Legacy**: `legacy/server/Codigo/modBanco.bas`
- **Propósito**: Depósito y retiro de objetos y oro en la bóveda bancaria del personaje.
- **Archivo C++ Propuesto**: `src/server/modBanco.hpp` / `src/server/modBanco.cpp`
- **Dependencias**: `Declares`, `InvUsuario`, `modSendData`.
- **Estimación**: **Mediano** (~350 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 interactuando con el NPC Banquero.

#### 21. `Comercio` y `mdlCOmercioConUsuario`
- **Archivos Legacy**: `legacy/server/Codigo/Comercio.bas`, `legacy/server/Codigo/mdlCOmercioConUsuario.bas`
- **Propósito**: Comercio de objetos con comerciantes NPC (`Comercio.bas`) y comercio seguro directo entre dos jugadores (`mdlCOmercioConUsuario.bas`).
- **Archivo C++ Propuesto**: `src/server/Comercio.hpp` / `src/server/Comercio.cpp`, `src/server/mdlCOmercioConUsuario.hpp` / `src/server/mdlCOmercioConUsuario.cpp`
- **Dependencias**: `Declares`, `InvUsuario`, `modSendData`, `Modulo_InventANDobj`.
- **Estimación**: **Mediano** (~800 líneas combinadas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 (compra/venta en NPC y comercio entre 2 usuarios).

---

### Capa 7: Lógica de Combate, Magia, Facciones y Oficios

#### 22. `SistemaCombate`
- **Archivos Legacy**: `legacy/server/Codigo/SistemaCombate.bas`
- **Propósito**: Fórmulas matemáticas de combate: daño físico (`CalcularDaño`), evasión (`ProbabilidadGolpe`), ataques cuerpo a cuerpo (`UsuarioAtaca`, `NpcAtacaUsuario`) y defensa de armaduras.
- **Archivo C++ Propuesto**: `src/server/SistemaCombate.hpp` / `src/server/SistemaCombate.cpp`
- **Dependencias**: `Declares`, `Matematicas`, `InvUsuario`, `modSendData`, `ModAreas`.
- **Estimación**: **Grande** (~2.100 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 realizando ataques a criaturas y jugadores.

#### 23. `modHechizos`
- **Archivos Legacy**: `legacy/server/Codigo/modHechizos.bas`
- **Propósito**: Sistema de magia: casteo de hechizos (`LanzarHechizo`), validación de objetivo, daño mágico, curación, parálisis, invisibilidad y convocaciones.
- **Archivo C++ Propuesto**: `src/server/modHechizos.hpp` / `src/server/modHechizos.cpp`
- **Dependencias**: `Declares`, `SistemaCombate`, `InvUsuario`, `modSendData`, `ModAreas`.
- **Estimación**: **Grande** (~2.200 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 (lanzamiento de hechizos y efectos en pantalla).

#### 24. `modInvisibles`
- **Archivos Legacy**: `legacy/server/Codigo/modInvisibles.bas`
- **Propósito**: Control de estados de invisibilidad y ocultamiento de personajes.
- **Archivo C++ Propuesto**: `src/server/modInvisibles.hpp` / `src/server/modInvisibles.cpp`
- **Dependencias**: `Declares`, `modSendData`.
- **Estimación**: **Chico** (~50 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6.

#### 25. `ModFacciones`
- **Archivos Legacy**: `legacy/server/Codigo/ModFacciones.bas`
- **Propósito**: Sistema de alineación y facciones (Ejército Real vs Fuerzas del Caos), estado de criminal o ciudadano, jerarquías y puntos de facción.
- **Archivo C++ Propuesto**: `src/server/ModFacciones.hpp` / `src/server/ModFacciones.cpp`
- **Dependencias**: `Declares`, `modSendData`, `FileIO`.
- **Estimación**: **Mediano** (~800 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 (cambio de status criminal y jerarquías).
- **Nota de Auditoría / Propagación Cruzada (Variables de Vestimentas, Armaduras Faccionarias y Recompensas de Facción)**:
  1. **Túnicas e Ítems Faccionarios (Grupo 3 FileIO)**: Las 24 variables globales de ítems de armaduras y túnicas faccionarias declaradas originalmente en `ModFacciones.bas:34-58` (`ArmaduraImperial1..3`, `TunicaMagoImperial`, `TunicaMagoImperialEnanos`, `ArmaduraCaos1..3`, `TunicaMagoCaos`, `TunicaMagoCaosEnanos`, `VestimentaImperialHumano`, `VestimentaImperialEnano`, `TunicaConspicuaHumano`, `TunicaConspicuaEnano`, `ArmaduraNobilisimaHumano`, `ArmaduraNobilisimaEnano`, `ArmaduraGranSacerdote`, `VestimentaLegionHumano`, `VestimentaLegionEnano`, `TunicaLobregaHumano`, `TunicaLobregaEnano`, `TunicaEgregiaHumano`, `TunicaEgregiaEnano`, `SacerdoteDemoniaco`) son pobladas desde `Server.ini` durante el arranque por `FileIO.cpp` (`LoadSini()`).
  2. **Defensas de Armaduras Faccionarias y Recompensas por Rango (Grupo 5 FileIO)**: La estructura `tFaccionArmaduras`, la enumeración `eTipoDefArmors`, la constante `NUM_RANGOS_FACCION` (15), el arreglo 2D/3D `ArmadurasFaccion(1 To NUMCLASES, 1 To NUMRAZAS)` y el arreglo de experiencia `RecompensaFacciones(NUM_RANGOS_FACCION)` declarados originalmente en `ModFacciones.bas:63,72-81` son poblados desde `Dat/ArmadurasFaccionarias.dat` (`LoadArmadurasFaccion()`) y `Dat/Balance.dat` (`LoadBalance()`) por `FileIO.cpp`.
  Todas estas variables ya se encuentran declaradas e instanciadas en `Declares.hpp` / `Declares.cpp`. Al portar `ModFacciones`, deben consumirse desde `Declares.hpp` sin volver a declararlas. Ver [`10-fileio-configuracion-servidor.md`](10-fileio-configuracion-servidor.md) y [`10-fileio-tablas-datos.md`](10-fileio-tablas-datos.md).

#### 26. `Trabajo` *(Falta auditoría detallada)*
- **Archivos Legacy**: `legacy/server/Codigo/Trabajo.bas`
- **Propósito**: Sistema de oficios y recolección: herrería, carpintería, minería, pesca y tala de árboles.
- **Archivo C++ Propuesto**: `src/server/Trabajo.hpp` / `src/server/Trabajo.cpp`
- **Dependencias**: `Declares`, `InvUsuario`, `Modulo_InventANDobj`, `modSendData`, `Matematicas`.
- **Estimación**: **Grande** (~2.300 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 (talar, minar, pescar y construir ítems).

---

### Capa 8: Inteligencia Artificial y Gestión de NPCs

#### 27. `PathFinding`
- **Archivos Legacy**: `legacy/server/Codigo/PathFinding.bas`
- **Propósito**: Algoritmos de búsqueda de caminos (BFS) para movimiento de NPCs esquivando obstáculos en la grilla.
- **Archivo C++ Propuesto**: `src/server/PathFinding.hpp` / `src/server/PathFinding.cpp`
- **Dependencias**: `Declares`.
- **Nota de Migración (`Queue.bas`)**: Al portar este módulo, la funcionalidad de `Queue.bas` (de la cual depende `PathFinding.bas`) ya quedó resuelta en la auditoría de Capa 0: no debe crearse una estructura ni módulo global/estático `Queue`. Debe implementarse como un `std::queue<tVertice>` local circunscrito al ámbito de la función `SeekPath`, en consonancia con el modelo de ejecución monohilo monobúsqueda confirmado en [`docs/implementation/04-modcola-queue-colaarray.md`](04-modcola-queue-colaarray.md). La estructura plana `tVertice` ya reside en `Declares.hpp`.
- **Estimación**: **Mediano** (~300 líneas).
- **Estrategia de Verificación**: Pruebas unitarias en C++ con **doctest** y pruebas con cliente VB6.

#### 28. `MODULO_NPCs`
- **Archivos Legacy**: `legacy/server/Codigo/MODULO_NPCs.bas`
- **Propósito**: Ciclo de vida de NPCs: spawn de criaturas, muerte (`MuereNpc`), dropeo de botín y otorgamiento de experiencia (`GiveEXP`).
- **Archivo C++ Propuesto**: `src/server/MODULO_NPCs.hpp` / `src/server/MODULO_NPCs.cpp`
- **Dependencias**: `Declares`, `FileIO`, `Modulo_InventANDobj`, `ModAreas`, `modSendData`, `Matematicas`.
- **Estimación**: **Grande** (~1.000 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 (aparición y muerte de NPCs).
- **Nota de Auditoría / Propagación Cruzada (Carga y Guardado de NPCs en Mapas)**:
  En el archivo binario `.inf`, el registro de NPC persiste el número de plantilla/tipo (`NpcNumber`, ej. 502, 536). En el legacy (`FileIO.bas:1410-1434`), `CargarMapa` lee temporalmente dicho número e invoca `OpenNPC(.NpcIndex)` para instanciar el NPC en el arreglo `Npclist(1 To MAXNPCS)`, asignando `Orig` y `Pos` y llamando a `MakeNPCChar`. Al serializar el mapa (`FileIO.bas:518-520`), `GrabarMapa` recupera y escribe el número de plantilla original mediante `Npclist(.NpcIndex).Numero`.
  En la Capa 2 / 3 (`FileIO.cpp`), para mantener el módulo desacoplado antes de la migración de `MODULO_NPCs`, se almacena directamente el número en `MapData[...].NpcIndex` y se replica en `Npclist`. Al implementar `MODULO_NPCs`, se debe conectar `OpenNPC` con la deserialización de mapas asegurando la correspondencia exacta entre el índice de runtime de `Npclist` y el número de plantilla en disco. Ver [`docs/implementation/10-fileio-mapas.md`](10-fileio-mapas.md).


#### 29. `AI_NPC`
- **Archivos Legacy**: `legacy/server/Codigo/AI_NPC.bas`
- **Propósito**: Inteligencia Artificial de NPCs (`NPC_AI` ejecutado periódicamente): persecución de jugadores, agresión, huida y ataque.
- **Archivo C++ Propuesto**: `src/server/AI_NPC.hpp` / `src/server/AI_NPC.cpp`
- **Dependencias**: `Declares`, `MODULO_NPCs`, `PathFinding`, `SistemaCombate`, `modHechizos`, `ModAreas`.
- **Estimación**: **Grande** (~1.200 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 observando comportamiento de agresión y persecución de NPCs.

#### 30. `praetorians` *(Falta auditoría detallada)*
- **Archivos Legacy**: `legacy/server/Codigo/praetorians.bas`
- **Propósito**: IA de guardias pretorianos y defensores de ciudades contra criminales.
- **Archivo C++ Propuesto**: `src/server/praetorians.hpp` / `src/server/praetorians.cpp`
- **Dependencias**: `Declares`, `AI_NPC`, `MODULO_NPCs`, `ModFacciones`.
- **Estimación**: **Grande** (~2.100 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 interactuando con guardias de ciudad.
- **Nota de Auditoría / Propagación Cruzada (`MAPA_PRETORIANO`)**: La constante/variable global `MAPA_PRETORIANO` (declarada originalmente en `praetorians.bas:40` para identificar el mapa de la fortaleza de los guardias pretorianos) es cargada desde `Server.ini` (`MapaPretoriano`) por `FileIO.cpp` (`LoadSini()`). Ya fue declarada e instanciada en `Declares.hpp` / `Declares.cpp`. Al portar `praetorians`, reutilizar `MAPA_PRETORIANO` desde `Declares.hpp` sin volver a declararla. Ver [`docs/implementation/10-fileio-configuracion-servidor.md`](10-fileio-configuracion-servidor.md).

---

### Capa 9: Sesión del Jugador, Parties y Posicionamiento

#### 31. `Characters`
- **Archivos Legacy**: `legacy/server/Codigo/Characters.bas`
- **Propósito**: Helper de posicionamiento y reseteo de coordenadas de personajes al morir o revivir en su ciudad origen (`Hogar`).
- **Archivo C++ Propuesto**: `src/server/Characters.hpp` / `src/server/Characters.cpp`
- **Dependencias**: `Declares`, `ModAreas`.
- **Estimación**: **Chico** (~70 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6.

#### 32. `clsParty` y `mdParty`
- **Archivos Legacy**: `legacy/server/Codigo/clsParty.cls`, `legacy/server/Codigo/mdParty.bas`
- **Propósito**: Sistema de grupos / party: creación de grupo, invitaciones, reparto equitativo de experiencia y canal de chat privado.
- **Archivo C++ Propuesto**: `src/server/clsParty.hpp` / `src/server/clsParty.cpp`, `src/server/mdParty.hpp` / `src/server/mdParty.cpp`
- **Dependencias**: `Declares`, `modSendData`, `Matematicas`.
- **Estimación**: **Mediano** (~900 líneas combinadas).
- **Estrategia de Verificación**: Pruebas con múltiples clientes VB6 en party.
- **Nota de Auditoría / Propagación Cruzada (Variable de Balance ExponenteNivelParty)**: La variable `ExponenteNivelParty` declarada originalmente en `mdParty.bas:67` (`Public ExponenteNivelParty As Single`) es poblada desde `Dat/Balance.dat` por `FileIO.cpp` (`LoadBalance()`). Ya se encuentra declarada e instanciada en `Declares.hpp` / `Declares.cpp`. Al portar `mdParty`, debe consumirse desde `Declares.hpp` sin volver a declararla. Ver [`10-fileio-tablas-datos.md`](10-fileio-tablas-datos.md).

#### 33. `Acciones`
- **Archivos Legacy**: `legacy/server/Codigo/Acciones.bas`
- **Propósito**: Procesamiento de interacciones del usuario en el mundo: clics en objetos, carteles, puertas, NPCs y disparadores (`triggers`).
- **Archivo C++ Propuesto**: `src/server/Acciones.hpp` / `src/server/Acciones.cpp`
- **Dependencias**: `Declares`, `InvUsuario`, `MODULO_NPCs`, `Comercio`, `modBanco`, `modSendData`, `ModAreas`.
- **Estimación**: **Mediano** (~400 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 interactuando con el mapa.
- **Nota de Auditoría / Migración (`cGarbage` / Supervivencia)**: Al portar la habilidad de Supervivencia (`CrearFuego`), acordate de incluir la lógica de instanciación y encolado de `cGarbage` en `TrashCollector` al encender una fogata (must include integration test coverage for TrashCollector/cGarbage cleanup behavior when this module is ported — see docs/audit/01a-clsdicc-cgarbage.md and docs/implementation/05-cgarbage.md).

#### 34. `Modulo_UsUaRiOs`
- **Archivos Legacy**: `legacy/server/Codigo/Modulo_UsUaRiOs.bas`
- **Propósito**: Orquestación del ciclo de vida del jugador: conexión/desconexión (`Cerrar_Usuario`), subida de nivel, ganancia de exp (`GiveEXP`), ticks de regeneración de HP/Maná (`RegenerarHP`, `RegenerarMana`) y validación de movimiento (`MoveUser`).
- **Archivo C++ Propuesto**: `src/server/Modulo_UsUaRiOs.hpp` / `src/server/Modulo_UsUaRiOs.cpp`
- **Dependencias**: `Declares`, `FileIO`, `Protocol`, `TCP`, `modSendData`, `SistemaCombate`, `InvUsuario`, `ModAreas`, `ModFacciones`.
- **Estimación**: **Grande** (~2.600 líneas).
- **Estrategia de Verificación**: Pruebas integrales de flujo de juego con cliente VB6 real.

---

### Capa 10: Herramientas de GM, Seguridad y Subsistemas Secundarios

#### 35. `Admin`
- **Archivos Legacy**: `legacy/server/Codigo/Admin.bas`
- **Propósito**: Comandos de consola de Game Master (`/telep`, `/summon`, `/ban`, `/ci`, `/acc`, `/invi`, `/invulnerable`), validación de rangos (`EsGM`).
- **Archivo C++ Propuesto**: `src/server/Admin.hpp` / `src/server/Admin.cpp`
- **Dependencias**: `Declares`, `Modulo_UsUaRiOs`, `MODULO_NPCs`, `Modulo_InventANDobj`, `modSendData`, `FileIO`.
- **Estimación**: **Mediano** (~500 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6 usando personaje GM.
- **Nota de Auditoría / Propagación Cruzada (Variables de Configuración, MOTD e Intervalos de Servidor)**:
  1. **Servidor y Red**: `BootDelBackUp` (flag de inicio desde backup) y `Puerto` (puerto TCP de escucha), declarados en `Admin.bas:86-88`, son poblados desde `Server.ini` por `FileIO.cpp` (`LoadSini()`).
  2. **Mensaje del Día**: La estructura `tMotd`, el vector global `MOTD` y `MaxLines`, declarados en `Admin.bas:32-38`, son poblados desde `Dat/Motd.ini` por `FileIO.cpp` (`LoadMotd()`).
  3. **Intervalos de Servidor**: Los 24 contadores e intervalos globales de refresco y combate (`SanaIntervaloSinDescansar`, `StaminaIntervaloSinDescansar`, `SanaIntervaloDescansar`, `StaminaIntervaloDescansar`, `IntervaloSed`, `IntervaloHambre`, `IntervaloVeneno`, `IntervaloParalizado`, `IntervaloInvisible`, `IntervaloFrio`, `IntervaloWavFx`, `IntervaloInvocacion`, `IntervaloParaConexion`, `IntervaloPuedeSerAtacado`, `IntervaloAtacable`, `IntervaloOwnedNpc`, `IntervaloUserPuedeCastear`, `IntervaloUserPuedeTrabajar`, `IntervaloUserPuedeAtacar`, `IntervaloMagiaGolpe`, `IntervaloGolpeMagia`, `IntervaloGolpeUsar`, `MinutosWs`, `IntervaloCerrarConexion`, `IntervaloUserPuedeUsar`, `IntervaloFlechasCazadores`, `IntervaloOculto`), declarados en `Admin.bas:51-85`, son poblados desde `Server.ini` (`[INTERVALOS]`) por `FileIO.cpp` (`LoadSini()`).
  4. **Balance y Apuestas (Grupo 5 FileIO)**: La variable `PorcentajeRecuperoMana` (declarada en `Admin.bas:83`) y la estructura/global `tAPuestas` / `Apuestas` (declaradas en `Admin.bas:40-45`) son pobladas desde `Dat/Balance.dat` (`LoadBalance()`) y `Dat/apuestas.dat` (`CargaApuestas()`) por `FileIO.cpp`.
  5. **Bans de IPs y WorldSave (Grupos 6 y 7 FileIO)**: La persistencia de `Dat/BanIps.dat` (`GuardarBanIps`, `CargarBanIps`) y la colección global `BanIps` pertenecen a `Admin.bas`, no a `FileIO`. Asimismo, la orquestación interna de `WorldSave` (`Admin.bas:134`) durante el proceso `DoBackUp` invoca directamente `FileIO::GrabarMapa` para los mapas con `BackUp = 1` y `FileIO::BackUPnPc` para los NPCs con `flags.BackUp = 1`.
  Todas estas variables ya se encuentran declaradas e instanciadas en `Declares.hpp` / `Declares.cpp`. Al portar `Admin`, deben reutilizarse desde `Declares.hpp` en lugar de volver a declararlas. Ver [`10-fileio-persistencia-personajes.md`](10-fileio-persistencia-personajes.md), [`10-fileio-configuracion-servidor.md`](10-fileio-configuracion-servidor.md), [`10-fileio-tablas-datos.md`](10-fileio-tablas-datos.md) y [`10-fileio-backup-logging.md`](10-fileio-backup-logging.md).
- **Nota de Auditoría / Migración (`modHexaStrings` / `MD5sCarga`)**: Al portar `MD5sCarga` y la validación `MD5ok`, recordar que `MD5s(LoopC) = txtOffset(hexMd52Asc(MD5s(LoopC)), 55)` depende de la conversión case-insensitive de `hexMd52Asc` sobre las entradas hexadecimales de `Server.ini` (`MD5AceptadoX`), la cual se compara sensible a mayúsculas/minúsculas con el buffer de 16 bytes recibido del cliente (`buffer.ReadASCIIStringFixed(16)`). Ver [`06-modhexastrings.md`](06-modhexastrings.md).


#### 36. `modCentinela` *(Falta auditoría detallada)*
- **Archivos Legacy**: `legacy/server/Codigo/modCentinela.bas`
- **Propósito**: Verificación interactiva anti-bot mediante preguntas matemáticas y de texto.
- **Archivo C++ Propuesto**: `src/server/modCentinela.hpp` / `src/server/modCentinela.cpp`
- **Dependencias**: `Declares`, `modSendData`, `Matematicas`.
- **Estimación**: **Mediano** (~350 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6.

#### 37. `modForum`
- **Archivos Legacy**: `legacy/server/Codigo/modForum.bas`
- **Propósito**: Tablero de mensajes / foros integrados dentro del juego por ciudad y por clan.
- **Archivo C++ Propuesto**: `src/server/modForum.hpp` / `src/server/modForum.cpp`
- **Dependencias**: `Declares`, `FileIO`, `modSendData`.
- **Estimación**: **Mediano** (~350 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6.
- **Nota de Auditoría**: Cobertura de auditoría profunda completada en [`docs/audit/11a-modforum-detalle.md`](../audit/11a-modforum-detalle.md).

#### 38. `ConsultasPopulares` *(Falta auditoría detallada)*
- **Archivos Legacy**: `legacy/server/Codigo/ConsultasPopulares.cls`
- **Propósito**: Sistema de votaciones y encuestas en el servidor.
- **Archivo C++ Propuesto**: `src/server/ConsultasPopulares.hpp` / `src/server/ConsultasPopulares.cpp`
- **Dependencias**: `Declares`, `FileIO`, `modSendData`.
- **Estimación**: **Chico** (~200 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6.

#### 39. `History` *(Falta auditoría detallada)*
- **Archivos Legacy**: `legacy/server/Codigo/History.bas`
- **Propósito**: Registro de noticias e historial del servidor.
- **Archivo C++ Propuesto**: `src/server/History.hpp` / `src/server/History.cpp`
- **Dependencias**: `Declares`, `FileIO`.
- **Estimación**: **Chico** (~150 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6.

#### 40. `Statistics` y `clsEstadisticasIPC` *(Falta auditoría detallada)*
- **Archivos Legacy**: `legacy/server/Codigo/Statistics.bas`, `legacy/server/Codigo/clsEstadisticasIPC.cls`
- **Propósito**: Recolección de métricas de rendimiento y estadísticas de tráfico del servidor.
- **Archivo C++ Propuesto**: `src/server/Statistics.hpp` / `src/server/Statistics.cpp`, `src/server/clsEstadisticasIPC.hpp` / `src/server/clsEstadisticasIPC.cpp`
- **Dependencias**: `Declares`.
- **Estimación**: **Mediano** (~400 líneas combinadas).
- **Estrategia de Verificación**: Diagnostics log / Pruebas en doctest.

#### 41. `clsMapSoundManager` *(Falta auditoría detallada)*
- **Archivos Legacy**: `legacy/server/Codigo/clsMapSoundManager.cls`
- **Propósito**: Disparador de efectos de audio ambiental según posición en mapa.
- **Archivo C++ Propuesto**: `src/server/clsMapSoundManager.hpp` / `src/server/clsMapSoundManager.cpp`
- **Dependencias**: `Declares`, `modSendData`.
- **Estimación**: **Chico** (~120 líneas).
- **Estrategia de Verificación**: Pruebas con cliente VB6.

---

### Capa 11: Game Loop Central y Bootstrap de Aplicación

#### 42. `modNuevoTimer` y `GameLogic` (absorbe timers de `frmMain.frm`)
- **Archivos Legacy**: `legacy/server/Codigo/modNuevoTimer.bas`, `legacy/server/Codigo/GameLogic.bas` (y lógica de timers de `frmMain.frm`)
- **Propósito**: Loop de ticks del motor (25 Hz / 40ms): ejecutor de `tNPCAI`, `tGameLoop`, `tCleanWorld` (limpieza de mapa) y `tSaveAuto` (backup periódico).
- **Archivo C++ Propuesto**: `src/server/modNuevoTimer.hpp` / `src/server/modNuevoTimer.cpp`, `src/server/GameLogic.hpp` / `src/server/GameLogic.cpp`
- **Dependencias**: `Declares`, `Modulo_UsUaRiOs`, `AI_NPC`, `MODULO_NPCs`, `FileIO`, `ModAreas`.
- **Estimación**: **Grande** (~1.200 líneas combinadas).
- **Estrategia de Verificación**: Pruebas de tiempo de ejecución del servidor C++ y cliente VB6 real.
- **Nota de Auditoría / Migración (`clsAntiMassClon` / AutoSave)**: La llamada periódica `aClon.VaciarColeccion` en el timer de autoguardado (`AutoSave_Timer` / `DoBackUp`, `frmMain.frm:405`) no debe implementarse por haber sido excluido el módulo como código muerto (ver [`docs/audit/02b-antimassclon-detalle.md`](../audit/02b-antimassclon-detalle.md)).
- **Nota de Migración (`TCP` / Game Loop)**: El bucle central de ejecución debe invocar `TCP::PollRed()` al inicio de cada frame para procesar de forma no bloqueante todos los eventos asíncronos de E/S de Asio. Asimismo, en el temporizador de 1 segundo (`General.PasarSegundo`) se debe invocar `TCP::PasarSegundoUsuarios()` para gestionar la cuenta regresiva de logout y la mecánica Anti-CombatLog. Ver [`14-tcp.md`](14-tcp.md).

#### 43. `General`
- **Archivos Legacy**: `legacy/server/Codigo/General.bas`
- **Propósito**: Punto de entrada principal (`Main`), inicialización global de todos los subsistemas (carga de mapas, INIs, hechizos, NPCs, escucha de sockets TCP) y apagado ordenado (`ShutdownServer`).
- **Archivo C++ Propuesto**: `src/server/General.hpp` / `src/server/General.cpp` (complementa a `src/server/main.cpp`)
- **Dependencias**: *Todos los módulos del servidor previamente migrados*.
- **Estimación**: **Grande** (~1.467 líneas).
- **Estrategia de Verificación**: Ejecución del servidor completo en C++ (`ArgentumServer.exe`) recibiendo conexiones del cliente VB6 real.
- **Nota de Auditoría / Migración (`cGarbage` / `LimpiarMundo`)**: Al portar el procedimiento de mantenimiento `LimpiarMundo`, acordate de incluir la rutina de recorrido y descolado de la colección `TrashCollector` para remover del mapa los objetos `cGarbage` (fogatas) (must include integration test coverage for TrashCollector/cGarbage cleanup behavior when this module is ported — see docs/audit/01a-clsdicc-cgarbage.md and docs/implementation/05-cgarbage.md).
- **Nota de Migración (`TCP` / Bootstrap y Shutdown)**: El procedimiento `Main` arranca la escucha de red invocando `TCP::IniciaServidor(Puerto, bind_ip)`, y `ShutdownServer` cierra ordenadamente todos los sockets y libera descriptores mediante `TCP::DetenerServidor()`. Ver [`14-tcp.md`](14-tcp.md).

---

## 5. Tabla Resumen de la Matriz de Porting

| Capa | Módulo Legacy | Archivo C++ Propuesto | Estimación | Categoría Crítica / Requisito | Estrategia de Verificación |
| :---: | :--- | :--- | :---: | :--- | :--- |
| **0** | `Matematicas.bas` | `src/server/Matematicas.hpp` | Chico | Base (Completado) | Pruebas Unitarias doctest |
| **0** | `clsIniReader.cls` | `src/server/clsIniReader.hpp` | Chico | Base | Pruebas Unitarias doctest |
| **0** | `clsdicc.cls` | `src/server/clsdicc.hpp` | Chico | Adaptador `std::map` (Audit. Pendiente) | Pruebas Unitarias doctest |
| **0** | `ModCola`, `Queue` (`cColaArray.cls` EXCLUIDO) | `src/server/ModCola.hpp`, `src/server/Queue.hpp` | Chico | Queues internas (Ver `06a-colaarray-dead-code.md`) | Pruebas Unitarias doctest |
| **0** | `cGarbage.cls` | `src/server/cGarbage.hpp` | Chico | Parcial (verificación diferida) | Compilación limpia |
| **0** | `modHexaStrings.bas` | `src/server/modHexaStrings.hpp` | Chico | Hex / Strings | Pruebas Unitarias doctest |
| **0** | `cSolicitud.cls` | `src/server/cSolicitud.hpp` | Chico | Solicitudes Clan | doctest + Fixtures `.sol` |
| **1** | `clsByteQueue.cls` | `src/server/clsByteQueue.hpp` | Mediano | 🚨 **CRÍTICO 2: Red Binaria** | doctest + Socket VB6 |
| **0** | `Declares.bas` | `src/server/Declares.hpp` | Grande | Estado Global (Completado) | Compilación C++ (`server_core`) |
| **3** | `FileIO.bas` | `src/server/FileIO.hpp` | Grande | 🚨 **CRÍTICO 1: Persistencia** (7 pasos lógicos) | **Ver [`10-fileio-breakdown.md`](10-fileio-breakdown.md)** (doctest + Fixtures `charfile/`) |
| **3** | `clsClan.cls` / `modGuilds.bas` | `src/server/modGuilds.hpp` | Grande | 🚨 **CRÍTICO 1: Clanes** | **doctest + Fixtures Byte-Exact `guilds/`** |
| **4** | `SecurityIp.bas` | `src/server/SecurityIp.hpp` | Mediano | Security / Anti-Flood (Parcial) | doctest + Multicliente VB6 |
| **4** | `clsAntiMassClon.cls` | *Ninguno (Excluido)* | Chico | **EXCLUIDO (Código Muerto)** | Documentado en [`02b-antimassclon-detalle.md`](../audit/02b-antimassclon-detalle.md) |
| **4** | `TCP.bas` (standalone Asio) | `src/server/TCP.hpp` | Grande | 🚨 **CRÍTICO 2: Multi-Conexión**| **Completado (Asio Monohilo)** (24 tests en `test_tcp.cpp`, ver [`14-tcp.md`](14-tcp.md)) |
| **4** | `modSendData.bas` | `src/server/modSendData.hpp` | Mediano | 🚨 **CRÍTICO 2: Broadcast** | **Completado (Zero-Copy)** (12 tests en `test_modsenddata.cpp`, ver [`15-modsenddata.md`](15-modsenddata.md) y [`15-modsenddata-breakdown.md`](15-modsenddata-breakdown.md)) |
| **4** | `Protocol.bas` | `src/server/Protocol.hpp` | Grande | 🚨 **CRÍTICO 2: Opcodes** | Login / Movimiento cliente VB6 |
| **5** | `ModAreas.bas` | `src/server/ModAreas.hpp` | Mediano | Grilla de Visión | 2+ Clientes VB6 en mapa |
| **6** | `Modulo_InventANDobj.bas` | `src/server/Modulo_InventANDobj.hpp` | Mediano | Objetos Mapa | Tirar/agarrar ítem cliente VB6 |
| **6** | `InvUsuario.bas` | `src/server/InvUsuario.hpp` | Grande | Inventario Jugador | Equipar y usar ítems cliente VB6 |
| **6** | `modBanco.bas` | `src/server/modBanco.hpp` | Mediano | Bóveda | NPC Banquero cliente VB6 |
| **6** | `Comercio.bas` / `mdlCOmercio...` | `src/server/Comercio.hpp` | Mediano | Comercio NPC / User | Ventana comercio cliente VB6 |
| **7** | `SistemaCombate.bas` | `src/server/SistemaCombate.hpp` | Grande | Fórmulas Combate | Ataque a NPC/User cliente VB6 |
| **7** | `modHechizos.bas` | `src/server/modHechizos.hpp` | Grande | Magia y Hechizos | Casteo hechizos cliente VB6 |
| **7** | `modInvisibles.bas` | `src/server/modInvisibles.hpp` | Chico | Invisibilidad | Cliente VB6 real |
| **7** | `ModFacciones.bas` | `src/server/ModFacciones.hpp` | Mediano | Alineación | Facciones cliente VB6 |
| **7** | `Trabajo.bas` | `src/server/Trabajo.hpp` | Grande | Oficios y Recolección | Minar/talar cliente VB6 |
| **8** | `PathFinding.bas` | `src/server/PathFinding.hpp` | Mediano | Pathfinding A* | doctest + Cliente VB6 |
| **8** | `MODULO_NPCs.bas` | `src/server/MODULO_NPCs.hpp` | Grande | Spawn / Muerte NPC | Spawn criaturas cliente VB6 |
| **8** | `AI_NPC.bas` | `src/server/AI_NPC.hpp` | Grande | IA Criaturas | Persecución NPC cliente VB6 |
| **8** | `praetorians.bas` | `src/server/praetorians.hpp` | Grande | Guardias Ciudad | Interacción guardias cliente VB6 |
| **9** | `Characters.bas` | `src/server/Characters.hpp` | Chico | Respawn / Posición | Resucitar cliente VB6 |
| **9** | `clsParty` / `mdParty` | `src/server/clsParty.hpp` | Mediano | Grupos / Party | Party 2+ clientes VB6 |
| **9** | `Acciones.bas` | `src/server/Acciones.hpp` | Mediano | Clics en Mundo | Interacción mapa cliente VB6 |
| **9** | `Modulo_UsUaRiOs.bas` | `src/server/Modulo_UsUaRiOs.hpp` | Grande | Ciclo Vida Jugador | Gameplay continuo cliente VB6 |
| **10** | `Admin.bas` | `src/server/Admin.hpp` | Mediano | Comandos GM | Comandos `/telep` cliente VB6 |
| **10** | `modCentinela.bas` | `src/server/modCentinela.hpp` | Mediano | Anti-Bot | Prompt Centinela cliente VB6 |
| **10** | `modForum.bas` | `src/server/modForum.hpp` | Mediano | Foros Juego | Leer/escribir foro cliente VB6 |
| **10** | `ConsultasPopulares.cls` | `src/server/ConsultasPopulares.hpp` | Chico | Votaciones | Cliente VB6 real |
| **10** | `History.bas` | `src/server/History.hpp` | Chico | Historial | Logs cliente VB6 real |
| **10** | `Statistics` / `clsEstadisticas...`| `src/server/Statistics.hpp` | Mediano | Métricas / IPC | Diagnostics log / doctest |
| **10** | `clsMapSoundManager.cls` | `src/server/clsMapSoundManager.hpp` | Chico | Sonidos Mapa | Cliente VB6 real |
| **11** | `GameLogic` / `modNuevoTimer` | `src/server/GameLogic.hpp` | Grande | Game Loop / Timers | Uptime servidor C++ |
| **11** | `General.bas` | `src/server/General.hpp` | Grande | Main / Bootstrap | `ArgentumServer.exe` final |
