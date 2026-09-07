---
area: estructura-del-proyecto
status: partial
audit_reference: docs/audit/01a-clsdicc-cgarbage.md
tags: [documentacion, port, cpp, cgarbage, capa-0]
last_updated: 2026-09-07
---

# Documentación de Implementación: cGarbage

Este documento describe la arquitectura C++, decisiones de diseño y estado de verificación del módulo de Capa 0 **`cGarbage`**, correspondiente a la clase legacy VB6 `legacy/server/Codigo/cGarbage.cls`.

---

## Estado del Módulo: Parcial (Partial)

> [!IMPORTANT]
> **Verificación Diferida**:
> Este módulo se encuentra en estado **parcial** (*partial*). De acuerdo a la Filosofía de Pruebas (*Testing Philosophy*) definida en [`docs/CONVENTIONS.md`](../CONVENTIONS.md), no se han escrito pruebas unitarias especulativas o artificiales para este struct, dado que un DTO plano sin lógica ni métodos carece de comportamiento aislado susceptible de ser probado significativamente sin un consumidor real.
>
> El comportamiento operativo real asociado a la estructura `cGarbage` y su colección global `TrashCollector` reside en dos módulos aún no porteados:
> 1. **`Acciones.bas`**: Instanciación y encolado al encender fogatas con la habilidad de Supervivencia (`CrearFuego`).
> 2. **`General.bas`**: Descolado y eliminación de objetos del mapa en el procedimiento de mantenimiento periódico (`LimpiarMundo`).
>
> La cobertura de pruebas de integración para este struct queda explícitamente adeudada para cuando se porten dichos módulos (ver sus posiciones agendadas en [`docs/implementation/00-port-plan.md`](00-port-plan.md)).

---

## Decisiones de Diseño (Design Decisions)

### 1. Representación como `struct` C++ Plano (sin Métodos)
En el código legacy VB6, `cGarbage.cls` era un módulo de clase minimalista sin métodos, constructores ni destructores, exponiendo únicamente tres variables públicas (`map`, `X`, `Y`). En C++, se traslada como un `struct` plano (`struct cGarbage`) definido en `src/server/cGarbage.hpp`.

### 2. Preservación de Nomenclatura Legacy (`cGarbage` / `TrashCollector`) y Aclaración de Propósito
De acuerdo con la Política de Nombres de [`docs/CONVENTIONS.md`](../CONVENTIONS.md), se preservan los identificadores engañosos `cGarbage` y `TrashCollector`. A pesar de lo que sugiere el término (que podría confundirse con un recolector de basura de memoria o gestor de referencias circulares), su verdadero propósito en Argentum Online es registrar objetos temporales del mundo (específicamente fogatas encendidas por personajes) para su posterior limpieza periódica. Se incluyó un comentario prominente en el archivo de cabecera C++ explicando esta realidad para evitar que sea redescubierto con confusión en el futuro.

### 3. Integración en el Estado Global (`Declares.hpp`)
La colección global `TrashCollector` de VB6 se declara como `extern std::vector<cGarbage> TrashCollector;` en `src/server/Declares.hpp` e instanciada en `src/server/Declares.cpp`.

---

## Interfaz Pública en C++ (`src/server/cGarbage.hpp`)

```cpp
struct cGarbage {
    std::int16_t map{0};
    std::int16_t X{0};
    std::int16_t Y{0};
};
```
