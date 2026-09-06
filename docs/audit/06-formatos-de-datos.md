---
area: formatos-de-datos
source_files:
  - legacy/server/Codigo/FileIO.bas
  - legacy/server/Codigo/General.bas
  - legacy/server/Codigo/ModMapIO.bas
  - legacy/client/CODIGO/TileEngine.bas
tags: [datos, formatos, ini, chr, map, dat, binario]
last_updated: 2026-09-06
---

## Resumen
Argentum Online v0.13.0 utiliza una combinación de archivos de texto con formato INI para la persistencia de usuarios (`.chr`) y configuración del mundo (`.dat`), junto con archivos binarios propietarios para las definiciones de mapas (`.map`).

## Hallazgos

- **Archivos de Personaje (`.chr`)**:
  - Se almacenan individualmente en la carpeta `legacy/server/Charfile/<NOMBRE>.chr`.
  - Estructura basada en INI dividida en secciones: `[INIT]` (contraseña, cuerpo, cabeza, email), `[STATS]` (nivel, exp, hp, mana, atributos), `[INVENTORY]` (slots de objetos y cantidades), `[SPELLS]` (hechizos aprendidos), `[FLAGS]` (estado criminal, ban, banip).
  > Fuente: `legacy/server/Codigo/FileIO.bas`, procedimientos `SaveUser`, `LoadUser`

- **Tablas de Configuración Estática (`.dat`)**:
  - `legacy/server/Dat/OBJ.dat`: Definición de todos los ítems (armas, armaduras, pociones).
  - `legacy/server/Dat/NPCs.dat`: Definición de criaturas y comerciantes (HP, experiencia otorgada, loot drops, gráficos).
  - `legacy/server/Dat/Hechizos.dat`: Lista de hechizos del juego (requerimiento de maná, nivel, efecto visual, daño/curación).
  > Fuente: `legacy/server/Codigo/General.bas`, procedimientos `CargarOBJ`, `CargarNPCs`, `CargarHechizos`

- **Archivos Binarios de Mapas (`.map`)**:
  - Se ubican en `legacy/server/Maps/Mapa<N>.map` y `legacy/client/Mapas/Mapa<N>.map`.
  - Contienen el encabezado de versión de mapa y una matriz de 100x100 tiles. Cada tile contiene la bandera de bloqueo (`Blocked`), capas gráficas 1 a 4, triggers y spawn points.
  > Fuente: `legacy/server/Codigo/ModMapIO.bas`, función `LoadMap`<br>Fuente: `legacy/client/CODIGO/TileEngine.bas`, función `CargarMapa`

## Lógica y Datos Extraídos

### Resumen de Formatos de Datos

| Tipo de Dato | Extensión | Ubicación | Formato Interno | Funciones de Lectura / Escritura |
| :--- | :--- | :--- | :--- | :--- |
| **Personaje** | `.chr` | `legacy/server/Charfile/` | Texto INI (`WritePrivateProfileString`) | `SaveUser`, `LoadUser` (`FileIO.bas`) |
| **Objetos** | `.dat` | `legacy/server/Dat/OBJ.dat` | Texto INI (`[OBJ<N>]`) | `CargarOBJ` (`General.bas`) |
| **NPCs** | `.dat` | `legacy/server/Dat/NPCs.dat` | Texto INI (`[NPC<N>]`) | `CargarNPCs` (`General.bas`) |
| **Hechizos** | `.dat` | `legacy/server/Dat/Hechizos.dat` | Texto INI (`[HECHIZO<N>]`) | `CargarHechizos` (`General.bas`) |
| **Mapas** | `.map` | `legacy/server/Maps/` / `legacy/client/Mapas/` | Stream Binario (100x100 tiles) | `LoadMap` (`ModMapIO.bas`), `CargarMapa` (`TileEngine.bas`) |
| **Clanes** | `.guild`, `.inf` | `legacy/server/guilds/` | Texto INI | `CargarGuildas` (`ModoGuilds.bas`) |

## Preguntas Abiertas
- La migración a C++ debe mantener 100% de compatibilidad binaria con los archivos `.map` e INI existentes para permitir la reutilización de mapas y recursos legacy.
