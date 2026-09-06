---
area: prediccion-de-movimiento
status: not-started
audit_reference: docs/audit/03-prediccion-de-movimiento.md
tags: [prediccion, movimiento, combate, sincronizacion, extrapolacion]
last_updated: 2026-09-06
---

## Decisiones de Diseño
- **Predicción de Movimiento en Cliente C++**: Mantener el modelo legacy de extrapolación inmediata del movimiento local del personaje (`MoveCharbyHead`, `MoveScreen`) mientras el combate sigue bajo estricta autoridad del servidor.

## Preguntas Abiertas / Riesgos
- *Heredado de la auditoría*: Asegurar que el manejo de desincronizaciones (`PosUpdate`) devuelva suavemente al personaje a la coordenada autoritativa sin saltos visuales bruscos.

## Tareas
- [ ] Implementar la predicción local de posición en la grilla del cliente C++.
- [ ] Implementar el gestor de paquetes de corrección de posición `HandlePosUpdate`.

## Archivos de Código Relacionados
- `legacy/client/CODIGO/General.bas`
- `legacy/client/CODIGO/ProtocolCmdParse.bas`
- `legacy/server/Codigo/Protocol.bas`
