# Migration Conventions — Structural Port Policy

This project is a structural (transliteration) port of a legacy VB6 codebase 
to C++, not a re-architecture. The goal is maximum familiarity for developers 
who already know the original VB6 codebase, not idiomatic "modern" C++ design.

## Naming Policy
For every file, class, function, and variable, apply this fallback order:
1. Preserve the exact legacy VB6 identifier, if one exists and is a valid 
   C++ identifier.
2. If no legacy identifier exists (something new, required only by C++ or 
   by a library we introduced), name it in Rioplatense Spanish.
3. If the Rioplatense term would be regionally unclear, fall back to a more 
   broadly understood Latin American Spanish term.

Exception: if a legacy identifier collides with a reserved C++ keyword 
(e.g. New, Class, Public), skip directly to step 2/3 of this same chain — 
do not fall back to English.

## Structure Policy
Mirror the legacy .bas/.cls/.frm module grouping as closely as possible — 
one legacy module maps to one C++ file or file pair with the same name. 
Where this isn't possible (e.g. a .frm file mixes UI and logic in a way 
with no direct C++/SFML equivalent), group by the same conceptual boundary 
instead, using standard .h/.cpp conventions.

## What is NOT covered by this policy
Library and tooling choices (standalone Asio, SFML, SQLite, TGUI, nlohmann/json, 
doctest, CMake/vcpkg) are unavoidable technology substitutions for VB6-specific 
APIs (Winsock, DirectX, flat files) and are not subject to the "preserve 
legacy structure" rule — only the game logic's own organization is.

## Testing Policy
All unit tests across this entire project use **doctest** as the single testing 
framework. It was chosen specifically for its minimal compile-time overhead 
given the large number of modules being ported.

## Testing Philosophy
This project follows the Classical/Detroit school of testing: prefer real 
objects and real collaborators over mocks. Only mock genuine external 
boundaries (network sockets, filesystem where fixture/temp-file testing is 
impractical, system clock, other non-deterministic or slow externalities) 
— never mock an internal module we're also porting in this project.

Every test must earn its place. Before writing a test, confirm it does at 
least one of the following, or don't write it:
- Tests a boundary or edge case (zero, negative, empty, max value, an 
  exact known cap from the legacy system).
- Tests a quirk of the ORIGINAL VB6 behavior specifically surfaced during 
  audit (rounding, truncation, range inclusivity, case sensitivity, etc.) 
  — cite the audit finding this test is protecting.
- Tests a known translation risk area (integer division/overflow 
  differences between VB6 and C++, string encoding boundaries).
- Tests real integration between two already-ported modules using real 
  objects, not isolated arithmetic.

Do not write tests that only restate a trivial one-line operation with no 
real risk of mistranslation — these add maintenance cost without adding 
protection. When in doubt, ask: "would this test have caught a real bug if 
the port had gotten something subtly wrong?" If no, skip it.

### Test Fixtures Policy: Synthetic vs. Real / Authoritative Data
The project maintains a strict architectural distinction between two categories of test fixtures in `tests/fixtures/`:
1. **Synthetic Fixtures (`tests/fixtures/*/`)**: Hand-crafted or script-generated test cases (via `tests/generate_fixtures.ps1` or dedicated test routines) designed deliberately to stress boundaries, edge cases, maximum capacities, string encodings, and error handling states (e.g. empty inventories, maxed stats, non-existent identifiers).
2. **Real / Authoritative Production Fixtures (`tests/fixtures/*/real/` and `tests/fixtures/maps/`)**: Original, unaltered data files copied directly from the shipped legacy server (`legacy/server/`). These provide authoritative validation of real-world backwards compatibility, byte-exact binary layouts (e.g. map headers and tile bitfields), and real configuration/table parsing (e.g. `obj.dat`, `Hechizos.dat`, `Balance.dat`, `Server.ini`, real `.chr` files).

Every module dealing with serialization/deserialization or data tables must validate against **both**: synthetic fixtures to guarantee edge case coverage, and authoritative fixtures to ensure fidelity against the original shipped game.


## Lista de Chequeo de Finalización de Módulos (Module-Porting Checklist)

El porting o investigación de un módulo **NO se considera completo** hasta que existan los **TRES** elementos de documentación requeridos:

1. **Comentarios de Código en el Fuente**: Comentarios explicativos directamente en los archivos de código fuente C++ (`.hpp` / `.cpp`) indicando cualquier comportamiento no obvio, quirk del legacy VB6 o desviación respecto a una traducción ilusa/directa.
2. **Documentación de Implementación en `docs/implementation/<modulo>.md`**: Una entrada correspondiente bajo la sección **"Decisiones de Diseño"** (*Design Decisions*) que describa los mismos hallazgos en lenguaje llano, redactada para alguien que no haya abierto el archivo fuente.
3. **Propagación de Decisiones Cruzadas entre Módulos (*Cross-Module Decision Propagation*)**: Antes de dar por completado un módulo, verificá explícitamente: ¿algún hallazgo, decisión de diseño o descubrimiento de comportamiento realizado durante este trabajo afecta a OTRO módulo que aún no haya sido porteado?

> [!IMPORTANT]
> **Regla de Incompletitud Cruzada**: Una decisión que solo vive en la documentación del módulo donde fue descubierta, pero afecta a un módulo diferente, es una **decisión incompleta**. La propagación a la entrada del módulo afectado en `docs/implementation/00-port-plan.md` es **obligatoria, no opcional**.

### Escenarios Típicos de Decisiones Cruzadas:
- **Estructuras compartidas o globales**: La investigación de un módulo revela que su comportamiento real vive o es consumido por otro módulo no porteado (ej. la limpieza real de `cGarbage` ejecutándose en `Acciones.bas` y `General.bas`).
- **Contratos y formatos de salida no obvios**: El formato o comportamiento de un módulo condiciona a un módulo consumidor futuro que necesita saberlo para evitar asumir un comportamiento incorrecto (ej. el desempate de `MayorValor` en `clsdicc` afectando a `clsClan`).
- **Resolución de dependencias**: Una dependencia que originalmente se asumía propia de un módulo cambia lo que un módulo futuro debe implementar (ej. el búfer global de `Queue.bas` convirtiéndose en un `std::queue` local dentro de `PathFinding`, una vez que `PathFinding` sea porteado).

Si existe tal referencia cruzada, **debés agregar una nota explícita en la entrada del módulo AFECTADO en `docs/implementation/00-port-plan.md`**, citando el documento donde reside la fundamentación completa. No consideres terminado el trabajo de un módulo hasta que este paso de propagación haya sido verificado y completado explícitamente — ya que un colaborador futuro trabajando en el módulo afectado no tiene motivos para revisar la documentación del módulo de origen.

## Aprendizajes de FileIO — Reglas Proactivas de Porting (Learnings from FileIO)

Durante el porteo del módulo `FileIO.bas`, se consolidaron cinco patrones observados de manera recurrente. Estas directivas quedan establecidas como **reglas proactivas permanentes** para todos los módulos futuros (especialmente para módulos de gran envergadura como el próximo `Protocol.bas`):

### 1. Umbral de Desglose para Módulos Grandes (*Large Module Breakdown Threshold*)
Todo módulo cuya estimación supere aproximadamente las **500-800 líneas**, o que esté marcado como **"Grande"** en [`00-port-plan.md`](implementation/00-port-plan.md), requiere obligatoriamente su propio archivo `<modulo>-breakdown.md` en `docs/implementation/`.
- **Planificación previa obligatoria**: Este desglose debe redactarse y acordarse **ANTES de escribir una sola línea de código C++** para el módulo en cuestión. Debe dividirlo en grupos lógicos e independientemente portables, secuenciados estrictamente por árbol de dependencias internas y nivel de riesgo.
- **Prohibición**: Queda prohibido encarar un módulo grande de manera monolítica y desglosarlo de forma reactiva recién cuando se vuelva inmanejable a mitad del desarrollo.

### 2. Justificar Comportamiento Adicional de Inmediato, No Reactivamente (*Justify Extra Behavior Immediately, Not Reactively*)
Si el código porteado realiza cualquier acción más permisiva, más defensiva, más "servicial" (*helpful*) o que invada el dominio de otro módulo en comparación con lo que produciría una traducción literal línea por línea del código original:
- **Justificación inmediata**: Debe justificarse con una **cita textual y directa del código fuente legacy EN LA MISMA RESPUESTA** en la que se introduce el cambio.
- **Presunción de culpabilidad**: No agregues lógica extra primero para explicarla únicamente si el usuario te la cuestiona. Cualquier agregado no literal se considera **culpable hasta que se demuestre su inocencia mediante una cita precisa del legacy**.

### 3. Nunca Replicar un Crasheo del Legacy como Comportamiento Indefinido en C++ (*Never Replicate a Legacy Crash as C++ Undefined Behavior*)
Cuando el código VB6 original crashearía ante cierta entrada no contemplada (por ejemplo, excepciones de *"Subscript out of range"*, desbordamientos numéricos o errores de discrepancia de tipos / *type mismatch* atrapados incidentalmente por un manejador general `On Error GoTo`):
- **Traducción segura y robusta**: El port a C++ jamás debe reproducir esto como comportamiento indefinido (*Undefined Behavior*), lecturas/escrituras fuera de rango descontroladas o corrupción de memoria.
- **Desviación deliberada documentada**: Debe traducirse a una excepción explícita y tipada o a un camino de error seguro y controlado. Dicha decisión debe señalarse de forma clara y destacada en el documento de implementación correspondiente como una **desviación deliberada por motivos de seguridad (*DELIBERATE safety-motivated deviation*)** respecto al comportamiento literal del legacy, distinguiéndola tajantemente de una invención o descuido accidental.

### 4. El Alcance Global Requiere Investigar Patrones de Acceso Verificados, No una Herencia Automática de VB6 (*Global Scope Requires Verified Access-Pattern Investigation, Not Default Inheritance from VB6*)
En VB6 era habitual declarar variables en `Declares.bas` como globales por conveniencia o limitaciones del lenguaje.
- **Investigación de concurrencia**: Antes de portar cualquier variable global de VB6 como global en C++ (en `Declares.hpp`), investigá y confirmá explícitamente si el modelo de ejecución del servidor permite en algún momento accesos concurrentes o superpuestos a dicha variable.
- **Preferencia por ámbito local**: Si se confirma que el acceso es estrictamente secuencial y de una única entidad a la vez (como ocurre con la cabecera `tCabecera` en `FileIO` o la cola de nodos en `Queue`/`PathFinding`), encapsulala como variable local al ámbito de la(s) función(es) pertinente(s) o estática interna a la unidad de traducción (`.cpp`). Esto preserva la restricción real del comportamiento sin el pasivo técnico de acarrear estado mutable global.
- Documentá siempre la investigación y la decisión tomada (sea local o global) en el documento de implementación del módulo.

### 5. Los Nuevos Datos de Fixtures se Propagan Hacia Atrás, No Solo Hacia Adelante (*New Fixture Data Propagates Backward, Not Just Forward*)
Descubrir datos de validación más sólidos y autoritativos (archivos reales de producción del juego original) para un módulo que previamente solo contaba con validación sobre datos sintéticos constituye en sí mismo un **evento de Propagación de Decisiones Cruzadas (*Cross-Module Decision Propagation*)**.
- **Reapertura y revalidación**: Exige reabrir y revalidar el módulo previamente considerado "completado" contra los nuevos datos reales de producción, garantizando que el comportamiento histórico no se degrade.
- **Actualización de estatus documental**: Debe actualizarse la documentación de implementación del módulo afectado para reflejar el nuevo estatus de verificación alcanzado, en lugar de limitarse a utilizar los fixtures únicamente para los módulos futuros.

## Documentation & Numbering Policy
Numbering schemes across different doc folders may diverge when they serve different purposes, but any divergence must be stated explicitly near the top of the relevant index file, not left implicit.


## Known necessary exception
Networking/concurrency: the legacy server already supports multiple 
simultaneous player connections. Faithfully porting that existing capability 
(not redesigning it) remains a requirement of this port, even though the 
underlying networking library (standalone Asio) is necessarily different from the 
original Winsock implementation.
