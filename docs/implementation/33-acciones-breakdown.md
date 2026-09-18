# Plan de Desglose Modular — Módulo #33: `Acciones`

> **Estado**: En Ejecución  
> **Área**: Capa 9 (Interacción con Mundo y Entidades)  
> **Documentación Relacionada**: [`docs/audit/15c-acciones-detalle.md`](../audit/15c-acciones-detalle.md), [`docs/implementation/00-port-plan.md`](00-port-plan.md), [`docs/CONVENTIONS.md`](../CONVENTIONS.md)  
> **Archivos C++**: `src/server/Acciones.hpp`, `src/server/Acciones.cpp`, `tests/test_acciones.cpp`

---

## 1. Resumen de Objetivos y Arquitectura

El presente documento establece el plan de desglose en **2 fases lógicas** para la transliteración y desacoplamiento del módulo `Acciones.bas` (`legacy/server/Codigo/Acciones.bas`) a C++.

### Principios Fundamentales
1. **Firma y Transliteración 1:1**:
   Se preservan las firmas exactas, nombres, orden y visibilidad de los 5 procedimientos declarados en VB6:
   - `Accion(user_index, map, x, y)`
   - `AccionParaForo(map, x, y, user_index)`
   - `AccionParaPuerta(map, x, y, user_index)`
   - `AccionParaCartel(map, x, y, user_index)`
   - `AccionParaRamita(map, x, y, user_index)`

2. **Callbacks Tipados en PascalCase (`AccionesCallbacks`)**:
   Toda llamada a subsistemas externos (red, usuarios, comercio, banco, foros, basurero) se canaliza mediante la estructura `AccionesCallbacks` con miembros en PascalCase (`WriteConsoleMsg`, `MakeObj`, `IniciarComercioNPC`, etc.).

3. **Salvaguarda de Memoria y Resguardo de Acceso a Matriz (Bug #47)**:
   En `AccionParaPuerta`, las modificaciones sobre la celda occidental `(X - 1, Y)` se protegen con la guarda `if (x > 1)` previa para prevenir accesos fuera de rango o desbordamientos en coordenadas en los bordes del mapa.
   En `Accion`, al consultar puertas multitile adyacentes en `(X + 1, Y)`, `(X + 1, Y + 1)` y `(X, Y + 1)`, se valida explícitamente `InMapBounds` previo al indexado de `MapData`.

4. **Replicación de Quirks (Bugs #48 y #49)**:
   - **Bug #48**: En `AccionParaRamita`, skill $\ge 10$ fuerza `Suerte = 1` ($100\%$ de éxito).
   - **Bug #49**: En `Accion`, se asigna inmediatamente `TargetNPC` o `TargetObj` al detectar entidad en la celda cliqueada, omitiendo validaciones previas de estado o rango.

5. **Integración con Capa 0 (`cGarbage`)**:
   En `AccionParaRamita`, el encendido exitoso de una fogata agrega una instancia `cGarbage{map, x, y}` a la colección global `TrashCollector`.

---

## 2. Desglose Fase por Fase

### Fase G1: Interacciones de Escenario, Puertas, Carteles, Foros y Supervivencia
- Implementación de `AccionParaPuerta`, `AccionParaCartel`, `AccionParaForo` y `AccionParaRamita`.
- Validación de resguardo `X > 1` en mutaciones de puertas.
- Integración de `cGarbage` en la recolección de basura de fogatas.
- Pruebas unitarias doctest para cada tipo de interacción de escenario.

### Fase G2: Despachador Principal de Acciones (`Accion`)
- Implementación de la rutina despachadora `Accion`.
- Lógica de asignación incondicional de target (`TargetNPC` / `TargetObj`).
- Derivación a comerciantes (`Comercia = 1`), banqueros y sacerdotes/revividores.
- Búsqueda de objetos multitile en casilleros adyacentes con validación de límites.
- Pruebas unitarias doctest para la función despachadora global.

---

## 3. Matriz de Cobertura de Tests (`tests/test_acciones.cpp`)

| Función / Característica | Escenarios de Prueba |
| :--- | :--- |
| **`AccionParaPuerta`** | Apertura/cierre, validación de llave, bloqueo de celdas `(X, Y)` y `(X - 1, Y)`, guarda `X = 1`. |
| **`AccionParaCartel`** | Lectura de texto e invocación de `WriteShowSignal`. |
| **`AccionParaForo`** | Rango max 2, consulta de hilos y notificación de formulario. |
| **`AccionParaRamita`** | Zonas seguras y ciudades prohibidas, tirada de skill (Bug #48), encolado de `cGarbage` en `TrashCollector`. |
| **`Accion`** | Targeting inmediato (Bug #49), derivación a NPCs especiales, escaneo de adyacencias multitile. |
