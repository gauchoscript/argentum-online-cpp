---
area: infraestructura-de-red
status: completed
module: TCP
layer: 4
legacy_source:
  - legacy/server/Codigo/TCP.bas
  - legacy/server/Codigo/wskapiAO.bas
  - legacy/server/Codigo/wsksock.bas
target_header: src/server/TCP.hpp
target_source: src/server/TCP.cpp
test_suite: tests/test_tcp.cpp
last_updated: 2026-09-11
---

# Módulo #14: TCP — Subsistema de Red y Conexiones Multijugador (Standalone Asio Monohilo)

## Resumen del Módulo

Este módulo implementa el subsistema central de comunicaciones de red de Argentum Online, condensando y unificando la totalidad de la lógica distribuida en tres módulos del servidor legacy de Visual Basic 6:
- `legacy/server/Codigo/TCP.bas` (1.865 líneas): Gestión de sesiones de usuario, ciclo de vida de conexión/desconexión y callbacks de entrada/salida.
- `legacy/server/Codigo/wskapiAO.bas` (592 líneas): Procedimiento de ventana `WndProc`, eventos asíncronos de sockets WinSock (`FD_READ`, `FD_WRITE`, `FD_ACCEPT`, `FD_CLOSE`), mapeo `WSAPISock2Usr` y buffers de transmisión.
- `legacy/server/Codigo/wsksock.bas` (819 líneas): Funciones auxiliares de conversión de direcciones IP (`inet_ntoa`, `inet_addr`) y apertura del socket de escucha (`ListenForConnect`).

En total, aproximadamente **3.276 líneas legacy** fueron analizadas y portadas a C++17 moderno en [`src/server/TCP.hpp`](../../src/server/TCP.hpp) y [`src/server/TCP.cpp`](../../src/server/TCP.cpp), integrando **standalone Asio** (header-only, sin dependencia de Boost) bajo un estricto modelo de ejecución **monohilo** coordinado con el Game Loop mediante `TCP::PollRed()`.

---

## Alcance Implementado por Grupo Funcional

La implementación se organizó y ejecutó siguiendo las 7 fases delimitadas en [`docs/implementation/14-tcp-breakdown.md`](14-tcp-breakdown.md):

1. **G1 — Utilidades de IP y Auxiliares de Red**:
   - `TCP::IsValidIP`: Validación sintáctica exhaustiva de cadenas IPv4 en formato decimal punteado (`"a.b.c.d"`).
   - `TCP::GetAscIP`: Conversión bidireccional desde entero de 32 bits en network byte order a texto punteado, con manejo de la dirección de broadcast `255.255.255.255` (`wsksock.bas:527-545`).
   - `TCP::GetLongIp`: Conversión de string a entero de red de 32 bits (`wsksock.bas:801-803`).

2. **G2 — Gestión de Slots de Usuario y Mapeo de Sockets**:
   - `TCP::NextOpenUser`: Búsqueda secuencial del primer slot disponible (1 a `MaxUsers`) evaluando `UserList[i].ConnID == -1 && !UserList[i].flags.UserLogged` (`Modulo_UsUaRiOs.bas:868-883`).
   - `TCP::ResetUserSlot`: Blanqueo e invalidación en memoria de los contadores y descriptores de red del slot (`TCP.bas:1719-1756`).
   - Mapeo asociativo `sock_id -> slot`: Transliteración con tabla hash `std::unordered_map` (`s_sock_to_slot`) reemplazando a la colección `WSAPISock2Usr` de `wskapiAO.bas` mediante `BuscaSlotSock`, `AgregaSlotSock`, `BorraSlotSock` y `ResetSockSlots`.

3. **G3 — Listener Asio y Aceptación de Conexiones**:
   - `TCP::IniciaServidor(puerto, bind_ip)`: Apertura, configuración (`SO_REUSEADDR`), enlace y escucha asíncrona mediante `asio::ip::tcp::acceptor` (`wsksock.bas:834-896`). Soporta puerto efímero (`0`) para pruebas unitarias y enlace a interfaces específicas.
   - `TCP::DetenerServidor`: Cierre ordenado de descriptores, cancelación de operaciones y reseteo del contexto de red.
   - `TCP::PuertoEscucha`: Inspección del endpoint local efectivo.
   - `TCP::PollRed`: Despacho no bloqueante de eventos de red mediante `io_context::poll()`.
   - Filtro Anti-Flood síncrono: Invocación en el acto de `SecurityIp::IpSecurityAceptarNuevaConexion` al aceptar el socket (`wskapiAO.bas:402`), mitigando el Bug #20.

4. **G4 — Recepción Asíncrona de Datos**:
   - `TCP::IniciarLectura`: Bucle continuo de lectura asíncrona mediante `async_read_some` con buffer de recepción de 8 KB (`SIZE_RCVBUF = 8192`, `wskapiAO.bas:255-275`).
   - Volcado FIFO a `incomingData`: Cada fragmento recibido se escribe directamente en la cola `clsByteQueue` del slot (`wskapiAO.bas:507`).
   - Callback `PacketHandler`: Interfaz desacoplada para notificar la llegada de tramas a la capa de decodificación (`Protocol.bas`).

5. **G5 — Envío de Datos, Buffers Salientes y Mitigación de Backpressure**:
   - `TCP::EnviarDatosASlot`: Despacho asíncrono con cola por slot (`OutgoingQueue`) y mitigación del Bug #19.
   - `TCP::FlushBuffer`: Extracción del contenido de `outgoingData` de `UserList` y transmisión al socket (`Protocol.bas:17233-17249`).
   - Cota de saturación de 64 KB (`MAX_OUTGOING_BUFFER_SIZE = 65536`): Desconexión preventiva ante backpressure para evitar busy-loops y colapso de memoria.

6. **G6 — Ciclo de Vida de Desconexión y Mecánica Anti-CombatLog**:
   - `TCP::CloseSocketSL`: Cierre inmediato del socket físico TCP en Asio y desvinculación del mapeo, pero reteniendo la entidad lógica (`ConnIDValida = false`, preservando `flags.UserLogged = true`).
   - `TCP::CerrarUsuario`: Inicio del temporizador de logout; asigna 0 segundos en mapas seguros y 10 segundos en zonas PK (`MapInfoList[map].Pk == true`). Cancelación inmediata e incondicional de invisibilidad y ocultamiento (`flags.invisible = 0`, `flags.Oculto = 0`).
   - `TCP::CloseSocket`: Desconexión final, ajuste de `LastUser`, vaciado de buffers, decremento de `NumUsers`, invocación de `CloseUserHandler` y blanqueo con `ResetUserSlot`.
   - `TCP::PasarSegundoUsuarios`: Decremento del temporizador cada segundo; al llegar a cero, ejecuta `CloseSocket(i)`.

7. **G7 — Diagnóstico y Cierre de Módulo**:
   - `TCP::WSApiReiniciarSockets`: Reseteo global de conexiones, vaciado de `UserList`, reinicio del acceptor de Asio y reenganche en el mismo puerto (`wskapiAO.bas:542-582`).
   - Resolución de `SecurityIp::DumpTables`: Volcado formateado de entradas activas de IP utilizando `TCP::GetAscIP` (`SecurityIp.bas:314-327`).

---

## Decisiones de Diseño

### 1. Modelo de Concurrencia: Standalone Asio Monohilo (`io_context.poll()`)
- **Contexto**: El servidor de Visual Basic 6 operaba bajo el modelo *Single-Threaded Apartment* (STA) de Windows, donde tanto el procesamiento de mensajes de red (`WndProc`) como el Game Loop (`modNuevoTimer`) y la actualización de entidades corrían en un único hilo.
- **Alternativas Evaluadas**:
  - *Modelo Multihilo (Worker Threads)*: Correr `io_context.run()` en hilos secundarios desacoplados del Game Loop.
  - *Modelo Monohilo Reactivo (`io_context.poll()`)*: Ejecutar todas las operaciones de red en el mismo hilo del Game Loop.
- **Decisión Adoptada**: Se adoptó **standalone Asio monohilo**. Al inicio de cada frame del Game Loop se invoca `TCP::PollRed()`, procesando síncronamente todos los handlers de E/S listos.
- **Justificación Técnica**:
  1. *Fidelidad al Estado Global*: `UserList`, `SecurityIp`, `Declares`, `NumUsers` y `LastUser` son estructuras globales mutables heredadas de VB6. Un modelo multihilo requeriría una proliferación masiva de mutexes o sincronización atómica en el *hot path*, introduciendo sobrecostos de contención y riesgo latente de *deadlocks*.
  2. *Determinismo en Pruebas Unitarias*: Permite que las pruebas unitarias con doctest sean 100% reproducibles y predecibles, ejecutando el ciclo de aceptación, lectura, escritura y desconexión paso a paso sin *race conditions* ni *flakiness*.

### 2. Mitigación de Backpressure (Bug #19)
- **Defecto Histórico**: En VB6, `outgoingData` poseía un búfer fijo de 10 KB (`DATA_BUFFER = 10240`). Cuando se saturaba, lanzaba `NOT_ENOUGH_SPACE`. En `Protocol.bas`, 101 funciones capturaban el error, llamaban a `FlushBuffer` y ejecutaban `Resume`. Sin embargo, si el búfer del kernel del socket estaba saturado (cliente lagueado), `send()` retornaba `-1` con `WSAEWOULDBLOCK`. Al ocurrir esto, `WsApiEnviar` reinyectaba los 10 KB en `outgoingData`, produciendo un rebote continuo que devenía en un **bucle infinito ocupado (busy-loop) al 100% de CPU**, congelando el servidor.
- **Solución en C++**:
  - Se desacopló la salida mediante colas de chunks asíncronos (`std::deque<std::vector<uint8_t>>`).
  - Se fijó la constante de saturación segura `MAX_OUTGOING_BUFFER_SIZE = 65536` (64 KB).
  - Si un cliente acumula más de 64 KB encolados sin drenar, se cancela su socket preventivamente canalizando a desconexión (`CloseSocketSL` + `CerrarUsuario` o `CloseSocket`), protegiendo la estabilidad del proceso y erradicando el busy-wait.

### 3. Cierre Seguro RAII en Aceptación (Bug #20)
- **Defecto Histórico**: En `wskapiAO.bas:402`, si el filtro anti-flood de `SecurityIp` rechazaba una conexión entrante, se invocaba `WSApiCloseSocket(NuevoSock)`. No obstante, la variable `NuevoSock` se inicializaba 15 líneas más abajo (`NuevoSock = Ret`). En consecuencia, se cerraba el descriptor 0 (stdin/inválido), mientras que el descriptor TCP devuelto por `accept()` (`Ret`) quedaba abierto en el kernel de Windows, generando una fuga acumulativa de sockets (*socket leak*).
- **Solución en C++**:
  - En `HandleAccept`, la conexión entrante es recibida por valor en un objeto `asio::ip::tcp::socket`.
  - Si `SecurityIp::IpSecurityAceptarNuevaConexion` rechaza la IP, se invoca explícitamente `socket.close(ec)` y la destrucción por RAII garantiza la liberación incondicional del descriptor.

### 4. Ciclo de Vida de Desconexión y Mecánica Anti-CombatLog
- **Regla del Juego**: En Argentum Online, si un usuario sufre una caída de red o fuerza el cierre de la ventana en una zona insegura (mapa PK), su personaje no debe desaparecer de inmediato del mundo; de lo contrario, los jugadores abusarían del corte de red para no ser ejecutados en combate (*combat-logging*).
- **Arquitectura en C++**:
  - `CloseSocketSL` destruye de inmediato el socket físico en Asio y limpia los buffers de red, pero **mantiene la entidad viva** con `flags.UserLogged = true`.
  - `CerrarUsuario` activa `Counters.Saliendo = true` y fija `Counters.Salir = 10` en mapas PK (o 0 en mapas seguros).
  - `NextOpenUser` evalúa `UserList[i].ConnID == -1 && !UserList[i].flags.UserLogged`, impidiendo que el slot sea usurpado por una nueva conexión entrante mientras el personaje está en combate.
  - Al llegar a 0 tras 10 llamadas de `PasarSegundoUsuarios()`, se invoca `CloseSocket`, disparando el callback `CloseUserHandler` (para persistir el archivo `.chr`), decrementando `NumUsers`, ajustando `LastUser` y blanqueando el slot mediante `ResetUserSlot`.
  - **Anti-Exploit**: Se anula de inmediato la invisibilidad y el ocultamiento (`flags.invisible = 0`, `flags.Oculto = 0`) para evitar que el jugador inicie la salida permaneciendo invisible.

### 5. Resolución de `SecurityIp::DumpTables`
- **Contexto**: El comando administrativo `SecurityIp::DumpTables` (`SecurityIp.bas:314-327`) dependía de `TCP.GetAscIP` y `General.LogCriticEvent`.
- **Implementación**: Se resolvió incorporando un parámetro de sink inyectable `std::function<void(std::string_view)> log_sink = nullptr`, permitiendo recolectar las líneas formateadas en pruebas unitarias o emitir a `std::clog` en consola sin acoplamientos rígidos con `General.bas`.

---

## Estrategia y Cobertura de Verificación (`tests/test_tcp.cpp`)

La suite de pruebas en [`tests/test_tcp.cpp`](../../tests/test_tcp.cpp) comprende **24 casos de prueba específicos de TCP** organizados en 7 suites doctest correspondientes a cada grupo funcional:

| Suite | Tests | Cobertura Funcional Clave |
| :--- | :---: | :--- |
| **`TCP_G1_UtilidadesIP`** | 3 | Sintaxis de IPv4, conversión string -> `uint32_t` (network byte order), round-trip `GetAscIP(GetLongIp(ip))`, broadcast y ceros. |
| **`TCP_G2_GestionSlots`** | 5 | Asignación secuencial de slots, huecos intermedios, saturación `MaxUsers + 1`, blanqueo `ResetUserSlot`, mapa asociativo `BuscaSlotSock` / `AgregaSlotSock` / `BorraSlotSock`. |
| **`TCP_G3_ListenerYAceptacion`** | 4 | Inicio/detención en loopback con puerto efímero, aceptación cliente real, rechazo síncrono anti-flood (Bug #20) y rechazo por servidor lleno. |
| **`TCP_G4_RecepcionDatos`** | 4 | Recepción de tramas TCP con `async_read_some`, volcado a `incomingData`, notificación vía `SetPacketHandler`, fragmentación y aislamiento multicliente. |
| **`TCP_G5_EnvioYBackpressure`** | 4 | Despacho asíncrono con `EnviarDatosASlot`, vaciado `FlushBuffer`, orden FIFO multicontenido, mitigación del Bug #19 con corte a los 64 KB. |
| **`TCP_G6_CicloVidaYAntiCombatLog`** | 4 | Desconexión en zona segura (0s), retención de 10s en zona PK con socket destruido, anti-exploit de invisibilidad/ocultamiento, y consunción segundo a segundo hasta liberación definitiva. |
| **`TCP_G7_DiagnosticoYCierre`** | 2 | Volcado `SecurityIp::DumpTables` con sink inyectable y reinicio global de red `TCP::WSApiReiniciarSockets` con reconexión multicliente. |

### Resultados de Ejecución
- **Tests unitarios totales del proyecto**: **99 de 99 aprobados (100% exitosos)**.
- **Aserciones doctest**: **3.056 superadas sin advertencias ni fallos**.
- **Ejecución ctest**: `100% tests passed out of 1`.

---

## Referencias Cruzadas

- [Normas de Arquitectura y Convenciones — `docs/CONVENTIONS.md`](../CONVENTIONS.md)
- [Plan Maestro de Porting C++ — `docs/implementation/00-port-plan.md`](00-port-plan.md)
- [Desglose Metodológico de `TCP.bas` — `docs/implementation/14-tcp-breakdown.md`](14-tcp-breakdown.md)
- [Registro Maestro de Bugs Históricos — `docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md) (Entradas #13, #14, #19 y #20)
- [Auditoría Exhaustiva de Red y TCP — `docs/audit/02c-tcp-detalle.md`](../audit/02c-tcp-detalle.md)
- [Especificación de `SecurityIp.bas` — `docs/implementation/12-securityip.md`](12-securityip.md)
- [Bugs de `SecurityIp.bas` — `docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md#entradas-11-a-16--securityip)
