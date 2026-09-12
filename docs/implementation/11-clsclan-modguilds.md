---
area: persistencia-clanes
status: in_progress
module: clsClan y modGuilds
layer: 4
legacy_source:
  - legacy/server/Codigo/clsClan.cls
  - legacy/server/Codigo/modGuilds.bas
target_header:
  - src/server/clsClan.hpp
  - src/server/modGuilds.hpp
target_source:
  - src/server/clsClan.cpp
  - src/server/modGuilds.cpp
test_suite: tests/test_clsclan.cpp
last_updated: 2026-09-10
---

# Módulo #15: clsClan y modGuilds (Sistema de Clanes)

## Resumen del Módulo

Este módulo constituye el núcleo de administración, persistencia en disco y diplomacia de clanes de Argentum Online. Está compuesto por dos unidades interconectadas:
1. **`clsClan`**: Representa una entidad individual de clan (patrón ADO / Active Record), manteniendo la persistencia en disco (`guildsinfo.inf`, `*-members.mem`, `*-solicitudes.sol`, `*-votaciones.vot`, `*-relaciones.rel`, `*-propositions.pro`).
2. **`modGuilds`**: Administrador global que gestiona el catálogo de clanes en el mundo (`guilds`), reglas de permanencia faccionaria, antifacción, fundación y comandos de usuario.

---

## Decisiones de Diseño (Design Decisions)

### 1. Frontera de Serialización de `cSolicitud` (Mapeo DTO vs. Claves INI)
> [!IMPORTANT]
> **Doble Convención en la Frontera de Serialización**:
> - En disco, los archivos `<GuildName>-solicitudes.sol` graban y leen obligatoriamente bajo cada sección `[SOLICITUD<N>]` las claves:
>   - `Nombre=<Nick>`
>   - `Detalle=<Texto>`
> - En memoria C++, el struct `cSolicitud` (`src/server/cSolicitud.hpp`) conserva obligatoriamente los nombres de campos originales en virtud de la *Naming Policy*:
>   - `cSolicitud::UserName`
>   - `cSolicitud::desc`
>
> Este desacoplamiento en la frontera de serialización (`UserName` $\longleftrightarrow$ `"Nombre"`, `desc` $\longleftrightarrow$ `"Detalle"`) es intencional y deliberado para asegurar compatibilidad byte a byte contra los archivos reales sin romper los identificadores de la base de código. Queda prohibido "corregirlo" para igualar ambos lados.

### 2. Resolución de Empates en Elecciones (`ContarVotos` / `MayorValor`)
> [!IMPORTANT]
> **Reproducción Fiel del Comportamiento Legacy ante Empates**:
> Cuando se cierra el período electoral (`RevisarElecciones` invocado durante el autoguardado `v_RutinaElecciones` en `DoBackUp`):
> 1. `ContarVotos(CantGanadores)` obtiene el resultado de `clsdicc.MayorValor(CantGanadores)`.
> 2. Si `CantGanadores > 1` (empate de 2 o más miembros con la misma cantidad máxima de votos):
>    - **NO se nombra ningún nuevo líder** (`SetLeader` no se llama). El líder actual del clan **permanece en funciones**.
>    - **NO existe balotaje ni desempate automático**: el primer clasificado no gana, ni se divide el liderazgo.
>    - **`RevisarElecciones` devuelve `False`**.
>    - **Se publica el mensaje con el error original del legacy**:  
>      `"*Empate en la votación. <Ganadores> con <CantGanadores> votos ganaron las elecciones del clan."*`  
>      donde `CantGanadores` contiene la cantidad de miembros empatados y no la cantidad de votos recibidos. Este error histórico se replica de manera intencional y exacta.
>    - **Se cierran los comicios**: `CerrarElecciones` setea `EleccionesAbiertas=0`, limpia `EleccionesFinalizan` y elimina el archivo `*-votaciones.vot`.

### 3. Encapsulación de Estado Global en Unidad de Traducción
En VB6, el array `guilds(1 To MAX_GUILDS) As clsClan` estaba declarado como `Private` dentro de `modGuilds.bas`. Siguiendo la regla de *Global Scope Requires Verified Access-Pattern Investigation* (`docs/CONVENTIONS.md`), en C++:
- `guilds` no se exporta a `Declares.hpp`.
- Se mantiene con visibilidad interna en `src/server/modGuilds.cpp` (`static` o namespace anónimo).
- El acceso es estrictamente secuencial, sincronizado con el bucle de procesamiento de paquetes del servidor.

### 4. Robustez en Borrado de Archivos Temporales
En VB6, `CerrarElecciones` ejecutaba `Call Kill(VOTACIONESFILE)`. En C++ se utiliza `std::filesystem::remove(path, ec)` verificando el código de error para evitar que la ausencia accidental del archivo lance excepciones o crashee el servidor (*desviación deliberada por seguridad*).
