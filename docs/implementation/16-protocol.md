---
area: infraestructura-de-red
status: completed
module: Protocol
layer: 4
legacy_source: legacy/server/Codigo/Protocol.bas
target_header: src/server/Protocol.hpp
target_source: src/server/Protocol.cpp
test_suite: tests/test_protocol.cpp
last_updated: 2026-09-13
---

# Módulo #16: Protocol — Decodificador y Encodificador Binario de Paquetes de Red

## Resumen del Módulo

Este documento describe la arquitectura, decisiones de diseño, especificación de tipos y comportamiento verificado del módulo de Capa 4 **`Protocol`** (`src/server/Protocol.hpp` y `src/server/Protocol.cpp`), correspondiente al archivo legacy `legacy/server/Codigo/Protocol.bas` (~8.500 líneas en Visual Basic 6.0).

El módulo `Protocol` constituye la espina dorsal de interoperabilidad binaria de red de Argentum Online. Su responsabilidad se divide en tres ejes fundamentales:
1. **Serialización Unicast (`Write...`) y Multicast (`PrepareMessage...`)**: Empaquetamiento fuertemente tipado de mensajes del servidor hacia los clientes (`ServerPacketID`), garantizando compatibilidad little-endian estricta y emulación exacta de las cadenas Windows-1252 con prefijo de longitud de 2 bytes.
2. **Recepción y Desfragmentación TCP (`HandleIncomingData`)**: Bucle iterativo de drenaje de ráfagas de paquetes entrantes que resuelve fragmentación de stream mediante transacciones reversibles atómicas.
3. **Despacho Monolítico Central (`DispatchPacket`)**: Réplica directa y exhaustiva en C++20 del `Select Case` de VB6 que enruta los 129 identificadores de paquete (`ClientPacketID`) hacia sus respectivos handlers y stubs de desacoplamiento de capas lógicas.

---

## Decisiones de Diseño y Desviaciones Intencionales

### 1. Transaccionalidad RAII en Recepción (`ByteQueueTransaction`)

#### Mecánica Legacy en VB6
En el servidor original de Visual Basic 6 (`Protocol.bas`), la tolerancia a paquetes fragmentados combinaba dos mecanismos manuales fragmentados:
- **Tramas Fijas**: Se comprobaba manualmente la disponibilidad mínima de bytes al inicio del sub antes de cualquier lectura (por ejemplo, `If UserList(UserIndex).incomingData.length < 3 Then Err.Raise ... Exit Sub`). Si la verificación pasaba, se leía el opcode y sus argumentos en bloque.
- **Tramas Dinámicas con Cadenas**: Cuando el paquete contenía cadenas de texto de longitud variable (como en `HandleTalk`, `HandleLoginExistingChar`, `HandleChangePassword`, `HandleGuildMessage`), el tamaño total no podía conocerse a priori mediante una constante. En estos casos, el desarrollador de VB6 instanciaba una cola auxiliar local:
  ```vb
  Dim buffer As New clsByteQueue
  Call buffer.CopyBuffer(.incomingData)
  Call buffer.ReadByte ' Descartar packet ID
  ...
  ' Si el paquete se leía completo:
  Call .incomingData.CopyBuffer(buffer)
  ```
  Si se producía un error de bajo nivel (`NOT_ENOUGH_DATA`), la rutina saltaba al manejador de errores `Errhandler:`, descartando la copia local `buffer` y dejando intacta la cola `incomingData` principal del usuario.
- **Reinyección Manual de Opcode**: En otros handlers secundarios que no utilizaban `CopyBuffer`, si fallaba una lectura intermedia tras haber consumido el identificador de paquete con `.ReadByte`, se intentaba paliar la desincronización reinyectando manualmente el opcode al principio de la cola o desconectando forzosamente al usuario.

#### Justificación en C++20
Al carecer Visual Basic 6 de destructores deterministas (RAII) y manejo moderno de excepciones estructuradas, la lógica de recuperación de buffers quedaba dispersa y propensa a desalineaciones del stream TCP si un desarrollador omitía el `CopyBuffer` en un handler nuevo.

En C++20, encapsulamos esa misma semántica en la clase interna `ByteQueueTransaction` dentro de `src/server/Protocol.cpp`:
```cpp
class ByteQueueTransaction {
public:
    explicit ByteQueueTransaction(clsByteQueue& queue)
        : m_queue(queue), m_snapshot(queue), m_committed(false) {}

    void commit() noexcept {
        m_committed = true;
    }

    ~ByteQueueTransaction() {
        if (!m_committed) {
            m_queue = m_snapshot; // Rollback atómico automático
        }
    }

    ByteQueueTransaction(const ByteQueueTransaction&) = delete;
    ByteQueueTransaction& operator=(const ByteQueueTransaction&) = delete;
    ByteQueueTransaction(ByteQueueTransaction&&) = delete;
    ByteQueueTransaction& operator=(ByteQueueTransaction&&) = delete;

private:
    clsByteQueue& m_queue;
    clsByteQueue m_snapshot;
    bool m_committed;
};
```

**Ventajas de la Abstracción**:
1. **Garantía de Rollback Atómico**: Si en cualquier punto de la deserialización de argumentos salta `NotEnoughDataException` (sea por un byte faltante en un entero o por una cadena truncada respecto a su prefijo de 2 bytes), el destructor de la transacción restaura `m_queue = m_snapshot`. El opcode y los bytes parciales retornan a su posición inicial sin haber sido consumidos, y el despachador devuelve limpiamente `PacketParseResult::NeedMoreData`.
2. **Cero Polución de los 129 `case`**: Ninguna de las 129 ramas del `switch` requiere bloques `try/catch` manuales ni código boilerplate de restauración; cada rama solo invoca `tx.commit()` al finalizar exitosamente sus lecturas.
3. **Encapsulamiento en la Unidad de Traducción**: `ByteQueueTransaction` reside estrictamente en el namespace anónimo de `src/server/Protocol.cpp`, evitando exponer tipos innecesarios en la cabecera pública `src/server/Protocol.hpp`.

#### Nota de Rendimiento y Deuda Técnica Futura
La implementación actual clona la cola en el constructor (`m_snapshot(queue)`), emulando fielmente el costo del `CopyBuffer` de VB6.

> [!NOTE]
> **Ruta de Optimización Futura (Cargas Masivas Concurrentes)**:
> Para fases posteriores de optimización bajo perfiles de carga masiva de producción (cientos o miles de conexiones concurrentes en ráfagas de alta tasa de paquetes), este componente debe evolucionar hacia un esquema de **checkpoint de cursor/offset de lectura** (`read_offset`). Mediante este mecanismo, el rollback consistirá únicamente en reasignar un puntero o entero de desplazamiento (`m_read_offset = m_saved_offset`), eliminando por completo las copias de vectores en el stack para cada paquete recibido.

---

### 2. Erradicación del Patrón `Resume` en Primitivas `Write...` (Bug #19)

- **Comportamiento en VB6**: En `Protocol.bas:1890` (y a lo largo de 101 funciones de salida), la captura de `NOT_ENOUGH_SPACE` ejecutaba `Call FlushBuffer(UserIndex)` seguido de `Resume`. Si el socket TCP del sistema operativo entraba en estado `WSAEWOULDBLOCK` (ventana de recepción del cliente llena), `FlushBuffer` rebotaba y `Resume` reintentaba la escritura indefinidamente. Al ejecutarse en el único hilo del servidor, provocaba un **bucle infinito ocupado (busy-wait) al 100% de CPU** que congelaba el proceso entero (Bug #19).
- **Tratamiento en C++20**: Las 102 funciones `Write...` simplemente delegan la serialización en `UserList[UserIndex].outgoingData`. La mitigación de saturación, el buffering asíncrono y el backpressure seguro se delegan de forma centralizada en la Capa 4 de Transporte (`TCP::EnviarDatosASlot` y `TCP::FlushBuffer`). Si un cliente satura su cola saliente superando el umbral de seguridad, el subsistema de transporte programa su desconexión controlada, protegiendo la estabilidad del servidor.

---

### 3. Truncamiento a 3 Bytes de Colores RGB en Tramas Salientes (Long de VB6 vs. Red)

- **Comportamiento en VB6**: En `Protocol.bas:825-835` (`WriteChatOverHead`), el parámetro `color` se declara como `Long` (32 bits, 4 bytes en memoria). Sin embargo, el código descompone y serializa únicamente 3 bytes individuales correspondientes a los canales R, G y B:
  ```vb
  Call .WriteByte(color And &HFF)
  Call .WriteByte((color \ &H100) And &HFF)
  Call .WriteByte((color \ &H10000) And &HFF)
  ```
  Descartando el byte superior (`color \ &H1000000`).
- **Tratamiento en C++20**: Una transliteración ingenua en C++ serializaría un `std::int32_t` completo (4 bytes), lo que rompería de forma catastrófica la alineación del cliente VB6 (que espera exactamente 3 lecturas `ReadByte`). En `src/server/Protocol.cpp` (`PrepareMessageChatOverHead` y `WriteChatOverHead`), se preserva estrictamente la descomposición manual y serialización de los 3 bytes componentes en Little-Endian, garantizando paridad binaria exacta a nivel de byte.

---

### 4. Desacoplamiento de Capas Posteriores (6 a 10) mediante Stubs

- **Comportamiento en VB6**: Los métodos `Handle...` del servidor original mezclaban la decodificación del paquete con llamadas directas e inmediatas a la lógica global de simulación (modificación de inventarios, casteo de hechizos, fórmulas de combate, comercio, base de datos de clanes, comandos de administración).
- **Tratamiento en C++20**: Para posibilitar la construcción incremental del servidor y permitir que la Capa 4 de Infraestructura de Red compile, se ejecute y se pruebe de forma autónoma sin requerir el resto del código del juego, cada una de las 129 ramas del `switch` en `DispatchPacket` extrae la totalidad de los argumentos de red del stream y delega en un stub desacoplado (`Handle...Stub`).
- **Contrato para Capas Posteriores**: Cuando se porten las Capas 6 (Usuarios, Inventario, Hechizos), 7 (Combate), 8 (Comercio, Clanes, NPCs) y 10 (Admin), los stubs serán conectados a las funciones de dominio correspondientes sin alterar la lógica de extracción de red ni el despachador monolítico.

---

### 5. Fidelidad Binaria Estricta (`uint8_t` Underlying Type)

- **Enumeraciones Fuertemente Tipadas**: `ClientPacketID`, `ServerPacketID`, `FontTypeNames`, `eEditOptions` y `PacketParseResult` se definieron con tipo subyacente explícito `std::uint8_t`. Esto previene desalineaciones de tamaño (en C++ un `enum` clásico puede compilarse a 4 bytes por defecto) y asegura que al serializar o deserializar opcodes se transfiera exactamente 1 byte por red.
- **Indexación Base de Opcodes**:
  - `ClientPacketID`: Inicia en `LoginExistingChar = 0` y culmina en `Consultation = 128` (129 opcodes exhaustivos), en paridad 1:1 con las constantes del cliente original de Argentum Online 0.13.0.
  - `ServerPacketID`: Inicia en `logged = 1` y culmina en `CancelOfferItem = 117` (117 opcodes de salida).

---

### 6. Reentrancia y Concurrencia Segura en Multicast (`PrepareMessage...`)

- **Erradicación del Estado Global**: En el servidor legacy existía una instancia global compartida (`auxiliarBuffer As New clsByteQueue`) utilizada secuencialmente por todas las funciones `PrepareMessage...`. Este diseño impedía cualquier grado de reentrancia o ejecución segura.
- **Escritor Local y Retorno Directo**: Cada una de las 22 rutinas `PrepareMessage...` instancia localmente su propio `clsByteQueue`, serializa los campos correspondientes y extrae el payload en un vector binario contiguo (`std::vector<std::uint8_t>`), listo para ser inyectado directamente en las funciones de difusión de `modSendData` mediante `std::span<const uint8_t>`.

---

### 7. Control Estricto de Sesión y Secuencia en `HandleIncomingData`

El despachador aplica una máquina de estados estricta sobre la bandera `UserLogged`:
1. **Usuarios No Logueados (`!user.flags.UserLogged`)**:
   - Solo tienen permitido transmitir los paquetes de autenticación: `LoginExistingChar`, `ThrowDices` y `LoginNewChar`.
   - Cualquier intento de transmitir paquetes de jugabilidad (ej. `Attack`, `Walk`, `CastSpell`) constituye una anomalía de seguridad y ejecuta `TCP::CloseSocket(UserIndex)` de forma inmediata.
2. **Usuarios Ya Autenticados (`user.flags.UserLogged`)**:
   - Tienen prohibido volver a transmitir paquetes de login (`LoginExistingChar` o `LoginNewChar`). La recepción de los mismos fuerza el cierre inmediato de la conexión (`TCP::CloseSocket`).
3. **Reseteo de Inactividad y Combate**:
   - Para cualquier opcode válido dentro de rango, se reinicia el contador de inactividad (`user.Counters.IdleCount = 0`).
   - El jugador pierde la protección pasiva de no ser atacado (`user.flags.NoPuedeSerAtacado = false`).

---

## Estructura de Componentes e Interfaces

```
ao::net::protocol
│
├── Opcodes y Enumeradores Tipados:
│   ├── ClientPacketID (0..128)
│   ├── ServerPacketID (1..117)
│   ├── FontTypeNames (1..14)
│   ├── eEditOptions (1..13)
│   └── PacketParseResult (Ok, NeedMoreData, InvalidSession, MalformedData, UnknownPacket)
│
├── Generadores Multicast (PrepareMessage...):
│   └── 22 funciones que retornan std::vector<std::uint8_t>
│
├── Primitivas Unicast (Write...):
│   ├── 102 funciones de salida hacia UserList[UserIndex].outgoingData
│   └── FlushBuffer(UserIndex)
│
├── Despachador Monolítico Central:
│   ├── DispatchPacket(UserIndex, opcode) -> switch (opcode) con 129 cases
│   └── 129 stubs desacoplados en namespace anónimo (Handle...Stub)
│
└── Bucle de Recepción y Desfragmentación:
    └── HandleIncomingData(UserIndex) -> while iterativo con ByteQueueTransaction
```

---

## Cobertura de Pruebas Unitarias

La suite de pruebas en `tests/test_protocol.cpp` valida de forma automatizada los 4 pasos del módulo:

| Test Suite / Caso de Prueba | Escenario Verificado | Estado |
| :--- | :--- | :---: |
| **Paso 1: Opcodes y Tipos Fuertes** | Verificación binaria de valores numéricos de `ClientPacketID`, `ServerPacketID`, `FontTypeNames` y `eEditOptions`. | Pasado |
| **Paso 2: Generadores Multicast** | Serialización little-endian exacta de los 22 generadores `PrepareMessage...` cotejada byte a byte con los paquetes salientes esperados. | Pasado |
| **Paso 3: Primitivas Unicast** | Encolamiento de paquetes salientes en `outgoingData` para las 102 funciones `Write...` y transaccionalidad de `FlushBuffer`. | Pasado |
| **Paso 4: Desfragmentación Fija (`Walk`)** | Llegada parcial de 1 byte (solo opcode) $\to$ Rollback verificado, cola intacta $\to$ Llegada del byte de heading restante $\to$ Consumo exitoso del paquete. | Pasado |
| **Paso 4: Desfragmentación Dinámica (`Talk`)** | Segmentación en 3 fases (solo opcode + 1 byte longitud $\to$ longitud completa + texto parcial $\to$ remanente de texto) $\to$ Rollback verificado en cada corte hasta completarse. | Pasado |
| **Paso 4: Ráfaga Concatenada** | Drenaje continuo e ininterrumpido de 3 paquetes en un mismo buffer (`Walk` + `SafeToggle` + `Talk`). | Pasado |
| **Paso 4: Violación de Sesión (No Logueado)** | Envío de `Attack` con `UserLogged = false` $\to$ Cierre inmediato del socket con `CloseSocket`. | Pasado |
| **Paso 4: Violación de Sesión (Ya Logueado)** | Envío de `LoginExistingChar` con `UserLogged = true` $\to$ Cierre inmediato del socket con `CloseSocket`. | Pasado |
| **Paso 4: Reseteo de Inactividad y Combate** | Paquete válido resetea `IdleCount` a 0 y desactiva `NoPuedeSerAtacado`. | Pasado |
| **Paso 4: Opcode Desconocido** | Envío de opcode 250 (fuera de rango) $\to$ Detección de `UnknownPacket` y desconexión inmediata. | Pasado |

**Resultado total de la suite de pruebas**: 147 test cases pasados, 0 fallados, 3480 aserciones exitosas.