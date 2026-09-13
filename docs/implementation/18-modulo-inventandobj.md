---
area: logica-de-juego
status: completed
module: Modulo_InventANDobj
layer: 6
legacy_source: legacy/server/Codigo/Modulo_InventANDobj.bas
target_header: src/server/Modulo_InventANDobj.hpp
target_source: src/server/Modulo_InventANDobj.cpp
test_suite: tests/test_modulo_inventandobj.cpp
last_updated: 2026-09-13
---

# Módulo #18: Modulo_InventANDobj — Gestión de Inventarios NPC, Drops Probabilísticos y Despacho Espacial

## Resumen del Módulo

Este documento formaliza la arquitectura, decisiones de bajo nivel, replicación de bugs legacy y cobertura de pruebas del módulo de Capa 6 **`Modulo_InventANDobj`** (`src/server/Modulo_InventANDobj.hpp` y `src/server/Modulo_InventANDobj.cpp`), transliterado a partir del módulo original `legacy/server/Codigo/Modulo_InventANDobj.bas` (345 líneas en Visual Basic 6.0).

El módulo centraliza las operaciones del ciclo de vida de los objetos e inventarios de criaturas (NPCs):
1. **Gestión de Stock de NPCs**: Inicialización, consulta, consumo y reposición automática de objetos para criaturas comerciantes y regulares.
2. **Ciclo de Drops y Descarte**: Evaluación de la cascada probabilística de botín al morir una criatura y tratamiento especial de criaturas de facción pretoriana.
3. **Fragmentación Monetaria**: División de cantidades arbitrarias de oro en pilas de hasta `MAX_INVENTORY_OBJS` (10.000 unidades).
4. **Despacho Espacial al Suelo**: Ubicación de objetos en celdas libres transitables adyacentes (`Tilelibre`) y su materialización física en el mapa (`MakeObj`).

### Estado de Cierre Formal: `Completado (Aislado / Cableado Pendiente)`

Conforme a la taxonomía definida en `docs/CONVENTIONS.md` (Lista de Chequeo de Finalización de Módulos), este módulo se encuentra en estado **`Completado (Aislado / Cableado Pendiente)`**:
- **Código C++ Cerrado**: La transliteración de `src/server/Modulo_InventANDobj.hpp` y `src/server/Modulo_InventANDobj.cpp` está 100% finalizada y testeada con 5 casos de prueba principales y 192 aserciones doctest. No requiere ediciones internas futuras.
- **Cableado Pendiente en Capas Superiores**: Su integración productiva en el bucle del servidor depende de conectar sus hooks inyectables a los módulos definitivos:
  - `SetMakeObjHook`: Será provisto por el Módulo #19 (`InvUsuario.bas`, Capa 6).
  - `SetTilelibreHook`: Será provisto por el Módulo #34 (`Modulo_UsUaRiOs.bas`, Capa 9).
  - `SetNPCTemplateLookupHook`: Será alimentado por los datos en memoria cargados por `FileIO` / `Declaraciones.dat` (`NPCs.dat`).
  - Invocación de `NPC_TIRAR_ITEMS`: Será consumido por `MuereNpc` en el Módulo #28 (`MODULO_NPCs.bas`, Capa 8).
  - Invocación de `QuitarNpcInvItem`, `ResetNpcInv` y `CargarInvent`: Será consumido por el Módulo #21 (`Comercio.bas`, Capa 7).

---

## Decisiones Estratégicas de Arquitectura y Paridad en C++20

### 1. Mapeo Directo de Inventario y Preservación Estructural (Sin OOP Compartida)

- **Diagnóstico en VB6**: El código legacy manipula el inventario de la criatura accediendo directamente a `Npclist(NpcIndex).Invent`. La estructura `Inventario` contiene un arreglo estático `Object(1 To MAX_INVENTORY_SLOTS) As UserOBJ` y un contador `NroItems As Integer`.
- **Decisión de Porting**: Se prohibió unificar los inventarios de jugadores y NPCs bajo jerarquías abstractas o clases OOP compartidas. `Modulo_InventANDobj` opera directamente sobre la estructura `Inventario` definida en `src/server/Declares.hpp`:
  - Se respeta estrictamente la indexación 1-based (`1..MAX_INVENTORY_SLOTS = 30`), aprovechando el padding de dimensión `[MAX_INVENTORY_SLOTS + 1]` declarado en `Declares.hpp`.
  - Las funciones validan rangos de forma segura y manipulan los campos `ObjIndex` y `Amount` sin introducir asignaciones dinámicas ni sobrecarga polimórfica.

### 2. Replicación del Bug #27: Asimetría de `GiveGLD` en `NPC_TIRAR_ITEMS`

- **Diagnóstico en VB6**: En `Modulo_InventANDobj.bas:67-136`, el procedimiento `NPC_TIRAR_ITEMS` contiene dos ramas claramente diferenciadas según el flag `EsPretoriano`:
  - Si la criatura es pretoriana (`EsPretoriano = True`), se recorren sus 30 slots de inventario arrojándolos al piso y, al finalizar, **se evalúa explícitamente `If .GiveGLD > 0 Then Call TirarOroNpc(.GiveGLD, .Pos)`**.
  - Si la criatura es regular (`EsPretoriano = False`), el flujo entra a la cascada de la matriz `.Drop`, arrojando como máximo un solo drop de dicha matriz (que puede o no ser oro si `Drop[x].ObjIndex == iORO`). El campo `.GiveGLD` **NUNCA se consulta ni se arroja**, quedando desestimado en el motor legacy para criaturas no pretorianas.
- **Decisión de Porting**: Se preservó textualmente esta asimetría histórica. Para criaturas no pretorianas, `.GiveGLD` se ignora por completo. Ver entrada oficial en el Master Bug Ledger: `docs/implementation/KNOWN-LEGACY-BUGS.md` (Entrada #27).

### 3. Replicación del Bug #28: Destrucción Silenciosa de Drops ante Retorno Nulo de `Tilelibre`

- **Diagnóstico en VB6**: En `Modulo_InventANDobj.bas:43-63`:
  ```vb
  Public Function TirarItemAlPiso(Pos As WorldPos, Obj As Obj, Optional NotPirata As Boolean = True) As WorldPos
      Dim NuevaPos As WorldPos
      Call Tilelibre(Pos, NuevaPos, Obj, NotPirata, True)
      If NuevaPos.X <> 0 And NuevaPos.Y <> 0 Then
          Call MakeObj(Obj, Pos.Map, NuevaPos.X, NuevaPos.Y)
      End If
      TirarItemAlPiso = NuevaPos
  End Function
  ```
  Si el radio de búsqueda de `Tilelibre` se encuentra totalmente bloqueado o saturado de objetos, la rutina devuelve `NuevaPos` con `X = 0` e `Y = 0`.
  Al ocurrir esto:
  1. La condición `NuevaPos.X <> 0 And NuevaPos.Y <> 0` evalúa a `False`.
  2. `MakeObj` **no se convoca**.
  3. La función retorna `{Pos.Map, 0, 0}` sin registrar errores en el log, sin lanzar excepciones y sin notificar a los bucles llamadores (`NPC_TIRAR_ITEMS` o `TirarOroNpc`).
  4. El objeto se evapora silenciosa y definitivamente del mundo.
- **Decisión de Porting**: Se transliteró la misma lógica condicional en C++20. Ante `(0, 0)` de `s_tilelibre_hook`, se retorna `WorldPos{pos.Map, 0, 0}` sin convocar a `s_make_obj_hook`, garantizando total compatibilidad legacy. Ver entrada oficial en el Master Bug Ledger: `docs/implementation/KNOWN-LEGACY-BUGS.md` (Entrada #28).

### 4. Cascada Geométrica de Drops y Exactitud Matemática

- **Diagnóstico en VB6**: La cascada probabilística de `NPC_TIRAR_ITEMS` para NPCs regulares utiliza la siguiente distribución:
  - Tirada inicial `Roll = RandomNumber(1, 100)`.
  - Si `Roll > 90` (10% de las veces): la criatura no arroja ningún ítem (drop nulo).
  - Si `Roll <= 90` (90% de las veces):
    - Se preselecciona `NroDrop = 1`.
    - Si `Roll <= 10` (10% de probabilidad condicional sobre la tirada inicial): asciende a `NroDrop = 2`.
    - Para etapas sucesivas (etapa 3, 4 y 5), ejecuta hasta 3 tiradas independientes `RandomNumber(1, 100) <= 10`. Cada éxito incrementa `NroDrop` en 1; ante el primer fallo, el bucle concluye (`Exit For`).
- **Probabilidades Teóricas Resultantes**:
  - Sin drop: $10.0\%$
  - Drop 1: $80.0\%$ (Roll entre 11 y 90)
  - Drop 2: $9.0\%$ ($10\% \times 90\%$)
  - Drop 3: $0.9\%$ ($10\% \times 10\% \times 90\%$)
  - Drop 4: $0.09\%$ ($10\% \times 10\% \times 10\% \times 90\%$)
  - Drop 5 (Legendario): $0.01\%$ ($10\% \times 10\% \times 10\% \times 10\%$)
- **Decisión de Porting**: Se respetó idénticamente la secuencia de llamadas a números pseudoaleatorios. Se incluyó el hook `SetRandomGeneratorHook` para inyectar secuencias deterministas en los tests unitarios y validar la cascada en cada uno de sus peldaños.

### 5. Erradicación de I/O Síncrona a Disco (`NPCs.dat`) y Aislamiento por Hooks

- **Diagnóstico en VB6**: Los procedimientos `CargarInvent` y `EncontrarCant` realizaban lecturas síncronas bloqueantes a disco llamando a `GetVar(DatPath & "NPCs.dat", "NPC" & NpcNumero, "Obj" & i)` cada vez que moría o comerciaba un NPC.
- **Decisión de Porting**: Conforme a las directrices de `docs/CONVENTIONS.md`, se eliminó por completo el acceso a disco en tiempo de juego. Se implementó el hook inyectable `SetNPCTemplateLookupHook`, el cual recibe un puntero o función en memoria capaz de consultar la plantilla de inventario base cargada durante el arranque del servidor (`FileIO`).

---

## Interfaz Pública y Puntos de Extensión

Declarados en `src/server/Modulo_InventANDobj.hpp`:

```cpp
namespace Modulo_InventANDobj {

struct NPCInventorySlot {
    std::int16_t obj_index{0};
    std::int16_t amount{0};
};

// Hooks de desacoplamiento e inyección de dependencias
using NPCTemplateLookupHook = std::function<std::vector<NPCInventorySlot>(std::int16_t npc_numero)>;
void SetNPCTemplateLookupHook(NPCTemplateLookupHook hook) noexcept;

using RandomGeneratorHook = std::function<std::int32_t(std::int32_t min, std::int32_t max)>;
void SetRandomGeneratorHook(RandomGeneratorHook hook) noexcept;

using TirarItemAlPisoHook = std::function<WorldPos(const WorldPos& pos, const Obj& obj, bool not_pirata)>;
void SetTirarItemAlPisoHook(TirarItemAlPisoHook hook) noexcept;

using TilelibreHook = std::function<void(const WorldPos& pos, WorldPos& n_pos, const Obj& obj, bool agua, bool tierra)>;
void SetTilelibreHook(TilelibreHook hook) noexcept;

using MakeObjHook = std::function<void(const Obj& obj, std::int16_t map, std::int16_t x, std::int16_t y)>;
void SetMakeObjHook(MakeObjHook hook) noexcept;

// G1: Núcleo de Inventario NPC
void ResetNpcInv(std::int16_t npc_index) noexcept;
[[nodiscard]] bool QuedanItems(std::int16_t npc_index, std::int16_t obj_index) noexcept;
[[nodiscard]] std::int16_t EncontrarCant(std::int16_t npc_index, std::int16_t obj_index) noexcept;
void QuitarNpcInvItem(std::int16_t npc_index, std::uint8_t slot, std::int16_t cantidad);
void CargarInvent(std::int16_t npc_index);

// G2: Drops Probabilísticos y Oro
void TirarOroNpc(std::int32_t cantidad, const WorldPos& pos);
void NPC_TIRAR_ITEMS(npc& current_npc, bool is_pretoriano);

// G3: Despacho Espacial
WorldPos TirarItemAlPiso(const WorldPos& pos, const Obj& obj, bool not_pirata = true);

} // namespace Modulo_InventANDobj
```

---

## Cobertura de Pruebas Unitarias

La suite de pruebas en `tests/test_modulo_inventandobj.cpp` implementa 5 casos de prueba estructurados en 3 suites con **192 aserciones verificadas**:

1. **`Modulo_InventANDobj - G1` (Gestión de Inventario NPC y Reposición de Stock)**:
   - `ResetNpcInv`: Blanqueo exhaustivo de slots 1..30 y puesta a cero de `NroItems` e `InvReSpawn`.
   - `QuedanItems`: Búsqueda lineal exitosa y descarte ante inventario vacío o NPC inexistente.
   - `EncontrarCant`: Consulta de plantillas desacoplada de disco vía hook.
   - `QuitarNpcInvItem`: Descuento parcial de stock.
   - `QuitarNpcInvItem`: Consumo total de ítem no crucial (liberación del slot y decremento de `NroItems`).
   - `QuitarNpcInvItem`: Consumo total de ítem crucial (`Crucial = 1`), reponiendo automáticamente el stock configurado en la plantilla base.
   - `QuitarNpcInvItem`: Ítem crucial no se duplica si ya existían unidades en otro slot activo.
   - `CargarInvent`: Carga inicial de múltiples slots y reposición total automática del inventario al quedar en 0 ítems.
   - `InvReSpawn = 1`: Inhibición de la recarga automática de inventario cuando la criatura tiene la bandera activa.

2. **`Modulo_InventANDobj - G2` (Drops Probabilísticos y Oro)**:
   - `TirarOroNpc`: Montos menores a 10.000 (pila única).
   - `TirarOroNpc`: Monto exacto de 10.000 unidades (límite superior de slot).
   - `TirarOroNpc`: Montos mayores a 10.000 (fragmentación matemática de 25.000 monedas en 3 pilas: 10k, 10k, 5k).
   - `TirarOroNpc`: Manejo seguro de valores nulos o negativos (0 llamadas a despacho).
   - `NPC_TIRAR_ITEMS`: Descarte por tirada nula (roll > 90).
   - `NPC_TIRAR_ITEMS`: Cascada geométrica hacia Drop[1], Drop[2], Drop[3], Drop[4] y Drop[5] (Legendario).
   - `NPC_TIRAR_ITEMS`: Redirección automática de drops monetarios (`ObjIndex == iORO`) hacia `TirarOroNpc`.
   - `NPC_TIRAR_ITEMS`: Rama Pretoriana (arroja todo el inventario activo más `GiveGLD` fragmentado).
   - `NPC_TIRAR_ITEMS`: **Verificación de Bug #27** (criatura regular con `GiveGLD > 0` jamás arroja dicho oro).

3. **`Modulo_InventANDobj - G3` (Despacho Espacial y Puntos de Extensión)**:
   - `TirarItemAlPiso`: Despacho exitoso a celda libre encontrada por `TilelibreHook` y materialización con `MakeObjHook`.
   - `TirarItemAlPiso`: Propagación exacta de banderas booleanas (`not_pirata = true` propaga `agua = true, tierra = true`; `not_pirata = false` propaga `agua = false, tierra = true`).
   - `TirarItemAlPiso`: **Verificación de Bug #28** (si `Tilelibre` retorna coordenadas (0, 0), `MakeObjHook` no es convocado y se retorna `(0, 0)` silenciosamente sin excepciones).
   - `TirarItemAlPiso`: Saturación integrada con `TirarOroNpc` (25.000 de oro en mapa lleno ejecutan sus 3 ciclos sin loop infinito y con 0 llamadas a `MakeObj`).
   - `TirarItemAlPiso`: Bypass prioritario de alto nivel (`SetTirarItemAlPisoHook`).
