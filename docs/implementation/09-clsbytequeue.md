---
area: protocolo-de-red
status: completed
source_files:
  - legacy/server/Codigo/clsByteQueue.cls
cpp_files:
  - src/server/clsByteQueue.hpp
  - src/server/clsByteQueue.cpp
  - tests/test_clsbytequeue.cpp
tags: [clsByteQueue, red, protocolo, fifo, buffer, serializacion, little-endian, cp1252]
last_updated: 2026-09-09
---

#clsByteQueue — Cola FIFO de Bytes y Serialización Binaria de Red

## Resumen

`clsByteQueue` es el componente central de serialización y deserialización binaria de red de Argentum Online. Maneja un búfer FIFO contiguo en memoria con semántica Little-Endian para empaquetar y desempaquetar tipos numéricos (`uint8_t`, `int16_t`, `int32_t`, `float`, `double`), valores booleanos de 1 byte y cadenas de texto en codificación Windows-1252 (ANSI) prefijadas por longitud.

---

## Decisiones de Diseño

### 1. Representación en Memoria y Copia de Bytes Nativa (Little-Endian)
- La clase legacy en VB6 utilizaba `CopyMemory` (`RtlMoveMemory` de Win32 API) para volcar directamente variables `Integer` (2 bytes), `Long` (4 bytes), `Single` (4 bytes) y `Double` (8 bytes) a la cola de bytes.
- En la implementación C++, se utiliza `std::memcpy` sobre los tipos C++ estándar (`std::int16_t`, `std::int32_t`, `float`, `double`), lo que preserva exactamente la disposición Little-Endian x86/x64 nativa sin introducir bibliotecas externas de serialización.

### 2. Estructura del Búfer y Desplazamiento en Eliminación (Shift-on-Remove)
- `clsByteQueue` **NO es una cola circular**. Es un búfer contiguo lineal `std::vector<std::uint8_t>` con capacidad por defecto de 10.240 bytes (`DATA_BUFFER = 10240`).
- Al invocar `RemoveData`, los bytes remanentes se desplazan hacia el comienzo del búfer (índice 0) mediante `std::memcpy`.

### 3. Jerarquía de Excepciones C++ y Semántica de Err.Raise (Underflow y Overflow)
- En lugar de códigos de retorno de error o aserciones que puedan ignorarse silenciosamente, la implementación utiliza tipos de excepción C++ fuertemente tipados derivados de la clase base compartida `ByteQueueException` (la cual hereda de `std::runtime_error`):
  - **`ByteQueueException`**: Clase base abstracta/compartida de excepciones del búfer. Provee el método `getErrorCode()` con el código numérico original de VB6.
  - **`NotEnoughDataException` (Underflow)**: Se lanza al intentar leer o inspeccionar más bytes de los disponibles (`dataLength > queueLength`), asociando el código legacy `NOT_ENOUGH_DATA` (`vbObjectError + 9` = `-2147221495`). **Invariante garantizado**: El búfer y su estado permanecen 100% inalterados (0 bytes consumidos).
  - **`NotEnoughSpaceException` (Overflow)**: Se lanza al intentar escribir superando la capacidad disponible (`queueCapacity - queueLength - dataLength < 0`), asociando el código legacy `NOT_ENOUGH_SPACE` (`vbObjectError + 10` = `-2147221494`).
- **Propagación Cruzada hacia `Protocol.bas` y `TCP.bas`**:
  En el servidor VB6, `Protocol.bas` envuelve las operaciones de escritura a `outgoingData` en bloques `On Error GoTo Errhandler`. Cuando se produce un desbordamiento de búfer (`NotEnoughSpaceErrCode`), el handler invoca `FlushBuffer(UserIndex)` para enviar inmediatamente el contenido acumulado al socket TCP y reintenta la escritura mediante `Resume`. Al portar `TCP` y `Protocol`, la captura de `NotEnoughSpaceException` debe replicar o adaptar esta semántica de vaciado automático de búfer.

### 4. Codificación y Serialización de Cadenas de Caracteres (`Strings`)
- **`WriteASCIIString` / `ReadASCIIString`**:
  - Escribe/lee un prefijo de 2 bytes con signo (`std::int16_t` Little-Endian) indicando la longitud en bytes.
  - A continuación escribe/lee $N$ bytes codificados en **Windows-1252 (CP1252 / ANSI)**.
  - **Sin terminador nulo (`\0`)**.
  - Si la cadena está vacía (`""`), se escribe un prefijo de 2 bytes `0x0000` y 0 bytes adicionales.
- **`WriteASCIIStringFixed` / `ReadASCIIStringFixed`**:
  - Escribe/lee $N$ bytes crudos en Windows-1252 sin prefijo de longitud ni terminador nulo.

### 5. Inexistencia de Hilos Internos / Concurrencia
- Cada conexión de cliente en el servidor posee sus propias instancias independientes `incomingData` y `outgoingData`.
- `clsByteQueue` es una clase ligera sin cerrojos ni mutexes internos (no thread-safe), idéntica a VB6. La sincronización entre hilos para Asio se administra a nivel del manejador de red (`TCP`).

---

## Verificación

Se implementaron pruebas unitarias con **doctest** en `tests/test_clsbytequeue.cpp`, con aserciones sobre secuencias de bytes crudas en literales hexadecimales:
1. **Entero positivo (`0x1234`)**: Verifica bytes `{ 0x34, 0x12 }`.
2. **Entero negativo (`-1234`)**: Verifica bytes complemento a dos `{ 0x2E, 0xFB }`.
3. **Flotante Single (`1.5f`)**: Verifica bytes IEEE-754 `{ 0x00, 0x00, 0xC0, 0x3F }`.
4. **String con acentos (`"ñandú"`)**: Verifica prefijo `{ 0x05, 0x00 }` y bytes CP1252 `{ 0xF1, 0x61, 0x6E, 0x64, 0xFA }`.
5. **String vacía (`""`)**: Verifica prefijo `{ 0x00, 0x00 }`.
6. **Underflow**: Verifica que al fallar la lectura de un entero en una cola de 1 byte, se lanza `NotEnoughDataException` y el byte de la cola se preserva intacto.
7. **Overflow**: Verifica que al superar la capacidad se lanza `NotEnoughSpaceException`.
8. **CopyBuffer y Peek**: Verifica la clonación completa de buffer e inspección sin remoción de bytes.
