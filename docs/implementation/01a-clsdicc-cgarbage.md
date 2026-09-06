---
area: estructura-del-proyecto
status: in-progress
audit_reference: docs/audit/01a-clsdicc-cgarbage.md
tags: [plan, migracion, cpp, clsdicc, cgarbage, capa-0]
last_updated: 2026-09-06
---

# Especificación de Migración a C++: clsdicc y cGarbage (Capa 0)

Este documento contiene las decisiones de diseño, notas de migración a C++ y requerimientos de implementación para los módulos de Capa 0 **`clsdicc.cls`** y **`cGarbage.cls`**, basándose en los hallazgos de la auditoría [`docs/audit/01a-clsdicc-cgarbage.md`](../audit/01a-clsdicc-cgarbage.md).

---

## Decisiones de Diseño y Notas de Migración

### 1. Módulo `cGarbage` (`cGarbage.cls`)

- **Estructura C++ Plano (`struct`)**: Se implementará como un `struct` C++ plano (`struct cGarbage` o `struct ElementoBasura`) en lugar de una `class`, dado que el código legacy carece por completo de métodos, miembros privados, propiedades calculadas o lógica de destructor; consiste únicamente en tres campos enteros (`map`, `X`, `Y`).
- **Preservación de Nombres y Comentarios Aclaratorios**: Siguiendo la política de nombres de `docs/CONVENTIONS.md`, los identificadores engañosos `cGarbage` y `TrashCollector` se preservarán tal cual en C++, priorizando la familiaridad para los desarrolladores que conocen la base de código legacy. No obstante, se debe agregar obligatoriamente un comentario descriptivo en la definición del `struct` en C++ explicando su propósito real (registro de objetos temporales del mapa como fogatas encendidas por jugadores), evitando que sea redescubierto con confusión en el futuro.
- **Referencia Cruzada de Dependencias Operativas**: El comportamiento real de esta estructura no es autosuficiente y está acoplado con otros tres módulos clave del servidor:
  - **Declaración Global**: La colección global `TrashCollector` declarada en `Declares.bas`.
  - **Lógica de Creación/Encolado**: La habilidad de *Supervivencia* para encender fogatas en `Acciones.bas` (`CrearFuego`).
  - **Lógica de Limpieza/Descolado**: El procedimiento de mantenimiento periódico `LimpiarMundo` en `General.bas`.

### 2. Módulo `clsdicc` (`clsdicc.cls` / `diccionario`)

- **Sensibilidad a Mayúsculas/Minúsculas en Búsqueda y Hasheado**: Reemplazar directamente `clsdicc` por un `std::unordered_map<std::string, ...>` nativo rompería el comportamiento en forma silenciosa. El código legacy normaliza las claves mediante `UCase$` tanto en inserciones como en búsquedas, mientras que la función de hash por defecto de `std::string` en C++ es sensible a mayúsculas y minúsculas (*case-sensitive*). El adaptador en C++ debe normalizar la clave a mayúsculas antes de cada inserción/búsqueda o utilizar funciones de hash y comparación custom *case-insensitive* para mantener un comportamiento idéntico.
- **Límite Duro de 100 Elementos (Desviación Intencional Registrada)**: La implementación legacy posee una restricción fija de 100 elementos (`MAX_ELEM = 100`). Se establece explícitamente la decisión de **no replicar este límite de 100 elementos en el port a C++**, al tratarse de un detalle interno de implementación de VB6 para evitar memoria dinámica que no tiene un efecto de juego observable. Esta omisión queda registrada formalmente como una desviación intencional y planificada, no como un descuido.

---

## Archivos de Código Relacionados en C++

- `src/server/cGarbage.hpp` / `src/server/cGarbage.cpp`
- `src/server/clsdicc.hpp` / `src/server/clsdicc.cpp`
