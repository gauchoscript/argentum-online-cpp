# Módulo #28: MODULO_NPCs (`MODULO_NPCs.bas`)

## Estado Formal
**✅ COMPLETADO (Aislado / Cableado Pendiente)**

---

## 1. Alcance y Responsabilidades del Módulo

El módulo `MODULO_NPCs` gestiona la totalidad del ciclo de vida y estado runtime de las entidades no-jugador (NPCs y mascotas/pets) en el servidor de Argentum Online:

- **Infraestructura de Slots y Resets**: Control e inicialización de la tabla `Npclist(1..MAXNPCS)` y sus slots de inventario `Invent.Object(1..30)`.
- **Instanciación Espacial y Visual**: Carga de plantillas desde `NPCs.dat` (`OpenNPC`), generación física con búsquedas de celdas transitables (`CrearNPC`, `SpawnNpc`, `ReSpawnNpc`), y sincronización con el motor gráfico `CharList` y `MapData` (`MakeNPCChar`, `ChangeNPCChar`, `EraseNPCChar`).
- **Desplazamiento y Permuta de Caspers**: Manejo de movimiento (`MoveNPCChar`) incluyendo el desalojo forzado de caspers (jugadores muertos) hacia la celda vaciada por el NPC, respetando barreras de superficie (`HayAgua`).
- **Ciclo de Muerte y Recompensas**: Procesamiento de decesos (`MuereNpc`), distribución de experiencia e incremento de reputación, ejecuciones pretorianas en cuadrícula (`8 To 90`), y despacho de botín vía `NPC_TIRAR_ITEMS`.
- **Gestión de Mascotas y Persecución**: Control de criaturas subordinadas (`QuitarMascota`, `QuitarMascotaNpc`, `QuitarPet`, `ValidarPermanenciaNpc`) y rutinas de seguimiento de IA (`DoFollow`, `FollowAmo`).

---

## 2. Decisiones Técnicas y Paridad con Quirks Legacy

### 2.1 Desacoplamiento mediante `NpcCallbacks`
Siguiendo las reglas del proyecto, no se crearon jerarquías virtuales de clases (`INpc...`). El desacoplamiento con módulos de red y mapas se resolvió mediante la estructura `NpcCallbacks`, garantizando firmas puras en C++20 con miembros `std::function` en PascalCase (`PartyObtenerExito`, `WriteConsoleMsg`, `NPC_TIRAR_ITEMS`, `CheckUpdateNeededNpc`, etc.).

### 2.2 Réplica Estricta del Bug #27 (`GiveGLD` Inerte)
En la rutina `MuereNpc`, la invocación a `NPCTirarOro` se encuentra históricamente comentada en VB6. Se conservó `NPCTirarOro` como función inerte y el botín se canaliza únicamente mediante la llamada a `s_callbacks.NPC_TIRAR_ITEMS(mi_npc, is_pretoriano)`. Por consiguiente, los NPCs no pretorianos descartan por completo el valor `GiveGLD` de `NPCs.dat` al morir.

### 2.3 Cota Aritmética de 32.000 Frags (`NPCsMuertos`)
Se preservó de forma exacta la guarda condicional:
```cpp
if (user.Stats.NPCsMuertos < 32000) {
    user.Stats.NPCsMuertos++;
}
```
Esta guarda previene desbordamientos de enteros con signo de 16 bits (`Integer` en VB6) sin alterar el límite histórico del juego.

### 2.4 Permuta de Caspers y Frontera de Agua en `MoveNPCChar`
Cuando un NPC se desplaza a una casilla ocupada por un jugador muerto (`UserIndex > 0`), el servidor permuta la posición del casper a la celda previamente ocupada por el NPC, notificando al usuario mediante `WriteForceCharMove(InvertHeading(n_heading))`. Si el movimiento cruza la frontera entre superficie terrestre y acuática (`HayAgua`), la permuta es abortada para evitar que caspers o criaturas queden atrapados en terrenos no válidos.

### 2.5 Barrido Cuadrático de Pretorianos (`8 To 90`)
Al morir el Rey Pretoriano (`Numero = 904`), `MuereNpc` ejecuta un doble bucle cuadrático literal desde `X = 8` hasta `90` y `Y = 8` hasta `90` sobre `MAPA_PRETORIANO` para reajustar los slots de equipamiento de las tropas sobrevivientes y otorgar el control del clan pretoriano.

---

## 3. Mapeo de Procedimientos y Cobertura Doctest

| Procedimiento C++ | Firma / Definición | Cobertura Doctest |
| :--- | :--- | :--- |
| `NextOpenNPC` | `std::int16_t NextOpenNPC() noexcept` | Asigna primer slot libre en `Npclist(1..MAXNPCS)`. |
| `OpenNPC` | `std::int16_t OpenNPC(std::int16_t npc_number, bool respawn)` | Carga plantilla `NPCs.dat` y configura atributos. |
| `QuitarNPC` | `void QuitarNPC(std::int16_t npc_index)` | Libera gráficos, borra inventarios y ajusta `LastNPC`/`NumNPCs`. |
| `CrearNPC` | `std::int16_t CrearNPC(...)` | Cascada de 100 intentos en `pos +/- 3` con fallbacks `altpos` y `(50,50)`. |
| `SpawnNpc` | `std::int16_t SpawnNpc(...)` | Instanciación con FX/sonidos de invocación. |
| `ReSpawnNpc` | `void ReSpawnNpc(npc& mi_npc)` | Regeneración de criaturas en posición original. |
| `MakeNPCChar` | `void MakeNPCChar(...)` | Enlace a `CharList`, `MapData` y emisión de paquetes `WriteCharacterCreate`. |
| `ChangeNPCChar` | `void ChangeNPCChar(...)` | Actualización de cuerpo, cabeza y orientación con difusión de área. |
| `EraseNPCChar` | `void EraseNPCChar(...)` | Remoción de entidad visual del mapa y ajuste de `LastChar`/`NumChars`. |
| `MoveNPCChar` | `void MoveNPCChar(...)` | Desplazamiento en mapa, permuta de caspers y cancelación por `HayAgua`. |
| `MuereNpc` | `void MuereNpc(...)` | Procesa exp, reputación, Bug #27, cota 32k y deceso de Rey Pretoriano. |
| `NpcEnvenenarUser` | `void NpcEnvenenarUser(...)` | Probabilidad del 30% de aplicar veneno al objetivo. |
| `QuitarMascota` | `void QuitarMascota(...)` | Desvincula mascota del arreglo `UserList.MascotasIndex`. |
| `QuitarPet` | `void QuitarPet(...)` | Remueve la entidad física y limpia la referencia del usuario. |
| `DoFollow` | `void DoFollow(...)` | Conmuta persecución e IA (`TipoAI::SigueAmo` vs `TipoAI::NPCMAmbula`). |
| `FollowAmo` | `void FollowAmo(...)` | Fuerza estado de persecución hacia el amo. |

---

## 4. Verificación de Compilación y Suite Doctest

El módulo fue verificado mediante la suite unitaria en `tests/test_modulo_npcs.cpp`:

```txt
===============================================================================
[doctest] test cases:  346 |  346 passed | 0 failed | 0 skipped
[doctest] assertions: 5648 | 5648 passed | 0 failed |
[doctest] Status: SUCCESS!
```
