# Plan de Desglose Modular — Módulo #31: Sistema de Party (`clsParty` y `mdParty`)

> **Estado**: Planificación Documental Completada  
> **Área**: Capa 9 (Sesión del Jugador, Parties y Posicionamiento)  
> **Auditoría Técnica**: [`docs/audit/11b-party-detalle.md`](../audit/11b-party-detalle.md)  
> **Ledger de Bugs**: [`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md) (Entrada #46)  
> **Plan Maestro**: [`docs/implementation/00-port-plan.md`](00-port-plan.md)  
> **Archivos Legacy**: `legacy/server/Codigo/mdParty.bas` (~502 líneas VB6) y `legacy/server/Codigo/clsParty.cls` (~491 líneas VB6)

---

## 1. Resumen Ejecutivo y Alcance del Módulo

El Módulo #31 administra el sistema de grupos tácticos (*Party*) de Argentum Online 0.13.0, gobernando la fundación de partidas, la validación de admisiones (distancia espacial y compatibilidad faccionaria), el chat interno de grupo, la transferencia de liderazgo, el reparto equitativo ponderado de experiencia por nivel y la disolución segura del grupo.

La transliteración a C++20 se compone de dos pares de archivos en `src/server/`:
- `clsParty.hpp` / `clsParty.cpp`: Clase de objeto que encapsula el estado interno del grupo (hasta 5 integrantes en slots dispersos/compactados, líder, suma ponderada de niveles y experiencia acumulada).
- `mdParty.hpp` / `mdParty.cpp`: Administrador global estático (*PartyManager*) que gestiona el vector global de parties (`Parties`), decodifica las intenciones del usuario y coordina la actualización de experiencias ante WorldSaves.

### Principios Fundamentales de Porting
1. **Acceso Directo a `UserList`**: Dado que `UserList` está disponible globalmente en la memoria del servidor (`Declares.hpp`), la lectura y mutación de propiedades del personaje (`.Stats.ELV`, `.Stats.UserSkills`, `.Stats.UserAtributos`, `.Pos`, `.flags.Muerto`, `.Faccion`, `.PartyIndex`, `.PartySolicitud`) se realiza de forma directa sobre la estructura central.
2. **Callbacks Exclusivos de Infraestructura**: La estructura `PartyCallbacks` se reserva únicamente para desacoplar el envío de paquetes de red de consola (`WriteConsoleMsg`), sincronización de estadísticas (`WriteUpdateUserStats`) y evaluación de subida de nivel (`CheckUserLevel`).
3. **Paridad Flotante Estricta Byte a Byte**: La variable interna `p_SumaNivelesElevados` **se mantiene obligatoriamente como `float`** (IEEE 754 de 32 bits) para reproducir con exactitud el comportamiento y redondeo del tipo `Single` de VB6.
4. **Replicación Literal de Bugs Históricos**: Replicación exacta de la **Entrada #46** en `KNOWN-LEGACY-BUGS.md` (sustracción errónea del nivel del líder `UserIndex` en lugar de cada integrante `p_members(j).UserIndex` en la suma ponderada durante el bucle de disolución `SaleMiembro`).

---

## 2. Contratos, Callbacks y Estructuras C++20

### 2.1. Estructuras Internas y Variables Globale

```cpp
namespace ModParty {

// Constantes de Dominio
constexpr std::int16_t MAX_PARTIES = 300;
constexpr std::uint8_t PARTY_MAXMEMBERS = 5;
constexpr std::uint8_t MAXDISTANCIAINGRESOPARTY = 2;
constexpr std::uint8_t PARTY_MAXDISTANCIA = 18;
constexpr bool PARTY_EXPERIENCIAPORGOLPE = false;

// Código muerto preservado
constexpr std::uint8_t MINPARTYLEVEL = 15;
constexpr std::uint8_t MAXPARTYDELTALEVEL = 7;
constexpr bool CASTIGOS = false;

// Estructura de integrante de party
struct tPartyMember {
    std::int16_t UserIndex = 0;
    double Experiencia = 0.0;
};

// Callbacks de infraestructura de red y nivel
struct PartyCallbacks {
    std::function<void(std::int16_t user_index, std::string_view msg, std::uint8_t font_type)> WriteConsoleMsg;
    std::function<void(std::int16_t user_index)> WriteUpdateUserStats;
    std::function<void(std::int16_t user_index)> CheckUserLevel;
};

void SetPartyCallbacks(const PartyCallbacks& callbacks);

} // namespace ModParty

// Contenedor Global de Parties (1..300)
extern std::array<std::shared_ptr<clsParty>, ModParty::MAX_PARTIES + 1> Parties;
```

---

## 3. Desglose Fase por Fase (G1 a G4)

---

### Fase G1 — Infraestructura, Estado Interno y Creación/Ingreso Base
**Objetivo**: Implementar la estructura interna de `clsParty`, los constructores, la reserva de slots globales en `Parties(1..300)`, las validaciones de atributos para fundar y la adición del líder fundador.

#### Componentes e Interfaces
- `clsParty.hpp` / `clsParty.cpp`:
  - `Class_Initialize()` / `Class_Terminate()`
  - `CantMiembros() const -> std::int16_t`
  - `ObtenerExperienciaTotal() const -> std::int32_t`
  - `EsPartyLeader(std::int16_t user_index) const -> bool`
  - `MiExperiencia(std::int16_t user_index) const -> std::int32_t`
  - `NuevoMiembro(std::int16_t user_index) -> bool`
  - `HacerLeader(std::int16_t user_index) -> bool`
- `mdParty.hpp` / `mdParty.cpp`:
  - `extern std::array<std::shared_ptr<clsParty>, MAX_PARTIES + 1> Parties;`
  - `NextParty() -> std::int16_t`
  - `PuedeCrearParty(std::int16_t user_index) -> bool`
  - `CrearParty(std::int16_t user_index)`

#### Lógica Detallada
1. **`NextParty`**: Recorre `Parties[1..300]`. Retorna el primer índice donde el puntero es `nullptr`, o `-1` si todos los slots están ocupados.
2. **`PuedeCrearParty`**: Verifica `UserList[user_index].flags.Muerto == 0` y $Carisma \times Liderazgo \ge 100$.
3. **`CrearParty`**: Valida `UserList[user_index].PartyIndex == 0` y `UserSkills[Liderazgo] >= 5`. Llama a `NextParty()`, instancia `std::make_shared<clsParty>()`, invoca `NuevoMiembro(user_index)` y `HacerLeader(user_index)`, y asigna `.PartyIndex = tInt`, `.PartySolicitud = 0`.
4. **`NuevoMiembro`**: Busca el primer slot con `UserIndex == 0` en `p_members[1..5]`. Asigna el usuario, resetea `Experiencia = 0.0`, incrementa `p_CantMiembros` y suma al acumulador flotante:
   $$p\_SumaNivelesElevados = p\_SumaNivelesElevados + \text{std::pow}(\text{float}(ELV), ExponenteNivelParty)$$
5. **`HacerLeader`** (`clsParty`): Intercambia físicamente el slot `p_members[1]` con el slot `p_members[UserIndexIndex]` (ambos `UserIndex` y `Experiencia`) y actualiza `p_Fundador = p_members[1].UserIndex`.

#### doctest Planificados (Suite `[Party_G1]`)
- `G1_NextParty_FindsFirstEmptySlot`: Verifica la asignación secuencial de slots 1..300 y retorno `-1` ante saturación.
- `G1_PuedeCrearParty_ValidatesAttributes`: Comprueba fallo con carisma/liderazgo bajo o personaje muerto.
- `G1_CrearParty_InstantiatesLeader`: Valida asignación de slot, `PartyIndex` en `UserList`, `p_Fundador` y cálculo inicial de `p_SumaNivelesElevados` como `float`.

---

### Fase G2 — Lógica de Aceptación, Distancia y Matriz Faccionaria
**Objetivo**: Implementar la solicitud de ingreso (`/PARTY`), las validaciones de reglas de juego en `PuedeEntrar` (distancia espacial y compatibilidad faccionaria/criminal) y la aprobación por el líder (`/ACEPTARPARTY`).

#### Componentes e Interfaces
- `clsParty.hpp` / `clsParty.cpp`:
  - `PuedeEntrar(std::int16_t user_index, std::string& razon) const -> bool`
- `mdParty.hpp` / `mdParty.cpp`:
  - `SolicitarIngresoAParty(std::int16_t user_index)`
  - `UserPuedeEjecutarComandos(std::int16_t user_index) -> bool`
  - `AprobarIngresoAParty(std::int16_t leader, std::int16_t new_member)`

#### Lógica Detallada
1. **`SolicitarIngresoAParty`**: Valida que el solicitante esté vivo y no tenga party. Lee `TargetUser = UserList[user_index].flags.TargetUser`. Si el target pertenece a una party, asigna `UserList[user_index].PartySolicitud = UserList[TargetUser].PartyIndex`.
2. **`UserPuedeEjecutarComandos`**: Retorna `true` únicamente si `UserList[user_index].PartyIndex > 0` y `Parties[PI]->EsPartyLeader(user_index)`.
3. **`PuedeEntrar`**:
   - Distancia espacial: `Distancia(PosLider, PosAspirante) <= 2` tiles.
   - Cupo disponible: `p_members[5].UserIndex == 0`.
   - Matriz Faccionaria y Criminal (`clsParty.cls:396-418`):
     - Aspirante Armada Real en party con al menos un Criminal -> Rechazo.
     - Aspirante Legión Oscura en party con al menos un Ciudadano -> Rechazo.
     - Aspirante Criminal en party con al menos un Armada Real -> Rechazo.
     - Aspirante Ciudadano en party con al menos un Legión Oscura -> Rechazo.
4. **`AprobarIngresoAParty`**: Valida que `UserList[new_member].PartySolicitud == PI`, esté vivo y `PartyIndex == 0`. Llama a `PuedeEntrar` y `NuevoMiembro(new_member)`.

#### doctest Planificados (Suite `[Party_G2]`)
- `G2_SolicitarIngreso_SetsPartySolicitud`: Valida la asignación del target index al solicitar ingreso.
- `G2_PuedeEntrar_DistanceValidation`: Confirma rechazo cuando la distancia supera los 2 tiles.
- `G2_PuedeEntrar_FactionMatrix`: Verifica los 4 escenarios de incompatibilidad Armada/Caos/Criminal/Ciudadano.
- `G2_AprobarIngreso_AddsMemberSuccessfully`: Valida la incorporación del nuevo integrante y reseteo de `PartySolicitud`.

---

### Fase G3 — Salida de Integrantes, Expulsión y Disolución (Con Replicación Bug #46)
**Objetivo**: Implementar la remoción voluntaria u forzada de miembros (`SaleMiembro`), la disolución total al salir el líder (con replicación de la **Entrada #46**), el traspaso de mando (`TransformarEnLider`) y la compactación de ranuras (`CompactMemberList`).

#### Componentes e Interfaces
- `clsParty.hpp` / `clsParty.cpp`:
  - `SaleMiembro(std::int16_t user_index) -> bool`
  - `CompactMemberList()` [Private]
- `mdParty.hpp` / `mdParty.cpp`:
  - `SalirDeParty(std::int16_t user_index)`
  - `ExpulsarDeParty(std::int16_t leader, std::int16_t old_member)`
  - `TransformarEnLider(std::int16_t old_leader, std::int16_t new_leader)`

#### Lógica Detallada
1. **`SaleMiembro` — Caso Salida del Líder (`i == 1`)**:
   - Retorna `true` (señalando disolución del grupo).
   - Itera en reversa sobre todos los slots (`j = 5` hasta `1`):
     - Si `p_members[j].UserIndex > 0`:
       - Acredita la experiencia acumulada `Fix(.Experiencia)` en `UserList[.UserIndex].Stats.Exp`.
       - Llama a `CheckUserLevel` y `WriteUpdateUserStats`.
       - Resetea `UserList[.UserIndex].PartyIndex = 0`.
       - Decrementa `p_CantMiembros`.
       - **Replicación de Bug #46 (`KNOWN-LEGACY-BUGS.md`)**: Sustrae erróneamente de `p_SumaNivelesElevados` la potencia del nivel del **líder** `UserIndex` en lugar de restar el nivel de `p_members[j].UserIndex`:
         $$p\_SumaNivelesElevados = p\_SumaNivelesElevados - \text{std::pow}(\text{float}(UserList[user\_index].Stats.ELV), ExponenteNivelParty)$$
2. **`SaleMiembro` — Caso Salida de Miembro (`i > 1`)**:
   - Retorna `false`.
   - Acredita la experiencia acumulada del miembro saliente en su `Stats.Exp`.
   - Decrementa `p_CantMiembros`.
   - Resta correctamente su nivel del acumulador flotante:
     $$p\_SumaNivelesElevados = p\_SumaNivelesElevados - \text{std::pow}(\text{float}(ELV_{saliente}), ExponenteNivelParty)$$
   - Resetea el slot `p_members[i]` y ejecuta `CompactMemberList()`.
3. **`CompactMemberList`**: Desplaza los slots activos (`UserIndex > 0`) hacia la izquierda para eliminar huecos intermedios en `p_members[1..5]`.
4. **`TransformarEnLider`** (`mdParty`): Valida que `old_leader` y `new_leader` compartan `PartyIndex`, `new_leader` no esté muerto, delega en `Parties[PI]->HacerLeader(new_leader)` y emite notificaciones por consola.

#### doctest Planificados (Suite `[Party_G3]`)
- `G3_SaleMiembro_NormalMember_CompactsList`: Valida la salida de un integrante común, desacreditación de niveles en `p_SumaNivelesElevados` y compactación.
- `G3_SaleMiembro_Leader_DissolvesAndReplicatesBug46`: Verifica que la salida del líder disuelva la party, transfiera la EXP a todos los miembros y ejecute la sustracción bugueada del nivel del líder (Bug #46).
- `G3_TransformarEnLider_SwapsSlotsPreservingExp`: Comprueba el intercambio de ranuras y preservación de experiencia acumulada.

---

### Fase G4 — Distribución de Experiencia, Comunicación y Flush
**Objetivo**: Implementar el cálculo matemático de ponderación de experiencia en `ObtenerExito`, el filtrado espacial (mapa, estado vivo y distancia $\le 18$ tiles), los comandos de comunicación y consulta (`BroadCastParty`, `OnlineParty`), y el flush síncrono previo a guardados de mundo.

#### Componentes e Interfaces
- `clsParty.hpp` / `clsParty.cpp`:
  - `ObtenerExito(std::int32_t exp_ganada, std::int16_t mapa, std::int16_t x, std::int16_t y)`
  - `MandarMensajeAConsola(std::string_view texto, std::string_view sender)`
  - `ObtenerMiembrosOnline(std::array<std::int16_t, 5>& member_list)`
  - `FlushExperiencia()`
  - `UpdateSumaNivelesElevados(std::int16_t lvl)`
- `mdParty.hpp` / `mdParty.cpp`:
  - `ObtenerExito(std::int16_t user_index, std::int32_t exp, std::int16_t mapa, std::int16_t x, std::int16_t y)`
  - `BroadCastParty(std::int16_t user_index, std::string_view texto)`
  - `OnlineParty(std::int16_t user_index)`
  - `ActualizaExperiencias()`
  - `ActualizarSumaNivelesElevados(std::int16_t user_index)`
  - `CantMiembros(std::int16_t user_index) -> std::int16_t`

#### Lógica Detallada
1. **`ObtenerExito`**:
   - Acumula `p_expTotal += exp_ganada`.
   - Itera sobre los 5 slots:
     $$\text{expThisUser} = \text{double}\left( \text{exp\_ganada} \times \frac{\text{std::pow}(\text{float}(ELV_i), ExponenteNivelParty)}{p\_SumaNivelesElevados} \right)$$
   - Evalúa condiciones espaciales (`mapa == UserList[UI].Pos.Map`, `UserList[UI].flags.Muerto == 0`, `Distance(...) <= 18`).
   - **Lógica Mutuamente Excluyente de Entrega**:
     - **Si `PARTY_EXPERIENCIAPORGOLPE == true`**: Acredita `std::trunc(expThisUser)` inmediatamente en `UserList[UI].Stats.Exp`, aplicando clamping a `MAXEXP` e invocando `CheckUserLevel(UI)` y `WriteUpdateUserStats(UI)`.
     - **De lo contrario (`else`)**: Acumula `p_members[i].Experiencia += expThisUser`.
2. **`FlushExperiencia`**: Recorre los slots activos y transfiere `std::trunc(p_members[i].Experiencia)` a `UserList[UI].Stats.Exp`, **preservando el remanente decimal**:
   $$p\_members[i].Experiencia = p\_members[i].Experiencia - \text{std::trunc}(p\_members[i].Experiencia)$$
   E invoca `CheckUserLevel` y `WriteUpdateUserStats`.
3. **`ActualizarSumaNivelesElevados`**: Llamado al subir de nivel en party:
   $$p\_SumaNivelesElevados = p\_SumaNivelesElevados - \text{std::pow}(\text{float}(lvl - 1), E) + \text{std::pow}(\text{float}(lvl), E)$$


#### doctest Planificados (Suite `[Party_G4]`)
- `G4_ObtenerExito_CalculatesWeightedExp`: Revalida la distribución proporcional de experiencia entre 3 miembros de distintos niveles.
- `G4_ObtenerExito_EnforcesSpatialRestrictions`: Confirma que miembros muertos, en otro mapa o a $> 18$ tiles no reciban la EXP.
- `G4_FlushExperiencia_TransfersAccumulatedExp`: Comprueba la acreditación masiva en `Stats.Exp` al ejecutar flush.
- `G4_UpdateSumaNivelesElevados_LevelUpRecalculation`: Valida la actualización de la suma ponderada al subir de nivel un integrante.

---

## 4. Plan de Pruebas Integrales y Verificación

### Pruebas Unitarias (`tests/test_party.cpp`)
Se creará `tests/test_party.cpp` con suites dedicadas para cada fase:
1. `[Party_G1]`: Inicialización, slots, creación y suma de niveles en `float`.
2. `[Party_G2]`: Admisión, matriz faccionaria y distancia.
3. `[Party_G3]`: Abandono, expulsión, disolución y **Entrada #46** (bug del líder).
4. `[Party_G4]`: Reparto proporcional de EXP, restricciones espaciales y flush.

---

## 5. Matriz de Riesgos y Mitigaciones

| Riesgo Técnico | Impacto | Estrategia de Mitigación |
| :--- | :--- | :--- |
| **Deriva Flotante** | Desincronización de reparto | Preservar `float` para `p_SumaNivelesElevados` y realizar operaciones interactivas con la misma precisión que `Single` de VB6. |
| **Punteros / Dangling `UserIndex`** | Crash o lectura de memoria corrupta | Validar `UserIndex > 0` y verificar que el `PartyIndex` del jugador coincida con la instancia activa de la party. |
| **Desincronización en Disolución** | Inconsistencia de estado | Replicar estrictamente el orden de reseteo de `PartyIndex` y notificaciones de consola de `SaleMiembro`. |

---

## 6. Conclusión y Próximos Pasos

El plan de desglose del Módulo #31 garantiza una transliteración limpia y modular en C++20, dividida en 4 fases ordenadas e independientes. Una vez obtenida la aprobación de este documento, se procederá a implementar la Fase G1 sin alterar el estado del control de versiones.
