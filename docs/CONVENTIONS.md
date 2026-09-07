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

## Documentation & Numbering Policy
Numbering schemes across different doc folders may diverge when they serve different purposes, but any divergence must be stated explicitly near the top of the relevant index file, not left implicit.


## Known necessary exception
Networking/concurrency: the legacy server already supports multiple 
simultaneous player connections. Faithfully porting that existing capability 
(not redesigning it) remains a requirement of this port, even though the 
underlying networking library (standalone Asio) is necessarily different from the 
original Winsock implementation.
