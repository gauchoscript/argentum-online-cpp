---
area: estructura-del-proyecto
status: completed
audit_reference: docs/audit/01a-clsdicc-cgarbage.md
tags: [documentacion, port, cpp, clsdicc, diccionario, capa-0]
last_updated: 2026-09-07
---

# Documentación de Implementación: clsdicc (diccionario)

Este documento describe la arquitectura C++, decisiones de diseño y comportamiento verificado del módulo de Capa 0 **`clsdicc`**, correspondiente a la clase legacy VB6 `legacy/server/Codigo/clsdicc.cls`.

---

## Resumen del Módulo

`clsdicc` es una estructura asociativa clave-valor adaptada a C++ mediante `std::unordered_map` y `std::vector` en `src/server/clsdicc.hpp` y `src/server/clsdicc.cpp`. Su propósito principal en el servidor legacy es acumular votos durante las elecciones internas de líderes de clan (`clsClan.cls`).

---

## Decisiones de Diseño (Design Decisions)

### 1. Normalización de Claves a Mayúsculas (*Case-Insensitive*)
En la implementación VB6, todas las claves se pasan por `UCase$` al insertar (`AtPut`) y buscar (`At`). En C++, el adaptador realiza esta normalización a mayúsculas implícitamente antes de operar sobre `std::unordered_map`. Esto garantiza que búsquedas con `"perez"`, `"PEREZ"` o `"PeReZ"` accedan a la misma entrada.

### 2. Eliminación del Cap de 100 Elementos (*Desviación Intencional Documentada*)
El código legacy VB6 definía una constante `MAX_ELEM = 100` sobre un arreglo estático. En C++ se omitió este límite arbitrario, permitiendo un crecimiento dinámico según los elementos agregados. Esta decisión fue acordada y documentada previamente en [`docs/audit/01a-clsdicc-cgarbage.md`](../audit/01a-clsdicc-cgarbage.md).

### 3. Preservación del Orden de Inserción y Comportamiento de Empates en `MayorValor`
> [!IMPORTANT]
> **Hallazgo Crítico de Comportamiento Legacy en Empates**:
> El método `MayorValor(cant)` en VB6 recorre la estructura en su **orden de inserción original**. En caso de que múltiples claves compartan la puntuación o cantidad de votos máxima:
> 1. **Concatena TODAS las claves empatadas** separadas por comas (ej. `"PEREZ,GOMEZ,LOPEZ"`).
> 2. **Conserva el orden de inserción** (NO se ordenan alfabéticamente).
> 3. **Asigna la cantidad total de empatados al parámetro de salida `cant`** (pasado por referencia).
> 4. **Retorna las claves en MAYÚSCULAS**.
>
> Para soportar este requerimiento de compatibilidad sin perder el acceso $O(1)$, la implementación en C++ almacena un `std::vector<std::string> m_order` con las claves únicas en su orden de inserción junto al map asociativo.

---

## Interfaz Pública en C++ (`src/server/clsdicc.hpp`)

```cpp
class clsdicc {
public:
    clsdicc() = default;
    ~clsdicc() = default;

    std::int16_t CantElem() const;
    bool AtPut(const std::string& clave, const std::string& elem);
    bool AtPut(const std::string& clave, int elem);
    std::string At(const std::string& clave) const;
    std::string AtIndex(int i) const;
    std::string MayorValor(int& cant) const;
    void DumpAll();
};
```

---

## Pruebas de Verificación (doctest)

Las pruebas unitarias se encuentran en `tests/test_clsdicc.cpp` y atienden a la Filosofía de Testing (`docs/CONVENTIONS.md`):
- **MayorValor**: Casos sin empate, empates de 2 claves, empates de 4 claves, y verificación de que el orden en el string devuelto sigue el orden de inserción y no el orden alfabético.
- **Normalización Case-Insensitive**: Verificación de inserción y actualización con diferente casing.
- **Casos Borde**: Comportamiento con diccionario vacío, consulta `AtIndex` fuera de rango 1-based, claves vacías y vaciado total con `DumpAll`.
