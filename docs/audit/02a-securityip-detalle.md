---
area: protocolo-de-red
status: audit
source_files:
  - legacy/server/Codigo/SecurityIp.bas
tags: [auditoria, SecurityIp, seguridad, ip, anti-flood, limites-conexiones, busqueda-binaria, winsock]
last_updated: 2026-09-10
---

# Auditoría Técnica Anexo: `SecurityIp.bas` (Adenda a `docs/audit/02-protocolo-de-red.md`)

## Resumen Ejecutivo

Este documento presenta la auditoría exhaustiva del módulo [`legacy/server/Codigo/SecurityIp.bas`](legacy/server/Codigo/SecurityIp.bas) (327 líneas en VB6 original), orientada a definir su especificación de comportamiento previa a la implementación en C++.

El módulo fue concebido para cubrir dos responsabilidades de seguridad perimetral sobre sockets TCP:
1. **Control anti-flood de intervalos entre conexiones entrantes** (`IpTables`): Rechaza intentos de conexión desde una misma dirección IP si ocurren en una ventana inferior a 1.000 milisegundos.
2. **Control de concurrencia máxima por IP** (`MaxConTables`): Lleva un conteo de sockets abiertos concurrentes por dirección IP para rechazar conexiones que superen un tope de 10 conexiones simultáneas.

De la auditoría de código se desprende un hallazgo arquitectónico de primer orden: **en el servidor de producción 0.13.0, el control de conexiones simultáneas por IP (`IPSecuritySuperaLimiteConexiones` y `IpRestarConexion`) se encontraba comentado y deshabilitado**, permaneciendo activo únicamente el control anti-flood de intervalos (`IpSecurityAceptarNuevaConexion`) y el comando administrativo de volcado (`DumpTables`).

---

## 1. Procedimientos y Funciones Públicas (`Public Sub / Function`)

A continuación se detalla cada subrutina y función pública expuesta por [`SecurityIp.bas`](legacy/server/Codigo/SecurityIp.bas), citando su signatura literal en VB6, rango de líneas y descripción en lenguaje llano:

### 1.1. `InitIpTables`
- **Signatura VB6**: `Public Sub InitIpTables(ByVal OptCountersValue As Long)`
- **Ubicación en el fuente**: [Líneas 63 a 78](legacy/server/Codigo/SecurityIp.bas#L63-L78)
- **Descripción**:
  Inicializa las tablas y variables de estado del módulo.
  - Asigna `EntrysCounter = OptCountersValue` y resetea el factor de escala `Multiplicado = 1`.
  - Redimensiona el array plano `IpTables` a un límite superior de `EntrysCounter * 2` elementos `Long` (en memoria VB6 representa $2 \times \text{EntrysCounter} + 1$ elementos).
  - Resetea el contador de entradas ocupadas en intervalos: `MaxValue = 0`.
  - Redimensiona el array plano de límites concurrentes `MaxConTables` a `Declaraciones.MaxUsers * 2 - 1` elementos `Long` (exactamente $2 \times \text{MaxUsers}$ elementos).
  - Resetea el puntero de entradas ocupadas en límites concurrentes: `MaxConTablesEntry = 0`.

### 1.2. `IpSecurityMantenimientoLista`
- **Signatura VB6**: `Public Sub IpSecurityMantenimientoLista()`
- **Ubicación en el fuente**: [Líneas 86 a 97](legacy/server/Codigo/SecurityIp.bas#L86-L97)
- **Descripción**:
  Rutina de mantenimiento periódico orientada a purgar y restablecer la tabla anti-flood de intervalos. Según el comentario del autor en la línea 92, está prevista para ejecutarse a intervalos regulares (cada 1 hora):
  - Reduce `EntrysCounter` dividiéndolo por `Multiplicado` mediante división entera (`EntrysCounter \ Multiplicado`), restableciendo la capacidad base original.
  - Restablece el multiplicador: `Multiplicado = 1`.
  - Reasigna `IpTables` desde cero con `ReDim IpTables(EntrysCounter * 2) As Long`, descartando todas las marcas de tiempo e IPs registradas sin conservar datos.
  - Reinicia `MaxValue = 0`.
  - **Nota**: No interviene sobre `MaxConTables` ni sobre `MaxConTablesEntry`.

### 1.3. `IpSecurityAceptarNuevaConexion`
- **Signatura VB6**: `Public Function IpSecurityAceptarNuevaConexion(ByVal ip As Long) As Boolean`
- **Ubicación en el fuente**: [Líneas 99 a 130](legacy/server/Codigo/SecurityIp.bas#L99-L130)
- **Descripción**:
  Determina si una conexión entrante desde la dirección `ip` (entero de 32 bits con signo) es aceptada o rechazada en base al intervalo anti-flood:
  - Invoca a la función privada de búsqueda binaria `FindTableIp(ip, IP_INTERVALOS)`.
  - **Caso IP ya registrada (`IpTableIndex >= 0`)**:
    - Evalúa la condición temporal: `If IpTables(IpTableIndex + 1) + IntervaloEntreConexiones <= GetTickCount Then`.
    - Si transcurrió el intervalo (1.000 ms o más desde la última conexión de esa IP): actualiza la marca de tiempo `IpTables(IpTableIndex + 1) = GetTickCount` y retorna `True` (conexión permitida).
    - Si no transcurrió el intervalo (menos de 1.000 ms): no actualiza la marca de tiempo y retorna `False` (conexión rechazada).
  - **Caso IP no registrada (`IpTableIndex < 0`)**:
    - Decodifica el índice de inserción aplicando el operador de complemento a uno: `IpTableIndex = Not IpTableIndex`.
    - Llama a `AddNewIpIntervalo ip, IpTableIndex` para insertar la nueva IP en el array manteniendo orden ascendente.
    - Establece la marca de tiempo inicial: `IpTables(IpTableIndex + 1) = GetTickCount`.
    - Retorna `True` (conexión permitida).

### 1.4. `IPSecuritySuperaLimiteConexiones`
- **Signatura VB6**: `Public Function IPSecuritySuperaLimiteConexiones(ByVal ip As Long) As Boolean`
- **Ubicación en el fuente**: [Líneas 162 a 190](legacy/server/Codigo/SecurityIp.bas#L162-L190)
- **Descripción**:
  Verifica si la cantidad de conexiones activas simultáneas desde `ip` superó el umbral `LIMITECONEXIONESxIP` (10 conexiones concurrentes):
  - Invoca a `FindTableIp(ip, IP_LIMITECONEXIONES)`.
  - **Caso IP ya registrada (`IpTableIndex >= 0`)**:
    - Evalúa: `If MaxConTables(IpTableIndex + 1) < LIMITECONEXIONESxIP Then`.
    - Si la cantidad actual es estrictamente menor a 10: incrementa el contador `MaxConTables(IpTableIndex + 1) = MaxConTables(IpTableIndex + 1) + 1`, registra la operación en el log mediante `LogIP`, y retorna `False` (el límite **no** fue superado; se acepta la conexión).
    - Si la cantidad actual es igual o mayor a 10: registra el rechazo mediante `LogIP` y retorna `True` (el límite **sí** fue superado; se rechaza la conexión).
  - **Caso IP no registrada (`IpTableIndex < 0`)**:
    - Asigna tentativamente el valor de retorno en `False`.
    - Evalúa si hay capacidad en la tabla: `If MaxConTablesEntry < Declaraciones.MaxUsers Then`.
    - Si hay slots disponibles: invierte el índice con `IpTableIndex = Not IpTableIndex`, inserta la IP llamando a `AddNewIpLimiteConexiones ip, IpTableIndex`, y fija el contador inicial en 1 (`MaxConTables(IpTableIndex + 1) = 1`).
    - Si se agotaron los slots de la tabla: emite un registro crítico con `Call LogCriticEvent("SecurityIP.IPSecuritySuperaLimiteConexiones: Se supero la disponibilidad de slots.")` y no inserta la IP, manteniendo el retorno en `False`.

### 1.5. `IpRestarConexion`
- **Signatura VB6**: `Public Sub IpRestarConexion(ByVal ip As Long)`
- **Ubicación en el fuente**: [Líneas 228 a 254](legacy/server/Codigo/SecurityIp.bas#L228-L254)
- **Descripción**:
  Decrementa en 1 el contador de conexiones activas para una IP que desconectó un socket:
  - Invoca a `FindTableIp(ip, IP_LIMITECONEXIONES)`.
  - **Caso IP encontrada (`key >= 0`)**:
    - Si el contador es mayor a 0, lo decrementa: `MaxConTables(key + 1) = MaxConTables(key + 1) - 1`.
    - Registra el evento en log vía `LogIP`.
    - Si el contador resultante es menor o igual a 0 (`MaxConTables(key + 1) <= 0`): elimina la IP del array desplazando los elementos restantes hacia la izquierda mediante `CopyMemory` y decrementa el total de entradas: `MaxConTablesEntry = MaxConTablesEntry - 1`.
  - **Caso IP no encontrada (`key < 0`)**:
    - Emite un registro informativo/crítico: `LogIP("restamos conexion a " & ip & " key=" & key & ". NEGATIVO!!")`.

### 1.6. `DumpTables`
- **Signatura VB6**: `Public Function DumpTables()` *(declarada sin tipo de retorno en VB6; por omisión retorna `Variant` con valor `Empty`)*
- **Ubicación en el fuente**: [Líneas 314 a 327](legacy/server/Codigo/SecurityIp.bas#L314-L327)
- **Descripción**:
  Vuelca el contenido completo de `MaxConTables` al registro de eventos críticos (`LogCriticEvent`) para fines de inspección administrativa:
  - Recorre el array mediante un bucle de paso doble: `For i = 0 To MaxConTablesEntry * 2 - 1 Step 2`.
  - Transforma la dirección numérica en string con formato decimal con puntos llamando a `GetAscIP(MaxConTables(i))`.
  - Emite la línea formateada: `"<IP en texto> > <Cantidad de Conexiones>"`.

---

## 2. Variables de Módulo, Globales y Estructuras de Datos

[`SecurityIp.bas`](legacy/server/Codigo/SecurityIp.bas) declara a nivel de módulo las siguientes variables, arrays y constantes:

| Identificador | Tipo VB6 | Ámbito | Línea | Propósito y Disposición de Memoria |
| :--- | :--- | :--- | :---: | :--- |
| `IpTables()` | `Long()` | `Private` | [44](legacy/server/Codigo/SecurityIp.bas#L44) | Array contiguo de enteros de 32 bits con pares contiguos `[IP, Timestamp]`. El índice par $2k$ contiene la IP (`Long`); el índice impar $2k+1$ contiene la marca de tiempo devuelta por `GetTickCount` (`Long`). |
| `EntrysCounter` | `Long` | `Private` | [45](legacy/server/Codigo/SecurityIp.bas#L45) | Capacidad asignada a `IpTables`. Se inicializa con el valor base pasado a `InitIpTables` (1.000). |
| `MaxValue` | `Long` | `Private` | [46](legacy/server/Codigo/SecurityIp.bas#L46) | Cantidad de IPs registradas actualmente en `IpTables`. |
| `Multiplicado` | `Long` | `Private` | [47](legacy/server/Codigo/SecurityIp.bas#L47) | Factor de multiplicación para el crecimiento dinámico escalonado de `IpTables` (inicia en 1). |
| `IntervaloEntreConexiones` | `Long` | `Private Const` | [48](legacy/server/Codigo/SecurityIp.bas#L48) | Constante con valor literal `1000`. Define el lapso mínimo en milisegundos entre conexiones permitidas por IP. |
| `MaxConTables()` | `Long()` | `Private` | [53](legacy/server/Codigo/SecurityIp.bas#L53) | Array contiguo de enteros de 32 bits con pares contiguos `[IP, CantidadConexiones]`. El índice par $2k$ almacena la IP; el impar $2k+1$ la cantidad de sockets concurrentes. |
| `MaxConTablesEntry` | `Long` | `Private` | [54](legacy/server/Codigo/SecurityIp.bas#L54) | Cantidad de IPs distintas registradas en `MaxConTables`. |
| `LIMITECONEXIONESxIP` | `Long` | `Private Const` | [56](legacy/server/Codigo/SecurityIp.bas#L56) | Constante con valor literal `10`. Máximo de conexiones concurrentes permitidas para una misma IP. |
| `e_SecurityIpTabla` | `Enum` | `Private` | [58-61](legacy/server/Codigo/SecurityIp.bas#L58-L61) | Enumeración de selección para `FindTableIp`: `IP_INTERVALOS = 1`, `IP_LIMITECONEXIONES = 2`. |

### Dependencias Globales Externas

El módulo interactúa con los siguientes símbolos y bibliotecas del sistema fuera de su archivo:

1. **`Declaraciones.MaxUsers`** (`Public MaxUsers As Integer` en [`Declares.bas:1509`](legacy/server/Codigo/Declares.bas#L1509)):
   - Leído en línea 75 para dimensionar `MaxConTables`: `ReDim MaxConTables(Declaraciones.MaxUsers * 2 - 1) As Long`.
   - Leído en línea 181 para verificar el tope de slots: `If MaxConTablesEntry < Declaraciones.MaxUsers Then`.
   - Leído en línea 214 en un mensaje de depuración: `Debug.Print "(Declaraciones.MaxUsers - index) = " & (Declaraciones.MaxUsers - index)`.
2. **`GetTickCount`** (API Win32 de tiempo de ejecución):
   - Invocado en líneas 111, 112 y 125 para obtener el tiempo transcurrido en milisegundos desde el arranque del sistema operativo.
3. **`CopyMemory`** (Alias en VB6 de la API Win32 `RtlMoveMemory`):
   - Invocado en líneas 149, 219, 220 y 247 para desplazar bloques de memoria en operaciones de inserción y compactación de arrays.
4. **Funciones de Logging y Conversión del Servidor**:
   - `LogIP` ([`TCP.bas:1898`](legacy/server/Codigo/TCP.bas#L1898)): Invocado en líneas 170, 175, 244 y 251 para escribir en `logs/ip.log`.
   - `LogCriticEvent` ([`General.bas:1048`](legacy/server/Codigo/General.bas#L1048)): Invocado en líneas 186 y 324 para escribir incidentes graves en `logs/EventosCriticos.log`.
   - `GetAscIP` ([`TCP.bas:1063`](legacy/server/Codigo/TCP.bas#L1063)): Invocado en línea 324 para convertir una IP numérica `Long` a su representación string en notación decimal con puntos.

---

## 3. Lecturas de Configuración (`Server.ini`)

> [!IMPORTANT]
> **Hallazgo Taxativo**: `SecurityIp.bas` **NO lee absolutamente ninguna clave de `Server.ini` ni de ningún otro archivo `.ini`**.

- El módulo no contiene ninguna llamada a `GetVar`, `GetPrivateProfileString` ni abstracciones similares.
- Todos sus parámetros operativos están fijados como constantes literales compiladas dentro del archivo:
  - `IntervaloEntreConexiones = 1000` ([Línea 48](legacy/server/Codigo/SecurityIp.bas#L48)).
  - `LIMITECONEXIONESxIP = 10` ([Línea 56](legacy/server/Codigo/SecurityIp.bas#L56)).
  - La capacidad inicial `OptCountersValue` se pasa como literal numérico `1000` desde el punto de llamada en [`General.bas:457`](legacy/server/Codigo/General.bas#L457) (`Call SecurityIp.InitIpTables(1000)`).
- **Aclaración sobre la clave `IntervaloParaConexion` en `Server.ini`**:
  En el archivo `Server.ini` existe una clave bajo la sección `[INTERVALOS]`: `IntervaloParaConexion= 3000`. No obstante, esta clave **no tiene relación alguna con `SecurityIp.bas`**. Dicha variable es leída en [`FileIO.bas:1579`](legacy/server/Codigo/FileIO.bas#L1579) y utilizada exclusivamente en [`frmMain.frm:687`](legacy/server/Codigo/frmMain.frm#L687) para desconectar por inactividad a clientes que se quedan en la pantalla de presentación sin loguear (`.Counters.IdleCount > IntervaloParaConexion`).
- **Discrepancia con el Plan de Porteo**:
  La entrada correspondiente a `SecurityIp` en [`docs/implementation/00-port-plan.md`](docs/implementation/00-port-plan.md#L231) indicaba hipotéticamente que el módulo leía `MaxConnectionsPerIP` desde `Server.ini` y dependía de `clsIniReader`. Esta presunción queda formalmente refutada: no existe tal clave ni dependencia de lectura de configuración en el código legacy original.

---

## 4. Entrada/Salida de Archivos (`File I/O`) y Persistencia de Baneos de IP

> [!IMPORTANT]
> **Hallazgo Taxativo**: `SecurityIp.bas` **NO realiza ninguna operación de File I/O**.

- El módulo no abre sockets ni archivos en disco: no utiliza las instrucciones de VB6 `Open`, `Close`, `Print #`, `Get #` ni `Put #`.
- Toda la persistencia de IPs baneadas (`BanIps.dat`) se encuentra desacoplada de `SecurityIp.bas` y pertenece íntegramente a los módulos `Admin.bas` y `Declares.bas`:
  1. **Estructura en Memoria**: Declarada como una colección global de VB6 en [`Declares.bas:1542`](legacy/server/Codigo/Declares.bas#L1542):
     ```vb
     Public BanIps As New Collection
     ```
  2. **Escritura a Disco (`BanIpGuardar`)**: Implementada en [`Admin.bas:405-419`](legacy/server/Codigo/Admin.bas#L405-L419). Abre `App.Path & "\Dat\BanIps.dat"` para volcar linealmente las IPs de la colección.
  3. **Lectura desde Disco (`BanIpCargar`)**: Implementada en [`Admin.bas:421-449`](legacy/server/Codigo/Admin.bas#L421-L449). Carga las IPs desde `Dat/BanIps.dat` al iniciar el servidor en [`General.bas:234`](legacy/server/Codigo/General.bas#L234).
  4. **Comprobación de Baneo en Tiempo de Conexión**: Se realiza directamente en [`wskapiAO.bas:461-470`](legacy/server/Codigo/wskapiAO.bas#L461-L470) recorriendo linealmente la colección `BanIps` contra la IP del nuevo usuario; `SecurityIp.bas` no participa de este chequeo.

---

## 5. Umbrales Exactos, Operadores y Comportamiento en los Límites (Off-by-One)

A continuación se analizan las comparaciones matemáticas y límites tal como están escritos en el código legacy:

### 5.1. Comparación en `IpSecurityAceptarNuevaConexion`
- **Expresión literal** ([Línea 111](legacy/server/Codigo/SecurityIp.bas#L111)):
  ```vb
  If IpTables(IpTableIndex + 1) + IntervaloEntreConexiones <= GetTickCount Then
  ```
- **Operador**: `<=` (menor o igual).
- **Comportamiento en el límite**:
  - Si transcurrieron exactamente 1.000 ms (`GetTickCount - UltimaConexion == 1000`): la condición evalúa a **True** y la conexión es **ACEPTADA**.
  - Si transcurrieron 999 ms: la condición evalúa a **False** y la conexión es **RECHAZADA**.

### 5.2. Comparación en `IPSecuritySuperaLimiteConexiones`
- **Expresión literal** ([Línea 169](legacy/server/Codigo/SecurityIp.bas#L169)):
  ```vb
  If MaxConTables(IpTableIndex + 1) < LIMITECONEXIONESxIP Then
  ```
- **Operador**: `<` (estrictamente menor).
- **Comportamiento en el límite**:
  - Si la IP tiene **9** conexiones concurrentes: `9 < 10` es **True** $\rightarrow$ se incrementa el contador a 10 y retorna `False` (el límite no se superó; se admite la 10ª conexión).
  - Si la IP tiene **10** conexiones concurrentes: `10 < 10` es **False** $\rightarrow$ salta al `Else`, mantiene el contador en 10 y retorna `True` (el límite fue superado; se deniega la 11ª conexión).
  - Por lo tanto, el umbral permite un máximo de **exactamente 10 conexiones simultáneas por IP**.

### 5.3. Capacidad de Slots en `IPSecuritySuperaLimiteConexiones`
- **Expresión literal** ([Línea 181](legacy/server/Codigo/SecurityIp.bas#L181)):
  ```vb
  If MaxConTablesEntry < Declaraciones.MaxUsers Then
  ```
- **Operador**: `<` (estrictamente menor).
- **Comportamiento en el límite**:
  - `MaxConTablesEntry` cuenta la cantidad de IPs distintas registradas.
  - Si hay menos de `MaxUsers` entradas (ej. 549 con `MaxUsers = 550`), se permite insertar la IP.
  - Al alcanzar exactamente `MaxUsers` (550), la condición es **False**: no se inserta la IP y se loguea error crítico.

### 5.4. Dimensión e Indexación de los Arrays
- **`IpTables`** ([Línea 72](legacy/server/Codigo/SecurityIp.bas#L72)):
  `ReDim IpTables(EntrysCounter * 2) As Long`
  En VB6, si no se especifica cota inferior, el array inicia en índice 0 y finaliza en `EntrysCounter * 2` inclusive. La cantidad total de elementos asignados es $2 \times \text{EntrysCounter} + 1$.
- **`MaxConTables`** ([Línea 75](legacy/server/Codigo/SecurityIp.bas#L75)):
  `ReDim MaxConTables(Declaraciones.MaxUsers * 2 - 1) As Long`
  El límite superior es `2 * MaxUsers - 1`. Dado que el índice base es 0, la cantidad total de elementos asignados es exactamente $2 \times \text{MaxUsers}$.

### 5.5. Crecimiento Dinámico en `AddNewIpIntervalo`
- **Expresión literal** ([Línea 140](legacy/server/Codigo/SecurityIp.bas#L140)):
  ```vb
  If MaxValue + 1 > EntrysCounter Then
  ```
- **Operador**: `>` (estrictamente mayor).
- **Comportamiento**:
  - `MaxValue` indica cuántas IPs están registradas.
  - Cuando `MaxValue` es igual a `EntrysCounter`, al evaluar `MaxValue + 1 > EntrysCounter` la condición se cumple y se dispara la expansión dinámica de `IpTables` (redimensionando con `ReDim Preserve`).
  - La expansión escala linealmente en múltiplos del tamaño base inicial (1.000, 2.000, 3.000...).

---

## 6. Comportamientos Anómalos y Errores del Legacy Observados

En esta sección se documentan, de manera neutral y sin reinterpretar ni corregir, los defectos lógicos y comportamientos anómalos identificados en el código fuente legacy:

### 6.1. Falla de Búsqueda Binaria: Cálculo Incorrecto del Punto de Inserción
- **Ubicación**: [Línea 291](legacy/server/Codigo/SecurityIp.bas#L291) y [Línea 310](legacy/server/Codigo/SecurityIp.bas#L310).
- **Código observado**:
  ```vb
  FindTableIp = Not (Middle * 2)
  ```
- **Comportamiento legacy verificado**:
  En el algoritmo de búsqueda binaria, cuando el elemento no se encuentra (`First > Last`), el punto exacto de inserción para preservar el ordenamiento reside en `First * 2`.
  En el código legacy, la función retorna el complemento a uno de `Middle * 2`. La variable `Middle` retiene el valor calculado en la última iteración del bucle, el cual no necesariamente coincide con la posición de inserción adecuada (especialmente si la última comparación redujo la cota superior `Last = Middle - 1`).
  Al utilizar `Not (Middle * 2)` como índice de inserción en `AddNewIpIntervalo` y `AddNewIpLimiteConexiones`, los nuevos elementos pueden insertarse fuera de orden en el array, rompiendo la precondición de ordenamiento requerida por la búsqueda binaria en llamadas sucesivas.

### 6.2. Condición Inicial de Búsqueda Binaria en Tablas Vacías o con Rango Desplazado
- **Ubicación**: [Línea 278](legacy/server/Codigo/SecurityIp.bas#L278) (`Last = MaxValue`) y [Línea 296](legacy/server/Codigo/SecurityIp.bas#L296) (`Last = MaxConTablesEntry`).
- **Comportamiento legacy verificado**:
  - Cuando la tabla no contiene ningún elemento (`MaxValue = 0` o `MaxConTablesEntry = 0`), las cotas iniciales se fijan en `First = 0` y `Last = 0`. El bucle `Do While First <= Last` se ejecuta una vez comparando contra el slot 0 del array, que contiene datos no inicializados (valor 0).
  - Cuando la tabla contiene $N > 0$ elementos válidos (cuyos índices van de 0 a $N - 1$), fijar `Last = N` hace que el rango de búsqueda incluya un elemento ficticio fuera de los datos cargados en la tabla.

### 6.3. Sobrecopia de Memoria / Lectura Fuera de Rango en `IpRestarConexion`
- **Ubicación**: [Línea 247](legacy/server/Codigo/SecurityIp.bas#L247).
- **Código observado**:
  ```vb
  Call CopyMemory(MaxConTables(key), MaxConTables(key + 2), (MaxConTablesEntry - (key \ 2) + 1) * 8)
  ```
- **Comportamiento legacy verificado**:
  Al compactar el array eliminando la entrada en la posición lógica $k = \text{key} \backslash 2$, la cantidad de entradas restantes a la derecha de $k$ es exactamente $\text{MaxConTablesEntry} - 1 - k$.
  La fórmula empleada calcula `(MaxConTablesEntry - (key \ 2) + 1) * 8` bytes. Esto implica que la rutina copia **2 entradas lógicas adicionales (16 bytes) más allá** del final de las entradas activas de la tabla.
  Si `MaxConTablesEntry` se encuentra cerca del tope asignado (`Declaraciones.MaxUsers`), esta copia lee y escribe fuera de los límites de memoria asignados a `MaxConTables`.

### 6.4. Falta de Manejo Defensivo ante Saturación de Slots en `IPSecuritySuperaLimiteConexiones`
- **Ubicación**: [Líneas 180 a 188](legacy/server/Codigo/SecurityIp.bas#L180-L188).
- **Comportamiento legacy verificado**:
  Si la tabla `MaxConTables` se llena (`MaxConTablesEntry >= Declaraciones.MaxUsers`), la función registra el evento crítico `LogCriticEvent`, pero mantiene la variable de retorno `IPSecuritySuperaLimiteConexiones = False` (asignada en la línea 180). Por ende, el servidor **acepta** la conexión en lugar de rechazarla, a pesar de no haber podido registrarla para su seguimiento.

### 6.5. Asimetría en el Mantenimiento Periódico
- **Ubicación**: [Línea 86](legacy/server/Codigo/SecurityIp.bas#L86).
- **Comportamiento legacy verificado**:
  `IpSecurityMantenimientoLista` purga y restablece periódicamente `IpTables`, pero no realiza ninguna limpieza sobre `MaxConTables`. La tabla `MaxConTables` depende estrictamente de que cada desconexión de usuario llame a `IpRestarConexion` para decrementar y compactar los registros.

---

## 7. Sitios de Llamada en la Base de Código Legacy (`Call Sites`)

Se analizó la totalidad del árbol de código fuente [`legacy/server/Codigo/`](legacy/server/Codigo/) para identificar todas las invocaciones a los procedimientos de `SecurityIp`:

### 7.1. Invocaciones Activas en Producción

1. **`InitIpTables`**:
   - [`General.bas:457`](legacy/server/Codigo/General.bas#L457):
     ```vb
     Call SecurityIp.InitIpTables(1000)
     ```
     Invocado durante la inicialización general del servidor en `Sub Main()`, inmediatamente antes de configurar el socket de escucha TCP.
2. **`IpSecurityMantenimientoLista`**:
   - [`General.bas:169`](legacy/server/Codigo/General.bas#L169):
     ```vb
     Call SecurityIp.IpSecurityMantenimientoLista
     ```
     Invocado en `Sub LimpiarMundo()`, ejecutado periódicamente por el recolector de basura del servidor.
3. **`IpSecurityAceptarNuevaConexion`**:
   - [`wskapiAO.bas:402`](legacy/server/Codigo/wskapiAO.bas#L402):
     ```vb
     If Not SecurityIp.IpSecurityAceptarNuevaConexion(sa.sin_addr) Then
         Call WSApiCloseSocket(NuevoSock)
         Exit Sub
     End If
     ```
     Invocado en `EventoSockAccept` inmediatamente después de aceptar un socket con la API `accept()`. Si la función retorna `False`, el socket se cierra de inmediato y se descarta la conexión.
4. **`DumpTables`**:
   - [`Protocol.bas:11393`](legacy/server/Codigo/Protocol.bas#L11393):
     ```vb
     Call SecurityIp.DumpTables
     ```
     Invocado en `HandleDumpIPTables` al recibir el paquete de administración correspondiente emitido por un Game Master de rango Dios.

### 7.2. Invocaciones Comentadas o en Código Muerto

1. **`IPSecuritySuperaLimiteConexiones`**:
   - [`wskapiAO.bas:433-438`](legacy/server/Codigo/wskapiAO.bas#L433-L438):
     ```vb
     'If SecurityIp.IPSecuritySuperaLimiteConexiones(sa.sin_addr) Then
         'tStr = "Limite de conexiones para su IP alcanzado."
         'Call send(ByVal NuevoSock, ByVal tStr, ByVal Len(tStr), ByVal 0)
         'Call WSApiCloseSocket(NuevoSock)
         'Exit Sub
     'End If
     ```
     **Estado**: **Comentado en el código fuente de producción**. El límite de conexiones simultáneas por IP no se ejecutaba al aceptar conexiones.
2. **`IpRestarConexion`**:
   - [`wskapiAO.bas:466`](legacy/server/Codigo/wskapiAO.bas#L466):
     ```vb
     'Call SecurityIp.IpRestarConexion(sa.sin_addr)
     ```
     **Estado**: Comentado al rechazar usuarios con IP baneada.
   - [`TCP.bas:626`](legacy/server/Codigo/TCP.bas#L626):
     ```vb
     'Call SecurityIp.IpRestarConexion(GetLongIp(UserList(UserIndex).ip))
     ```
     **Estado**: Comentado dentro de `CloseSocket(UserIndex)` al cerrarse la sesión de un usuario.
3. **`IpSecurityAceptarNuevaConexion` (en `CondicionSocket`)**:
   - [`wskapiAO.bas:607`](legacy/server/Codigo/wskapiAO.bas#L607):
     ```vb
     If Not SecurityIp.IpSecurityAceptarNuevaConexion(sa.sin_addr) Then
         CondicionSocket = CF_REJECT
         Exit Function
     End If
     ```
     **Estado**: Código muerto. Esta rutina correspondía a la función de condición para la API `WSAAccept`, la cual fue reemplazada en `EventoSockAccept` (línea 394) por la llamada estándar `accept(SockID, sa, Tam)`.

---

## 8. Evaluación de Cobertura en `docs/audit/02-protocolo-de-red.md`

> [!WARNING]
> **Diagnóstico**: La cobertura actual de `SecurityIp.bas` en [`docs/audit/02-protocolo-de-red.md`](docs/audit/02-protocolo-de-red.md) es **completamente inexistente**.

- En el frontmatter YAML de [`docs/audit/02-protocolo-de-red.md:12`](docs/audit/02-protocolo-de-red.md#L12), el archivo `legacy/server/Codigo/SecurityIp.bas` se encuentra listado dentro de la clave `source_files:`.
- Sin embargo, a lo largo de todo el cuerpo del documento (secciones 1 a 3, tablas de opcodes y preguntas abiertas), no existe **ni una sola mención** a la seguridad por IP, control anti-flood, topes de concurrencia, límites temporales ni estructuras de búsqueda binaria.
- El documento `02-protocolo-de-red.md` se restringió con exclusividad al encuadre de paquetes, tipos binarios, colas de bytes (`clsByteQueue`) y formatos de serialización de mensajes cliente-servidor de `Protocol.bas`.
- En consecuencia, toda la lógica, interfaces y particularidades de [`SecurityIp.bas`](legacy/server/Codigo/SecurityIp.bas) encontradas en el código fuente crudo estuvieron completamente ausentes de la auditoría de red previa.
