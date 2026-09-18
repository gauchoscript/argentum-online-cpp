# Índice de Auditoría del Código de Argentum Online v0.13.0

Este directorio contiene la documentación de auditoría histórica del código fuente original en Visual Basic 6 de Argentum Online v0.13.0, incluyendo especificaciones de protocolo, fórmulas matemáticas, lógica de combate y estructura de subsistemas.

## Índice de Documentos de Auditoría

| # | Documento | Descripción Corta | Slug de Área | Última Actualización |
| :-: | :--- | :--- | :--- | :-: |
| 01 | [01-estructura-del-proyecto.md](01-estructura-del-proyecto.md) | Estructura de carpetas y organización de módulos en VB6. | `estructura-del-proyecto` | 2026-09-06 |
| 02 | [02-protocolo-de-red.md](02-protocolo-de-red.md) | Opcodes binarios del protocolo por sockets, serialización y handlers. | `protocolo-de-red` | 2026-09-06 |
| 03 | [03-prediccion-de-movimiento.md](03-prediccion-de-movimiento.md) | Predicción de movimiento en cliente vs. autoridad estricta de combate en servidor. | `prediccion-de-movimiento` | 2026-09-06 |
| 04 | [04-formulas-de-combate.md](04-formulas-de-combate.md) | Fórmulas de daño (`CalcularDaño`), evasión, experiencia y regeneración. | `formulas-de-combate` | 2026-09-06 |
| 05 | [05-loop-del-juego.md](05-loop-del-juego.md) | Registro de timers `VB.Timer`, frecuencias de tick y callbacks. | `loop-del-juego` | 2026-09-06 |
| 06 | [06-formatos-de-datos.md](06-formatos-de-datos.md) | Formatos de archivos INI (`.chr`, `.dat`, `.guild`) y mapas binarios (`.map`). | `formatos-de-datos` | 2026-09-06 |
| 07 | [07-pantallas-e-interfaz.md](07-pantallas-e-interfaz.md) | Inventario de los 45+ formularios del cliente (`.frm`) y pantallas interactivas. | `pantallas-e-interfaz` | 2026-09-06 |
| 08 | [08-recursos-y-multimedia.md](08-recursos-y-multimedia.md) | Sprites Bitmaps (`.bmp`), índices `.ind`, audio WAV, MIDI y MP3. | `recursos-y-multimedia` | 2026-09-06 |
| 09 | [09-seguridad-y-autenticacion.md](09-seguridad-y-autenticacion.md) | Flujo de autenticación, contraseñas en texto plano y vulnerabilidades. | `seguridad-y-autenticacion` | 2026-09-06 |
| 10 | [10-herramientas-de-gm.md](10-herramientas-de-gm.md) | Jerarquía de 5 rangos de GM (`Server.ini`), opcodes (`/BAN`, `/CI`) y paneles GUI. | `herramientas-de-gm` | 2026-09-06 |
| 11 | [11-foros-y-comunicacion.md](11-foros-y-comunicacion.md) | Foros de juego, mensajería y tableros comunitarios (`modForum`). | `foros-y-comunicacion` | 2026-09-13 |
| 12 | [12-objetos-inventario-comercio.md](12-objetos-inventario-comercio.md) | Manipulación de objetos en mapa, inventarios de NPC y usuario, equipamiento y comercio. | `objetos-inventario-comercio` | 2026-09-13 |
| 13 | [13-criaturas-e-ia.md](13-criaturas-e-ia.md) | Ciclo de vida de criaturas (`MODULO_NPCs`), tablas de atributos, máquinas de estados e IA. | `criaturas-e-ia` | 2026-09-13 |
| 14 | [14-magia-y-hechizos.md](14-magia-y-hechizos.md) | Lanzamiento de conjuros (`modHechizos`), fórmulas de daño/curación mágica y efectos. | `magia-y-hechizos` | 2026-09-13 |
| 15 | [15-entidad-usuario-y-estado.md](15-entidad-usuario-y-estado.md) | Estructura central `UserList`, ciclo de vida de conexión, máquina de estados y clases. | `entidad-usuario-y-estado` | 2026-09-13 |

## Anexos y Documentos de Detalle / Código Muerto

| Documento | Descripción Corta | Macro-Área Padre | Estado / Tipo | Última Actualización |
| :--- | :--- | :---: | :--- | :---: |
| [01a-clsdicc-cgarbage.md](01a-clsdicc-cgarbage.md) | Análisis de `clsdicc.cls` y `cGarbage.cls`. | Área 01 | Detalle Módulos Base | 2026-09-08 |
| [02a-securityip-detalle.md](02a-securityip-detalle.md) | Auditoría de `SecurityIp.bas` (anti-flood e IP security). | Área 02 | Detalle Protocolo / Red | 2026-09-10 |
| [02b-antimassclon-detalle.md](02b-antimassclon-detalle.md) | Auditoría de `clsAntiMassClon.cls` y exclusión por código muerto. | Área 02 | Exclusión (Código Muerto) | 2026-09-11 |
# Índice de Auditoría del Código de Argentum Online v0.13.0

Este directorio contiene la documentación de auditoría histórica del código fuente original en Visual Basic 6 de Argentum Online v0.13.0, incluyendo especificaciones de protocolo, fórmulas matemáticas, lógica de combate y estructura de subsistemas.

## Índice de Documentos de Auditoría

| # | Documento | Descripción Corta | Slug de Área | Última Actualización |
| :-: | :--- | :--- | :--- | :-: |
| 01 | [01-estructura-del-proyecto.md](01-estructura-del-proyecto.md) | Estructura de carpetas y organización de módulos en VB6. | `estructura-del-proyecto` | 2026-09-06 |
| 02 | [02-protocolo-de-red.md](02-protocolo-de-red.md) | Opcodes binarios del protocolo por sockets, serialización y handlers. | `protocolo-de-red` | 2026-09-06 |
| 03 | [03-prediccion-de-movimiento.md](03-prediccion-de-movimiento.md) | Predicción de movimiento en cliente vs. autoridad estricta de combate en servidor. | `prediccion-de-movimiento` | 2026-09-06 |
| 04 | [04-formulas-de-combate.md](04-formulas-de-combate.md) | Fórmulas de daño (`CalcularDaño`), evasión, experiencia y regeneración. | `formulas-de-combate` | 2026-09-06 |
| 05 | [05-loop-del-juego.md](05-loop-del-juego.md) | Registro de timers `VB.Timer`, frecuencias de tick y callbacks. | `loop-del-juego` | 2026-09-06 |
| 06 | [06-formatos-de-datos.md](06-formatos-de-datos.md) | Formatos de archivos INI (`.chr`, `.dat`, `.guild`) y mapas binarios (`.map`). | `formatos-de-datos` | 2026-09-06 |
| 07 | [07-pantallas-e-interfaz.md](07-pantallas-e-interfaz.md) | Inventario de los 45+ formularios del cliente (`.frm`) y pantallas interactivas. | `pantallas-e-interfaz` | 2026-09-06 |
| 08 | [08-recursos-y-multimedia.md](08-recursos-y-multimedia.md) | Sprites Bitmaps (`.bmp`), índices `.ind`, audio WAV, MIDI y MP3. | `recursos-y-multimedia` | 2026-09-06 |
| 09 | [09-seguridad-y-autenticacion.md](09-seguridad-y-autenticacion.md) | Flujo de autenticación, contraseñas en texto plano y vulnerabilidades. | `seguridad-y-autenticacion` | 2026-09-06 |
| 10 | [10-herramientas-de-gm.md](10-herramientas-de-gm.md) | Jerarquía de 5 rangos de GM (`Server.ini`), opcodes (`/BAN`, `/CI`) y paneles GUI. | `herramientas-de-gm` | 2026-09-06 |
| 11 | [11-foros-y-comunicacion.md](11-foros-y-comunicacion.md) | Foros de juego, mensajería y tableros comunitarios (`modForum`). | `foros-y-comunicacion` | 2026-09-13 |
| 12 | [12-objetos-inventario-comercio.md](12-objetos-inventario-comercio.md) | Manipulación de objetos en mapa, inventarios de NPC y usuario, equipamiento y comercio. | `objetos-inventario-comercio` | 2026-09-13 |
| 13 | [13-criaturas-e-ia.md](13-criaturas-e-ia.md) | Ciclo de vida de criaturas (`MODULO_NPCs`), tablas de atributos, máquinas de estados e IA. | `criaturas-e-ia` | 2026-09-13 |
| 14 | [14-magia-y-hechizos.md](14-magia-y-hechizos.md) | Lanzamiento de conjuros (`modHechizos`), fórmulas de daño/curación mágica y efectos. | `magia-y-hechizos` | 2026-09-13 |
| 15 | [15-entidad-usuario-y-estado.md](15-entidad-usuario-y-estado.md) | Estructura central `UserList`, ciclo de vida de conexión, máquina de estados y clases. | `entidad-usuario-y-estado` | 2026-09-13 |

## Anexos y Documentos de Detalle / Código Muerto

| Documento | Descripción Corta | Macro-Área Padre | Estado / Tipo | Última Actualización |
| :--- | :--- | :---: | :--- | :---: |
| [01a-clsdicc-cgarbage.md](01a-clsdicc-cgarbage.md) | Análisis de `clsdicc.cls` y `cGarbage.cls`. | Área 01 | Detalle Módulos Base | 2026-09-08 |
| [02a-securityip-detalle.md](02a-securityip-detalle.md) | Auditoría de `SecurityIp.bas` (anti-flood e IP security). | Área 02 | Detalle Protocolo / Red | 2026-09-10 |
| [02b-antimassclon-detalle.md](02b-antimassclon-detalle.md) | Auditoría de `clsAntiMassClon.cls` y exclusión por código muerto. | Área 02 | Exclusión (Código Muerto) | 2026-09-11 |
| [02c-tcp-detalle.md](02c-tcp-detalle.md) | Auditoría de `TCP.bas`, `wskapiAO.bas` y `wsksock.bas` (red y Asio). | Área 02 | Detalle Protocolo / Red | 2026-09-11 |
| [02d-modsenddata-detalle.md](02d-modsenddata-detalle.md) | Auditoría de `modSendData.bas` (despacho, broadcast y áreas). | Área 02 | Detalle Protocolo / Red | 2026-09-12 |
| [02e-protocol-detalle.md](02e-protocol-detalle.md) | Auditoría de `Protocol.bas` (despachador de entrada, serialización y GM). | Área 02 | Detalle Protocolo / Red | 2026-09-12 |
| [03a-modareas-detalle.md](03a-modareas-detalle.md) | Auditoría de `ModAreas.bas` (gestión espacial, cuadrículas y visibilidad). | Área 03 | Detalle Movimiento / Espacial | 2026-09-13 |
| [04a-sistemacombate-detalle.md](04a-sistemacombate-detalle.md) | Auditoría de `SistemaCombate.bas` (combate PvP/PvE, daño, evasión y frags). | Área 04 | Detalle Fórmulas / Combate | 2026-09-14 |
| [06a-colaarray-dead-code.md](06a-colaarray-dead-code.md) | Auditoría de `cColaArray.cls` y exclusión por código muerto. | Área 06 | Exclusión (Código Muerto) | 2026-09-08 |
| [11a-modforum-detalle.md](11a-modforum-detalle.md) | Auditoría de `modForum.bas` (sistema de foros). | Área 11 | Detalle Foros / Comunicación | 2026-09-08 |
| [11b-party-detalle.md](11b-party-detalle.md) | Auditoría de `mdParty.bas` y `clsParty.cls` (sistema de party, distribución de EXP, disolución y quirks). | Área 11 | Detalle Foros / Comunicación | 2026-09-17 |
| [12a-modulo-inventandobj-detalle.md](12a-modulo-inventandobj-detalle.md) | Auditoría de `Modulo_InventANDobj.bas` (inventario NPC, suelo y drops). | Área 12 | Detalle Inventario / Objetos | 2026-09-13 |
| [12b-invusuario-detalle.md](12b-invusuario-detalle.md) | Auditoría de `InvUsuario.bas` (inventario usuario, equipamiento y exploits). | Área 12 | Detalle Inventario / Objetos | 2026-09-13 |
| [12c-modbanco-detalle.md](12c-modbanco-detalle.md) | Auditoría de `modBanco.bas` (bóveda bancaria, transacciones y exploits). | Área 12 | Detalle Inventario / Objetos | 2026-09-13 |
| [12d-comercio-detalle.md](12d-comercio-detalle.md) | Auditoría de `Comercio.bas` y `mdlCOmercioConUsuario.bas` (comercio NPC y P2P seguro). | Área 12 | Detalle Inventario / Comercio | 2026-09-14 |
| [12e-trabajo-detalle.md](12e-trabajo-detalle.md) | Auditoría de `Trabajo.bas` (oficios de recolección, manufactura, combate y habilidades). | Área 12 | Detalle Oficios / Habilidades | 2026-09-16 |
| [14a-hechizos-detalle.md](14a-hechizos-detalle.md) | Auditoría de `modHechizos.bas` (magia, conjuros y estados alterados). | Área 14 | Detalle Magia / Hechizos | 2026-09-15 |
| [14b-invisibles-detalle.md](14b-invisibles-detalle.md) | Auditoría de `modInvisibles.bas` y exclusión por código muerto. | Área 14 | Exclusión (Código Muerto) | 2026-09-16 |
| [15b-facciones-detalle.md](15b-facciones-detalle.md) | Auditoría de `ModFacciones.bas` (facciones Armada Real y Legión Oscura, jerarquías y expulsión). | Área 15 | Detalle Entidad / Estado | 2026-09-16 |
| [15c-acciones-detalle.md](15c-acciones-detalle.md) | Auditoría de `Acciones.bas` (interacciones sobre grilla, NPCs, puertas, foros y fogatas). | Área 15 | Detalle Entidad / Estado | 2026-09-18 |
| [15d-usuarios-detalle.md](15d-usuarios-detalle.md) | Auditoría de `Modulo_UsUaRiOs.bas` (ciclo de vida, sesión, nivel, exp, warp, caspers y quirks). | Área 15 | Detalle Entidad / Estado | 2026-09-18 |
| [08a-pathfinding-detalle.md](08a-pathfinding-detalle.md) | Auditoría de `PathFinding.bas` y `Queue.bas` (búsqueda BFS, orden determinista y quirks). | Área 08 | Detalle NPCs / Búsqueda de Caminos | 2026-09-16 |
| [13a-modulonpcs-detalle.md](13a-modulonpcs-detalle.md) | Auditoría de `MODULO_NPCs.bas` (ciclo de vida, grilla espacial, red y quirks). | Área 13 | Detalle Criaturas / NPCs | 2026-09-17 |
| [13b-ainpc-detalle.md](13b-ainpc-detalle.md) | Auditoría de `AI_NPC.bas` (toma de decisiones, blancado, combate e IA de criaturas). | Área 13 | Detalle Criaturas / IA | 2026-09-17 |
| [13c-praetorians-detalle.md](13c-praetorians-detalle.md) | Auditoría de `praetorians.bas` (IA cooperativa de escuadrón pretoriano, roles, soporte mutuo y alcobas). | Área 13 | Detalle Criaturas / IA Pretoriana | 2026-09-17 |
