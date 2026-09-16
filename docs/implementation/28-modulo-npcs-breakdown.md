# Plan de Desglose Modular — Módulo #28: `MODULO_NPCs.bas` (Capa 8)

> **Estado**: Planificación Aprobada — Pendiente de Ejecución  
> **Área**: Capa 8 (Criaturas e IA)  
> **Documentación Relacionada**: [`docs/audit/13a-modulonpcs-detalle.md`](../audit/13a-modulonpcs-detalle.md), [`docs/implementation/00-port-plan.md`](00-port-plan.md), [`docs/CONVENTIONS.md`](../CONVENTIONS.md), [`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md)  
> **Archivos a Crear**: `src/server/MODULO_NPCs.hpp`, `src/server/MODULO_NPCs.cpp`, `tests/test_modulo_npcs.cpp`, `docs/implementation/28-modulo-npcs.md`

---

## 1. Resumen de Objetivos y Arquitectura

El presente documento establece el plan de desglose modular en **5 fases lógicas secuenciales** para la migración del módulo de gestión de criaturas y mascotas `legacy/server/Codigo/MODULO_NPCs.bas` (~1.095 líneas de VB6) a C++20 (`src/server/MODULO_NPCs.hpp` y `src/server/MODULO_NPCs.cpp`).

### Decisiones Arquitectónicas Vinculantes

1. **Desacoplamiento Mediante Estructura de Callbacks (`NpcCallbacks`)**:
   - Siguiendo la convención del proyecto (Regla #1), se prohíbe el uso de jerarquías OOP virtuales (`INpc...`).
   - Las dependencias hacia capas superiores (Capas 9 a 11: `Modulo_UsUaRiOs`, `mdParty`, `ModFacciones`, `ModAreas`, `modNuevoTimer`, `Protocol`) se inyectan mediante una estructura limpia `NpcCallbacks` en el namespace `MODULO_NPCs` con funciones `std::function`:
     ```cpp
     namespace MODULO_NPCs {
         struct NpcCallbacks {
             std::function<void(std::int16_t user_index, std::int32_t exp, std::int16_t map, std::int16_t x, std::int16_t y)> party_obtener_exito;
             std::function<void(std::int16_t user_index, const std::string& msg, std::int16_t font_type)> console_msg;
             std::function<void(std::int16_t user_index)> expulsar_faccion_real;
             std::function<void(std::int16_t user_index)> expulsar_faccion_caos;
             std::function<void(std::int16_t user_index)> refresh_char_status;
             std::function<void(std::int16_t user_index)> check_user_level;
             std::function<void(std::int16_t user_index)> perdio_npc;
             std::function<bool(std::int16_t owner)> intervalo_perdio_npc;
             std::function<void(std::int16_t x)> crear_clan_pretoriano;
             std::function<void(npc& npc_ref, bool is_pretoriano)> npc_tirar_items;
             std::function<void(const WorldPos& pos, const Obj& item)> tirar_item_al_piso;
             std::function<void(std::int16_t npc_index, eHeading heading)> check_update_needed_npc;
             std::function<void(std::int16_t npc_index)> agregar_npc;
             std::function<void(SendTarget target, std::int16_t target_index, const std::vector<std::uint8_t>& data)> send_data;
             std::function<void(std::int16_t socket_or_user)> flush_buffer;
         };

         void SetCallbacks(const NpcCallbacks& cb);
     }
     ```

2. **Preservación Estricta de Paridad y Quirks Históricos**:
   - **Bug #27 (`GiveGLD` Inerte en NPCs no Pretorianos)**: En `MuereNpc`, la llamada a `NPCTirarOro` se mantiene inerte/comentada, preservando la regla de que la clave `GiveGLD` en `NPCs.dat` es ignorada para criaturas estándar.
   - **Tope de Frags a 32.000**: Preservación del chequeo `If .Stats.NPCsMuertos < 32000 Then .Stats.NPCsMuertos = .Stats.NPCsMuertos + 1` en `MuereNpc`.
   - **Barrido Cuadrático de Pretorianos**: Replicación exacta del bucle `For i = 8 To 90` / `For j = 8 To 90` sobre `MapData` cuando cae el Rey Pretoriano (`esPretoriano(NpcIndex) = 4`).
   - **Desalojo Forzado de Caspers (Fantasmas)**: Replicación exacta de la permuta espacial en `MoveNPCChar` cuando un NPC intenta pisar la casilla de un usuario muerto (`UserIndex > 0`), actualizando `UserIndex` en `MapData`, emitiendo `WriteForceCharMove` con `InvertHeading` y verificando la restricción de cambio de superficie `HayAgua`.
   - **Indexación Base 1**: Mantenimiento de la indexación 1-based para `Npclist(1..MAXNPCS)` e inventario `Invent.Object(1..30)`.

---

## 2. Plan de Implementación por Fases

---

### Fase 1: G1 — Infraestructura, Resets y Gestión de Slots

#### Componentes
- Estructuración del namespace `MODULO_NPCs` y registro de callbacks `NpcCallbacks`.
- Implementación de las rutinas de blanqueo y reseteo de estructuras de datos:
  - `Private Sub ResetNpcFlags(ByVal NpcIndex As Integer)`
  - `Private Sub ResetNpcCounters(ByVal NpcIndex As Integer)`
  - `Private Sub ResetNpcCharInfo(ByVal NpcIndex As Integer)`
  - `Private Sub ResetNpcCriatures(ByVal NpcIndex As Integer)`
  - `Sub ResetExpresiones(ByVal NpcIndex As Integer)`
  - `Private Sub ResetNpcMainInfo(ByVal NpcIndex As Integer)`
- Implementación del gestor de ranuras e instanciación en memoria:
  - `Function NextOpenNPC() As Integer`
  - `Public Function OpenNPC(ByVal NpcNumber As Integer, Optional ByVal Respawn = True) As Integer`
  - `Public Sub QuitarNPC(ByVal NpcIndex As Integer)`

#### Criterios de Aceptación y Verificación
- `NextOpenNPC` localiza linealmente el primer índice inactivo.
- `OpenNPC` deserializa campos desde `clsIniReader` (`NPCs.dat`), incrementa `NumNPCs` y actualiza `LastNPC`.
- `QuitarNPC` resetea la memoria de la entidad, decrementa `NumNPCs` y ajusta `LastNPC` descendiendo adecuadamente.

---

### Fase 2: G2 — Instanciación Espacial y Gráfica

#### Componentes
- Implementación de verificaciones de mapa y colocación física:
  - `Private Function TestSpawnTrigger(Pos As WorldPos, Optional PuedeAgua As Boolean = False) As Boolean`
  - `Sub CrearNPC(NroNPC As Integer, mapa As Integer, OrigPos As WorldPos)`
  - `Function SpawnNpc(ByVal NpcIndex As Integer, Pos As WorldPos, ByVal FX As Boolean, ByVal Respawn As Boolean) As Integer`
  - `Sub ReSpawnNpc(MiNPC As npc)`
- Vinculación con la grilla gráfica y difusión de red:
  - `Public Sub MakeNPCChar(ByVal toMap As Boolean, sndIndex As Integer, NpcIndex As Integer, ByVal Map As Integer, ByVal X As Integer, ByVal Y As Integer)`
  - `Public Sub ChangeNPCChar(ByVal NpcIndex As Integer, ByVal body As Integer, ByVal Head As Integer, ByVal heading As eHeading)`
  - `Private Sub EraseNPCChar(ByVal NpcIndex As Integer)`

#### Criterios de Aceptación y Verificación
- `CrearNPC` ejecuta el bucle de hasta 100 intentos (`MAXSPAWNATTEMPS`) validando `Not HayPCarea` y `TestSpawnTrigger`, con fallback a `altpos`, `(50,50)` vía `ClosestLegalPos` o remoción con `QuitarNPC`.
- `MakeNPCChar` vincula `CharList` y `MapData`, notificando al área o invocando `AgregarNpc`.
- `EraseNPCChar` libera `CharList`, blanquea `MapData` y actualiza `LastChar` y `NumChars`.

---

### Fase 3: G3 — Movimiento y Desalojo de Caspers

#### Componentes
- Implementación de la lógica de desplazamiento y colisiones:
  - `Public Sub MoveNPCChar(ByVal NpcIndex As Integer, ByVal nHeading As Byte)`

#### Lógica Detallada de Desalojo de Fantasmas
1. Evalúa `LegalPosNPC(.Pos.Map, nPos.X, nPos.Y, .flags.AguaValida = 1, .MaestroUser <> 0)`.
2. Si la casilla destino contiene a un usuario (`UserIndex > 0`):
   - Valida que no cruce entre agua y tierra (`HayAgua`).
   - Mueve al usuario muerto a la casilla previa del NPC en `MapData`.
   - Emite `PrepareMessageCharacterMove` hacia los demás usuarios del área (`ToPCAreaButIndex`).
   - Emite `WriteForceCharMove(UserIndex, InvertHeading(nHeading))` directamente al usuario.
3. Actualiza la casilla del NPC en `MapData`, modifica su orientación e invoca `CheckUpdateNeededNpc(NpcIndex, nHeading)`.

#### Criterios de Aceptación y Verificación
- Permuta correcta de posiciones con usuarios muertos.
- Bloqueo de cruce si el movimiento implica cambio de superficie no permitido.
- Fallback para resetting de ruta si el NPC tiene IA de Pathfinding y encuentra el camino bloqueado.

---

### Fase 4: G4 — Ciclo de Muerte, Recompensas y Mascotas

#### Componentes
- Implementación del deceso de criaturas y entrega de recompensas:
  - `Sub MuereNpc(ByVal NpcIndex As Integer, ByVal UserIndex As Integer)`
  - `Private Sub NPCTirarOro(ByRef MiNPC As npc)` *(Inerte / Código muerto)*
- Mecánicas de envenenamiento y mascotas:
  - `Sub NpcEnvenenarUser(ByVal UserIndex As Integer)`
  - `Sub QuitarMascota(ByVal UserIndex As Integer, ByVal NpcIndex As Integer)`
  - `Sub QuitarMascotaNpc(ByVal Maestro As Integer)`
  - `Public Sub QuitarPet(ByVal UserIndex As Integer, ByVal NpcIndex As Integer)`
  - `Public Sub ValidarPermanenciaNpc(ByVal NpcIndex As Integer)`

#### Criterios de Aceptación y Verificación
- `MuereNpc` ejecuta el barrido cuadrático (`8 To 90`) para pretorianos si cae el Rey Pretoriano.
- Otorgamiento de experiencia vía `mdParty.ObtenerExito` o directa al usuario (con tope `MAXEXP`).
- Incremento de `NPCsMuertos` respetando la cota de 32.000.
- Despacho de botín delegando en `NPC_TIRAR_ITEMS(MiNPC, IsPretoriano)` (Bug #27).
- Re-generación vía `ReSpawnNpc`.

---

### Fase 5: G5 — Rutinas de Seguimiento, Integración y Suite Doctest

#### Componentes
- Rutinas finales de IA de seguimiento:
  - `Public Sub DoFollow(ByVal NpcIndex As Integer, ByVal UserName As String)`
  - `Public Sub FollowAmo(ByVal NpcIndex As Integer)`
- Integración en el sistema de compilación:
  - Registro de `src/server/MODULO_NPCs.hpp` y `src/server/MODULO_NPCs.cpp` en `CMakeLists.txt`.
- Creación de la suite de pruebas unitarias:
  - `tests/test_modulo_npcs.cpp`

#### Cobertura Doctest Diseñada
- Pruebas de ciclo de vida completo: `OpenNPC` -> `CrearNPC` -> `MoveNPCChar` -> `MuereNpc` -> `QuitarNPC`.
- Verificación de desalojo forzado de caspers en `MoveNPCChar`.
- Verificación de paridad en Bug #27 (`GiveGLD` inerte para NPCs normales).
- Pruebas de límites de `LastNPC`, `NumNPCs` y tope 32.000 de `NPCsMuertos`.

---

## 3. Conclusión y Próximos Pasos

Una vez aprobado este plan de desglose modular, se procederá secuencialmente a la implementación en C++ de las Fases 1 a 5 conforme al flujo formal del proyecto.
