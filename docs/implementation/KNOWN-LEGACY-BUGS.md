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
| **19** | `TCP` / `Protocol` | `wskapiAO.bas:334-342`<br>`clsByteQueue.cls:196-199`<br>`Protocol.bas:858-863` | Congelamiento del servidor por bucle infinito ocupado (busy-loop) ante `WSAEWOULDBLOCK` en `WsApiEnviar` combinado con `NOT_ENOUGH_SPACE` y `Resume` | **Activo** | **Mitigated (Safe Backpressure & Erradicación en Protocol)**<br>[`TCP::EnviarDatosASlot`](src/server/TCP.cpp)<br>Primitivas `Protocol::Write...`<br>Test: [`test_tcp.cpp:653`](tests/test_tcp.cpp#L653), `test_protocol.cpp` | [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#54-umbrales-de-socket-os-y-la-trampa-de-bloqueo-wsaewouldblock)<br>[`14-tcp.md`](14-tcp.md#2-mitigación-de-backpressure-bug-19)<br>[`16-protocol.md`](16-protocol.md#2-erradicación-del-patrón-resume-en-primitivas-write-bug-19) |
| **20** | `TCP` / `wskapiAO` | `wskapiAO.bas:402-405, 420` | Fuga de descriptor de socket (*socket leak*) en rechazo anti-flood por invocar `WSApiCloseSocket(NuevoSock)` antes de asignar `NuevoSock = Ret` (cerrando descriptor 0) | **Activo** | **Mitigated (Safe RAII / Explicit Close)**<br>[`TCP::HandleAccept`](src/server/TCP.cpp)<br>Test: [`test_tcp.cpp:301`](tests/test_tcp.cpp#L301) | [`docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md#c-procesamiento-del-evento-de-conexión-entrante-fd_accept)<br>[`14-tcp.md`](14-tcp.md#3-cierre-seguro-raii-en-aceptación-bug-20) |
| **21** | `modSendData` | `modSendData.bas:39, 70-302` | Constante `SendTarget.ToGM` declarada en el Enum pero sin bloque de procesamiento en el `Select Case` de `SendData` (cae en el vacío) | **Muerto / Huérfano** | **Excluded (dead code, not ported)** | [`docs/audit/02d-modsenddata-detalle.md`](../audit/02d-modsenddata-detalle.md#61-sendtargettogm-huérfano) |
| **22** | `modSendData` | `modSendData.bas:116-200` | Broadcasts administrativos y faccionarios no chequean `flags.UserLogged`, enviando paquetes de juego a sockets en pantalla de login | **Activo** | **Mitigated (Safe Logic)** | [`docs/audit/02d-modsenddata-detalle.md`](../audit/02d-modsenddata-detalle.md#62-ausencia-de-verificación-flagsuserlogged-en-broadcasts-globales)<br>[`15-modsenddata-breakdown.md`](15-modsenddata-breakdown.md#14-mitigación-de-bugs-legacy) |
| **23** | `Protocol` | `Protocol.bas:825-835` | Serialización de color RGB truncado a 3 bytes en red a pesar de declararse como Long (32 bits / 4 bytes) en VB6 | **Quirk / Riesgo** | **Preserved (Byte-Exact Parity)**<br>[`Protocol::WriteChatOverHead`](src/server/Protocol.cpp)<br>Test: `test_protocol.cpp` | [`16-protocol.md`](16-protocol.md#3-truncamiento-a-3-bytes-de-colores-rgb-en-tramas-salientes-long-de-vb6-vs-red) |
| **24** | `ModAreas` | `ModAreas.bas:84, 139` | Colisión de `AreaID` por multiplicación no inyectiva `(X\9 + 1) * (Y\9 + 1)` entre celdas distintas | **Quirk / Bug Lógico** | **Replicated (Strict Parity)**<br>Fórmula literal de VB6 preservada | [`../audit/03a-modareas-detalle.md`](../audit/03a-modareas-detalle.md#32-identificador-de-área-areaid-y-colisiones-matemáticas)<br>[`17-modareas.md`](17-modareas.md#1-replicación-estricta-de-la-fórmula-de-areaid-bug-24)<br>[Entrada #24](#entrada-24--modareas-colisión-de-areaid-por-producto-no-inyectivo) |
| **25** | `ModAreas` | `ModAreas.bas:186-187, 330-331` | Persistencia de coordenadas negativas en `AreasInfo.MinX/MinY` (asignadas previo al clamp local) | **Quirk / Escala Desfasada** | **Replicated (Signed Int16)**<br>Tipado `int16_t` preservando aritmética literal | [`../audit/03a-modareas-detalle.md`](../audit/03a-modareas-detalle.md#34-condiciones-de-borde-y-aritmética-negativa-en-memoria)<br>[`17-modareas.md`](17-modareas.md#2-aritmética-de-viewport-y-persistencia-de-coordenadas-negativas-bug-25)<br>[Entrada #25](#entrada-25--modareas-persistencia-de-coordenadas-negativas-en-minx--miny) |
| **26** | `ModAreas` | `ModAreas.bas:57, 78, 89-130` | Auto-optimización periódica a disco (`AreasStats.dat`) y matriz huérfana inoperante (`PosToArea`) | **Muerto / I/O Obsoleta** | **Excluded (dead code / obsolete I/O, not ported)** | [`../audit/03a-modareas-detalle.md`](../audit/03a-modareas-detalle.md#6-detección-de-quirks-bugs-históricos-y-código-muerto)<br>[`17-modareas.md`](17-modareas.md#6-exclusiones-por-código-muerto-e-io-obsoleta-bug-26)<br>[Entrada #26](#entrada-26--modareas-auto-optimización-obsoleta-areasstatsdat-y-arreglo-ocioso-postoarea) |
| **27** | `Modulo_InventANDobj` | `Modulo_InventANDobj.bas:97-98`<br>`MODULO_NPCs.bas:219` | Descarte de `GiveGLD` en NPCs no pretorianos (`NPCTirarOro` comentada; solo pretorianos evalúan `GiveGLD`) | **Quirk / Asimetría** | **Replicated (Strict Parity)** | [`18-modulo-inventandobj.md`](18-modulo-inventandobj.md#2-replicación-del-bug-27-asimetría-de-givegld-en-npc_tirar_items)<br>[Entrada #27](#entrada-27--modulo_inventandobj-descarte-de-givegld-en-npcs-no-pretorianos) |
| **28** | `Modulo_InventANDobj` | `Modulo_InventANDobj.bas:45-51`<br>`Modulo_UsUaRiOs.bas:1552-1585` | Destrucción silenciosa de ítems y oro ante saturación espacial en `Tilelibre` (radio > 15 sin celdas transitables) | **Quirk / Pérdida Silenciosa** | **Replicated (Strict Parity)** | [`18-modulo-inventandobj.md`](18-modulo-inventandobj.md#3-replicación-del-bug-28-destrucción-silenciosa-de-drops-ante-retorno-nulo-de-tilelibre)<br>[Entrada #28](#entrada-28--modulo_inventandobj-destrucción-silenciosa-de-ítems-y-oro-por-saturación-espacial-en-tilelibre) |
| **29** | `InvUsuario` | `InvUsuario.bas:368-380` | Exploit histórico de duplicación en `DropObj` por desfase de cantidades (descuenta recortado pero crea íntegro) | **Activo / Exploit** | **Replicated (Strict Parity)** | [`19-invusuario-breakdown.md`](19-invusuario-breakdown.md#fase-1-g1--mutaciones-en-el-mundo-y-suelo)<br>[Entrada #29](#entrada-29--invusuario-exploit-histórico-de-duplicación-en-dropobj-por-desfase-de-cantidades)<br>Detalle: [`../audit/12b-invusuario-detalle.md`](../audit/12b-invusuario-detalle.md#41-exploit-de-duplicación-en-dropobj-bug-29) |
| **30** | `InvUsuario` | `InvUsuario.bas:228-268` | Pérdida silenciosa de saldo excedente en `TirarOro (> 500k)` (deducción incondicional de `Extra` en billetera) | **Quirk / Pérdida Silenciosa** | **Replicated (Strict Parity)** | [`19-invusuario-breakdown.md`](19-invusuario-breakdown.md#fase-2-g2--gestión-base-de-inventario-y-descarte)<br>[Entrada #30](#entrada-30--invusuario-pérdida-silenciosa-de-saldo-excedente-en-tiraroro--500k)<br>Detalle: [`../audit/12b-invusuario-detalle.md`](../audit/12b-invusuario-detalle.md#42-pérdida-silenciosa-de-saldo-en-tiraroro-bug-30) |
| **31** | `modBanco` | `modBanco.bas:168-172, 285-290` | Acreditación previa al débito en `UserDejaObj` y `UserReciveObj` (riesgo de duplicación neta si el débito aborta) | **Activo / Exploit** | **Replicated (Strict Parity)** | [`20-modbanco.md`](20-modbanco.md#2-replicación-del-bug-31-acreditación-previa-al-débito-item-dupe)<br>[`20-modbanco-breakdown.md`](20-modbanco-breakdown.md#fase-2-g2--retiro-y-depósito-de-objetos)<br>[Entrada #31](#entrada-31--modbanco-acreditación-previa-al-débito-en-userdejaobj-y-userreciveobj)<br>Detalle: [`../audit/12c-modbanco-detalle.md`](../audit/12c-modbanco-detalle.md#51-riesgos-de-desincronización-y-duplicación-de-ítems-item-dupe) |
| **32** | `modBanco` | `modBanco.bas:216-245, 247-299` | Ausencia de restricciones de almacenamiento para barcos, objetos faccionarios e ítems de novato (`Newbie = 1`) | **Quirk / Exploit** | **Replicated (Strict Parity)** | [`20-modbanco.md`](20-modbanco.md#3-replicación-del-bug-32-ausencia-de-restricciones-de-almacenamiento)<br>[`20-modbanco-breakdown.md`](20-modbanco-breakdown.md#fase-2-g2--retiro-y-depósito-de-objetos)<br>[Entrada #32](#entrada-32--modbanco-ausencia-de-restricciones-de-almacenamiento-para-barcos-faccionarios-y-novatos)<br>Detalle: [`../audit/12c-modbanco-detalle.md`](../audit/12c-modbanco-detalle.md#55-restricciones-de-ítems-faccionarios-barcos-newbies) |
| **33** | `Comercio` | `Comercio.bas:203-232, 257`<br>`Declares.bas:492, 496` | Asimetría de capacidad de slots en mercaderes NPC (`MAX_NORMAL_INVENTORY_SLOTS = 20` vs `MAX_INVENTORY_SLOTS = 30`) | **Quirk / Asimetría** | **Replicated (Strict Parity)** | [`21-comercio-breakdown.md`](21-comercio-breakdown.md#fase-1-g1--comercio-con-mercaderes-npc-comerciobas)<br>[Entrada #33](#entrada-33--comercio-asimetría-de-capacidad-de-slots-en-mercaderes-npc)<br>Detalle: [`../audit/12d-comercio-detalle.md`](../audit/12d-comercio-detalle.md#34-la-gran-asimetría-de-ranuras-max_normal_inventory_slots-vs-max_inventory_slots) |
| **34** | `mdlCOmercioConUsuario` | `mdlCOmercioConUsuario.bas:178-182, 221-225`<br>`Modulo_InventANDobj.bas:56-59` | Desborde a suelo y evaporación silenciosa de ítems al colmar inventario en intercambio seguro | **Quirk / Pérdida Silenciosa** | **Replicated (Strict Parity)** | [`21-comercio-breakdown.md`](21-comercio-breakdown.md#fase-3-g3--ejecución-bilateral-del-intercambio-p2p)<br>[Entrada #34](#entrada-34--mdlcomercioconusuario-desborde-a-suelo-y-evaporación-silenciosa-de-ítems-en-comercio-seguro)<br>Detalle: [`../audit/12d-comercio-detalle.md`](../audit/12d-comercio-detalle.md#53-capacidad-de-inventario-y-evaporación-de-ítems) |
| **35** | `mdlCOmercioConUsuario` | `mdlCOmercioConUsuario.bas:178-182, 221-225`<br>`Trabajo.bas:333` | Acreditación previa al débito y riesgo de duplicación masiva en intercambio P2P por desbordamiento de tipo en `QuitarObjetos` | **Activo / Exploit** | **Replicated (Strict Parity)** | [`21-comercio-breakdown.md`](21-comercio-breakdown.md#fase-3-g3--ejecución-bilateral-del-intercambio-p2p)<br>[Entrada #35](#entrada-35--mdlcomercioconusuario-acreditación-previa-al-débito-y-duplicación-de-ítems-en-comercio-p2p)<br>Detalle: [`../audit/12d-comercio-detalle.md`](../audit/12d-comercio-detalle.md#54-orden-de-transferencia-y-falla-de-atomicidad-riesgo-de-duplicación) |
| **36** | `mdlCOmercioConUsuario` | `mdlCOmercioConUsuario.bas:168, 210` | Omisión de validación de tope `MAXORO` y desbordamiento aritmético en transferencia P2P | **Quirk / Riesgo Aritmético** | **Replicated (Strict Parity)** | [`21-comercio-breakdown.md`](21-comercio-breakdown.md#fase-3-g3--ejecución-bilateral-del-intercambio-p2p)<br>[Entrada #36](#entrada-36--mdlcomercioconusuario-omisión-de-validación-de-maxoro-en-comercio-seguro)<br>Detalle: [`../audit/12d-comercio-detalle.md`](../audit/12d-comercio-detalle.md#52-desbordamiento-de-oro-statsgld) |
| **37** | `SistemaCombate` | `SistemaCombate.bas:271-274, 292-295` | Omisión de daño de flechas en `DañoMaxArma` para bono por fuerza con proyectiles | **Quirk / Asimetría Aritmética** | **Replicated (Strict Parity)** | [Entrada #37](#entrada-37--sistemacombate-omisión-de-daño-de-flechas-en-el-bono-de-fuerza-dañomaxarma)<br>Detalle: [`../audit/04a-sistemacombate-detalle.md`](../audit/04a-sistemacombate-detalle.md#61-descarte-del-maxhit-de-munición-en-bono-por-fuerza-bug-37) |
| **38** | `modHechizos` | `modHechizos.bas:1775-1855` | Omisión de asignación de `daño` en modificadores de Maná y Estamina (`HechizoPropUsuario`) | **Quirk / Bug Silencioso** | **Replicated (Strict Parity)**<br>[`src/server/modHechizos.cpp:218-245`](src/server/modHechizos.cpp#L218-L245)<br>Test: [`test_modhechizos.cpp:115`](tests/test_modhechizos.cpp#L115) | [Entrada #38](#entrada-38--modhechizos-omisión-de-asignación-de-daño-en-modificadores-de-maná-y-estamina-hechizopropusuario)<br>Detalle: [`23-modhechizos.md`](23-modhechizos.md#1-replicación-del-bug-38-omisión-de-daño-en-maná-y-estamina) |
| **39** | `modHechizos` | `modHechizos.bas:1104-1120` | Fuga de flujo de ejecución y asimetría de coste en muerte por resurrección (`HechizoEstadoUsuario`) | **Activo / Asimetría Transaccional** | **Replicated (Strict Parity)**<br>[`src/server/modHechizos.cpp:468-477`](src/server/modHechizos.cpp#L468-L477)<br>Test: [`test_modhechizos.cpp:190`](tests/test_modhechizos.cpp#L190) | [Entrada #39](#entrada-39--modhechizos-fuga-de-flujo-de-ejecución-y-asimetría-de-coste-en-muerte-por-resurrección-hechizoestadousuario)<br>Detalle: [`23-modhechizos.md`](23-modhechizos.md#2-replicación-del-bug-39-fuga-de-flujo-en-muerte-por-resurrección) |
| **40** | `modHechizos` | `modHechizos.bas:1139, 1159` | Colisión y sobrescritura mutua de contadores temporales entre Ceguera y Estupidez (`HechizoEstadoUsuario`) | **Activo / Colisión de Estados** | **Replicated (Strict Parity)**<br>[`src/server/modHechizos.cpp:498, 513`](src/server/modHechizos.cpp#L498)<br>Test: [`test_modhechizos.cpp:210`](tests/test_modhechizos.cpp#L210) | [Entrada #40](#entrada-40--modhechizos-colisión-de-contadores-temporales-entre-ceguera-y-estupidez-hechizoestadousuario)<br>Detalle: [`23-modhechizos.md`](23-modhechizos.md#3-replicación-del-bug-40-colisión-de-temporizadores-cegueraestupidez) |
| **41** | `modInvisibles` | `modInvisibles.bas:1-41`<br>`SERVER.VBP:45` | Módulo huérfano y rutina `PonerInvisible` sin invocaciones en el juego; rama `#Else` no compilable por variable no definida `Modo` | **Muerto / Huérfano** | **Excluded (dead code, not ported)** | [Entrada #41](#entrada-41--modinvisibles-módulo-huérfano-y-rutina-ponerinvisible-sin-invocaciones)<br>Detalle: [`docs/audit/14b-invisibles-detalle.md`](../audit/14b-invisibles-detalle.md) |
| **42** | `Trabajo` | `Trabajo.bas:1891` | Reducción del daño al 75% (`daño * 0.75`) en tirada exitosa de `DoGolpeCritico` en lugar de incrementarlo | **Quirk / Asimetría** | **Replicated (Strict Parity)**<br>[`src/server/Trabajo.cpp:1516`](src/server/Trabajo.cpp#L1516)<br>Test: [`test_trabajo.cpp`](tests/test_trabajo.cpp) | [`26-trabajo.md`](26-trabajo.md#31-replicación-del-bug-42-en-dogolpecritico)<br>Detalle: [`../audit/12e-trabajo-detalle.md`](../audit/12e-trabajo-detalle.md#41-quirk-prominente-en-dogolpecritico-reducción-de-daño) |
| **43** | `PathFinding` | `PathFinding.bas:217-237` | Variable `steps` inoperante sin incrementar en el bucle principal BFS (`SeekPath`) | **Activo / Bug de Bucle** | **Replicated (Strict Parity)** | [Entrada #43](#entrada-43--pathfinding-variable-steps-inoperante-sin-incrementar-en-el-bucle-principal-bfs-seekpath) |
| **44** | `PathFinding` | `PathFinding.bas:212-220` | Limpieza incompleta de matriz global `TmpArray` (`InitializeTable`) limitando reseteo a sub-grilla | **Activo / Side-Effect** | **Replicated (Strict Parity)** | [Entrada #44](#entrada-44--pathfinding-limpieza-incompleta-de-matriz-global-tmparray-initializetable) |
| **45** | `PathFinding` / `AI_NPC` | `PathFinding.bas:1-19`<br>`AI_NPC.bas:986` | Inversión histórica de coordenadas $X \leftrightarrow Y$ en la grilla interna del pathfinding | **Activo / Quirk Arquitectura** | **Replicated (Strict Parity)** | [Entrada #45](#entrada-45--pathfinding--ai_npc-asimetría-e-inversión-histórica-de-coordenadas-x--y) |
| **46** | `clsParty` / `mdParty` | `clsParty.cls:248` | Sustracción errónea del nivel del líder (`UserIndex`) en lugar del nivel de cada integrante en la suma ponderada durante la disolución | **Activo / Bug de Disolución** | **Replicated (Strict Parity)** | [Entrada #46](#entrada-46--clsparty--mdparty-sustracción-errónea-del-nivel-del-líder-en-lugar-de-cada-integrante-durante-la-disolución) |





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

---

### Entrada #23 — `Protocol`: Serialización de Color RGB Truncado a 3 Bytes (Long de VB6 vs. Payload de Red)
- **Cita Legacy**: [`legacy/server/Codigo/Protocol.bas:825-835`](../../legacy/server/Codigo/Protocol.bas#L825-L835) (`WriteChatOverHead`).
- **Descripción**: En la rutina `WriteChatOverHead`, el parámetro `color` se declara como `ByVal color As Long` (tipo de 32 bits, 4 bytes en memoria). Sin embargo, la serialización binaria en VB6 descompone y empaqueta únicamente 3 bytes individuales correspondientes a los canales R, G y B:
  ```vb
  Call .WriteByte(color And &HFF)
  Call .WriteByte((color \ &H100) And &HFF)
  Call .WriteByte((color \ &H10000) And &HFF)
  ```
  Descartando por completo el byte superior (`color \ &H1000000`). En una transliteración estándar en C++20, se tendería a serializar un `int32_t` completo (4 bytes), lo que rompería la alineación de lectura del cliente oficial de VB6 (que espera exactamente 3 lecturas `ReadByte` para recomponer el color RGB).
- **Camino de Producción**: **Quirk / Riesgo de Desalineación**.
- **Estado en C++**: **Preserved (Byte-Exact Parity)**. En `src/server/Protocol.cpp` (`PrepareMessageChatOverHead` y `WriteChatOverHead`), se preserva estrictamente la descomposición manual de los 3 bytes componentes individuales en Little-Endian, garantizando paridad binaria exacta a nivel de byte con el cliente original.
- **Documentación Detallada**: [`16-protocol.md`](16-protocol.md#3-truncamiento-a-3-bytes-de-colores-rgb-en-tramas-salientes-long-de-vb6-vs-red).

---

### Entrada #24 — `ModAreas`: Colisión de `AreaID` por Producto no Inyectivo
- **Cita Legacy**: [`legacy/server/Codigo/ModAreas.bas:84, 139`](../../legacy/server/Codigo/ModAreas.bas#L84).
- **Descripción**: La matriz `AreasInfo` precalcula el identificador de área mediante un producto simple de dos factores escalares en el rango $[1, 12]$:
  $$\text{AreaID}(X, Y) = \left( \left\lfloor \frac{X}{9} \right\rfloor + 1 \right) \times \left( \left\lfloor \frac{Y}{9} \right\rfloor + 1 \right)$$
  Al ser una multiplicación aritmética no inyectiva, múltiples pares de cuadrantes distintos comparten el mismo identificador numérico (ej. franjas (1,5), (2,3), (3,2), (5,1), (0,11) y (11,0) producen todas $\text{AreaID} = 12$). Si una entidad es teletransportada entre cuadrantes que comparten el mismo producto sin resetear `AreasInfo.AreaID = 0`, la instrucción de control:
  ```vb
  If UserList(UserIndex).AreasInfo.AreaID = AreasInfo(UserList(UserIndex).Pos.X, UserList(UserIndex).Pos.Y) Then Exit Sub
  ```
  asume falsamente que no hubo cambio de área y sale prematuramente sin despachar los paquetes de visibilidad.
- **Camino de Producción**: **Activo / Quirk Lógico**.
- **Estado en C++**: **Replicated**. Conforme a la política estricta de preservación histórica de [`docs/CONVENTIONS.md`](../CONVENTIONS.md#4-prohibición-estricta-de-optimizaciones-prematuras-y-corrección-de-bugs-lógicos-strict-prohibition-of-optimization--logic-bug-fixes), se replica exactamente la fórmula de VB6: `static_cast<uint8_t>((x / 9 + 1) * (y / 9 + 1))`. Queda prohibido sustituirla por fórmulas modernas o identificadores inyectivos.
- **Documentación Detallada**: [`docs/audit/03a-modareas-detalle.md`](../audit/03a-modareas-detalle.md#32-identificador-de-área-areaid-y-colisiones-matemáticas) y [`17-modareas.md`](17-modareas.md#1-replicación-estricta-de-la-fórmula-de-areaid-bug-24).

---

### Entrada #25 — `ModAreas`: Persistencia de Coordenadas Negativas en `MinX` / `MinY`
- **Cita Legacy**: [`legacy/server/Codigo/ModAreas.bas:186-187, 330-331`](../../legacy/server/Codigo/ModAreas.bas#L186-L187).
- **Descripción**: En la inicialización de área para nuevas entidades (`Head = USER_NUEVO`), cuando $X < 9$ o $Y < 9$, la fórmula `MinX = ((.Pos.X \ 9) - 1) * 9` arroja el valor `-9`. Las líneas 186-187 ejecutan `.AreasInfo.MinX = CInt(MinX)` persistiendo el valor negativo en la estructura del usuario. Las cláusulas posteriores `If MinX < 1 Then MinX = 1` operan únicamente sobre las variables locales del bucle de barrido inmediato. Cuando la entidad posteriormente camina hacia el Este o Sur, el código recupera este `-9` base y le aplica offsets literales (`MinX + 27 = 18`, etc.).
- **Camino de Producción**: **Activo / Escala Desfasada**.
- **Estado en C++**: **Replicated**. Se preservan los campos `MinX` y `MinY` en `AreaInfo` tipados como enteros con signo (`int16_t`), manteniendo la aritmética literal y los offsets relativos originales sin forzar saneamientos ni clampleos que romperían la escala asumida por el algoritmo de barrido.
- **Documentación Detallada**: [`docs/audit/03a-modareas-detalle.md`](../audit/03a-modareas-detalle.md#34-condiciones-de-borde-y-aritmética-negativa-en-memoria) y [`17-modareas.md`](17-modareas.md#2-aritmética-de-viewport-y-persistencia-de-coordenadas-negativas-bug-25).

---

### Entrada #26 — `ModAreas`: Auto-optimización Obsoleta (`AreasStats.dat`) y Arreglo Ocioso (`PosToArea`)
- **Cita Legacy**: [`legacy/server/Codigo/ModAreas.bas:57, 78, 89-130`](../../legacy/server/Codigo/ModAreas.bas#L57).
- **Descripción**: 
  1. `ModAreas` declara el arreglo privado `PosToArea(1 To 100) As Byte` y lo inicializa en `InitAreas` con `LoopC \ 9`. Sin embargo, ninguna rutina del servidor ni del cliente lee jamás este arreglo (código muerto).
  2. Las rutinas `InitAreas` y `AreasOptimizacion` abren y escriben un archivo INI en disco (`AreasStats.dat`) para promediar la concurrencia por día de semana y bloque horario, con el único fin de calcular el tamaño de reserva para `ReDim Preserve ConnGroups(Map).UserEntrys`.
- **Camino de Producción**: **Muerto (`PosToArea`) / Obsoleto (`AreasStats.dat`)**.
- **Estado en C++**: **Excluded (dead code / obsolete I/O, not ported)**. Clasificado bajo la *Única Excepción* de [`docs/CONVENTIONS.md`](../CONVENTIONS.md#4-prohibición-estricta-de-optimizaciones-prematuras-y-corrección-de-bugs-lógicos-strict-prohibition-of-optimization--logic-bug-fixes): `PosToArea` se omite por código formalmente muerto, y `AreasStats.dat` se descarta por ser una optimización arcaica de gestión de memoria de VB6 basada en I/O síncrona a disco. En C++, `ConnGroups` utiliza contenedores estándar en memoria sin persistencia bloqueante.
- **Documentación Detallada**: [`docs/audit/03a-modareas-detalle.md`](../audit/03a-modareas-detalle.md#6-detección-de-quirks-bugs-históricos-y-código-muerto) y [`17-modareas.md`](17-modareas.md#6-exclusiones-por-código-muerto-e-io-obsoleta-bug-26).

---

### Entrada #27 — `Modulo_InventANDobj`: Descarte de `GiveGLD` en NPCs no Pretorianos
- **Cita Legacy**: [`legacy/server/Codigo/Modulo_InventANDobj.bas:97-98`](../../legacy/server/Codigo/Modulo_InventANDobj.bas#L97-L98) y [`legacy/server/Codigo/MODULO_NPCs.bas:219`](../../legacy/server/Codigo/MODULO_NPCs.bas#L219).
- **Descripción**: En la rutina `MuereNpc` de `MODULO_NPCs.bas`, la llamada a `NPCTirarOro(MiNPC)` (rutina que arrojaba el oro configurado en el campo `.GiveGLD`) se encuentra deliberadamente comentada (`' Call NPCTirarOro(MiNPC)`). Por su parte, la rutina `NPC_TIRAR_ITEMS` solo evalúa y arroja el campo `.GiveGLD` (mediante `TirarOroNpc`) si la criatura es pretoriana (`If IsPretoriano Then ... If .GiveGLD > 0 Then Call TirarOroNpc(.GiveGLD, .Pos)`). Para todas las criaturas no pretorianas, el oro solo se arroja si fue configurado explícitamente dentro del arreglo `Drop(1..5)` con `ObjIndex = 12` (`iORO`). En consecuencia, cualquier valor asignado a la clave `GiveGLD` en `NPCs.dat` para NPCs normales es completamente ignorado al morir y jamás cae al suelo.
- **Camino de Producción**: **Quirk / Asimetría de Lógica**.
- **Estado en C++**: **Replicated (Strict Parity)**. En observancia estricta de la Regla #4 de [`docs/CONVENTIONS.md`](../CONVENTIONS.md#4-prohibición-estricta-de-optimizaciones-prematuras-y-corrección-de-bugs-lógicos-strict-prohibition-of-optimization--logic-bug-fixes), se replica con exactitud la asimetría original tanto en `Modulo_InventANDobj.cpp` como en `src/server/MODULO_NPCs.cpp:MuereNpc`: `NPCTirarOro` es una función inerte y `NPC_TIRAR_ITEMS` solo arroja `.GiveGLD` si `is_pretoriano` es `true`; las criaturas estándar continúan dependiendo exclusivamente de su matriz `Drop(1..5)` para arrojar oro. Probado en `tests/test_modulo_npcs.cpp`.
- **Documentación Detallada**: [`18-modulo-inventandobj.md`](18-modulo-inventandobj.md#2-replicación-del-bug-27-asimetría-de-givegld-en-npc_tirar_items), [`28-modulo-npcs.md`](28-modulo-npcs.md#22-réplica-estricta-del-bug-27-givegld-inerte) y [`docs/audit/13a-modulonpcs-detalle.md`](../audit/13a-modulonpcs-detalle.md#41-bug-27-givegld-inerte-para-npcs-no-pretorianos).

---

### Entrada #28 — `Modulo_InventANDobj`: Destrucción Silenciosa de Ítems y Oro por Saturación Espacial en `Tilelibre`
- **Cita Legacy**: [`legacy/server/Codigo/Modulo_InventANDobj.bas:45-51`](../../legacy/server/Codigo/Modulo_InventANDobj.bas#L45-L51) y [`legacy/server/Codigo/Modulo_UsUaRiOs.bas:1552-1585`](../../legacy/server/Codigo/Modulo_UsUaRiOs.bas#L1552-L1585).
- **Descripción**: La subrutina `Tilelibre` busca celdas adyacentes libres o combinables recorriendo anillos cuadrados de radio concéntrico incremental hasta un radio máximo de 15 tiles (`LoopC > 15`). Si todas las celdas dentro de dicha cuadrícula de $31 \times 31$ están bloqueadas, ocupadas por objetos incompatibles, saturadas en su capacidad máxima (`Amount + Obj.Amount > MAX_INVENTORY_OBJS`) o contienen traslados (`TileExit.map <> 0`), `Tilelibre` finaliza sin asignar coordenadas válidas, retornando `nPos = (0, 0)`. Al volver a `TirarItemAlPiso`:
  ```vb
  If NuevaPos.X <> 0 And NuevaPos.Y <> 0 Then
      Call MakeObj(Obj, Pos.Map, NuevaPos.X, NuevaPos.Y)
  End If
  TirarItemAlPiso = NuevaPos
  ```
  Al ser `(0, 0)`, `MakeObj` es omitido por completo. La función retorna silenciosamente `(0, 0)`. Tanto en drops de NPCs como en fragmentación de oro (`TirarOroNpc`), los bucles continúan descontando el saldo restante, provocando que los ítems o bolsas de oro que no encontraron tile libre se destruyan silenciosamente en el limbo sin notificar a nadie ni registrar error.
- **Camino de Producción**: **Quirk / Pérdida Silenciosa**.
- **Estado en C++**: **Replicated (Strict Parity)**. En la transliteración de `TirarItemAlPiso`, si el hook de búsqueda de celda libre devuelve `(0, 0)`, no se invoca `MakeObj` y se retorna `WorldPos{map, 0, 0}`, reproduciendo de forma idéntica la omisión del drop y la evaporación del excedente.
- **Documentación Detallada**: [`18-modulo-inventandobj.md`](18-modulo-inventandobj.md#3-replicación-del-bug-28-destrucción-silenciosa-de-drops-ante-retorno-nulo-de-tilelibre) y [`docs/audit/12a-modulo-inventandobj-detalle.md`](../audit/12a-modulo-inventandobj-detalle.md#51-algoritmo-de-dispersión-de-celdas-libres-tilelibre).

---

### Entrada #29 — `InvUsuario`: Exploit Histórico de Duplicación en `DropObj` por Desfase de Cantidades
- **Cita Legacy**: [`legacy/server/Codigo/InvUsuario.bas:368-380`](../../legacy/server/Codigo/InvUsuario.bas#L368-L380).
- **Descripción**: Al arrojar un objeto al suelo mediante `DropObj`, la estructura `Obj` retiene en `Obj.Amount` la cantidad total pretendida por el usuario (asignada en la línea 363: `Obj.Amount = num`). Si la celda receptora ya contiene unidades del mismo ítem y la adición superaría el límite de acumulación (`MAX_INVENTORY_OBJS = 10000`), el código recorta la variable escalar `num`:
  ```vb
  If num + MapData(.Pos.Map, X, Y).ObjInfo.Amount > MAX_INVENTORY_OBJS Then
      num = MAX_INVENTORY_OBJS - MapData(.Pos.Map, X, Y).ObjInfo.Amount
  End If
  
  Call MakeObj(Obj, Map, X, Y)
  Call QuitarUserInvItem(UserIndex, Slot, num)
  Call UpdateUserInv(False, UserIndex, Slot)
  ```
  Sin embargo, la invocación `MakeObj(Obj, Map, X, Y)` recibe la estructura `Obj` cuyo campo `Obj.Amount` **no fue recortado** (conserva el valor original). Por lo tanto:
  1. `MakeObj` suma la cantidad completa original a la celda del piso (desbordando incluso la cota máxima y acumulando por encima de 10.000).
  2. `QuitarUserInvItem` descuenta del inventario del jugador únicamente la cantidad recortada `num`.
  3. La diferencia entre la cantidad original y `num` se crea de la nada en el suelo sin ser retirada de la mochila del usuario, constituyendo un exploit severo de duplicación de ítems.
- **Camino de Producción**: **Activo / Exploit de Duplicación**.
- **Estado en C++**: **Replicated (Strict Parity)**. Conforme a las decisiones arquitectónicas del porting y la política de preservación histórica de [`docs/CONVENTIONS.md`](../CONVENTIONS.md), se replica 1:1 el comportamiento original: `Obj.Amount` retiene la cantidad original al invocar `MakeObj`, y `QuitarUserInvItem` descuenta la variable recortada. Queda prohibido aplicar parches o mitigaciones arbitrarias en el motor base.
- **Documentación Detallada**: [`19-invusuario.md`](19-invusuario.md#4-replicación-del-bug-29-exploit-de-duplicación-en-dropobj), [`19-invusuario-breakdown.md`](19-invusuario-breakdown.md#fase-1-g1--mutaciones-en-el-mundo-y-suelo) y [`docs/audit/12b-invusuario-detalle.md`](../audit/12b-invusuario-detalle.md#41-exploit-de-duplicación-en-dropobj-bug-29).

---

### Entrada #30 — `InvUsuario`: Pérdida Silenciosa de Saldo Excedente en `TirarOro (> 500k)`
- **Cita Legacy**: [`legacy/server/Codigo/InvUsuario.bas:228-268`](../../legacy/server/Codigo/InvUsuario.bas#L228-L268).
- **Descripción**: La subrutina `TirarOro` implementa una salvaguarda para evitar saturación de entidades arrojadas cuando la cantidad solicitada supera las 500.000 monedas:
  ```vb
  Dim Extra As Long
  Dim TeniaOro As Long
  TeniaOro = .Stats.GLD
  If Cantidad > 500000 Then 'Para evitar explotar demasiado
      Extra = Cantidad - 500000
      Cantidad = 500000
  End If
  ...
  If TeniaOro = .Stats.GLD Then Extra = 0
  If Extra > 0 Then
      .Stats.GLD = .Stats.GLD - Extra
  End If
  ```
  Si el jugador intenta tirar un monto superior a 500k (ej. 800k), el excedente `Extra = 300000` se resguarda en una variable local y `Cantidad` se trunca a 500k, arrojando hasta 50 pilas de 10k mediante `TirarItemAlPiso`. Al finalizar el bucle, si se arrojó al menos una pila, el saldo del jugador cambió (`TeniaOro <> .Stats.GLD`), por lo que la cláusula `If TeniaOro = .Stats.GLD Then Extra = 0` no se cumple. A continuación, el bloque `If Extra > 0 Then .Stats.GLD = .Stats.GLD - Extra` descuenta incondicionalmente las 300.000 monedas restantes de la billetera sin haberlas arrojado al suelo, evaporando el dinero en el limbo.
- **Camino de Producción**: **Quirk / Pérdida Silenciosa de Saldo**.
- **Estado en C++**: **Replicated (Strict Parity)**. Se preserva la deducción literal de `Extra` en la billetera del usuario tras concretar el arrojamiento de pilas, reproduciendo idénticamente la evaporación del excedente.
- **Documentación Detallada**: [`19-invusuario.md`](19-invusuario.md#5-replicación-del-bug-30-evaporación-de-saldo-en-tiraroro--500k), [`19-invusuario-breakdown.md`](19-invusuario-breakdown.md#fase-2-g2--gestión-base-de-inventario-y-descarte) y [`docs/audit/12b-invusuario-detalle.md`](../audit/12b-invusuario-detalle.md#42-pérdida-silenciosa-de-saldo-en-tiraroro-bug-30).

---

### Entrada #31 — `modBanco`: Acreditación Previa al Débito en `UserDejaObj` y `UserReciveObj`
- **Cita Legacy**: [`legacy/server/Codigo/modBanco.bas:168-172, 285-290`](../../legacy/server/Codigo/modBanco.bas#L168-L172).
- **Descripción**: En los flujos de depósito (`UserDejaObj`) y retiro (`UserReciveObj`) de la bóveda bancaria, la lógica histórica de VB6 invierte el orden natural de una transacción atómica:
  1. Al depositar (`UserDejaObj`), primero se asigna e incrementa el objeto en el slot de la bóveda:
     ```vb
     .BancoInvent.Object(Slot).ObjIndex = obji
     .BancoInvent.Object(Slot).Amount = .BancoInvent.Object(Slot).Amount + Cantidad
     Call QuitarUserInvItem(UserIndex, CByte(ObjIndex), Cantidad)
     ```
     Si `QuitarUserInvItem` aborta silenciosamente (por ejemplo, ante un índice fuera de rango `Slot < 1 Or Slot > CurrentInventorySlots` o fallo en `Desequipar`), el objeto ya quedó acreditado en el banco sin haber sido retirado de la mochila del jugador, produciendo una duplicación neta (*item dupe*).
  2. Al retirar (`UserReciveObj`), primero se incrementa el objeto en el inventario del usuario:
     ```vb
     .Invent.Object(Slot).ObjIndex = obji
     .Invent.Object(Slot).Amount = .Invent.Object(Slot).Amount + Cantidad
     Call QuitarBancoInvItem(UserIndex, CByte(ObjIndex), Cantidad)
     ```
     El ítem se materializa en la mochila del usuario antes de que se garantice o ejecute el débito en la bóveda bancaria.
- **Camino de Producción**: **Activo / Exploit de Duplicación**.
- **Estado en C++**: **Replicated (Strict Parity)**. Conforme a las decisiones arquitectónicas del porting y la directiva vinculante de preservación de asimetría histórica, queda prohibida cualquier alteración de la secuencia de operaciones o introducción de rollback automático transaccional. Se replica estrictamente el orden original de VB6 (acreditación en destino previa al débito en origen).
- **Documentación Detallada**: [`20-modbanco.md`](20-modbanco.md#2-replicación-del-bug-31-acreditación-previa-al-débito-item-dupe), [`20-modbanco-breakdown.md`](20-modbanco-breakdown.md#fase-2-g2--retiro-y-depósito-de-objetos) y [`docs/audit/12c-modbanco-detalle.md`](../audit/12c-modbanco-detalle.md#51-riesgos-de-desincronización-y-duplicación-de-ítems-item-dupe).

---

### Entrada #32 — `modBanco`: Ausencia de Restricciones de Almacenamiento para Barcos, Faccionarios y Novatos
- **Cita Legacy**: [`legacy/server/Codigo/modBanco.bas:216-245, 247-299`](../../legacy/server/Codigo/modBanco.bas#L216-L245).
- **Descripción**: La subrutina `UserDepositaItem` y su ejecutora interna `UserDejaObj` no realizan comprobación alguna sobre los atributos o tipo del objeto (`ObjDataList(obji)`):
  1. Permite depositar embarcaciones y navíos (`OBJTYPE_BARCO`), almacenando barcas y galeones como ítems estáticos en la bóveda.
  2. Permite depositar armaduras y objetos faccionarios (pertenecientes a la Armada Real o a las Fuerzas del Caos), permitiendo transferirlos fuera del uso inmediato.
  3. Permite depositar objetos de novato con flag `Newbie = 1`. Esta omisión habilitaba el exploit histórico mediante el cual los personajes recién creados resguardaban sus pertenencias de novato en la bóveda bancaria antes de alcanzar el nivel 13 o abandonar Newbie Dungeon, eludiendo la rutina de purga `QuitarNewbieObj` (`InvUsuario.bas:93`).
- **Camino de Producción**: **Quirk / Exploit Histórico**.
- **Estado en C++**: **Replicated (Strict Parity)**. Se preserva la ausencia total de validaciones restrictivas de tipo de ítem en `UserDepositaItem` y `UserDejaObj`, permitiendo el depósito de cualquier objeto presente en el inventario del usuario.
- **Documentación Detallada**: [`20-modbanco.md`](20-modbanco.md#3-replicación-del-bug-32-ausencia-de-restricciones-de-almacenamiento), [`20-modbanco-breakdown.md`](20-modbanco-breakdown.md#fase-2-g2--retiro-y-depósito-de-objetos) y [`docs/audit/12c-modbanco-detalle.md`](../audit/12c-modbanco-detalle.md#55-restricciones-de-ítems-faccionarios-barcos-newbies).

---

### Entrada #33 — `Comercio`: Asimetría de Capacidad de Slots en Mercaderes NPC
- **Cita Legacy**: [`legacy/server/Codigo/Comercio.bas:203-232, 257`](../../legacy/server/Codigo/Comercio.bas#L203) y [`legacy/server/Codigo/Declares.bas:492, 496`](../../legacy/server/Codigo/Declares.bas#L492).
- **Descripción**: El inventario de los NPCs comerciantes se dimensiona con `MAX_INVENTORY_SLOTS = 30`. Cuando un usuario vende un ítem al NPC, la rutina `SlotEnNPCInv` busca y almacena el objeto hasta el límite de 30 ranuras (`NpcSlot <= MAX_INVENTORY_SLOTS`). Sin embargo, la rutina de sincronización con el cliente `EnviarNpcInv` itera únicamente hasta `MAX_NORMAL_INVENTORY_SLOTS = 20` (`For Slot = 1 To MAX_NORMAL_INVENTORY_SLOTS`). En consecuencia, cualquier mercancía vendida por los jugadores que caiga en los slots 21 a 30 permanece en memoria del servidor pero jamás se transmite al cliente, convirtiéndose en ítems invisibles e incomprables mediante la interfaz gráfica oficial.
- **Camino de Producción**: **Quirk / Asimetría de Sincronización**.
- **Estado en C++**: **Replicated (Strict Parity)**. En cumplimiento de las directivas vinculantes de paridad histórica, queda prohibido unificar o ampliar arbitrariamente el límite de despacho. Se replica fielmente la asimetría: el contenedor del NPC almacena hasta 30 ranuras, pero la sincronización de inventario despacha estrictamente los primeros 20 slots (`MAX_NORMAL_INVENTORY_SLOTS`).
- **Documentación Detallada**: [`21-comercio.md`](21-comercio.md#1-replicación-del-bug-33-asimetría-de-sincronización-en-mercaderes-npc), [`21-comercio-breakdown.md`](21-comercio-breakdown.md#fase-1-g1--comercio-con-mercaderes-npc-comerciobas) y [`docs/audit/12d-comercio-detalle.md`](../audit/12d-comercio-detalle.md#34-la-gran-asimetría-de-ranuras-max_normal_inventory_slots-vs-max_inventory_slots).

---

### Entrada #34 — `mdlCOmercioConUsuario`: Desborde a Suelo y Evaporación Silenciosa de Ítems en Comercio Seguro
- **Cita Legacy**: [`legacy/server/Codigo/mdlCOmercioConUsuario.bas:178-182, 221-225`](../../legacy/server/Codigo/mdlCOmercioConUsuario.bas#L178) y [`legacy/server/Codigo/Modulo_InventANDobj.bas:56-59`](../../legacy/server/Codigo/Modulo_InventANDobj.bas#L56).
- **Descripción**: Al consumarse el intercambio seguro en `AceptarComercioUsu`, si el inventario de la parte receptora se encuentra colmado (`MeterItemEnInventario` retorna `False`), el servidor no aborta ni revierte la transacción. En su lugar, intenta arrojar el ítem al suelo debajo de la posición del receptor mediante `TirarItemAlPiso`. Si la casilla del receptor y sus baldosas circundantes están saturadas o bloqueadas (`TileLibre` retorna `(0, 0)`), la rutina `MakeObj` no se ejecuta. Sin embargo, la instrucción siguiente en `AceptarComercioUsu` ejecuta incondicionalmente `QuitarObjetos` sobre el usuario emisor. El objeto es sustraído de la mochila del originador pero jamás se entrega ni se crea físicamente en el mapa, evaporándose de forma silenciosa e irrecuperable.
- **Camino de Producción**: **Quirk / Pérdida Silenciosa de Ítems**.
- **Estado en C++**: **Replicated (Strict Parity)**. Conforme a la regla de preservación de defectos legacy, queda terminantemente prohibido introducir un rollback automático o bloquear el intercambio previo ante falta de slots. Se replica con estricta paridad el intento de descarte a piso y la posterior invocación de sustracción que consuma la evaporación del ítem en baldosas colmadas.
- **Documentación Detallada**: [`21-comercio.md`](21-comercio.md#2-replicación-del-bug-34-desborde-a-suelo-y-evaporación-silenciosa-de-ítems-en-comercio-p2p), [`21-comercio-breakdown.md`](21-comercio-breakdown.md#fase-3-g3--ejecución-bilateral-del-intercambio-p2p) y [`docs/audit/12d-comercio-detalle.md`](../audit/12d-comercio-detalle.md#53-capacidad-de-inventario-y-evaporación-de-ítems).

---

### Entrada #35 — `mdlCOmercioConUsuario`: Acreditación Previa al Débito y Duplicación de Ítems en Comercio P2P
- **Cita Legacy**: [`legacy/server/Codigo/mdlCOmercioConUsuario.bas:178-182, 221-225`](../../legacy/server/Codigo/mdlCOmercioConUsuario.bas#L178) y [`legacy/server/Codigo/Trabajo.bas:333`](../../legacy/server/Codigo/Trabajo.bas#L333).
- **Descripción**: En `AceptarComercioUsu`, la entrega de objetos de cada slot de oferta se procesa invirtiendo la atomicidad transaccional: primero se acredita el ítem en el receptor (`MeterItemEnInventario`) y recién después se descuenta del emisor (`QuitarObjetos`). Además, el procedimiento auxiliar `QuitarObjetos` fue declarado históricamente con su parámetro de cantidad como `cant As Integer` (entero de 16 bits con signo, máximo 32.767), mientras que la estructura de comercio `ComUsu.cant()` maneja enteros de 32 bits (`Long`). Al comerciar más de 32.767 unidades de ítems apilables (flechas, leña, pociones), el casteo a `Integer` detona un desbordamiento aritmético en VB6. La acreditación en el receptor ya fue consumada, pero el débito en el emisor falla o se trunca, provocando la duplicación neta de ítems apilables (*item dupe*).
- **Camino de Producción**: **Activo / Exploit de Duplicación**.
- **Estado en C++**: **Replicated (Strict Parity)**. Se preserva el orden estricto de acreditación en receptor previa al débito en emisor. Para evitar comportamiento indefinido (UB) por desbordamiento con signo en C++, la aritmética se promueve a `std::int64_t`, manteniendo la semántica y los puntos de contacto originales.
- **Documentación Detallada**: [`21-comercio.md`](21-comercio.md#3-replicación-del-bug-35-acreditación-previa-al-débito-y-falla-de-atomicidad-en-comercio-p2p), [`21-comercio-breakdown.md`](21-comercio-breakdown.md#fase-3-g3--ejecución-bilateral-del-intercambio-p2p) y [`docs/audit/12d-comercio-detalle.md`](../audit/12d-comercio-detalle.md#54-orden-de-transferencia-y-falla-de-atomicidad-riesgo-de-duplicación).

---

### Entrada #36 — `mdlCOmercioConUsuario`: Omisión de Validación de `MAXORO` en Comercio Seguro
- **Cita Legacy**: [`legacy/server/Codigo/mdlCOmercioConUsuario.bas:168, 210`](../../legacy/server/Codigo/mdlCOmercioConUsuario.bas#L168).
- **Descripción**: En la transferencia bilateral de oro (`OfferSlot = GOLD_OFFER_SLOT`), el servidor acredita directamente el monto acordado mediante adición aritmética pura: `UserList(OtroUserIndex).Stats.GLD = UserList(OtroUserIndex).Stats.GLD + .ComUsu.GoldAmount`. A diferencia de la venta a mercaderes NPC (`Comercio.bas:157`), no se evalúa el tope global `MAXORO` (90.000.000). Un jugador receptor puede superar libremente los 90 millones de monedas de oro en su billetera a través de intercambios P2P.
- **Camino de Producción**: **Quirk / Riesgo Aritmético**.
- **Estado en C++**: **Replicated (Strict Parity)**. Se replica fielmente la ausencia de clampleo a `MAXORO` en la acreditación de oro entre usuarios. La adición se computa utilizando enteros de 64 bits para prevenir desbordamiento con signo (UB en C++), asignando el resultado directo al saldo del personaje conforme al comportamiento de VB6.
- **Documentación Detallada**: [`21-comercio.md`](21-comercio.md#4-replicación-del-bug-36-omisión-de-validación-de-maxoro-en-comercio-p2p), [`21-comercio-breakdown.md`](21-comercio-breakdown.md#fase-3-g3--ejecución-bilateral-del-intercambio-p2p) y [`docs/audit/12d-comercio-detalle.md`](../audit/12d-comercio-detalle.md#52-desbordamiento-de-oro-statsgld).

---

### Entrada #37 — `SistemaCombate`: Omisión de Daño de Flechas en el Bono de Fuerza (`DañoMaxArma`)
- **Cita Legacy**: [`legacy/server/Codigo/SistemaCombate.bas:271-274, 292-295`](../../legacy/server/Codigo/SistemaCombate.bas#L271).
- **Descripción**: En el cálculo de daño a distancia con proyectiles (`CalcularDaño`), cuando el personaje utiliza arco y flechas, el motor suma el daño mínimo y máximo de la munición al intervalo aleatorio de daño base (`DañoArma = RandomNumber(ObjData(Arma).MinHIT + ObjData(Municion).MinHIT, ObjData(Arma).MaxHIT + ObjData(Municion).MaxHIT)`). Sin embargo, la variable auxiliar `DañoMaxArma` —utilizada exclusivamente para computar el escalado por fuerza `((DañoMaxArma / 5) * Max(0, Fuerza - 15))`— solo toma el valor `ObjData(Arma).MaxHIT` del arco, omitiendo por completo sumar el daño máximo de la flecha (`ObjData(Municion).MaxHIT`). En consecuencia, el bono de daño otorgado por la fuerza del arquero escala únicamente según la calidad del arco e ignora la munición cargada.
- **Camino de Producción**: **Quirk Histórico / Asimetría Aritmética**.
- **Estado en C++**: **Replicated (Strict Parity)**. Se preserva la fórmula exacta de VB6: la munición suma a `DañoArma` pero no se incorpora en `DañoMaxArma`.
- **Documentación Detallada**: [`22-sistemacombate.md`](22-sistemacombate.md#1-replicación-del-bug-37-omisión-de-proyectilmaxhit-en-dañomaxarma), [`22-sistemacombate-breakdown.md`](22-sistemacombate-breakdown.md#fase-2-g2--daño-bruto-y-acierto-rng) y [`docs/audit/04a-sistemacombate-detalle.md`](../audit/04a-sistemacombate-detalle.md#61-descarte-del-maxhit-de-munición-en-bono-por-fuerza-bug-37).

---

### Entrada #38 — `modHechizos`: Omisión de Asignación de `daño` en Modificadores de Maná y Estamina (`HechizoPropUsuario`)
- **Cita Legacy**: `legacy/server/Codigo/modHechizos.bas:1775-1855`.
- **Descripción**: En los bloques correspondientes a `SubeMana = 1`, `SubeMana = 2`, `SubeSta = 1` y `SubeSta = 2`, la rutina utiliza la variable local `daño` para alterar los puntos de maná (`.Stats.MinMAN`) o estamina (`.Stats.MinSta`). Sin embargo, el código omite completamente computar `daño = RandomNumber(Hechizos(SpellIndex).MiMana, Hechizos(SpellIndex).MaMana)` o `daño = RandomNumber(Hechizos(SpellIndex).MinSta, Hechizos(SpellIndex).MaxSta)`. En consecuencia, si un hechizo solo modifica maná o estamina, `daño` permanece en 0 (siendo inocuo), o bien hereda el valor residual de un bloque previo ejecutado en la misma subrutina (como daño de HP, Fuerza o Hambre), provocando alteraciones numéricas no correspondientes a la definición del conjuro.
- **Camino de Producción**: **Quirk / Bug Silencioso**. En la base oficial `Hechizos.dat`, los 46 conjuros poseen `SubeMana = 0` y `SubeSta = 0`, por lo que el camino estuvo inerte en producción estándar pero activo en cualquier conjuro personalizado.
- **Estado en C++**: **Replicated (Strict Parity)**. Replicado fidedignamente en `src/server/modHechizos.cpp` (`HechizoPropUsuario`), donde `SubeMana` y `SubeSta` aplican la variable `daño` sin invocar `RandomNumber`, aplicando 0 si no hubo mutación cuantitativa previa o arrastrando el remanente de HP/atributos. Verificado y cubierto en `tests/test_modhechizos.cpp`.
- **Documentación Detallada**: [`23-modhechizos.md`](23-modhechizos.md#1-replicación-del-bug-38-omisión-de-daño-en-maná-y-estamina), [`23-modhechizos-breakdown.md`](23-modhechizos-breakdown.md#fase-2-g2--efectos-cuantitativos-y-modificación-de-atributos) y [`docs/audit/14a-hechizos-detalle.md`](../audit/14a-hechizos-detalle.md#61-defectos-técnicos-reales-candidatos-a-known-legacy-bugsmd).

---

### Entrada #39 — `modHechizos`: Fuga de Flujo de Ejecución y Asimetría de Coste en Muerte por Resurrección (`HechizoEstadoUsuario`)
- **Cita Legacy**: `legacy/server/Codigo/modHechizos.bas:1104-1120`.
- **Descripción**: Al resucitar a un personaje, el lanzador sufre un descuento de vida proporcional al nivel del objetivo (`.Stats.MinHp = .Stats.MinHp * (1 - Target.ELV * 0.015)`). Si la salud cae a `<= 0`, se invoca la muerte del lanzador `Call UserDie(UserIndex)` y se marca `HechizoCasteado = False`. No obstante, el procedimiento omite la sentencia `Exit Sub`, continuando la ejecución hasta la invocación incondicional de `Call RevivirUsuario(TargetIndex)` (el objetivo resucita a costa de la vida del lanzador). Luego, al retornar `HechizoCasteado = False` al despachador `HandleHechizoUsuario`, el motor no descuenta el maná ni la estamina ni otorga skill al lanzador ya muerto.
- **Camino de Producción**: **Activo / Asimetría Transaccional**.
- **Estado en C++**: **Replicated (Strict Parity)**. Replicado en `src/server/modHechizos.cpp` (`HechizoEstadoUsuario`): ante la muerte del lanzador al resucitar (`MinHp <= 0`), se invoca `UserDie(user_index)` y `hechizo_casteado = false` sin retornar prematuramente de la función, completando la resurrección de la víctima (`RevivirUsuario(target_index)`) y dispensando al lanzador fallecido del débito de maná/estamina en `HandleHechizoUsuario`. Verificado en `tests/test_modhechizos.cpp`.
- **Documentación Detallada**: [`23-modhechizos.md`](23-modhechizos.md#2-replicación-del-bug-39-fuga-de-flujo-en-muerte-por-resurrección), [`23-modhechizos-breakdown.md`](23-modhechizos-breakdown.md#fase-3-g3--estados-alterados-metamorfosis-e-invocaciones) y [`docs/audit/14a-hechizos-detalle.md`](../audit/14a-hechizos-detalle.md#61-defectos-técnicos-reales-candidatos-a-known-legacy-bugsmd).

---

### Entrada #40 — `modHechizos`: Colisión de Contadores Temporales entre Ceguera y Estupidez (`HechizoEstadoUsuario`)
- **Cita Legacy**: `legacy/server/Codigo/modHechizos.bas:1139, 1159`.
- **Descripción**: Tanto el estado alterado de Ceguera (`Ceguera = 1`) como el de Estupidez (`Estupidez = 1`) modifican y reinician la misma variable temporal del usuario: `.Counters.Ceguera` (`IntervaloParalizado / 3` para ceguera y `IntervaloParalizado` para estupidez). La aplicación simultánea de ambos estados produce que el último conjuro recibido sobreescriba y altere arbitrariamente la expiración del estado previo.
- **Camino de Producción**: **Activo / Colisión de Estados**.
- **Estado en C++**: **Replicated (Strict Parity)**. Replicado en `src/server/modHechizos.cpp` (`HechizoEstadoUsuario`): tanto el estado Ceguera (`IntervaloParalizado / 3`) como Estupidez (`IntervaloParalizado`) manipulan y sobrescriben el mismo temporizador `target.Counters.Ceguera`. Verificado en `tests/test_modhechizos.cpp`.
- **Documentación Detallada**: [`23-modhechizos.md`](23-modhechizos.md#3-replicación-del-bug-40-colisión-de-temporizadores-cegueraestupidez), [`23-modhechizos-breakdown.md`](23-modhechizos-breakdown.md#fase-3-g3--estados-alterados-metamorfosis-e-invocaciones) y [`docs/audit/14a-hechizos-detalle.md`](../audit/14a-hechizos-detalle.md#61-defectos-técnicos-reales-candidatos-a-known-legacy-bugsmd).
---

### Entrada #41 — `modInvisibles`: Módulo Huérfano y Rutina `PonerInvisible` sin Invocaciones
- **Cita Legacy**: `legacy/server/Codigo/modInvisibles.bas:1-41`, `legacy/server/SERVER.VBP:45`.
- **Descripción**: El módulo `modInvisibles.bas` constituye un borrador trunco en VB6. La rutina `PonerInvisible` no posee call-sites en todo el proyecto legacy y la directiva `#If MODO_INVISIBILIDAD` evalúa la constante inerte `MODO_INVISIBILIDAD = 0`, mientras que la rama `#Else` contiene código sintácticamente inválido que falla al compilar bajo `Option Explicit` debido a la variable no declarada `Modo`.
- **Camino de Producción**: **Muerto / Huérfano**. En el flujo real de producción, la visibilidad de los personajes se gestiona mediante la rutina `SetInvisible` en `Modulo_UsUaRiOs.bas` y sus contadores en `modNuevoTimer.bas`.
- **Estado en C++**: **Excluded (dead code, not ported)**. Módulo excluido del port a C++ siguiendo el precedente de `clsAntiMassClon` y `cColaArray`.
- **Documentación Detallada**: [`docs/audit/14b-invisibles-detalle.md`](../audit/14b-invisibles-detalle.md).

---

### Entrada #42 — `Trabajo`: Reducción del Daño al 75% en Golpe Crítico (`DoGolpeCritico`)
- **Cita Legacy**: `legacy/server/Codigo/Trabajo.bas:1891`.
- **Descripción**: En `DoGolpeCritico`, al resultar exitosa la probabilidad de asestar un golpe crítico, la rutina realiza `daño = static_cast<std::int16_t>(daño * 0.75)`, reduciendo el impacto en un 25% en lugar de bonificarlo o multiplicarlo positivamente.
- **Camino de Producción**: **Quirk / Asimetría**. Invocado durante la resolución de ataques con armas que posibilitan golpe crítico.
- **Estado en C++**: **Replicated (Strict Parity)**. Replicado idénticamente en `src/server/Trabajo.cpp` (`DoGolpeCritico`), aplicando el factor `0.75` sobre el daño recibido. Verificado y cubierto en `tests/test_trabajo.cpp`.
- **Documentación Detallada**: [`26-trabajo.md`](26-trabajo.md#31-replicación-del-bug-42-en-dogolpecritico), [`26-trabajo-breakdown.md`](26-trabajo-breakdown.md#fase-4-g4--combate-sigiloso-hurto-y-domación) y [`docs/audit/12e-trabajo-detalle.md`](../audit/12e-trabajo-detalle.md#41-quirk-prominente-en-dogolpecritico-reducción-de-daño).

---

### Entrada #43 — `PathFinding`: Variable `steps` Inoperante sin Incrementar en el Bucle Principal BFS (`SeekPath`)
- **Cita Legacy**: [`legacy/server/Codigo/PathFinding.bas:217-237`](../../legacy/server/Codigo/PathFinding.bas#L217-L237).
- **Descripción**: La variable local `steps` se inicializa en `0` antes del bucle BFS y dentro del bucle `Do While (Not IsEmpty)` la condición `If steps > MaxSteps Then Exit Do` evalúa dicha variable. Sin embargo, no existe ninguna instrucción `steps = steps + 1` dentro del loop. Por ende, `steps` permanece estancada en `0` y la cota `MaxSteps` nunca logra interrumpir el recorrido BFS por iteraciones. El corte ocurre únicamente si la cola se vacía (`IsEmpty`), si se alcanza la meta `tar_npc_pos` o por desbordamiento de la cola de 1000 elementos (`MAXELEM`).
- **Camino de Producción**: **Activo / Bug de Bucle**.
- **Estado en C++**: **Replicated (Strict Parity)**. Replicado en [`src/server/PathFinding.cpp:177`](../../src/server/PathFinding.cpp#L177) dentro de `SeekPath`, manteniendo `steps` constante en `0`. Probado en [`tests/test_pathfinding.cpp:281`](../../tests/test_pathfinding.cpp#L281).
- **Documentación Detallada**: [`27-pathfinding.md`](27-pathfinding.md#3-bugs-históricos-replicados-11), [`27-pathfinding-breakdown.md`](27-pathfinding-breakdown.md) y [`docs/audit/08a-pathfinding-detalle.md`](../audit/08a-pathfinding-detalle.md#2-variable-steps-sin-incrementar-bug-en-bucle-principal).

---

### Entrada #44 — `PathFinding`: Limpieza Incompleta de Matriz Global `TmpArray` (`InitializeTable`)
- **Cita Legacy**: [`legacy/server/Codigo/PathFinding.bas:212-220`](../../legacy/server/Codigo/PathFinding.bas#L212-L220).
- **Descripción**: La rutina `InitializeTable` restablece los campos de `TmpArray` a sus valores por defecto (`Known = False`, `DistV = MAXINT`, `PrevV = (0,0)`) limitando la iteración exclusivamente al sub-cuadrante de `[S.Y - MaxSteps, S.Y + MaxSteps]` y `[S.X - MaxSteps, S.X + MaxSteps]`. Si en una búsqueda BFS previa el algoritmo visitó nodos fuera de esa ventana o si se ejecuta una consulta con otra coordenada inicial, la matriz conserva distancias y punteros `PrevV` obsoletos de llamadas anteriores, pudiendo provocar corrupciones de camino o bucles en la reconstrucción.
- **Camino de Producción**: **Activo / Side-Effect de Estado Global**.
- **Estado en C++**: **Replicated (Strict Parity)**. Replicado en [`src/server/PathFinding.cpp:52`](../../src/server/PathFinding.cpp#L52) dentro de `InitializeTable`. Probado en [`tests/test_pathfinding.cpp:115`](../../tests/test_pathfinding.cpp#L115).
- **Documentación Detallada**: [`27-pathfinding.md`](27-pathfinding.md#3-bugs-históricos-replicados-11), [`27-pathfinding-breakdown.md`](27-pathfinding-breakdown.md) y [`docs/audit/08a-pathfinding-detalle.md`](../audit/08a-pathfinding-detalle.md#3-limpieza-incompleta-de-tmparray-efecto-colateral-de-sub-grilla).

---

### Entrada #45 — `PathFinding` / `AI_NPC`: Asimetría e Inversión Histórica de Coordenadas $X \leftrightarrow Y$
- **Cita Legacy**: [`legacy/server/Codigo/PathFinding.bas:1-19, 221-222`](../../legacy/server/Codigo/PathFinding.bas#L1-L19) y [`legacy/server/Codigo/AI_NPC.bas:986-987, 1033-1034`](../../legacy/server/Codigo/AI_NPC.bas#L986-L987).
- **Descripción**: Debido a la convención histórica del autor frente a ORE/AO (`MapData(Map, X, Y)` vs `TmpArray(Y, X)`), `PathFinding.bas` indexa internamente la primera dimensión como `Y` (fila) y la segunda como `X` (columna). Tanto en `SeekPath` como en los puntos de llamada en `AI_NPC.bas`, se intercambian manualmente `X` e `Y` al setear `Target` (`Target.X = Pos.Y`, `Target.Y = Pos.X`), al inicializar la búsqueda (`cur_npc_pos.X = Pos.Y`, `cur_npc_pos.Y = Pos.X`) y al leer el arreglo devuelto (`tmpPos.X = Path(i).Y`, `tmpPos.Y = Path(i).X`).
- **Camino de Producción**: **Activo / Quirk de Arquitectura**.
- **Estado en C++**: **Replicated (Strict Parity)**. Replicado en `src/server/PathFinding.cpp:144, 169` (`SeekPath` / `MakePath`) y en los puntos de consumo de `src/server/AI_NPC.cpp` (`PathFindingAI` L223-224 y `FollowPath` L195-196). Verificado en `tests/test_pathfinding.cpp:202` y `tests/test_ai_npc.cpp:115-135`.
- **Documentación Detallada**: [`27-pathfinding.md`](27-pathfinding.md#3-bugs-históricos-replicados-11), [`29-ai-npc.md`](29-ai-npc.md#4-quirks-históricos-y-reglas-de-dominio-replicadas), [`29-ai-npc-breakdown.md`](29-ai-npc-breakdown.md) y [`docs/audit/08a-pathfinding-detalle.md`](../audit/08a-pathfinding-detalle.md#1-inversión-histórica-de-coordenadas-x---y).

---

### Entrada #46 — `clsParty` / `mdParty`: Sustracción Errónea del Nivel del Líder en Lugar de Cada Integrante Durante la Disolución (`clsParty.cls:248`)
- **Cita Legacy**: `legacy/server/Codigo/clsParty.cls:248`.
- **Descripción**: Dentro del bucle `For j = PARTY_MAXMEMBERS To 1 Step -1` en `SaleMiembro` cuando se disuelve la party por salida del líder, la rutina descuenta de la suma ponderada de niveles la potencia del nivel del líder (`UserList(UserIndex).Stats.ELV ^ ExponenteNivelParty`) en cada iteración del bucle, en lugar de restar la potencia correspondiente al nivel de cada integrante iterado (`UserList(p_members(j).UserIndex).Stats.ELV`).
- **Camino de Producción**: **Activo / Bug de Disolución**.
- **Estado en C++**: **Replicated (Strict Parity)**. Preservado para paridad comportamental estricta.
- **Documentación Detallada**: [`docs/audit/11b-party-detalle.md`](../audit/11b-party-detalle.md#61-bug-1-inconsistencia-de-parámetros-en-salemiembro-durante-disolución), [`31-party.md`](31-party.md), [`src/server/clsParty.cpp`](../../src/server/clsParty.cpp) y test `G3_SaleMiembro_Lider_Disolucion` en [`tests/test_party.cpp`](../../tests/test_party.cpp).




