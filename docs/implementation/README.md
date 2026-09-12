# Índice de Implementación de la Migración C++

Este directorio realiza el seguimiento de las decisiones de diseño, especificaciones de arquitectura, tareas y documentación de migración del código legacy VB6 de Argentum Online v0.13.0 hacia C++.

## Referencia Maestra

- **[`00-port-plan.md`](00-port-plan.md)**: Es el **índice y plan maestro de porting**. Establece la secuencia teórica de migración módulo por módulo según el árbol de dependencias por capas (Capa 0 a Capa 11).

> [!NOTE]
> ### Aclaración sobre la Numeración de Documentos
> Los documentos individuales de este directorio (`01-matematicas.md`, `02-clsinireader.md`, `09-declares.md`, `10-fileio-*.md`, `14-tcp.md`, etc.) están numerados estrictamente según el **ID de Módulo oficial asignado en el Port Plan** (`00-port-plan.md`), agrupando física y visualmente todos los archivos (implementación, anexos y desgloses) de un mismo componente.

---

## Documentos de Módulos (Agrupados por ID de Módulo del Port Plan)

| ID | Documento(s) | Módulo(s) / Tema | Capa | Estado |
| :-: | :--- | :--- | :-: | :-: |
| 00 | [`00-port-plan.md`](00-port-plan.md) | Plan Maestro de Porting Secuencial | Todas | `in-progress` |
| 01 | [`01-matematicas.md`](01-matematicas.md) | Matematicas (`Matematicas.bas`) | 0 | `completed` |
| 02 | [`02-clsinireader.md`](02-clsinireader.md) | clsIniReader (`clsIniReader.cls`) | 0 | `completed` |
| 03 | [`03-clsdicc.md`](03-clsdicc.md) | clsdicc (`clsdicc.cls`) | 0 | `completed` |
| 04 | [`04-modcola-queue-colaarray.md`](04-modcola-queue-colaarray.md) | ModCola (`ModCola.cls`), Queue (`Queue.bas`), cColaArray (`cColaArray.cls`) | 0 | `completed` |
| 05 | [`05-cgarbage.md`](05-cgarbage.md) | cGarbage (`cGarbage.cls`) | 0 | `partial` |
| 06 | [`06-modhexastrings.md`](06-modhexastrings.md) | modHexaStrings (`modHexaStrings.bas`) | 0 | `completed` |
| 07 | [`07-csolicitud.md`](07-csolicitud.md) | cSolicitud (`cSolicitud.cls`) | 0 | `completed` |
| 08 | [`08-clsbytequeue.md`](08-clsbytequeue.md) | clsByteQueue (`clsByteQueue.cls`) | 1 | `completed` |
| 09 | [`09-declares.md`](09-declares.md) | Declares (`Declares.bas`) | 0 | `completed` |
| 10 | [`10-fileio-*.md`](10-fileio-persistencia-personajes.md) (5 partes + [Breakdown](10-fileio-breakdown.md)) | FileIO (`FileIO.bas`) | 3 | `completed` |
| 11 | [`11-clsclan-modguilds.md`](11-clsclan-modguilds.md) / [`11-clsclan-breakdown.md`](11-clsclan-breakdown.md) | clsClan (`clsClan.cls`) y modGuilds (`modGuilds.bas`) | 3 | `completed` |
| 12 | [`12-securityip.md`](12-securityip.md) | SecurityIp (`SecurityIp.bas`) | 4 | `partial` |
| 14 | [`14-tcp.md`](14-tcp.md) / [`14-tcp-breakdown.md`](14-tcp-breakdown.md) | TCP (`TCP.bas`, `wskapiAO.bas`, `wsksock.bas`) | 4 | `completed` |
| 15 | [`15-modsenddata.md`](15-modsenddata.md) / [`15-modsenddata-breakdown.md`](15-modsenddata-breakdown.md) | modSendData (`modSendData.bas`) | 4 | `completed` |

