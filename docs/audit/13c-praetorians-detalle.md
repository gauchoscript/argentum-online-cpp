# Auditoría Técnica del Módulo #30: praetorians.bas

> **Estado**: Documento Final de Auditoría  
> **Macro-Área**: Área 13 (Criaturas e IA)  
> **Fecha**: 17 de Septiembre de 2026  
> **Módulo Legacy**: legacy/server/Codigo/praetorians.bas (2065 líneas)  
> **Módulos Vinculados**: MODULO_NPCs.bas (Módulo #28), AI_NPC.bas (Módulo #29), SistemaCombate.bas (Módulo #22), modHechizos.bas (Módulo #23), ModAreas.bas (Módulo #17), modSendData.bas (Módulo #15).

---

## 1. Introducción y Propósito

El presente documento constituye la auditoría técnica exhaustiva del Módulo #30: praetorians.bas, responsable de la Inteligencia Artificial cooperativa de los **Pretorianos** (NPCs de guardias/ejército elite que defienden la fortaleza pretoriana en el mapa designado por MAPA_PRETORIANO).

Este sistema implementa un comportamiento de clan/escuadrón cooperativo con 5 roles diferenciados (Sacerdote, Mago, Cazador, Guerrero y Rey), tácticas de soporte mutuo (curación, desparalización, remoción de invisibilidad y veneno), cambio espacial coordinado entre alcobas y mecánicas de invulnerabilidad jerárquica para el Rey mientras existan tropas vivas.

---

## 2. Catálogo Completo de Procedimientos

A continuación se detalla cada una de las 30 rutinas (subrutinas y funciones) presentes en praetorians.bas:

| # | Procedimiento | Firma Legacy | Visibilidad | Descripción Sintética |
| :-: | :--- | :--- | :-: | :--- |
| 01 | esPretoriano | Function esPretoriano(ByVal NpcIndex As Integer) As Integer | Public | Devuelve el rol del pretoriano (1=Clérigo/Sacerdote, 2=Mago, 3=Cazador, 4=Rey, 5=Guerrero) o 0 si no es pretoriano. |
| 02 | CrearClanPretoriano | Sub CrearClanPretoriano(ByVal X As Integer) | Private (Implicit) | Spawnhea un escuadrón completo de 8 pretorianos (1 Rey, 2 Sacerdotes, 3 Guerreros, 1 Cazador y 1 Mago) en la alcoba opuesta. |
| 03 | PRCAZA_AI | Sub PRCAZA_AI(ByVal npcind As Integer) | Private (Implicit) | IA táctica del Cazador Pretoriano. Escanea enemigos a distancia, lanza flechas/hechizos ofensivos y retrocede si se acercan. |
| 04 | PRMAGO_AI | Sub PRMAGO_AI(ByVal npcind As Integer) | Private (Implicit) | IA táctica del Mago Pretoriano. Escanea en espiral, remueve invisibilidad (35%), lanza Apocalipsis o detona su vara si agota HP (<750). |
| 05 | PRREY_AI | Sub PRREY_AI(ByVal npcind As Integer) | Private (Implicit) | IA pasiva/defensiva del Rey Pretoriano. Permanece impasible en su alcoba y retoma el centro si es desplazado. |
| 06 | PRGUER_AI | Sub PRGUER_AI(ByVal npcind As Integer) | Private (Implicit) | IA de vanguardia del Guerrero Pretoriano. Busca usuarios cuerpo a cuerpo (distancia 1), ataca físicamente o avanza con GreedyWalkTo. |
| 07 | PRCLER_AI | Sub PRCLER_AI(ByVal npcind As Integer) | Private (Implicit) | IA de soporte del Sacerdote Pretoriano. Prioriza soporte mutuo a aliados (remover veneno, parálisis, curar) o paraliza/ataca enemigos. |
| 08 | EsMagoOClerigo | Function EsMagoOClerigo(ByVal PJEnInd As Integer) As Boolean | Private (Implicit) | Determina si un usuario objetivo es Mago o Clérigo consultando UserList(PJEnInd).clase. |
| 09 | NPCRemueveVenenoNPC | Sub NPCRemueveVenenoNPC(ByVal npcind As Integer, ByVal NPCAlInd As Integer, ByVal indice As Integer) | Private (Implicit) | Aplica el hechizo de Remoción de Veneno sobre un pretoriano aliado envenenado (Veneno = 1). |
| 10 | NPCCuraLevesNPC | Sub NPCCuraLevesNPC(ByVal npcind As Integer, ByVal NPCAlInd As Integer, ByVal indice As Integer) | Private (Implicit) | Aplica el hechizo de Curación sobre un aliado dañado restaurando su HP. |
| 11 | NPCRemueveParalisisNPC | Sub NPCRemueveParalisisNPC(ByVal npcind As Integer, ByVal NPCAlInd As Integer, ByVal indice As Integer) | Private (Implicit) | Aplica el hechizo de Remoción de Parálisis sobre un aliado inmovilizado. |
| 12 | NPCparalizaNPC | Sub NPCparalizaNPC(ByVal paralizador As Integer, ByVal Paralizado As Integer, ByVal indice) | Private (Implicit) | Lanza el hechizo de Parálisis sobre un objetivo (utilizable contra usuarios). |
| 13 | NPCcuraNPC | Sub NPCcuraNPC(ByVal curador As Integer, ByVal curado As Integer, ByVal indice As Integer) | Private (Implicit) | Sobrecarga de curación mutual entre pretorianos. |
| 14 | NPCLanzaCegueraPJ | Sub NPCLanzaCegueraPJ(ByVal npcind As Integer, ByVal PJEnInd As Integer, ByVal indice As Integer) | Private (Implicit) | Lanza el conjuro de Ceguera sobre un usuario (Ceguera = 1). |
| 15 | NPCLanzaEstupidezPJ | Sub NPCLanzaEstupidezPJ(ByVal npcind As Integer, ByVal PJEnInd As Integer, ByVal indice As Integer) | Private (Implicit) | Lanza el conjuro de Estupidez sobre un usuario (Estupidez = 1). |
| 16 | NPCRemueveInvisibilidad | Sub NPCRemueveInvisibilidad(ByVal npcind As Integer, ByVal PJEnInd As Integer, ByVal indice As Integer) | Private (Implicit) | Remueve el estado de invisibilidad (invisible = 0, Oculto = 0) de un usuario objetivo. |
| 17 | NpcLanzaSpellSobreUser2 | Sub NpcLanzaSpellSobreUser2(ByVal NpcIndex As Integer, ByVal UserIndex As Integer, ByVal Spell As Integer) | Private (Implicit) | Ejecuta el lanzamiento de un conjuro genérico desde un NPC hacia un usuario ajustando maná y timers. |
| 18 | MagoDestruyeWand | Sub MagoDestruyeWand(ByVal npcind As Integer, ByVal bs As Byte, ByVal indice As Integer) | Private (Implicit) | Mecánica de autodestrucción kamikaze del Mago al agonizar (causa explosión y daño de área). |
| 19 | GreedyWalkTo | Sub GreedyWalkTo(ByVal npcorig As Integer, ByVal Map As Integer, ByVal X As Integer, ByVal Y As Integer) | Private (Implicit) | Algoritmo de desplazamiento voraz (Greedy) celda a celda con evaluación de obstáculos directos. |
| 20 | MoverAba | Sub MoverAba(ByVal npcorig As Integer) | Private (Implicit) | Desplaza el NPC una celda hacia abajo (Sur) mediante MoveNPCChar. |
| 21 | MoverArr | Sub MoverArr(ByVal npcorig As Integer) | Private (Implicit) | Desplaza el NPC una celda hacia arriba (Norte) mediante MoveNPCChar. |
| 22 | MoverIzq | Sub MoverIzq(ByVal npcorig As Integer) | Private (Implicit) | Desplaza el NPC una celda hacia la izquierda (Oeste) mediante MoveNPCChar. |
| 23 | MoverDer | Sub MoverDer(ByVal npcorig As Integer) | Private (Implicit) | Desplaza el NPC una celda hacia la derecha (Este) mediante MoveNPCChar. |
| 24 | VolverAlCentro | Sub VolverAlCentro(ByVal npcind As Integer) | Private (Implicit) | Ordena al pretoriano regresar a las coordenadas centrales de su alcoba activa. |
| 25 | EstoyMuyLejos | Function EstoyMuyLejos(ByVal npcind) As Boolean | Private (Implicit) | Evalúa si el pretoriano superó la distancia máxima respecto al centro de la fortaleza (>22 celdas). |
| 26 | EstoyLejos | Function EstoyLejos(ByVal npcind) As Boolean | Private (Implicit) | Evalúa si el pretoriano se alejó de las inmediaciones de su alcoba (>12 celdas). |
| 27 | EsAlcanzable | Function EsAlcanzable(ByVal npcind As Integer, ByVal PJEnInd As Integer) As Boolean | Private (Implicit) | Valida si existe línea de visión/alcance libre de bloqueos sólidos entre el NPC y un jugador. |
| 28 | CasperBlock | Function CasperBlock(ByVal npc As Integer) As Boolean | Private (Implicit) | Detecta si un usuario muerto (Casper/fantasma) está bloqueando la casilla de avance del pretoriano. |
| 29 | LiberarCasperBlock | Sub LiberarCasperBlock(ByVal npcind As Integer) | Private (Implicit) | Fuerza el desplazamiento o teletransporte del fantasma bloqueante para liberar el camino. |
| 30 | CambiarAlcoba | Public Sub CambiarAlcoba(ByVal npcind As Integer) | Public | Guía la migración del pretoriano entre Alcoba 1 y Alcoba 2 mediante una secuencia de 8 waypoints espaciales. |

---

## 3. Máquina de Estados y IA Táctica por Rol

El ciclo de IA pretoriana se dispara desde AI_NPC.bas (ExecutiveNPCAI) mediante la invocación de la rutina específica según la clase del NPC:

``mermaid
flowchart TD
    AI[ExecutiveNPCAI en AI_NPC.bas] --> Check{esPretoriano?}
    Check -->|1: Sacerdote| PRCLER[PRCLER_AI]
    Check -->|2: Mago| PRMAGO[PRMAGO_AI]
    Check -->|3: Cazador| PRCAZA[PRCAZA_AI]
    Check -->|4: Rey| PRREY[PRREY_AI]
    Check -->|5: Guerrero| PRGUER[PRGUER_AI]

    PRCLER --> ClericChoice{HP de Aliados < Max?}
    ClericChoice -->|Sí| NPCCura[NPCCuraLevesNPC]
    ClericChoice -->|No| CheckEnemies[Escanear Jugadores]

    PRMAGO --> WandCheck{MinHp < 750?}
    WandCheck -->|Sí: Agonía| MagoKamikaze[MagoDestruyeWand]
    WandCheck -->|No| CastApoc[NpcLanzaSpellSobreUser2: Apocalipsis]

    PRGUER --> MeleeCheck{Distancia == 1?}
    MeleeCheck -->|Sí| AtacarMelee[NpcAtacaUser]
    MeleeCheck -->|No| GreedyWalk[GreedyWalkTo Hacia Enemigo]
`

### 3.1. Roles y Tácticas
1. **Sacerdote/Clérigo (PRCLER_AI)**:
   - **Prioridad 1 (Soporte Mutuo)**: Escanea aliados en un radio de 12x12. Si detecta un pretoriano envenenado (NPCRemueveVenenoNPC), paralizado (NPCRemueveParalisisNPC) o herido (NPCCuraLevesNPC), interrumpe sus ataques para socorrerlo.
   - **Prioridad 2 (Control de Masas y Ataque)**: Paraliza a Magos y Clérigos enemigos (NPCparalizaNPC), aplica Ceguera/Estupidez y ataca con hechizos de daño.

2. **Mago (PRMAGO_AI)**:
   - **Prioridad 1 (Detección de Invisibles)**: Posee un 35% de probabilidad de detectar y remover la invisibilidad/ocultamiento (NPCRemueveInvisibilidad) de jugadores furtivos.
   - **Prioridad 2 (Bombardeo Mágico)**: Lanza *Apocalipsis* prioritariamente sobre enemigos paralizados o expuestos.
   - **Prioridad 3 (Protocolo Kamikaze)**: Si su salud cae por debajo de 750 HP (MinHp < 750), entra en estado de sobrecarga (BarcoSlot = 6) e inicia la cuenta regresiva para detonar su vara (MagoDestruyeWand), autoinfligiéndose la muerte y provocando un colapso en área.

3. **Cazador (PRCAZA_AI)**:
   - Mantiene distancia táctica. Dispara proyectiles físicos y magia a distancia. Si un enemigo logra recortar la distancia cuerpo a cuerpo, retrocede mediante GreedyWalkTo.

4. **Guerrero (PRGUER_AI)**:
   - Táctica de embestida frontal. Avanza agresivamente mediante GreedyWalkTo hacia el enemigo más cercano y ejecuta ataques físicos cuerpo a cuerpo (NpcAtacaUser).

5. **Rey Pretoriano (PRREY_AI)**:
   - Unidad comandante pasiva. No persigue ni abandona el estrado de la alcoba. Si es forzado fuera de posición por un empujón o spell, retorna inmediatamente a su coordenada real (ALCOBA1 o ALCOBA2).

---

## 4. Frontera de Desacoplamiento y Dependencias Mapeadas

praetorians.bas presenta una fuerte interdependencia con módulos de Capa 8, Capa 9 y sistemas globales de red:

### 4.1. Mapeo de Puntos de Contacto Explícitos
- **MODULO_NPCs.bas (Módulo #28)**:
  - Consumo directo de la estructura Npclist(NpcIndex).
  - Invocación de MoveNPCChar para desplazamientos paso a paso.
  - Reacción ante la muerte de NPCs en MuereNpc: Si muere el Rey (esPretoriano(NpcIndex) = 4), desata la migración de tropas sobrevivientes mediante Invent.ArmourEqpSlot y respawnea el clan completo con CrearClanPretoriano. Si muere un soldado, decrementa pretorianosVivos.

- **SistemaCombate.bas (Módulo #22)**:
  - Contiene la regla de invulnerabilidad del Rey (SistemaCombate.bas:710-711 y 1788-1789): Si un jugador o mascota intenta atacar al Rey Pretoriano (PRKING_NPC) mientras pretorianosVivos > 0, el ataque es bloqueado emitiendo la advertencia: * Debes matar al resto del ejército antes de atacar al rey!*.

- **ModAreas.bas (Módulo #17) y modSendData.bas (Módulo #15)**:
  - Invocación de SendData(SendTarget.ToNPCArea, ...) para gritos de combate overhead (PrepareMessageChatOverHead), efectos visuales FX (PrepareMessageCreateFX) y animaciones de movimiento (PrepareMessageCharacterMove).

- **Dependencias con UserList (Capa 9)**:
  - Lectura de flags de usuarios (UserList(PJIndex).flags.invisible, .Oculto, .Muerto, .Paralizado, .AdminPerseguible).
  - Filtrado de clases mediante EsMagoOClerigo(PJIndex).

---

## 5. Jerarquía, Formaciones y Lógica Espacial

La fortaleza pretoriana se estructura alrededor de dos alcobas principales fijas en el mapa MAPA_PRETORIANO:

- **Alcoba 1**: Coordenadas centro (X=35, Y=25).
- **Alcoba 2**: Coordenadas centro (X=67, Y=25).

### 5.1. Migración de Alcobas y Rutas Espaciales
Cuando el Rey de una alcoba es asesinado, los pretorianos supervivientes entran en estado de retirada y cambian de alcoba. Para evitar atascamientos en el mapa, CambiarAlcoba implementa una máquina de estados espacial de 8 pasos basada en **waypoints predefinidos**:

1. Waypoint 1: (73, 56)
2. Waypoint 2: (48, 70)
3. Waypoint 3: (31, 48)
4. Waypoint 4: Reorganización al alcanzar destino.
5. Waypoint 5: (31, 56)
6. Waypoint 7: (73, 48)
8. Waypoint 8: Ingreso final a Alcoba 2/1.

---

## 6. Quirks Históricos, Asimetrías y Bugs Detectados

Durante la auditoría se identificaron las siguientes anomalías históricas relevantes:

### 6.1. Hack de Almacenamiento de Estado en Slots de Inventario
- **Quirk / Abuso de Campos DTO**: En lugar de definir campos específicos en la estructura 
pc para la IA pretoriana, el módulo reutiliza campos inactivos del inventario de NPCs:
  - Npclist(npcind).Invent.ArmourEqpSlot: Almacena el número de waypoint activo (1..8) durante la rutina CambiarAlcoba.
  - Npclist(npcind).Invent.BarcoSlot: Funciona como contador regresivo de rondas/ticks para la autodestrucción de la vara del Mago (MagoDestruyeWand).
- **Tratamiento en C++**: Debe formalizarse mediante miembros nativos de estado dentro de la clase/estructura NpcAIContext o PraetorianState, erradicando el uso espurio de slots de inventario.

### 6.2. Algoritmo Voraz GreedyWalkTo y Bucles de Persecución
- GreedyWalkTo opera calculando la delta Manhattan directa hacia la meta e intentando avanzar celda a celda. Ante obstáculos complejos o esquinas cerradas en L, el NPC queda oscillation-locked (rebotando entre dos celdas).
- No utiliza el motor PathFinding.bas (Módulo #8) para evitar sobrecarga de CPU, lo que genera bloqueos de IA si un jugador se posiciona detrás de un objeto o columna.

### 6.3. Búsqueda Espacial Cuadrática en Memoria
- Rutinas como PRMAGO_AI y PRCAZA_AI escanean la grilla espacial mediante bucles anidados For X = ... For Y = ... accediendo directamente a MapData(Map, X, Y). En escenarios de alta densidad de NPCs o ticks acelerados del servidor, estos escaneos generan caídas de performance por degradación O(N*M).

---

## 7. Propuesta Preliminar de Callback-Hooks para Desacoplamiento en C++

Para portar el módulo a C++ garantizando arquitectura limpia y sin acoplamientos circulares, se propone abstraer la frontera de praetorians mediante una interfaz de callbacks o listeners:

`cpp
namespace ao::ai {

struct IPraetorianHostBridge {
    virtual ~IPraetorianHostBridge() = default;
    
    virtual bool is_tile_blocked(int16_t map, int16_t x, int16_t y) const = 0;
    virtual void move_npc(int16_t npc_index, uint8_t heading) = 0;
    virtual void cast_spell_on_user(int16_t npc_index, int16_t user_index, int16_t spell_id) = 0;
    virtual void attack_user_melee(int16_t npc_index, int16_t user_index) = 0;
    virtual void broadcast_npc_overhead(int16_t npc_index, std::string_view message, uint32_t color) = 0;
    virtual void broadcast_npc_fx(int16_t npc_index, int16_t fx_id, uint16_t loops) = 0;
};

} // namespace ao::ai
`

---

## 8. Conclusión

La auditoría técnica del Módulo #30: praetorians.bas ha completado la catalogación total de sus 30 rutinas, máquinas de estado por rol, patrones de soporte mutuo y acoplamientos con la arquitectura del servidor. Toda la documentación resultante ha quedado consolidada para la fase posterior de transliteración y diseño modular en C++.
