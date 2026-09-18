---
area: foros-y-comunicacion
source_files:
  - legacy/server/Codigo/modForum.bas
  - legacy/server/Codigo/Party.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/Acciones.bas
tags: [foros, comunicacion, party, mensajes, anuncios, sticky, comunidad]
last_updated: 2026-09-13
---

# Macro-Área 11: Foros y Comunicación

Este documento define la visión arquitectónica transversal y el alcance del sistema de foros comunitarios, mensajería y canales de comunicación interactiva en Argentum Online v0.13.0.

---

## 1. Resumen

El subsistema de foros y comunicación gestiona la interacción social asincrónica y sincrónica entre jugadores dentro del servidor:
- **Foros en Juego (`modForum.bas`)**: Carteleras públicas, foros de ciudades y espacios de discusión de clanes y facciones persistidos en disco mediante archivos INI/secuenciales `.for`.
- **Canales de Chat y Mensajería (`Protocol.bas` / `modSendData.bas`)**: Transmisión de mensajes privados (`/whisper`), broadcasts de clanes, facciones (Real/Caos), gritos de mapa y consola pública con reglas de visibilidad por área y roles de usuario.
- **Sistema de Grupos / Party (`Party.bas`)**: Coordinación táctica multijugador, canal de comunicación exclusivo de grupo y distribución equitativa de experiencia.

---

## 2. Alcance Arquitectónico

### A. Foros del Juego (`modForum.bas`)
- **Persistencia en Disco**: Directorio `Foros/` con formato `.for` estructurado en índices INI (`[INFO]`) y archivos de texto plano para publicaciones generales y anuncios fijados (*stickies*).
- **Control de Acceso y Moderación**: Permisos de publicación y anclaje condicionados por alineación faccionaria (`eForumAlignment`), liderazgo de clan o privilegios de Game Master.
- **Protocolo y Despacho**: Flujo bidireccional mediante paquetes `ShowForumForm`, `AddForumMsg` y `ForumPost`.

### B. Comunicación Social y Canales de Chat
- **Mensajería Directa**: Comunicación punto a punto entre usuarios conectados con validaciones de silencio administrativo (`flags.Silenciado`).
- **Comunicaciones Colectivas**: Emisión a miembros de un mismo clan (`SendToClan`), grupo (`SendToParty`) o facción (`SendToArmada` / `SendToCaos`).
- **Regulación y Anti-Spam**: Detección de flood de texto y filtros de caracteres inválidos antes de la retransmisión.

### C. Sistema de Party (`Party.bas`)
- **Gestión de Membresía**: Creación, invitación, aceptación y expulsión de miembros (hasta un límite de 5 integrantes).
- **Reparto de Experiencia y Recompensas**: Algoritmo de ponderación de experiencia según el nivel relativo de los miembros en el radio de visión.

---

## 3. Módulos y Subsistemas Involucrados

| Módulo Legacy | Responsabilidad Principal | Informe de Detalle |
| :--- | :--- | :--- |
| `modForum.bas` | Lógica de foros en memoria, persistencia `.for` y serialización de mensajes. | [`11a-modforum-detalle.md`](11a-modforum-detalle.md) |
| `mdParty.bas` / `clsParty.cls` | Creación de grupos, coordinación de integrantes y distribución de experiencia. | [`11b-party-detalle.md`](11b-party-detalle.md) |

| `Acciones.bas` | Detección de doble clic en carteles de foro y disparo del formulario. | Transversal |
| `Protocol.bas` | Manejadores de red para paquetes de foros y mensajería de chat. | [`02e-protocol-detalle.md`](02e-protocol-detalle.md) |
