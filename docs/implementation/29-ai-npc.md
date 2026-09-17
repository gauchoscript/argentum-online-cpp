# Especificación Técnica — Módulo #29: `AI_NPC`

## 1. Estado de la Migración
`✅ COMPLETADO (Aislado / Cableado Pendiente en Capas 9 y 10 / GameLogic)`

El módulo legacy `legacy/server/Codigo/AI_NPC.bas` de la **Capa 8 (Inteligencia Artificial)** ha sido portado íntegramente a C++20 bajo la arquitectura desacoplada por callbacks. La implementación reside de forma autónoma en:
- `src/server/AI_NPC.hpp`
- `src/server/AI_NPC.cpp`

Toda interacción con entidades de usuario (`UserList`), combate (`SistemaCombate`), magia (`modHechizos`), escaneo de mapas por área (`ModAreas`), protocolo de red y logs se encuentra desacoplada mediante la estructura plana de callbacks `AiNpcCallbacks`.

---

## 2. Catálogo de Procedimientos Implementados

Se migró la totalidad de las 19 rutinas de `AI_NPC.bas`:

| # | Procedimiento C++ | Visibilidad | Firma / Tipo Retorno | Descripción |
|---|-------------------|-------------|-----------------------|-------------|
| 1 | `NpcLanzaUnSpell` | `Public` | `void(std::int16_t npc_index, std::int16_t user_index)` | Selecciona y ejecuta un hechizo aleatorio de la lista del NPC sobre un usuario. |
| 2 | `NpcLanzaUnSpellSobreNpc` | `Public` | `void(std::int16_t npc_index, std::int16_t target_npc)` | Selecciona y ejecuta un hechizo aleatorio de la lista del NPC sobre otro NPC. |
| 3 | `UserNear` | `Public` | `bool(std::int16_t npc_index)` | Determina si el usuario objetivo del pathfinding está a distancia adyacente ($\le 1$). |
| 4 | `ReCalculatePath` | `Public` | `bool(std::int16_t npc_index)` | Evalúa si corresponde recalcular la trayectoria BFS hacia el usuario objetivo. |
| 5 | `PathEnd` | `Public` | `bool(std::int16_t npc_index)` | Verifica si se alcanzó el paso final de la lista de waypoints en `PFINFO`. |
| 6 | `FollowPath` | `Public` | `bool(std::int16_t npc_index)` | Avanza un paso en la grilla siguiendo los waypoints de `PFINFO.Path`. |
| 7 | `PathFindingAI` | `Public` | `bool(std::int16_t npc_index)` | Escanea un área de $10 \times 10$ baldosas para hallar blanco e invocar `SeekPath` (Módulo #27). |
| 8 | `GuardiasAI` | `Public` | `void(std::int16_t npc_index, bool del_caos)` | Escaneo de agresión adyacente para Guardias Reales y del Caos. |
| 9 | `HostilMalvadoAI` | `Public` | `void(std::int16_t npc_index, bool del_caos)` | Escaneo de agresión adyacente para criaturas hostiles malvadas contra ciudadanos/armada. |
| 10 | `HostilBuenoAI` | `Public` | `void(std::int16_t npc_index)` | Escaneo de agresión adyacente para criaturas hostiles buenas contra criminales/caos. |
| 11 | `RestoreOldMovement` | `Public` | `void(std::int16_t npc_index)` | Restablece `.Movement` y `.Hostile` a sus valores originales (`OldMovement` / `OldHostil`). |
| 12 | `AiNpcObjeto` | `Public` | `void(std::int16_t npc_index)` | Comportamiento estático/objeto interactivo de criaturas estatuas con azar 2/3. |
| 13 | `IrUsuarioCercano` | `Public` | `void(std::int16_t npc_index)` | Búsqueda y avance direccional en rango visual $8 \times 6$ hacia usuarios vulnerables. |
| 14 | `SeguirAgresor` | `Public` | `void(std::int16_t npc_index)` | Persecución del agresor registrado en `AttackedBy` (con regla de seguro de mascota). |
| 15 | `PersigueCiudadano` | `Public` | `void(std::int16_t npc_index)` | Persecución de usuarios ciudadanos/armables por parte de criaturas del caos. |
| 16 | `PersigueCriminal` | `Public` | `void(std::int16_t npc_index)` | Persecución de usuarios criminales por parte de Guardias Reales. |
| 17 | `SeguirAmo` | `Public` | `void(std::int16_t npc_index)` | Persecución del amo (`MaestroUser`) cuando la distancia es $> 3$ baldosas. |
| 18 | `AiNpcAtacaNpc` | `Public` | `void(std::int16_t npc_index)` | Búsqueda y agresión inter-NPC (incluyendo frenesí Elemental de Fuego vs Dragón). |
| 19 | `NPCAI` | `Public` | `void(std::int16_t npc_index)` | **Orquestador central del tick de IA**. Despacha agresión, movimiento y recupera excepciones. |

---

## 3. Arquitectura de Desacoplamiento (`AiNpcCallbacks`)

Toda la comunicación externa del módulo `AI_NPC` está strictly aislada en la estructura `AiNpcCallbacks`:

```cpp
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
```

---

## 4. Quirks Históricos y Reglas de Dominio Replicadas

1. **Paridad de Operadores en `UserNear`**:
   - Para evitar la alteración de la evaluación booleana por la precedencia del operador `Not Int(Distance) > 1` de VB6, en C++ se escribe explícitamente:
     `!(static_cast<int>(dist) > 1)`

2. **Entrada #45 (Inversión de Coordenadas X/Y en Pathfinding)**:
   - En `PathFindingAI`, se replica la inversión deliberada hacia la estructura BFS:
     `Npclist[npc_index].PFINFO.Target.X = npc_ref.Pos.Y;`
     `Npclist[npc_index].PFINFO.Target.Y = npc_ref.Pos.X;`
   - En `FollowPath`, se lee desinvirtiendo la coordenada para el mapa de movimiento:
     `tmpPos.X = npc_ref.PFINFO.Path[cur_pos].Y;`
     `tmpPos.Y = npc_ref.PFINFO.Path[cur_pos].X;`

3. **Exclusión de Ataque Melee para Elemental de Agua (`92`)**:
   - En `SeguirAgresor`, cuando la criatura persigue al atacante registrado, si `Numero == ELEMENTALAGUA` (92), se omite el ataque cuerpo a cuerpo (`NpcAtacaUser`).

4. **Frenesí Mágico Elemental de Fuego (`93`) vs Dragón (`13`)**:
   - En `AiNpcAtacaNpc`, cuando un Elemental de Fuego combate a un Dragón, le dispara un hechizo y fuerza instantáneamente `Npclist[ni].CanAttack = 1` en el Dragón para forzar la respuesta mágica.

5. **Regla de Seguro de Mascotas**:
   - Si una mascota de un jugador ciudadano o Armada Real atacó a otro ciudadano pacífico con seguro activado, rehúsa atacar, envía el mensaje `"La mascota no atacará a ciudadanos si eres miembro del ejército real o tienes el seguro activado."`, limpia `AttackedBy` y regresa al amo (`FollowAmo`).

6. **Manejo de Excepciones Destructivo (`ErrorHandler` legacy)**:
   - La función `NPCAI` encapsula todo el ciclo en un bloque `try / catch` recuperativo que invoca `LogError`, remueve la entidad con `QuitarNPC(npc_index)` y la respawnea mediante `ReSpawnNpc(copy_npc)`.

---

## 5. Cobertura de Pruebas Unitarias (Doctest)

La suite `tests/test_ai_npc.cpp` contiene 13 clases de prueba exhaustivas con **35 aserciones** que validan la totalidad de las 4 fases:
- Magia de NPCs y selección aleatoria de Spells.
- Búsqueda de caminos BFS, paridad `UserNear` y Bug #45.
- Escaneo espacial de Guardias Reales/Caos y criaturas hostiles.
- Control de mascotas, seguro de ciudadanos y exclusión de Elemental de Agua.
- Frenesí inter-NPC (Elemental de Fuego vs Dragón).
- Orquestador central `NPCAI` y recuperador de excepciones.

---

## 6. Verificación Global
- **Compilación**: `server_core` y `unit_tests` compilan sin advertencias.
- **Pruebas Doctest**: 360 test cases en verde (0 fallidos), 5704 aserciones completadas exitosamente.
