---
area: estructura-del-proyecto
source_files:
  - legacy/client/CODIGO/Application.bas
  - legacy/client/CODIGO/Declares.bas
  - legacy/client/CODIGO/GameIni.bas
  - legacy/client/CODIGO/General.bas
  - legacy/client/CODIGO/TileEngine.bas
  - legacy/server/Codigo/Declares.bas
  - legacy/server/Codigo/FileIO.bas
  - legacy/server/Codigo/GameLogic.bas
  - legacy/server/Codigo/General.bas
tags: [arquitectura, codigo, vb6, modulos]
last_updated: 2026-09-06
---

## Resumen
El sistema original está estructurado en dos aplicaciones independientes: `legacy/client/` y `legacy/server/`. El código fuente reside en `legacy/client/CODIGO/` y `legacy/server/Codigo/`, compuesto por módulos estándar (`.bas`), módulos de clase (`.cls`) y formularios de interfaz gráfica (`.frm`) de Visual Basic 6.

## Hallazgos
La estructura de directorios separa claramente el cliente y los recursos gráficos del servidor dedicado y la base de datos de usuarios.

- **Estructura del Cliente**:
  - `legacy/client/CODIGO/`: Contiene 117 archivos fuente en VB6 (`.bas`, `.cls`, `.frm`, `.frx`).
  > Fuente: `legacy/client/CODIGO/Application.bas`, procedimiento `Main`
  - Carpetas de recursos gráficos, sonidos, mapas y configuraciones (`Graficos/`, `WAV/`, `MIDI/`, `MP3/`, `Mapas/`, `INIT/`).
  > Fuente: `legacy/client/CODIGO/GameIni.bas`, función `LeerGameIni`

- **Estructura del Servidor**:
  - `legacy/server/Codigo/`: Contiene 64 archivos fuente en VB6 (`.bas`, `.cls`, `.frm`).
  > Fuente: `legacy/server/Codigo/General.bas`, procedimiento `Main`
  - Carpetas de persistencia de personajes (`Charfile/`), archivos de mapa binario (`Maps/`), configuraciones estáticas (`Dat/`), clanes (`guilds/`) y logs (`Logs/`).
  > Fuente: `legacy/server/Codigo/FileIO.bas`, procedimiento `DoBackUp`

## Lógica y Datos Extraídos

### Mapeo de Carpetas Principales

| Ruta del Directorio | Descripción y Responsabilidad | Categoría |
| :--- | :--- | :--- |
| `legacy/client/` | Raíz del cliente legacy y sus recursos | Raíz Cliente |
| `legacy/client/CODIGO/` | Código fuente VB6 del cliente (`.bas`, `.cls`, `.frm`) | Código Cliente |
| `legacy/client/Graficos/` | Spritesheets y texturas de tilesets (`.bmp`) | Recursos |
| `legacy/client/INIT/` | Archivos de configuración e índices (`.ini`, `.ind`) | Configuración |
| `legacy/client/Mapas/` | Archivos binarios de mapas del cliente (`.map`) | Mapas |
| `legacy/client/WAV/` | Archivos de efectos de sonido (`.wav`) | Audio |
| `legacy/client/MIDI/` y `legacy/client/MP3/` | Pistas de música ambiental de fondo | Audio |
| `legacy/server/` | Raíz del servidor dedicado legacy | Raíz Servidor |
| `legacy/server/Charfile/` | Perfiles guardados de personajes (`.chr`) | Base de Datos |
| `legacy/server/Codigo/` | Código fuente VB6 del servidor (`.bas`, `.cls`, `.frm`) | Código Servidor |
| `legacy/server/Dat/` | Tablas de datos del servidor (`OBJ.dat`, `NPCs.dat`, `Hechizos.dat`) | Datos |
| `legacy/server/guilds/` | Archivos de datos de clanes (`.guild`, `.inf`, `.mem`) | Clanes |
| `legacy/server/Logs/` | Registros del servidor (`GMs.log`, `Hack.log`, `Errores.log`) | Logs |
| `legacy/server/Maps/` | Archivos binarios de mapas y triggers (`.map`, `.inf`) | Mapas |

## Preguntas Abiertas
- Varios formularios en `legacy/client/CODIGO/` poseen recursos gráficos incrustados en archivos binarios `.frx`; es necesario asegurar que todos los assets visuales estén extraídos a formato PNG estándar para la interfaz en C++.
