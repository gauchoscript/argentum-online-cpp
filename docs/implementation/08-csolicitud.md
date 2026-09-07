---
area: implementacion
status: completed
module: cSolicitud
layer: 0
legacy_source: legacy/server/Codigo/cSolicitud.cls
target_header: src/server/cSolicitud.hpp
target_source: N/A (Header-Only DTO Struct)
test_suite: N/A (DTO sin comportamiento, ver Testing Philosophy)
last_updated: 2026-09-08
---

# Módulo #8: cSolicitud

## Resumen

Se completó el puerto transliterado de la clase `legacy/server/Codigo/cSolicitud.cls` a C++ en `src/server/cSolicitud.hpp`.

En la base de código legacy VB6, `cSolicitud.cls` es un contenedor plano de datos (DTO) sin métodos, propiedades calculadas ni lógica de validación interna. Siguiendo las convenciones de transliteración y la decisión aplicada previamente en `cGarbage`, se implementó como un `struct` C++ puro en `src/server/cSolicitud.hpp`.

## Decisiones de Diseñadores e Investigación

### 1. Elección de `struct` sobre `class`
`cSolicitud.cls` no posee métodos privados ni públicos, validaciones de longitud, ni lógica de negocio. Es un DTO puro. Por ello, se definió como `struct cSolicitud` en lugar de una clase con métodos accessor/mutator artificiales.

### 2. Nombres de Miembros de Datos según la Naming Policy
A pesar de que los archivos de configuración de solicitudes de clan (`<GuildName>-solicitudes.sol`) guardados en disco por `clsClan.cls` utilizan los nombres de clave INI `Nombre` y `Detalle`, la definición original de la clase VB6 `cSolicitud.cls` declara explícitamente los campos:

```vb
Public UserName As String
Public desc As String
```

En estricto cumplimiento de la **Naming Policy** ("Preserve the exact legacy VB6 identifier, if one exists and is a valid C++ identifier"), el struct C++ conserva `UserName` y `desc`:

```cpp
struct cSolicitud {
    std::string UserName;
    std::string desc;
};
```

### 3. Codificación de Cadenas de Texto (ANSI / Windows-1252)
El campo `desc` representa la petición enviada por un jugador al postularse a un clan. Este campo admite texto libre ingresado por el usuario que puede incluir caracteres acentuados o especiales en codificación mono-byte ANSI (Windows-1252). La representación `std::string` en C++ preserva sin modificaciones los octetos de 8 bits leídos o transmitidos.

### 4. Justificación de Omisión de Pruebas Unitarias (`doctest`)
De acuerdo con la **Testing Philosophy** (Filosofía de Pruebas de la Escuela Clásica / Detroit):
- No se deben inventar pruebas unitarias artificiales para estructuras de datos planas (DTO) que carecen de métodos y comportamiento.
- La omisión de `test_csolicitud.cpp` es una **decisión deliberada y fundamentada**, idéntica a la tomada para `cGarbage.hpp`. La verificación del intercambio de datos de solicitudes se difiere a las pruebas de integración de `clsClan` / `modGuilds` contra los archivos de fixture `.sol` en `tests/fixtures/guilds/`.

## Estructura Migrada (`src/server/cSolicitud.hpp`)

```cpp
#pragma once

#include <string>

struct cSolicitud {
    std::string UserName;
    std::string desc;
};
```
