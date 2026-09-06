---
area: formulas-de-combate
status: not-started
audit_reference: docs/audit/04-formulas-de-combate.md
tags: [combate, formulas, daño, evasion, experiencia, regeneracion]
last_updated: 2026-09-06
---

## Decisiones de Diseño
- **Motor de Combate Deterministico**: Implementar la tabla de fórmulas de daño y evasión en una clase/módulo `CombatSystem` en C++ con preservación exacta de redondeos y rangos min/max.

## Preguntas Abiertas / Riesgos
- *Heredado de la auditoría*: Asegurar que los modificadores por clase y constantes de multiplicador `EXP_MUL` lean correctamente de la configuración en runtime.

## Tareas
- [ ] Implementar la clase `CombatSystem` en C++ con métodos `calculate_damage` y `hit_probability`.
- [ ] Escribir pruebas unitarias comparando los outputs determinísticos con los resultados esperados del servidor VB6.

## Archivos de Código Relacionados
- `legacy/server/Codigo/SistemaCombate.bas`
- `legacy/server/Codigo/Modulo_UsUaRiOs.bas`
