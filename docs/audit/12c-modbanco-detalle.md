---
area: objetos-inventario-comercio
source_files:
  - legacy/server/Codigo/modBanco.bas
  - legacy/server/Codigo/Declares.bas
  - legacy/server/Codigo/InvUsuario.bas
  - legacy/server/Codigo/Protocol.bas
  - legacy/server/Codigo/FileIO.bas
tags: [banco, boveda, inventario, objetos, transacciones, exploits, overflow, economia]
last_updated: 2026-09-13
---

# Auditoría Técnica: Módulo de Bóveda Bancaria `modBanco.bas` (Capa 6, Módulo #20)

## 1. Resumen Ejecutivo y Alcance

Este documento expone la auditoría técnica exhaustiva del módulo `legacy/server/Codigo/modBanco.bas` (300 líneas en Visual Basic 6.0 del servidor original de Argentum Online v0.13.0), complementado con el análisis de sus fronteras transaccionales con `InvUsuario.bas`, `Protocol.bas` y `FileIO.bas`.

`modBanco.bas` es el responsable de gestionar el almacenamiento seguro de objetos pertenecientes a los personajes jugadores dentro de la bóveda bancaria del mundo de juego. Sus responsabilidades comprenden:
1. **Apertura y Cierre de Bóveda**: Coordinar el inicio de la sesión de depósito con el NPC Banquero (`IniciarDeposito`), notificando estados de red.
2. **Depósito y Retiro de Objetos**: Transferir ítems entre la mochila del usuario (`UserList(UserIndex).Invent`) y su inventario de bóveda (`UserList(UserIndex).BancoInvent`), preservando la cantidad apilada y el identificador de tipo.
3. **Sincronización con el Cliente**: Despachar actualizaciones completas o unitarias de los slots bancarios (`SendBanObj`, `UpdateBanUserInv`, `UpdateVentanaBanco`).
4. **Herramientas de Auditoría GM**: Permitir la inspección textual en tiempo real del contenido de la bóveda tanto de personajes conectados (`SendUserBovedaTxt`) como de personajes desconectados leyendo su archivo de personaje (`SendUserBovedaTxtFromChar`).

Adicionalmente, esta auditoría examina la segregación del manejo de **oro en banco** (`Stats.Banco`), el cual históricamente **no reside en `modBanco.bas`**, sino que fue implementado directamente en `Protocol.bas` en los manejadores de paquetes `HandleBankExtractGold` y `HandleBankDepositGold`.

---

## 2. Catálogo Completo de Procedimientos y Constantes

El módulo `legacy/server/Codigo/modBanco.bas` declara un total de **11 procedimientos** (todos `Sub`, retorno `void`). Ninguno cuenta con modificador de ámbito explícito, por lo que bajo la semántica de Visual Basic 6.0 poseen visibilidad **`Public` implícita**.

### 2.1. Inventario Exhaustivo de Procedimientos

| Procedimiento | Firma Exacta en VB6 | Visibilidad | Líneas VB6 | Propósito Operativo |
| :--- | :--- | :---: | :---: | :--- |
| `IniciarDeposito` | `Sub IniciarDeposito(ByVal UserIndex As Integer)` | Implícita (`Public`) | `L25-L48` | Inicia la interacción con el banquero: sincroniza toda la bóveda al cliente (`UpdateBanUserInv`), refresca estadísticas/oro (`WriteUpdateUserStats`), emite apertura de diálogo (`WriteBankInit`) y activa `.flags.Comerciando = True`. |
| `SendBanObj` | `Sub SendBanObj(UserIndex As Integer, Slot As Byte, Object As UserOBJ)` | Implícita (`Public`) | `L50-L58` | Asigna `Object` en `.BancoInvent.Object(Slot)` y despacha el paquete unitario `WriteChangeBankSlot`. Nota: los parámetros se pasan por referencia (`ByRef` por defecto en VB6). |
| `UpdateBanUserInv` | `Sub UpdateBanUserInv(ByVal UpdateAll As Boolean, ByVal UserIndex As Integer, ByVal Slot As Byte)` | Implícita (`Public`) | `L60-L91` | Si `UpdateAll = True`, recorre en bucle los 40 slots bancarios despachando `SendBanObj`. Si es `False`, despacha únicamente el `Slot` provisto. Envía un objeto nulo si `ObjIndex = 0`. |
| `UserRetiraItem` | `Sub UserRetiraItem(ByVal UserIndex As Integer, ByVal i As Integer, ByVal Cantidad As Integer)` | Implícita (`Public`) | `L93-L122` | Punto de entrada invocado desde `HandleBankExtractItem`. Comprueba `Cantidad < 1`, acota `Cantidad` al disponible en el slot bancario `i`, invoca `UserReciveObj`, y fuerza el refresco total de mochila (`UpdateUserInv`), bóveda (`UpdateBanUserInv`) y confirmación (`UpdateVentanaBanco`). |
| `UserReciveObj` | `Sub UserReciveObj(ByVal UserIndex As Integer, ByVal ObjIndex As Integer, ByVal Cantidad As Integer)` | Implícita (`Public`) | `L124-L178` | Transfiere el objeto desde el slot bancario `ObjIndex` hacia la mochila: busca slot apilable existente o primera ranura vacía, suma cantidad en mochila e invoca `QuitarBancoInvItem`. (*Nota: contiene errata histórica de tipeo en el identificador legacy `UserReciveObj`*). |
| `QuitarBancoInvItem` | `Sub QuitarBancoInvItem(ByVal UserIndex As Integer, ByVal Slot As Byte, ByVal Cantidad As Integer)` | Implícita (`Public`) | `L180-L204` | Resta `Cantidad` de `.BancoInvent.Object(Slot).Amount`. Si la cantidad resultante es <= 0, decrementa `.BancoInvent.NroItems` y blanquea el slot (`ObjIndex = 0`, `Amount = 0`). |
| `UpdateVentanaBanco` | `Sub UpdateVentanaBanco(ByVal UserIndex As Integer)` | Implícita (`Public`) | `L206-L214` | Emite al socket del cliente el mensaje `WriteBankOK` notificando finalización exitosa de transacción de ítems. |
| `UserDepositaItem` | `Sub UserDepositaItem(ByVal UserIndex As Integer, ByVal Item As Integer, ByVal Cantidad As Integer)` | Implícita (`Public`) | `L216-L245` | Punto de entrada invocado desde `HandleBankDeposit`. Comprueba disponibilidad en mochila, acota `Cantidad`, invoca `UserDejaObj`, y fuerza el refresco total de mochila, bóveda y ventana bancaria. |
| `UserDejaObj` | `Sub UserDejaObj(ByVal UserIndex As Integer, ByVal ObjIndex As Integer, ByVal Cantidad As Integer)` | Implícita (`Public`) | `L247-L299` | Transfiere el objeto desde el slot de mochila `ObjIndex` hacia la bóveda: busca slot bancario apilable o primera ranura vacía, asigna en bóveda e invoca `QuitarUserInvItem` de `InvUsuario.bas`. |
| `SendUserBovedaTxt` | `Sub SendUserBovedaTxt(ByVal sendIndex As Integer, ByVal UserIndex As Integer)` | Implícita (`Public`) | `L301-L321` | Inspección GM: envía mensajes de texto por consola (`WriteConsoleMsg`) al GM `sendIndex` listando todos los ítems ocupados en la bóveda del usuario conectado `UserIndex`. |
| `SendUserBovedaTxtFromChar` | `Sub SendUserBovedaTxtFromChar(ByVal sendIndex As Integer, ByVal charName As String)` | Implícita (`Public`) | `L323-L350` | Inspección GM offline: lee mediante `GetVar` la sección `[BancoInventory]` del archivo `.chr` de `charName` e imprime el listado por consola. |

### 2.2. Constantes de Dominio y Parámetros Globales

Las siguientes constantes globales de `legacy/server/Codigo/Declares.bas` gobiernan el funcionamiento de la bóveda:

- `MAX_BANCOINVENTORY_SLOTS As Byte = 40` (`Declares.bas:886`): Define el límite absoluto de ranuras fijas de la bóveda bancaria para cualquier personaje.
- `MAX_INVENTORY_OBJS As Integer = 10000` (`Declares.bas:488`): Tope máximo de acumulación/apilamiento de unidades por ranura individual, aplicable tanto al inventario del usuario como a la bóveda bancaria.
- `FontTypeNames.FONTTYPE_INFO` (`Declares.bas`): Formato de texto informativo para las respuestas por consola de error o inspección de bóveda.
- **Oro en Bóveda**: No existe constante de tope máximo de oro en banco (`MAX_ORO_BANCO`) en el código legacy; el valor `.Stats.Banco` opera como un entero de 32 bits con signo (`Long` de VB6, hasta 2.147.483.647), careciendo de tope defensivo frente a desbordes aritméticos.

---

## 3. Mapeo de Estructuras y Modelo de Almacenamiento

### 3.1. Estructura de Memoria: `BancoInventario`

En `legacy/server/Codigo/Declares.bas:890-893`, la bóveda se modela dentro del tipo `User` (`UserList(UserIndex).BancoInvent`) como:

```vb
Public Type BancoInventario
    Object(1 To MAX_BANCOINVENTORY_SLOTS) As UserOBJ
    NroItems As Integer
End Type
```

Donde `UserOBJ` (`Declares.bas:879-883`) almacena:
```vb
Public Type UserOBJ
    ObjIndex As Integer
    Amount As Integer
    Equipped As Byte
End Type
```

> [!NOTE]
> Aunque `UserOBJ` incluye el campo `Equipped`, en la bóveda bancaria este campo carece de significado semántico y permanece en `0`.

### 3.2. Modelo de Grilla: Grilla Fija Dispersa (*Sparse Grid*) vs. Compactación

Una pregunta de diseño esencial es si la bóveda compacta sus elementos hacia la izquierda al retirar o si preserva los slots vacíos.

La auditoría de `QuitarBancoInvItem` (`modBanco.bas:195-201`) demuestra de manera concluyente que **la bóveda opera como una grilla fija dispersa (*sparse grid*)**:

```vb
.BancoInvent.Object(Slot).Amount = .BancoInvent.Object(Slot).Amount - Cantidad

If .BancoInvent.Object(Slot).Amount <= 0 Then
    .BancoInvent.NroItems = .BancoInvent.NroItems - 1
    .BancoInvent.Object(Slot).ObjIndex = 0
    .BancoInvent.Object(Slot).Amount = 0
End If
```

- **Preservación de Ranuras**: Al vaciar un slot, el procedimiento únicamente blanquea `ObjIndex = 0` y `Amount = 0`, y decrementa el contador `NroItems`. **En ningún momento desplaza o compacta los elementos subsiguientes.**
- **Reordenamiento Explícito por el Cliente**: En `Protocol.bas:3888` (`HandleMoveBank`), el protocolo expone el comando para mover e intercambiar ítems entre ranuras contiguas (`dir = 1` desplaza hacia arriba e intercambia con `Slot - 1`; `dir = -1` desplaza hacia abajo con `Slot + 1`), lo cual ratifica que la posición exacta de cada objeto en la grilla de 40 ranuras es un estado persistente y visualmente controlado por el jugador.

### 3.3. Mecánica de Apilamiento e Inserción (`UserDepositaItem` / `UserDejaObj`)

Al depositar un objeto (`modBanco.bas:247-298`):
1. **Validación Previa**: Si `Cantidad < 1`, interrumpe de inmediato (`Exit Sub`).
2. **Búsqueda de Ranura Apilable**:
   ```vb
   Slot = 1
   Do Until .BancoInvent.Object(Slot).ObjIndex = obji And _
       .BancoInvent.Object(Slot).Amount + Cantidad <= MAX_INVENTORY_OBJS
       Slot = Slot + 1
       If Slot > MAX_BANCOINVENTORY_SLOTS Then Exit Do
   Loop
   ```
   Recorre desde el slot 1 hasta el 40 buscando coincidencia exacta de `ObjIndex` donde la suma de `Amount + Cantidad` no supere `MAX_INVENTORY_OBJS` (10.000).
3. **Búsqueda de Primera Ranura Vacía**:
   Si no halló ranura apilable (`Slot > MAX_BANCOINVENTORY_SLOTS`), reinicia `Slot = 1` y busca la primera celda con `ObjIndex = 0`:
   ```vb
   If Slot > MAX_BANCOINVENTORY_SLOTS Then
       Slot = 1
       Do Until .BancoInvent.Object(Slot).ObjIndex = 0
           Slot = Slot + 1
           If Slot > MAX_BANCOINVENTORY_SLOTS Then
               Call WriteConsoleMsg(UserIndex, "No tienes mas espacio en el banco!!", FontTypeNames.FONTTYPE_INFO)
               Exit Sub
           End If
       Loop
       .BancoInvent.NroItems = .BancoInvent.NroItems + 1
   End If
   ```
4. **Inserción y Remoción**:
   Si `Slot <= MAX_BANCOINVENTORY_SLOTS` y `Amount + Cantidad <= MAX_INVENTORY_OBJS`:
   - Asigna `.BancoInvent.Object(Slot).ObjIndex = obji`
   - Suma `.BancoInvent.Object(Slot).Amount = Amount + Cantidad`
   - Invoca a la mochila: `Call QuitarUserInvItem(UserIndex, CByte(ObjIndex), Cantidad)`

### 3.4. Mecánica de Extracción (`UserRetiraItem` / `UserReciveObj`)

Al retirar un objeto (`modBanco.bas:93-178`):
1. **Validación Previa**: Si `Cantidad < 1`, sale silenciosamente. Acota `Cantidad` si el usuario solicita retirar más de lo que posee el slot bancario.
2. **Búsqueda en Mochila de Usuario**:
   En `UserReciveObj`:
   - Busca en la mochila (`.Invent.Object`) si ya existe un slot con el mismo objeto donde `Amount + Cantidad <= MAX_INVENTORY_OBJS`.
   - Si no existe, busca la primera ranura vacía (`ObjIndex = 0`) hasta `.CurrentInventorySlots` (20 o 30 según si lleva mochila equipada). Si la encuentra, incrementa `.Invent.NroItems = .Invent.NroItems + 1`. Si no hay ranuras libres, emite `"No podés tener mas objetos."` y aborta.
3. **Acreditación y Descuento en Bóveda**:
   - Acredita el objeto en `.Invent.Object(Slot)`.
   - Descuenta de la bóveda llamando a `QuitarBancoInvItem(UserIndex, CByte(ObjIndex), Cantidad)`.

---

## 4. Frontera Transaccional con `InvUsuario` y Protocolo

### 4.1. Interacción con `InvUsuario.bas`

El módulo `modBanco.bas` se acopla directamente con `InvUsuario.bas` a través de dos rutinas:

1. **`QuitarUserInvItem(UserIndex, CByte(ObjIndex), Cantidad)` (`InvUsuario.bas:277`)**:
   - Invocada al depositar dentro de `UserDejaObj` (`modBanco.bas:290`).
   - Se encarga de descontar las unidades de la mochila del jugador, gestionar el desequipamiento automático si el ítem estaba vestido (`Desequipar`), y limpiar el slot del inventario cuando la cantidad llega a cero.
2. **`UpdateUserInv(True, UserIndex, 0)` (`InvUsuario.bas:310`)**:
   - Invocada al finalizar `UserRetiraItem` (`modBanco.bas:112`) y `UserDepositaItem` (`modBanco.bas:234`).
   - Al recibir `UpdateAll = True`, itera del slot 1 al 30 invocando `ChangeUserInv` (`Modulo_UsUaRiOs.bas:835`), el cual despacha hacia el cliente los paquetes de red `WriteChangeInventorySlot`.

> [!WARNING]
> **Ausencia de `MeterItemEnInventario`**: A diferencia de `GetObj` o el comercio con NPCs que delegan la inserción en la función unificada `MeterItemEnInventario(UserIndex, MiObj)` (`InvUsuario.bas:452`), `modBanco.bas` **duplica de forma manual la lógica de búsqueda e inserción de slots** en `UserReciveObj` (`modBanco.bas:138-175`). Esto genera divergencias potenciales en el cálculo de ranuras libres y el control de inventarios expandidos por mochila.

### 4.2. Emisión de Mensajes de Protocolo hacia el Cliente

La sincronización de la bóveda bancaria genera los siguientes mensajes del protocolo binario:

| Mensaje de Protocolo | Origen en VB6 | Disparador y Efecto en Cliente |
| :--- | :--- | :--- |
| `WriteBankInit(UserIndex)` | `modBanco.bas:36` | Notifica al cliente la apertura de la interfaz gráfica del banco (`eMessages.BankInit`). |
| `WriteBankOK(UserIndex)` | `modBanco.bas:213` (`UpdateVentanaBanco`) | Confirma la finalización exitosa de una transacción de ítems (`eMessages.BankOK`). |
| `WriteChangeBankSlot(UserIndex, Slot)` | `modBanco.bas:55` (`SendBanObj`) | Actualiza un slot específico de la bóveda en el cliente enviando `ObjIndex`, `Amount`, `GrhIndex`, `ObjType`, `Name`, etc. |
| `WriteChangeInventorySlot(...)` | `InvUsuario.bas:1473` (vía `UpdateUserInv`) | Actualiza visualmente las 30 ranuras de la mochila del jugador en el cliente. |
| `WriteUpdateUserStats(UserIndex)` | `modBanco.bas:33, 104` | Sincroniza las estadísticas del jugador (vida, maná, energía, oro actual). |
| `WriteUpdateGold(UserIndex)` | `Protocol.bas:7057, 7201` | Sincroniza la billetera de oro del usuario tras depósitos o retiros de oro. |
| `WriteUpdateBankGold(UserIndex)` | `Protocol.bas:7058, 7202` | Sincroniza el balance de oro en la bóveda bancaria del usuario. |
| `WriteConsoleMsg(UserIndex, ...)` | `modBanco.bas:159, 174, 276, 292` | Informa errores de capacidad: `"No podés tener mas objetos."`, `"No tienes mas espacio en el banco!!"`, etc. |

---

## 5. Auditoría Quirúrgica de Casos de Borde, Desbordamientos y Exploits

### 5.1. Riesgos de Desincronización y Duplicación de Ítems (*Item Dupe*)

El análisis del orden operacional revela una **falla crítica de atomicidad transaccional** en `UserDejaObj` y `UserReciveObj`:

#### Vulnerabilidad en Depósito (`UserDejaObj`, líneas 288-291)
```vb
'1. Acredita en la bóveda antes de cobrar en mochila:
.BancoInvent.Object(Slot).ObjIndex = obji
.BancoInvent.Object(Slot).Amount = .BancoInvent.Object(Slot).Amount + Cantidad

'2. Descuenta de la mochila:
Call QuitarUserInvItem(UserIndex, CByte(ObjIndex), Cantidad)
```

**Mecánica del Fallo**:
1. El servidor acredita el objeto y la cantidad en el banco **en el paso 1**.
2. En el paso 2, invoca `QuitarUserInvItem(UserIndex, CByte(ObjIndex), Cantidad)`.
3. En `InvUsuario.bas:283`:
   ```vb
   If Slot < 1 Or Slot > UserList(UserIndex).CurrentInventorySlots Then Exit Sub
   ```
   Si el slot provisto resulta inconsistente con `CurrentInventorySlots` (o si se produce una excepción en `Desequipar`), `QuitarUserInvItem` aborta con `Exit Sub` sin descontar el ítem de la mochila.
4. **Resultado**: El objeto quedó creado en la bóveda y preservado en la mochila del usuario, materializando una duplicación neta (*dupe*).

#### Vulnerabilidad en Retiro (`UserReciveObj`, líneas 169-172)
```vb
'1. Acredita en el inventario del usuario:
.Invent.Object(Slot).ObjIndex = obji
.Invent.Object(Slot).Amount = .Invent.Object(Slot).Amount + Cantidad

'2. Descuenta del banco:
Call QuitarBancoInvItem(UserIndex, CByte(ObjIndex), Cantidad)
```
Idéntica asimetría: el ítem se crea en la mochila antes de comprobar o asegurar el débito de la bóveda bancaria.

> [!IMPORTANT]
> **Directiva para la Transliteración en C++20**:
> La implementación en `src/server/modBanco.hpp` debe adoptar un **patrón transaccional atómico estricto**:
> 1. Fase de Validación: Comprobar existencia, capacidades de stack, límites de ranura y consistencia de ambas colecciones.
> 2. Fase de Ejecución: Debitar primero del contenedor origen y, solo si el débito resulta estrictamente exitoso, acreditar en el contenedor destino; si alguna fase falla, realizar rollback inmediato sin alterar contadores.

### 5.2. Desbordamiento Aritmético en Depósito de Oro (`Integer Overflow`)

En `Protocol.bas:7197` (`HandleBankDepositGold`):
```vb
If Amount > 0 And Amount <= .Stats.GLD Then
    .Stats.Banco = .Stats.Banco + Amount
    .Stats.GLD = .Stats.GLD - Amount
    Call WriteChatOverHead(UserIndex, "Tenés " & .Stats.Banco & " monedas de oro en tu cuenta.", Npclist(.flags.TargetNPC).Char.CharIndex, vbWhite)
```

- Tanto `.Stats.Banco`, `.Stats.GLD` como `Amount` son enteros de 32 bits con signo (`Long` de VB6, con límite positivo 2.147.483.647).
- **Escenario de Explotación**: Un personaje que posee 1.500.000.000 de oro en el banco y deposita 1.000.000.000 de oro que lleva en su inventario provoca la operación:
  $$1.500.000.000 + 1.000.000.000 = 2.500.000.000 > 2.147.483.647$$
- En Visual Basic 6.0, esto arroja inmediatamente un `Runtime Error 6: Overflow`. En tiempo de ejecución sin captura adecuada, puede voltear el servidor o interrumpir la transacción a medio camino. En C++, la adición de enteros con signo con desbordamiento constituye Comportamiento Indefinido (*Undefined Behavior*) o provocaría que el saldo del banco caiga a valores negativos ($\approx -1.794.967.296$).
- **Mitigación**: Debe imponerse un tope explícito de balance máximo (ej. `MAX_ORO_BANCO = 2'000'000'000` o saturación aritmética con verificación preventiva de desborde `MAX_LONG - .Stats.Banco < Amount`).

### 5.3. Validación de Cantidades Negativas o Cero

- **Depósito y Retiro de Oro**: `Protocol.bas` valida preventivamente `If Amount > 0 And Amount <= .Stats.Banco` (retiro) y `If Amount > 0 And Amount <= .Stats.GLD` (depósito). Valores negativos o nulos son rechazados emitiendo el mensaje aéreo `"No tienes esa cantidad."`.
- **Depósito y Retiro de Ítems**:
  - En `UserRetiraItem`: `If Cantidad < 1 Then Exit Sub` (`modBanco.bas:102`).
  - En `UserDepositaItem`: `If Cantidad > 0 Then` (`modBanco.bas:226`) y en `UserDejaObj`: `If Cantidad < 1 Then Exit Sub` (`modBanco.bas:254`).
  - Ambas barreras previenen de manera efectiva la inyección de números negativos mediante paquetes manipulados.

### 5.4. Validación de Distancia Física y Estado del Banquero

Un hallazgo de suma relevancia es la **falta de validación espacial en los manejadores de ítems de `Protocol.bas`**:

1. **Al iniciar el banco (`HandleBankStart`, `Protocol.bas:5864`)**:
   - Valida `.flags.Muerto = 1`.
   - Valida `Distancia(Npclist(.flags.TargetNPC).Pos, .Pos) <= 3`.
   - Valida que `Npclist(.flags.TargetNPC).NPCtype == eNPCType.Banquero`.
2. **Al operar con oro (`HandleBankExtractGold` / `DepositGold`, `Protocol.bas:7044, 7191`)**:
   - Valida `Distancia <= 10` al NPC objetivo.
3. **Al depositar o retirar ítems (`HandleBankExtractItem` y `HandleBankDeposit`, `Protocol.bas:3646, 3737`)**:
   - Comprueban `.flags.Muerto = 1`.
   - Comprueban `.flags.TargetNPC < 1`.
   - Comprueban `Npclist(.flags.TargetNPC).NPCtype == eNPCType.Banquero`.
   - **¡NO VALIDAN LA DISTANCIA FÍSICA!**: Una vez abierta la bóveda, el cliente podía alejarse a cualquier distancia en el mapa (incluso a celdas lejanas) y, mientras mantuviera asignado su `TargetNPC`, continuar extrayendo y depositando ítems libremente.
   - **¡NO VALIDAN `.flags.Comerciando`!**: No se verifica que el estado de comercio bancario siga activo.

### 5.5. Restricciones de Ítems (Faccionarios, Barcos, Newbies)

En el código original de `modBanco.bas`, **no existe ningún filtro por tipo de ítem**:
- **Barcos**: Un usuario puede depositar galeones o barcas (`OBJTYPE_BARCO`) en la bóveda sin restricción.
- **Objetos Faccionarios**: Armaduras de la Armada Real o de la Legión del Caos pueden ser depositadas.
- **Ítems Newbie**: Pueden depositarse ítems con flag `Newbie = 1`. Esto permitía un exploit histórico donde un usuario novato que estaba a punto de superar el nivel 12 (umbral de novato) guardaba sus ítems de novato en la bóveda bancaria para evitar que la rutina `QuitarNewbieObj` (`InvUsuario.bas:93`) se los destruyera al subir a nivel 13 o abandonar el Newbie Dungeon.

### 5.6. Vulnerabilidad en Desplazamiento de Slots (`HandleMoveBank`, `Protocol.bas:3888`)

En el manejador del paquete para mover slots bancarios:
```vb
If .ReadBoolean() Then
    dir = 1
Else
    dir = -1
End If

Slot = .ReadByte()

If dir = 1 Then 'Mover arriba
    .BancoInvent.Object(Slot) = .BancoInvent.Object(Slot - 1)
    .BancoInvent.Object(Slot - 1).ObjIndex = TempItem.ObjIndex
    .BancoInvent.Object(Slot - 1).Amount = TempItem.Amount
Else 'mover abajo
    .BancoInvent.Object(Slot) = .BancoInvent.Object(Slot + 1)
    .BancoInvent.Object(Slot + 1).ObjIndex = TempItem.ObjIndex
    .BancoInvent.Object(Slot + 1).Amount = TempItem.Amount
End If
```

- Si un cliente malicioso envía `dir = 1` con `Slot = 1`, accede a `.BancoInvent.Object(0)`.
- Si envía `dir = -1` con `Slot = 40`, accede a `.BancoInvent.Object(41)`.
- Como el array está declarado `(1 To MAX_BANCOINVENTORY_SLOTS)`, esto produce un desborde de índice (`Subscript out of range`), provocando la caída del servidor si no está atrapado por el despachador de errores.

---

## 6. Hoja de Ruta para la Transliteración a C++20

1. **Ubicación Canónica**: `src/server/modBanco.hpp` (y `src/server/modBanco.cpp`).
2. **Dependencias**:
   - `src/server/Declares.hpp` (definiciones de `BancoInventario`, `UserOBJ`, `MAX_BANCOINVENTORY_SLOTS`).
   - `src/server/InvUsuario.hpp` (para débito atómico de inventario de usuario).
   - `src/server/Protocol.hpp` (mensajes de sincronización de bóveda y consola).
3. **Refactorización Transaccional Segura**:
   - Reemplazar el orden de inserción/débito por transacciones que validan precondiciones completas antes de mutar cualquier slot.
   - Proteger los accesos por índice de ranura contra valores fuera del rango [1, 40].
   - Proveer un método unificado para depósito y extracción de oro con chequeo estricto contra desbordes aritméticos (`std::numeric_limits<int32_t>::max()`).
4. **Relación con Documentación del Proyecto**:
   - Directrices generales de arquitectura y codificación: [`../CONVENTIONS.md`](../CONVENTIONS.md).
   - Mapeo global de migración y capas: [`../implementation/00-port-plan.md`](../implementation/00-port-plan.md).
   - Especificaciones de red y paquetes bancarios: [`../implementation/16-protocol-breakdown.md`](../implementation/16-protocol-breakdown.md).
   - Lógica de inventario de usuario y objetos en mapa: [`../implementation/19-invusuario.md`](../implementation/19-invusuario.md).
