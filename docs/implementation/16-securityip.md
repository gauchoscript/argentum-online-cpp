---
area: infraestructura-de-red
status: completed
module: SecurityIp
layer: 1
legacy_source: legacy/server/Codigo/SecurityIp.bas
target_header: src/server/SecurityIp.hpp
target_source: src/server/SecurityIp.cpp
test_suite: tests/test_securityip.cpp
last_updated: 2026-09-10
---

# Módulo #16: SecurityIp — Control Anti-Flood y Tablas de Seguridad IP

## Resumen del Módulo

Este módulo implementa el subsistema de seguridad perimetral para conexiones entrantes de sockets de Argentum Online (`legacy/server/Codigo/SecurityIp.bas`). Provee la protección contra ataques de saturación rápida o *connection flooding* mediante el seguimiento de marcas de tiempo por dirección IP en memoria (`IpTables`), así como rutinas periódicas de mantenimiento horario y reescalado de tablas.

---

## Alcance Implementado en este Módulo

Conforme a la auditoría técnica previa ([`docs/audit/02a-securityip-detalle.md`](../audit/02a-securityip-detalle.md)), el alcance implementado cubre el camino 100% activo en el servidor de producción 0.13.0:

1. **`InitIpTables(OptCountersValue)`**: Inicializa la capacidad base de `IpTables` (por defecto 1000 entradas), resetea `Multiplicado = 1` y dimensiona el vector en ceros.
2. **`IpSecurityMantenimientoLista()`**: Subrutina invocada cada una hora por el temporizador general del servidor (`General.bas:169`) para purgar las marcas de intervalos acumuladas y reducir la capacidad al tamaño base dividiendo `EntrysCounter` por `Multiplicado`.
3. **`IpSecurityAceptarNuevaConexion(ip)`**: Función principal evaluada en cada nueva conexión entrante de socket (`wskapiAO.bas:402`). Determina si la IP debe ser aceptada o rechazada por superar el umbral de frecuencia de 1000 milisegundos.
4. **Helpers Privados Translaterados**:
   - `FindTableIp(ip)`: Búsqueda binaria literal sobre `IpTables` preservando los defectos históricos de cota inicial `Last = MaxValue` y retorno defectuoso `~(Middle * 2)`.
   - `AddNewIpIntervalo(ip, index)`: Inserción y desplazamiento en memoria con crecimiento dinámico de capacidad (`EntrysCounter = EntrysCounter * Multiplicado`).

---

## Decisiones de Diseño (Design Decisions)

### 1. Inyección de Fuente de Tiempo para `GetTickCount` (*Testing Philosophy*)
- **Contexto**: En el código legacy de VB6, `IpSecurityAceptarNuevaConexion` consulta directamente la API Win32 `GetTickCount()` de Windows.
- **Decisión en C++**: Para el entorno de producción, se mantiene la invocación directa a la API nativa de Win32 `::GetTickCount()` en Windows (y un fallback equivalente con `std::chrono::steady_clock` para entornos no-Windows).
- **Abstracción para Testing**: En estricto cumplimiento de la *Testing Philosophy* de [`docs/CONVENTIONS.md`](../CONVENTIONS.md) (*"Only mock genuine external boundaries: system clock..."*), se incorporó un mecanismo liviano de inyección de tiempo mediante un puntero a función estático (`g_TimeSource`). Esto permite que la suite de pruebas unitarias simule marcas temporales arbitrarias (casos de 999 ms, 1000 ms exactos, 1001 ms) de forma instantánea y determinista, sin requerir esperas activas (`sleep`), sin clases abstractas pesadas y sin alterar la firma pública original de la función.
- **Restablecimiento**: Se proveen las funciones auxiliares `SetTimeSourceForTesting` y `ResetTimeSource` para aislar los tests y garantizar que el reloj del sistema se use por defecto.

### 2. Reproducción del Crash de Producción por Desbordamiento Aritmético (Error 6 de VB6)
- **Evidencia del Binario Legacy**: La auditoría del archivo de configuración del proyecto [`legacy/server/SERVER.VBP`](legacy/server/SERVER.VBP#L48-L49) determinó:
  ```ini
  CompilationType=0
  OptimizationType=0
  FavorPentiumPro(tm)=0
  CodeViewDebugInfo=0
  NoAliasing=0
  BoundsCheck=0
  OverflowCheck=0
  FlPointCheck=0
  FDIVCheck=0
  UnroundedFP=0
  ```
  Con `CompilationType=0` (código nativo) y `OverflowCheck=0`, las comprobaciones de desbordamiento en enteros no estaban deshabilitadas. Por lo tanto, cualquier operación aritmética sobre enteros con signo (`Long` de 32 bits en VB6) que sobrepasara el rango $[-2^{31}, 2^{31}-1]$ disparaba un fallo fatal en tiempo de ejecución: **Error 6 ("Overflow")**.
- **Comportamiento en Producción (*Load-Bearing Crash*)**:
  En `SecurityIp.bas:111`:
  ```vb
  If IpTables(IpTableIndex + 1) + IntervaloEntreConexiones <= GetTickCount Then
  ```
  Como `IntervaloEntreConexiones = 1000`, si el timestamp previo almacenado en `IpTables` superaba $2^{31} - 1 - 1000 = 2.147.482.647$ ms (aproximadamente **24.85 días** de tiempo de actividad del sistema operativo obtenido por `GetTickCount`), la adición con signo desbordaba a nivel de hardware/runtime, abortando de inmediato el proceso del servidor.
- **Nota Inferencial Histórica (Práctica Operativa de los 2000s)**:
  En los entornos de hosting de juegos online basados en Windows de la década de 2000, los administradores solían programar reinicios periódicos automáticos del servidor (diarios o semanales) mediante tareas programadas o scripts por lotes (batch), principalmente para mitigar fugas de memoria, aplicar parches o limpiar el estado del mundo. En consecuencia, en la operación real rara vez un proceso de servidor alcanzaba 25 días continuos de *uptime* ininterrumpido sin reiniciarse, lo que mantuvo este defecto latente.
- **Solución en C++ sin Comportamiento Indefinido (UB)**:
  En el estándar de C++, el desbordamiento de enteros con signo es *Undefined Behavior* (UB). Implementar la suma directa habría permitido que el optimizador del compilador asumiera que el desbordamiento nunca ocurre, produciendo código errático. Asimismo, un *wraparound* silencioso o un rechazo silencioso de conexiones habrían alterado de forma encubierta la semántica del servidor.
  Por ello, se adoptó la **reproducción explícita mediante excepción tipada**:
  1. Se definió la jerarquía de excepciones [`SecurityIpException`](src/server/SecurityIp.hpp) y [`TickCountOverflowException`](src/server/SecurityIp.hpp), emulando el código de error `ERR_OVERFLOW = 6` de VB6 bajo el patrón de [`ByteQueueException`](src/server/clsByteQueue.hpp).
  2. En [`IpSecurityAceptarNuevaConexion`](src/server/SecurityIp.cpp), se realiza un pre-check aritmético antes de la suma:
     ```cpp
     if (lastTicks > std::numeric_limits<std::int32_t>::max() - IntervaloEntreConexiones) {
         throw TickCountOverflowException();
     }
     ```
  3. **Garantía Fuerte de Excepción (*Strong Exception Guarantee*)**: Si el chequeo falla, se lanza la excepción inmediatamente sin modificar `IpTables`, `MaxValue` ni ninguna variable del módulo.

### 3. Por qué no se toca `MaxConTables` en este paso
- En el código legacy de VB6, `InitIpTables` contenía dos líneas adicionales:
  ```vb
  ReDim MaxConTables(Declaraciones.MaxUsers * 2 - 1) As Long
  MaxConTablesEntry = 0
  ```
- **Razón de la Omisión**: Como se constató en la auditoría ([`docs/audit/02a-securityip-detalle.md`](../audit/02a-securityip-detalle.md) §1), el sistema de límite de conexiones simultáneas por IP (`MaxConTables`, `IPSecuritySuperaLimiteConexiones` e `IpRestarConexion`) se encontraba **completado pero comentado / deshabilitado en el código fuente de producción** (`wskapiAO.bas:433` y `466`, `TCP.bas:626`).
- Asignar memoria y definir variables para una tabla cuyo ciclo de vida no tiene consumidores activos en este estrato violaría la política de implementar únicamente código justificado y testeable. Por ende, la infraestructura de `MaxConTables` se posterga deliberadamente hasta la etapa de porteo del subsistema de red (`TCP.bas`).

### 4. Resumen de Aspectos Diferidos y Enlaces al Registro Maestro
Los siguientes componentes de `SecurityIp.bas` quedan formalmente diferidos para etapas posteriores, registrados en [`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md):
- **Entrada #13 (`Bug #3`)**: Sobrecopia de 16 bytes en la compactación de `MaxConTables` dentro de `IpRestarConexion` (`SecurityIp.bas:247`). Diferido a `TCP.bas`.
- **Entrada #14 (`Bug #4`)**: Retorno permisivo (`False`) ante agotamiento de slots en `IPSecuritySuperaLimiteConexiones` (`SecurityIp.bas:180-188`). Diferido a `TCP.bas`.
- **Entrada #15 (`Quirk #5`)**: Asimetría en el mantenimiento horario en `IpSecurityMantenimientoLista` (`SecurityIp.bas:86-97`), que solo purga `IpTables` pero no `MaxConTables`. Preservado fielmente en la implementación actual.
- **Entrada #16 (`Quirk #6`)**: Crash por desbordamiento de `GetTickCount` tras ~24.85 días de uptime. **Replicado fielmente** como [`SecurityIp::TickCountOverflowException`](src/server/SecurityIp.hpp).
- **`DumpTables`**: Comando administrativo de volcado (`SecurityIp.bas:314-327`). Depende de `TCP.GetAscIP` y `General.LogCriticEvent`, por lo que su migración se realizará junto con los comandos administrativos de red.

---

## Verificación y Pruebas Unitarias (`tests/test_securityip.cpp`)

La suite de pruebas en `tests/test_securityip.cpp` ejercita tanto los ayudantes privados con la traza manual del bug de inserción desordenada, como las funciones públicas y el manejo de desbordamiento:

1. **`InitIpTables`**:
   - Valida la asignación de capacidad base (`EntrysCounter = 500`), el dimensionamiento de `IpTables` en 1001 enteros, y el reseteo de `Multiplicado = 1` y `MaxValue = 0`.
2. **`IpSecurityAceptarNuevaConexion` (Escenario de Conexiones Real)**:
   - Conexión aceptada por primera vez desde IP A (`192.168.1.50` numérico).
   - Conexión rechazada dentro de la ventana de 1000 ms (+500 ms y +999 ms), confirmando que el timestamp anterior no se sobrescribe.
   - Conexión aceptada en el **límite exacto de 1000 ms** (`<=`), verificando la semántica estricta del operador de VB6.
   - Conexión aceptada desde IP B de forma concurrente e independiente de IP A.
   - Conexión aceptada tras intervalo prolongado (> 1000 ms).
   - Purga mediante `IpSecurityMantenimientoLista()` a mitad de secuencia, confirmando que una IP previamente bloqueada vuelve a ser aceptada de inmediato como nueva conexión.
3. **`IpSecurityMantenimientoLista` (Reescalado)**:
   - Valida que tras un crecimiento dinámico (`Multiplicado = 3`, `EntrysCounter = 300`), la llamada a mantenimiento divide `EntrysCounter` por `Multiplicado`, devolviéndolo a 100 y reseteando `Multiplicado = 1`.
4. **Reproducción Fiel del Error 6 de VB6 (`TickCountOverflowException`)**:
   - Comprueba que un timestamp exactamente en el límite seguro `INT32_MAX - IntervaloEntreConexiones` (2.147.482.647) no lanza excepción al evaluar el delta.
   - Comprueba que un timestamp almacenado en `INT32_MAX - IntervaloEntreConexiones + 1` (2.147.482.648) lanza inmediatamente [`SecurityIp::TickCountOverflowException`](src/server/SecurityIp.hpp).
   - Valida que la excepción transporte el código de error `ERR_OVERFLOW = 6` de VB6.
   - Valida la **invariancia absoluta del estado interno** (*strong exception guarantee*): tras dispararse la excepción, `IpTables`, `MaxValue`, `EntrysCounter` y `Multiplicado` permanecen exactamente idénticos al estado previo a la invocación.

---

## Nota para TCP.bas (Propagación Cruzada de Decisiones)

> [!IMPORTANT]
> **Directivas para el futuro porteo de `TCP.bas` (Módulo 14)**:
> 1. **Estado Comentado en Producción**: En `wskapiAO.bas:433` y `466`, las llamadas a `IPSecuritySuperaLimiteConexiones` e `IpRestarConexion` estaban comentadas en el código legacy. Al portar `TCP.bas`, se deberá decidir si este límite concurrente se activa o se mantiene en paridad con el binario legacy de producción.
> 2. **Implementación de `MaxConTables`**: Si se decide activar el control de conexiones concurrentes por IP, `InitIpTables` deberá incorporar el redimensionamiento de `MaxConTables(Declaraciones.MaxUsers * 2 - 1)`, y `IpRestarConexion` deberá implementarse corrigiendo o aislando el desbordamiento de 16 bytes documentado en la Entrada #13 de `KNOWN-LEGACY-BUGS.md`.
> 3. **Consumo de `IpSecurityAceptarNuevaConexion`**: En el handshake de conexión entrante de Asio (reemplazo de `wskapiAO.bas` / Winsock), debe llamarse a `SecurityIp::IpSecurityAceptarNuevaConexion(ip)` antes de aceptar el socket. Si retorna `false`, se debe cerrar la conexión inmediatamente sin crear buffers de usuario.
> 4. **Decisión de Manejo de `TickCountOverflowException` (Atrapar vs. Propagar)**:
>    - En VB6 (`wskapiAO.bas:402`), el evento del socket entrante no disponía de manejador local de error para esta rutina, por lo que el Error 6 derribaba el proceso completo del servidor tras ~24.85 días de uptime.
>    - Al estructurar el listener asincrónico en `TCP.bas`, queda pendiente la decisión de arquitectura de resiliencia:
>      - **Opción A (Resiliencia Moderna)**: Capturar `SecurityIp::TickCountOverflowException` en el accept handler, loguear un evento crítico (`LogCriticEvent`) alertando sobre la necesidad de reiniciar el servidor y cerrar el socket entrante, preservando el resto de la sesión de los usuarios conectados.
>      - **Opción B (Paridad Estricta de Crash)**: Dejar propagar la excepción sin captura hacia el nivel superior del loop del servidor para emular el crash fatal del proceso en paridad con el binario VB6.
