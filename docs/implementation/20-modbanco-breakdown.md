---
area: subsistema-inventario-objetos
module_id: 20
source_files:
  - legacy/server/Codigo/modBanco.bas
  - docs/audit/12c-modbanco-detalle.md
  - docs/CONVENTIONS.md
  - docs/implementation/KNOWN-LEGACY-BUGS.md
tags: [modbanco, banco, boveda, inventario, objetos, sparse-grid, exploit, transacciones, cpp20, breakdown, plan]
last_updated: 2026-09-13
---

# Plan de Desglose Modular: Módulo de Bóveda Bancaria `modBanco.bas` (Capa 6, Módulo #20)

Este documento define la planificación técnica detallada, los guardrails arquitectónicos y la estrategia de implementación progresiva en C++20 para el módulo [`legacy/server/Codigo/modBanco.bas`](../../legacy/server/Codigo/modBanco.bas) (300 líneas en Visual Basic 6).

Conforme a las directivas de porting institucional de [`docs/CONVENTIONS.md`](../CONVENTIONS.md), los hallazgos de la auditoría técnica detallada ([`docs/audit/12c-modbanco-detalle.md`](../audit/12c-modbanco-detalle.md)) y los registros estratégicos del Master Bug Ledger ([`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md)), este desglose estructura la transliteración en **cuatro fases lógicas y secuenciales**.

---

## 1. Resumen Ejecutivo y Alcance

`modBanco.bas` constituye el administrador central de la bóveda bancaria de los jugadores en Argentum Online:
- **Apertura e Inicialización de Sesión Bancaria**: Sincronización completa de los 40 slots bancarios, envío de estadísticas, despacho de apertura de diálogo y activación del flag de comercio (`IniciarDeposito`).
- **Transferencia de Objetos**: Depósito de ítems desde la mochila hacia la bóveda (`UserDepositaItem`, `UserDejaObj`) y extracción desde la bóveda hacia la mochila (`UserRetiraItem`, `UserReciveObj`).
- **Modelo de Grilla Fija Dispersa (*Sparse Grid*)**: Mutación de slots unitarios en base 1 (`1..MAX_BANCOINVENTORY_SLOTS`) preservando celdas vacías (`ObjIndex = 0`, `Amount = 0`) sin corrimiento ni compactación (`QuitarBancoInvItem`).
- **Sincronización de Red**: Envío masivo o individual de ranuras bancarias (`SendBanObj`, `UpdateBanUserInv`) y confirmación de operación completada (`UpdateVentanaBanco`).
- **Inspección Administrativa de Game Masters**: Listado de contenido de bóveda para usuarios conectados (`SendUserBovedaTxt`) y lectura de archivos de personaje `.chr` para usuarios desconectados (`SendUserBovedaTxtFromChar`).

### Delimitación de Responsabilidades:
1. **Dominio de `modBanco` (Módulo #20)**:
   - Los 11 procedimientos canónicos declarados en `modBanco.bas`.
   - Modificación directa de la colección `UserList(UserIndex).BancoInvent` (`Object(1..40)` y `NroItems`).
   - Invocación de `InvUsuario::QuitarUserInvItem` y `InvUsuario::UpdateUserInv`.
   - Replicación estricta y literal de los bugs históricos #31 (acreditación previa al débito) y #32 (ausencia de restricciones de almacenamiento).
2. **Dominio de `Protocol` (Módulo #16 - Capa 4)**:
   - Manejo del oro bancario (`Stats.Banco`): los comandos `HandleBankExtractGold` y `HandleBankDepositGold` pertenecen exclusivamente a `Protocol.bas`. `modBanco` no contiene lógica de saldo monetario ni despacha tramas de oro.
   - Reordenamiento visual de slots bancarios (`HandleMoveBank`): pertenece a `Protocol.bas`.
3. **Dominio de `InvUsuario` (Módulo #19 - Capa 6)**:
   - Mutaciones en el inventario del usuario (`Invent.Object`) y desequipamiento automático al depositar stacks completos en uso.

### Estado de Cierre Proyectado:
Al completar sus cuatro fases y suite de pruebas, este módulo alcanzará el estado de **`Completado (Aislado / Cableado Pendiente)`** conforme a [`docs/CONVENTIONS.md`](../CONVENTIONS.md#definición-formal-de-los-tres-estados-de-cierre-de-módulo-module-closure-states).

---

## 2. Decisiones Estratégicas y Guardrails de Arquitectura en C++20

### 2.1. Prohibición de Rollback y Refactorización Transaccional (Bug #31)

> [!CAUTION]
> **DIRECTIVA VINCULANTE: PRESERVACIÓN DE ASIMETRÍA HISTÓRICA**
> Queda terminantemente prohibido alterar el orden operacional de `UserDejaObj` y `UserReciveObj`.
> - En `UserDejaObj`, se debe replicar la secuencia original de VB6: **acreditar en la bóveda bancaria antes de debitar en la mochila** mediante `QuitarUserInvItem`.
> - En `UserReciveObj`, se debe replicar la secuencia original: **acreditar en la mochila del usuario antes de debitar en la bóveda** mediante `QuitarBancoInvItem`.
> - Queda prohibido implementar esquemas de commit de dos fases, comprobaciones preventivas no presentes en legacy o reversión automática (*rollback*) de transacciones. La vulnerabilidad de duplicación neta (*item dupe*) documentada en la Entrada #31 del bug ledger debe ser reproducida fielmente.

### 2.2. Preservación Léxica Estricta y Tipado Seguro

1. **Preservación de Nombres Legacy**:
   - Se mantiene PascalCase idéntico a VB6 para todas las funciones públicas: `IniciarDeposito`, `SendBanObj`, `UpdateBanUserInv`, `UserRetiraItem`, `UserReciveObj`, `QuitarBancoInvItem`, `UpdateVentanaBanco`, `UserDepositaItem`, `UserDejaObj`, `SendUserBovedaTxt`, `SendUserBovedaTxtFromChar`.
   - Se preserva de forma obligatoria la **errata histórica de tipeo** en `UserReciveObj` (sin "i" intermedia).
2. **Tipado de Índices y Cantidades**:
   - `UserIndex`: `int16_t` (correspondiente a `Integer` de VB6).
   - `Slot`: `uint8_t` (correspondiente a `Byte` de VB6, rango 1..40).
   - `Cantidad` y `Amount`: `int16_t` (correspondiente a `Integer` de VB6, acotado por `MAX_INVENTORY_OBJS = 10000`).
   - `ObjIndex`: `int16_t`.

### 2.3. Desacoplamiento de E/S de Archivo mediante Hook Funcional

La subrutina `SendUserBovedaTxtFromChar` lee sincrónicamente el archivo `.chr` del personaje desde disco utilizando `GetVar`. Para preservar la pureza del bucle principal y facilitar las pruebas unitarias:
- Se implementará un hook de abstracción `SetCharReaderHook` (del tipo `std::function<std::optional<std::string>(const std::string& char_name, const std::string& section, const std::string& key)>`).
- Por defecto, delegará en `FileIO::GetVar`. En el entorno de test doctest, permitirá simular personajes offline en memoria sin tocar el sistema de archivos real.

---

## 3. Desglose en Cuatro Fases Secuenciales

```mermaid
graph TD
    subgraph Fase 1: G1 - Apertura y Sincronizacion
        F1A["IniciarDeposito"] --> F1B["SendBanObj"]
        F1B --> F1C["UpdateBanUserInv"]
        F1C --> F1D["UpdateVentanaBanco"]
    end

    subgraph Fase 2: G2 - Transacciones de Objetos
        F2A["UserRetiraItem"] --> F2B["UserReciveObj (Bug #31)"]
        F2B --> F2C["QuitarBancoInvItem"]
        F2D["UserDepositaItem"] --> F2E["UserDejaObj (Bugs #31 y #32)"]
    end

    subgraph Fase 3: G3 - Inspeccion GM y Hooks de IO
        F3A["SendUserBovedaTxt"]
        F3B["SendUserBovedaTxtFromChar"]
        F3C["Hook de Abstraccion FileIO"]
    end

    subgraph Fase 4: G4 - Verificacion Doctest
        F4A["test_modbanco.cpp (100% Cobertura)"]
        F4B["Validacion Sparse Grid"]
        F4C["Reproduccion Bugs #31 y #32"]
    end

    Fase 1 --> Fase 2
    Fase 2 --> Fase 3
    Fase 3 --> Fase 4
```

---

### Fase 1 (G1 — Apertura, GUI y Sincronización)

Esta fase abarca la inicialización de la sesión bancaria, la asignación unitaria de slots y la sincronización visual bidireccional con el cliente.

#### Procedimientos a Implementar:

1. **`IniciarDeposito(int16_t user_index)`** (`legacy/server/Codigo/modBanco.bas:25-48`):
   - Invoca `UpdateBanUserInv(true, user_index, 0)` para sincronizar los 40 slots de bóveda al cliente.
   - Invoca `Protocol::WriteUpdateUserStats(user_index)` para refrescar estadísticas y oro en pantalla.
   - Invoca `Protocol::WriteBankInit(user_index)` enviando el opcode de apertura de la interfaz gráfica del banco.
   - Establece `UserList[user_index].flags.Comerciando = true`.

2. **`SendBanObj(int16_t user_index, uint8_t slot, const UserOBJ& object)`** (`legacy/server/Codigo/modBanco.bas:50-58`):
   - Asigna el objeto en la estructura de bóveda del jugador:
     `UserList[user_index].BancoInvent.Object[slot] = object;`
   - Despacha hacia el cliente la trama unitaria:
     `Protocol::WriteChangeBankSlot(user_index, slot);`

3. **`UpdateBanUserInv(bool update_all, int16_t user_index, uint8_t slot)`** (`legacy/server/Codigo/modBanco.bas:60-91`):
   - Si `update_all == false`:
     - Si `UserList[user_index].BancoInvent.Object[slot].ObjIndex > 0`, convoca a `SendBanObj` con dicho objeto.
     - En caso contrario, convoca a `SendBanObj` con una estructura `UserOBJ` vacía (`ObjIndex = 0, Amount = 0, Equipped = 0`).
   - Si `update_all == true`:
     - Itera con un bucle `for` del slot 1 al 40 (`MAX_BANCOINVENTORY_SLOTS`), aplicando la lógica anterior a cada ranura.

4. **`UpdateVentanaBanco(int16_t user_index)`** (`legacy/server/Codigo/modBanco.bas:206-214`):
   - Emite la confirmación de operación bancaria hacia el cliente:
     `Protocol::WriteBankOK(user_index);`

---

### Fase 2 (G2 — Retiro y Depósito de Objetos)

Esta fase implementa la lógica transaccional de movimiento de objetos entre mochila y bóveda, el apilamiento de cantidades hasta 10.000 unidades y el modelo de grilla dispersa (*sparse grid*).

#### Procedimientos a Implementar:

1. **`UserRetiraItem(int16_t user_index, int16_t i, int16_t cantidad)`** (`legacy/server/Codigo/modBanco.bas:93-122`):
   - Si `cantidad < 1`, interrumpe inmediatamente (`return`).
   - Invoca `Protocol::WriteUpdateUserStats(user_index)`.
   - Si `UserList[user_index].BancoInvent.Object[i].Amount > 0`:
     - Si `cantidad > UserList[user_index].BancoInvent.Object[i].Amount`, acota `cantidad = UserList[user_index].BancoInvent.Object[i].Amount`.
     - Invoca `UserReciveObj(user_index, i, cantidad)`.
     - Invoca `InvUsuario::UpdateUserInv(true, user_index, 0)`.
     - Invoca `UpdateBanUserInv(true, user_index, 0)`.
   - Invoca `UpdateVentanaBanco(user_index)`.

2. **`UserReciveObj(int16_t user_index, int16_t obj_index, int16_t cantidad)`** (`legacy/server/Codigo/modBanco.bas:124-178`):
   - Si `UserList[user_index].BancoInvent.Object[obj_index].Amount <= 0`, retorna.
   - Obtiene el identificador `obji = UserList[user_index].BancoInvent.Object[obj_index].ObjIndex`.
   - **Paso A: Búsqueda de Ranura Apilable en Mochila**:
     - Recorre desde `slot = 1` hasta `UserList[user_index].CurrentInventorySlots`.
     - Si `Invent.Object[slot].ObjIndex == obji` y `Invent.Object[slot].Amount + cantidad <= MAX_INVENTORY_OBJS` (10.000), selecciona dicho `slot` y finaliza la búsqueda.
   - **Paso B: Búsqueda de Ranura Libre si no hubo coincidencia apilable**:
     - Si `slot > CurrentInventorySlots`, reinicia `slot = 1` y busca la primera celda con `Invent.Object[slot].ObjIndex == 0`.
     - Si no halla ranura libre dentro de `CurrentInventorySlots`, emite `WriteConsoleMsg(user_index, "No podés tener mas objetos.", FONTTYPE_INFO)` y retorna.
     - Al hallar ranura libre, incrementa `Invent.NroItems++`.
   - **Paso C: Acreditación Previa al Débito (Bug #31)**:
     - Si `Invent.Object[slot].Amount + cantidad <= MAX_INVENTORY_OBJS`:
       - Asigna `Invent.Object[slot].ObjIndex = obji;`
       - Suma `Invent.Object[slot].Amount += cantidad;`
       - Ejecuta el débito en bóveda: `QuitarBancoInvItem(user_index, static_cast<uint8_t>(obj_index), cantidad);`
     - En caso contrario, emite `"No podés tener mas objetos."`.

3. **`QuitarBancoInvItem(int16_t user_index, uint8_t slot, int16_t cantidad)`** (`legacy/server/Codigo/modBanco.bas:180-204`):
   - Resta `cantidad` de `BancoInvent.Object[slot].Amount`.
   - Si el saldo resultante es `<= 0`:
     - Decrementa `BancoInvent.NroItems--`.
     - Blanquea la ranura en la grilla dispersa: `ObjIndex = 0`, `Amount = 0` (**sin compactar**).

4. **`UserDepositaItem(int16_t user_index, int16_t item, int16_t cantidad)`** (`legacy/server/Codigo/modBanco.bas:216-245`):
   - Si `Invent.Object[item].Amount > 0` y `cantidad > 0`:
     - Si `cantidad > Invent.Object[item].Amount`, acota `cantidad = Invent.Object[item].Amount`.
     - Invoca `UserDejaObj(user_index, item, cantidad)`.
     - Invoca `InvUsuario::UpdateUserInv(true, user_index, 0)`.
     - Invoca `UpdateBanUserInv(true, user_index, 0)`.
   - Invoca `UpdateVentanaBanco(user_index)`.

5. **`UserDejaObj(int16_t user_index, int16_t obj_index, int16_t cantidad)`** (`legacy/server/Codigo/modBanco.bas:247-299`):
   - Si `cantidad < 1`, retorna.
   - Obtiene `obji = Invent.Object[obj_index].ObjIndex` (**sin validar restricciones de tipo, Bug #32**).
   - **Paso A: Búsqueda de Ranura Apilable en Bóveda**:
     - Recorre desde `slot = 1` hasta `MAX_BANCOINVENTORY_SLOTS` (40).
     - Si `BancoInvent.Object[slot].ObjIndex == obji` y `BancoInvent.Object[slot].Amount + cantidad <= MAX_INVENTORY_OBJS`, selecciona dicho `slot` y concluye la búsqueda.
   - **Paso B: Búsqueda de Ranura Libre en Bóveda**:
     - Si `slot > MAX_BANCOINVENTORY_SLOTS`, reinicia `slot = 1` y busca la primera celda con `BancoInvent.Object[slot].ObjIndex == 0`.
     - Si supera 40, emite `WriteConsoleMsg(user_index, "No tienes mas espacio en el banco!!", FONTTYPE_INFO)` y retorna.
     - Al hallar ranura libre, incrementa `BancoInvent.NroItems++`.
   - **Paso C: Acreditación Previa al Débito (Bug #31)**:
     - Si `slot <= MAX_BANCOINVENTORY_SLOTS`:
       - Si `BancoInvent.Object[slot].Amount + cantidad <= MAX_INVENTORY_OBJS`:
         - Asigna `BancoInvent.Object[slot].ObjIndex = obji;`
         - Suma `BancoInvent.Object[slot].Amount += cantidad;`
         - Ejecuta el débito en mochila: `InvUsuario::QuitarUserInvItem(user_index, static_cast<uint8_t>(obj_index), cantidad);`
       - En caso contrario, emite `"El banco no puede cargar tantos objetos."`.

---

### Fase 3 (G3 — Inspección Administrativa GM y Desacoplamiento de E/S)

Esta fase comprende las herramientas de consulta y auditoría de bóvedas utilizadas por los Game Masters.

#### Procedimientos a Implementar:

1. **`SendUserBovedaTxt(int16_t send_index, int16_t user_index)`** (`legacy/server/Codigo/modBanco.bas:301-321`):
   - Envía por consola al GM `send_index` el nombre del usuario inspeccionado y la cantidad de ítems: `"Tiene " & BancoInvent.NroItems & " objetos."`.
   - Itera de `j = 1` a `MAX_BANCOINVENTORY_SLOTS` (40):
     - Si `BancoInvent.Object[j].ObjIndex > 0`, despacha:
       `"Objeto " & j & " " & ObjData[ObjIndex].name & " Cantidad:" & Amount`.

2. **`SendUserBovedaTxtFromChar(int16_t send_index, const std::string& char_name)`** (`legacy/server/Codigo/modBanco.bas:323-350`):
   - Resuelve la ruta del archivo de personaje (`CharPath + char_name + ".chr"`).
   - Comprueba existencia del archivo. Si no existe, notifica `"Usuario inexistente: " + char_name`.
   - Si existe:
     - Consulta `CantidadItems` en la sección `[BancoInventory]`.
     - Itera `j = 1` a 40 leyendo la clave `Obj" & j` (formato `"ObjIndex-Amount"`).
     - Parsea índice y cantidad, y si `ObjIndex > 0`, despacha la línea por consola al GM.
3. **Hook de Desacoplamiento de E/S**:
   - `SetCharReaderHook(CharReaderHook hook)`: Permite interceptar las consultas de archivos `.chr` en tests unitarios para proveer perfiles sintéticos en memoria sin tocar el disco.

---

### Fase 4 (G4 — Suite de Pruebas Doctest e Integración)

Diseño de la suite exhaustiva de pruebas unitarias en `tests/test_modbanco.cpp`. La suite debe alcanzar **100% de cobertura** sobre los 11 procedimientos del módulo y validar las anomalías históricas.

#### Casos de Prueba Requeridos:

1. **`TEST_CASE("modBanco: IniciarDeposito y sincronización visual")`**:
   - Apertura de diálogo bancario, seteo de `flags.Comerciando = true`, despacho de `WriteBankInit` y refresco total de 40 ranuras mediante `UpdateBanUserInv`.
2. **`TEST_CASE("modBanco: Depósito atómico en grilla dispersa")`**:
   - Depósito de ítem en ranura vacía, verificación de asignación fija en slot 1, incremento de `BancoInvent.NroItems` y débito exitoso en mochila del usuario.
   - Depósito de ítem en ranura con stack existente (acumulación hasta 10.000 unidades).
   - Rechazo por bóveda llena (40 slots ocupados) con emisión de `"No tienes mas espacio en el banco!!"`.
3. **`TEST_CASE("modBanco: Retiro de objetos y preservación de grilla dispersa")`**:
   - Retiro parcial de un stack bancario (saldo remanente correcto).
   - Retiro total de un stack bancario: comprobación estricta de que el slot queda en `ObjIndex = 0, Amount = 0`, decremento de `NroItems` y **verificación de que los slots posteriores no se desplazan ni compactan**.
   - Rechazo por mochila llena con emisión de `"No podés tener mas objetos."`.
4. **`TEST_CASE("modBanco: Reproducción del Bug #31 (Acreditación previa al débito)")`**:
   - Simulación de llamada a `UserDejaObj` donde el débito en mochila falla o es omitido: validación de que el objeto quedó acreditado en la bóveda sin descontar de la mochila (duplicación histórica confirmada).
5. **`TEST_CASE("modBanco: Reproducción del Bug #32 (Almacenamiento de barcos y newbies)")`**:
   - Depósito de barco (`OBJTYPE_BARCO`) y de ítem de novato (`Newbie = 1`): verificación de que el depósito se concreta sin bloqueos restrictivos.
6. **`TEST_CASE("modBanco: Inspección GM online y offline")`**:
   - Ejecución de `SendUserBovedaTxt` validando los mensajes emitidos al GM.
   - Ejecución de `SendUserBovedaTxtFromChar` mediante hook en memoria simulando un archivo `.chr`.

---

## 4. Matriz de Trazabilidad y Dependencias

| Procedimiento C++ | Procedimiento VB6 Original | Dependencias Directas | Hooks Requeridos |
| :--- | :--- | :--- | :--- |
| `IniciarDeposito` | `modBanco.bas:25` | `Protocol::WriteBankInit`, `Protocol::WriteUpdateUserStats` | Ninguno |
| `SendBanObj` | `modBanco.bas:50` | `Protocol::WriteChangeBankSlot` | Ninguno |
| `UpdateBanUserInv` | `modBanco.bas:60` | `SendBanObj` | Ninguno |
| `UserRetiraItem` | `modBanco.bas:93` | `UserReciveObj`, `InvUsuario::UpdateUserInv`, `UpdateBanUserInv` | Ninguno |
| `UserReciveObj` | `modBanco.bas:124` | `QuitarBancoInvItem`, `Protocol::WriteConsoleMsg` | Ninguno |
| `QuitarBancoInvItem`| `modBanco.bas:180` | Modificación de `BancoInvent` en memoria | Ninguno |
| `UpdateVentanaBanco`| `modBanco.bas:206` | `Protocol::WriteBankOK` | Ninguno |
| `UserDepositaItem` | `modBanco.bas:216` | `UserDejaObj`, `InvUsuario::UpdateUserInv`, `UpdateBanUserInv` | Ninguno |
| `UserDejaObj` | `modBanco.bas:247` | `InvUsuario::QuitarUserInvItem`, `Protocol::WriteConsoleMsg` | Ninguno |
| `SendUserBovedaTxt`| `modBanco.bas:301` | `Protocol::WriteConsoleMsg`, `ObjData` | Ninguno |
| `SendUserBovedaTxtFromChar` | `modBanco.bas:323` | `Protocol::WriteConsoleMsg`, `ObjData`, `FileIO::GetVar` | `CharReaderHook` |
