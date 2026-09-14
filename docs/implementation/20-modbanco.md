---
area: subsistema-inventario-objetos
module_id: 20
source_files:
  - src/server/modBanco.hpp
  - src/server/modBanco.cpp
  - legacy/server/Codigo/modBanco.bas
  - docs/audit/12c-modbanco-detalle.md
  - docs/implementation/20-modbanco-breakdown.md
  - docs/implementation/KNOWN-LEGACY-BUGS.md
  - docs/CONVENTIONS.md
tags: [modbanco, banco, boveda, inventario, objetos, sparse-grid, exploit, transacciones, cpp20, specification]
last_updated: 2026-09-14
---

# Módulo de Bóveda Bancaria: `modBanco` (Capa 6, Módulo #20)

## Resumen del Módulo

Este documento formaliza la arquitectura, especificación técnica definitiva, decisiones de bajo nivel, replicación de exploits históricos y cobertura de pruebas del módulo de Capa 6 **modBanco** (`src/server/modBanco.hpp` y `src/server/modBanco.cpp`), transliterado a partir del módulo original [`legacy/server/Codigo/modBanco.bas`](../../legacy/server/Codigo/modBanco.bas) (300 líneas en Visual Basic 6.0).

El módulo centraliza la administración y el resguardo de objetos de los personajes jugadores en la bóveda bancaria del servidor:
1. **Apertura, GUI y Sincronización (G1)**: Coordinación del inicio de la sesión con el NPC Banquero (`IniciarDeposito`), despacho unitario de ranuras (`SendBanObj`), sincronización masiva o unitaria de la grilla bancaria (`UpdateBanUserInv`) y confirmación de transacción completada (`UpdateVentanaBanco`).
2. **Retiro y Depósito de Objetos (G2)**: Puntos de entrada y orquestadores de transferencia de ítems entre la mochila (`UserList.Invent`) y la bóveda (`UserList.BancoInvent`) (`UserRetiraItem`, `UserReciveObj`, `UserDepositaItem`, `UserDejaObj`), respetando el apilamiento de hasta 10.000 unidades y la deducción en la grilla fija dispersa sin corrimientos (`QuitarBancoInvItem`).
3. **Inspección Administrativa GM y Desacoplamiento de E/S (G3)**: Consultas textuales del contenido de la bóveda para Game Masters sobre usuarios conectados (`SendUserBovedaTxt`) y sobre usuarios desconectados a través de archivos de personaje `.chr` (`SendUserBovedaTxtFromChar`), incorporando un hook funcional de abstracción de E/S (`SetCharReaderHook`).

---

### Estado de Cierre Formal: Completado (Autónomo)

Conforme a la taxonomía definida en [`docs/CONVENTIONS.md`](../CONVENTIONS.md) (Lista de Chequeo de Finalización de Módulos), este módulo se clasifica en estado **Completado (Autónomo)**:
- **Código C++ Cerrado**: La implementación de los 11 procedimientos en `src/server/modBanco.hpp` y `src/server/modBanco.cpp` está 100% finalizada, compilada en `server_core` y verificada con 21 casos de prueba y 489 aserciones doctest.
- **Autonomía Operativa**: Todas sus dependencias directas (`Declares.hpp` en Capa 0, `Protocol.hpp` en Capa 4, `InvUsuario.hpp` en Capa 6 y `FileIO.hpp` en Capa 3) pertenecen a capas iguales o inferiores y ya se encuentran plenamente operativas en C++20.
- **Sin Hooks Pendientes en Runtime**: A diferencia de módulos que requieren inyecciones dinámicas desde Capas 7 o 9 (como `InvUsuario` respecto de `Modulo_UsUaRiOs`), `modBanco` resuelve íntegramente sus responsabilidades dentro de su propio ámbito y el de sus dependencias base.

---

## Decisiones Estratégicas de Arquitectura y Paridad en C++20

### 1. Modelo de Bóveda como Grilla Fija y Dispersa (*Sparse Grid*)

- **Diagnóstico en VB6**: En `UserList(UserIndex).BancoInvent.Object(1 To MAX_BANCOINVENTORY_SLOTS)`, la bóveda modela exactamente 40 ranuras independientes. Al retirar un objeto o vaciar una ranura, el código legacy en `QuitarBancoInvItem` (`L197-L201`) únicamente pone `ObjIndex = 0` y `Amount = 0`, y decrementa `BancoInvent.NroItems--`. **Bajo ninguna circunstancia desplaza o compacta los elementos subsiguientes hacia la izquierda.**
- **Decisión de Porting**: En `modBanco::QuitarBancoInvItem`, cuando `banco_slot.Amount <= 0`, la celda se blanquea en su índice puntual y se decrementa el contador. Está **estrictamente prohibido** utilizar corrimientos a la izquierda (`std::vector::erase` o compactaciones), preservando la integridad de los índices del 1 al 40. Esto concuerda con el comando de red `HandleMoveBank` en `Protocol.bas`, que permite al cliente reordenar e intercambiar slots contiguos de forma explícita.

### 2. Replicación del Bug #31: Acreditación Previa al Débito (*Item Dupe*)

- **Diagnóstico en VB6**: En `modBanco.bas:288-291` (`UserDejaObj`) y `modBanco.bas:169-172` (`UserReciveObj`), el servidor invierte el orden canónico de una transacción atómica:
  1. Al depositar, primero acredita el ítem en la bóveda (`BancoInvent.Object[slot].Amount += cantidad`) y recién después convoca a `QuitarUserInvItem` en la mochila. Si `QuitarUserInvItem` aborta silenciosamente (por slot fuera de rango o excepción al desequipar), el ítem ya quedó creado en la bóveda sin descontarse del usuario.
  2. Al retirar, primero acredita en la mochila (`Invent.Object[slot].Amount += cantidad`) y recién después convoca a `QuitarBancoInvItem`.
- **Decisión de Porting**: En estricto cumplimiento de la directiva vinculante y la política institucional de paridad de [`docs/CONVENTIONS.md`](../CONVENTIONS.md), **se prohibió agregar rollback o reordenar la secuencia**. Se replica de forma idéntica la acreditación en destino previa al débito en origen, documentada en el Master Bug Ledger ([`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md#entrada-31--modbanco-acreditación-previa-al-débito-en-userdejaobj-y-userreciveobj), Entrada #31).

### 3. Replicación del Bug #32: Ausencia de Restricciones de Almacenamiento

- **Diagnóstico en VB6**: En `modBanco.bas:216-245` (`UserDepositaItem`) y `L247-L299` (`UserDejaObj`), el servidor no valida las banderas ni la categoría de los ítems en `ObjDataList`: permite almacenar barcos (`OBJTYPE_BARCO`), armaduras faccionarias (Armada Real/Caos) e ítems marcados como de novato (`Newbie = 1`). Esto permitía a los personajes novatos resguardar sus pertenencias en la bóveda antes de subir al nivel 13 para evadir su destrucción automática en `QuitarNewbieObj`.
- **Decisión de Porting**: Se preserva la ausencia total de restricciones de tipo de objeto al depositar en la bóveda, documentada en el Master Bug Ledger ([`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md#entrada-32--modbanco-ausencia-de-restricciones-de-almacenamiento-para-barcos-faccionarios-y-novatos), Entrada #32).

### 4. Desacoplamiento de E/S mediante `CharReaderHook`

- **Diagnóstico en VB6**: `SendUserBovedaTxtFromChar` (`L323-L350`) ejecuta lecturas síncronas de disco mediante `GetVar` sobre el archivo `CharPath & charName & ".chr"`. En pruebas unitarias automatizadas, el acceso a disco físico introduce fragilidad, lentitud y dependencia de rutas externas.
- **Decisión de Porting**: Se introdujo el hook funcional `CharReaderHook` (`SetCharReaderHook`). En tiempo de ejecución estándar, delega en `FileIO::GetVar` y `std::filesystem::exists`. En el entorno de pruebas doctest, permite inyectar perfiles `.chr` sintéticos en memoria sin tocar el disco.

### 5. Delimitación de Fronteras Respecto de `Protocol.bas`

- **Diagnóstico en VB6**: Históricamente, el manejo del saldo de oro en banco (`UserList(UserIndex).Stats.Banco`) **no reside en `modBanco.bas`**, sino que fue implementado directamente dentro de `Protocol.bas` en los manejadores de paquetes `HandleBankExtractGold` y `HandleBankDepositGold`. De igual manera, la permutación y desplazamiento de slots (`HandleMoveBank`) pertenece a `Protocol.bas`.
- **Decisión de Porting**: `modBanco` delimita estrictamente su alcance a la administración de objetos en bóveda. Las operaciones sobre saldo monetario y reordenamiento manual de slots se mantienen formalmente diferidas a la capa de protocolo.

### 6. Preservación Léxica y Semántica de Parámetros

- **Preservación de Nombres**: Se conserva PascalCase idéntico a VB6 en las 11 funciones públicas, incluyendo la errata histórica de tipeo `UserReciveObj`.
- **Semántica de `obj_index`**: En `UserReciveObj` y `UserDejaObj`, el parámetro `obj_index` representa el **número de ranura/slot de origen** (1..40 en bóveda, 1..`CurrentInventorySlots` en mochila), no el tipo de ítem.

---

## Catálogo Canónico de Procedimientos Implementados

| Procedimiento C++ | Firma Completa | Grupo | Estado |
| :--- | :--- | :---: | :---: |
| `IniciarDeposito` | `void IniciarDeposito(std::int16_t user_index)` | G1 | ✅ Implementado / Testeado |
| `SendBanObj` | `void SendBanObj(std::int16_t user_index, std::uint8_t slot, const UserOBJ& object)` | G1 | ✅ Implementado / Testeado |
| `UpdateBanUserInv` | `void UpdateBanUserInv(bool update_all, std::int16_t user_index, std::uint8_t slot)` | G1 | ✅ Implementado / Testeado |
| `UpdateVentanaBanco` | `void UpdateVentanaBanco(std::int16_t user_index)` | G1 | ✅ Implementado / Testeado |
| `UserRetiraItem` | `void UserRetiraItem(std::int16_t user_index, std::int16_t i, std::int16_t cantidad)` | G2 | ✅ Implementado / Testeado |
| `UserReciveObj` | `void UserReciveObj(std::int16_t user_index, std::int16_t obj_index, std::int16_t cantidad)` | G2 | ✅ Implementado / Testeado |
| `QuitarBancoInvItem` | `void QuitarBancoInvItem(std::int16_t user_index, std::uint8_t slot, std::int16_t cantidad)` | G2 | ✅ Implementado / Testeado |
| `UserDepositaItem` | `void UserDepositaItem(std::int16_t user_index, std::int16_t item, std::int16_t cantidad)` | G2 | ✅ Implementado / Testeado |
| `UserDejaObj` | `void UserDejaObj(std::int16_t user_index, std::int16_t obj_index, std::int16_t cantidad)` | G2 | ✅ Implementado / Testeado |
| `SendUserBovedaTxt` | `void SendUserBovedaTxt(std::int16_t send_index, std::int16_t user_index)` | G3 | ✅ Implementado / Testeado |
| `SendUserBovedaTxtFromChar` | `void SendUserBovedaTxtFromChar(std::int16_t send_index, const std::string& char_name)` | G3 | ✅ Implementado / Testeado |

---

## Cobertura de Pruebas Unitarias (`tests/test_modbanco.cpp`)

La suite doctest de `modBanco` alcanza **100% de cobertura** sobre los 11 procedimientos del módulo, estructurada en tres subsuites funcionales con un total de **21 casos de prueba y 489 aserciones**:

```
===============================================================================
[doctest] test cases:  21 |  21 passed | 0 failed | 187 skipped
[doctest] assertions: 489 | 489 passed | 0 failed |
[doctest] Status: SUCCESS!
```

### Detalle de Suites de Prueba:

1. **`TEST_SUITE("modBanco - G1")` (7 casos)**:
   - `IniciarDeposito`: Verifica activación de `flags.Comerciando = true` y ráfaga de 40 paquetes `ChangeBankSlot`, `UpdateUserStats` y `BankInit`.
   - `SendBanObj`: Asignación directa en `BancoInvent.Object[slot]` y emisión de `ChangeBankSlot`.
   - `SendBanObj`: Manejo defensivo ante índices de usuario o slots bancarios fuera de rango (0, >40, >MaxUsers).
   - `UpdateBanUserInv` (unitario): Despacho de un único slot específico sin tocar el resto.
   - `UpdateBanUserInv` (masivo): Despacho iterativo de los 40 slots del banco (vacíos y poblados).
   - `UpdateVentanaBanco`: Emisión del opcode de confirmación `BankOK`.
   - Manejo defensivo ante usuarios inválidos en `UpdateVentanaBanco` e `IniciarDeposito`.

2. **`TEST_SUITE("modBanco - G2")` (10 casos)**:
   - `UserRetiraItem`: Retiro exitoso con apilamiento en slot existente de mochila.
   - `UserRetiraItem`: Retiro exitoso ocupando nueva ranura libre en mochila.
   - `UserRetiraItem`: Rechazo por mochila llena emitiendo `"No podés tener mas objetos."` sin alterar inventarios.
   - `UserRetiraItem`: Clamp de cantidad solicitada si supera el saldo existente en bóveda.
   - `UserDepositaItem`: Depósito exitoso con apilamiento en slot preexistente de bóveda.
   - `UserDepositaItem`: Depósito exitoso ocupando nueva ranura libre en bóveda (1..40).
   - `UserDepositaItem`: Rechazo por bóveda llena (40 slots ocupados) emitiendo `"No tienes mas espacio en el banco!!"`.
   - `QuitarBancoInvItem`: Verificación estricta del modelo de grilla dispersa fija (vaciar slot 2 blanquea dicho slot dejando slots 1 y 3 intactos sin corrimiento).
   - **Reproducción del Bug #31**: Inyección de fallo en el débito de mochila comprobando que la bóveda retiene las unidades acreditadas (*item dupe* confirmado).
   - **Reproducción del Bug #32**: Depósito exitoso de barcos (`otBarcos`), armaduras de la Armada Real e ítems de novato (`Newbie = 1`) sin bloqueos restrictivos.

3. **`TEST_SUITE("modBanco - G3")` (4 casos)**:
   - `SendUserBovedaTxt`: Inspección de personaje conectado online, reportando nombre, cantidad de ítems y únicamente los slots ocupados con descripciones de `ObjDataList`.
   - `SendUserBovedaTxtFromChar`: Inspección offline mediante perfiles sintéticos inyectados a través de `SetCharReaderHook` sin acceso a disco.
   - `SendUserBovedaTxtFromChar`: Reporte de `"Usuario inexistente: <Nombre>"` ante perfiles no encontrados.
   - Manejo defensivo ante índices fuera de rango.

---

## Propagación Cruzada y Cableado con Otros Módulos

1. **Módulo #16 (`Protocol.bas`, Capa 4)**:
   - Los manejadores de protocolo `HandleBankStart`, `HandleBankDeposit` y `HandleBankExtractItem` deben cablear directamente con:
     - `modBanco::IniciarDeposito(UserIndex)`
     - `modBanco::UserDepositaItem(UserIndex, Slot, Amount)`
     - `modBanco::UserRetiraItem(UserIndex, Slot, Amount)`
   - Los manejadores `HandleBankExtractGold`, `HandleBankDepositGold` y `HandleMoveBank` residen íntegramente en la capa de protocolo y no interactúan con `modBanco`.
2. **Módulo #19 (`InvUsuario.bas`, Capa 6)**:
   - `modBanco` invoca `InvUsuario::QuitarUserInvItem` y `InvUsuario::UpdateUserInv`. Ambas interfaces operan plenamente coordinadas y vinculadas.
3. **Módulo #34 (`Acciones.bas` / `Modulo_UsUaRiOs.bas`, Capa 9)**:
   - La interacción física por doble clic sobre el NPC Banquero convocará a `modBanco::IniciarDeposito(user_index)`.
