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
| **01** | `clsClan` / `modGuilds` | `clsClan.cls:593-596`<br>`clsdicc.cls:124-127` | Empate electoral no reelige líder, cierra comicios y reporta cantidad de empatados como votos | **Activo** | **Replicated**<br>[`src/server/clsClan.cpp:387-414`](src/server/clsClan.cpp#L387-L414)<br>Test: [`test_clsclan.cpp:170`](tests/test_clsclan.cpp#L170) | [`15-clsclan-modguilds.md`](15-clsclan-modguilds.md#2-resolucion-de-empates-en-elecciones-contarvotos--mayorvalor) |
| **02** | `clsdicc` | `clsdicc.cls:124-127` | `MayorValor(cant)` concatena claves empatadas por inserción y retorna en `cant` la cantidad de empatados en vez de los votos máximos | **Activo** | **Replicated**<br>[`src/server/clsdicc.cpp:78-107`](src/server/clsdicc.cpp#L78-L107)<br>Test: [`test_clsdicc.cpp:16`](tests/test_clsdicc.cpp#L16) | [`04-clsdicc.md`](04-clsdicc.md#3-preservacion-del-orden-de-insercion-y-comportamiento-de-empates-en-mayorvalor) |
| **03** | `clsdicc` | `clsdicc.cls:21` | Límite estático de 100 elementos (`MAX_ELEM = 100`) omitido para usar capacidad dinámica en memoria | **Activo** | **Replicated (Desviación Intencional)**<br>[`src/server/clsdicc.hpp:20`](src/server/clsdicc.hpp#L20) | [`04-clsdicc.md`](04-clsdicc.md#2-eliminacion-del-cap-de-100-elementos-desviacion-intencional-documentada) |
| **04** | `clsIniReader` | `clsIniReader.cls:437-444` | `KeyExists(name)` solo valida la existencia de la sección `[name]`, no de una clave individual dentro de la sección | **Activo** | **Replicated**<br>[`src/server/clsIniReader.cpp:166`](src/server/clsIniReader.cpp#L166)<br>Test: [`test_clsinireader.cpp:52`](tests/test_clsinireader.cpp#L52) | [`03-clsinireader.md`](03-clsinireader.md#quirk-prominente-de-keyexists) |
| **05** | `ModCola` (`cCola`) | `ModCola.cls:1-50` | Retorno de `"0"` ante cola vacía o índices fuera de rango por coerción de tipo y `On Error Resume Next` de VB6 | **Activo** | **Replicated**<br>[`src/server/ModCola.cpp:44,54,76`](src/server/ModCola.cpp#L44)<br>Test: [`test_modcola.cpp:54`](tests/test_modcola.cpp#L54) | [`06-modcola-queue-colaarray.md`](06-modcola-queue-colaarray.md#respuestas-por-defecto-en-casos-error-o-cola-vacia) |
| **06** | `modHexaStrings` | `modHexaStrings.bas:18-20` | Relleno implícito con cero a la izquierda si la longitud de entrada es impar (`Len(MD5) And &H1`) en `hexMd52Asc` | **Activo** | **Replicated**<br>[`src/server/modHexaStrings.cpp:14`](src/server/modHexaStrings.cpp#L14)<br>Test: [`test_modhexastrings.cpp:25`](tests/test_modhexastrings.cpp#L25) | [`07-modhexastrings.md`](07-modhexastrings.md#3-relleno-de-ceros-a-la-izquierda-zero-padding-para-longitud-impar) |
| **07** | `modHexaStrings` | `modHexaStrings.bas:40-49` | `hexHex2Dec` detiene silenciosamente el parseo ante caracteres no hexadecimales y devuelve el acumulado (`Val("&H...")`) | **Activo** | **Replicated**<br>[`src/server/modHexaStrings.cpp:38`](src/server/modHexaStrings.cpp#L38)<br>Test: [`test_modhexastrings.cpp:42`](tests/test_modhexastrings.cpp#L42) | [`07-modhexastrings.md`](07-modhexastrings.md#4-tolerancia-a-caracteres-no-hexadecimales-y-espacios-valh--hex) |
| **08** | `cSolicitud` / `clsClan` | `cSolicitud.cls:18-20`<br>`clsClan.cls:315-325` | Desacoplamiento entre nombres de miembros en memoria (`UserName`, `desc`) y claves en disco `.sol` (`Nombre`, `Detalle`) | **Activo** | **Replicated**<br>[`src/server/cSolicitud.hpp:10`](src/server/cSolicitud.hpp#L10)<br>[`src/server/clsClan.cpp:202`](src/server/clsClan.cpp#L202) | [`08-csolicitud.md`](08-csolicitud.md#2-nombres-de-miembros-de-datos-segun-la-naming-policy) |
| **09** | `FileIO` (Grupo 5) | `FileIO.bas:895-903` | En `LoadOBJData`, typo en `CP<N>` causaba `Subscript out of range` (Error 9); en C++ se asigna `0` seguro | **Activo** | **Replicated (Desviación Deliberada de Seguridad)**<br>[`src/server/FileIO.cpp:520`](src/server/FileIO.cpp#L520)<br>Test: [`test_fileio_gamedata.cpp:66`](tests/test_fileio_gamedata.cpp#L66) | [`12-fileio-tablas-datos.md`](12-fileio-tablas-datos.md#3-analisis-de-caso-de-borde-en-loadobjdata-claseprohibida) |
| **10** | `FileIO` (Grupo 7) | `FileIO.bas:2158, 2178` | `LogBanFromName` y `Ban` persisten registros en `logs/BanDetail.dat` con extensión `.dat` en lugar de `.log` | **Activo** | **Replicated**<br>[`src/server/FileIO.cpp:1159, 1177`](src/server/FileIO.cpp#L1159)<br>Test: [`test_fileio_backup_logging.cpp:45`](tests/test_fileio_backup_logging.cpp#L45) | [`14-fileio-backup-logging.md`](14-fileio-backup-logging.md#a-formatos-de-registro-y-quirk-historico-de-extensiones-log-vs-dat) |
| **11** | `SecurityIp` #1 | `SecurityIp.bas:291` / `310` | Búsqueda binaria retorna `~(Middle * 2)` en vez de `~(First * 2)`, insertando fuera de orden | **Activo** (`IP_INTERVALOS`) / **Muerto** (`IP_LIMITECONEXIONES`) | **Replicated**<br>[`src/server/SecurityIp.cpp:44`](src/server/SecurityIp.cpp#L44)<br>Test: [`test_securityip.cpp:121`](tests/test_securityip.cpp#L121) | [`16a-securityip-known-bugs.md`](16a-securityip-known-bugs.md#bug-1-retorno-de-middle--2-en-vez-de-first--2-en-búsqueda-binaria) |
| **12** | `SecurityIp` #2 | `SecurityIp.bas:278` / `296` | Cota superior inicial `Last = MaxValue` (off-by-one) que compara contra elemento no inicializado o fuera de rango | **Activo** (`IP_INTERVALOS`) / **Muerto** (`IP_LIMITECONEXIONES`) | **Replicated**<br>[`src/server/SecurityIp.cpp:21`](src/server/SecurityIp.cpp#L21)<br>Test: [`test_securityip.cpp:17`](tests/test_securityip.cpp#L17) | [`16a-securityip-known-bugs.md`](16a-securityip-known-bugs.md#quirk-2-cota-superior-inicial-desplazada-last--maxvalue-off-by-one) |
| **13** | `SecurityIp` #3 | `SecurityIp.bas:247` | Sobrecopia de 16 bytes en compactación de `MaxConTables` dentro de `IpRestarConexion` provocando lecturas y escrituras fuera de rango | **Muerto / Comentado** | **Excluded (dead code, not ported)** | [`16a-securityip-known-bugs.md`](16a-securityip-known-bugs.md#bug-3-sobrecopia-de-memoria--lectura-fuera-de-rango-en-iprestarconexion)<br>Detalle: [`../audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#43-estado-de-ipsecuritysuperalimiteconexiones-e-iprestarconexion) |
| **14** | `SecurityIp` #4 | `SecurityIp.bas:180-188` | Retorno permisivo (`False`) ante agotamiento de slots en `IPSecuritySuperaLimiteConexiones`, admitiendo conexiones sin registrar | **Muerto / Comentado** | **Excluded (dead code, not ported)** | [`16a-securityip-known-bugs.md`](16a-securityip-known-bugs.md#bug-4-retorno-permisivo-ante-agotamiento-de-slots-en-ipsecuritysuperalimiteconexiones)<br>Detalle: [`../audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#43-estado-de-ipsecuritysuperalimiteconexiones-e-iprestarconexion) |
| **15** | `SecurityIp` #5 | `SecurityIp.bas:86-97` | Asimetría de mantenimiento horario: purga `IpTables` pero no interviene sobre `MaxConTables` | **Activo** | **Replicated**<br>[`src/server/SecurityIp.cpp:78`](src/server/SecurityIp.cpp#L78)<br>Test: [`test_securityip.cpp:325`](tests/test_securityip.cpp#L325) | [`16a-securityip-known-bugs.md`](16a-securityip-known-bugs.md#quirk-5-asimetria-en-el-mantenimiento-periodico-ipsecuritymantenimientolista) |
| **16** | `SecurityIp` #6 | `SecurityIp.bas:111` | Error 6 ("Overflow") de VB6 tras ~24.85 días de uptime (`SERVER.VBP` `OverflowCheck=0`) | **Activo** | **Replicated (Excepción Tipada)**<br>[`SecurityIp::TickCountOverflowException`](src/server/SecurityIp.hpp)<br>Test: [`test_securityip.cpp:342`](tests/test_securityip.cpp#L342) | [`16a-securityip-known-bugs.md`](16a-securityip-known-bugs.md#quirk-6-aritmetica-temporal-con-desbordamiento--wraparound-de-gettickcount) |
| **17** | `cColaArray` | `cColaArray.cls` | Clase de búfer circular de texto inalcanzable, bloqueada bajo `#If UsarQueSocket = 3` y sin campos en `User` | **Muerto / Inalcanzable** | **Excluded (dead code, not ported)** | [`docs/audit/06a-colaarray-dead-code.md`](../audit/06a-colaarray-dead-code.md) |
| **18** | `clsAntiMassClon` | `clsAntiMassClon.cls:46-67, 58-63`<br>`SERVER.VBP:76` | Inserción de IPs inoperante por condicional `#If SeguridadAlkon` apagado y tipo inexistente `UserIpAdress` (bypass total en producción) | **Muerto / Inoperante** | **Excluded (dead code, not ported)** | [`docs/audit/02b-antimassclon-detalle.md`](../audit/02b-antimassclon-detalle.md) |
| **19** | `TCP` / `wskapiAO` | `wskapiAO.bas:334-342`<br>`clsByteQueue.cls:196-199`<br>`Protocol.bas:858-863` | Congelamiento del servidor por bucle infinito ocupado (busy-loop) ante `WSAEWOULDBLOCK` en `WsApiEnviar` combinado con `NOT_ENOUGH_SPACE` y `Resume` | **Activo** | **Mitigated (Safe Backpressure)**<br>[`TCP::EnviarDatosASlot`](src/server/TCP.cpp)<br>Test: [`test_tcp.cpp:653`](tests/test_tcp.cpp#L653) | [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#54-umbrales-de-socket-os-y-la-trampa-de-bloqueo-wsaewouldblock)<br>[`17-tcp.md`](17-tcp.md#2-mitigación-de-backpressure-bug-19) |
| **20** | `TCP` / `wskapiAO` | `wskapiAO.bas:402-405, 420` | Fuga de descriptor de socket (*socket leak*) en rechazo anti-flood por invocar `WSApiCloseSocket(NuevoSock)` antes de asignar `NuevoSock = Ret` (cerrando descriptor 0) | **Activo** | **Mitigated (Safe RAII / Explicit Close)**<br>[`TCP::HandleAccept`](src/server/TCP.cpp)<br>Test: [`test_tcp.cpp:301`](tests/test_tcp.cpp#L301) | [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#c-procesamiento-del-evento-de-conexión-entrante-fd_accept)<br>[`17-tcp.md`](17-tcp.md#3-cierre-seguro-raii-en-aceptación-bug-20) |
| **21** | `modSendData` | `modSendData.bas:39, 70-302` | Constante `SendTarget.ToGM` declarada en el Enum pero sin bloque de procesamiento en el `Select Case` de `SendData` (cae en el vacío) | **Muerto / Huérfano** | **Excluded (dead code, not ported)** | [`docs/audit/02d-modsenddata-detalle.md`](../audit/02d-modsenddata-detalle.md#61-sendtargettogm-huérfano) |
| **22** | `modSendData` | `modSendData.bas:116-200` | Broadcasts administrativos y faccionarios no chequean `flags.UserLogged`, enviando paquetes de juego a sockets en pantalla de login | **Activo** | **Mitigated (Safe Logic)** | [`docs/audit/02d-modsenddata-detalle.md`](../audit/02d-modsenddata-detalle.md#62-ausencia-de-verificación-flagsuserlogged-en-broadcasts-globales)<br>[`modSendData-breakdown.md`](modSendData-breakdown.md#14-mitigación-de-bugs-legacy) |

---

## Detalle de Entradas Específicas

A continuación se documentan en detalle aquellas entradas que provienen de módulos previos y no contaban con un documento de bug dedicado individual (como sí lo tiene `SecurityIp.bas` en `16a-securityip-known-bugs.md`).

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
- **Referencia**: [`15-clsclan-modguilds.md`](15-clsclan-modguilds.md#2-resolucion-de-empates-en-elecciones-contarvotos--mayorvalor).

---

### Entrada #02 — `clsdicc`: Concatención por Inserción y Retorno de Empates en `MayorValor`
- **Cita Legacy**: [`legacy/server/Codigo/clsdicc.cls:124-127`](legacy/server/Codigo/clsdicc.cls#L124-L127).
- **Descripción**: La función `MayorValor(cant)` busca la clave con el valor numérico más alto. Si existen varias claves con dicho valor:
  1. Concatena los nombres de todas las claves separadas por comas en el **orden de inserción original** (no en orden alfabético).
  2. Sobrescribe el parámetro por referencia `cant` con la **cantidad de claves empatadas**, en lugar del puntaje o cantidad de votos alcanzada.
- **Camino de Producción**: **Activo**. Es el motor subyacente que consume `clsClan.cls` para la votación de líderes.
- **Estado en C++**: **Replicated**. Implementado en `src/server/clsdicc.cpp:78-107` manteniendo un vector auxiliar `m_order` para garantizar el orden de inserción sin penalizar el acceso en tiempo constante. Probado en `tests/test_clsdicc.cpp:16-56`.
- **Referencia**: [`04-clsdicc.md`](04-clsdicc.md#3-preservacion-del-orden-de-insercion-y-comportamiento-de-empates-en-mayorvalor).

---

### Entrada #03 — `clsdicc`: Capacidad Estática de 100 Elementos (`MAX_ELEM = 100`)
- **Cita Legacy**: [`legacy/server/Codigo/clsdicc.cls:21`](legacy/server/Codigo/clsdicc.cls#L21).
- **Descripción**: En VB6, `clsdicc` utilizaba un array estático de tamaño fijo `arr(MAX_ELEM)`. Si un clan superaba los 100 miembros votantes o se insertaban más de 100 elementos, la operación fallaba o desbordaba el array.
- **Camino de Producción**: **Activo**.
- **Estado en C++**: **Replicated (Desviación Intencional Documentada)**. Se omitió la cota de 100 elementos sustituyendo el array estático por contenedores dinámicos estándar (`std::unordered_map` y `std::vector`) en `src/server/clsdicc.hpp:20-25`. Esta decisión se tomó al tratarse de una limitación de bajo nivel de VB6 para no utilizar memoria dinámica y no de una regla de negocio del juego.
- **Referencia**: [`04-clsdicc.md`](04-clsdicc.md#2-eliminacion-del-cap-de-100-elementos-desviacion-intencional-documentada) y [`../audit/01a-clsdicc-cgarbage.md`](../audit/01a-clsdicc-cgarbage.md).

---

### Entrada #04 — `clsIniReader`: Falsa Comprobación de Clave en `KeyExists`
- **Cita Legacy**: [`legacy/server/Codigo/clsIniReader.cls:437-444`](legacy/server/Codigo/clsIniReader.cls#L437-L444).
- **Descripción**: La función pública `KeyExists(name)` ejecuta internamente `FindMain(UCase$(name)) >= 0`. A pesar de que su nombre indica comprobación de clave (*key*), **valida únicamente si existe la sección principal (`MainNode`, `[name]`)** y no un par clave-valor específico.
- **Camino de Producción**: **Activo**. Utilizado a lo largo del servidor para verificar si un encabezado de sección se encuentra en memoria.
- **Estado en C++**: **Replicated**. Preservado idénticamente en `src/server/clsIniReader.cpp:166-169` para evitar romper las llamadas que dependen de esta semántica original. Probado en `tests/test_clsinireader.cpp:52-60`.
- **Referencia**: [`03-clsinireader.md`](03-clsinireader.md#quirk-prominente-de-keyexists).

---

### Entrada #05 — `ModCola` (`cCola`): Respuestas de Error `"0"` ante Índices Inválidos o Cola Vacía
- **Cita Legacy**: [`legacy/server/Codigo/ModCola.cls:1-50`](legacy/server/Codigo/ModCola.cls#L1-L50).
- **Descripción**: La clase de gestión de pedidos de ayuda de Game Masters (`/AYUDA`) recurría a `On Error Resume Next` sobre un objeto `Collection`. Al consultar o descolar sobre una cola vacía o mediante un índice fuera de rango, las funciones devuelven la cadena `"0"` en lugar de una cadena vacía o código de falla.
- **Camino de Producción**: **Activo**. Consumido por el comando administrativo `/AYUDA` y la visualización de peticiones en el cliente de GM.
- **Estado en C++**: **Replicated**. Preservado en `src/server/ModCola.cpp:44, 54, 76` y verificado en `tests/test_modcola.cpp:54-68`.
- **Referencia**: [`06-modcola-queue-colaarray.md`](06-modcola-queue-colaarray.md#respuestas-por-defecto-en-casos-de-error-o-cola-vacia).

---

### Entrada #06 — `modHexaStrings`: Padding Impar Implícito en `hexMd52Asc`
- **Cita Legacy**: [`legacy/server/Codigo/modHexaStrings.bas:18-20`](legacy/server/Codigo/modHexaStrings.bas#L18-L20).
- **Descripción**: Si la cadena hexadecimal provista posee un número impar de caracteres (`Len(MD5) And &H1`), el procedimiento le antepone automáticamente un `"0"` a la izquierda antes de segmentarla en pares de 2 dígitos.
- **Camino de Producción**: **Activo**. Participa en la carga y comparación de los hashes MD5 aceptados de los clientes en `Admin.bas:MD5sCarga`.
- **Estado en C++**: **Replicated**. Implementado en `src/server/modHexaStrings.cpp:14-17` y verificado en `tests/test_modhexastrings.cpp:25-32`.
- **Referencia**: [`07-modhexastrings.md`](07-modhexastrings.md#3-relleno-de-ceros-a-la-izquierda-zero-padding-para-longitud-impar).

---

### Entrada #07 — `modHexaStrings`: Truncamiento Silencioso ante Caracteres No Hexadecimales en `hexHex2Dec`
- **Cita Legacy**: [`legacy/server/Codigo/modHexaStrings.bas:40-49`](legacy/server/Codigo/modHexaStrings.bas#L40-L49).
- **Descripción**: Emulando la semántica permisiva de `Val("&H" & hex)` de Visual Basic, la función recorre la cadena e interrumpe el procesamiento en cuanto encuentra el primer carácter no válido, retornando la porción decimal acumulada hasta ese instante sin lanzar errores.
- **Camino de Producción**: **Activo**.
- **Estado en C++**: **Replicated**. Implementado en `src/server/modHexaStrings.cpp:38-51` y verificado en `tests/test_modhexastrings.cpp:42-53`.
- **Referencia**: [`07-modhexastrings.md`](07-modhexastrings.md#4-tolerancia-a-caracteres-no-hexadecimales-y-espacios-valh--hex).

---

### Entrada #08 — `cSolicitud` / `clsClan`: Desacoplamiento de Claves en Frontera de Persistencia
- **Cita Legacy**: [`legacy/server/Codigo/cSolicitud.cls:18-20`](legacy/server/Codigo/cSolicitud.cls#L18-L20) vs [`legacy/server/Codigo/clsClan.cls:315-325`](legacy/server/Codigo/clsClan.cls#L315-L325).
- **Descripción**: La clase `cSolicitud` declara sus miembros en memoria como `UserName` y `desc`. Sin embargo, `clsClan.cls` guarda y lee de disco bajo las secciones `[SOLICITUD<N>]` las claves `Nombre` y `Detalle`.
- **Camino de Producción**: **Activo**.
- **Estado en C++**: **Replicated**. El struct `cSolicitud` conserva los identificadores canónicos `UserName` y `desc` (`src/server/cSolicitud.hpp:10-13`), mientras que la lógica de I/O en `clsClan.cpp:202-230` efectúa la traducción bidireccional contra `Nombre` y `Detalle`.
- **Referencia**: [`08-csolicitud.md`](08-csolicitud.md#2-nombres-de-miembros-de-datos-segun-la-naming-policy) y [`15-clsclan-modguilds.md`](15-clsclan-modguilds.md#1-frontera-de-serializacion-de-csolicitud-mapeo-dto-vs-claves-ini).

---

### Entrada #09 — `FileIO` (Grupo 5): Error de Subíndice en `LoadOBJData` (`CP<N>`)
- **Cita Legacy**: [`legacy/server/Codigo/FileIO.bas:895-903`](legacy/server/Codigo/FileIO.bas#L895-L903).
- **Descripción**: Al cargar las clases prohibidas de un objeto, si el archivo `Dat/Obj.dat` contiene un valor no coincidente en `CP<N>` (un error tipográfico o clase inexistente), el bucle `Do While` en VB6 incrementaba `N` hasta `NUMCLASES + 1` (13), produciendo un fallo de runtime `Subscript out of range` (Error 9 de VB6).
- **Camino de Producción**: **Activo** (en el arranque del servidor al parsear `Obj.dat`).
- **Estado en C++**: **Replicated (Desviación Deliberada de Seguridad)**. En C++, la búsqueda se acota estrictamente dentro del rango `1` a `NUMCLASES` y ante discrepancias asigna `0` (`static_cast<eClass>(0)`, sin restricción de clase), previniendo desbordamientos de memoria o fallos fatales en tiempo de ejecución. Implementado en `src/server/FileIO.cpp:520` y validado en `tests/test_fileio_gamedata.cpp:66-79`.
- **Referencia**: [`12-fileio-tablas-datos.md`](12-fileio-tablas-datos.md#3-analisis-de-caso-de-borde-en-loadobjdata-claseprohibida).

---

### Entrada #10 — `FileIO` (Grupo 7): Quirk de Extensión de Registro (`BanDetail.dat`)
- **Cita Legacy**: [`legacy/server/Codigo/FileIO.bas:2158, 2178`](legacy/server/Codigo/FileIO.bas#L2158).
- **Descripción**: Mientras que `LogBan` escribe en `logs/BanDetail.log`, las rutinas `LogBanFromName` y `Ban` persisten la información en `logs/BanDetail.dat` utilizando la extensión `.dat` en lugar de `.log`.
- **Camino de Producción**: **Activo** (ejecutado en sanciones de personajes por GMs).
- **Estado en C++**: **Replicated**. Se mantuvo la extensión histórica `.dat` en `src/server/FileIO.cpp:1159, 1177` para preservar la interoperabilidad con herramientas de monitoreo y scripts externos de administración. Probado en `tests/test_fileio_backup_logging.cpp:45-70`.
- **Referencia**: [`14-fileio-backup-logging.md`](14-fileio-backup-logging.md#a-formatos-de-registro-y-quirk-historico-de-extensiones-log-vs-dat).

---

### Entradas #11 a #16 — `SecurityIp.bas`: Bugs y Quirks del Subsistema de Seguridad IP
Para el desglose analítico completo de las entradas #11 a #16 correspondientes al módulo `SecurityIp.bas`:
- Bug #11 (Búsqueda binaria con inserción desordenada por `~(Middle * 2)`).
- Quirk #12 (Cota inicial `Last = MaxValue` con acceso espurio fuera de rango).
- Bug #13 (Sobrecopia de 16 bytes en `IpRestarConexion`): **Excluido por código muerto**, al confirmarse en [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#43-estado-de-ipsecuritysuperalimiteconexiones-e-iprestarconexion) y [`17-tcp.md`](17-tcp.md) que todas sus llamadas en `wskapiAO.bas:466` y `TCP.bas:626` están comentadas con apóstrofe en el servidor de producción.
- Bug #14 (Retorno permisivo ante saturación de slots en `IPSecuritySuperaLimiteConexiones`): **Excluido por código muerto**, al confirmarse en [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#43-estado-de-ipsecuritysuperalimiteconexiones-e-iprestarconexion) y [`17-tcp.md`](17-tcp.md) que su única llamada en `wskapiAO.bas:433` está comentada con apóstrofe en producción.
- Quirk #15 (Mantenimiento horario asimétrico en `IpSecurityMantenimientoLista`).
- Quirk #16 (Aritmética temporal propensa a wraparound de `GetTickCount`).

Consultá directamente el documento dedicado: [`16a-securityip-known-bugs.md`](16a-securityip-known-bugs.md).

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
- **Documentación Detallada**: [`17-tcp.md`](17-tcp.md#2-mitigación-de-backpressure-bug-19) y [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#54-umbrales-de-socket-os-y-la-trampa-de-bloqueo-wsaewouldblock).

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
- **Documentación Detallada**: [`17-tcp.md`](17-tcp.md#3-cierre-seguro-raii-en-aceptación-bug-20) y [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#c-procesamiento-del-evento-de-conexión-entrante-fd_accept).

---

### Entrada #21 — `modSendData`: Constante `SendTarget.ToGM` Huérfana en el Enumerador de Ruteo
- **Cita Legacy**: [`legacy/server/Codigo/modSendData.bas:39, 70-302`](../../legacy/server/Codigo/modSendData.bas#L39).
- **Descripción**: La constante `ToGM = 6` está declarada formalmente en la definición pública de `SendTarget` ([L39](../../legacy/server/Codigo/modSendData.bas#L39)). Sin embargo, en el despachador maestro `SendData` ([L70-L302](../../legacy/server/Codigo/modSendData.bas#L70-L302)), no existe ninguna rama `Case SendTarget.ToGM`. Si cualquier módulo o función invocara `SendData(SendTarget.ToGM, ...)`, la ejecución salta silenciosamente sin despachar bytes ni emitir aviso alguno. En el código legacy la funcionalidad administrativa fue cubierta posteriormente por `ToAdmins` y `ToGMsAreaButRmsOrCounselors`.
- **Camino de Producción**: **Muerto / Huérfano**.
- **Estado en C++**: **Excluded (dead code, not ported)**. La constante no se incluye en el `enum class SendTarget` de C++20, eliminando código muerto y garantizando que un `switch` con control exhaustivo detecte en tiempo de compilación cualquier caso no manejado.
- **Documentación Detallada**: [`docs/audit/02d-modsenddata-detalle.md`](../audit/02d-modsenddata-detalle.md#61-sendtargettogm-huérfano) y [`modSendData-breakdown.md`](modSendData-breakdown.md#14-mitigación-de-bugs-legacy).

---

### Entrada #22 — `modSendData`: Omisión de Verificación `UserLogged` en Broadcasts Globales Administrativos y Faccionarios
- **Cita Legacy**: [`legacy/server/Codigo/modSendData.bas:116-200`](../../legacy/server/Codigo/modSendData.bas#L116-L200) (Bloques `ToAdmins`, `ToHigherAdmins`, `ToConsejo`, `ToConsejoCaos`, `ToRolesMasters`, `ToCiudadanos`, `ToCriminales`, `ToReal`, `ToCaos`, etc.).
- **Descripción**: Mientras que las difusiones globales estándar `ToAll` y `ToAllButIndex` evalúan rigurosamente tanto `UserList(LoopC).ConnID <> -1` como `UserList(LoopC).flags.UserLogged`, todos los canales globales de administración y facción omiten evaluar `flags.UserLogged`, verificando únicamente `ConnID <> -1`. Si una conexión TCP está abierta y asignada a un slot pero el usuario aún no completó la autenticación (está en la pantalla de login o selección de personaje), un paquete administrativo o faccionario global le sería transmitido si los campos correspondientes contienen valores residuales o estados preexistentes.
- **Camino de Producción**: **Activo**.
- **Estado en C++**: **Mitigated (Safe Logic)**. En la implementación en C++20, todas las funciones de filtrado global incorporan obligatoriamente la verificación de que la sesión se encuentre plenamente autenticada e ingresada al mundo (`is_logged_in()`), mitigando cualquier fuga o desincronización de paquetes durante la fase previa al login.
- **Documentación Detallada**: [`docs/audit/02d-modsenddata-detalle.md`](../audit/02d-modsenddata-detalle.md#62-ausencia-de-verificación-flagsuserlogged-en-broadcasts-globales) y [`modSendData-breakdown.md`](modSendData-breakdown.md#14-mitigación-de-bugs-legacy).
