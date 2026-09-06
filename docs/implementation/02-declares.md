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

# Módulo Declares

## Resumen

Se completó la migración integral de `legacy/server/Codigo/Declares.bas` a un único par de archivos `src/server/Declares.hpp` y `src/server/Declares.cpp`.

La investigación confirmó que `Declares.bas` en el código fuente VB6 original es un módulo exclusivamente declarativo (no contiene cuerpos ejecutables de `Sub` o `Function`). Por lo tanto, no posee dependencias ejecutables reales de `Matematicas` ni de `clsIniReader`, lo que permitió moverlo a la Capa 0 como módulo base declarativo.

## Estructura Porteada

- **Constantes**: Todas las constantes `Public Const` fueron migradas como `constexpr` o `const` preservando sus valores numéricos y cadenas exactas.
- **Enumeraciones**: Migradas como `enum` C++ tradicionales para mantener la compatibilidad con el código legacy que utiliza enums como índices de arreglos (`UserSkills`, `UserAtributos`) o máscaras de bits (`PlayerType`, `eNickColor`).
- **Estructuras (UDTs)**: Todos los `Type...End Type` fueron migrados como `struct` preservando el orden exacto y los nombres de sus miembros.
- **Módulos de Clase y Punteros**:
  - `outgoingData` e `incomingData` en `User` y las globales `aDos`, `aClon`, `Ayuda`, `Parties`, `ConsultaPopular` y `SonidosMapas` utilizan `std::unique_ptr` con definiciones base livianas para garantizar la compilación limpia e independencia de módulos no porteados aún.
- **API Win32**: Mantenida vía `<windows.h>` con fallbacks multiplataforma portables.

## Verificación

El módulo fue incorporado a la librería `server_core` en `CMakeLists.txt` y verificado mediante compilación 100% limpia sin errores ni advertencias de símbolos incompletos.
