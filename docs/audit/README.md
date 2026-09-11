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

## Anexos y Documentos de Detalle / Código Muerto

| Documento | Descripción Corta | Estado / Tipo | Última Actualización |
| :--- | :--- | :--- | :---: |
| [01a-clsdicc-cgarbage.md](01a-clsdicc-cgarbage.md) | Análisis de `clsdicc.cls` y `cGarbage.cls`. | Detalle Módulos Base | 2026-09-08 |
| [02a-securityip-detalle.md](02a-securityip-detalle.md) | Auditoría de `SecurityIp.bas` (anti-flood e IP security). | Detalle Protocolo / Red | 2026-09-10 |
| [02b-antimassclon-detalle.md](02b-antimassclon-detalle.md) | Auditoría de `clsAntiMassClon.cls` y exclusión por código muerto. | Exclusión (Código Muerto) | 2026-09-11 |
| [06a-colaarray-dead-code.md](06a-colaarray-dead-code.md) | Auditoría de `cColaArray.cls` y exclusión por código muerto. | Exclusión (Código Muerto) | 2026-09-08 |
| [11a-modforum-detalle.md](11a-modforum-detalle.md) | Auditoría de `modForum.bas` (sistema de foros). | Detalle Sub-sistema | 2026-09-08 |
