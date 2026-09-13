---
area: criaturas-e-ia
source_files:
  - legacy/server/Codigo/MODULO_NPCs.bas
  - legacy/server/Codigo/AI.bas
  - legacy/server/Codigo/PathFinding.bas
  - legacy/server/Codigo/NPCs.dat
tags: [npcs, criaturas, ia, pathfinding, spawn, maquinas-de-estados, hostilidad]
last_updated: 2026-09-13
---

# Macro-Área 13: Criaturas e Inteligencia Artificial

Este documento define la visión arquitectónica transversal y el alcance del subsistema de criaturas no jugadoras (NPCs), ciclo de vida, tablas de atributos, navegación espacial e inteligencia artificial en Argentum Online v0.13.0.

---

## 1. Resumen

El subsistema de criaturas e inteligencia artificial da vida al ecosistema del mundo de Argentum Online:
- **Ciclo de Vida y Gestión de Entidades (`MODULO_NPCs.bas`)**: Carga de plantillas desde `NPCs.dat`, invocación (*spawn*), mantenimiento en el vector global `Npclist`, control de muerte, regeneración de salud/maná y temporizadores de respawn.
- **Tipología y Alineación**: Criaturas hostiles (monstruos), pasivas (fauna), guardias imperiales/caóticos, pretorianos reales y mercaderes/banqueros con diálogos e interacciones específicas.
- **Inteligencia Artificial y Toma de Decisiones (`AI.bas`)**: Máquinas de estados reactivas para patrullaje, detección de objetivos por radio de visión, cambio de blanco por daño recibido, casteo autónomo de hechizos ofensivos/defensivos y huida ante baja vitalidad.
- **Navegación Espacial y Rutas (`PathFinding.bas`)**: Búsqueda de caminos hacia el objetivo esquivando obstáculos estáticos (bloqueos del mapa) y dinámicos (otros personajes o criaturas).

---

## 2. Alcance Arquitectónico

### A. Ciclo de Vida de Criaturas (`MODULO_NPCs.bas`)
- **Inicialización y Carga**: Lectura de propiedades fijas (HP, Maná, Poder de Ataque, Evasión, Alineación, Drop de Oro) y asignación al array `Npclist`.
- **Muerte y Limpieza**: Disparo de la cascada de recompensas en inventario y áreas, eliminación del mapa (`EraseNPCChar`) y programación de reaparición si el NPC tiene flag de respawn activo.
- **Regeneración Periódica**: Rutinas ejecutadas en los ciclos del loop principal para recuperación paulatina de salud y energía.

### B. Máquinas de Estados e Inteligencia Artificial (`AI.bas`)
- **Estados de Comportamiento**:
  - *Reposo / Patrulla*: Movimiento aleatorio pausado dentro de un radio de confinamiento (*spawn origin*).
  - *Alerta / Búsqueda*: Escaneo de celdas adyacentes para detectar usuarios hostiles, faccionarios enemigos o criaturas agresoras.
  - *Persecución y Combate*: Aproximación al objetivo, ejecución de golpes cuerpo a cuerpo o a distancia y uso táctico de magia según el perfil del NPC.
  - *Huida*: Alejamiento estratégico del objetivo cuando la vitalidad cae por debajo de un umbral porcentual predefinido.
- **Especialización por Arquetipo**: Guardias de ciudad con detección instantánea de criminales; sacerdotes con curación y remoción de veneno; monstruos con daño elemental.

### C. Navegación y Búsqueda de Caminos (`PathFinding.bas`)
- **Algoritmo de Ruta**: Cálculo del siguiente paso en la cuadrícula ortogonal/diagonal para acortar distancia con el objetivo sin quedar atascado contra muros o cuerpos de agua no navegables.
- **Manejo de Bloqueos Temporales**: Lógica de desvío lateral cuando el camino óptimo se encuentra transitoriamente obstruido por otra entidad.

---

## 3. Módulos y Subsistemas Involucrados

| Módulo Legacy | Responsabilidad Principal | Informe de Detalle |
| :--- | :--- | :--- |
| `MODULO_NPCs.bas` | Estructuras de datos, spawn, respawn, tablas de atributos y muerte. | Futura auditoría `13a-modulo-npcs-detalle.md` |
| `AI.bas` | Máquinas de estados, heurísticas de combate, selección de blanco y huida. | Futura auditoría `13b-ai-detalle.md` |
| `PathFinding.bas` | Algoritmos de navegación espacial y evasión de obstáculos. | Futura auditoría `13c-pathfinding-detalle.md` |
| `NPCs.dat` | Base de datos de criaturas, atributos, gráficos asociados y recompensas. | [`06-formatos-de-datos.md`](06-formatos-de-datos.md) |
