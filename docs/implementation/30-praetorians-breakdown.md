# Plan de Desglose Modular — Módulo #30: praetorians.bas (Capa 8)

> **Estado**: Planificación Pendiente de Aprobación  
> **Área**: Capa 8 (Criaturas e IA)  
> **Documentación Relacionada**: [docs/audit/13c-praetorians-detalle.md](../audit/13c-praetorians-detalle.md), [docs/implementation/00-port-plan.md](00-port-plan.md), [docs/CONVENTIONS.md](../CONVENTIONS.md), [docs/implementation/KNOWN-LEGACY-BUGS.md](KNOWN-LEGACY-BUGS.md)  
> **Archivos a Crear**: src/server/praetorians.hpp, src/server/praetorians.cpp, 	ests/test_praetorians.cpp, docs/implementation/30-praetorians-breakdown.md  

---

## 1. Resumen de Objetivos y Arquitectura

El presente documento establece el plan de desglose modular en **4 fases lógicas secuenciales** para la migración de la Inteligencia Artificial cooperativa de los Pretorianos legacy/server/Codigo/praetorians.bas (2.065 líneas de VB6) a C++20 (src/server/praetorians.hpp y src/server/praetorians.cpp).

### Decisiones Arquitectónicas Vinculantes

1. **Ubicación y Namespace Estricto**:
   - Los archivos de cabecera e implementación se ubicarán en src/server/praetorians.hpp y src/server/praetorians.cpp bajo el namespace praetorians.
   - Se prohíbe crear subcarpetas auxiliares o alterar el nombre del módulo.

2. **Desacoplamiento Plano Mediante Estructura PraetorianCallbacks**:
   - Toda interacción con capas superiores o sistemas no migrados (UserList, SistemaCombate, modHechizos, ModAreas, modSendData, logs, etc.) se canaliza mediante una estructura plana PraetorianCallbacks compuesta exclusivamente por miembros std::function con nombres en **PascalCase estricto**.
   - Quedan estrictamente prohibidas las clases intermedias (resolvers, dispatchers, interfaces abstractas como IPraetorianHostBridge, etc.).

   `cpp
   namespace praetorians {
       struct PraetorianCallbacks {
           std::function<void(std::int16_t npc_index, std::uint8_t heading)> MoveNPCChar;
           std::function<void(std::int16_t npc_index)> QuitarNPC;
           std::function<void(std::int16_t npc_number, std::int16_t map, const WorldPos& pos)> CrearNPC;
           std::function<void(const WorldPos& pos, WorldPos& npos, bool check_water, bool check_land)> ClosestLegalPos;
           std::function<bool(std::int16_t map, std::int16_t x, std::int16_t y)> LegalPos;
           std::function<void(std::int16_t npc_index, std::int16_t target_user)> NpcAtacaUser;
           std::function<void(std::int16_t npc_index, std::int16_t target_user, std::int16_t spell_index)> NpcLanzaSpellSobreUser;
           std::function<void(std::int16_t send_target, std::int16_t target_index, const std::string& msg)> SendData;
           std::function<std::string(std::int16_t char_index, std::int16_t x, std::int16_t y)> PrepareMessageCharacterMove;
           std::function<std::string(const std::string& text, std::int16_t char_index, std::int32_t color)> PrepareMessageChatOverHead;
           std::function<std::string(std::int16_t char_index, std::int16_t fx, std::int16_t loops)> PrepareMessageCreateFX;
           std::function<void(std::int16_t user_index, const std::string& msg, std::int16_t font_type)> WriteConsoleMsg;
           std::function<void(std::int16_t npc_index, std::int16_t attacker_user)> MuereNpc;
           std::function<void(const std::string& error_msg)> LogError;
           std::function<std::int32_t(std::int32_t min, std::int32_t max)> RandomNumber;
       };

       void SetCallbacks(const PraetorianCallbacks& cb);
       void ResetCallbacks();
   }
   `

3. **Preservación Literal del Estado en Slots DTO**:
   - Se prohíbe crear estructuras o clases auxiliares como NpcAIContext o PraetorianState.
   - Se preserva literalmente el almacenamiento de estado legacy sobre los campos de inventario:
     * Npclist[npc].Invent.ArmourEqpSlot: Almacena el waypoint activo ( \dots 8$) para la rutina CambiarAlcoba.
     * Npclist[npc].Invent.BarcoSlot: Almacena el contador regresivo de rondas para la autodestrucción de la vara del Mago en MagoDestruyeWand.

4. **Preservación Literal de Algoritmos y Quirks**:
   - GreedyWalkTo: Preservación literal del algoritmo de aproximación voraz celda a celda (con sus oscilaciones y bloqueos en esquinas en  L).
   - Escaneo Espacial Cuadrático: Preservación literal de los bucles de inspección For X = ... For Y = ... sobre MapData.
   - Consumo de MAPA_PRETORIANO: Se consume directamente desde Declares.hpp (cargado en FileIO.cpp). Queda prohibido redefinirlo.

5. **Estrategia de Compilación y Testeo Temprano**:
   - La integración en CMakeLists.txt (server_core y unit_tests) y la creación de la suite 	ests/test_praetorians.cpp se realizan desde la Fase 1 (G1).

---

## 2. Plan de Implementación por Fases

---

### Fase 1: G1 — Infraestructura, Callbacks y Navegación Espacial Básica

#### Componentes
- Definición de PraetorianCallbacks y funciones de registro/reseteo (SetCallbacks, ResetCallbacks).
- Constantes públicas de clase pretoriana (PRCLER_NPC = 900, PRGUER_NPC = 901, PRMAGO_NPC = 902, PRCAZA_NPC = 903, PRKING_NPC = 904).
- Constantes de coordenadas para Alcoba 1 (ALCOBA1_X = 35, ALCOBA1_Y = 25) y Alcoba 2 (ALCOBA2_X = 67, ALCOBA2_Y = 25).
- Variable global compartida pretorianosVivos.
- Rutinas de consulta e inspección espacial:
  * Function esPretoriano(ByVal NpcIndex As Integer) As Integer
  * Function EstoyMuyLejos(ByVal npcind As Integer) As Boolean (distancia > 22 celdas).
  * Function EstoyLejos(ByVal npcind As Integer) As Boolean (distancia > 12 celdas).
  * Function EsAlcanzable(ByVal npcind As Integer, ByVal PJEnInd As Integer) As Boolean (línea de visión y bloqueos).
- Rutinas de gestión de fantasmas:
  * Function CasperBlock(ByVal npc As Integer) As Boolean
  * Sub LiberarCasperBlock(ByVal npcind As Integer)
- Modificación de CMakeLists.txt agregando src/server/praetorians.cpp a server_core y 	ests/test_praetorians.cpp a unit_tests.

#### Criterios de Aceptación y Verificación
- esPretoriano mapea correctamente los 5 IDs de NPC hostil a su código de rol (1..5) y 0 para otros NPCs.
- EstoyLejos y EstoyMuyLejos calculan correctamente los radios de alejamiento respecto al centro de la alcoba activa.
- CasperBlock detecta usuarios muertos en la celda adyacente de avance y LiberarCasperBlock invoca la relocalización.
- cmake --build build compila limpiamente y ctest --output-on-failure ejecuta 	est_praetorians con la suite G1 pasando al 100%.

---

### Fase 2: G2 — Movimiento Cardinal, Voraz y Rutas de Waypoints

#### Componentes
- Primitivas de movimiento paso a paso:
  * Sub MoverAba(ByVal npcorig As Integer)
  * Sub MoverArr(ByVal npcorig As Integer)
  * Sub MoverIzq(ByVal npcorig As Integer)
  * Sub MoverDer(ByVal npcorig As Integer)
- Algoritmo de aproximación voraz:
  * Sub GreedyWalkTo(ByVal npcorig As Integer, ByVal Map As Integer, ByVal X As Integer, ByVal Y As Integer)
- Reposicionamiento y migración de escuadrón entre alcobas:
  * Sub VolverAlCentro(ByVal npcind As Integer)
  * Public Sub CambiarAlcoba(ByVal npcind As Integer) (secuencia íntegra de 8 waypoints alternando Invent.ArmourEqpSlot).

#### Criterios de Aceptación y Verificación
- GreedyWalkTo reduce la distancia Manhattan hacia las coordenadas destino celda a celda.
- CambiarAlcoba avanza secuencialmente el campo Invent.ArmourEqpSlot desde el waypoint 1 hasta el 8 al alcanzar cada posición intermedia.
- Pruebas unitarias en 	ests/test_praetorians.cpp verificando el comportamiento de navegación, atascamiento y cambio de alcoba.

---

### Fase 3: G3 — Magia Táctica, Soporte Aliado e Inmolación

#### Componentes
- Identificación de clases del jugador:
  * Function EsMagoOClerigo(ByVal PJEnInd As Integer) As Boolean
- Soporte mutual y restauración entre pretorianos:
  * Sub NPCRemueveVenenoNPC(ByVal npcind As Integer, ByVal NPCAlInd As Integer, ByVal indice As Integer)
  * Sub NPCCuraLevesNPC(ByVal npcind As Integer, ByVal NPCAlInd As Integer, ByVal indice As Integer)
  * Sub NPCRemueveParalisisNPC(ByVal npcind As Integer, ByVal NPCAlInd As Integer, ByVal indice As Integer)
  * Sub NPCcuraNPC(ByVal curador As Integer, ByVal curado As Integer, ByVal indice As Integer)
- Ofensiva y control de masas contra usuarios:
  * Sub NPCparalizaNPC(ByVal paralizador As Integer, ByVal Paralizado As Integer, ByVal indice As Integer)
  * Sub NPCLanzaCegueraPJ(ByVal npcind As Integer, ByVal PJEnInd As Integer, ByVal indice As Integer)
  * Sub NPCLanzaEstupidezPJ(ByVal npcind As Integer, ByVal PJEnInd As Integer, ByVal indice As Integer)
  * Sub NPCRemueveInvisibilidad(ByVal npcind As Integer, ByVal PJEnInd As Integer, ByVal indice As Integer)
  * Sub NpcLanzaSpellSobreUser2(ByVal NpcIndex As Integer, ByVal UserIndex As Integer, ByVal Spell As Integer)
- Inmolación y sobrecarga del Mago:
  * Sub MagoDestruyeWand(ByVal npcind As Integer, ByVal bs As Byte, ByVal indice As Integer) (manejo del contador en Invent.BarcoSlot).

#### Criterios de Aceptación y Verificación
- NPCRemueveInvisibilidad cancela exitosamente los flags invisible y Oculto del usuario blanco.
- MagoDestruyeWand decrementa Invent.BarcoSlot y al alcanzar 0 invoca MuereNpc.
- Pruebas unitarias en 	ests/test_praetorians.cpp validando la lógica de hechizos y efectos visuales salientes.

---

### Fase 4: G4 — Máquinas de Estado por Rol, Formación y Spawning

#### Componentes
- Rutinas principales de toma de decisiones por rol:
  * Sub PRREY_AI(ByVal npcind As Integer)
  * Sub PRGUER_AI(ByVal npcind As Integer)
  * Sub PRCAZA_AI(ByVal npcind As Integer)
  * Sub PRMAGO_AI(ByVal npcind As Integer)
  * Sub PRCLER_AI(ByVal npcind As Integer)
- Generación de la formación militar completa:
  * Sub CrearClanPretoriano(ByVal X As Integer) (spawnhea 1 Rey, 2 Sacerdotes, 3 Guerreros, 1 Cazador, 1 Mago e inicializa pretorianosVivos = 7).

#### Criterios de Aceptación y Verificación
- CrearClanPretoriano ubica los 8 NPCs en la alcoba opuesta con sus tipos y posiciones relativas exactas.
- PRCLER_AI prioriza la curación/desparalización de aliados sobre el ataque a enemigos.
- PRMAGO_AI pasa a estado de destrucción de vara cuando Stats.MinHp < 750.
- Suite completa en 	ests/test_praetorians.cpp validando la ejecución armónica del clan pretoriano.

---

## 3. Matriz de Cobertura de Rutinas (30/30)

| Routine Legacy | Fase | Cabecera C++ |
| :--- | :-: | :--- |
| esPretoriano | G1 | std::int16_t esPretoriano(std::int16_t npc_index); |
| EstoyMuyLejos | G1 | ool EstoyMuyLejos(std::int16_t npc_index); |
| EstoyLejos | G1 | ool EstoyLejos(std::int16_t npc_index); |
| EsAlcanzable | G1 | ool EsAlcanzable(std::int16_t npc_index, std::int16_t user_index); |
| CasperBlock | G1 | ool CasperBlock(std::int16_t npc_index); |
| LiberarCasperBlock | G1 | oid LiberarCasperBlock(std::int16_t npc_index); |
| MoverAba | G2 | oid MoverAba(std::int16_t npc_index); |
| MoverArr | G2 | oid MoverArr(std::int16_t npc_index); |
| MoverIzq | G2 | oid MoverIzq(std::int16_t npc_index); |
| MoverDer | G2 | oid MoverDer(std::int16_t npc_index); |
| GreedyWalkTo | G2 | oid GreedyWalkTo(std::int16_t npc_index, std::int16_t map, std::int16_t x, std::int16_t y); |
| VolverAlCentro | G2 | oid VolverAlCentro(std::int16_t npc_index); |
| CambiarAlcoba | G2 | oid CambiarAlcoba(std::int16_t npc_index); |
| EsMagoOClerigo | G3 | ool EsMagoOClerigo(std::int16_t user_index); |
| NPCRemueveVenenoNPC | G3 | oid NPCRemueveVenenoNPC(std::int16_t npc_index, std::int16_t ally_npc_index, std::int16_t spell_slot); |
| NPCCuraLevesNPC | G3 | oid NPCCuraLevesNPC(std::int16_t npc_index, std::int16_t ally_npc_index, std::int16_t spell_slot); |
| NPCRemueveParalisisNPC | G3 | oid NPCRemueveParalisisNPC(std::int16_t npc_index, std::int16_t ally_npc_index, std::int16_t spell_slot); |
| NPCcuraNPC | G3 | oid NPCcuraNPC(std::int16_t healer_npc, std::int16_t target_npc, std::int16_t spell_slot); |
| NPCparalizaNPC | G3 | oid NPCparalizaNPC(std::int16_t caster_npc, std::int16_t target_index, std::int16_t spell_slot); |
| NPCLanzaCegueraPJ | G3 | oid NPCLanzaCegueraPJ(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_slot); |
| NPCLanzaEstupidezPJ | G3 | oid NPCLanzaEstupidezPJ(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_slot); |
| NPCRemueveInvisibilidad | G3 | oid NPCRemueveInvisibilidad(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_slot); |
| NpcLanzaSpellSobreUser2 | G3 | oid NpcLanzaSpellSobreUser2(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_index); |
| MagoDestruyeWand | G3 | oid MagoDestruyeWand(std::int16_t npc_index, std::uint8_t wand_counter, std::int16_t spell_slot); |
| PRREY_AI | G4 | oid PRREY_AI(std::int16_t npc_index); |
| PRGUER_AI | G4 | oid PRGUER_AI(std::int16_t npc_index); |
| PRCAZA_AI | G4 | oid PRCAZA_AI(std::int16_t npc_index); |
| PRMAGO_AI | G4 | oid PRMAGO_AI(std::int16_t npc_index); |
| PRCLER_AI | G4 | oid PRCLER_AI(std::int16_t npc_index); |
| CrearClanPretoriano | G4 | oid CrearClanPretoriano(std::int16_t previous_king_x); |

---

## 4. Estrategia de Verificación y Compilación

- **Compilación Progresiva**:
  Se ejecutará cmake --build build al finalizar cada fase.
- **Suite de Tests Unitarios**:
  Se mantendrá 	ests/test_praetorians.cpp con cobertura de cada fase. Se ejecutará mediante ctest --output-on-failure.
