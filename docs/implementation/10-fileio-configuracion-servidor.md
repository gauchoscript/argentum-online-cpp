---
area: persistencia-fileio
source_files:
  - legacy/server/Codigo/FileIO.bas
  - src/server/FileIO.hpp
  - src/server/FileIO.cpp
  - tests/test_fileio_serverconfig.cpp
tags: [fileio, configuracion, server-ini, motd, roles, admines, doctest, c++]
last_updated: 2026-09-09
---

# Implementación de Configuración del Servidor y Roles (`FileIO` — Grupo 3)

Este documento detalla la migración a C++ de las funciones de configuración del servidor y validación de roles administrativos pertenecientes al **Grupo 3 de `FileIO.bas`** (`EsAdmin`, `EsDios`, `EsSemiDios`, `EsConsejero`, `EsRolesMaster`, `LoadMotd`, `LoadSini`).

---

## Decisiones de Diseño e Investigación del Legacy VB6

### 1. Verificación Dinámica y Directa a Disco de Roles Administrativos
- **Evidencia Legacy**: En VB6 (`FileIO.bas:49-172`), cada llamada a `EsAdmin`, `EsDios`, `EsSemiDios`, `EsConsejero` o `EsRolesMaster` realiza lecturas dinámicas sobre `Server.ini` en disco mediante `GetVar`.
- **Comportamiento Preservado**: La versión en C++ mantiene este mismo comportamiento dinámico en cada invocación (no precarga en memoria permanente), garantizando que ediciones en caliente sobre `Server.ini` tomen efecto de inmediato.
- **Claves de Consulta**:
  - `[Admines]` -> `Admin<N>`
  - `[Dioses]` -> `Dios<N>`
  - `[SemiDioses]` -> `SemiDios<N>`
  - `[Consejeros]` -> `Consejero<N>`
  - `[RolesMasters]` -> `RM<N>` (nótese el prefijo específico `RM` en lugar del nombre completo de sección).

### 2. Comparación Insensible a Mayúsculas/Minúsculas y Limpieza de Prefijos
- **Insensibilidad**: VB6 convierte a mayúsculas ambas cadenas mediante `UCase$` (`UCase$(name) = UCase$(GetVar(...))`). Se implementó comparación case-insensitive equivalente en C++.
- **Prefijos Especiales**: Si el nombre almacenado en el archivo INI comienza con los caracteres `*` o `+` (ej. `*PepeAdmin`), el procedimiento remueve dicho carácter inicial antes de comparar (`substr(1)`).

### 3. Independencia No-Jerárquica Estricta entre Roles
- **Evidencia Legacy**: No existe ninguna jerarquía o herencia entre las 5 funciones de roles. Cada función consulta de forma aislada y mutuamente exclusiva su propia sección en `Server.ini`.
- **Garantía de Seguridad**: Un personaje que figura como `[Admines]` devolverá `true` en `EsAdmin()`, pero `false` en `EsDios()`, `EsSemiDios()`, `EsConsejero()` o `EsRolesMaster()` a menos que también esté listado explícitamente en esas secciones. No se asumió ninguna jerarquía implícita para evitar vulnerabilidades o cambios inadvertidos de permisos.

### 4. Carga del Mensaje del Día (`LoadMotd`)
- `LoadMotd()` consulta `Dat/Motd.ini`, leyendo la cantidad de líneas en `[INIT] NumLines`.
- Dimensiona el vector global `MOTD` con índice base 1 (`MOTD.resize(MaxLines + 1)`), poblando `MOTD[i].texto` para $1 \le i \le \text{MaxLines}$.
- Si `NumLines` es 0 o negativo, el vector `MOTD` queda de tamaño 1 (vacío) sin generar fallos ni excepciones.

### 5. Carga de Parámetros Globales de Servidor (`LoadSini`)
- `LoadSini()` lee la configuración general desde `Server.ini` y las coordenadas de ciudades desde `Dat/Ciudades.dat`.
- Se omitieron todas las referencias a controles de interfaz gráfica de usuario VB6 (`FrmInterv.txtSanaIntervaloSinDescansar`, `frmMain.txStatus`, etc.) al tratarse de un servidor C++ headless.

---

## Propagación de Decisiones Cruzadas entre Módulos (Cross-Module Decisions)

- **`Admin.bas` (Capa 10, Módulo #30)**:
  Las variables globales de intervalos (`SanaIntervaloSinDescansar`, `StaminaIntervaloSinDescansar`, `IntervaloSed`, `IntervaloHambre`, etc.) y las estructuras de mensaje del día (`tMotd`, `MOTD`, `MaxLines`) fueron declaradas como `extern` en `Declares.hpp`/`Declares.cpp`. Al portar `Admin.bas`, se deben consumir directamente sin volver a declararlas.
- **`ModFacciones.bas` (Capa 9, Módulo #25)**:
  Las constantes de vestimentas y armaduras faccionarias (`ArmaduraImperial1..3`, `ArmaduraCaos1..3`, etc.) fueron declaradas en `Declares.hpp` y son inicializadas por `LoadSini()`.
- **`praetorians.bas` (Capa 10, Módulo #31)**:
  La constante `MAPA_PRETORIANO` fue declarada en `Declares.hpp` y es cargada por `LoadSini()`.

---

## Estrategia de Verificación y Pruebas Unitarias

Todas las pruebas unitarias para el Grupo 3 fueron implementadas en `tests/test_fileio_serverconfig.cpp` utilizando **doctest**. El módulo cuenta con validación dual:

1. **Validación contra Fixtures Sintéticos (Bordes y Casos Extremos)**:
   - **Prueba de Roles (Insensibilidad a Mayúsculas y Prefijos `*`/`+`)**: Se verificó que nicks en mayúsculas, minúsculas o mezcla (ej. `"pepeadmin"`, `"PEPEADMIN"`) e identificadores con prefijo `*` o `+` sean validados correctamente.
   - **Prueba de Ausencia de Jerarquía entre Roles**: Se probó explícitamente que un nick configurado en `[Admines]` retorne `true` en `EsAdmin()`, pero `false` en `EsDios()`, `EsSemiDios()`, `EsConsejero()` y `EsRolesMaster()`.
   - **Prueba de Límites de `LoadMotd`**: Se validaron escenarios de `NumLines = 0` (MOTD vacío) y `NumLines = 3` (carga correcta de 3 líneas delimitadas).
   - **Prueba de Carga Global `LoadSini`**: Se verificó la inicialización de puertos, flags de testing, récords, intervalos y coordenadas de ciudades (`Ullathorpe`, `Nix`).

2. **Validación contra Configuración REAL de Producción (`tests/fixtures/serverconfig/real/`)**:
   - **`Server.ini`**: Verificación de carga de claves reales de producción (`Puerto=7666`, `Record=300`, `HideServer=0`, límites faccionarios `ArmaduraImperial1 = 370` con espacios en clave, roles reales con `Dioses=1` validando que `EsDios("Elio") == true`).
   - **`Motd.ini`**: Verificación de carga de mensaje del día real con 12 líneas y strings de bienvenida.
   - **`Ciudades.Dat`**: Verificación de carga de las 5 ciudades canónicas (`Ullathorpe`, `Nix`, `Banderbill`, `Lindos`, `Arghal`) con sus coordenadas reales de producción (`Ullathorpe Map=1, X=50, Y=50`, `Lindos Map=62`, `Arghal Map=196`).

> [!IMPORTANT]
> **Estatus de Fixtures Duales**: Los tests sintéticos aseguran la invariabilidad de la lógica de roles e intervalos ante datos arbitrarios, mientras que los archivos en `tests/fixtures/serverconfig/real/` autentican la interoperabilidad directa con los archivos INI reales del servidor Argentum Online.

