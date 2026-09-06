---
area: plan-de-port
status: in-progress
audit_reference: docs/audit/01-estructura-del-proyecto.md
tags: [plan, arquitectura, port, dependencias, modulos]
last_updated: 2026-09-06
---

# Plan de Port Secuencial — Argentum Online v0.13.0 (VB6 a C++)

Este documento define el orden de migración módulo por módulo a partir de los módulos identificados en la auditoría de estructura ([docs/audit/01-estructura-del-proyecto.md](file:///C:/Users/Elio/Documents/ArgentumOnline0.13.0/docs/audit/01-estructura-del-proyecto.md)). El objetivo es avanzar en una secuencia estricta basada en dependencias: los módulos base o sin dependencias se portan primero, de modo que cada paso subsiguiente sólo dependa de código ya migrado.

Todas las convenciones de nombres y estructura siguen estrictamente las políticas establecidas en [docs/CONVENTIONS.md](file:///C:/Users/Elio/Documents/ArgentumOnline0.13.0/docs/CONVENTIONS.md).

---

## 1. Orden de Port por Dependencias — Servidor (`server`)

A continuación se listan los módulos del servidor identificados en la auditoría, ordenados de menor a mayor nivel de dependencia:

### 1.1. `Declares`
- **Archivo Legacy**: `legacy/server/Codigo/Declares.bas`
- **Qué hace (según auditoría/código)**: Contiene todas las estructuras de datos globales, tipos definidos por el usuario (`User`, `WorldPos`, `Item`, `tHechizo`), constantes globales del mundo y límites del motor (`MaxUsers`, `MAXMAPS`).
- **Archivo Nuevo Propuesto**: `src/server/Declares.h` (y `src/server/Declares.cpp` si se requieren definiciones de variables globales/instancias).
- **Dependencias**: *Ninguna* dentro del código del juego. Es la base sobre la que se apoyan todos los demás módulos.
- **Estimación de Tamaño y Complejidad**: **Mediano** (~1.594 líneas). Aunque no contiene lógica algorítmica compleja, posee una gran cantidad de structs y enums que definen el estado en memoria de todo el servidor.

### 1.2. `FileIO`
- **Archivo Legacy**: `legacy/server/Codigo/FileIO.bas`
- **Qué hace (según auditoría/código)**: Responsable de la persistencia de datos: lectura y escritura de personajes (`Charfile/` en formato `.chr`), carga de mapas binarios (`Maps/` en formato `.map` e `.inf`), tablas de datos (`Dat/` como `OBJ.dat`, `NPCs.dat`), clanes (`guilds/`), logs (`Logs/`) y el procedimiento de backup `DoBackUp`.
- **Archivo Nuevo Propuesto**: `src/server/FileIO.h` y `src/server/FileIO.cpp`.
- **Dependencias**: Depende directamente de `Declares` (necesita las definiciones de `User`, `ObjData`, `MapData`, etc. para mapear los buffers de disco a memoria).
- **Estimación de Tamaño y Complejidad**: **Grande** (~2.246 líneas). Implica manejo de streams binarios, parseo de archivos de texto/INI y compatibilidad binaria exacta con los mapas y datos legacy.

### 1.3. `GameLogic`
- **Archivo Legacy**: `legacy/server/Codigo/GameLogic.bas`
- **Qué hace (según auditoría/código)**: Controla las reglas centrales del mundo, la actualización de estados del juego, validaciones espaciales y de interacción entre entidades en cada tick del motor.
- **Archivo Nuevo Propuesto**: `src/server/GameLogic.h` y `src/server/GameLogic.cpp`.
- **Dependencias**: Depende de `Declares` (estructuras y estado global) y de `FileIO` (para acceder a recursos y datos cargados).
- **Estimación de Tamaño y Complejidad**: **Mediano** (~1.063 líneas). Lógica de juego secuencial pura, alta cantidad de ramas condicionales que deben replicar exactamente la matemática y comportamiento de VB6.

### 1.4. `General`
- **Archivo Legacy**: `legacy/server/Codigo/General.bas`
- **Qué hace (según auditoría/código)**: Punto de entrada del servidor dedicado (procedimiento `Main`), bootstrap de la aplicación, inicialización de subsistemas, bucle principal de ejecución y secuencias de apagado (`Shutdown`).
- **Archivo Nuevo Propuesto**: `src/server/General.h` y `src/server/General.cpp` (reemplaza o complementa a `src/server/main.cpp`).
- **Dependencias**: Depende de `Declares`, `FileIO` y `GameLogic` (coordina la carga inicial mediante `FileIO`, prepara las estructuras de `Declares` y orquesta el loop con `GameLogic`).
- **Estimación de Tamaño y Complejidad**: **Mediano - Grande** (~1.467 líneas). Coordina todo el ciclo de vida del servidor y la sincronización del loop principal.

---

## 2. Orden de Port por Dependencias — Cliente (`client`)

A continuación se listan los módulos del cliente identificados en la auditoría, ordenados por nivel de dependencia:

### 2.1. `Declares`
- **Archivo Legacy**: `legacy/client/CODIGO/Declares.bas`
- **Qué hace (según auditoría/código)**: Define estructuras de renderizado (`Grh`, `GrhData`), posiciones (`Position`), cuerpos (`BodyData`), cabezas, armas, y constantes del cliente gráfico.
- **Archivo Nuevo Propuesto**: `src/client/Declares.h` (y `src/client/Declares.cpp` de ser necesario).
- **Dependencias**: *Ninguna*. Estructura base del cliente.
- **Estimación de Tamaño y Complejidad**: **Chico - Mediano** (~829 líneas). Principalmente definiciones de tipos y estructuras gráficas.

### 2.2. `GameIni`
- **Archivo Legacy**: `legacy/client/CODIGO/GameIni.bas`
- **Qué hace (según auditoría/código)**: Contiene la función `LeerGameIni` encargada de leer la configuración inicial (`INIT/Inicio.ini`, `Config.ini`) y fijar los directorios de recursos (`Graficos/`, `WAV/`, `Mapas/`).
- **Archivo Nuevo Propuesto**: `src/client/GameIni.h` y `src/client/GameIni.cpp`.
- **Dependencias**: Depende de `Declares` (para asignar los valores leídos a las variables globales).
- **Estimación de Tamaño y Complejidad**: **Chico** (~98 líneas). Parseo básico de archivos de configuración INI.

### 2.3. `TileEngine`
- **Archivo Legacy**: `legacy/client/CODIGO/TileEngine.bas`
- **Qué hace (según auditoría/código)**: Motor de renderizado 2D de tiles, capas de mapa, personajes, animaciones y gestión de pantalla (originalmente usando DirectDraw 7, sustituido por SFML en C++).
- **Archivo Nuevo Propuesto**: `src/client/TileEngine.h` y `src/client/TileEngine.cpp`.
- **Dependencias**: Depende de `Declares` y `GameIni` (utiliza las estructuras de gráficos y las rutas de texturas leídas al inicio).
- **Estimación de Tamaño y Complejidad**: **Grande** (~2.069 líneas). Alta complejidad por el cambio de API de renderizado (DirectDraw -> SFML) preservando el orden exacto de capas y el timing de cuadros de animación.

### 2.4. `General`
- **Archivo Legacy**: `legacy/client/CODIGO/General.bas`
- **Qué hace (según auditoría/código)**: Módulo general de rutinas del cliente, manejo del bucle principal de juego, procesamiento de eventos de entrada y actualización del estado del cliente.
- **Archivo Nuevo Propuesto**: `src/client/General.h` y `src/client/General.cpp`.
- **Dependencias**: Depende de `Declares`, `GameIni` y `TileEngine`.
- **Estimación de Tamaño y Complejidad**: **Mediano** (~1.335 líneas). Manejo de estados de la aplicación, timers y flujo general.

### 2.5. `Application`
- **Archivo Legacy**: `legacy/client/CODIGO/Application.bas`
- **Qué hace (según auditoría/código)**: Procedimiento `Main` de arranque del cliente, control de inicio de la aplicación y configuración de arranque previa a la apertura de las pantallas.
- **Archivo Nuevo Propuesto**: `src/client/Application.h` y `src/client/Application.cpp` (vinculado con `src/client/main.cpp`).
- **Dependencias**: Depende de `General`, `GameIni` y `Declares`.
- **Estimación de Tamaño y Complejidad**: **Chico** (~35 líneas). Bootstrap simple del proceso cliente.

---

## 3. Señalamientos Explícitos Críticos (Flags)

> [!WARNING]
> ### FLAG 1: Módulos no cubiertos o parcialmente cubiertos por la auditoría de estructura
> 
> La auditoría en `docs/audit/01-estructura-del-proyecto.md` se limitó a un inventario general y solo listó **9 archivos fuente explícitos** de los **181 archivos totales** que componen el proyecto legacy (64 en el servidor y 117 en el cliente):
> 
> 1. **Falta de cobertura profunda en los 9 módulos listados**:
>    - La auditoría solo citó procedimientos puntuales (`Main`, `LeerGameIni`, `DoBackUp`) sin desglosar el mapa completo de llamadas ni sus dependencias cruzadas con otros módulos no listados.
>    - `FileIO.bas`, por ejemplo, interactúa íntimamente con módulos de clanes (`modGuilds.bas`), facciones y personajes que no están mencionados en la auditoría 01.
> 
> 2. **Módulos esenciales completamente omitidos en la auditoría 01**:
>    - **Servidor (60 archivos sin cubrir en la auditoría 01)**:
>      - *Red y protocolo*: `TCP.bas`, `wsksock.bas`, `wskapiAO.bas`, `modSendData.bas`, `clsByteQueue.cls`, `Protocol.bas`.
>      - *Gestión de usuarios y entidades*: `Modulo_UsUaRiOs.bas`, `MODULO_NPCs.bas`, `AI_NPC.bas`, `Characters.bas`.
>      - *Sistemas de juego*: `SistemaCombate.bas`, `InvUsuario.bas`, `Trabajo.bas`, `modHechizos.bas`, `modBanco.bas`, `ModAreas.bas`, `PathFinding.bas`, `modGuilds.bas`, `clsClan.cls`, `mdParty.bas`, `clsParty.cls`.
>      - *Seguridad e infraestructura*: `SecurityIp.bas`, `modCentinela.bas`, `clsIniReader.cls`, `modNuevoTimer.bas`.
>      - *Formularios de control y monitoreo*: `frmServidor.frm`, `frmMain.frm`, `frmAdmin.frm`, `frmUserList.frm`.
>    - **Cliente (112 archivos sin cubrir en la auditoría 01)**:
>      - *Red y protocolo*: `TCP.bas`, `Protocol.bas`, `ProtocolCmdParse.bas`, `clsByteQueue.cls`.
>      - *Multimedia*: `clsAudio.cls`, `clsSurfaceManager.cls`, `clsSurfaceManStatic.cls`, `clsSurfaceManDyn.cls`, `cDIBSection.cls`.
>      - *Interfaz y formularios*: Más de 45 formularios `.frm` y archivos binarios `.frx` (`frmConnect.frm`, `frmCrearPersonaje.frm`, `frmMain.frm`, `frmComerciar.frm`, etc.).
>      - *Mapeo de controles y entrada*: `clsCustomKeys.cls`, `clsGraphicalButton.cls`, `clsGrapchicalInventory.cls`, `clsDialogs.cls`.
> 
> **Acción requerida**: No se debe portar "a ciegas" ninguno de estos subsistemas sin antes consultar sus auditorías temáticas específicas (e.g. `02-protocolo-de-red.md`, `04-formulas-de-combate.md`, `06-formatos-de-datos.md`, `07-pantallas-e-interfaz.md`) o realizar una inspección directa del archivo `.bas`/`.cls` correspondiente.

---

> [!IMPORTANT]
> ### FLAG 2: Módulos responsables de la concurrencia y conexiones múltiples simultáneas
> 
> De acuerdo con las convenciones de [docs/CONVENTIONS.md](file:///C:/Users/Elio/Documents/ArgentumOnline0.13.0/docs/CONVENTIONS.md), **la capacidad del servidor de soportar múltiples conexiones simultáneas de jugadores debe ser preservada fielmente**, aun cuando la biblioteca de red subyacente cambie de Winsock a Asio.
> 
> Los módulos legacy responsables de esta arquitectura son:
> 
> 1. **`legacy/server/Codigo/TCP.bas`**:
>    - Administra la tabla global de usuarios conectados `UserList(1 To MaxUsers)`.
>    - Asigna un `UserIndex` disponible en `AcceptNewUser` ante cada conexión entrante.
>    - Limpia la sesión y libera recursos ante desconexiones en `CloseSocket`.
>    - Monitorea caídas o timeouts periódicos mediante `SendCheckTimeout`.
> 
> 2. **`legacy/server/Codigo/wsksock.bas` y `wskapiAO.bas`**:
>    - Implementa el procedimiento de ventana de Windows (`SocketWindowProc`) que recibe notificaciones asincrónicas de red multiplexadas (`FD_ACCEPT`, `FD_READ`, `FD_WRITE`, `FD_CLOSE`) para cada descriptor de socket.
>    - En C++, esta arquitectura multiplexada monocapa de Windows se traslada directamente al bucle asincrónico de eventos de **Asio** (`asio::io_context`, `asio::ip::tcp::acceptor` y sesiones `asio::ip::tcp::socket`), manteniendo el manejo individual de cada sesión atada a su respectivo `UserIndex`.
> 
> 3. **`legacy/server/Codigo/clsByteQueue.cls`**:
>    - Cada conexión en `UserList(UserIndex)` dispone de dos colas de bytes dedicadas (`incomingData` y `outgoingData`).
>    - Permite recibir fragmentos de stream TCP concurrentes sin mezclar paquetes entre distintos usuarios.
> 
> 4. **`legacy/server/Codigo/modSendData.bas`**:
>    - Modulo de despacho y broadcast concurrente (`SendData`, `SendToUserArea`, `SendToAll`). Distribuye ráfagas de paquetes a múltiples clientes simultáneamente basándose en la visibilidad de mapa/área.
> 
> 5. **`legacy/server/Codigo/SecurityIp.bas`**:
>    - Control de concurrencia por IP (`MaxConnectionsPerIP`) para mitigar ataques de denegación de servicio o saturación de slots.
> 
> 6. **`legacy/server/Codigo/Modulo_UsUaRiOs.bas`**:
>    - Orquesta el ciclo de vida del usuario dentro de la sesión activa (`Cerrar_Usuario`, desconexión limpia, persistencia de estado concurrente).
