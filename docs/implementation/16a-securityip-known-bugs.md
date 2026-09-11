---
area: infraestructura-de-red
status: ledger
source_files:
  - legacy/server/Codigo/SecurityIp.bas
tags: [SecurityIp, bugs, quirks, anti-flood, limites-conexiones, ledger]
last_updated: 2026-09-10
---

# Registro Consolidado de Bugs y Quirks — `SecurityIp.bas`

Este documento cataloga de forma exhaustiva todos los bugs conocidos, comportamientos anómalos y quirks históricos identificados en [`legacy/server/Codigo/SecurityIp.bas`](legacy/server/Codigo/SecurityIp.bas) durante la auditoría técnica ([`docs/audit/02a-securityip-detalle.md`](docs/audit/02a-securityip-detalle.md)), especificando su estado de replicación en el port a C++, su ubicación en el código fuente y su impacto en el camino de producción.

---

## Índice de Entradas

| ID | Origen Legacy | Descripción Sintética | Camino de Producción | Estado en C++ |
| :-: | :--- | :--- | :-: | :-: |
| **#1** | `SecurityIp.bas:291` / `310` | Retorno de `~(Middle * 2)` en vez de `~(First * 2)` en búsqueda binaria | **Activo** (`IP_INTERVALOS`) / **Muerto** (`IP_LIMITECONEXIONES`) | **Replicated** |
| **#2** | `SecurityIp.bas:278` / `296` | Cota superior inicial desplazada `Last = MaxValue` (off-by-one) | **Activo** (`IP_INTERVALOS`) / **Muerto** (`IP_LIMITECONEXIONES`) | **Replicated** |
| **#3** | `SecurityIp.bas:247` | Sobrecopia de memoria de 16 bytes en compactación de `IpRestarConexion` | **Muerto / Comentado** | **Deferred** |
| **#4** | `SecurityIp.bas:180-188` | Retorno permisivo (`False`) ante agotamiento de slots en `IPSecuritySuperaLimiteConexiones` | **Muerto / Comentado** | **Deferred** |
| **#5** | `SecurityIp.bas:86-97` | Mantenimiento asimétrico: solo purga `IpTables`, dejando `MaxConTables` perpetua | **Activo** | **Replicated** |
| **#6** | `SecurityIp.bas:111` | Error 6 ("Overflow") de VB6 tras ~24.85 días de uptime (`SERVER.VBP` `OverflowCheck=0`) | **Activo** | **Replicated** |

---

## Detalle de Entradas

### Bug #1: Retorno de `~(Middle * 2)` en vez de `~(First * 2)` en Búsqueda Binaria
- **Cita Legacy**: [`legacy/server/Codigo/SecurityIp.bas:291`](legacy/server/Codigo/SecurityIp.bas#L291) (rama `IP_INTERVALOS`) y [`legacy/server/Codigo/SecurityIp.bas:310`](legacy/server/Codigo/SecurityIp.bas#L310) (rama `IP_LIMITECONEXIONES`).
- **Descripción**: Cuando la búsqueda binaria `FindTableIp` no encuentra la IP (`First > Last`), retorna el complemento a uno de `Middle * 2` (`Not (Middle * 2)`) en lugar del punto real de inserción `First * 2`, usando un valor residual de `Middle` que provoca inserciones fuera de orden y corrompe la propiedad de ordenamiento monótono de los arrays.
- **Camino de Producción**:
  - Rama `IP_INTERVALOS` (línea 291): **Activo** (invocado en cada conexión entrante en [`legacy/server/Codigo/wskapiAO.bas:402`](legacy/server/Codigo/wskapiAO.bas#L402)).
  - Rama `IP_LIMITECONEXIONES` (línea 310): **Muerto / Comentado** (las llamadas a `IPSecuritySuperaLimiteConexiones` están comentadas en producción).
- **Estado**: **Replicated**.
  - Ubicación C++: [`src/server/SecurityIp.cpp`](src/server/SecurityIp.cpp) dentro de `SecurityIp::FindTableIp`.
  - Línea C++ comentada: [`src/server/SecurityIp.cpp:44`](src/server/SecurityIp.cpp#L44).
  - Verificación: Test unitario en [`tests/test_securityip.cpp:121`](tests/test_securityip.cpp#L121) validando que la secuencia `[100, 300, 200, 400, 150]` termina exactamente en `[150, 200, 100, 300, 400]`.

---

### Quirk #2: Cota Superior Inicial Desplazada `Last = MaxValue` (Off-by-One)
- **Cita Legacy**: [`legacy/server/Codigo/SecurityIp.bas:278`](legacy/server/Codigo/SecurityIp.bas#L278) (`Last = MaxValue`) y [`legacy/server/Codigo/SecurityIp.bas:296`](legacy/server/Codigo/SecurityIp.bas#L296) (`Last = MaxConTablesEntry`).
- **Descripción**: La búsqueda binaria inicializa `Last = MaxValue` en lugar de `MaxValue - 1`, lo que causa que en tablas vacías (`MaxValue = 0`) se ejecute una iteración espuria comparando contra memoria no inicializada (valor 0), y en tablas con $N > 0$ elementos se evalúe un elemento ficticio fuera de los datos cargados.
- **Camino de Producción**:
  - Rama `IP_INTERVALOS` (línea 278): **Activo**.
  - Rama `IP_LIMITECONEXIONES` (línea 296): **Muerto / Comentado**.
- **Estado**: **Replicated**.
  - Ubicación C++: [`src/server/SecurityIp.cpp`](src/server/SecurityIp.cpp) dentro de `SecurityIp::FindTableIp`.
  - Línea C++ comentada: [`src/server/SecurityIp.cpp:21`](src/server/SecurityIp.cpp#L21).
  - Verificación: Ejercitado en el primer paso de inserción en [`tests/test_securityip.cpp:17-32`](tests/test_securityip.cpp#L17-L32).

---

### Bug #3: Sobrecopia de Memoria / Lectura Fuera de Rango en `IpRestarConexion`
- **Cita Legacy**: [`legacy/server/Codigo/SecurityIp.bas:247`](legacy/server/Codigo/SecurityIp.bas#L247).
- **Descripción**: La fórmula `(MaxConTablesEntry - (key \ 2) + 1) * 8` empleada en `CopyMemory` para compactar `MaxConTables` copia 2 entradas lógicas (16 bytes) más allá del final de los datos válidos, provocando lecturas y escrituras fuera de rango cuando la tabla se acerca al límite `Declaraciones.MaxUsers`.
- **Camino de Producción**: **Muerto / Comentado** (las llamadas a `IpRestarConexion` en [`legacy/server/Codigo/wskapiAO.bas:466`](legacy/server/Codigo/wskapiAO.bas#L466) y [`legacy/server/Codigo/TCP.bas:626`](legacy/server/Codigo/TCP.bas#L626) están comentadas con apóstrofe).
- **Estado**: **Deferred** (postergado al porteo del módulo `TCP.bas` y las estructuras de `MaxConTables`).

---

### Bug #4: Retorno Permisivo ante Agotamiento de Slots en `IPSecuritySuperaLimiteConexiones`
- **Cita Legacy**: [`legacy/server/Codigo/SecurityIp.bas:180-188`](legacy/server/Codigo/SecurityIp.bas#L180-L188).
- **Descripción**: Si `MaxConTablesEntry >= Declaraciones.MaxUsers`, la función emite una alerta crítica mediante `LogCriticEvent`, pero mantiene la variable de retorno en `False` (línea 180), permitiendo el ingreso de la conexión sin registrarla para su seguimiento.
- **Camino de Producción**: **Muerto / Comentado** (la llamada a `IPSecuritySuperaLimiteConexiones` en [`legacy/server/Codigo/wskapiAO.bas:433`](legacy/server/Codigo/wskapiAO.bas#L433) está comentada).
- **Estado**: **Deferred** (postergado al porteo del módulo `TCP.bas` y las estructuras de `MaxConTables`).

---

### Quirk #5: Asimetría en el Mantenimiento Periódico (`IpSecurityMantenimientoLista`)
- **Cita Legacy**: [`legacy/server/Codigo/SecurityIp.bas:86-97`](legacy/server/Codigo/SecurityIp.bas#L86-L97).
- **Descripción**: La rutina de mantenimiento horario `IpSecurityMantenimientoLista` purga y restablece únicamente `IpTables`, reduciendo `EntrysCounter \ Multiplicado`, pero no interviene sobre `MaxConTables`, la cual depende exclusivamente del decremento reactivo por desconexión en `IpRestarConexion`.
- **Camino de Producción**: **Activo** (invocado en `Sub LimpiarMundo()` en [`legacy/server/Codigo/General.bas:169`](legacy/server/Codigo/General.bas#L169)).
- **Estado**: **Replicated**.
  - Ubicación C++: [`src/server/SecurityIp.cpp:78-93`](src/server/SecurityIp.cpp#L78-L93).
  - Verificación: Validado en [`tests/test_securityip.cpp:325-340`](tests/test_securityip.cpp#L325-L340).

---

### Quirk #6: Error 6 ("Overflow") de VB6 por Desbordamiento Aritmético en `IpSecurityAceptarNuevaConexion`
- **Cita Legacy**: [`legacy/server/Codigo/SecurityIp.bas:111`](legacy/server/Codigo/SecurityIp.bas#L111).
- **Descripción**: La expresión `IpTables(IpTableIndex + 1) + IntervaloEntreConexiones <= GetTickCount` en enteros de 32 bits con signo (`Long` de VB6) produce un desbordamiento aritmético cuando el timestamp almacenado supera `INT32_MAX - IntervaloEntreConexiones` (2.147.482.647 ms, aprox. 24.85 días de uptime del sistema). Con `CompilationType=0` y `OverflowCheck=0` en [`legacy/server/SERVER.VBP`](legacy/server/SERVER.VBP#L48), el ejecutable nativo de producción lanzaba el fallo fatal de tiempo de ejecución **Error 6 ("Overflow")**, abortando el proceso del servidor. En la práctica de administración de servidores de juegos de los años 2000, los reinicios periódicos programados (diarios/semanales) impedían alcanzar ese uptime ininterrumpido.
- **Camino de Producción**: **Activo** (invocado en cada conexión entrante en [`legacy/server/Codigo/wskapiAO.bas:402`](legacy/server/Codigo/wskapiAO.bas#L402)).
- **Estado**: **Replicated (Excepción Tipada)**.
  - Implementación: Pre-check en [`src/server/SecurityIp.cpp`](src/server/SecurityIp.cpp) que valida `lastTicks > std::numeric_limits<std::int32_t>::max() - IntervaloEntreConexiones` y lanza [`SecurityIp::TickCountOverflowException`](src/server/SecurityIp.hpp) (con código legacy `ERR_OVERFLOW = 6`), emulando el crash de forma segura sin incurrir en *Undefined Behavior* (UB) y garantizando la invariancia total del estado interno (*strong exception guarantee*).
  - Verificación: Validado en [`tests/test_securityip.cpp:342`](tests/test_securityip.cpp#L342) comprobando el límite seguro en `INT32_MAX - 1000` (sin excepción), el disparo obligatorio de la excepción en `INT32_MAX - 999`, y la invariabilidad de `IpTables` y `MaxValue`.
