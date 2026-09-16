# Auditoría Técnica Detallada — Módulo #25: `ModFacciones.bas`

> **Estado**: Completado  
> **Área**: Capa 7 (Lógica de Combate, Magia, Facciones y Oficios)  
> **Subordinado a**: [`docs/audit/15-entidad-usuario-y-estado.md`](15-entidad-usuario-y-estado.md)  
> **Archivo Legacy**: `legacy/server/Codigo/ModFacciones.bas` (~809 líneas VB6)

---

## 1. Resumen Ejecutivo y Alcance

El módulo `ModFacciones.bas` administra el sistema de alineación faccionaria de Argentum Online 0.13.0, gobernando la membresía, rangos, recompensas y títulos de las dos facciones enfrentadas del juego:
1. **Armada Real** (Ejército Imperial del Rey, facción ciudadana/noble).
2. **Legión Oscura** (Fuerzas del Caos, facción criminal).

Este informe vuelca los hallazgos de la auditoría estricta del código fuente legacy en Visual Basic 6.0, identificando todos sus procedimientos, estructuras de datos, tablas de balance, puntos de contacto con `UserList` y subsistemas de red/inventario, así como las asimetrías y quirks históricos que deben preservarse o abstraerse durante la migración a C++.

---

## 2. Catálogo Completo de Procedimientos y Estructuras

### 2.1. Estructuras de Datos y Variables Globale (`ModFacciones.bas:26-81`)

- **Constantes y Enumeraciones**:
  - `NUM_RANGOS_FACCION` (`Integer = 15`, L. 67): Cantidad máxima de escalafones de rango en ambas facciones (1 a 15).
  - `NUM_DEF_FACCION_ARMOURS` (`Byte = 3`, L. 68): Tres categorías de armadura por clase/raza (`ieBaja = 0`, `ieMedia = 1`, `ieAlta = 2`).
  - `eTipoDefArmors` (Enum L. 70-74): Índices de defensa baja, media y alta.

- **Estructuras**:
  - `tFaccionArmaduras` (Type L. 76-79): Arreglos de `Armada(0..2)` y `Caos(0..2)` con los `ObjIndex` de armaduras faccionarias.

- **Variables de Memoria y Tablas de Carga (`Server.ini` y `Dat/ArmadurasFaccionarias.dat`)**:
  - `ArmadurasFaccion(1 To NUMCLASES, 1 To NUMRAZAS)` (`tFaccionArmaduras`, L. 78): Matriz poblada durante el arranque por `FileIO::LoadArmadurasFaccion` (`FileIO.bas:2389-2399`).
  - `RecompensaFacciones(0 To NUM_RANGOS_FACCION)` (`Long`, L. 81): Arreglo poblado desde `Dat/Balance.dat` (`FileIO::LoadBalance`) con los puntos de experiencia otorgados en cada ascenso.
  - **Variables de Vestimentas y Armaduras Faccionarias (`ModFacciones.bas:26-58`)**: `ArmaduraImperial1..3`, `TunicaMagoImperial`, `TunicaMagoImperialEnanos`, `ArmaduraCaos1..3`, `TunicaMagoCaos`, `TunicaMagoCaosEnanos`, `VestimentaImperialHumano`, `VestimentaImperialEnano`, `TunicaConspicuaHumano`, `TunicaConspicuaEnano`, `ArmaduraNobilisimaHumano`, `ArmaduraNobilisimaEnano`, `ArmaduraGranSacerdote`, `VestimentaLegionHumano`, `VestimentaLegionEnano`, `TunicaLobregaHumano`, `TunicaLobregaEnano`, `TunicaEgregiaHumano`, `TunicaEgregiaEnano`, `SacerdoteDemoniaco`. *(Pobladas globalmente en `Declares.hpp` desde `Server.ini` via `FileIO::LoadSini`)*.

---

### 2.2. Catálogo de Rutinas y Funciones

| Procedimiento | Visibilidad | Firma / Retorno | Ubicación | Propósito Funcional Exacto |
| :--- | :---: | :--- | :---: | :--- |
| `GetArmourAmount` | `Private` | `(Rango As Integer, TipoDef As eTipoDefArmors) As Integer` | L. 83-103 | Calcula la cantidad de unidades de armadura faccionaria a otorgar para un determinado rango ($1..15$) y categoría de defensa (`ieBaja`, `ieMedia`, `ieAlta`). |
| `GiveFactionArmours` | `Private` | `(UserIndex As Integer, IsCaos As Boolean)` | L. 105-163 | Entrega las 3 tandas de armaduras (baja, media, alta) consultando `ArmadurasFaccion(clase, raza)`. Intenta `MeterItemEnInventario`, si falla tira la armadura al suelo (`TirarItemAlPiso`). |
| `GiveExpReward` | `Public` | `(UserIndex As Integer, Rango As Long)` | L. 165-188 | Acredita la experiencia `RecompensaFacciones(Rango)` en `.Stats.Exp` clamping a `MAXEXP`, notifica por consola y evalúa subida de nivel (`CheckUserLevel`). |
| `EnlistarArmadaReal` | `Public` | `(UserIndex As Integer)` | L. 190-276 | Procesa el enrolamiento en la Armada Real previa verificación de 9 requisitos. Incrementa `.Faccion.Reenlistadas`, otorga kit inicial de vestimenta/exp si no los recibió antes, setea `NextRecompensa = 70`, actualiza barca y registra log. |
| `RecompensaArmadaReal` | `Public` | `(UserIndex As Integer)` | L. 278-414 | Evalúa la cantidad de criminales matados, nivel y nobleza frente a `NextRecompensa`. Avanza `RecompensasReal` (1 a 14), actualiza `NextRecompensa`, y otorga armaduras (`GiveFactionArmours`) + exp (`GiveExpReward`). |
| `ExpulsarFaccionReal` | `Public` | `(UserIndex As Integer, Optional Expulsado As Boolean = True)` | L. 416-445 | Remueve al usuario de la Armada Real (`ArmadaReal = 0`), desequipa armadura o escudo real si están equipados, actualiza apariencia/barca si navega y notifica por consola. |
| `ExpulsarFaccionCaos` | `Public` | `(UserIndex As Integer, Optional Expulsado As Boolean = True)` | L. 447-476 | Remueve al usuario de la Legión Oscura (`FuerzasCaos = 0`), desequipa armadura o escudo caos si están equipados, actualiza apariencia/barca si navega y notifica por consola. |
| `TituloReal` | `Public` | `(UserIndex As Integer) As String` | L. 478-536 | Retorna la cadena con el título nobiliario de la Armada Real según `.Faccion.RecompensasReal` (0: Aprendiz ... 14+: Campeón de la Luz). |
| `EnlistarCaos` | `Public` | `(UserIndex As Integer)` | L. 538-626 | Procesa el enrolamiento en la Legión Oscura previa verificación de 8 requisitos. Setea `FuerzasCaos = 1`, otorga kit inicial de vestimenta/exp si no los recibió antes, setea `NextRecompensa = 160`, actualiza barca y registra log `LogEjercitoCaos`. |
| `RecompensaCaos` | `Public` | `(UserIndex As Integer)` | L. 628-751 | Evalúa la cantidad de ciudadanos matados y nivel frente a `NextRecompensa`. Avanza `RecompensasCaos` (1 a 14), actualiza `NextRecompensa`, y otorga armaduras (`GiveFactionArmours`) + exp (`GiveExpReward`). |
| `TituloCaos` | `Public` | `(UserIndex As Integer) As String` | L. 753-809 | Retorna la cadena de título para la Legión Oscura según `.Faccion.RecompensasCaos` (0: Acólito ... 14+: Campeón de la Oscuridad). |

---

## 3. Mecánica de Jerarquías, Requisitos y Recompensas

### 3.1. Condiciones de Ingreso y Reingreso

#### Armada Real (`EnlistarArmadaReal`, L. 190-276)
Para ingresar a las tropas del Rey, el personaje debe cumplir **simultáneamente** 9 condiciones:
1. `ArmadaReal = 0`: No pertenecer previamente a la Armada.
2. `FuerzasCaos = 0`: No pertenecer a la Legión Oscura.
3. `criminal(UserIndex) = False`: Tener estado de ciudadano (karma/reputación no criminal).
4. `CriminalesMatados >= 30`: Haber derrotado al menos 30 criminales.
5. `Stats.ELV >= 25`: Nivel de personaje igual o mayor a 25.
6. `CiudadanosMatados = 0`: No haber matado nunca a ningún ciudadano inocente.
7. `Reenlistadas <= 4`: No haber sido expulsado o renuncio de la Armada más de 4 veces.
8. `Reputacion.NobleRep >= 1000000`: Poseer al menos 1.000.000 de puntos de nobleza.
9. `GuildAlignment != "Neutral"`: No ser miembro de un clan con alineamiento neutral.

#### Legión Oscura (`EnlistarCaos`, L. 538-626)
Para ingresar a las tropas de las Sombras, el personaje debe cumplir **simultáneamente** 8 condiciones:
1. `criminal(UserIndex) = True`: Tener estado criminal activo.
2. `FuerzasCaos = 0`: No pertenecer previamente al Caos.
3. `ArmadaReal = 0`: No pertenecer a la Armada Real.
4. **`RecibioExpInicialReal = 0`**: **¡Requisito Irreversible!** Si el personaje ingresó alguna vez a la Armada Real y recibió el kit inicial, la Legión Oscura rechaza su ingreso de forma permanente.
5. `CiudadanosMatados >= 70`: Haber asesinado al menos 70 ciudadanos inocentes.
6. `Stats.ELV >= 25`: Nivel de personaje igual o mayor a 25.
7. `GuildAlignment != "Neutral"`: No ser miembro de un clan con alineamiento neutral.
8. `Reenlistadas <= 4`: No haber sido expulsado más de 4 veces (salvo excepción `Reenlistadas = 200` que indica rebelión con ataque a tropas oscuras).

---

### 3.2. Tabla Comparativa de Rangos y Recompensas

| Rango | Armada Real (`RecompensasReal`) | Legión Oscura (`RecompensasCaos`) | Requisitos Armada | Requisitos Caos | Título Armada | Título Caos |
| :-: | :--- | :--- | :--- | :--- | :--- | :--- |
| **0** | Aprendiz | Acólito | Ingreso ($30$ Crimis) | Ingreso ($70$ Ciudas) | Aprendiz | Acólito |
| **1** | Escudero | Alma Corrupta | $70$ Crimis | $160$ Ciudas | Escudero | Alma Corrupta |
| **2** | Soldado | Paria | $130$ Crimis | $300$ Ciudas | Soldado | Paria |
| **3** | Sargento | Condenado | $210$ Crimis | $490$ Ciudas | Sargento | Condenado |
| **4** | Teniente | Esbirro | $320$ Crimis | $740$ Ciudas | Teniente | Esbirro |
| **5** | Comandante | Sanguinario | $460$ Crimis | $1100$ Ciudas | Comandante | Sanguinario |
| **6** | Capitán | Corruptor | $640$ Crimis + Nivel $\ge 27$ | $1500$ Ciudas + Nivel $\ge 27$ | Capitán | Corruptor |
| **7** | Senescal | Heraldo Impío | $870$ Crimis | $2010$ Ciudas | Senescal | Heraldo Impío |
| **8** | Mariscal | Caballero Oscuridad | $1160$ Crimis | $2700$ Ciudas | Mariscal | Caballero de la Oscuridad |
| **9** | Condestable | Señor del Miedo | $2000$ Crimis + Nivel $\ge 30$ | $4600$ Ciudas + Nivel $\ge 30$ | Condestable | Señor del Miedo |
| **10** | Ejecutor Imperial | Ejecutor Infernal | $2500$ Crimis + 2M Nobleza | $5800$ Ciudas + Nivel $\ge 31$ | Ejecutor Imperial | Ejecutor Infernal |
| **11** | Protector del Reino | Protector Averno | $3000$ Crimis + 3M Nobleza | $6990$ Ciudas + Nivel $\ge 33$ | Protector del Reino | Protector del Averno |
| **12** | Avatar de la Justicia | Avatar Destrucción | $3500$ Crimis + Nvl $\ge 35$ + 4M Nob | $8100$ Ciudas + Nivel $\ge 35$ | Avatar de la Justicia | Avatar de la Destrucción |
| **13** | Guardián del Bien | Guardián del Mal | $4000$ Crimis + Nvl $\ge 36$ + 5M Nob | $9300$ Ciudas + Nivel $\ge 36$ | Guardián del Bien | Guardián del Mal |
| **14** | Campeón de la Luz | Campeón Oscuridad | $5000$ Crimis + Nvl $\ge 37$ + 6M Nob | $11500$ Ciudas + Nivel $\ge 37$ | Campeón de la Luz | Campeón de la Oscuridad |
| **15** | *Tope Máximo* | *Tope Máximo* | $10000$ Crimis (Mensaje final) | $23000$ Ciudas (Mensaje final) | Campeón de la Luz | Campeón de la Oscuridad |

---

### 3.3. Criterios y Disparadores de Expulsión

La expulsión o renuncia de la facción se ejecuta mediante `ExpulsarFaccionReal` y `ExpulsarFaccionCaos`:

1. **Expulsión de la Armada Real (`ExpulsarFaccionReal`)**:
   - **Cometer Crímenes**: Atacar o matar a un ciudadano, o atacar a otro personaje Armada Real (`MODULO_NPCs.bas:202`, `SistemaCombate.bas:1243`, `modHechizos.bas:2080`).
   - **Ingreso/Creación de Clan Incompatible**: Si el personaje ingresa a un clan Neutral o Caos (`modGuilds.bas:226`).
   - **Comando de Renuncia o GM**: Invocación explícita por el usuario (`Protocol.bas:7117`) o expulsión administrativa (`Protocol.bas:12004`).
   - **Efectos**: Resetea `ArmadaReal = 0`, desequipa armadura y escudo real si los tiene vestidos (`Desequipar`), actualiza la barca faccionaria si navega (`RefreshCharStatus`) y notifica por consola.

2. **Expulsión de la Legión Oscura (`ExpulsarFaccionCaos`)**:
   - **Perder Estado Criminal**: Volverse ciudadano/noble tras ganar suficiente reputación (`MODULO_NPCs.bas:203`, `SistemaCombate.bas:501`).
   - **Ingreso/Creación de Clan Incompatible**: Si el personaje ingresa a un clan Neutral o Armada (`modGuilds.bas:230`).
   - **Comando de Renuncia o GM**: Invocación explícita por el usuario (`Protocol.bas:7134`) o expulsión administrativa (`Protocol.bas:11929`).
   - **Efectos**: Resetea `FuerzasCaos = 0`, desequipa armadura y escudo caos si los tiene vestidos (`Desequipar`), actualiza la barca faccionaria si navega y notifica por consola.

---

## 4. Puntos de Contacto e Interfaces con `UserList`

### 4.1. Mutación de Estado en `UserList(UserIndex)`

- **Estructura `.Faccion`**:
  - `ArmadaReal` (Byte/Integer flag 0 o 1)
  - `FuerzasCaos` (Byte/Integer flag 0 o 1)
  - `Reenlistadas` (Integer contador de ingresos)
  - `CriminalesMatados` (Long)
  - `CiudadanosMatados` (Long)
  - `RecompensasReal` / `RecompensasCaos` (Integer / Byte rango alcanzado 0..15)
  - `NextRecompensa` (Long meta de muertes para el próximo rango)
  - `RecibioArmaduraReal` / `RecibioArmaduraCaos` (Byte flag)
  - `RecibioExpInicialReal` / `RecibioExpInicialCaos` (Byte flag)
  - `NivelIngreso` (Byte)
  - `FechaIngreso` (Date / String)
  - `MatadosIngreso` (Long)

- **Otros Campos Consultados/Mutados**:
  - `.Stats.ELV` (Byte nivel de usuario)
  - `.Stats.Exp` (Long experiencia acumulada)
  - `.Reputacion.NobleRep` (Long puntos de nobleza)
  - `.flags.TargetNPC` (Integer índice del NPC con quien interactúa)
  - `.flags.Navegando` (Boolean estado de navegación en barco)
  - `.clase` y `.raza` (Bytes para indexar `ArmadurasFaccion`)

### 4.2. Invocación de Funciones de Otros Módulos
- **Red y Diálogos**:
  - `Protocol::WriteChatOverHead(UserIndex, msg, target_npc_char_index, color)`: Diálogos overhead sobre el NPC enrolador.
  - `Protocol::WriteConsoleMsg(UserIndex, msg, font_type)`: Mensajes en la consola de combate/sistema.
  - `Modulo_UsUaRiOs::RefreshCharStatus(UserIndex)`: Actualización del gráfico del barco/barca si navega.
  - `Modulo_UsUaRiOs::CheckUserLevel(UserIndex)`: Comprobación de subida de nivel al acreditar exp.
- **Inventario**:
  - `InvUsuario::MeterItemEnInventario(UserIndex, Obj)`
  - `InvUsuario::TirarItemAlPiso(Pos, Obj)`
  - `InvUsuario::Desequipar(UserIndex, Slot)`
- **Clanes**:
  - `modGuilds::GuildAlignment(GuildIndex)`: Consulta de alineamiento del clan.
- **Logging**:
  - `FileIO::LogEjercitoReal(text)` y `FileIO::LogEjercitoCaos(text)`.

---

## 5. Hallazgos Críticos, Quirks y Posibles Exploits Legacy

### 5.1. Quirk #1: Asimetría Irreversible de Reingreso
- **Descripción**: `EnlistarCaos` (L. 564) verifica estrictamente `If .Faccion.RecibioExpInicialReal = 1 Then Exit Sub`. Si un personaje ingresó una sola vez a la Armada Real y recibió el kit inicial, **nunca podrá ingresar a la Legión Oscura en toda su vida**, aunque sea expulsado, se vuelva criminal extremo y mate miles de ciudadanos.
- **Asimetría**: En la dirección opuesta, `EnlistarArmadaReal` **no chequea** `RecibioExpInicialCaos`, pero exige `CiudadanosMatados = 0`. Por lo tanto, un ex-Caos que nunca haya matado a ningún ciudadano inocente (si ingresó por algún exploit o bug) sí podría ingresar a la Armada Real.

### 5.2. Exploit #2: Retención de Ítems Faccionarios por Llamada Comentada
- **Descripción**: En `ExpulsarFaccionReal` y `ExpulsarFaccionCaos` (L. 425 y 456), la llamada al procedimiento de despojo de ítems está **comentada con apóstrofe**:
  ```vb
  'Call PerderItemsFaccionarios(UserIndex)
  ```
- **Impacto**: Al ser expulsado o al renunciar voluntariamente a la facción, el servidor solo desequipa la armadura o escudo si el usuario los tiene **puestos en ese instante** (`Desequipar`). Si el jugador guarda las armaduras faccionarias en la mochila o en la bóveda bancaria antes de cometer un crimen o renunciar, **conserva las armaduras faccionarias en su inventario**, pudiendo acumularlas o venderlas a comerciantes.

### 5.3. Quirk #3: Ausencia de Exigencias de Reputación en el Caos
- **Descripción**: Mientras que la Armada Real exige puntos de nobleza crecientes (desde 1.000.000 para entrar hasta 6.000.000 para Rango 14), la Legión Oscura **no exige ningún valor de Karma o BandidoRep** para ascender en sus 14 rangos; únicamente evalúa `CiudadanosMatados` y Nivel (`Stats.ELV`).

### 5.4. Quirk #4: Fórmulas Aritméticas de Armaduras en `GetArmourAmount`
- **Descripción**: `GetArmourAmount` (L. 83-103) utiliza expresiones aritméticas con redondeo/división entera de VB6:
  - `ieBaja`: `20 / (Rango + 1)`. Para Rango 0 da 20 armaduras; Rango 1 da 10; Rango 14 da `20 / 15 = 1` armadura.
  - `ieMedia`: `Rango * 2 / MaximoInt((Rango - 4), 1)`. Para Rango 0..4 da `Rango * 2`. Para Rango 14 da `14 * 2 / 10 = 2.8` (redondeado a 3 en VB6).
  - `ieAlta`: `Rango * 1.35`. Para Rango 1 da 1 armadura; para Rango 15 da `15 * 1.35 = 20.25` (20 armaduras).

### 5.5. Quirk #5: Marca Mágica `Reenlistadas = 200`
- **Descripción**: En `EnlistarCaos` (L. 594), se incluye el chequeo `If .Faccion.Reenlistadas = 200 Then`. Corresponde a una constante mágica histórica para identificar a ex-miembros de la Legión Oscura declarados en "rebelión" por haber atacado a sus propios compañeros de facción.

---

## 6. Contratos Necesarios para la Migración C++

Para preservar la independencia de la arquitectura y la compatibilidad 1:1 con el cliente VB6 real sin introducir dependencias circulares con capas superiores, se define la siguiente interfaz de callbacks en C++:

```cpp
namespace ModFacciones {

struct FactionCallbacks {
    std::function<bool(std::int16_t user_index)> IsCriminal;
    std::function<void(std::int16_t user_index, std::string_view msg, std::int16_t target_npc_char_index, std::uint32_t color)> WriteChatOverHead;
    std::function<void(std::int16_t user_index, std::string_view msg, std::uint8_t font_type)> WriteConsoleMsg;
    std::function<void(std::int16_t user_index)> RefreshCharStatus;
    std::function<void(std::int16_t user_index)> CheckUserLevel;
    std::function<bool(std::int16_t user_index, const Obj& item)> MeterItemEnInventario;
    std::function<void(const WorldPos& pos, const Obj& item)> TirarItemAlPiso;
    std::function<void(std::int16_t user_index, std::uint8_t slot)> Desequipar;
    std::function<std::string(std::int16_t guild_index)> GetGuildAlignment;
    std::function<void(std::string_view text)> LogEjercitoReal;
    std::function<void(std::string_view text)> LogEjercitoCaos;
};

void SetFactionCallbacks(const FactionCallbacks& callbacks);

void EnlistarArmadaReal(std::int16_t user_index);
void RecompensaArmadaReal(std::int16_t user_index);
void ExpulsarFaccionReal(std::int16_t user_index, bool expulsado = true);
void ExpulsarFaccionCaos(std::int16_t user_index, bool expulsado = true);
void EnlistarCaos(std::int16_t user_index);
void RecompensaCaos(std::int16_t user_index);
std::string TituloReal(std::int16_t user_index);
std::string TituloCaos(std::int16_t user_index);
void GiveExpReward(std::int16_t user_index, std::int32_t rango);

} // namespace ModFacciones
```

---

## 7. Conclusión del Análisis

El módulo `ModFacciones.bas` presenta una estructura bien delimitada y autocontenida de ~809 líneas VB6. Sus rutinas principales se comunican con `Protocol.bas` (opcodes de enrolamiento `/ENLISTAR` y recompensa `/RECOMPENSA`), `MODULO_NPCs.bas` y `SistemaCombate.bas` (expulsión automática por crímenes o alineamiento).

La replicación en C++ debe implementar estrictamente el clamping de rangos 0..15, la reproducción exacta de las 5 fórmulas y quirks identificados, y la inyección de callbacks para su desacoplamiento.
