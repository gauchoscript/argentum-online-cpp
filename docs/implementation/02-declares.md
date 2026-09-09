---
area: implementacion
status: completed
module: Declares
layer: 0
legacy_source: legacy/server/Codigo/Declares.bas
target_header: src/server/Declares.hpp
target_source: src/server/Declares.cpp
last_updated: 2026-09-07
---

# Módulo #2: Declares

## Resumen

Se completó la migración integral de `legacy/server/Codigo/Declares.bas` a un único par de archivos `src/server/Declares.hpp` y `src/server/Declares.cpp`.

La investigación confirmó que `Declares.bas` en el código fuente VB6 original es un módulo exclusivamente declarativo (no contiene cuerpos ejecutables de `Sub` o `Function`). Por lo tanto, no posee dependencias ejecutables reales de `Matematicas` ni de `clsIniReader`, lo que permitió moverlo a la Capa 0 como módulo base declarativo.

## Decisiones de Diseño

### 1. Naturaleza Puramente Declarativa de `Declares.bas`

Se confirmó mediante la inspección completa del archivo `legacy/server/Codigo/Declares.bas` que no contiene ningún procedimiento executable (`Sub` o `Function`). Dado que solo define constantes, tipos de datos (UDTs), arreglos globales y firmas de Win32 API, fue seguro portarlo como un único módulo C++ completo unificado en la Capa 0, en lugar de dividirlo en submódulos o posponer su migración.

### 2. Corrección al Plan de Port Inicial sobre Dependencias

En el plan de porting original (`docs/implementation/00-port-plan.md`) se había registrado preliminarmente que `Declares` "dependía de Matematicas y clsIniReader".

Al auditar el código fuente real del servidor VB6 (inspeccionando el contenido y los cuerpos de código en lugar de confiar ciegamente en resúmenes ejecutivos iniciales), se descubrió que dicha supuesta dependencia era inexacta. Se trataba de una asociación organizacional suelta del plan inicial y no de una dependencia real de compilación o código ejecutable. Por esta razón, el módulo `Declares` se reubicó correctamente a la Capa 0 como una cabecera maestra declarativa base.

### 3. Representación de Miembros de Clase con `std::unique_ptr` y Forward Declarations

Todos los miembros de estructuras y variables globales que en VB6 corresponden a instancias de clases (`clsByteQueue`, `clsAntiDoS`, `clsAntiMassClon`, `clsParty`, `cCola`, `ConsultasPopulares`, `SoundMapInfo`) fueron representados en C++ como `std::unique_ptr<T>` acompañados de *forward declarations* en el header (`class ClassName;`).

**Razón del Diseño**: En VB6, las variables de tipo clase son internamente tipos de referencia o punteros (handles COM) administrados en el heap, y jamás tipos de datos incrustados por valor dentro de un UDT. Usar `std::unique_ptr` preserva exactamente esta semántica de referencia y permite compilar `Declares.hpp` sin requerir la inclusión de headers de clases que aún no han sido migradas.

> [!TIP]
> **Patrón Estándar del Proyecto**: Se establece la representación de miembros `New ClassName` o referencias de clase de VB6 mediante `std::unique_ptr<ClassName>` con *forward declaration* como la convención a aplicar en todo el resto del codebase de C++ al encontrarse con miembros equivalentes en futuros módulos.

### 4. Uso Deliberado de `std::unique_ptr` vs `std::shared_ptr`

Se optó conscientemente por **NO** utilizar `std::shared_ptr` en las estructuras y globales de `Declares`. Todas las intancias globales (como `aDos`, `aClon`, `Parties`) y los miembros de `User` (`outgoingData`, `incomingData`) tienen una semántica clara de propiedad única (*single-ownership*).

El uso de `std::shared_ptr` se descartó deliberadamente para evitar sobrecostos innecesarios de conteo de referencias atómico y para explicitar el ciclo de vida del objeto. `std::shared_ptr` se reservará exclusivamente para casos genuinos de propiedad compartida (*multiple-ownership*) si llegaran a surgir en módulos posteriores.

### 5. Elección de Enumeraciones: `enum` Tradicional vs `enum class`

Las enumeraciones principales del servidor (`PlayerType`, `eClass`, `eCiudad`, `eRaza`, `eGenero`, `UserSkills`, `UserAtributos`, `eNickColor`, etc.) se migraron utilizando `enum` C++ tradicional (con tipo subyacente `std::int32_t`) en lugar de `enum class` fuertemente tipado.

**Razón del Diseño**: En VB6, las constantes de enumeración pertenecen al espacio de nombres global y se utilizan constantemente en operaciones aritméticas, índices de arreglos (`UserList[UserIndex].Stats.UserSkills[UserSkills::Magia]`) y máscaras de bits (`UserList[UserIndex].flags.Privilegios & PlayerType::Admin`). El uso de `enum class` hubiera exigido plagar el código portado de conversiones explícitas `static_cast<int>(...)` en miles de líneas, perjudicando la legibilidad y violando el principio de transliteración directa 1:1. Únicamente se usó `enum class` en tipos auxiliares aislados introducidos para *forward declarations*.

### 6. Definición e Inclusión de `struct tVertice`

La estructura plana `tVertice` (`struct tVertice { std::int16_t X{0}; std::int16_t Y{0}; };`) se incluyó en `src/server/Declares.hpp` desde el port inicial del módulo.

**Razón del Diseño**: En el código legacy de VB6, `tVertice` estaba definida originalmente en `Queue.bas` (L32). Sin embargo, en `Declares.bas` la estructura de datos del `npc` contiene la subestructura `NpcPathFindingInfo` (`PFINFO`), que a su vez contiene el camino calculado `Path() As tVertice` (representado en C++ como `std::vector<tVertice> Path;`). Para que `Declares.hpp` pudiera compilar de forma autónoma la estructura `npc` y la declaración externa `Npclist`, fue indispensable definir `struct tVertice` en las primeras líneas de `Declares.hpp`.

Posteriormente, al auditar `Queue.bas`, se confirmó que la cola `Queue` era un contenedor monohilo de uso exclusivo dentro de `SeekPath` en `PathFinding.bas`. Por ende, `Queue.bas` no requiere un módulo global C++ propio y su lógica se implementará como un `std::queue<tVertice>` local dentro de `PathFinding.hpp` al portar la Capa 8, manteniendo a `tVertice` en `Declares.hpp` como la estructura plana de coordenadas compartida por la entidad `npc`.

### 7. Declaración Forward-Reference de `IntervaloParalizado`

La variable global `IntervaloParalizado` (`extern std::int16_t IntervaloParalizado;`) fue incorporada en `src/server/Declares.hpp` como una declaración *forward-reference*.

**Razón del Diseño**: En el código fuente legacy de VB6, la variable pública `IntervaloParalizado` no fue declarada en `Declares.bas`, sino en `Admin.bas` (L58: `Public IntervaloParalizado As Integer`). Sin embargo, su valor es cargado desde `Server.ini` por el módulo `FileIO.bas` (`LoadSini`) y es consumido ampliamente por `FileIO.bas` (`LoadUserInit`), `modHechizos.bas`, `praetorians.bas` y `Trabajo.bas`. Dado que `Admin.bas` pertenece a la Capa 10 y aún no ha sido migrado, se declaró `IntervaloParalizado` en `Declares.hpp` / `Declares.cpp` (poblada por `FileIO.cpp`) para evitar dependencias circulares y permitir que `FileIO.hpp` compile y funcione de forma autónoma. Al migrar `Admin.bas` en el futuro, se reutilizará esta declaración en lugar de redeclararla.


### 8. Declaraciones Forward-Reference para Variables de Servidor, MOTD e Intervalos (FileIO Grupo 3)

Se incorporaron en `src/server/Declares.hpp` / `src/server/Declares.cpp` las declaraciones `extern` e instanciaciones globales de las variables de configuración de `Server.ini`, `Motd.ini` e intervalos de servidor:
- **`BootDelBackUp` y `Puerto`**: Declarados originalmente en `Admin.bas:86-88`, poblados desde `Server.ini` por `FileIO.cpp` (`LoadSini()`).
- **`tMotd`, `MOTD` y `MaxLines`**: Declarados originalmente en `Admin.bas:32-38`, poblados desde `Dat/Motd.ini` por `FileIO.cpp` (`LoadMotd()`).
- **Armaduras y Túnicas Faccionarias**: Declaradas originalmente en `ModFacciones.bas:34-58` (`ArmaduraImperial1..3`, `ArmaduraCaos1..3`, `VestimentaImperialHumano`, etc.), pobladas desde `Server.ini` por `FileIO.cpp` (`LoadSini()`).
- **`MAPA_PRETORIANO`**: Declarada originalmente en `praetorians.bas:40`, poblada desde `Server.ini` por `FileIO.cpp` (`LoadSini()`).
- **Intervalos de Servidor**: 24 variables de refresco, casteo, trabajo, ataque e invisibilidad (`SanaIntervaloSinDescansar`, `IntervaloUserPuedeCastear`, `MinutosWs`, etc.), declaradas originalmente en `Admin.bas:51-85`, pobladas desde `Server.ini` (`[INTERVALOS]`) por `FileIO.cpp` (`LoadSini()`).

**Razón del Diseño**: `FileIO.cpp` inicializa estas variables en el arranque del servidor (`LoadSini` / `LoadMotd`). Declararlas en `Declares.hpp` / `Declares.cpp` permite que `FileIO.cpp` opere de forma autónoma sin esperar a la migración futura de `ModFacciones.bas` (Capa 9), `praetorians.bas` (Capa 10) y `Admin.bas` (Capa 10), garantizando que dichos módulos consuman las globales declaradas sin redefinirlas.


### 9. Estructuras y Variables Globale de Tablas de Datos del Juego (FileIO Grupo 5)

Se incorporaron en `src/server/Declares.hpp` / `src/server/Declares.cpp` los tipos y declaraciones de variables globales requeridas por las tablas de datos del juego:
- **`eTipoDefArmors` y `tFaccionArmaduras`**: Estructura y enumeración para las defensas de armaduras faccionarias (`Armadas` y `Caos`), declaradas originalmente en `ModFacciones.bas:72-78`, pobladas desde `Dat/ArmadurasFaccionarias.dat` por `FileIO.cpp` (`LoadArmadurasFaccion()`).
- **`NUM_RANGOS_FACCION` y `RecompensaFacciones`**: Constante de rangos (15) y arreglo de experiencia/recompensas faccionarias, declarados en `ModFacciones.bas:63,81`, poblados por `FileIO.cpp` (`LoadBalance()`).
- **`tAPuestas` y `Apuestas`**: Estructura y global de métricas de apuestas (`Ganancias`, `Perdidas`, `Jugadas`), declaradas originalmente en `Admin.bas:40-45`, pobladas desde `Dat/apuestas.dat` por `FileIO.cpp` (`CargaApuestas()`).
- **`PorcentajeRecuperoMana`**: Variable de recupero de maná declarada en `Admin.bas:83`, poblada desde `Dat/Balance.dat` por `FileIO.cpp` (`LoadBalance()`).
- **`ExponenteNivelParty`**: Variable de balance de party declarada en `mdParty.bas:67`, poblada desde `Dat/Balance.dat` por `FileIO.cpp` (`LoadBalance()`).

**Razón del Diseño**: `FileIO.cpp` carga estas tablas durante la inicialización del servidor. Al declararlas en `Declares.hpp`, los futuros módulos de lógica de juego (`ModFacciones.bas`, `mdParty.bas`, `Admin.bas`) accederán directamente a estos datos ya cargados en memoria.


## Estructura Porteada

- **Constantes**: Todas las constantes `Public Const` fueron migradas como `constexpr` o `const` preservando sus valores numéricos y cadenas exactas.
- **Enumeraciones**: Migradas como `enum` C++ tradicionales para mantener la compatibilidad con el código legacy que utiliza enums como índices de arreglos (`UserSkills`, `UserAtributos`) o máscaras de bits (`PlayerType`, `eNickColor`).
- **Estructuras (UDTs)**: Todos los `Type...End Type` fueron migrados como `struct` preservando el orden exacto y los nombres de sus miembros.
- **Módulos de Clase y Punteros**:
  - `outgoingData` e `incomingData` en `User` y las globales `aDos`, `aClon`, `Ayuda`, `Parties`, `ConsultaPopular` y `SonidosMapas` utilizan `std::unique_ptr` con definiciones base livianas para garantizar la compilación limpia e independencia de módulos no porteados aún.
- **API Win32**: Mantenida vía `<windows.h>` con fallbacks multiplataforma portables.

## Verificación

El módulo fue incorporado a la librería `server_core` en `CMakeLists.txt` y verificado mediante compilación 100% limpia sin errores ni advertencias de símbolos incompletos.
