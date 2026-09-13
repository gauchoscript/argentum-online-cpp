---
area: inventario-usuario
status: audit
source_files:
  - legacy/server/Codigo/InvUsuario.bas
  - legacy/server/Codigo/Modulo_InventANDobj.bas
  - legacy/server/Codigo/Modulo_UsUaRiOs.bas
  - legacy/server/Codigo/Declares.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/modSendData.bas
tags: [auditoria, inventario-usuario, equipamiento, dropobj, tiraroro, makeobj, exploits, ao-legacy, vb6, cpp]
last_updated: 2026-09-13
---

# Auditoría Técnica: Módulo de Inventario de Usuario y Objetos en el Mundo `InvUsuario.bas` (Capa 6, Módulo #19)

## 1. Resumen Ejecutivo y Alcance

Este documento expone la auditoría técnica exhaustiva del módulo `legacy/server/Codigo/InvUsuario.bas` (1.740 líneas en Visual Basic 6.0 del servidor original de Argentum Online v0.13.0).

`InvUsuario.bas` es una de las piezas angulares de la lógica del juego. Su responsabilidad abarca dos dominios estrechamente acoplados:
1. **Manipulación Física de Objetos en el Mundo**: Es el ejecutor material de la presencia física de ítems en los mapas (`MakeObj`, `EraseObj`, `DropObj`, `GetObj`), gestionando la capa `MapData(Map, X, Y).ObjInfo`.
2. **Ciclo de Vida del Inventario del Jugador**: Administra la posesión de ítems (`UserList(UserIndex).Invent`), el consumo de víveres y pociones (`UseInvItem`), la fragmentación y descarte de oro (`TirarOro`), el equipamiento defensivo y ofensivo (`EquiparInvItem`, `Desequipar`) y el descarte de pertenencias al morir (`TirarTodo`, `TirarTodosLosItems`).

El objetivo principal de esta auditoría es desglosar de forma quirúrgica sus 28 procedimientos, auditar las vulnerabilidades y fallas aritméticas históricas más notorias del juego (el exploit de duplicación de ítems en `DropObj` y la evaporación de oro en `/TIRARORO`), identificar su acoplamiento con subsistemas de red y establecer el diseño para su transliteración paritaria a C++20.

---

## 2. Catálogo Completo de Procedimientos

El módulo `legacy/server/Codigo/InvUsuario.bas` no define constantes locales ni variables estáticas a nivel módulo; todas sus operaciones consumen las estructuras globales de `Declares.bas`. Declara un total de **28 procedimientos** (27 públicos y 1 privado).

### 2.1. Clasificación Funcional en Cuatro Áreas

#### Área A: Mutaciones en el Mundo (Objetos en Mapa)
Primitivas que alteran directamente el campo `MapData(Map, X, Y).ObjInfo` y emiten paquetes de visibilidad espacial a través de `modSendData`:

| Procedimiento | Firma Exacta en VB6 | Visibilidad | Líneas VB6 | Propósito Operativo |
| :--- | :--- | :---: | :---: | :--- |
| `MakeObj` | `Sub MakeObj(ByRef Obj As Obj, ByVal Map As Integer, ByVal X As Integer, ByVal Y As Integer)` | Implícita (`Public`) | `L430-L450` | Materializa un objeto en la celda del mapa. Si ya existe el mismo ítem, acumula `Amount`; si es nuevo, asigna y difunde `PrepareMessageObjectCreate` vía `SendToAreaByPos`. |
| `EraseObj` | `Sub EraseObj(ByVal num As Integer, ByVal Map As Integer, ByVal X As Integer, ByVal Y As Integer)` | Implícita (`Public`) | `L410-L428` | Descuenta `num` unidades del objeto en el suelo. Si llega a $\le 0$, purga `ObjIndex` y `Amount`, y emite `PrepareMessageObjectDelete`. |
| `DropObj` | `Sub DropObj(ByVal UserIndex As Integer, ByVal Slot As Byte, ByVal num As Integer, ByVal Map As Integer, ByVal X As Integer, ByVal Y As Integer)` | Implícita (`Public`) | `L354-L408` | Descarta una cantidad de ítems del inventario del usuario hacia una celda del piso. Contiene el histórico **Exploit de Duplicación**. |
| `GetObj` | `Sub GetObj(ByVal UserIndex As Integer)` | Implícita (`Public`) | `L517-L578` | Recoge el objeto ubicado bajo los pies del jugador (`.Pos`). Si es oro (`otGuita`), va directo a la billetera; si es ítem, invoca `MeterItemEnInventario` y descuenta con `EraseObj`. |

#### Área B: Inventario del Jugador (Gestión de Mochila y Descarte)
Procedimientos que modifican la colección `UserList(UserIndex).Invent.Object` (slots 1 a 30) y resuelven el botín al morir o limpiar el personaje:

| Procedimiento | Firma Exacta en VB6 | Visibilidad | Líneas VB6 | Propósito Operativo |
| :--- | :--- | :---: | :---: | :--- |
| `TieneObjetosRobables` | `Public Function TieneObjetosRobables(ByVal UserIndex As Integer) As Boolean` | `Public` | `L32-L58` | Determina si el usuario posee al menos un ítem desarmable/robable (excluye newbie, barcos, oro y elementos protegidos). |
| `QuitarNewbieObj` | `Sub QuitarNewbieObj(ByVal UserIndex As Integer)` | Implícita (`Public`) | `L93-L136` | Remueve todos los ítems con flag `Newbie = 1` del inventario cuando el personaje supera el nivel máximo de novato, transportándolo a su ciudad de origen si estaba en Newbie Dungeon. |
| `LimpiarInventario` | `Sub LimpiarInventario(ByVal UserIndex As Integer)` | Implícita (`Public`) | `L138-L181` | Blanquea todos los slots de inventario (1..30) y pone en cero los 8 slots y punteros de equipamiento rápido (`ArmourEqpSlot`, `WeaponEqpSlot`, etc.). |
| `QuitarUserInvItem` | `Sub QuitarUserInvItem(ByVal UserIndex As Integer, ByVal Slot As Byte, ByVal Cantidad As Integer)` | Implícita (`Public`) | `L277-L308` | Descuenta `Cantidad` del slot indicado. Si el stack se agota y estaba equipado, convoca a `Desequipar`. Al llegar a $\le 0$, decrementa `NroItems` y blanquea el slot. |
| `UpdateUserInv` | `Sub UpdateUserInv(ByVal UpdateAll As Boolean, ByVal UserIndex As Integer, ByVal Slot As Byte)` | Implícita (`Public`) | `L310-L352` | Dispara la actualización visual del inventario hacia el cliente. Delega en `ChangeUserInv` (`Modulo_UsUaRiOs.bas:835`), el cual envía `WriteChangeInventorySlot`. |
| `MeterItemEnInventario` | `Function MeterItemEnInventario(ByVal UserIndex As Integer, ByRef MiObj As Obj) As Boolean` | Implícita (`Public`) | `L452-L515` | Intenta ubicar un ítem en un slot con el mismo `ObjIndex` cuya suma no exceda `MAX_INVENTORY_OBJS` ($10.000$); de no haber, busca un slot vacío. Si no hay espacio, aborta y devuelve `False`. |
| `TirarTodo` | `Sub TirarTodo(ByVal UserIndex As Integer)` | Implícita (`Public`) | `L1556-L1577` | Punto de entrada invocado al morir el jugador. Salvo que esté en trigger 6 (zona segura), invoca `TirarTodosLosItems` y tira el excedente de oro que supere `Nivel * 10.000` con `TirarOro`. |
| `ItemSeCae` | `Public Function ItemSeCae(ByVal index As Integer) As Boolean` | `Public` | `L1579-L1594` | Consulta en `ObjDataList` si un ítem cae al suelo al morir el usuario (verifica `NoSeCae`, facciones real/caos, llaves y barcos). |
| `TirarTodosLosItems` | `Sub TirarTodosLosItems(ByVal UserIndex As Integer)` | Implícita (`Public`) | `L1596-L1643` | Recorre los slots activos (1 a `CurrentInventorySlots`); para cada ítem que `ItemSeCae`, busca celda con `Tilelibre` e invoca `DropObj`. |
| `ItemNewbie` | `Function ItemNewbie(ByVal ItemIndex As Integer) As Boolean` | Implícita (`Public`) | `L1645-L1655` | Devuelve `True` si el ítem tiene configurado el flag `Newbie = 1` en `ObjDataList`. |
| `TirarTodosLosItemsNoNewbies` | `Sub TirarTodosLosItemsNoNewbies(ByVal UserIndex As Integer)` | Implícita (`Public`) | `L1657-L1693` | Variante de descarte para zonas donde los novatos no caen con ítems newbie. |
| `TirarTodosLosItemsEnMochila` | `Sub TirarTodosLosItemsEnMochila(ByVal UserIndex As Integer)` | Implícita (`Public`) | `L1695-L1727` | Arroja al suelo todos los ítems situados en slots expandidos (slots 21 a 30) al desequipar una mochila. |
| `getObjType` | `Public Function getObjType(ByVal ObjIndex As Integer) As eOBJType` | `Public` | `L1729-L1740` | Rutina utilitaria de consulta rápida que devuelve el `OBJType` configurado para un índice de objeto. |

> [!NOTE]
> **Aclaración sobre `MoverItem`**: En el código fuente legacy de Visual Basic 6.0 **no existe** una subrutina denominada `MoverItem` en `InvUsuario.bas`. En el cliente de AO el reordenamiento visual de ítems en la cuadrícula de inventario es manejado de manera puramente local por el cliente hasta que interactúa con el servidor, y la actualización unitaria de slots en el servidor se canaliza a través de `ChangeUserInv(UserIndex, Slot, Object)` (`Modulo_UsUaRiOs.bas:835`) y `Protocol.WriteChangeInventorySlot`.

#### Área C: Equipamiento y Validaciones de Restricción
Lógica de verificación de compatibilidad física, racial, faccionaria y de clase, y asignación de slots de equipamiento activo:

| Procedimiento | Firma Exacta en VB6 | Visibilidad | Líneas VB6 | Propósito Operativo |
| :--- | :--- | :---: | :---: | :--- |
| `ClasePuedeUsarItem` | `Function ClasePuedeUsarItem(ByVal UserIndex As Integer, ByVal ObjIndex As Integer, Optional ByRef sMotivo As String) As Boolean` | Implícita (`Public`) | `L60-L91` | Valida contra el arreglo `ClaseProhibida(1..NUMCLASES)` de `ObjDataList`. Admins con privilegios tienen bypass total. |
| `SexoPuedeUsarItem` | `Function SexoPuedeUsarItem(ByVal UserIndex As Integer, ByVal ObjIndex As Integer, Optional ByRef sMotivo As String) As Boolean` | Implícita (`Public`) | `L694-L717` | Valida las banderas `Mujer = 1` y `Hombre = 1` de `ObjDataList` contra el género del personaje. |
| `FaccionPuedeUsarItem` | `Function FaccionPuedeUsarItem(ByVal UserIndex As Integer, ByVal ObjIndex As Integer, Optional ByRef sMotivo As String) As Boolean` | Implícita (`Public`) | `L719-L744` | Valida las banderas `Real = 1` (requiere ser ciudadano no criminal y miembro de la Armada) y `Caos = 1` (requiere ser criminal y miembro de la Legión). |
| `EquiparInvItem` | `Sub EquiparInvItem(ByVal UserIndex As Integer, ByVal Slot As Byte)` | Implícita (`Public`) | `L746-L1000` | Equipa un ítem en su slot correspondiente (`otWeapon`, `otAnillo`, `otFlechas`, `otArmadura`, `otCASCO`, `otESCUDO`, `otMochilas`). Si ya estaba equipado, lo desequipa. Quita el ítem anteriormente equipado en ese slot. Actualiza animaciones gráficas (`ChangeUserChar`). |
| `CheckRazaUsaRopa` | `Private Function CheckRazaUsaRopa(ByVal UserIndex As Integer, ItemIndex As Integer, Optional ByRef sMotivo As String) As Boolean` | `Private` | `L1002-L1034` | Valida que enanos y gnomos usen exclusivamente ropa con `RazaEnana = 1`, y que humanos/elfos/drows usen ropa con `RazaEnana = 0`. Bloquea ropa con `RazaDrow = 1` para no-drows. |
| `Desequipar` | `Sub Desequipar(ByVal UserIndex As Integer, ByVal Slot As Byte)` | Implícita (`Public`) | `L580-L692` | Desequipa el ítem del slot. Pone a cero los slots y punteros rápidos (`WeaponEqpSlot`, etc.), resetea animaciones corporales a sus estados por defecto (`NingunArma`, `NingunEscudo`, `NingunCasco`, `DarCuerpoDesnudo`) y si era mochila, tira los ítems residuales con `TirarTodosLosItemsEnMochila`. |

#### Área D: Uso e Interacción (Consumibles, Herramientas, Oro y Crafting)
Comandos de interacción activa por parte del usuario:

| Procedimiento | Firma Exacta en VB6 | Visibilidad | Líneas VB6 | Propósito Operativo |
| :--- | :--- | :---: | :---: | :--- |
| `TirarOro` | `Sub TirarOro(ByVal Cantidad As Long, ByVal UserIndex As Integer)` | Implícita (`Public`) | `L183-L275` | Descuenta oro de la billetera y lo arroja al piso en pilas de hasta $10.000$ usando `TirarItemAlPiso`. Contiene el histórico **Bug de Pérdida de Saldo > 500k**. |
| `UseInvItem` | `Sub UseInvItem(ByVal UserIndex As Integer, ByVal Slot As Byte)` | Implícita (`Public`) | `L1036-L1524` | Máquina de estados gigante para uso de ítems: `otUseOnce` (comida), `otGuita` (ingreso de oro a billetera), `otWeapon` (activación de herramientas de trabajo o forja), `otPociones` (modificadores de atributos, vida, maná, veneno), `otBebidas`, `otLlaves`, `otBotellaVacia/Llena`, `otPergaminos` (aprender hechizo), `otMinerales`, `otInstrumentos` y `otBarcos` (`DoNavega`). |
| `EnivarArmasConstruibles` | `Sub EnivarArmasConstruibles(ByVal UserIndex As Integer)` | Implícita (`Public`) | `L1526-L1534` | Despacha la lista de armas que puede forjar el herrero (`WriteBlacksmithWeapons`). Conserva el typo histórico "Enivar". |
| `EnivarObjConstruibles` | `Sub EnivarObjConstruibles(ByVal UserIndex As Integer)` | Implícita (`Public`) | `L1536-L1544` | Despacha la lista de objetos de carpintería (`WriteCarpenterObjects`). Conserva el typo histórico "Enivar". |
| `EnivarArmadurasConstruibles` | `Sub EnivarArmadurasConstruibles(ByVal UserIndex As Integer)` | Implícita (`Public`) | `L1546-L1554` | Despacha la lista de armaduras que puede forjar el herrero (`WriteBlacksmithArmors`). Conserva el typo histórico "Enivar". |

---

## 3. Mapeo de Estructuras y Estado Global

### 3.1. Acceso a `UserList(UserIndex).Invent`
El inventario del jugador es una estructura híbrida que contiene tanto el arreglo plano de slots como los punteros de acceso rápido a los ítems actualmente vestidos:

```text
UserList(UserIndex).Invent:
├── Object(1 To 30) As UserOBJ:
│   ├── ObjIndex As Integer  (Índice a ObjDataList)
│   ├── Amount As Integer    (Cantidad en el slot, hasta 10.000)
│   └── Equipped As Byte     (1 si está vestido/en uso, 0 si no)
├── NroItems As Integer       (Cantidad de slots ocupados actualmente)
├── WeaponEqpSlot / ObjIndex  (Slot y ObjIndex del arma activa)
├── ArmourEqpSlot / ObjIndex  (Slot y ObjIndex de la armadura activa)
├── CascoEqpSlot / ObjIndex   (Slot y ObjIndex del casco/gorro activo)
├── EscudoEqpSlot / ObjIndex  (Slot y ObjIndex del escudo activo)
├── AnilloEqpSlot / ObjIndex  (Slot y ObjIndex del anillo/accesorio activo)
├── MunicionEqpSlot / ObjIndex(Slot y ObjIndex de flechas/munición activa)
├── BarcoSlot / BarcoObjIndex (Slot y ObjIndex del barco activo)
├── MochilaEqpSlot / ObjIndex (Slot y ObjIndex de la mochila equipada)
└── CurrentInventorySlots     (Capacidad visible: 20 base, 25 o 30 con mochila)
```

### 3.2. Dependencias de Red y Protocolo Saliente
`InvUsuario.bas` no interactúa con sockets crudos; despacha paquetes a través de dos canales:
1. **Difusión Espacial (`modSendData.SendToAreaByPos`)**:
   - `PrepareMessageObjectCreate(GrhIndex, X, Y)`: Emitido en `MakeObj` cuando un ítem nuevo aparece en el suelo.
   - `PrepareMessageObjectDelete(X, Y)`: Emitido en `EraseObj` cuando una pila del suelo se extingue.
   - `PrepareMessagePlayWave(Snd, X, Y)`: Sonido al desenvainar arma (`SND_SACARARMA = 30`) o tocar instrumentos.
2. **Mensajes Unicast al Cliente (`Protocol.Write...`)**:
   - `WriteChangeInventorySlot(UserIndex, Slot)`: Actualización del slot tras modificar cantidad, equipar o vaciar.
   - `WriteUpdateUserStats(UserIndex)`: Actualización de fuerza/agilidad/defensa tras tomar pociones o equipar.
   - `WriteUpdateGold(UserIndex)`: Tras recoger o tirar oro.
   - `WriteUpdateHungerAndThirst(UserIndex)`: Tras comer o beber.
   - `WriteConsoleMsg(UserIndex, mensaje, font)`: Notificaciones de error ("No hay espacio en el piso", "Tu clase no puede usar este objeto", etc.).
   - `WriteAddSlots(UserIndex, MochilaType)`: Avisa al cliente que debe expandir la grilla de inventario a 25 o 30 slots.

---

## 4. Auditoría Quirúrgica de Exploits y Defectos Históricos

### 4.1. Exploit de Duplicación en `DropObj` (Bug #29)

- **Cita en Código Legacy**: `legacy/server/Codigo/InvUsuario.bas:368-380`.
- **Diagnóstico del Fallo**:
  ```vb
  368:  'Check objeto en el suelo
  369:  If MapData(.Pos.Map, X, Y).ObjInfo.ObjIndex = 0 Or MapData(.Pos.Map, X, Y).ObjInfo.ObjIndex = Obj.ObjIndex Then
  370:      If num + MapData(.Pos.Map, X, Y).ObjInfo.Amount > MAX_INVENTORY_OBJS Then
  371:          num = MAX_INVENTORY_OBJS - MapData(.Pos.Map, X, Y).ObjInfo.Amount
  372:      End If
  373:      
  374:      Call MakeObj(Obj, Map, X, Y)
  375:      Call QuitarUserInvItem(UserIndex, Slot, num)
  376:      Call UpdateUserInv(False, UserIndex, Slot)
  ```
  En la línea 363 previa, se asignó `Obj.Amount = num` (con el valor original pretendido por el jugador, por ejemplo $1.000$).
  En la línea 370-371, si la celda receptora ya tiene $9.990$ unidades, el código detecta el desborde y recorta la variable local `num` a $10$ (`10000 - 9990`).
  **Sin embargo, la estructura `Obj` que se le pasa a `MakeObj` en la línea 374 retiene `Obj.Amount = 1000`**.
  En consecuencia:
  - `MakeObj` suma los $1.000$ íntegros al suelo, inflando la celda a $10.990$.
  - `QuitarUserInvItem` en la línea 375 descuenta únicamente el `num` recortado ($10$) del inventario del jugador.
  - El jugador retiene $990$ ítems en su mochila y la celda del piso recibe los $1.000$ completos: **se acaban de generar $990$ ítems de la nada**.
- **Impacto y Tratamiento en C++**: Este es un **exploit crítico de economía**. Siguiendo la política de paridad, se documenta formalmente en el Master Bug Ledger para registrar el quirk histórico, y se evaluará su mitigación controlada o preservación bajo flag de configuración.

### 4.2. Pérdida Silenciosa de Saldo en `TirarOro` (Bug #30)

- **Cita en Código Legacy**: `legacy/server/Codigo/InvUsuario.bas:228-268`.
- **Diagnóstico del Fallo**:
  ```vb
  228:  Dim Extra As Long
  229:  Dim TeniaOro As Long
  230:  TeniaOro = .Stats.GLD
  231:  If Cantidad > 500000 Then 'Para evitar explotar demasiado
  232:      Extra = Cantidad - 500000
  233:      Cantidad = 500000
  234:  End If
  ...
  264:  If TeniaOro = .Stats.GLD Then Extra = 0
  265:  If Extra > 0 Then
  266:      .Stats.GLD = .Stats.GLD - Extra
  267:  End If
  ```
  Si un jugador ejecuta `/TIRARORO 800000`:
  - `Extra` almacena $300.000$ ($800.000 - 500.000$).
  - `Cantidad` se acota a $500.000$, los cuales se arrojan al piso en 50 pilas de $10.000$.
  - Como el jugador arrojó oro, su balance cambió (`TeniaOro <> .Stats.GLD`), por lo que la línea 264 no anula `Extra`.
  - La línea 266 ejecuta `.Stats.GLD = .Stats.GLD - 300000`.
  - **Resultado**: Los $300.000$ de oro excedentes se descuentan incondicionalmente de la billetera del usuario **sin materializarse en ninguna parte del mundo**. Se evaporan silenciosamente.

### 4.3. Riesgo de Desbordamiento Entero de 16 Bits en `MakeObj` (Run-time Error 6)

- **Cita en Código Legacy**: `legacy/server/Codigo/InvUsuario.bas:436`.
  ```vb
  If .ObjInfo.ObjIndex = Obj.ObjIndex Then
      .ObjInfo.Amount = .ObjInfo.Amount + Obj.Amount
  ```
  En VB6, `ObjInfo.Amount` y `Obj.Amount` son `Integer` con signo ($[-32.768, 32.767]$).
  Si a causa del exploit anterior o manipulación indebida la celda acumula más de $32.767$ unidades, la adición dispara en tiempo de ejecución el `Error 6: Overflow`, provocando la caída del servidor si no está atrapado por un `On Error`. En C++20, la suma de tipos `int16_t` con signo debe cuidarse mediante casting previo a `int32_t` para evitar comportamiento indefinido (*Undefined Behavior* por integer overflow).

### 4.4. Bug del "Ítem Fantasma" en `MeterItemEnInventario`

- **Cita en Código Legacy**: `legacy/server/Codigo/InvUsuario.bas:500-504`.
  ```vb
  If .Invent.Object(Slot).Amount + MiObj.Amount <= MAX_INVENTORY_OBJS Then
      .Invent.Object(Slot).ObjIndex = MiObj.ObjIndex
      .Invent.Object(Slot).Amount = .Invent.Object(Slot).Amount + MiObj.Amount
  Else
      .Invent.Object(Slot).Amount = MAX_INVENTORY_OBJS
  End If
  ```
  Si un usuario recoge una pila corrompida que excede $10.000$ (por ejemplo $10.990$) sobre un slot vacío:
  - Entra a la rama `Else`.
  - Se asigna `.Amount = 10000`.
  - **¡Se omite asignar `.ObjIndex = MiObj.ObjIndex`!**
  - El slot permanece con `ObjIndex = 0` pero con `Amount = 10000`: se genera un slot fantasma corrupto que traba el inventario del usuario.

### 4.5. Otros Quirks y Vicios de Sintaxis Detectados

1. **Incoherencia de Mapa en `DropObj` (Línea 370)**:
   La subrutina recibe el parámetro `ByVal Map As Integer`, pero al verificar si la celda está ocupada evalúa `MapData(.Pos.Map, X, Y)` utilizando la posición del usuario en vez del mapa destino recibido por argumento.
2. **Indentación Engañosa en `QuitarNewbieObj` (Líneas 106-107)**:
   ```vb
   If ObjData(.Invent.Object(j).ObjIndex).Newbie = 1 Then _
          Call QuitarUserInvItem(UserIndex, j, MAX_INVENTORY_OBJS)
          Call UpdateUserInv(False, UserIndex, j)
   ```
   El guión bajo `_` enlaza únicamente la llamada a `QuitarUserInvItem`. La llamada `Call UpdateUserInv` queda fuera del condicional y se ejecuta indiscriminadamente para todos los slots activos del jugador.
3. **Hardcodeo de `MAX_INVENTORY_OBJS` en `TirarTodosLosItems` (Línea 1637)**:
   Al arrojar las pertenencias al morir, invoca `DropObj(UserIndex, i, MAX_INVENTORY_OBJS, ...)` en lugar de pasar la cantidad exacta `.Invent.Object(i).Amount`.
4. **Retención de Ítems en Celdas Saturadas al Morir (Línea 1636)**:
   Si `Tilelibre` devuelve `(0, 0)` porque todo el radio de 15 celdas está bloqueado, `DropObj` no se convoca y el personaje muerto **retiene el ítem en su inventario**, violando la regla de pérdida de botín al morir.
5. **Typos en Nombres de Métodos de Crafting (Líneas 1526, 1536, 1546)**:
   Se nombraron históricamente `EnivarArmasConstruibles`, `EnivarObjConstruibles` y `EnivarArmadurasConstruibles` (con "Enivar" en lugar de "Enviar").

---

## 5. Integración con Módulos Previos y Puntos de Conexión

### Conexión con el Hook `SetMakeObjHook` de `Modulo_InventANDobj`
En el cierre del Módulo #18 (`Modulo_InventANDobj.hpp`), se expuso el siguiente hook:
```cpp
using MakeObjHook = std::function<void(const Obj& obj, std::int16_t map, std::int16_t x, std::int16_t y)>;
void SetMakeObjHook(MakeObjHook hook) noexcept;
```
La primitiva `MakeObj` de `InvUsuario.bas:430` es exactamente la implementación productiva de este contrato. Al implementar `src/server/InvUsuario.hpp` y `InvUsuario.cpp`:
1. `InvUsuario::MakeObj` recibirá `(const Obj& obj, int16_t map, int16_t x, int16_t y)`.
2. Durante el arranque del servidor (`General.pas` / `main.cpp`), se conectará:
   ```cpp
   Modulo_InventANDobj::SetMakeObjHook(InvUsuario::MakeObj);
   ```
3. Esto permitirá que la muerte de criaturas (`NPC_TIRAR_ITEMS` -> `TirarItemAlPiso`) materialice el botín en el suelo a través de `MakeObj` y despache las notificaciones de área `PrepareMessageObjectCreate` sin acoplamiento circular.

---

## 6. Conclusiones y Próximos Pasos

1. `InvUsuario.bas` concentra la lógica crítica de objetos del jugador y del mapa, conteniendo fallas de sincronización aritmética históricas que moldearon la economía del juego original.
2. Para el port a C++20, se recomienda estructurar la implementación en tres fases bien delimitadas:
   - **Fase 1 (G1 — Mutaciones en el Mundo)**: `MakeObj`, `EraseObj`, `DropObj`, `GetObj` y replicación del Bug #29.
   - **Fase 2 (G2 — Inventario Base y Descarte)**: `MeterItemEnInventario`, `QuitarUserInvItem`, `UpdateUserInv`, `LimpiarInventario`, `TirarTodo`, `TirarTodosLosItems`, `TirarOro` y replicación del Bug #30.
   - **Fase 3 (G3 — Restricciones y Equipamiento)**: `EquiparInvItem`, `Desequipar`, `ClasePuedeUsarItem`, `SexoPuedeUsarItem`, `FaccionPuedeUsarItem`, `CheckRazaUsaRopa` y `UseInvItem`.
3. Siguiente paso: registrar formalmente los Bugs #29 y #30 en `docs/implementation/KNOWN-LEGACY-BUGS.md` y redactar el plan de trabajo modular en `docs/implementation/19-invusuario-breakdown.md`.
