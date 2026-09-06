---
area: pantallas-e-interfaz
status: not-started
audit_reference: docs/audit/07-pantallas-e-interfaz.md
tags: [ui, interfaz, formularios, frm, cliente, pantallas]
last_updated: 2026-09-06
---

## Decisiones de Diseño
- **Reconstrucción del GUI en C++**: Implementar el sistema de UI en el cliente C++ replicando el layout original, botones gráficos (`clsGraphicalButton`) e inventario renderizado en grilla (`clsGrapchicalInventory`).

## Preguntas Abiertas / Riesgos
- *Heredado de la auditoría*: Asegurar que los assets de fondo de cada pantalla (`.bmp`) se carguen exactamente en las mismas posiciones absolutas de pantalla (e.g. 800x600 px).

## Tareas
- [ ] Definir el framework de UI a utilizar en el nuevo cliente C++.
- [ ] Recrear la pantalla de Login (`frmConnect`), Creación de Personaje (`frmCrearPersonaje`) y HUD Principal (`frmMain`).

## Archivos de Código Relacionados
- `legacy/client/CODIGO/frmConnect.frm`
- `legacy/client/CODIGO/frmMain.frm`
- `legacy/client/CODIGO/clsGraphicalButton.cls`
