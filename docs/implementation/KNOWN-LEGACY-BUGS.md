---
area: auditoria-y-compatibilidad
status: living_document
title: Registro Centralizado de Bugs y Quirks Históricos del Legacy VB6
tags: [ledger, bugs, quirks, off-by-one, legacy-vb6, paridad, compatibilidad]
last_updated: 2026-09-11
---

# Registro Centralizado de Bugs y Quirks Históricos del Legacy VB6 (`KNOWN-LEGACY-BUGS.md`)

Este documento constituye el **registro maestro y permanente** de todos los defectos lógicos, anomalías de cálculo, errores de límite (*off-by-one*), comportamientos extraños y desviaciones deliberadas identificadas en el código fuente original de Visual Basic 6.0 (`legacy/server/Codigo/`) a lo largo del proceso de auditoría y transliteración a C++.

De acuerdo con la convención del proyecto ([`docs/CONVENTIONS.md`](../CONVENTIONS.md)), el objetivo primordial del port es la **paridad comportamental estricta** contra el servidor original histórico. Por ello, los comportamientos anómalos o defectos conocidos del legacy se clasifican y replican fielmente salvo que representen riesgos críticos de seguridad de memoria comprobados.

---

## Tabla Maestra de Entradas

| ID | Módulo | Cita Legacy | Descripción Sintética | Camino de Producción | Estado en C++ | Documentación Detallada |
| :-: | :--- | :--- | :--- | :-: | :-: | :--- |
| **01** | `clsClan` / `modGuilds` | `clsClan.cls:593-596`<br>`clsdicc.cls:124-127` | Empate electoral no reelige líder, cierra comicios y reporta cantidad de empatados como votos | **Activo** | **Replicated**<br>[`src/server/clsClan.cpp:387-414`](src/server/clsClan.cpp#L387-L414)<br>Test: [`test_clsclan.cpp:170`](tests/test_clsclan.cpp#L170) | [`11-clsclan-modguilds.md`](11-clsclan-modguilds.md#2-resolucion-de-empates-en-elecciones-contarvotos--mayorvalor) |
| **02** | `clsdicc` | `clsdicc.cls:124-127` | `MayorValor(cant)` concatena claves empatadas por inserción y retorna en `cant` la cantidad de empatados en vez de los votos máximos | **Activo** | **Replicated**<br>[`src/server/clsdicc.cpp:78-107`](src/server/clsdicc.cpp#L78-L107)<br>Test: [`test_clsdicc.cpp:16`](tests/test_clsdicc.cpp#L16) | [`03-clsdicc.md`](03-clsdicc.md#3-preservacion-del-orden-de-insercion-y-comportamiento-de-empates-en-mayorvalor) |
| **03** | `clsdicc` | `clsdicc.cls:21` | Límite estático de 100 elementos (`MAX_ELEM = 100`) omitido para usar capacidad dinámica en memoria | **Activo** | **Replicated (Desviación Intencional)**<br>[`src/server/clsdicc.hpp:20`](src/server/clsdicc.hpp#L20) | [`03-clsdicc.md`](03-clsdicc.md#2-eliminacion-del-cap-de-100-elementos-desviacion-intencional-documentada) |
| **04** | `clsIniReader` | `clsIniReader.cls:437-444` | `KeyExists(name)` solo valida la existencia de la sección `[name]`, no de una clave individual dentro de la sección | **Activo** | **Replicated**<br>[`src/server/clsIniReader.cpp:166`](src/server/clsIniReader.cpp#L166)<br>Test: [`test_clsinireader.cpp:52`](tests/test_clsinireader.cpp#L52) | [`02-clsinireader.md`](02-clsinireader.md#quirk-prominente-de-keyexists) |
| **05** | `ModCola` (`cCola`) | `ModCola.cls:1-50` | Retorno de `"0"` ante cola vacía o índices fuera de rango por coerción de tipo y `On Error Resume Next` de VB6 | **Activo** | **Replicated**<br>[`src/server/ModCola.cpp:44,54,76`](src/server/ModCola.cpp#L44)<br>Test: [`test_modcola.cpp:54`](tests/test_modcola.cpp#L54) | [`04-modcola-queue-colaarray.md`](04-modcola-queue-colaarray.md#respuestas-por-defecto-en-casos-error-o-cola-vacia) |
| **06** | `modHexaStrings` | `modHexaStrings.bas:18-20` | Relleno implícito con cero a la izquierda si la longitud de entrada es impar (`Len(MD5) And &H1`) en `hexMd52Asc` | **Activo** | **Replicated**<br>[`src/server/modHexaStrings.cpp:14`](src/server/modHexaStrings.cpp#L14)<br>Test: [`test_modhexastrings.cpp:25`](tests/test_modhexastrings.cpp#L25) | [`06-modhexastrings.md`](06-modhexastrings.md#3-relleno-de-ceros-a-la-izquierda-zero-padding-para-longitud-impar) |
| **07** | `modHexaStrings` | `modHexaStrings.bas:40-49` | `hexHex2Dec` detiene silenciosamente el parseo ante caracteres no hexadecimales y devuelve el acumulado (`Val("&H...")`) | **Activo** | **Replicated**<br>[`src/server/modHexaStrings.cpp:38`](src/server/modHexaStrings.cpp#L38)<br>Test: [`test_modhexastrings.cpp:42`](tests/test_modhexastrings.cpp#L42) | [`06-modhexastrings.md`](06-modhexastrings.md#4-tolerancia-a-caracteres-no-hexadecimales-y-espacios-valh--hex) |
| **08** | `cSolicitud` / `clsClan` | `cSolicitud.cls:18-20`<br>`clsClan.cls:315-325` | Desacoplamiento entre nombres de miembros en memoria (`UserName`, `desc`) y claves en disco `.sol` (`Nombre`, `Detalle`) | **Activo** | **Replicated**<br>[`src/server/cSolicitud.hpp:10`](src/server/cSolicitud.hpp#L10)<br>[`src/server/clsClan.cpp:202`](src/server/clsClan.cpp#L202) | [`07-csolicitud.md`](07-csolicitud.md#2-nombres-de-miembros-de-datos-segun-la-naming-policy) |
| **09** | `FileIO` (Grupo 5) | `FileIO.bas:895-903` | En `LoadOBJData`, typo en `CP<N>` causaba `Subscript out of range` (Error 9); en C++ se asigna `0` seguro | **Activo** | **Replicated (Desviación Deliberada de Seguridad)**<br>[`src/server/FileIO.cpp:520`](src/server/FileIO.cpp#L520)<br>Test: [`test_fileio_gamedata.cpp:66`](tests/test_fileio_gamedata.cpp#L66) | [`10-fileio-tablas-datos.md`](10-fileio-tablas-datos.md#3-analisis-de-caso-de-borde-en-loadobjdata-claseprohibida) |
| **10** | `FileIO` (Grupo 7) | `FileIO.bas:2158, 2178` | `LogBanFromName` y `Ban` persisten registros en `logs/BanDetail.dat` con extensión `.dat` en lugar de `.log` | **Activo** | **Replicated**<br>[`src/server/FileIO.cpp:1159, 1177`](src/server/FileIO.cpp#L1159)<br>Test: [`test_fileio_backup_logging.cpp:45`](tests/test_fileio_backup_logging.cpp#L45) | [`10-fileio-backup-logging.md`](10-fileio-backup-logging.md#a-formatos-de-registro-y-quirk-historico-de-extensiones-log-vs-dat) |
| **11** | `SecurityIp` #1 | `SecurityIp.bas:291` / `310` | Búsqueda binaria retorna `~(Middle * 2)` en vez de `~(First * 2)`, insertando fuera de orden | **Activo** (`IP_INTERVALOS`) / **Muerto** (`IP_LIMITECONEXIONES`) | **Replicated**<br>[`src/server/SecurityIp.cpp:44`](src/server/SecurityIp.cpp#L44)<br>Test: [`test_securityip.cpp:121`](tests/test_securityip.cpp#L121) | [Entrada #11](#entrada-11--securityip-retorno-de-middle--2-en-vez-de-first--2-en-búsqueda-binaria) |
| **12** | `SecurityIp` #2 | `SecurityIp.bas:278` / `296` | Cota superior inicial `Last = MaxValue` (off-by-one) que compara contra elemento no inicializado o fuera de rango | **Activo** (`IP_INTERVALOS`) / **Muerto** (`IP_LIMITECONEXIONES`) | **Replicated**<br>[`src/server/SecurityIp.cpp:21`](src/server/SecurityIp.cpp#L21)<br>Test: [`test_securityip.cpp:17`](tests/test_securityip.cpp#L17) | [Entrada #12](#entrada-12--securityip-cota-superior-inicial-desplazada-last--maxvalue-off-by-one) |
| **13** | `SecurityIp` #3 | `SecurityIp.bas:247` | Sobrecopia de 16 bytes en compactación de `MaxConTables` dentro de `IpRestarConexion` provocando lecturas y escrituras fuera de rango | **Muerto / Comentado** | **Excluded (dead code, not ported)** | [Entrada #13](#entrada-13--securityip-sobrecopia-de-memoria--lectura-fuera-de-rango-en-iprestarconexion)<br>Detalle: [`../audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#43-estado-de-ipsecuritysuperalimiteconexiones-e-iprestarconexion) |
| **14** | `SecurityIp` #4 | `SecurityIp.bas:180-188` | Retorno permisivo (`False`) ante agotamiento de slots en `IPSecuritySuperaLimiteConexiones`, admitiendo conexiones sin registrar | **Muerto / Comentado** | **Excluded (dead code, not ported)** | [Entrada #14](#entrada-14--securityip-retorno-permisivo-ante-agotamiento-de-slots-en-ipsecuritysuperalimiteconexiones)<br>Detalle: [`../audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#43-estado-de-ipsecuritysuperalimiteconexiones-e-iprestarconexion) |
| **15** | `SecurityIp` #5 | `SecurityIp.bas:86-97` | Asimetría de mantenimiento horario: purga `IpTables` pero no interviene sobre `MaxConTables` | **Activo** | **Replicated**<br>[`src/server/SecurityIp.cpp:78`](src/server/SecurityIp.cpp#L78)<br>Test: [`test_securityip.cpp:325`](tests/test_securityip.cpp#L325) | [Entrada #15](#entrada-15--securityip-asimetría-en-el-mantenimiento-periódico-ipsecuritymantenimientolista) |
| **16** | `SecurityIp` #6 | `SecurityIp.bas:111` | Error 6 ("Overflow") de VB6 tras ~24.85 días de uptime (`SERVER.VBP` `OverflowCheck=0`) | **Activo** | **Replicated (Excepción Tipada)**<br>[`SecurityIp::TickCountOverflowException`](src/server/SecurityIp.hpp)<br>Test: [`test_securityip.cpp:342`](tests/test_securityip.cpp#L342) | [Entrada #16](#entrada-16--securityip-error-6-overflow-de-vb6-por-desbordamiento-aritmético-en-ipsecurityaceptarnuevaconexion) |
| **17** | `cColaArray` | `cColaArray.cls` | Clase de búfer circular de texto inalcanzable, bloqueada bajo `#If UsarQueSocket = 3` y sin campos en `User` | **Muerto / Inalcanzable** | **Excluded (dead code, not ported)** | [`docs/audit/06a-colaarray-dead-code.md`](../audit/06a-colaarray-dead-code.md) |
| **18** | `clsAntiMassClon` | `clsAntiMassClon.cls:46-67, 58-63`<br>`SERVER.VBP:76` | Inserción de IPs inoperante por condicional `#If SeguridadAlkon` apagado y tipo inexistente `UserIpAdress` (bypass total en producción) | **Muerto / Inoperante** | **Excluded (dead code, not ported)** | [`docs/audit/02b-antimassclon-detalle.md`](../audit/02b-antimassclon-detalle.md) |
| **19** | `TCP` / `wskapiAO` | `wskapiAO.bas:334-342`<br>`clsByteQueue.cls:196-199`<br>`Protocol.bas:858-863` | Congelamiento del servidor por bucle infinito ocupado (busy-loop) ante `WSAEWOULDBLOCK` en `WsApiEnviar` combinado con `NOT_ENOUGH_SPACE` y `Resume` | **Activo** | **Mitigated (Safe Backpressure)**<br>[`TCP::EnviarDatosASlot`](src/server/TCP.cpp)<br>Test: [`test_tcp.cpp:653`](tests/test_tcp.cpp#L653) | [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#54-umbrales-de-socket-os-y-la-trampa-de-bloqueo-wsaewouldblock)<br>[`14-tcp.md`](14-tcp.md#2-mitigación-de-backpressure-bug-19) |
| **20** | `TCP` / `wskapiAO` | `wskapiAO.bas:402-405, 420` | Fuga de descriptor de socket (*socket leak*) en rechazo anti-flood por invocar `WSApiCloseSocket(NuevoSock)` antes de asignar `NuevoSock = Ret` (cerrando descriptor 0) | **Activo** | **Mitigated (Safe RAII / Explicit Close)**<br>[`TCP::HandleAccept`](src/server/TCP.cpp)<br>Test: [`test_tcp.cpp:301`](tests/test_tcp.cpp#L301) | [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#c-procesamiento-del-evento-de-conexión-entrante-fd_accept)<br>[`14-tcp.md`](14-tcp.md#3-cierre-seguro-raii-en-aceptación-bug-20) |
| **21** | `modSendData` | `modSendData.bas:39, 70-302` | Constante `SendTarget.ToGM` declarada en el Enum pero sin bloque de procesamiento en el `Select Case` de `SendData` (cae en el vacío) | **Muerto / Huérfano** | **Excluded (dead code, not ported)** | [`docs/audit/02d-modsenddata-detalle.md`](../audit/02d-modsenddata-detalle.md#61-sendtargettogm-huérfano) |
| **22** | `modSendData` | `modSendData.bas:116-200` | Broadcasts administrativos y faccionarios no chequean `flags.UserLogged`, enviando paquetes de juego a sockets en pantalla de login | **Activo** | **Mitigated (Safe Logic)** | [`docs/audit/02d-modsenddata-detalle.md`](../audit/02d-modsenddata-detalle.md#62-ausencia-de-verificación-flagsuserlogged-en-broadcasts-globales)<br>[`15-modsenddata-breakdown.md`](15-modsenddata-breakdown.md#14-mitigación-de-bugs-legacy) |

---

## Detalle de Entradas Específicas

A continuación se documentan en detalle todas las entradas del registro maestro de defectos lógicos, anomalías y quirks del código legacy VB6, especificando su impacto en el camino de producción y su tratamiento arquitectónico en C++.

---

### Entrada #01 — `clsClan` / `modGuilds`: Resolución Defectuosa de Empates en Elecciones
- **Cita Legacy**: [`legacy/server/Codigo/clsClan.cls:593-596`](legacy/server/Codigo/clsClan.cls#L593-L596) y [`legacy/server/Codigo/clsdicc.cls:124-127`](legacy/server/Codigo/clsdicc.cls#L124-L127).
- **Descripción**: Cuando las elecciones internas de un clan concluyen con dos o más candidatos empatados en primer lugar (`CantGanadores > 1`):
  1. No se transfiere el liderazgo a ningún candidato: `SetLeader` no se invoca, por lo que el líder previo retiene el mandato.
  2. No se dispara balotaje ni desempate: se borra el archivo `*-votaciones.vot` y se cierran los comicios.
  3. El mensaje publicado en las noticias del clan (`GuildNews`) contiene un error histórico flagrante:
     `"*Empate en la votación. " & Ganador & " con " & CantGanadores & " votos ganaron las elecciones del clan."*`
     donde `CantGanadores` contiene la **cantidad de candidatos empatados** (ej. `2`) y no la cantidad de votos que recibieron.
- **Camino de Producción**: **Activo**. El procedimiento `RevisarElecciones` es convocado periódicamente por el timer de autoguardado en `DoBackUp` (`General.bas` / `FileIO.bas`).
- **Estado en C++**: **Replicated**. Preservado de forma literal en `src/server/clsClan.cpp:387-414` y validado exhaustivamente en `tests/test_clsclan.cpp:170-204`.
- **Referencia**: [`11-clsclan-modguilds.md`](11-clsclan-modguilds.md#2-resolucion-de-empates-en-elecciones-contarvotos--mayorvalor).

---

### Entrada #02 — `clsdicc`: Concatención por Inserción y Retorno de Empates en `MayorValor`
- **Cita Legacy**: [`legacy/server/Codigo/clsdicc.cls:124-127`](legacy/server/Codigo/clsdicc.cls#L124-L127).
- **Descripción**: La función `MayorValor(cant)` busca la clave con el valor numérico más alto. Si existen varias claves con dicho valor:
  1. Concatena los nombres de todas las claves separadas por comas en el **orden de inserción original** (no en orden alfabético).
  2. Sobrescribe el parámetro por referencia `cant` con la **cantidad de claves empatadas**, en lugar del puntaje o cantidad de votos alcanzada.
- **Camino de Producción**: **Activo**. Es el motor subyacente que consume `clsClan.cls` para la votación de líderes.
- **Estado en C++**: **Replicated**. Implementado en `src/server/clsdicc.cpp:78-107` manteniendo un vector auxiliar `m_order` para garantizar el orden de inserción sin penalizar el acceso en tiempo constante. Probado en `tests/test_clsdicc.cpp:16-56`.
- **Referencia**: [`03-clsdicc.md`](03-clsdicc.md#3-preservacion-del-orden-de-insercion-y-comportamiento-de-empates-en-mayorvalor).

---

### Entrada #03 — `clsdicc`: Capacidad Estática de 100 Elementos (`MAX_ELEM = 100`)
- **Cita Legacy**: [`legacy/server/Codigo/clsdicc.cls:21`](legacy/server/Codigo/clsdicc.cls#L21).
- **Descripción**: En VB6, `clsdicc` utilizaba un array estático de tamaño fijo `arr(MAX_ELEM)`. Si un clan superaba los 100 miembros votantes o se insertaban más de 100 elementos, la operación fallaba o desbordaba el array.
- **Camino de Producción**: **Activo**.
- **Estado en C++**: **Replicated (Desviación Intencional Documentada)**. Se omitió la cota de 100 elementos sustituyendo el array estático por contenedores dinámicos estándar (`std::unordered_map` y `std::vector`) en `src/server/clsdicc.hpp:20-25`. Esta decisión se tomó al tratarse de una limitación de bajo nivel de VB6 para no utilizar memoria dinámica y no de una regla de negocio del juego.
- **Referencia**: [`03-clsdicc.md`](03-clsdicc.md#2-eliminacion-del-cap-de-100-elementos-desviacion-intencional-documentada) y [`../audit/01a-clsdicc-cgarbage.md`](../audit/01a-clsdicc-cgarbage.md).

---

### Entrada #04 — `clsIniReader`: Falsa Comprobación de Clave en `KeyExists`
- **Cita Legacy**: [`legacy/server/Codigo/clsIniReader.cls:437-444`](legacy/server/Codigo/clsIniReader.cls#L437-L444).
- **Descripción**: La función pública `KeyExists(name)` ejecuta internamente `FindMain(UCase$(name)) >= 0`. A pesar de que su nombre indica comprobación de clave (*key*), **valida únicamente si existe la sección principal (`MainNode`, `[name]`)** y no un par clave-valor específico.
- **Camino de Producción**: **Activo**. Utilizado a lo largo del servidor para verificar si un encabezado de sección se encuentra en memoria.
- **Estado en C++**: **Replicated**. Preservado idénticamente en `src/server/clsIniReader.cpp:166-169` para evitar romper las llamadas que dependen de esta semántica original. Probado en `tests/test_clsinireader.cpp:52-60`.
- **Referencia**: [`02-clsinireader.md`](02-clsinireader.md#quirk-prominente-de-keyexists).

---

### Entrada #05 — `ModCola` (`cCola`): Respuestas por Defecto en Caso de Error o Cola Vacía
- **Cita Legacy**: [`legacy/server/Codigo/ModCola.cls:1-50`](legacy/server/Codigo/ModCola.cls#L1-L50).
- **Descripción**: Los métodos de inspección (`VerElemento`) y extracción (`Quitar`) devuelven `"0"` si la cola está vacía o si el índice está fuera de rango (`Index > m_largo`). Esto ocurría debido a la inicialización implícita de variantes a cero en VB6 y al uso generalizado de `On Error Resume Next`.
- **Camino de Producción**: **Activo**. Consumido por el sistema de `/AYUDA` y comandos de Game Masters.
- **Estado en C++**: **Replicated**. Preservado en `src/server/ModCola.cpp:44,54,76` retornando `"0"` para entradas inválidas. Probado en `tests/test_modcola.cpp:54-69`.
- **Referencia**: [`04-modcola-queue-colaarray.md`](04-modcola-queue-colaarray.md#respuestas-por-defecto-en-casos-de-error-o-cola-vacia).

---

### Entrada #06 — `modHexaStrings`: Padding Implícito de Ceros a la Izquierda en `hexMd52Asc`
- **Cita Legacy**: [`legacy/server/Codigo/modHexaStrings.bas:18-20`](legacy/server/Codigo/modHexaStrings.bas#L18-L20).
- **Descripción**: Si la cadena hexadecimal recibida en `hexMd52Asc` posee una longitud impar, VB6 ejecuta `If (Len(MD5) And &H1) Then MD5 = "0" & MD5`, anteponiendo un cero a la izquierda para emparejar la cantidad de nibbles.
- **Camino de Producción**: **Activo**. Consumido por el validador de MD5 de clientes en `Admin.bas` (`MD5sCarga`).
- **Estado en C++**: **Replicated**. Preservado en `src/server/modHexaStrings.cpp:14-17` y verificado en `tests/test_modhexastrings.cpp:25-34`.
- **Referencia**: [`06-modhexastrings.md`](06-modhexastrings.md#3-relleno-de-ceros-a-la-izquierda-zero-padding-para-longitud-impar).

---

### Entrada #07 — `modHexaStrings`: Tolerancia a Caracteres No Hexadecimales en `hexHex2Dec`
- **Cita Legacy**: [`legacy/server/Codigo/modHexaStrings.bas:40-49`](legacy/server/Codigo/modHexaStrings.bas#L40-L49).
- **Descripción**: `hexHex2Dec` delega la conversión a la función intrínseca `Val("&H" & HexStr)` de VB6. `Val()` detiene el parseo ante el primer carácter no válido (espacios, letras fuera de A-F) y devuelve el número acumulado hasta ese punto sin emitir error.
- **Camino de Producción**: **Activo**.
- **Estado en C++**: **Replicated**. Implementado en `src/server/modHexaStrings.cpp:38-55` replicando el comportamiento tolerante de `Val("&H...")`. Probado en `tests/test_modhexastrings.cpp:42-56`.
- **Referencia**: [`06-modhexastrings.md`](06-modhexastrings.md#4-tolerancia-a-caracteres-no-hexadecimales-y-espacios-valh--hex).

---

### Entrada #08 — `cSolicitud` / `clsClan`: Desacoplamiento de Identificadores DTO vs. Claves INI
- **Cita Legacy**: [`legacy/server/Codigo/cSolicitud.cls:18-20`](legacy/server/Codigo/cSolicitud.cls#L18-L20) y [`legacy/server/Codigo/clsClan.cls:315-325`](legacy/server/Codigo/clsClan.cls#L315-L325).
- **Descripción**: En la clase `cSolicitud.cls`, los campos públicos se llamaban `UserName` y `desc`. Sin embargo, al persistirse en los archivos `.sol` en disco (`SaveSolicitudes`), las claves INI utilizadas eran `Nombre` y `Detalle`.
- **Camino de Producción**: **Activo**.
- **Estado en C++**: **Replicated**. Se mantuvieron los identificadores `UserName` y `desc` en `src/server/cSolicitud.hpp:10-12` según la convención de preservar nombres de VB6, documentando el mapeo explícito hacia `Nombre` y `Detalle` en la serialización de `clsClan.cpp`.
- **Referencia**: [`07-csolicitud.md`](07-csolicitud.md#2-nombres-de-miembros-de-datos-segun-la-naming-policy) y [`11-clsclan-modguilds.md`](11-clsclan-modguilds.md#1-frontera-de-serializacion-de-csolicitud-mapeo-dto-vs-claves-ini).

---

### Entrada #09 — `FileIO`: Typo en `LoadOBJData` Causando Acceso Fuera de Rango (`CP<N>`)
- **Cita Legacy**: [`legacy/server/Codigo/FileIO.bas:895-903`](legacy/server/Codigo/FileIO.bas#L895-L903).
- **Descripción**: En la carga de ítems (`LoadOBJData`), el código legacy lee `CP1` a `CP8` para las clases prohibidas. Si un valor en el archivo INI excede la cantidad de clases (`NUMCLASES`), en VB6 se producía el error en tiempo de ejecución 9 (`Subscript out of range`).
- **Camino de Producción**: **Activo**.
- **Estado en C++**: **Replicated (Desviación Deliberada de Seguridad)**. Para evitar comportamiento indefinido (UB) en C++, se valida el rango antes de indexar el arreglo; ante un valor corrupto se descarta con seguridad asignando `0` y logueando la advertencia. Probado en `tests/test_fileio_gamedata.cpp:66-80`.
- **Referencia**: [`10-fileio-tablas-datos.md`](10-fileio-tablas-datos.md#3-analisis-de-caso-de-borde-en-loadobjdata-claseprohibida).

---

### Entrada #10 — `FileIO`: Extensión `.dat` Inconsistente en Archivos de Log de Sanciones
- **Cita Legacy**: [`legacy/server/Codigo/FileIO.bas:2158, 2178`](legacy/server/Codigo/FileIO.bas#L2158).
- **Descripción**: Las rutinas `LogBanFromName` y `Ban` persisten los motivos y detalles de sanciones en `logs/BanDetail.dat` en lugar de utilizar la extensión estándar `.log` como el resto del sistema de logging (`Ban.log`, `Connect.log`, etc.).
- **Camino de Producción**: **Activo**.
- **Estado en C++**: **Replicated**. Se preserva de forma estricta la ruta `logs/BanDetail.dat` para garantizar la compatibilidad con herramientas de administración externas. Probado en `tests/test_fileio_backup_logging.cpp:45-60`.
- **Referencia**: [`10-fileio-backup-logging.md`](10-fileio-backup-logging.md#a-formatos-de-registro-y-quirk-historico-de-extensiones-log-vs-dat).

---

### Entrada #11 — `SecurityIp`: Retorno de `~(Middle * 2)` en vez de `~(First * 2)` en Búsqueda Binaria
- **Cita Legacy**: [`legacy/server/Codigo/SecurityIp.bas:291`](legacy/server/Codigo/SecurityIp.bas#L291) (rama `IP_INTERVALOS`) y [`legacy/server/Codigo/SecurityIp.bas:310`](legacy/server/Codigo/SecurityIp.bas#L310) (rama `IP_LIMITECONEXIONES`).
- **Descripción**: Cuando la búsqueda binaria `FindTableIp` no encuentra la IP (`First > Last`), retorna el complemento a uno de `Middle * 2` (`Not (Middle * 2)`) en lugar del punto real de inserción `First * 2`, usando un valor residual de `Middle` que provoca inserciones fuera de orden y corrompe la propiedad de ordenamiento monótono de los arrays.
- **Camino de Producción**:
  - Rama `IP_INTERVALOS` (línea 291): **Activo** (invocado en cada conexión entrante en [`legacy/server/Codigo/wskapiAO.bas:402`](legacy/server/Codigo/wskapiAO.bas#L402)).
  - Rama `IP_LIMITECONEXIONES` (línea 310): **Muerto / Comentado** (las llamadas a `IPSecuritySuperaLimiteConexiones` están comentadas en producción).
- **Estado en C++**: **Replicated**.
  - Ubicación C++: [`src/server/SecurityIp.cpp:44`](src/server/SecurityIp.cpp#L44) dentro de `SecurityIp::FindTableIp`.
  - Verificación: Test unitario en [`tests/test_securityip.cpp:121`](tests/test_securityip.cpp#L121) validando que la secuencia `[100, 300, 200, 400, 150]` termina exactamente en `[150, 200, 100, 300, 400]`.
- **Referencia**: [`12-securityip.md`](12-securityip.md) y [`docs/audit/02a-securityip-detalle.md`](../audit/02a-securityip-detalle.md#41-anomalía-en-retorno-de-búsqueda-binaria-findtableip).

---

### Entrada #12 — `SecurityIp`: Cota Superior Inicial Desplazada `Last = MaxValue` (Off-by-One)
- **Cita Legacy**: [`legacy/server/Codigo/SecurityIp.bas:278`](legacy/server/Codigo/SecurityIp.bas#L278) (`Last = MaxValue`) y [`legacy/server/Codigo/SecurityIp.bas:296`](legacy/server/Codigo/SecurityIp.bas#L296) (`Last = MaxConTablesEntry`).
- **Descripción**: La búsqueda binaria inicializa `Last = MaxValue` en lugar de `MaxValue - 1`, lo que causa que en tablas vacías (`MaxValue = 0`) se ejecute una iteración espuria comparando contra memoria no inicializada (valor 0), y en tablas con $N > 0$ elementos se evalúe un elemento ficticio fuera de los datos cargados.
- **Camino de Producción**:
  - Rama `IP_INTERVALOS` (línea 278): **Activo**.
  - Rama `IP_LIMITECONEXIONES` (línea 296): **Muerto / Comentado**.
- **Estado en C++**: **Replicated**.
  - Ubicación C++: [`src/server/SecurityIp.cpp:21`](src/server/SecurityIp.cpp#L21) dentro de `SecurityIp::FindTableIp`.
  - Verificación: Ejercitado en el primer paso de inserción en [`tests/test_securityip.cpp:17-32`](tests/test_securityip.cpp#L17-L32).
- **Referencia**: [`12-securityip.md`](12-securityip.md) y [`docs/audit/02a-securityip-detalle.md`](../audit/02a-securityip-detalle.md#42-cota-superior-inicial-desplazada-last--maxvalue).

---

### Entrada #13 — `SecurityIp`: Sobrecopia de Memoria / Lectura Fuera de Rango en `IpRestarConexion`
- **Cita Legacy**: [`legacy/server/Codigo/SecurityIp.bas:247`](legacy/server/Codigo/SecurityIp.bas#L247).
- **Descripción**: La fórmula `(MaxConTablesEntry - (key \ 2) + 1) * 8` empleada en `CopyMemory` para compactar `MaxConTables` copia 2 entradas lógicas (16 bytes) más allá del final de los datos válidos, provocando lecturas y escrituras fuera de rango cuando la tabla se acerca al límite `Declaraciones.MaxUsers`.
- **Camino de Producción**: **Muerto / Comentado** (las llamadas a `IpRestarConexion` en [`legacy/server/Codigo/wskapiAO.bas:466`](legacy/server/Codigo/wskapiAO.bas#L466) y [`legacy/server/Codigo/TCP.bas:626`](legacy/server/Codigo/TCP.bas#L626) están comentadas con apóstrofe en el servidor de producción).
- **Estado en C++**: **Excluded (dead code, not ported)**. Excluido formalmente al confirmarse en [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#43-estado-de-ipsecuritysuperalimiteconexiones-e-iprestarconexion) que sus llamadas están comentadas en el código de producción 0.13.0.
- **Referencia**: [`14-tcp.md`](14-tcp.md) y [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#43-estado-de-ipsecuritysuperalimiteconexiones-e-iprestarconexion).

---

### Entrada #14 — `SecurityIp`: Retorno Permisivo ante Agotamiento de Slots en `IPSecuritySuperaLimiteConexiones`
- **Cita Legacy**: [`legacy/server/Codigo/SecurityIp.bas:180-188`](legacy/server/Codigo/SecurityIp.bas#L180-L188).
- **Descripción**: Si `MaxConTablesEntry >= Declaraciones.MaxUsers`, la función emite una alerta crítica mediante `LogCriticEvent`, pero mantiene la variable de retorno en `False` (línea 180), permitiendo el ingreso de la conexión sin registrarla para su seguimiento.
- **Camino de Producción**: **Muerto / Comentado** (la llamada a `IPSecuritySuperaLimiteConexiones` en [`legacy/server/Codigo/wskapiAO.bas:433`](legacy/server/Codigo/wskapiAO.bas#L433) está comentada con apóstrofe en producción).
- **Estado en C++**: **Excluded (dead code, not ported)**. Excluido formalmente al confirmarse en [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#43-estado-de-ipsecuritysuperalimiteconexiones-e-iprestarconexion) que su llamada está comentada en producción 0.13.0.
- **Referencia**: [`14-tcp.md`](14-tcp.md) y [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#43-estado-de-ipsecuritysuperalimiteconexiones-e-iprestarconexion).

---

### Entrada #15 — `SecurityIp`: Asimetría en el Mantenimiento Periódico (`IpSecurityMantenimientoLista`)
- **Cita Legacy**: [`legacy/server/Codigo/SecurityIp.bas:86-97`](legacy/server/Codigo/SecurityIp.bas#L86-L97).
- **Descripción**: La rutina de mantenimiento horario `IpSecurityMantenimientoLista` purga y restablece únicamente `IpTables`, reduciendo `EntrysCounter \ Multiplicado`, pero no interviene sobre `MaxConTables`, la cual depende exclusivamente del decremento reactivo por desconexión en `IpRestarConexion`.
- **Camino de Producción**: **Activo** (invocado en `Sub LimpiarMundo()` en [`legacy/server/Codigo/General.bas:169`](legacy/server/Codigo/General.bas#L169)).
- **Estado en C++**: **Replicated**.
  - Ubicación C++: [`src/server/SecurityIp.cpp:78-93`](src/server/SecurityIp.cpp#L78-L93).
  - Verificación: Validado en [`tests/test_securityip.cpp:325-340`](tests/test_securityip.cpp#L325-L340).
- **Referencia**: [`12-securityip.md`](12-securityip.md).

---

### Entrada #16 — `SecurityIp`: Error 6 ("Overflow") de VB6 por Desbordamiento Aritmético en `IpSecurityAceptarNuevaConexion`
- **Cita Legacy**: [`legacy/server/Codigo/SecurityIp.bas:111`](legacy/server/Codigo/SecurityIp.bas#L111).
- **Descripción**: La expresión `IpTables(IpTableIndex + 1) + IntervaloEntreConexiones <= GetTickCount` en enteros de 32 bits con signo (`Long` de VB6) produce un desbordamiento aritmético cuando el timestamp almacenado supera `INT32_MAX - IntervaloEntreConexiones` (2.147.482.647 ms, aprox. 24.85 días de uptime del sistema). Con `CompilationType=0` y `OverflowCheck=0` en [`legacy/server/SERVER.VBP`](legacy/server/SERVER.VBP#L48), el ejecutable nativo de producción lanzaba el fallo fatal de tiempo de ejecución **Error 6 ("Overflow")**, abortando el proceso del servidor.
- **Camino de Producción**: **Activo** (invocado en cada conexión entrante en [`legacy/server/Codigo/wskapiAO.bas:402`](legacy/server/Codigo/wskapiAO.bas#L402)).
- **Estado en C++**: **Replicated (Excepción Tipada)**.
  - Implementación: Pre-check en [`src/server/SecurityIp.cpp`](src/server/SecurityIp.cpp) que valida `lastTicks > std::numeric_limits<std::int32_t>::max() - IntervaloEntreConexiones` y lanza [`SecurityIp::TickCountOverflowException`](src/server/SecurityIp.hpp) (con código legacy `ERR_OVERFLOW = 6`), emulando el crash de forma segura sin incurrir en *Undefined Behavior* (UB) y garantizando la invariancia total del estado interno (*strong exception guarantee*).
  - Verificación: Validado en [`tests/test_securityip.cpp:342`](tests/test_securityip.cpp#L342) comprobando el límite seguro en `INT32_MAX - 1000` (sin excepción), el disparo obligatorio de la excepción en `INT32_MAX - 999`, y la invariabilidad de `IpTables` y `MaxValue`.
- **Referencia**: [`12-securityip.md`](12-securityip.md) y [`docs/audit/02a-securityip-detalle.md`](../audit/02a-securityip-detalle.md#44-posible-overflow-en-aritmética-temporal-de-gettickcount).

---

### Entrada #18 — `clsAntiMassClon`: Inserción de IPs Inoperante por `#If SeguridadAlkon` Apagado y Tipo Inexistente
- **Cita Legacy**: [`legacy/server/Codigo/clsAntiMassClon.cls:46-67, 58-63`](../../legacy/server/Codigo/clsAntiMassClon.cls#L46-L67) y [`legacy/server/SERVER.VBP:76`](../../legacy/server/SERVER.VBP#L76).
- **Descripción**: La clase `clsAntiMassClon` fue diseñada para limitar la creación masiva de personajes a un máximo de 15 por IP entre WorldSaves. Sin embargo:
  1. En `SERVER.VBP:76` la directiva `CondComp` es `"UsarQueSocket = 1 : ConUpTime = 1"`, no incluyendo `SeguridadAlkon`.
  2. La sentencia `m_coleccion.Add oIp` está encerrada bajo `#If SeguridadAlkon Then`, por lo que nunca se compila ni se insertan IPs en la colección.
  3. En ejecución real, `m_coleccion.Count` permanece en `0`, haciendo que `MaxPersonajes(sIp)` siempre retorne `False` (bypass absoluto de la restricción en producción).
  4. La clase/tipo referenciado `UserIpAdress` no existe en ninguna parte de los fuentes del servidor ni del cliente. Si se hubiera habilitado `SeguridadAlkon = 1`, el compilador de VB6 hubiera fallado con `Compile error: User-defined type not defined`.
- **Camino de Producción**: **Muerto / Inoperante**.
- **Estado en C++**: **Excluded (dead code, not ported)**. La clase se excluye del porting para respetar el principio de no rediseñar ni inventar tipos inexistentes, preservando el comportamiento observable de la versión 0.13.0 de producción.
- **Documentación Detallada**: [`docs/audit/02b-antimassclon-detalle.md`](../audit/02b-antimassclon-detalle.md).

---

### Entrada #19 — `TCP` / `wskapiAO`: Congelamiento del Servidor por Bucle Infinito Ocupado ante `WSAEWOULDBLOCK` y `NOT_ENOUGH_SPACE`
- **Cita Legacy**: [`legacy/server/Codigo/wskapiAO.bas:334-342`](legacy/server/Codigo/wskapiAO.bas#L334-L342), [`legacy/server/Codigo/clsByteQueue.cls:196-199`](legacy/server/Codigo/clsByteQueue.cls#L196-L199) y [`legacy/server/Codigo/Protocol.bas:858-863`](legacy/server/Codigo/Protocol.bas#L858-L863).
- **Descripción**: En el servidor legacy, la cola de salida `outgoingData As clsByteQueue` posee una capacidad fija de 10 KB (`DATA_BUFFER = 10240`). Cuando la serialización de un paquete satura la cola, se dispara la excepción `NOT_ENOUGH_SPACE`. En 101 funciones de `Protocol.bas`, el bloque `Errhandler:` captura el error, ejecuta `Call FlushBuffer(UserIndex)` y luego `Resume` para reintentar la instrucción. Sin embargo:
  1. `FlushBuffer` toma los 10 KB y los intenta enviar síncronamente mediante `send()` en `WsApiEnviar`.
  2. Si el búfer del kernel del sistema operativo está lleno (cliente laggeado o ventana TCP saturada), `send()` retorna `-1` con error `WSAEWOULDBLOCK` (10035).
  3. Al recibir `WSAEWOULDBLOCK`, `WsApiEnviar` reinyecta íntegramente los 10 KB de vuelta en `outgoingData` (`Call UserList(Slot).outgoingData.WriteASCIIStringFixed(str)`).
  4. La instrucción `Resume` en `Protocol.bas` vuelve a intentar escribir sobre la cola que sigue 100% llena, relanzando `NOT_ENOUGH_SPACE`, llamando nuevamente a `FlushBuffer`, rebotando en `WSAEWOULDBLOCK` y ejecutando `Resume` de forma perpetua.
  5. Dado que todo el servidor legacy corre en un único hilo de ejecución (STA), este rebote genera un **bucle infinito ocupado (busy-wait) al 100% de CPU**, congelando el servidor completo.
- **Camino de Producción**: **Activo**.
- **Estado en C++**: **Mitigated (Safe Backpressure)**. En el diseño con Asio se elimina el control de flujo por excepciones y reintentos infinitos, reemplazándolo por una cola de buffers salientes con umbral máximo de seguridad (backpressure limit); si un cliente satura su cola saliente sin drenar, se programa su desconexión controlada protegiendo la estabilidad del servidor. Implementado en `src/server/TCP.cpp` (`MAX_OUTGOING_BUFFER_SIZE`) y probado en `tests/test_tcp.cpp:653-698`.
- **Documentación Detallada**: [`14-tcp.md`](14-tcp.md#2-mitigación-de-backpressure-bug-19) y [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#54-umbrales-de-socket-os-y-la-trampa-de-bloqueo-wsaewouldblock).

---

### Entrada #20 — `TCP` / `wskapiAO`: Socket Leak por Invocación Prematura de `WSApiCloseSocket` en Rechazo Anti-Flood
- **Cita Legacy**: [`legacy/server/Codigo/wskapiAO.bas:402-405, 420`](legacy/server/Codigo/wskapiAO.bas#L402-L405).
- **Descripción**: En la subrutina `EventoSockAccept`:
  1. Se acepta la conexión entrante: `Ret = accept(SockID, sa, Tam)` en la línea 394.
  2. Inmediatamente en la línea 402 se valida la seguridad de la IP:
     ```vb
     If Not SecurityIp.IpSecurityAceptarNuevaConexion(sa.sin_addr) Then
         Call WSApiCloseSocket(NuevoSock)
         Exit Sub
     End If
     ```
  3. Sin embargo, la variable local `NuevoSock` recién se asigna **15 líneas después** en la línea 420 (`NuevoSock = Ret`).
  4. Como consecuencia, si el anti-flood rechaza la conexión, `WSApiCloseSocket` recibe `NuevoSock = 0` (cerrando el descriptor 0 correspondiente a stdin o inválido), mientras que el descriptor TCP real devuelto por `accept()` (`Ret`) queda huérfano y abierto en el kernel de Windows, provocando una **fuga acumulativa de descriptores de sockets (socket leak)** ante ráfagas de ataques de conexión.
- **Camino de Producción**: **Activo**.
- **Estado en C++**: **Mitigated (Safe RAII / Explicit Close)**. En C++, la aceptación del socket se realiza mediante `asio::ip::tcp::socket`, cuyo destructor RAII garantiza el cierre inmediato del descriptor si la validación anti-flood rechaza la conexión antes de enrolarla en la sesión. Implementado en `src/server/TCP.cpp:61` y probado en `tests/test_tcp.cpp:301-341`.
- **Documentación Detallada**: [`14-tcp.md`](14-tcp.md#3-cierre-seguro-raii-en-aceptación-bug-20) y [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#c-procesamiento-del-evento-de-conexión-entrante-fd_accept).

---

### Entrada #21 — `modSendData`: Constante `SendTarget.ToGM` Huérfana en el Enumerador de Ruteo
- **Cita Legacy**: [`legacy/server/Codigo/modSendData.bas:39, 70-302`](../../legacy/server/Codigo/modSendData.bas#L39).
- **Descripción**: La constante `ToGM = 6` está declarada formalmente en la definición pública de `SendTarget` ([L39](../../legacy/server/Codigo/modSendData.bas#L39)). Sin embargo, en el despachador maestro `SendData` ([L70-L302](../../legacy/server/Codigo/modSendData.bas#L70-L302)), no existe ninguna rama `Case SendTarget.ToGM`. Si cualquier módulo o función invocara `SendData(SendTarget.ToGM, ...)`, la ejecución salta silenciosamente sin despachar bytes ni emitir aviso alguno. En el código legacy la funcionalidad administrativa fue cubierta posteriormente por `ToAdmins` y `ToGMsAreaButRmsOrCounselors`.
- **Camino de Producción**: **Muerto / Huérfano**.
- **Estado en C++**: **Excluded (dead code, not ported)**. La constante no se incluye en el `enum class SendTarget` de C++20, eliminando código muerto y garantizando que un `switch` con control exhaustivo detecte en tiempo de compilación cualquier caso no manejado.
- **Documentación Detallada**: [`docs/audit/02d-modsenddata-detalle.md`](../audit/02d-modsenddata-detalle.md#61-sendtargettogm-huérfano) y [`15-modsenddata-breakdown.md`](15-modsenddata-breakdown.md#14-mitigación-de-bugs-legacy).

---

### Entrada #22 — `modSendData`: Omisión de Verificación `UserLogged` en Broadcasts Globales Administrativos y Faccionarios
- **Cita Legacy**: [`legacy/server/Codigo/modSendData.bas:116-200`](../../legacy/server/Codigo/modSendData.bas#L116-L200) (Bloques `ToAdmins`, `ToHigherAdmins`, `ToConsejo`, `ToConsejoCaos`, `ToRolesMasters`, `ToCiudadanos`, `ToCriminales`, `ToReal`, `ToCaos`, etc.).
- **Descripción**: Mientras que las difusiones globales estándar `ToAll` y `ToAllButIndex` evalúan rigurosamente tanto `UserList(LoopC).ConnID <> -1` como `UserList(LoopC).flags.UserLogged`, todos los canales globales de administración y facción omiten evaluar `flags.UserLogged`, verificando únicamente `ConnID <> -1`. Si una conexión TCP está abierta y asignada a un slot pero el usuario aún no completó la autenticación (está en la pantalla de login o selección de personaje), un paquete administrativo o faccionario global le sería transmitido si los campos correspondientes contienen valores residuales o estados preexistentes.
- **Camino de Producción**: **Activo**.
- **Estado en C++**: **Mitigated (Safe Logic)**. En la implementación en C++20, todas las funciones de filtrado global incorporan obligatoriamente la verificación de que la sesión se encuentre plenamente autenticada e ingresada al mundo (`is_logged_in()`), mitigando cualquier fuga o desincronización de paquetes durante la fase previa al login.
- **Documentación Detallada**: [`docs/audit/02d-modsenddata-detalle.md`](../audit/02d-modsenddata-detalle.md#62-ausencia-de-verificación-flagsuserlogged-en-broadcasts-globales) y [`15-modsenddata-breakdown.md`](15-modsenddata-breakdown.md#14-mitigación-de-bugs-legacy).

