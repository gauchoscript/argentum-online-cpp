---
area: magia-y-hechizos
source_files:
  - legacy/server/Codigo/modHechizos.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/Hechizos.dat
  - legacy/server/Codigo/ModAreas.bas
tags: [magia, hechizos, mana, lanzamiento, curacion, dano-magico, efectos]
last_updated: 2026-09-13
---

# Macro-Área 14: Magia y Hechizos

Este documento define la visión arquitectónica transversal y el alcance del sistema de magia, casteo de conjuros, fórmulas de daño y curación mágica, estados alterados y efectos especiales en Argentum Online v0.13.0.

---

## 1. Resumen

El subsistema de magia gobierna las interacciones arcanas tanto ofensivas como defensivas y de utilidad entre personajes y criaturas:
- **Catálogo de Hechizos (`Hechizos.dat`)**: Definición estática de conjuros, requerimientos de nivel y maná, consumo de energía/estamina, tipos de target (usuario, criatura, celda de terreno) y animaciones visuales/efectos de sonido (FX).
- **Lanzamiento y Validación (`modHechizos.bas`)**: Verificación de distancias máximas de casteo, líneas de visión directa, estado de inmovilización del lanzador, tiempos de recuperación (*cooldowns*) e intervalos entre hechizo y ataque físico.
- **Fórmulas de Impacto y Mitigación**: Cálculo de daño mágico directo, curación de puntos de vida, daño elemental y mitigación mediante resistencia mágica o equipamiento especial (anillos de resistencia mágica).
- **Efectos Secundarios y Alteraciones de Estado**: Parálisis, inmovilización, ceguera, envenenamiento, invisibilidad, mimetismo, invocación de familiares y remoción de maldiciones.

---

## 2. Alcance Arquitectónico

### A. Ciclo de Ejecución de Conjuros (`modHechizos.bas`)
- **Petición del Cliente**: Recepción de opcode `CastSpell` indicando el slot del libro de hechizos y coordenadas o ID del objetivo.
- **Validación Preliminar**:
  - Puntos de maná (`UserList.Stats.MinMAN >= Hechizo.ManaRequerido`).
  - Nivel y clase del lanzador (hechizos exclusivos para clérigos, magos, bardos, paladines o druidas).
  - Estado del objetivo (vivo, muerto, navegando, zona segura).
- **Consumo de Recursos y Aplicación**: Reducción de maná y estamina, invocación del cálculo de daño/curación y notificación a clientes en el área de visibilidad mediante paquetes de animación mágica (`CreateFX`).

### B. Efectos de Hechizos y Estados Alterados
- **Control de Masas**:
  - *Parálisis e Inmovilización*: Congelamiento temporal del movimiento del personaje manteniendo o no la posibilidad de lanzar magia.
  - *Ceguera y Estupidez*: Alteraciones de visibilidad y distorsión de mensajes en el cliente.
- **Transformación e Invisibilidad**: Modificación transitoria del cuerpo (`Body`), cabeza (`Head`) o flag de invisibilidad (`flags.Invisible`), afectando el renderizado en clientes y la hostilidad de los NPCs.
- **Invocaciones**: Creación temporal de criaturas aliadas (`CrearNPCChar`) supeditadas al carisma y control del invocador.

### C. Coexistencia con el Combate Físico
- **Intervalos de Magia**: Temporizadores estrictos entre casteo de hechizo y golpe físico (`IntervaloGolpeMagia`), piedra angular del combate dinámico de Argentum Online.

---

## 3. Módulos y Subsistemas Involucrados

| Módulo Legacy | Responsabilidad Principal | Informe de Detalle |
| :--- | :--- | :--- |
| `modHechizos.bas` | Lógica de conjuros, casteo, daño mágico, curación y efectos de estado. | Futura auditoría `14a-modhechizos-detalle.md` |
| `Protocol.bas` | Despacho de paquetes de casteo y notificación de animaciones FX. | [`02e-protocol-detalle.md`](02e-protocol-detalle.md) |
| `Hechizos.dat` | Base de datos de conjuros, propiedades, requisitos y animaciones asociadas. | [`06-formatos-de-datos.md`](06-formatos-de-datos.md) |
| `ModAreas.bas` | Notificación espacial de efectos visuales y auditivos en 3x3 cuadrantes. | [`03a-modareas-detalle.md`](03a-modareas-detalle.md) |
