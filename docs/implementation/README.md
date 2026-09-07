# Índice de Implementación de la Migración C++

Este directorio realiza el seguimiento de las decisiones de diseño, especificaciones de arquitectura, tareas y documentación de migración del código legacy VB6 de Argentum Online v0.13.0 hacia C++.

## Referencia Maestra

- **[`00-port-plan.md`](00-port-plan.md)**: Es el **índice y plan maestro de porting**. Establece la secuencia teórica de migración módulo por módulo según el árbol de dependencias por capas (Capa 0 a Capa 11).

> [!NOTE]
> ### Aclaración sobre la Numeración de Documentos
> Los documentos individuales de este directorio (`01-matematicas.md`, `02-declares.md`, etc.) están numerados según el **orden cronológico de completitud real** de la migración. Esta numeración es independiente de la numeración temática de `docs/audit/` y del orden teórico definido en `00-port-plan.md`.

---

## Documentos de Módulos (Orden de Completitud Real)

| # | Documento | Módulo(s) / Tema | Capa | Estado |
| :-: | :--- | :--- | :-: | :-: |
| 00 | [`00-port-plan.md`](00-port-plan.md) | Plan Maestro de Porting Secuencial | Todas | `in-progress` |
| 01 | [`01-matematicas.md`](01-matematicas.md) | Matematicas (`Matematicas.bas`) | 0 | `completed` |
| 01a | [`01a-clsdicc-cgarbage.md`](01a-clsdicc-cgarbage.md) | Especificación de Migración `clsdicc` / `cGarbage` | 0 | `in-progress` |
| 02 | [`02-declares.md`](02-declares.md) | Declares (`Declares.bas`) | 0 | `completed` |
| 03 | [`03-clsinireader.md`](03-clsinireader.md) | clsIniReader (`clsIniReader.cls`) | 0 | `completed` |
| 04 | [`04-clsdicc.md`](04-clsdicc.md) | clsdicc (`clsdicc.cls`) | 0 | `completed` |
| 05 | [`05-cgarbage.md`](05-cgarbage.md) | cGarbage (`cGarbage.cls`) | 0 | `partial` |
| 06 | [`06-modcola-queue-colaarray.md`](06-modcola-queue-colaarray.md) | ModCola (`ModCola.cls`), Queue (`Queue.bas`), cColaArray (`cColaArray.cls`) | 0 | `completed` |
| 07 | [`07-modhexastrings.md`](07-modhexastrings.md) | modHexaStrings (`modHexaStrings.bas`) | 0 | `completed` |
| 08 | [`08-csolicitud.md`](08-csolicitud.md) | cSolicitud (`cSolicitud.cls`) | 0 | `completed` |
