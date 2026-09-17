# Módulo #30 — Ejército de la Fortaleza Pretoriana (`praetorians.bas`)

## Estado del Módulo
`Completado (Aislado / Cableado Pendiente en Capas 9 y 11)`

---

## Resumen y Alcance
El módulo `praetorians` gestiona la Inteligencia Artificial táctica y el ciclo de vida del clan militar pretoriano alojado en el mapa especial de la fortaleza (`MAPA_PRETORIANO`). Incluye la coordinación táctica de 5 roles militarizados especializados (Rey, Guerrero, Cazador, Mago y Clérigo), patrullas por waypoints en alcobas, magia de soporte en cascada, disipación de invisibilidad, protocolo de autodestrucción kamikaze e instanciación del clan militar.

---

## Decisiones de Diseño y Paridad Legacy

1. **Reutilización de Slots de Inventario**:
   - `Npclist[npc].Invent.ArmourEqpSlot`: Persiste el waypoint activo (1..8) de la rutina `CambiarAlcoba`.
   - `Npclist[npc].Invent.BarcoSlot`: Funciona como el contador regresivo kamikaze en `MagoDestruyeWand`.
   - Se respetó strictly la paridad con VB6 sin alterar el modelo de datos `npc`.

2. **Navegación Voraz (`GreedyWalkTo`)**:
   - Preservación intencional del comportamiento voraz celda a celda (evaluación de deltas $\Delta X$ y $\Delta Y$).
   - Se mantuvieron las oscilaciones y bloqueos ante esquinas en "L" sin reemplazar por algoritmos globales como A* o BFS.

3. **Cascada Táctica del Clérigo (`PRCLER_AI`)**:
   - Priorización estricta de auxilio a pretorianos aliados en un radio de 12 baldosas:
     1. Remoción de veneno (`NPCRemueveVenenoNPC`).
     2. Disipación de parálisis/inmovilización (`NPCRemueveParalisisNPC`).
     3. Curación de salud (`NPCCuraLevesNPC`, topeada a `Stats.MaxHp`).
   - En combate ofensivo, prioriza aplicar parálisis contra clases arcanas (`EsMagoOClerigo`).

4. **Inmolación Kamikaze del Mago (`PRMAGO_AI` / `MagoDestruyeWand`)**:
   - Al caer a `MinHp < 750`, activa el temporizador en `Invent.BarcoSlot = 6`.
   - Al llegar a $\le 1$, emite explosión masiva ($5\times 5$), inflige 300 de daño residual a usuarios vivos y convoca `MuereNpc(npc_index, 0)`.

5. **Instanciación y Alternancia del Clan (`CrearClanPretoriano`)**:
   - Alterna la alcoba de spawn según la posición del rey derrotado (`previous_king_x >= 50` $\rightarrow$ Alcoba 1 `(35, 25)`; de lo contrario Alcoba 2 `(67, 25)`).
   - Genera la formación de 8 unidades (1 Rey, 2 Clérigos, 3 Guerreros, 1 Cazador, 1 Mago) e inicializa `pretorianosVivos = 7`.

---

## Catálogo de Desacoplamiento (`PraetorianCallbacks`)

Todas las dependencias salientes hacia otros subsistemas del servidor están desacopladas mediante `PraetorianCallbacks`:

| Callback | Firma / Propósito |
| :--- | :--- |
| `WarpUserChar` | `(user_index, map, x, y)` — Teletransporte de usuario. |
| `MoveNPCChar` | `(npc_index, heading)` — Desplazamiento cardinal del NPC. |
| `QuitarNPC` | `(npc_index)` — Remoción de NPC del mapa. |
| `CrearNPC` | `(npc_number, map, pos)` — Instanciación de NPC en el mapa. |
| `ClosestLegalPos` | `(pos, npos, check_water, check_land)` — Búsqueda de posición legal cercana. |
| `LegalPos` | `(map, x, y)` — Validación de celda transitable. |
| `NpcAtacaUser` | `(npc_index, target_user)` — Ataque físico contra usuario. |
| `NpcLanzaSpellSobreUser` | `(npc_index, target_user, spell_index)` — Conjuro de hechizo sobre usuario. |
| `SendData` | `(send_target, target_index, msg)` — Transmisión de paquetes de red. |
| `PrepareMessageCharacterMove` | `(char_index, x, y)` — Formateo de paquete de movimiento. |
| `PrepareMessageChatOverHead` | `(text, char_index, color)` — Formateo de texto overhead. |
| `PrepareMessageCreateFX` | `(char_index, fx, loops)` — Formateo de efecto visual FX. |
| `WriteConsoleMsg` | `(user_index, msg, font_type)` — Mensajes de consola a usuario. |
| `MuereNpc` | `(npc_index, attacker_user)` — Muerte y respawn de NPC. |
| `LogError` | `(error_msg)` — Registro de errores. |
| `RandomNumber` | `(min, max)` — Generador de números aleatorios. |

---

## Métricas de Cobertura de Pruebas Unitarias (`tests/test_praetorians.cpp`)

La suite doctest de `praetorians` cuenta con 15 test cases organizados en 4 grupos funcionales:

- **G1 (Infraestructura y Navegación Básica)**:
  - Mapeo de `esPretoriano`.
  - Rangos de `EstoyLejos` y `EstoyMuyLejos`.
  - Alcance visual de `EsAlcanzable`.
  - Liberación de bloqueo fantasma en `CasperBlock` y `LiberarCasperBlock`.
- **G2 (Movimiento Cardinal y Waypoints)**:
  - Despacho de headings exactos en `MoverArr`, `MoverAba`, `MoverIzq`, `MoverDer`.
  - Reducción de distancia Manhattan en `GreedyWalkTo`.
  - Verificación del quirk de detención ante obstáculos en "L".
  - Enrutamiento de `VolverAlCentro` a Alcoba 1 / Alcoba 2.
  - Avance secuencial de waypoints 1..8 en `CambiarAlcoba`.
- **G3 (Magia Táctica e Inmolación)**:
  - Identificación de Mago y Clérigo en `EsMagoOClerigo`.
  - Tope de curación en `NPCCuraLevesNPC` a `Stats.MaxHp`.
  - Limpieza de parálisis y veneno en aliados.
  - Remoción simultánea de `invisible` y `Oculto` en `NPCRemueveInvisibilidad`.
  - Conteo kamikaze y detonación en `MagoDestruyeWand`.
- **G4 (IA por Rol y Formación Militar)**:
  - Retorno al centro de `PRREY_AI` tras desplazamiento.
  - Ataque a dist 1 y persecución en `PRGUER_AI`.
  - Cascada de prioridad de auxilio aliado en `PRCLER_AI`.
  - Activación de autodestrucción en `PRMAGO_AI`.
  - Generación militar de 8 unidades y asignación de `pretorianosVivos = 7` en `CrearClanPretoriano`.
