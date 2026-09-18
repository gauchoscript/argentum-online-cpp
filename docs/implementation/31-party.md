# Módulo #31 — Sistema de Party (`mdParty.bas` y `clsParty.cls`)

## Estado del Módulo
`Completado (Aislado / Cableado Pendiente en Capa 9)`

---

## Resumen y Arquitectura

El Sistema de Party administra la agrupación de personajes para la experiencia compartida, comunicación privada y coordinación táctica. La portación a C++ preserva la separación estricta entre la entidad contenedora de estado (`clsParty`) y el módulo orquestador global (`mdParty`):

1. **Clase de Entidad `clsParty` (`src/server/clsParty.hpp` / `src/server/clsParty.cpp`)**:
   - Mantiene el estado interno de la party: el arreglo fijo de integrantes `p_members` (`tPartyMember[5]`), la experiencia acumulada total `p_expTotal`, el índice del fundador/líder `p_Fundador`, la cantidad actual de integrantes `p_CantMiembros` y la suma de los niveles elevados `p_SumaNivelesElevados`.
   - Modela los métodos de membresía (`NuevoMiembro`, `SaleMiembro`, `HacerLeader`), las validaciones de acceso (`PuedeEntrar`), el cálculo de experiencia compartida (`ObtenerExito`) y el vaciado transaccional (`FlushExperiencia`).

2. **Módulo Orquestador `mdParty` (`src/server/mdParty.hpp` / `src/server/mdParty.cpp`)**:
   - Gobierna la tabla global `Parties` (`std::array<std::shared_ptr<clsParty>, 301>`).
   - Implementa los puntos de entrada de comando y despacho: `CrearParty`, `SolicitarIngresoAParty`, `AprobarIngresoAParty`, `SalirDeParty`, `ExpulsarDeParty`, `TransformarEnLider`, `BroadCastParty`, `OnlineParty` y `ActualizaExperiencias`.
   - Utiliza la estructura `PartyCallbacks` para desacoplar las notificaciones de red y la actualización de cliente (`WriteConsoleMsg`, `WriteUpdateUserStats`, `CheckUserLevel`).

---

## Decisiones de Diseño y Paridad Legacy

1. **Tratamiento de Tipos y Precisión de Cálculo**:
   - `p_SumaNivelesElevados`: Se mantiene strictly como `float` (IEEE 754 de 32 bits) para garantizar la paridad exacta con el tipo `Single` de Visual Basic 6. La fórmula exponencial utiliza la variable de balance global `ExponenteNivelParty` (por defecto `1.4f`):
     $$\text{SumaNivelesElevados} = \sum (\text{ELV})^{1.4}$$
   - `Experiencia` en `tPartyMember`: Se mantiene como `double` (IEEE 754 de 64 bits) para evitar acumulación de errores de redondeo en cuotas fraccionarias de experiencia por golpe.
   - Truncamiento de Experiencia: El cálculo e incremento de experiencia usa `std::trunc` para convertir los valores flotantes a enteros de 32 bits (`std::int32_t`) al acreditar en `UserList[UI].Stats.Exp`.

2. **Filtros Espaciales y Pérdida de Experiencia en `ObtenerExito`**:
   - Al derrotar un NPC o ganar experiencia, se calcula la cuota proporcional de cada miembro:
     $$\text{expThisUser} = \text{expGanada} \times \frac{(\text{ELV})^{1.4}}{\text{SumaNivelesElevados}}$$
   - Para recibir su cuota, el integrante debe cumplir simultáneamente:
     1. Estar en el mismo mapa que el evento (`UserList[UI].Pos.Map == mapa`).
     2. Estar vivo (`UserList[UI].flags.Muerto == 0`).
     3. Estar a una distancia $\le 18$ baldosas (`PARTY_MAXDISTANCIA`) del origen.
   - Si un integrante no califica por alguno de estos filtros, su cuota se pierde **sin redistribución** entre el resto de los miembros activos.

3. **Preservación del Residuo Decimal en `FlushExperiencia`**:
   - Durante la acreditación periódica (`PARTY_EXPERIENCIAPORGOLPE == false`), `FlushExperiencia()` acredita la porción entera `std::trunc(p_members[i].Experiencia)` en `Stats.Exp` y substrae exactamente esa porción entera, conservando los decimales remanentes en `p_members[i].Experiencia` para el siguiente ciclo.

4. **Replicación Estricta del Bug Legacy #46**:
   - Al disolverse una party por salida del líder (`SaleMiembro` sobre slot 0), el bucle de limpieza substrae `p_SumaNivelesElevados` utilizando erróneamente el nivel del **líder saliente** (`user_index`) para cada integrante de la party en lugar del nivel individual de cada miembro:
     $$\text{p\_SumaNivelesElevados} \leftarrow \text{p\_SumaNivelesElevados} - (\text{ELV}_{\text{líder}})^{1.4}$$
   - Este comportamiento está documentado en `KNOWN-LEGACY-BUGS.md` (Entrada #46) y verificado con un caso de test doctest explícito.

---

## Suite de Pruebas Unitarias doctest

La suite de pruebas `[Party_G1]` a `[Party_G4]` en `tests/test_party.cpp` cubre 18 casos de prueba con 68 aserciones:

| Suite / Test Case | Propósito y Cobertura |
| :--- | :--- |
| `[Party_G1]` / `G1_NextParty_FindsFirstEmptySlot` | Búsqueda determinista del primer slot libre en `Parties` (1..300). |
| `[Party_G1]` / `G1_PuedeCrearParty_ValidatesAttributes` | Validación de requisito de Carisma $\times$ Liderazgo $\ge 100$ y estado vivo. |
| `[Party_G1]` / `G1_CrearParty_InstantiatesLeader` | Instanciación de party, asignación de líder y cálculo inicial de `SumaNivelesElevados()`. |
| `[Party_G2]` / `G2_SolicitarIngreso` | Asignación y verificación de `PartySolicitud`. |
| `[Party_G2]` / `G2_PuedeEntrar_Distance` | Rechazo de ingreso cuando la distancia excede los 2 tiles (`MAXDISTANCIAINGRESOPARTY`). |
| `[Party_G2]` / `G2_PuedeEntrar_Cupo` | Rechazo de ingreso con razón `"La party está llena."` al intentar superar 5 integrantes (`PARTY_MAXMEMBERS`). |
| `[Party_G2]` / `G2_PuedeEntrar_FactionMatrix` | Verificación de los 4 casos de la matriz faccionaria y criminal (Armada vs Criminal, Legión vs Ciudadano). |
| `[Party_G2]` / `G2_AprobarIngreso_Exito` | Aceptación de miembros, actualización de `PartyIndex` y recálculo acumulativo de `SumaNivelesElevados()`. |
| `[Party_G3]` / `G3_SaleMiembro_Comun` | Salida de integrante común, acreditación de exp, compactación de slots (`CompactMemberList()`) y recálculo. |
| `[Party_G3]` / `G3_SaleMiembro_Lider_Disolucion` | Disolución por salida del líder, acreditación masiva y verificación de la Entrada #46. |
| `[Party_G3]` / `G3_SalirDeParty_DestruccionInstancia` | Verificación de destrucción de instancia (`reset()`) tras la disolución. |
| `[Party_G3]` / `G3_ExpulsarDeParty` | Expulsión de integrante por el líder y rechazo ante intentos de no-líderes. |
| `[Party_G3]` / `G3_TransformarEnLider` | Transferencia de liderazgo (`HacerLeader`) y rechazo si el candidato está muerto o en otra party. |
| `[Party_G4]` / `G4_ObtenerExito_RepartoPonderado` | Cálculo de reparto ponderado de experiencia según nivel. |
| `[Party_G4]` / `G4_ObtenerExito_FiltrosEspaciales` | Pérdida de experiencia sin redistribución si un miembro está muerto, lejos ($> 18$ tiles) o en otro mapa. |
| `[Party_G4]` / `G4_FlushExperiencia_ResiduoDecimal` | Acreditación entera en `Stats.Exp` y preservación del remanente decimal. |
| `[Party_G4]` / `G4_UpdateSumaNivelesElevados` | Actualización precisa del acumulador flotante tras subida de nivel. |
| `[Party_G4]` / `G4_BroadCastParty_OnlineParty` | Formateo e impresión por consola de lista de miembros y mensajes de party. |

---

## Documentos Relacionados

- Auditoría Técnica: `docs/audit/11b-party-detalle.md` (subordinado a `docs/audit/11-foros-y-comunicacion.md`).
- Plan de Desglose Modular: `docs/implementation/31-party-breakdown.md`.
- Registro de Bugs Legacy: Entrada #46 en `docs/implementation/KNOWN-LEGACY-BUGS.md`.
