# Auditoría Técnica: Módulo de Gestión y Ciclo de Vida de Criaturas (`MODULO_NPCs.bas`)

## Resumen Ejecutivo

Este documento presenta el análisis técnico exhaustivo y la auditoría formal del Módulo #28 (`legacy/server/Codigo/MODULO_NPCs.bas`), perteneciente a la Capa 8 del servidor de Argentum Online v0.13.0 y subordinado a la Macro-Área 13 (**Criaturas e IA**).

El módulo `MODULO_NPCs.bas` es el núcleo responsable del ciclo de vida de las entidades no-jugador (NPCs y mascotas/pets): desde su instanciación (`OpenNPC`, `CrearNPC`, `SpawnNpc`), sincronización con la grilla espacial (`MapData`) y el motor gráfico (`CharList`), hasta el procesamiento de movimiento (`MoveNPCChar`), muerte y distribución de recompensas (`MuereNpc`), y recolección de basura/liberación de slots (`QuitarNPC`).

---

## 1. Catálogo Exhaustivo de Procedimientos y Firmas

El módulo `MODULO_NPCs.bas` contiene 26 procedimientos en total (11 públicos, 6 privados y 9 con visibilidad pública implícita por omisión de modificador en VB6).

| # | Procedimiento | Visibilidad | Líneas | Propósito Funcional |
| :-: | :--- | :---: | :-: | :--- |
| 1 | `QuitarMascota` | Pública | 47-65 | Desvincula una mascota del arreglo `UserList(UserIndex).MascotasIndex` y decrementa `NroMascotas`. |
| 2 | `QuitarMascotaNpc` | Pública | 67-75 | Decrementa el contador de mascotas de un NPC maestro (`Npclist(Maestro).Mascotas`). |
| 3 | `MuereNpc` | Pública | 77-231 | Procesa el deceso de un NPC: eventos pretorianos, experiencia, frags, reputación, facciones, drops y respawn. |
| 4 | `ResetNpcFlags` | Privada | 233-268 | Restablece los flags de estado (`AfectaParalisis`, `Envenenado`, `invisible`, `Paralizado`, etc.) a 0. |
| 5 | `ResetNpcCounters` | Privada | 270-281 | Restablece los temporizadores de parálisis y tiempo de existencia a 0. |
| 6 | `ResetNpcCharInfo` | Privada | 283-301 | Blanquea la estructura gráfica `.Char` (body, head, heading, charIndex, animaciones). |
| 7 | `ResetNpcCriatures` | Privada | 303-320 | Limpia la lista de criaturas subordinadas de un entrenador. |
| 8 | `ResetExpresiones` | Pública | 322-338 | Limpia el arreglo dinámico de expresiones de diálogo del NPC. |
| 9 | `ResetNpcMainInfo` | Privada | 340-392 | Blanquea los atributos base del NPC, desvincula maestros y reinicia sub-estructuras. |
| 10 | `QuitarNPC` | Pública | 394-435 | Desinstancia un NPC: borra gráfico, limpia inventario, libera slot y actualiza `LastNPC` y `NumNPCs`. |
| 11 | `QuitarPet` | Pública | 437-471 | Elimina una mascota de un usuario y remueve la entidad del juego vía `QuitarNPC`. |
| 12 | `TestSpawnTrigger` | Privada | 473-487 | Valida que una celda no contenga triggers de mapa 1, 2 o 3 antes del spawn. |
| 13 | `CrearNPC` | Pública | 489-603 | Instancia un NPC en el mapa buscando una posición válida y no visible por jugadores (`Not HayPCarea`). |
| 14 | `MakeNPCChar` | Pública | 605-628 | Asigna un `CharIndex`, vincula la entidad a `CharList` y `MapData`, e informa el gráfico al cliente/área. |
| 15 | `ChangeNPCChar` | Pública | 630-646 | Notifica un cambio de apariencia (cuerpo, cabeza, orientación) a los clientes del área. |
| 16 | `EraseNPCChar` | Privada | 648-678 | Remueve la representación gráfica del NPC del mapa, decrementa `NumChars` y desasigna `CharIndex`. |
| 17 | `MoveNPCChar` | Pública | 680-744 | Desplaza un NPC a una celda adyacente; gestiona colisiones y desaloja forzadamente a caspers (usuarios muertos). |
| 18 | `NextOpenNPC` | Pública | 746-766 | Busca linealmente la primera ranura libre en `Npclist(1..MAXNPCS)`. |
| 19 | `NpcEnvenenarUser` | Pública | 768-782 | Aplica probabilidad (30%) de envenenar a un usuario atacado por una criatura venenosa. |
| 20 | `SpawnNpc` | Pública | 784-855 | Invocación de creación con efectos visuales/sonoros opcionales (warp FX/SND). |
| 21 | `ReSpawnNpc` | Pública | 857-866 | Invoca `CrearNPC` para regenerar la criatura en su punto de origen si `flags.Respawn = 0`. |
| 22 | `NPCTirarOro` | Privada | 868-892 | Arroja oro en pilas de hasta 10.000 monedas (**Código inerte / inactivo** en 0.13.0). |
| 23 | `OpenNPC` | Pública | 894-1051 | Lee los datos base de un NPC desde `NPCs.dat` (vía `clsIniReader`) y lo carga en `Npclist`. |
| 24 | `DoFollow` | Pública | 1053-1073 | Alterna el modo persecución (`Follow`) de un NPC hacia un objetivo específico. |
| 25 | `FollowAmo` | Pública | 1075-1089 | Configura la IA del NPC para seguir a su amo (`TipoAI.SigueAmo`). |
| 26 | `ValidarPermanenciaNpc` | Pública | 1091-1097 | Verifica si transcurrió el intervalo para que un dueño pierda el control de su mascota. |

---

## 2. Estructuras de Datos y Gestión de Estado

### 2.1 Ciclo de Vida del Arreglo `Npclist`

El estado de todas las criaturas activas se almacena en el arreglo global `Npclist(1 To MAXNPCS) As npc`.

```
                  ┌────────────────────────┐
                  │ OpenNPC(NpcNumber)     │
                  │ (Asigna slot libre)    │
                  └───────────┬────────────┘
                              │
                              ▼
                  ┌────────────────────────┐
                  │ CrearNPC / SpawnNpc    │
                  │ (Asigna Posición/Char) │
                  └───────────┬────────────┘
                              │
            ┌─────────────────┴─────────────────┐
            │                                   │
            ▼                                   ▼
┌───────────────────────┐           ┌───────────────────────┐
│ MoveNPCChar           │           │ ChangeNPCChar         │
│ (Desplazamiento)      │           │ (Modifica Apariencia) │
└───────────┬───────────┘           └───────────┬───────────┘
            │                                   │
            └─────────────────┬─────────────────┘
                              │
                              ▼
                  ┌────────────────────────┐
                  │ MuereNpc               │
                  │ (Procesa drop/exp/gold)│
                  └───────────┬────────────┘
                              │
                              ▼
                  ┌────────────────────────┐
                  │ QuitarNPC              │
                  │ (EraseChar/Reset/Slot) │
                  └────────────────────────┘
```

1. **Búsqueda de Slots (`NextOpenNPC`)**:
   Itera linealmente desde `1` hasta `MAXNPCS + 1`. Retorna el primer índice donde `Npclist(LoopC).flags.NPCActive` es `False`. Si no hay ranuras libres, retorna `MAXNPCS + 1`.
2. **Punteros de Monitoreo (`LastNPC` y `NumNPCs`)**:
   - `OpenNPC` incrementa `NumNPCs` y actualiza `LastNPC = NpcIndex` si el índice asignado es superior al máximo actual.
   - `QuitarNPC` decrementa `NumNPCs` y recorre en sentido descendente `LastNPC` hasta encontrar la nueva criatura activa más alta.
3. **Liberación y Reset (`ResetNpcMainInfo` y `QuitarNPC`)**:
   `QuitarNPC` desactiva `flags.NPCActive = False`, borra la entidad visual, limpia el inventario (`ResetNpcInv`), restablece los contadores (`ResetNpcCounters`), los flags (`ResetNpcFlags`) y la información principal (`ResetNpcMainInfo`).

### 2.2 Gestión de Inventario y Drops

- **Inventario Activo (`Npclist(NpcIndex).Invent`)**:
  Contiene hasta `MAX_INVENTORY_SLOTS` (30 ranuras) de tipo `tInvent`. Cargado desde `NPCs.dat` en `OpenNPC` (`NROITEMS` y `Obj1..N`).
- **Drops de Muerte (`Npclist(NpcIndex).Drop(1..5)`)**:
  Contiene hasta `MAX_NPC_DROPS` (5 ranuras) cargadas en `OpenNPC`. Al morir la criatura, `MuereNpc` delega el arrojado de objetos a `NPC_TIRAR_ITEMS(MiNPC, IsPretoriano)`.

### 2.3 Acoplamiento con el Motor Gráfico (`CharList` y `CharIndex`)

El motor gráfico de Argentum Online mapea entidades mediante `CharList(1 To LoopL) As Long`.
- **Asignación (`MakeNPCChar`)**:
  Si `Npclist(NpcIndex).Char.CharIndex` es 0, solicita un nuevo índice mediante `NextOpenCharIndex`, vincula `CharList(CharIndex) = NpcIndex` y asigna `MapData(Map, X, Y).NpcIndex = NpcIndex`.
- **Remoción (`EraseNPCChar`)**:
  Desvincula `CharList(CharIndex) = 0`, limpia `MapData(Pos).NpcIndex = 0`, notifica la remoción a los clientes con `PrepareMessageCharacterRemove` y ajusta el límite global `LastChar`.

---

## 3. Frontera de Red y Acoplamiento Espacial

### 3.1 Interacción con la Grilla Espacial (`MapData`)

1. **Ocupación de Celda**:
   La presencia física del NPC en el mundo está gobernada estrictamente por `MapData(Map, X, Y).NpcIndex`.
2. **Desalojo Forzado de Caspers (Usuarios Muertos)** en `MoveNPCChar`:
   Cuando un NPC camina hacia una celda legal que contiene a un usuario muerto (`UserIndex > 0`), el servidor desplaza al jugador a la posición anterior que ocupaba el NPC:
   ```vb
   MapData(.Pos.Map, .Pos.X, .Pos.Y).UserIndex = 0
   .Pos.X = Npclist(NpcIndex).Pos.X
   .Pos.Y = Npclist(NpcIndex).Pos.Y
   MapData(.Pos.Map, .Pos.X, .Pos.Y).UserIndex = UserIndex
   ```
   Notifica a los observadores mediante `PrepareMessageCharacterMove` y fuerza al cliente con `WriteForceCharMove(UserIndex, InvertHeading(nHeading))`. Se prohíbe explícitamente el desplazamiento si la permuta cruza los límites de superficie entre agua y tierra (`HayAgua`).
3. **Triggers de Terreno (`TestSpawnTrigger`)**:
   Verifica que la casilla de generación no contenga triggers de mapa prohibidos (1: Zona Segura/Teleport, 2: Anti-Monster/Trigger, 3: Trigger 3).

### 3.2 Sincronización Espacial y Difusión de Red (`ModAreas` y `Protocol`)

`MODULO_NPCs.bas` no interactúa directamente con los sockets TCP, sino que construye y despacha paquetes de protocolo a través de `modSendData` y `Protocol`:
- **Creación de Entidad**: `MakeNPCChar` invoca `AgregarNpc(NpcIndex)` en `ModAreas` (si `toMap = True`) o `WriteCharacterCreate` (si se despacha a una conexión directa).
- **Desplazamiento**: `MoveNPCChar` invoca `CheckUpdateNeededNpc(NpcIndex, nHeading)` en `ModAreas` y emite `PrepareMessageCharacterMove` hacia `SendTarget.ToNPCArea`.
- **Efectos y Modificaciones**: `ChangeNPCChar` emite `PrepareMessageCharacterChange`, `SpawnNpc` emite `PrepareMessageCreateFX` y `PrepareMessagePlayWave`.

---

## 4. Taxonomía de Hallazgos y Quirks Históricos

### 4.1 Bug #27: `GiveGLD` Inerte para NPCs no Pretorianos

- **Cita Legacy**: `legacy/server/Codigo/MODULO_NPCs.bas:219, 868-892` y `legacy/server/Codigo/Modulo_InventANDobj.bas:97-98`.
- **Descripción**: En la rutina `MuereNpc`, la llamada a `NPCTirarOro(MiNPC)` se encuentra comentada (`' Call NPCTirarOro(MiNPC)`). Por ende, la función `NPCTirarOro` es código completamente inerte. La rutina sustituta `NPC_TIRAR_ITEMS` solo evalúa y tira `.GiveGLD` si la criatura es de tipo pretoriano (`If IsPretoriano Then ... If .GiveGLD > 0 Then Call TirarOroNpc(.GiveGLD, .Pos)`). Para el resto de los NPCs estándar del juego, el valor asignado a la clave `GiveGLD` en `NPCs.dat` es totalmente descartado al morir y jamás cae al suelo.

### 4.2 Límite Aritmético en Contador de Frags de NPCs (`NPCsMuertos`)

- **Cita Legacy**: `legacy/server/Codigo/MODULO_NPCs.bas:134-135`.
- **Descripción**: En `MuereNpc`, al incrementar el contador de criaturas asesinadas del usuario:
  ```vb
  If .Stats.NPCsMuertos < 32000 Then _
      .Stats.NPCsMuertos = .Stats.NPCsMuertos + 1
  ```
  La cota de 32.000 evita que el tipo entero de 16 bits firmado de VB6 (`Integer`, rango max 32.767) sufra un desbordamiento aritmético (`Error 6: Overflow`) al acumular frags.

### 4.3 Mecanismo de Reintentos Extremos en `CrearNPC`

- **Cita Legacy**: `legacy/server/Codigo/MODULO_NPCs.bas:545-573`.
- **Descripción**: Al instanciar un NPC, `CrearNPC` intenta hasta `MAXSPAWNATTEMPS` (100 iteraciones) hallar una casilla transitable que no esté dentro de la visión de ningún usuario (`Not HayPCarea`). Si el bucle alcanza las 100 fallas, recurre a una posición alternativa `altpos`, o en su defecto intenta colocar al NPC cerca del punto central `(50,50)` usando `ClosestLegalPos`. Si esta última alternativa también falla, aborta la creación llamando a `QuitarNPC` y registrando una entrada en el log de errores.

### 4.4 Lógica de Batalla Pretoriana en `MuereNpc`

- **Cita Legacy**: `legacy/server/Codigo/MODULO_NPCs.bas:88-117`.
- **Descripción**: Cuando un NPC de tipo Rey Pretoriano (`esPretoriano(NpcIndex) = 4`) muere en el `MAPA_PRETORIANO`, se ejecuta un doble bucle cuadrático de barrido sobre las coordenadas de la grilla (`For i = 8 To 90`, `For j = 8 To 90`) para alterar la propiedad `ArmourEqpSlot` de todos los demás pretorianos vivos (modificando su alcoba/comportamiento) e invoca a `CrearClanPretoriano`.

---

## 5. Catálogo Preliminar de Desacoplamiento (Hooks)

Para mantener la Capa 8 aislada de las capas superiores de lógica de juego, interfaz de usuario y temporizadores (Capas 9 a 11), `MODULO_NPCs.bas` requiere la abstracción de las siguientes funciones mediante callbacks tipados o interfaces de eventos:

```
                          ┌────────────────────────┐
                          │   MODULO_NPCs (Capa 8) │
                          └───────────┬────────────┘
                                      │
           ┌──────────────────────────┼──────────────────────────┐
           │ (Party/Consola)          │ (Facciones/Status)       │ (Objetos/Tirado)
           ▼                          ▼                          ▼
┌─────────────────────┐    ┌─────────────────────┐    ┌─────────────────────┐
│ INpcPartyHook       │    │ INpcFactionHook     │    │ INpcDropHook        │
│ - ObtenerExito      │    │ - ExpulsarFaccion   │    │ - NpcTirarItems     │
│ - WriteConsoleMsg   │    │ - RefreshStatus     │    │ - TirarItemAlPiso   │
└─────────────────────┘    └─────────────────────┘    └─────────────────────┘
           │                          │                          │
           └──────────────────────────┼──────────────────────────┘
                                      │ (Áreas/Visibilidad)
                                      ▼
                           ┌─────────────────────┐
                           │ INpcAreaHook        │
                           │ - CheckUpdateNeeded │
                           │ - AgregarNpc        │
                           └─────────────────────┘
```

1. **Gestión de Grupos y Experiencia (`mdParty`)**:
   - `ObtenerExito(UserIndex, Exp, Map, X, Y)`: Invocado en `MuereNpc` para repartir la experiencia del NPC entre los miembros de la party.
2. **Notificaciones e Interfaz (`Protocol` / Consola)**:
   - `WriteConsoleMsg(UserIndex, Message, FontType)`: Emisión de mensajes de consola al jugador (exp ganado, muerte de criatura, envenenamiento).
3. **Sistema de Facciones y Reputación (`ModFacciones` / `Modulo_UsUaRiOs`)**:
   - `ExpulsarFaccionReal(UserIndex)` y `ExpulsarFaccionCaos(UserIndex)`: Expulsión de facción por matar guardianes o cambiar alineación.
   - `RefreshCharStatus(UserIndex)`: Actualización del estado visual de criminalidad y tags de personaje.
   - `CheckUserLevel(UserIndex)`: Evaluación de subida de nivel del jugador tras recibir la experiencia del NPC.
4. **Inventario, Drops y Suelo (`Modulo_InventANDobj`)**:
   - `NPC_TIRAR_ITEMS(Npc, IsPretoriano)`: Despacho de inventario y drops al morir el NPC.
   - `TirarItemAlPiso(Pos, Item)`: Arrojado directo de objetos o bolsas de oro.
5. **Gestión Espacial y Áreas Visibles (`ModAreas`)**:
   - `CheckUpdateNeededNpc(NpcIndex, Heading)`: Actualización de grilla de visibilidad al desplazar un NPC.
   - `AgregarNpc(NpcIndex)`: Inserción de un NPC recién creado en el gestor de áreas.
6. **Temporización de Mascotas (`modNuevoTimer`)**:
   - `IntervaloPerdioNpc(Owner)` y `PerdioNpc(Owner)`: Verificación de expiración de control sobre criaturas domadas.
7. **Eventos Pretorianos (`Acciones`)**:
   - `CrearClanPretoriano(X)`: Invocación del evento especial pretoriano al caer el rey pretoriano.

---

## 6. Conclusión y Próximos Pasos

El relevamiento de `MODULO_NPCs.bas` confirma que el módulo concentra la gestión integral de ciclo de vida de los NPCs y sus colisiones espaciales. En las fases subsiguientes del port:
1. La asimetría del **Bug #27** deberá ser preservada con estricta paridad en C++.
2. Las 26 rutinas identificadas deberán abstraer sus llamadas hacia `ModAreas`, `mdParty`, `Modulo_InventANDobj` y `ModFacciones` a través de interfaces inyectadas o callbacks tipados.
3. El informe queda oficialmente asentado en `docs/audit/13a-modulonpcs-detalle.md` y registrado en la tabla de anexos de `docs/audit/README.md`.
