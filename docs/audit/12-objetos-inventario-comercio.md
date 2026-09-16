---
area: objetos-inventario-comercio
source_files:
  - legacy/server/Codigo/Modulo_InventANDobj.bas
  - legacy/server/Codigo/InvUsuario.bas
  - legacy/server/Codigo/Comercio.bas
  - legacy/server/Codigo/Trabajo.bas
tags: [objetos, inventario, comercio, economia, equipamiento, profesiones, floor-items]
last_updated: 2026-09-13
---

# Macro-Área 12: Objetos, Inventario y Comercio

Este documento define la visión arquitectónica transversal y el alcance del subsistema de manipulación de objetos, gestión de inventarios (usuarios y NPCs), equipamiento, transacciones comerciales y manufactura artesanal en Argentum Online v0.13.0.

---

## 1. Resumen

El subsistema de objetos e inventario gobierna la economía material del mundo persistente de Argentum Online:
- **Entidades Físicas en el Mundo (`MapData.ObjInfo`)**: Representación de objetos arrojados en celdas transitables del mapa, apilamiento de cantidades (hasta 10.000 unidades), flags de bloqueo por ocupación y temporizadores de limpieza (*garbage collector*).
- **Inventarios de Entidades**: Almacenamiento indexado (base 1) para personajes jugadores (`UserList(UserIndex).Invent`) y criaturas/vendedores (`Npclist(NpcIndex).Invent`).
- **Equipamiento y Modificadores de Combate**: Lógica de equipamiento por slots (arma, armadura, escudo, casco, anillo, munición, herramienta) y cálculo acumulativo de poder de ataque/defensa.
- **Economía y Comercio (`Comercio.bas`)**: Transacciones con mercaderes NPC y usuarios, control de billetera de oro y listas de precios de compra/venta.
- **Oficios y Manufactura (`Trabajo.bas`)**: Extracción de materias primas (tala, minería, pesca) y elaboración de herramientas, armas y vestimentas (herrería, carpintería).

---

## 2. Alcance Arquitectónico

### A. Objetos en el Mundo y Despacho Espacial
- **Creación y Destrucción en Suelo**: Rutinas `MakeObj`, `EraseObj`, `DropObj` y `GetObj`.
- **Búsqueda de Celdas Transitables**: Algoritmo espiral `Tilelibre` para ubicar coordenadas válidas de tierra o agua ante caídas y drops.
- **Transmisión Espacial**: Sincronización a clientes en el área de 9 cuadrantes mediante paquetes de red de creación y borrado de objetos (`ObjectCreate`, `ObjectDelete`).

### B. Inventario de Personajes Jugadores (`InvUsuario.bas`)
- **Gestión de Ranuras**: Catálogo de 30 ranuras por usuario, apilamiento automático, intercambio de posiciones (`MoverItem`) y validación de peso total (`MaxHIT`).
- **Equipamiento Seguro**: Reglas de clase, raza y nivel para vestir armaduras y empuñar armas. Prevención de vulnerabilidades históricas de doble equipamiento.

### C. Inventario de Criaturas y Drops (`Modulo_InventANDobj.bas`)
- **Carga de Plantillas**: Asignación de stock inicial a partir de definiciones de NPCs sin accesos bloqueantes a disco.
- **Drops Probabilísticos**: Disparo de la cascada de recompensas al morir un NPC (`NPC_TIRAR_ITEMS`), fraccionamiento de oro en montículos (`TirarOroNpc`) y replicación de reglas históricas de NPCs pretorianos.

### D. Economía, Comercio y Oficios
- **Mercaderes NPC**: Venta, compra con devaluación porcentual, reposición periódica de stock (`ReiniciarInventarioNPCs`) y control de fondos.
- **Oficios y Herramientas**: Recolección de leña, minerales y peces; fundición de lingotes y confección de ítems mediante tablas de requerimientos de habilidad (*skills*).

---

## 3. Módulos y Subsistemas Involucrados

| Módulo Legacy | Responsabilidad Principal | Informe de Detalle |
| :--- | :--- | :--- |
| `Modulo_InventANDobj.bas` | Inventario de NPCs, drops probabilísticos y despacho `TirarItemAlPiso`. | [`12a-modulo-inventandobj-detalle.md`](12a-modulo-inventandobj-detalle.md) |
| `InvUsuario.bas` | Manipulación de `UserList.Invent`, mutaciones en mapa y equipamiento. | [`12b-invusuario-detalle.md`](12b-invusuario-detalle.md) |
| `Comercio.bas` | Lógica de compra y venta entre usuarios y mercaderes NPC. | [`12d-comercio-detalle.md`](12d-comercio-detalle.md) |
| `Trabajo.bas` | Habilidades de recolección y fabricación de objetos. | [`12e-trabajo-detalle.md`](12e-trabajo-detalle.md) |
