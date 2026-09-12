---
area: persistencia-fileio
source_files:
  - legacy/server/Codigo/FileIO.bas
  - src/server/FileIO.hpp
  - src/server/FileIO.cpp
  - tests/test_fileio_charfile.cpp
tags: [fileio, persistencia, charfile, ini, personajes, doctest, c++]
last_updated: 2026-09-09
---

# Implementación de Persistencia de Personajes (`FileIO` — Grupo 2)

Este documento detalla la migración a C++ de las funciones de persistencia de personajes (archivos `.chr`) pertenecientes al **Grupo 2 de `FileIO.bas`** (`SaveUser`, `LoadUserInit`, `LoadUserStats`, `LoadUserReputacion`, `criminal`), complementado por los helpers primitivos del **Grupo 1** (`GetVar`, `WriteVar`, `TxtDimension`, `ReadField`).

---

## Decisiones de Diseño

### 1. Preservación Estricta del Formato INI ANSI y Terminación de Línea CRLF
- `SaveUser` serializa la información del personaje utilizando la función helper `WriteVar`.
- En VB6, `WriteVar` invocaba a la API Win32 `WritePrivateProfileStringA`, la cual modificaba claves *in-place* sobre archivos existentes sin eliminar secciones no accedidas explícitamente por el procedimiento de guardado (tales como `[FLAGS]`, `[FACCIONES]` o `[INIT] Password`).
- En C++, la implementación de `WriteVar` preserva este mismo comportamiento: al modificar o guardar un archivo `.chr` existente, mantiene el orden de secciones y claves existentes, escribiendo en codificación ANSI (Windows-1252) con terminaciones de línea CRLF (`\r\n`).

### 2. Conversión de Cadenas Delimitadas mediante `ReadField`
- En VB6, campos compuestos como la posición del usuario (`"Mapa-X-Y"`) o ítems del inventario/banco (`"ObjIndex-Cantidad-Equipado"`) utilizan la función de parsing `ReadField(Pos, Text, SepASCII)`.
- Se implementó `ReadField` en C++ con semántica base 1 (`1-indexed`), permitiendo extraer de forma segura los valores numéricos mediante `std::stoi` / `std::stol` con valores por defecto resguardados ante cadenas vacías.

### 3. Mapeo de Estructuras y Enumeraciones de `Declares.hpp`
- La estructura global `User` (así como sus subtipos `UserStats`, `UserFlags`, `UserCounters`, `tReputacion`, `tFacciones`, `Inventario`, `BancoInventario`, `WorldPos`) proviene del módulo base `Declares.hpp`.
- Para evitar colisiones con identificadores reservables o tipos definidos en C++, la apariencia gráfica del personaje se mapea a través del campo `char_appearance` de la estructura `User`, preservando la paridad de atributos respecto al legacy VB6.

### 4. Fórmula de Reputación y Cálculo de Criminal
- El procedimiento `criminal(userIndex)` calcula el promedio de reputación $L$ como:
  $$L = \frac{-\text{AsesinoRep} - \text{BandidoRep} + \text{BurguesRep} - \text{LadronesRep} + \text{NobleRep} + \text{PlebeRep}}{6}$$
  Retornando `true` (criminal) si $L < 0$.

### 5. Validación de Límites de `UserIndex` y Seguridad de Memoria
- En VB6, el arreglo `UserList` es dimensionado una sola vez durante el inicio del servidor (`ReDim UserList(1 To MaxUsers)`).
- En C++, para prevenir corrupción de memoria y asegurar que `userIndex` no acceda a memoria no asignada o fuera de rango, se reemplazó la lógica de redimensionamiento dinámico implícito (`EnsureUserListCapacity`) por una verificación estricta `CheckUserIndexBounds(userIndex)`.
- Si `userIndex < 1` o `userIndex >= UserList.size()`, `FileIO` lanza una excepción `std::out_of_range`. En las pruebas unitarias y en el entorno de ejecución, `UserList` debe redimensionarse explícitamente (`UserList.resize(MaxUsers + 1)`) en la inicialización del sistema.

---

## Correcciones Aplicadas y Desviaciones de Diseño

### 1. Formato Estricto de Claves de Inventario (`Obj<N>`)
- **Evidencia Legacy**: En `FileIO.bas:1136` y `1882`, el código VB6 utiliza únicamente el formato `"Obj" & LoopC` para leer y escribir las claves de la sección `[INVENTORY]`.
- **Corrección**: Se removió cualquier lógica de fallback dual (`Item<N>`) en `FileIO.cpp` y `SaveUser`. Se corrigió el generador `tests/generate_fixtures.ps1`, se regeneraron los fixtures sintéticos en `tests/fixtures/charfile/`, y se actualizó `tests/fixtures/manifest.md`.

### 2. Integración de `IntervaloParalizado` (Forward Reference)
- **Evidencia Legacy**: `IntervaloParalizado` está declarado globalmente en `Admin.bas:58` y es consumido en `FileIO.bas:1121` (`user.Counters.Paralisis = IntervaloParalizado`).
- **Solución**: Se declaró como `extern std::int16_t IntervaloParalizado;` en `Declares.hpp` / `Declares.cpp` a modo de *forward-reference* para evitar dependencias circulares. Esta decisión fue propagada a `docs/implementation/00-port-plan.md` (sección `Admin.bas` — Capa 10) y a `docs/implementation/09-declares.md`.

### 3. Desviación Deliberada de Seguridad de Memoria (`CheckUserIndexBounds`)
- **Motivación**: Reemplazar cualquier redimensionamiento dinámico implícito de `UserList` en funciones de entrada/salida por una verificación de límites determinista que arroja `std::out_of_range` cuando el índice es inválido.
- **Verificación**: Se incluyó un caso de prueba específico en `tests/test_fileio_charfile.cpp` que valida la captura de `std::out_of_range` ante índices fuera de rango.

---

## Estrategia de Verificación y Pruebas Unitarias

Todas las pruebas unitarias fueron implementadas en `tests/test_fileio_charfile.cpp` utilizando el framework **doctest**. El módulo cuenta con validación dual:

1. **Validación contra Fixtures Sintéticos (`tests/fixtures/charfile/`)**:
   - `PEPE.chr` (Mago Humano Nivel 25): Verificación de valores exactos de atributos, vida, maná, inventario, clan y hechizos.
   - `GONZALO.chr` (Guerrero Elfo Nivel 45): Verificación de equipamiento completo y estadísticas altas.
   - `NOVATO.chr` (Aventurero Nivel 1): Verificación del comportamiento de bordes con inventario vacío (`CantidadItems = 0`).
   - `PODEROSO.chr` (Alto Elfo Nivel 50 Max): Verificación del tope de atributos (21) y límites máximos de oro y estadísticas.
   - **Prueba de Consistencia Round-Trip (Guardado y Recarga)**: Verificación de que ejecutar `SaveUser` sobre un personaje cargado produzca un archivo `.chr` que, al ser re-cargado con `LoadUserInit`, mantenga paridad en campos de estado.
   - **Prueba de Excepción por Índice Fuera de Rango**: Verificación de que invocaciones con `userIndex = 0` o mayor al tamaño de `UserList` arrojen `std::out_of_range`.

2. **Validación contra Datos REALES de Producción (`tests/fixtures/charfile/real/`)**:
   - Carga de personajes históricos del servidor legacy original: `ELIO.chr`, `USER.chr`, `BETATESTER.chr`, `MASTER.chr`.
   - Verificación de atributos reales (ej. `ELIO.chr` con `Fuerza=38`, `Agilidad=35`, `Oro=907869`), banco e inventario real con objetos de producción.
   - Verificación de roundtrip completo con `SaveUser` y re-lectura sobre `ELIO.chr`, confirmando compatibilidad retroactiva absoluta.

> [!IMPORTANT]
> **Estatus de Fixtures Duales**: Los fixtures sintéticos garantizan la cobertura de bordes y manejo de excepciones según la especificación, mientras que los fixtures en `tests/fixtures/charfile/real/` otorgan estatus autoritativo de compatibilidad con datos reales del juego histórico.


---

## Propagación de Decisiones Cruzadas entre Módulos (Cross-Module Decisions)

- **`Admin.bas` (Capa 10, Módulo #30)**:
  `IntervaloParalizado` ya fue declarado en `Declares.hpp` y es consumido por `FileIO.cpp`. Al portar `Admin.bas`, se debe utilizar esta variable sin volver a declararla.
- **`modGuilds` / `clsClan` (Capa 3, Módulo #11)**:
  `LoadUserInit` lee la sección `[GUILD] GUILDINDEX` y `Miembro`. Al migrar `clsClan` y `modGuilds`, acordarse de que la referencia de membresía del jugador leída en el `.chr` condiciona la inicialización del índice de clan en `UserList(UserIndex).GuildIndex`.
  *(Se verifica que la nota explicativa ya se encuentra reflejada en la entrada de `clsClan` / `modGuilds` en `docs/implementation/00-port-plan.md`)*.

