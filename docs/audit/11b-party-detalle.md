# Auditoría Técnica Detallada — Módulo #31: `mdParty.bas` y `clsParty.cls`

> **Estado**: Completado  
> **Área**: Capa 9 (Sistemas Sociales, Foros y Party)  
> **Subordinado a**: [`docs/audit/11-foros-y-comunicacion.md`](11-foros-y-comunicacion.md)  
> **Archivos Legacy**: `legacy/server/Codigo/mdParty.bas` (~502 líneas VB6) y `legacy/server/Codigo/clsParty.cls` (~491 líneas VB6)

---

## 1. Resumen Ejecutivo y Alcance

El sistema de Party de Argentum Online 0.13.0 permite a un grupo de hasta 5 jugadores (`PARTY_MAXMEMBERS = 5`) cooperar en la caza de criaturas y compartir los puntos de experiencia obtenidos. El subsistema está estructurado en dos componentes principales:
1. **`mdParty.bas`**: Módulo estático que actúa como administrador global (*PartyManager*) gestionando la creación de instancias, la validación de comandos del protocolo de red (`/PARTY`, `/ACEPTARPARTY`, `/EXPULSARPARTY`, `/SALIRPARTY`, `/PARTYONLINE`), la gestión del vector global de parties (`Parties(1 To 300)`) y la coordinación de flushes periódicos de experiencia.
2. **`clsParty.cls`**: Clase de objeto que encapsula el estado interno de cada grupo (integrantes, líder, experiencia acumulada total, suma ponderada de niveles) y la lógica matemática de ponderación y distribución de experiencia.

Este informe contiene los resultados de la auditoría técnica estricta del código fuente en Visual Basic 6.0, catalogando exhaustivamente sus rutinas, estructuras, fórmulas matemáticas, quirks de ciclo de vida y condiciones de desincronización para su futura portabilidad a C++20.

---

## 2. Catálogo Exhaustivo de Procedimientos y Estructuras

### 2.1. Constantes y Estructuras Globales (`mdParty.bas:32-80`)

- **Constantes de Configuración**:
  - `MAX_PARTIES` (`Integer = 300`, L. 32): Capacidad máxima simultánea de parties en el servidor.
  - `MINPARTYLEVEL` (`Byte = 15`, L. 36): *Código Muerto*. Nivel mínimo teórico para crear party (comentado en implementación).
  - `PARTY_MAXMEMBERS` (`Byte = 5`, L. 40): Cantidad máxima de miembros por party.
  - `PARTY_EXPERIENCIAPORGOLPE` (`Boolean = False`, L. 45): Determina si la experiencia se acredita en tiempo real por cada criatura abatida (`True`) o si se acumula en la party y se acredita al salir/disolver/flush (`False`).
  - `MAXPARTYDELTALEVEL` (`Byte = 7`, L. 49): *Código Muerto*. Máxima diferencia de nivel entre miembros (nunca evaluada).
  - `MAXDISTANCIAINGRESOPARTY` (`Byte = 2`, L. 53): Distancia máxima en tiles entre el aspirante y el líder al momento de la aceptación.
  - `PARTY_MAXDISTANCIA` (`Byte = 18`, L. 57): Distancia máxima en tiles desde la criatura abatida para que un miembro reciba experiencia.
  - `CASTIGOS` (`Boolean = False`, L. 61): Flag para penalización de experiencia negativa (sin uso práctico en v0.13.0).
  - `ExponenteNivelParty` (`Single`, L. 67): Variable global poblada durante el arranque desde `Dat/Balance.dat` (`FileIO::LoadBalance`). Exponente $E$ para la ponderación de niveles.

- **Estructuras de Datos**:
  - `tPartyMember` (`Type`, `mdParty.bas:75-78`):
    ```vb
    Public Type tPartyMember
        UserIndex As Integer
        Experiencia As Double
    End Type
    ```
  - Vector Global de Parties: `Parties(1 To MAX_PARTIES) As clsParty` (Declarado en `Declares.bas:381`).

---

### 2.2. Estado Interno de la Clase `clsParty.cls` (`clsParty.cls:35-50`)

| Miembro Privado | Tipo de Dato | Propósito y Descripción |
| :--- | :--- | :--- |
| `p_members(1 To 5)` | `tPartyMember` | Arreglo fijo de 5 slots. El slot `p_members(1)` corresponde obligatoriamente al Líder/Fundador. |
| `p_expTotal` | `Long` | Experiencia histórica total acumulada por la party desde su creación. |
| `p_Fundador` | `Integer` | `UserIndex` del líder del grupo (idéntico a `p_members(1).UserIndex`). |
| `p_CantMiembros` | `Integer` | Contador activo de integrantes pertenecientes al grupo ($1 \le \text{cant} \le 5$). |
| `p_SumaNivelesElevados` | `Single` | Suma ponderada de los niveles de todos los integrantes activos: $\sum (ELV_i)^E$. |

---

### 2.3. Catálogo de Rutinas de `mdParty.bas`

| Procedimiento | Visibilidad | Firma / Retorno | Ubicación | Propósito Funcional |
| :--- | :---: | :--- | :---: | :--- |
| `NextParty` | `Public` | `() As Integer` | L. 81-96 | Busca el primer índice vacante (`Nothing`) en `Parties(1..300)`. Retorna el slot o `-1` si está lleno. |
| `PuedeCrearParty` | `Public` | `(UserIndex As Integer) As Boolean` | L. 98-115 | Valida requerimientos para fundar: $Carisma \times Liderazgo \ge 100$ y `Muerto = 0`. |
| `CrearParty` | `Public` | `(UserIndex As Integer)` | L. 117-161 | Valida `Liderazgo >= 5`, instancia `New clsParty`, agrega al fundador como primer miembro y líder. |
| `SolicitarIngresoAParty` | `Public` | `(UserIndex As Integer)` | L. 163-201 | Registra la solicitud del usuario asignando `UserList(UserIndex).PartySolicitud = PartyIndexDelTarget`. |
| `SalirDeParty` | `Public` | `(UserIndex As Integer)` | L. 203-223 | Ejecuta la salida del integrante. Si era el líder, destruye la party (`Set Parties(PI) = Nothing`). |
| `ExpulsarDeParty` | `Public` | `(leader As Integer, OldMember As Integer)` | L. 225-247 | Valida pertenencia y remueve al integrante. Disuelve la party si el expulsado es el líder. |
| `UserPuedeEjecutarComandos` | `Public` | `(User As Integer) As Boolean` | L. 254-275 | Retorna `True` únicamente si el usuario es el líder activo de la party. |
| `AprobarIngresoAParty` | `Public` | `(leader As Integer, NewMember As Integer)` | L. 277-340 | Valida `PartySolicitud`, distancia ($\le 2$), vivo, cupo y compatibilidad faccionaria antes de agregar. |
| `IsPartyMember` | `Private` | `(UserIndex As Integer, PartyIndex As Integer)` | L. 342-348 | **Código Muerto / Incompleto**. Bucle `For` vacío sin valor de retorno ni invocaciones. |
| `BroadCastParty` | `Public` | `(UserIndex As Integer, texto As String)` | L. 350-365 | Transmite un mensaje por la consola del chat de grupo a todos los integrantes de la party. |
| `OnlineParty` | `Public` | `(UserIndex As Integer)` | L. 367-392 | Formatea e imprime en la consola del jugador la lista de miembros conectados y sus exps. |
| `TransformarEnLider` | `Public` | `(OldLeader As Integer, NewLeader As Integer)` | L. 395-422 | Transfiere el liderazgo al nuevo integrante si está vivo y pertenece a la misma party. |
| `ActualizaExperiencias` | `Public` | `() ` | L. 425-454 | Flushea la experiencia acumulada de todas las parties activas antes de un WorldSave/shutdown. |
| `ObtenerExito` | `Public` | `(UserIndex As Integer, Exp As Long, mapa, X, Y)` | L. 456-470 | Punto de entrada invocado al matar un NPC para acreditar la experiencia ganada a la party. |
| `CantMiembros` | `Public` | `(UserIndex As Integer) As Integer` | L. 472-484 | Retorna la cantidad de integrantes activos de la party a la que pertenece el usuario. |
| `ActualizarSumaNivelesElevados` | `Public` | `(UserIndex As Integer)` | L. 491-500 | Recalcula la suma ponderada de niveles cuando un usuario sube de nivel durante la party. |

---

### 2.4. Catálogo de Métodos de `clsParty.cls`

| Método | Visibilidad | Firma / Retorno | Ubicación | Propósito Funcional |
| :--- | :---: | :--- | :---: | :--- |
| `Class_Initialize` | `Public` | `()` | L. 53-64 | Constructor. Inicializa `p_expTotal = 0`, `p_CantMiembros = 0`, `p_SumaNivelesElevados = 0`. |
| `Class_Terminate` | `Public` | `()` | L. 67-68 | Destructor estándar de VB6. |
| `UpdateSumaNivelesElevados` | `Public` | `(Lvl As Integer)` | L. 75-82 | Actualiza `p_SumaNivelesElevados` al subir de nivel: $- (Lvl-1)^E + Lvl^E$. |
| `MiExperiencia` | `Public` | `(UserIndex As Integer) As Long` | L. 84-107 | Retorna la experiencia truncada (`Fix`) acumulada por un integrante específico en la party. |
| `ObtenerExito` | `Public` | `(ExpGanada As Long, mapa, X, Y)` | L. 109-151 | Calcula y distribuye la experiencia proporcional de un monstruo entre los miembros calificados. |
| `MandarMensajeAConsola` | `Public` | `(texto As String, Sender As String)` | L. 153-163 | Difunde un mensaje de consola (`FONTTYPE_PARTY`) a todos los integrantes con `UserIndex > 0`. |
| `EsPartyLeader` | `Public` | `(UserIndex As Integer) As Boolean` | L. 165-167 | Evalúa si el `UserIndex` coincide con el `p_Fundador`. |
| `NuevoMiembro` | `Public` | `(UserIndex As Integer) As Boolean` | L. 169-194 | Registra un nuevo integrante en el primer slot libre de `p_members` e incrementa `p_SumaNivelesElevados`. |
| `SaleMiembro` | `Public` | `(UserIndex As Integer) As Boolean` | L. 196-286 | Remueve a un usuario. Si es el líder (slot 1), disuelve la party; si no, transfiere su EXP y compacta. |
| `HacerLeader` | `Public` | `(UserIndex As Integer) As Boolean` | L. 288-335 | Intercambia las posiciones del slot 1 y el slot del nuevo líder en `p_members`, preservando exps. |
| `ObtenerMiembrosOnline` | `Public` | `(ByRef MemberList() As Integer)` | L. 338-357 | Popula un arreglo de enteros con los `UserIndex` activos del grupo. |
| `ObtenerExperienciaTotal` | `Public` | `() As Long` | L. 359-366 | Retorna `p_expTotal`. |
| `PuedeEntrar` | `Public` | `(UserIndex As Integer, ByRef razon) As Boolean` | L. 368-430 | Valida distancia al líder ($\le 2$), vacantes ($< 5$) y compatibilidad Armada/Caos/Criminal/Ciudadano. |
| `FlushExperiencia` | `Public` | `()` | L. 433-466 | Transfiere la experiencia acumulada en `p_members` a `UserList.Stats.Exp` y actualiza niveles. |
| `CompactMemberList` | `Private` | `()` | L. 468-487 | Reordena `p_members` desplazando los slots vacíos (`UserIndex = 0`) hacia el final del arreglo. |
| `CantMiembros` | `Public` | `() As Integer` | L. 489-491 | Getter de `p_CantMiembros`. |

---

## 3. Mecánica de Creación, Membresía y Disolución

### 3.1. Flujo de Creación de Party
1. **Comando `/CREARPARTY`**: El usuario ejecuta el comando (`Protocol.bas`).
2. **Validación de Atributos (`mdParty.bas:98-115`)**:
   - `UserList(UI).flags.Muerto == 0`.
   - `Carisma * Liderazgo >= 100` (`PuedeCrearParty`).
   - `UserList(UI).Stats.UserSkills(Liderazgo) >= 5` (`CrearParty:129`).
3. **Reserva de Slot y Creación (`mdParty.bas:130-148`)**:
   - `tInt = NextParty()` busca un slot `Nothing` en `Parties(1..300)`.
   - `Set Parties(tInt) = New clsParty`.
   - Invoca `NuevoMiembro(UserIndex)` -> Agrega en `p_members(1)`, `p_CantMiembros = 1`, `p_SumaNivelesElevados = (ELV)^E`.
   - Invoca `HacerLeader(UserIndex)` -> Setea `p_Fundador = UserIndex`.
   - Setea `UserList(UI).PartyIndex = tInt` y `UserList(UI).PartySolicitud = 0`.

### 3.2. Solicitud y Aceptación de Integrantes
1. **Solicitud (`/PARTY`) (`mdParty.bas:163-201`)**:
   - El aspirante hace click sobre el líder (`TargetUser`) y ejecuta `/PARTY`.
   - Setea `UserList(Aspirante).PartySolicitud = UserList(TargetUser).PartyIndex`.
2. **Aprobación (`/ACEPTARPARTY <Nombre>`) (`mdParty.bas:277-340`)**:
   - El líder selecciona o escribe el nombre del personaje.
   - **Validaciones Previsibles**:
     - `UserList(Aspirante).PartySolicitud == PartyIndexDelLider`.
     - `UserList(Aspirante).flags.Muerto == 0`.
     - `UserList(Aspirante).PartyIndex == 0`.
   - **Validaciones de Reglas del Juego (`clsParty.cls:368-430`, `PuedeEntrar`)**:
     - Distancia espacial: `Distancia(PosLider, PosAspirante) <= 2` tiles.
     - Cupo disponible: `p_members(5).UserIndex == 0` (capacidad máxima de 5).
     - **Matriz de Compatibilidad Faccionaria y Criminal**:
       - *Armada Real*: No puede ingresar si en la party hay algún personaje con estado Criminal (`criminal(UI) = True`).
       - *Legión Oscura*: No puede ingresar si en la party hay algún personaje con estado Ciudadano (`criminal(UI) = False`).
       - *Criminal*: No puede ingresar a una party donde haya al menos un miembro pertenenciente a la Armada Real.
       - *Ciudadano*: No puede ingresar a una party donde haya al menos un miembro perteneciente a la Legión Oscura.
   - **Ingreso**: Si `PuedeEntrar` es afirmativo, se invoca `NuevoMiembro(NewMember)`, actualizando `p_SumaNivelesElevados`, `PartyIndex` y reseteando `PartySolicitud = 0`.

### 3.3. Expulsión Voluntaria, Forzada y Disolución
- **Salida Voluntaria (`/SALIRPARTY`)** o **Expulsión (`/EXPULSARPARTY <Nombre>`)**:
  - Ambas invocan `Parties(PI).SaleMiembro(UserIndex)`.
- **Comportamiento Diferenciado de Disolución (`clsParty.cls:196-286`)**:
  - **Salida del Líder (`i == 1`)**:
    - `SaleMiembro` retorna `True`.
    - La party **se disuelve por completo**. Se itera en reversa sobre todos los integrantes (`j = 5 To 1 Step -1`), se les notifica el fin del grupo, se les acredita la experiencia acumulada `Fix(.Experiencia)` en `Stats.Exp`, y se resetea su `PartyIndex = 0`.
    - En `mdParty.bas` (L. 215 / L. 237), al recibir `True`, se ejecuta `Set Parties(PI) = Nothing`.
  - **Salida de un Miembro No Líder (`i > 1`)**:
    - `SaleMiembro` retorna `False`.
    - Se acredita la experiencia acumulada del integrante que abandona en su `Stats.Exp`.
    - Se decrementa `p_CantMiembros`, se resta de `p_SumaNivelesElevados` la potencia de su nivel: `p_SumaNivelesElevados = p_SumaNivelesElevados - (ELV)^E`.
    - Se invoca `CompactMemberList()` para desplazar los slots activos hacia la izquierda y no dejar huecos intermedios.

---

## 4. Cálculo Matemático y Distribución de Experiencia

### 4.1. Fórmula Exacta de Ponderación
Cuando una criatura es abatida por un integrante del grupo, se invoca `ObtenerExito(ExpGanada, mapa, X, Y)` (`clsParty.cls:109-151`).

1. **Incremento de Estadística Global**:
   $$p\_expTotal = p\_expTotal + ExpGanada$$

2. **Cálculo de Proporción Individual por Miembro**:
   Para cada slot $i \in [1, 5]$ con $UI = p\_members(i).UserIndex > 0$:
   $$\text{expThisUser} = \text{CDbl}\left( ExpGanada \times \frac{(UserList(UI).Stats.ELV)^{ExponenteNivelParty}}{p\_SumaNivelesElevados} \right)$$

3. **Promoción de Tipos y Truncamiento**:
   - `ExpGanada`: Entero largo signed de 32 bits (`Long`).
   - `ELV`: Byte de 8 bits (`Byte`).
   - `ExponenteNivelParty`: Cargado desde `Balance.dat` como flotante de precisión simple (`Single`).
   - `p_SumaNivelesElevados`: Flotante de precisión simple (`Single`).
   - `expThisUser`: Flotante de doble precisión (`Double`).
   - Acreditación final en `Stats.Exp`: Truncamiento entero estricto mediante la función `Fix(expThisUser)` (descarta la parte decimal).

4. **Modo de Entrega (`PARTY_EXPERIENCIAPORGOLPE`)**:
   - Si `PARTY_EXPERIENCIAPORGOLPE = True`: Se suma `Fix(expThisUser)` inmediatamente a `UserList(UI).Stats.Exp` en cada golpe/muerte y se sincroniza con el cliente (`WriteUpdateUserStats`).
   - Si `PARTY_EXPERIENCIAPORGOLPE = False` (*Configuración Predeterminada v0.13.0*): La experiencia se acumula en `p_members(i).Experiencia` (`Double`) y **solo se entrega al personaje cuando abandona la party, cuando la party se disuelve o durante un WorldSave (`FlushExperiencia`)**.

### 4.2. Condicionales Espaciales de Reparto
Para que un integrante perciba su porción de la experiencia calculada (`expThisUser`), debe cumplir **estrictamente las 3 condiciones espaciales**:
1. **Mismo Mapa**: `mapa == UserList(UI).Pos.map`.
2. **Estado Vivo**: `UserList(UI).flags.Muerto == 0`.
3. **Distancia en Tiles**: `Distance(UserList(UI).Pos.X, UserList(UI).Pos.Y, X, Y) <= PARTY_MAXDISTANCIA` (18 tiles en rango Manhattan/Euclídeo).

---

## 5. Tratamiento de Desconexión y Cambios de Mapa

### 5.1. Desconexión Voluntaria e Involuntaria (`TCP.bas:1805`)
- Al cerrarse el socket de un cliente en `TCP.bas::CloseSocket`:
  ```vb
  If UserList(UserIndex).PartyIndex > 0 Then Call mdParty.SalirDeParty(UserIndex)
  ```
- **Consecuencias**:
  - Si el usuario desconectado era el **Líder**, la party **se disuelve inmediatamente**, acreditando la experiencia a los miembros presentes que quedan conectados y destruyendo el objeto `Parties(PI)`.
  - Si el usuario era un **Miembro**, se procesa su abandono, acreditando su experiencia acumulada en su archivo de personaje antes del guardado final.
- **Sin Persistencia en Disco**: `PartyIndex` es una variable volátil de memoria. No se guarda en la ficha `.chr`. Al reconectar, `TCP.bas:1526` inicializa `.PartyIndex = 0`.

### 5.2. Cambios de Mapa y Teleportación (`WarpUserChar`)
- La membresía de la party se mantiene intacta tras un cambio de mapa o teleportación.
- **Efecto de Desincronización Espacial**:
  - Si un miembro está en otro mapa o a más de 18 tiles, no cumple la condición en `ObtenerExito`.
  - **Pérdida de Experiencia**: La fórmula **no redistribuye** la porción asignada a ese integrante entre los miembros presentes. La fracción `expThisUser` calculada para el ausente no se suma a su acumulador, **perdiéndose definitivamente para todo el grupo**.

---

## 6. Hallazgos Críticos, Quirks y Bugs Históricos

### 6.1. Bug #1: Inconsistencia de Parámetros en `SaleMiembro` durante Disolución
- **Ubicación**: `clsParty.cls:248`
- **Código VB6**:
  ```vb
  p_SumaNivelesElevados = p_SumaNivelesElevados - (UserList(UserIndex).Stats.ELV ^ ExponenteNivelParty)
  ```
- **Descripción**: Dentro del bucle `For j = PARTY_MAXMEMBERS To 1 Step -1` que disuelve la party por salida del líder, el código resta del acumulador el nivel de `UserIndex` (el líder que se va) en lugar de restar `p_members(j).UserIndex`. Aunque la instancia `clsParty` se destruye inmediatamente después (`Set Parties(PI) = Nothing`), esta imprecisión corrompe el valor de `p_SumaNivelesElevados` durante el bucle de notificaciones. Registrado en `KNOWN-LEGACY-BUGS.md` (Entrada #46).

### 6.2. Quirk #2: Deriva de Precisión en `Single` de `p_SumaNivelesElevados`
- **Ubicación**: `clsParty.cls:47`
- **Descripción**: `p_SumaNivelesElevados` está declarada como `Single` (32-bit floating point IEEE 754). Al realizar restas y sumas sucesivas de potencias flotantes (`Lvl ^ ExponenteNivelParty`) cuando entran y salen miembros o suben de nivel, la imprecisión acumulada de `Single` puede causar deriva numérica. Para mantener paridad comportamiento estricta byte a byte con VB6, se preserva obligatoriamente el tipo `float` en C++.

### 6.3. Bug #3: Incompleitud de la Rutina `IsPartyMember`
- **Ubicación**: `mdParty.bas:342-348`
- **Código VB6**:
  ```vb
  Private Function IsPartyMember(ByVal UserIndex As Integer, ByVal PartyIndex As Integer)
      Dim MemberIndex As Integer
      For MemberIndex = 1 To PARTY_MAXMEMBERS
      Next MemberIndex
  End Function
  ```
- **Descripción**: La función está declarada pero completamente vacía (bucle sin cuerpo ni valor de retorno). No es invocada desde ninguna otra parte del proyecto, constituyendo código muerto sintácticamente defectuoso.

### 6.4. Código Muerto: Constantes Inoperantes
- `MINPARTYLEVEL = 15` (`mdParty.bas:36`): Ignorada por completo; en `PuedeCrearParty` el chequeo de nivel fue comentado.
- `MAXPARTYDELTALEVEL = 7` (`mdParty.bas:49`): Ignorada por completo; en `PuedeEntrar` no se valida la brecha de niveles entre los integrantes.

---

## 7. Contratos Necesarios para la Migración C++20

Dado que `UserList` se encuentra disponible globalmente en la memoria del servidor (`Declares.hpp`), las propiedades del personaje (`.Stats.ELV`, `.Stats.UserSkills`, `.Stats.UserAtributos`, `.Pos`, `.flags.Muerto`, `.Faccion`) se leen y mutan de forma directa sobre la estructura central.

Los callbacks de abstracción se reservan **exclusivamente** para desacoplar el envío de tramas de red y notificaciones de interfaz:

```cpp
namespace ModParty {

struct PartyCallbacks {
    std::function<void(std::int16_t user_index, std::string_view msg, std::uint8_t font_type)> WriteConsoleMsg;
    std::function<void(std::int16_t user_index)> WriteUpdateUserStats;
    std::function<void(std::int16_t user_index)> CheckUserLevel;
};

void SetPartyCallbacks(const PartyCallbacks& callbacks);

void CrearParty(std::int16_t user_index);
void SolicitarIngresoAParty(std::int16_t user_index);
void AprobarIngresoAParty(std::int16_t leader, std::int16_t new_member);
void SalirDeParty(std::int16_t user_index);
void ExpulsarDeParty(std::int16_t leader, std::int16_t old_member);
void TransformarEnLider(std::int16_t old_leader, std::int16_t new_leader);
void ObtenerExito(std::int16_t user_index, std::int32_t exp_ganada, std::int16_t mapa, std::int16_t x, std::int16_t y);
void ActualizaExperiencias();

} // namespace ModParty
```

---

## 8. Conclusión del Análisis

El subsistema de Party está compuesto por ~993 líneas totales de VB6 divididas entre `mdParty.bas` y `clsParty.cls`. Presenta una lógica matemática bien delimitada para el reparto ponderado de experiencia basada en exponentes de nivel.

Para garantizar la **paridad matemática exacta byte a byte con VB6**, la variable `p_SumaNivelesElevados` **DEBE mantenerse obligatoriamente como `float`** (IEEE 754 de 32 bits), reproduciendo la misma precisión y redondeo flotante del tipo `Single` de Visual Basic 6.0. La gestión de instancias de `clsParty` utilizará almacenamiento estático o punteros administrados sin alterar la semántica de la grilla de memoria.
