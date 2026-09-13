---
area: logica-de-juego
status: completed
module: InvUsuario
layer: 6
legacy_source: legacy/server/Codigo/InvUsuario.bas
target_header: src/server/InvUsuario.hpp
target_source: src/server/InvUsuario.cpp
test_suite: tests/test_invusuario.cpp
last_updated: 2026-09-13
---

# Módulo #19: InvUsuario — Gestión del Inventario de Usuarios, Restricciones y Uso de Objetos

## Resumen del Módulo

Este documento formaliza la arquitectura, decisiones de bajo nivel, replicación de exploits históricos y cobertura de pruebas del módulo de Capa 6 **InvUsuario** (src/server/InvUsuario.hpp y src/server/InvUsuario.cpp), transliterado a partir del módulo original legacy/server/Codigo/InvUsuario.bas (1.685 líneas en Visual Basic 6.0).

El módulo centraliza las operaciones del ciclo de vida de los objetos del usuario y su interacción con el mapa:
1. **Mutaciones en el Mundo y Suelo (G1)**: Creación (MakeObj), reducción (EraseObj), descarte (DropObj) y recolección (GetObj) física de objetos sobre la grilla del mapa (MapData).
2. **Gestión Base de Inventario y Descarte (G2)**: Adición apilable (MeterItemEnInventario), consumo de cantidades en grilla fija y dispersa (QuitarUserInvItem), sincronización de paquetes de inventario (UpdateUserInv), limpieza total (LimpiarInventario), evaluación de susceptibilidad de robo (TieneObjetosRobables), purga y arrojamiento por muerte (TirarTodo, TirarTodosLosItems, TirarTodosLosItemsNoNewbies, TirarTodosLosItemsEnMochila, QuitarNewbieObj) y dispersión monetaria en lotes (TirarOro).
3. **Restricciones y Sistema de Equipamiento (G3)**: Validaciones de clase (ClasePuedeUsarItem), sexo (SexoPuedeUsarItem), facción (FaccionPuedeUsarItem) y raza (CheckRazaUsaRopa), junto con las transiciones de estado para vestir y desvestir prendas, armas, escudos, cascos, municiones y mochilas (EquiparInvItem, Desequipar).
4. **Uso de Ítems e Interacción con Oficios (G4)**: Despacho de consumibles según tipo de ítem (UseInvItem para alimentos, bebidas, pociones de vida/maná, oro directo, lectura de pergaminos arcanos, embarcaciones y minerales en fraguas), y emisión de catálogos construibles (EnivarArmasConstruibles, EnivarObjConstruibles, EnivarArmadurasConstruibles).

### Estado de Cierre Formal: Completado (Aislado / Cableado Pendiente en Capas 7 y 9)

Conforme a la taxonomía definida en [docs/CONVENTIONS.md](../CONVENTIONS.md) (Lista de Chequeo de Finalización de Módulos), este módulo se encuentra en estado **Completado (Aislado / Cableado Pendiente en Capas 7 y 9)**:
- **Código C++ Cerrado**: La implementación de los 28 procedimientos en src/server/InvUsuario.hpp y src/server/InvUsuario.cpp está 100% finalizada y verificada con 187 subcasos de prueba y más de 4.100 aserciones doctest. No requiere modificaciones internas futuras.
- **Cableado con Módulo #18 (Modulo_InventANDobj)**: Completamente compatible con la signatura esperada por Modulo_InventANDobj::SetMakeObjHook(InvUsuario::MakeObj).
- **Cableado Pendiente en Capas Superiores**: Su integración final en el servidor productivo depende de conectar sus hooks inyectables a los módulos definitivos:
  - SetQuitarUserInvItemHook / SetUpdateUserInvHook / SetMeterItemEnInventarioHook: Provistos localmente de forma predeterminada, desacoplables para mocking.
  - SetChangeUserInvHook: Invocará a modSendData::SendToUser en Capa 4 / Capa 9 para emitir el paquete de actualización de slot.
  - SetTilelibreHook: Será conectado a la implementación definitiva de dispersión espacial en el Módulo #34 (Modulo_UsUaRiOs.bas, Capa 9).
  - SetChangeUserCharHook y SetDarCuerpoDesnudoHook: Serán provistos por el Módulo #34 (Modulo_UsUaRiOs.bas, Capa 9) para reflejar cambios corporales y equipamiento visual ante otros jugadores del área.
  - SetLearnSpellHook: Será provisto por el Módulo #23 (modHechizos.bas, Capa 7) para registrar hechizos en la memoria del personaje.
  - SetNavegaHook: Será provisto por el Módulo #34 (Modulo_UsUaRiOs.bas, Capa 9) para iniciar la navegación sobre agua.
  - SetWorkRequestTargetHook: Será provisto por el Módulo #26 (Trabajo.bas, Capa 7) para abrir la mira de trabajo de herrería / fundición.
  - Invocación desde Capa 6: Servirá como base para el Módulo #20 (modBanco.bas) y Módulo #21 (Comercio.bas).

---

## Decisiones Estratégicas de Arquitectura y Paridad en C++20

### 1. Modelo de Inventario como Grilla Fija y Dispersa (Sparse Grid)

- **Diagnóstico en VB6**: En User.Invent, el arreglo Object(1 To MAX_INVENTORY_SLOTS) modela 30 celdas independientes. Las ranuras no se compactan ni se desplazan cuando un ítem se agota o se destruye.
- **Decisión de Porting**: En QuitarUserInvItem, cuando item.Amount <= 0, la ranura correspondiente se resetea (ObjIndex = 0, Amount = 0, Equipped = 0) en su posición exacta y se decrementa user.Invent.NroItems--. Está **estrictamente prohibido** utilizar corrimientos a la izquierda (std::vector::erase o compactaciones) para no desfasar los accesos directos por slot del cliente VB6 ni invalidar los punteros de equipamiento rápido (ArmourEqpSlot, WeaponEqpSlot, etc.).

### 2. Saneamiento de Desbordamientos Aritméticos en MakeObj

- **Diagnóstico en VB6**: En InvUsuario.bas:300, la acumulación de cantidades en el suelo se realiza con enteros de 16 bits: MapData(Map, X, Y).ObjInfo.Amount = MapData(Map, X, Y).ObjInfo.Amount + Obj.Amount. Si la suma supera 32.767, en VB6 se produce un desbordamiento que en C++ causaría *Undefined Behavior* por overflow con signo.
- **Decisión de Porting**: La suma se promueve explícitamente a std::int32_t antes de la asignación truncada a std::int16_t, preservando la semántica legacy sin incurrir en comportamiento indefinido de compilador.

### 3. Incoherencia de Parámetro de Mapa en DropObj

- **Diagnóstico en VB6**: En InvUsuario.bas:368-375, la subrutina DropObj recibe los parámetros Map, X, Y. Sin embargo, la verificación de si la celda está libre u ocupada consulta erróneamente el mapa del usuario: MapData(.Pos.Map, X, Y).ObjInfo.ObjIndex, ignorando el parámetro Map entrante. Luego, al llamar a MakeObj(Obj, Map, X, Y), sí utiliza el parámetro Map.
- **Decisión de Porting**: Se preservó textualmente esta asimetría histórica. La lectura de ocupación y cálculo de espacio remanente se realiza sobre MapData[get_map_block_index(user.Pos.Map, x, y)], mientras que MakeObj se despacha hacia el map pasado por argumento.

### 4. Replicación del Bug #29: Exploit de Duplicación en DropObj

- **Diagnóstico en VB6**: En InvUsuario.bas:368-375, si el total acumulado en el suelo supera MAX_INVENTORY_OBJS (10.000), el código recorta la variable local 
um = MAX_INVENTORY_OBJS - MapData(.Pos.Map, X, Y).ObjInfo.Amount. No obstante, la estructura Obj (que retiene el valor original completo Obj.Amount) se pasa intacta a MakeObj(Obj, Map, X, Y). A continuación, QuitarUserInvItem descuenta únicamente la variable 
um recortada. La diferencia entre el monto original y 
um se materializa en el suelo sin ser retirada del inventario del jugador.
- **Decisión de Porting**: Se prohibió mitigar este comportamiento. Conforme a la Regla #4 de [docs/CONVENTIONS.md](../CONVENTIONS.md), el exploit se replica 1:1, documentado oficialmente en el Master Bug Ledger ([docs/implementation/KNOWN-LEGACY-BUGS.md](KNOWN-LEGACY-BUGS.md#entrada-29--invusuario-exploit-histórico-de-duplicación-en-dropobj-por-desfase-de-cantidades), Entrada #29).

### 5. Replicación del Bug #30: Evaporación de Saldo en TirarOro (> 500k)

- **Diagnóstico en VB6**: En InvUsuario.bas:228-268, cuando un usuario arroja más de 500.000 monedas de oro, la subrutina calcula Extra = Cantidad - 500000 y recorta Cantidad = 500000. Luego intenta arrojar hasta 50 pilas de 10.000 monedas mediante TirarItemAlPiso. Al finalizar el bucle, si se arrojó al menos una pila, el saldo del jugador cambió (TeniaOro <> user.Stats.GLD), por lo que la salvaguarda de anulación If TeniaOro = .Stats.GLD Then Extra = 0 no se ejecuta. Inmediatamente después, user.Stats.GLD -= Extra descuenta el saldo excedente de la billetera sin crearlo en el suelo.
- **Decisión de Porting**: Se preservó la deducción incondicional de Extra en la billetera del jugador, documentada oficialmente en el Master Bug Ledger ([docs/implementation/KNOWN-LEGACY-BUGS.md](KNOWN-LEGACY-BUGS.md#entrada-30--invusuario-pérdida-silenciosa-de-saldo-excedente-en-tiraroro--500k), Entrada #30).

### 6. Preservación Literal de Nombres y Typos Legacy

- **Decisión de Porting**: Se mantuvieron estrictamente los nombres en PascalCase y las faltas de ortografía históricas de las rutinas de crafting: EnivarArmasConstruibles, EnivarObjConstruibles y EnivarArmadurasConstruibles, garantizando trazabilidad total con el código legacy y el protocolo.

---

## Interfaz Pública y Catálogo de Procedimientos

Declarados en src/server/InvUsuario.hpp:

`cpp
namespace InvUsuario {

// Hooks de desacoplamiento e inyección de dependencias
void SetQuitarUserInvItemHook(QuitarUserInvItemHook hook) noexcept;
void SetUpdateUserInvHook(UpdateUserInvHook hook) noexcept;
void SetMeterItemEnInventarioHook(MeterItemEnInventarioHook hook) noexcept;
void SetChangeUserInvHook(ChangeUserInvHook hook) noexcept;
void SetDesequiparHook(DesequiparHook hook) noexcept;
void SetTilelibreHook(TilelibreHook hook) noexcept;
void SetChangeUserCharHook(ChangeUserCharHook hook) noexcept;
void SetDarCuerpoDesnudoHook(DarCuerpoDesnudoHook hook) noexcept;
void SetLearnSpellHook(LearnSpellHook hook) noexcept;
void SetNavegaHook(NavegaHook hook) noexcept;
void SetWorkRequestTargetHook(WorkRequestTargetHook hook) noexcept;

// G1 — Mutaciones en el Mundo y Suelo
void MakeObj(const Obj& obj, std::int16_t map, std::int16_t x, std::int16_t y);
void EraseObj(std::int16_t num, std::int16_t map, std::int16_t x, std::int16_t y);
void DropObj(std::int16_t user_index, std::uint8_t slot, std::int16_t num, std::int16_t map, std::int16_t x, std::int16_t y);
void GetObj(std::int16_t user_index);

// G2 — Gestión Base de Inventario y Descarte
bool MeterItemEnInventario(std::int16_t user_index, Obj& mi_obj);
void QuitarUserInvItem(std::int16_t user_index, std::uint8_t slot, std::int16_t cantidad);
void UpdateUserInv(bool update_all, std::int16_t user_index, std::uint8_t slot);
void LimpiarInventario(std::int16_t user_index);
bool TieneObjetosRobables(std::int16_t user_index);
void QuitarNewbieObj(std::int16_t user_index);
bool ItemSeCae(std::int16_t index);
bool ItemNewbie(std::int16_t item_index);
eOBJType getObjType(std::int16_t obj_index);
void TirarTodo(std::int16_t user_index);
void TirarTodosLosItems(std::int16_t user_index);
void TirarTodosLosItemsNoNewbies(std::int16_t user_index);
void TirarTodosLosItemsEnMochila(std::int16_t user_index);
void TirarOro(std::int32_t cantidad, std::int16_t user_index);

// G3 — Restricciones y Sistema de Equipamiento
bool ClasePuedeUsarItem(std::int16_t user_index, std::int16_t obj_index, std::string* motivo = nullptr);
bool SexoPuedeUsarItem(std::int16_t user_index, std::int16_t obj_index, std::string* motivo = nullptr);
bool FaccionPuedeUsarItem(std::int16_t user_index, std::int16_t obj_index, std::string* motivo = nullptr);
bool CheckRazaUsaRopa(std::int16_t user_index, std::int16_t item_index, std::string* motivo = nullptr);
void EquiparInvItem(std::int16_t user_index, std::uint8_t slot);
void Desequipar(std::int16_t user_index, std::uint8_t slot);

// G4 — Uso de Ítems, Interacción y Oficios
void UseInvItem(std::int16_t user_index, std::uint8_t slot);
void EnivarArmasConstruibles(std::int16_t user_index);
void EnivarObjConstruibles(std::int16_t user_index);
void EnivarArmadurasConstruibles(std::int16_t user_index);

} // namespace InvUsuario
`

---

## Cobertura de Pruebas Unitarias

La suite de pruebas en 	ests/test_invusuario.cpp cuenta con **187 casos de prueba** y más de **4.100 aserciones verificadas** estructuradas en 4 suites correspondientes a cada fase:

1. **G1: Mutaciones en el Mundo y Suelo (40 tests)**:
   - MakeObj: Creación inicial, emisión de paquete ObjectCreate al área, acumulación en celda con promoción a 32 bits, validación de coordenadas y límites de mapa.
   - EraseObj: Reducción parcial de cantidades, remoción completa a Amount <= 0, emisión de ObjectDelete.
   - DropObj: Descarte estándar, recorte por capacidad máxima, prohibición de tirar ítems NoSeCae o Newbie, verificación de celda ocupada en el mapa del usuario, y **prueba exhaustiva del Bug #29 (duplicación por recorte asimétrico)**.
   - GetObj: Recolección directa de oro a la billetera (Stats.GLD), ingreso de objetos al inventario, comprobación de Agarrable == 1, celda vacía o inventario lleno.

2. **G2: Gestión Base de Inventario y Descarte (50 tests)**:
   - MeterItemEnInventario: Apilamiento en slot existente hasta 10.000, búsqueda de primer slot libre en grilla dispersa, rechazo por inventario lleno, respeto del límite de slots activos (CurrentInventorySlots).
   - QuitarUserInvItem: Descuento parcial, vaciado total de slot sin corrimiento de elementos contiguos, desequipamiento automático si estaba en uso, actualización del contador NroItems.
   - UpdateUserInv: Actualización de slot individual y barrido completo (update_all = true) invocando protocolo / hook.
   - LimpiarInventario: Blanqueo completo de los 30 slots y reseteo de los 8 punteros de equipamiento rápido.
   - TieneObjetosRobables: Exclusión de barcos y llaves, detección de ítems comunes.
   - QuitarNewbieObj: Remoción de ítems novatos y preservación del quirk de UpdateUserInv incondicional.
   - ItemSeCae / ItemNewbie / getObjType: Comportamiento frente a facciones, cotas de NumObjDatas y tipos inválidos.
   - TirarTodo, TirarTodosLosItems, TirarTodosLosItemsNoNewbies, TirarTodosLosItemsEnMochila: Comprobación de zonas seguras (ZONAPELEA), arrojamiento a celdas libres adyacentes y exclusión de newbies.
   - TirarOro: Arrojamiento de oro en pilas de hasta 10k, quirk de barco pirata (476), y **prueba exhaustiva del Bug #30 (evaporación de excedente > 500k)**.

3. **G3: Restricciones y Sistema de Equipamiento (47 tests)**:
   - Validaciones de restricciones: ClasePuedeUsarItem (incluyendo bypass total de Dios/GM), SexoPuedeUsarItem, FaccionPuedeUsarItem (Armada Real, Fuerzas del Caos) y CheckRazaUsaRopa (compatibilidad de enanos/gnomos y exclusivo drow).
   - EquiparInvItem: Comportamiento de toggle si ya estaba equipado, reemplazo de ítem preexistente en la misma categoría, bloqueo recíproco de armas a dos manos vs. escudos, ampliación de mochila a 25/30 slots (WriteAddSlots), aplicación de modificadores de defensa/ataque, y actualización de apariencia visual (ChangeUserChar).
   - Desequipar: Restitución de apariencia por defecto (NingunArma, NingunEscudo, NingunCasco, cuerpo desnudo por raza), deducción de bonificaciones, reducción de mochila a 20 slots arrojando el excedente al suelo (TirarTodosLosItemsEnMochila), y emisión de paquete de slot.

4. **G4: Uso de Ítems, Interacción y Oficios (50 tests)**:
   - UseInvItem: Consumo de comida/bebida satisfaciendo hambre y sed, consumo de pociones rojas/azules respetando topes de vida y maná, uso de oro para acreditarse en billetera, lectura de pergaminos mágicos delegada a s_learn_spell_hook, navegación delegada a s_navega_hook, uso de minerales para abrir ventana de fundición en fraguas (s_work_request_target_hook), y bloqueo general ante usuario muerto.
   - Despacho de recetas: Verificación de invocación a los paquetes de protocolo WriteBlacksmithWeapons, WriteCarpenterObjects y WriteBlacksmithArmors desde EnivarArmasConstruibles, EnivarObjConstruibles y EnivarArmadurasConstruibles.
