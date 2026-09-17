# Plan de Desglose Modular — Módulo #29: `AI_NPC.bas` (Capa 8)

> **Estado**: Planificación Pendiente de Aprobación  
> **Área**: Capa 8 (Criaturas e IA)  
> **Documentación Relacionada**: [`docs/audit/13b-ainpc-detalle.md`](../audit/13b-ainpc-detalle.md), [`docs/implementation/00-port-plan.md`](00-port-plan.md), [`docs/CONVENTIONS.md`](../CONVENTIONS.md), [`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md)  
> **Archivos a Crear**: `src/server/AI_NPC.hpp`, `src/server/AI_NPC.cpp`, `tests/test_ai_npc.cpp`, `docs/implementation/29-ai-npc.md`  

---

## 1. Resumen de Objetivos y Arquitectura

El presente documento establece el plan de desglose modular en **4 fases lógicas secuenciales** para la migración de la inteligencia artificial de criaturas `legacy/server/Codigo/AI_NPC.bas` (1.069 líneas de VB6) a C++20 (`src/server/AI_NPC.hpp` y `src/server/AI_NPC.cpp`).

### Decisiones Arquitectónicas Vinculantes

1. **Ubicación y Namespace Estricto**:
   - Los archivos de cabecera e implementación se ubicarán en `src/server/AI_NPC.hpp` y `src/server/AI_NPC.cpp` bajo el namespace `AI_NPC`.
   - Se prohíbe crear subcarpetas auxiliares o alterar el nombre del módulo.

2. **Desacoplamiento Plano Mediante Estructura `AiNpcCallbacks`**:
   - Toda interacción con capas superiores o sistemas no migrados (`UserList`, `SistemaCombate`, `modHechizos`, `ModAreas`, mensajes de consola, logs) se canaliza mediante una estructura plana `AiNpcCallbacks` compuesta exclusivamente por punteros a función `std::function` con nombres en **PascalCase estricto**.
   - Quedan estrictamente prohibidas las clases intermedias (resolvers, dispatchers, etc.).
   
   ```cpp
   namespace AI_NPC {
       struct AiNpcCallbacks {
           std::function<bool(std::int16_t user_index)> IntervaloPermiteSerAtacado;
           std::function<bool(std::int16_t user_index)> EsCriminal;
           std::function<bool(std::int16_t npc_index, std::int16_t user_index)> NpcAtacaUser;
           std::function<void(std::int16_t npc_index, std::int16_t target_npc, bool is_pet)> NpcAtacaNpc;
           std::function<void(std::int16_t npc_index, std::int16_t user_index, std::int16_t spell_id)> NpcLanzaSpellSobreUser;
           std::function<void(std::int16_t npc_index, std::int16_t target_npc, std::int16_t spell_id)> NpcLanzaSpellSobreNpc;
           std::function<void(std::int16_t user_index, const std::string& msg, std::int16_t font_type)> WriteConsoleMsg;
           std::function<void(std::int16_t user_index)> FlushBuffer;
           std::function<void(const std::string& error_msg)> LogError;
           std::function<std::int32_t(std::int32_t min, std::int32_t max)> RandomNumber;
           std::function<std::uint8_t(const WorldPos& from, const WorldPos& to)> FindDirection;
           std::function<void(std::uint8_t heading, WorldPos& pos)> HeadtoPos;
           std::function<bool(std::int16_t map, std::int16_t x, std::int16_t y)> InMapBounds;
           std::function<double(const WorldPos& pos1, const WorldPos& pos2)> Distancia;
           std::function<const std::vector<std::int16_t>&(std::int16_t map)> GetConnGroupUserEntries;
           std::function<std::int16_t(std::int16_t map, std::int16_t x, std::int16_t y)> GetMapUserIndex;
           std::function<std::int16_t(std::int16_t map, std::int16_t x, std::int16_t y)> GetMapNpcIndex;
           std::function<void(std::int16_t npc_index, std::int16_t body, std::int16_t head, std::uint8_t heading)> ChangeNPCChar;
           std::function<void(std::int16_t npc_index, std::uint8_t heading)> MoveNPCChar;
           std::function<void(std::int16_t npc_index)> QuitarNPC;
           std::function<void(const npc& npc_data)> ReSpawnNpc;
           std::function<void(std::int16_t npc_index)> FollowAmo;
           std::function<void(std::int16_t npc_index)> SeekPath;
           std::function<const User&(std::int16_t user_index)> GetUser;
       };

       void SetCallbacks(const AiNpcCallbacks& cb);
       void ResetCallbacks();
   }
   ```

3. **Paridad de Operadores y Precisión de Evaluación**:
   - En `UserNear`, para evitar la inversión de precedencia de operadores de C++ y replicar exactamente el comportamiento de VB6 (`Not Int(Distance(...)) > 1`), se escribirá explícitamente: `!(static_cast<int>(dist) > 1)`.

4. **Replicación Estricta de Quirks Históricos**:
   - **Bug #45 (Inversión X/Y en `PFINFO`)**: En `PathFindingAI` y `FollowPath`, mantener el intercambio deliberado de coordenadas `Target.X = Pos.Y` y `Target.Y = Pos.X`.
   - **Elemental de Agua (`Numero = 92`)**: Exclusión explícita de ataque cuerpo a cuerpo directo al seguir agresores (`Npclist(NpcIndex).Numero != 92`).
   - **Elemental de Fuego (`Numero = 93`) vs Dragón (`13`)**: Provocación de frenesí mágico con `CanAttack = 1` en `AiNpcAtacaNpc`.

---

## 2. Plan de Implementación por Fases

---

### Fase 1: G1 — Infraestructura, Magia y Utilidades de Pathfinding

#### Componentes
- Definición de `AiNpcCallbacks`, funciones de configuración (`SetCallbacks`, `ResetCallbacks`).
- Implementación de lanzamiento aleatorio de conjuros:
  - `Sub NpcLanzaUnSpell(ByVal NpcIndex As Integer, ByVal UserIndex As Integer)`
  - `Sub NpcLanzaUnSpellSobreNpc(ByVal NpcIndex As Integer, ByVal TargetNPC As Integer)`
- Implementación de rutinas auxiliares de búsqueda y trayectoria BFS:
  - `Function UserNear(ByVal NpcIndex As Integer) As Boolean` (con `!(static_cast<int>(dist) > 1)`).
  - `Function ReCalculatePath(ByVal NpcIndex As Integer) As Boolean`
  - `Function PathEnd(ByVal NpcIndex As Integer) As Boolean`
  - `Function FollowPath(ByVal NpcIndex As Integer) As Boolean` (con inversión `tmpPos.X = Path.Y`, `tmpPos.Y = Path.X`).
  - `Function PathFindingAI(ByVal NpcIndex As Integer) As Boolean` (con ventana 10x10 y `Target.X = Pos.Y`, `Target.Y = Pos.X`).

#### Criterios de Aceptación y Verificación
- Lanzamiento de magia con selección uniforme entre `1` y `flags.LanzaSpells`.
- `UserNear` devuelve `true` solo para usuarios inmediatamente adyacentes ($\le 1$).
- `PathFindingAI` invierte adecuadamente las coordenadas al setear `PFINFO.Target` antes de invocar `SeekPath`.

---

### Fase 2: G2 — Escaneo Adyacente, Centinelas y Objetos Mágicos

#### Componentes
- Escaneos adyacentes de 4 direcciones:
  - `Private Sub GuardiasAI(ByVal NpcIndex As Integer, ByVal DelCaos As Boolean)`
  - `Private Sub HostilMalvadoAI(ByVal NpcIndex As Integer)`
  - `Private Sub HostilBuenoAI(ByVal NpcIndex As Integer)`
- Restablecimiento de postura e IA estática/objeto:
  - `Private Sub RestoreOldMovement(ByVal NpcIndex As Integer)`
  - `Public Sub AiNpcObjeto(ByVal NpcIndex As Integer)`

#### Criterios de Aceptación y Verificación
- `GuardiasAI` distingue correctamente entre Guardias Reales (`DelCaos = False`, atacan criminales) y Guardias del Caos (`DelCaos = True`, atacan ciudadanos).
- `HostilMalvadoAI` evalúa adecuadamente el flag `UserProtected` y aplica un 50% de probabilidad de casteo de hechizo antes de atacar.
- `RestoreOldMovement` restablece `.Movement` y `.Hostile` únicamente si el NPC no posee amo (`MaestroUser == 0`).
- `AiNpcObjeto` ejecuta el casteo con probabilidad 2/3 sobre usuarios no protegidos.

---

### Fase 3: G3 — Búsqueda de Blancos, Persecución y Mascotas

#### Componentes
- Exploración de la cuadrícula de visión (`RANGO_VISION_X = 8`, `RANGO_VISION_Y = 6`):
  - `Private Sub IrUsuarioCercano(ByVal NpcIndex As Integer)` (prioridad de `Owner` sobre extraños y escaneo direccional cuando está inmovilizado).
- Persecución defensiva y faccionaria:
  - `Private Sub SeguirAgresor(ByVal NpcIndex As Integer)` (con verificación de seguro de amo para Armada Real/ciudadanos y omisión de melee para Elemental de Agua `92`).
  - `Private Sub PersigueCiudadano(ByVal NpcIndex As Integer)`
  - `Private Sub PersigueCriminal(ByVal NpcIndex As Integer)`
- Control de criaturas e inter-combat de NPCs:
  - `Private Sub SeguirAmo(ByVal NpcIndex As Integer)` (umbral de distancia $> 3$ tiles).
  - `Private Sub AiNpcAtacaNpc(ByVal NpcIndex As Integer)` (frenesí Elemental de Fuego `93` vs Dragón `13`).

#### Criterios de Aceptación y Verificación
- Mascotas pertenecientes a ciudadanos o Armada Real rehusando atacar a otros ciudadanos cuando el amo tiene el seguro activado.
- Elemental de Agua (`92`) excluido de ataques melee cuerpo a cuerpo directos durante la persecución.
- Frenesí activado entre Elemental de Fuego (`93`) y Dragón (`13`) forzando `CanAttack = 1`.
- Mascotas siguiendo al amo únicamente cuando la distancia entre ambos supera 3 baldosas.

---

### Fase 4: G4 — Orquestador Central `NPCAI`, CMake y Suite Doctest Integral

#### Componentes
- Rutina maestra periódica:
  - `Sub NPCAI(ByVal NpcIndex As Integer)`
  - Bloque `try / catch` recuperativo con fallback a `QuitarNPC` y `ReSpawnNpc` replicando el `ErrorHandler` legacy.
- Integración en CMake:
  - Inclusión de `src/server/AI_NPC.hpp` y `src/server/AI_NPC.cpp` en `CMakeLists.txt`.
- Suite de pruebas unitarias:
  - `tests/test_ai_npc.cpp`

#### Cobertura Doctest Diseñada
- Prueba de despacho de `NPCAI` según cada valor de `TipoAI` (`MueveAlAzar`, `NpcMaloAtacaUsersBuenos`, `NPCDEFENSA`, `GuardiasAtacanCriminales`, `SigueAmo`, `NpcAtacaNpc`, `NpcObjeto`, `NpcPathfinding`).
- Prueba de inmunidad de blancos (`UserProtected` por invi, oculto, muerto, ignorado o consulta).
- Prueba de combate faccionario (Guardias Reales persiguiendo criminales, Guardias del Caos persiguiendo ciudadanos).
- Prueba de la paridad en `UserNear` con `!(static_cast<int>(dist) > 1)`.
- Prueba del quirk $X \leftrightarrow Y$ en la búsqueda e iteración de trayectoria BFS.

---

## 3. Conclusión y Próximos Pasos

Tras la aprobación del usuario de este plan de desglose modular, se procederá secuencialmente con la implementación C++ de las Fases 1 a 4.
