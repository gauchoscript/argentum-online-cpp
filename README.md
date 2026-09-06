# Argentum Online (AO v0.13.0) — Proyecto de Migración a C++

## Descripción del Proyecto

Este repositorio alberga el proyecto de reescritura y migración a C++ moderno para **Argentum Online (v0.13.0)**, el clásico MMORPG 2D creado originalmente en Visual Basic 6 (VB6).

### Objetivo Principal
El objetivo central de este proyecto es portar tanto el servidor dedicado como el cliente desde el código legacy en VB6 hacia C++ moderno y multiplataforma, **manteniendo 100% de compatibilidad a nivel de protocolo binario y formatos de datos** con el sistema original.

---

## Principios Arquitectónicos y Metas de Compatibilidad

1. **Interoperabilidad de Protocolo**:
   - El nuevo servidor en C++ DEBE aceptar conexiones entrantes de clientes legacy en VB6 sin ninguna modificación.
   - El nuevo cliente en C++ DEBE conectarse y operar en servidores legacy en VB6 de forma transparente.
   - Se debe preservar estrictamente el orden de serialización binaria, los opcodes (`ClientPacketID` y `ServerPacketID`) y el empaquetado de datos.

2. **Compatibilidad de Datos y Mundo**:
   - **Mapas Binarios**: Soporte completo para lectura y escritura de archivos binarios de mapas de 100x100 tiles (`.map` e `.inf`).
   - **Bases de Datos DAT**: Compatibilidad total con los archivos `.dat` (`OBJ.dat`, `NPCs.dat`, `Hechizos.dat`, `Balance.dat`).
   - **Persistencia de Personajes**: Compatibilidad nativa para leer y grabar perfiles de personajes en formato INI (`.chr`).

3. **Modernización Interna del Engine**:
   - Arquitectura de servidor multihilo de alto rendimiento que reemplaza los timers legacy monocapa de VB6 (`VB.Timer`).
   - Backends gráficos y de audio modernos y multiplataforma que reemplazan las interfaces obsoletas de DirectX 7 (DirectDraw 7, DirectSound 7, DirectMusic 7).

---

## Estructura del Repositorio

```
ArgentumOnline0.13.0/
├── CMakeLists.txt          # Configuración del sistema de construcción CMake
├── vcpkg.json              # Manifiesto de dependencias C++ (Asio, SFML)
├── README.md               # Descripción principal del proyecto y guía de compilación
├── legacy/                 # Código fuente e información original de VB6 (Solo Lectura)
├── docs/                   # Hub de documentación de la migración
└── src/                    # Código fuente moderno en C++
    ├── client/             # Pruebas de concepto y código del cliente (`client_poc`)
    └── server/             # Pruebas de concepto y código del servidor (`server_poc`)
```

---

## Guía de Compilación e Instalación (Build & Setup Guide)

### Requisitos Previos
- Compilador de C++ con soporte C++17 o superior (MSVC / GCC / Clang)
- [CMake](https://cmake.org/) (versión 3.20 o superior)
- [vcpkg](https://github.com/microsoft/vcpkg) (gestor de paquetes C++)

### Paso 1: Clonar e Inicializar vcpkg
Si no tienes `vcpkg` instalado, clonalo e inicialízalo ejecutando:

```cmd
git clone https://github.com/microsoft/vcpkg.git C:\vcpkg
C:\vcpkg\bootstrap-vcpkg.bat
```

### Paso 2: Configurar el Proyecto con CMake y vcpkg
Ejecuta el comando de configuración desde la raíz del repositorio. CMake utilizará el manifiesto `vcpkg.json` para descargar e integrar automáticamente `asio` y `sfml`:

```cmd
cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake
```

### Paso 3: Compilar los Ejecutables
Compila las metas `server_poc` y `client_poc`:

```cmd
cmake --build build --config Release
```

### Paso 4: Ejecutar las Pruebas de Concepto (PoC)

**Ejecutar el Servidor (`server_poc`):**
```cmd
.\build\server_poc.exe
# En generadores multiconfiguración (ej. MSVC):
# .\build\Release\server_poc.exe
```
*Salida esperada:*
`Server skeleton OK`

**Ejecutar el Cliente (`client_poc`):**
```cmd
.\build\client_poc.exe
# En generadores multiconfiguración (ej. MSVC):
# .\build\Release\client_poc.exe
```
*Salida esperada:*
Abre una ventana SFML 3.0.2 de 800x600 ("AO Migration - Render POC") con una forma geométrica renderizada en el centro. Al cerrar con la **X** o la tecla **Escape**, imprime en consola:
`Window closed cleanly`


---

## Hub de Documentación

- 📖 **[Índice de Auditoría del Código](docs/audit/README.md)**: Análisis detallado de opcodes, fórmulas de combate (`CalcularDaño`), IA de NPCs, loops de juego y estructuras de datos de VB6.
- 🛠️ **[Hoja de Ruta de Implementación en C++](docs/implementation/README.md)**: Especificaciones de diseño moderno, selección de librerías y checklists de avance por subsistema.

---

## Licencia y Créditos

Argentum Online es un desarrollo de código abierto bajo la licencia **Affero General Public License (AGPL)**.
Código original VB6 Copyright (C) 2002 Pablo Ignacio Márquez, Aaron Perkins, Otto Perez, Matías Fernando Pequeño.
