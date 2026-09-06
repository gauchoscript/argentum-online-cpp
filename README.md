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
├── README.md               # Descripción principal del proyecto y metas de migración
├── legacy/                 # Código fuente e información original de VB6 (Solo Lectura)
│   ├── client/             # Código fuente del cliente VB6 (CODIGO) y recursos gráficos/audio
│   └── server/             # Código fuente del servidor dedicado VB6 (Codigo) y base de datos
├── docs/                   # Hub de documentación de la migración
│   ├── audit/              # Registro de auditoría histórica y fórmulas extraídas de VB6
│   └── implementation/     # Decisiones de diseño en C++, especificaciones y checklists
├── client/                 # (Próximamente) Cliente moderno en C++
└── server/                 # (Próximamente) Servidor dedicado moderno en C++
```

---

## Hub de Documentación

- 📖 **[Índice de Auditoría del Código](docs/audit/README.md)**: Análisis detallado de opcodes, fórmulas de combate (`CalcularDaño`), IA de NPCs, loops de juego y estructuras de datos de VB6.
- 🛠️ **[Hoja de Ruta de Implementación en C++](docs/implementation/README.md)**: Especificaciones de diseño moderno, selección de librerías y checklists de avance por subsistema.

---

## Licencia y Créditos

Argentum Online es un desarrollo de código abierto bajo la licencia **Affero General Public License (AGPL)**.
Código original VB6 Copyright (C) 2002 Pablo Ignacio Márquez, Aaron Perkins, Otto Perez, Matías Fernando Pequeño.
