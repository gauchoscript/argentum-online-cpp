---
area: subsistema-inventario-objetos
module_id: 21
source_files:
  - src/server/Comercio.hpp
  - src/server/Comercio.cpp
  - src/server/mdlCOmercioConUsuario.hpp
  - src/server/mdlCOmercioConUsuario.cpp
  - legacy/server/Codigo/Comercio.bas
  - legacy/server/Codigo/mdlCOmercioConUsuario.bas
  - docs/audit/12d-comercio-detalle.md
  - docs/implementation/21-comercio-breakdown.md
  - docs/implementation/KNOWN-LEGACY-BUGS.md
  - docs/CONVENTIONS.md
tags: [comercio, npc, comercio-usuario, p2p, inventario, bugs-replicated, transacciones, cpp20, specification]
last_updated: 2026-09-14
---

# Módulo de Comercio: `Comercio` y `mdlCOmercioConUsuario` (Capa 6, Módulo #21)

## Resumen del Módulo

Este documento formaliza la arquitectura, especificación técnica definitiva, decisiones de bajo nivel, replicación de defectos históricos y cobertura de pruebas del módulo de Capa 6 **Comercio y mdlCOmercioConUsuario** (`src/server/Comercio.hpp`, `src/server/Comercio.cpp`, `src/server/mdlCOmercioConUsuario.hpp` y `src/server/mdlCOmercioConUsuario.cpp`), transliterado a partir de los módulos originales [`legacy/server/Codigo/Comercio.bas`](../../legacy/server/Codigo/Comercio.bas) (288 líneas) y [`legacy/server/Codigo/mdlCOmercioConUsuario.bas`](../../legacy/server/Codigo/mdlCOmercioConUsuario.bas) (352 líneas en Visual Basic 6.0).

El subsistema orquesta los dos canales económicos fundamentales de intercambio de bienes y divisas en el mundo de Argentum Online:
1. **Comercio con Mercaderes NPC (G1 - `Comercio.bas`)**:
   - Apertura de ventana comercial (`IniciarComercioNPC`).
   - Sincronización y transmisión de las ranuras del mercader al cliente (`EnviarNpcInv`).
   - Ubicación y apilamiento en el inventario del mercader (`SlotEnNPCInv`).
   - Fórmulas de tasación económica (`Descuento`, `SalePrice`).
   - Ejecución transaccional unitaria de compra y venta (`Comercio`), con guardianes contra solicitudes anómalas (> 10.000 unidades con autoban), validación de fondos y mochilas, restricciones de sastres faccionarios ("SR" y "SC"), clamping de oro a `MAXORO` en ventas y desacoplamiento de llaves de mansión.
2. **Comercio Seguro P2P — Negociación y Ofertas (G2 - `mdlCOmercioConUsuario.bas`)**:
   - Solicitud e inicialización recíproca de sesión (/COMERCIAR) (`IniciarComercioConUsuario`).
   - Transmisión selectiva de ranuras de oferta al interlocutor (`EnviarOferta`).
   - Cancelación y reseteo integral de la mesa comercial (`FinComerciarUsu`).
   - Carga y modificación incremental de ítems u oro en las 31 ranuras de oferta (`AgregarOferta`), bloqueando mutaciones una vez confirmada la propuesta.
   - Guardianes de persistencia de sesión (`PuedeSeguirComerciando`), preservando el quirk de omisión de distancia física.
3. **Comercio Seguro P2P — Ejecución Bilateral (G3 - `mdlCOmercioConUsuario.bas`)**:
   - Confirmación bilateral atómica (`AceptarComercioUsu`).
   - Transferencia cruzada simultánea de los 31 slots en ambas direcciones.
   - Replicación estricta de defectos legacy vinculantes: evaporación en limbo ante suelo saturado (Bug #34), acreditación previa al débito (Bug #35) y omisión de clampleo contra `MAXORO` (Bug #36).

---

### Estado de Cierre Formal: Completado (Aislado / Cableado Pendiente en Capas 7 y 9)

Conforme a la taxonomía definida en [`docs/CONVENTIONS.md`](../CONVENTIONS.md) (Lista de Chequeo de Finalización de Módulos), este módulo se clasifica en estado **Completado (Aislado / Cableado Pendiente en Capas 7 y 9)**:
- **Código C++ Cerrado**: La implementación de las 11 funciones públicas y sus estructuras auxiliares en `src/server/Comercio.hpp`, `src/server/Comercio.cpp`, `src/server/mdlCOmercioConUsuario.hpp` y `src/server/mdlCOmercioConUsuario.cpp` está 100% finalizada, compilada en `server_core` y verificada con 19 casos de prueba y 121 aserciones doctest.
- **Aislamiento por Inyección de Hooks**: Los puntos de contacto con capas superiores e I/O dependiente del mapa fueron abstraídos mediante callbacks funcionales (`KeyPurchaseHook`, `BanUserHook`, `SubirSkillHook`, `QuitarObjetosHook`, `TirarItemAlPisoHook`).
- **Cableado Pendiente en Capas Futuras**:
  - **Capa 7 (`Trabajo.bas`, Módulo #22)**: Provee la implementación de producción de `QuitarObjetos` para el consumo de ítems al aceptar comercio seguro.
  - **Capa 7 (`Protocol.bas`, Módulo #16)**: Despacho de paquetes de red (`HandleCommerceInit`, `HandleCommerceBuy`, `HandleCommerceSell`, `HandleCommerceEnd`, `HandleUserCommerceInit`, `HandleUserCommerceOffer`, `HandleUserCommerceConfirm`, `HandleUserCommerceEnd`, `HandleUserCommerceOk`).
  - **Capa 9 (`Modulo_UsUaRiOs.bas`, Módulo #34)**: Proveerá la lógica de `SubirSkill`, expulsión/baneo de usuarios y el algoritmo de `TileLibre` para resolución espacial en mapa cuando la mochila está saturada.

---

## Decisiones Estratégicas de Arquitectura y Paridad en C++20

### 1. Replicación del Bug #33: Asimetría de Sincronización en Mercaderes NPC

- **Diagnóstico en VB6**: En `Comercio.bas:257`, la rutina `EnviarNpcInv` itera rígidamente hasta `MAX_NORMAL_INVENTORY_SLOTS = 20` (`For Slot = 1 To MAX_NORMAL_INVENTORY_SLOTS`). No obstante, la estructura física del inventario del NPC (`Npclist[NpcIndex].Invent.Object`) posee `MAX_INVENTORY_SLOTS = 30` ranuras y la rutina `SlotEnNPCInv` asigna ítems hasta la ranura 30. Los ítems vendidos por los jugadores que ocupan los slots 21 a 30 quedan alojados en el servidor pero resultan completamente invisibles e inalcanzables para el cliente oficial.
- **Decisión de Porting**: En estricto cumplimiento de la directiva vinculante de paridad de [`docs/CONVENTIONS.md`](../CONVENTIONS.md), se replica fielmente esta asimetría histórica. `EnviarNpcInv` despacha únicamente las primeras 20 ranuras. Documentado en el Master Bug Ledger ([`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md#entrada-33--comercio-asimetría-de-capacidad-de-slots-en-mercaderes-npc), Entrada #33).

### 2. Replicación del Bug #34: Desborde a Suelo y Evaporación Silenciosa de Ítems en Comercio P2P

- **Diagnóstico en VB6**: En `mdlCOmercioConUsuario.bas:178-182, 221-225`, si la mochila del jugador receptor se encuentra colmada (`MeterItemEnInventario` retorna `False`), el servidor intenta arrojar el ítem al suelo debajo de la posición del receptor (`TirarItemAlPiso`). Si la baldosa y las casillas circundantes están saturadas o bloqueadas (`TileLibre` retorna `(0, 0)`), el ítem no se materializa físicamente en el mapa. Sin embargo, la instrucción siguiente invoca de forma incondicional a `QuitarObjetos` sobre el emisor. El objeto es sustraído de la mochila de origen sin acreditarse en el destino ni en el mapa, evaporándose en el limbo de forma silenciosa e irrecuperable.
- **Decisión de Porting**: Queda terminantemente prohibido incorporar validaciones previas de espacio en suelo o rollback transaccional automático. Se replica de forma idéntica la secuencia legacy original de descarte y posterior débito ciego. Documentado en el Master Bug Ledger ([`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md#entrada-34--mdlcomercioconusuario-desborde-a-suelo-y-evaporación-silenciosa-de-ítems-en-comercio-seguro), Entrada #34).

### 3. Replicación del Bug #35: Acreditación Previa al Débito y Falla de Atomicidad en Comercio P2P

- **Diagnóstico en VB6**: En `AceptarComercioUsu`, para cada ranura de oferta, el servidor primero entrega el ítem al destinatario (`MeterItemEnInventario`) y recién después convoca a `QuitarObjetos` en el emisor. Además, históricamente el parámetro de cantidad de `QuitarObjetos` en `Trabajo.bas:333` era un entero con signo de 16 bits (`cant As Integer`), mientras que `ComUsu.cant` almacenaba un entero de 32 bits (`Long`), propiciando desbordamientos que duplicaban ítems apilables al superar 32.767 unidades.
- **Decisión de Porting**: Se preserva rigurosamente el orden de acreditación en el receptor previa al débito en el emisor. Para evitar Comportamiento Indefinido (*Undefined Behavior*) por desbordamiento de enteros con signo en C++, la aritmética de cálculo de cantidades se promueve a `std::int64_t`, manteniendo la semántica y puntos de contacto del código legacy. Documentado en el Master Bug Ledger ([`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md#entrada-35--mdlcomercioconusuario-acreditación-previa-al-débito-y-duplicación-de-ítems-en-comercio-p2p), Entrada #35).

### 4. Replicación del Bug #36: Omisión de Validación de `MAXORO` en Comercio P2P

- **Diagnóstico en VB6**: En `mdlCOmercioConUsuario.bas:168, 210`, al transferir la oferta monetaria (`offer_slot == GOLD_OFFER_SLOT`), el servidor adiciona directamente el saldo al receptor sin verificar el tope global de 90.000.000 monedas (`MAXORO`), a diferencia de la venta en mercaderes NPC donde sí se clamplea.
- **Decisión de Porting**: Se replica fielmente la ausencia de clampleo contra `MAXORO` en la transferencia entre usuarios. La suma se computa utilizando `std::int64_t` antes de almacenar el resultado directo en `Stats.GLD`, evitando desbordamiento con signo en C++ y preservando la posibilidad histórica de superar los 90M de oro vía P2P. Documentado en el Master Bug Ledger ([`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md#entrada-36--mdlcomercioconusuario-omisión-de-validación-de-maxoro-en-comercio-seguro), Entrada #36).

### 5. Paridad Aritmética en Tasación Económica

- **Compras a Mercaderes NPC**:
  $$\text{Descuento} = 1.0 + \frac{\text{UserSkills}[\text{Comerciar}]}{100.0}$$
  $$\text{PrecioUnitario} = \left\lfloor \left( \frac{\text{ObjData}[\text{ObjIndex}].\text{Valor}}{\text{Descuento}} \times 2.0 \right) + 0.5 \right\rfloor$$
  Se preserva la adición explícita de `+0.5` para emular el redondeo bancario de Visual Basic 6.0 (`Round`), redondeando fracciones de medio centavo hacia arriba.
- **Ventas a Mercaderes NPC**:
  - Si `ObjData[ObjIndex].Newbie == 1`, el valor de recompra es forzado estrictamente a `0.0f`.
  - De lo contrario:
    $$\text{SalePrice} = \frac{\text{ObjData}[\text{ObjIndex}].\text{Valor}}{\text{REDUCTOR\_PRECIOVENTA}} \quad (\text{con } \text{REDUCTOR\_PRECIOVENTA} = 3)$$
    Truncamiento estándar vía conversión entera de C++, consistente con el operador `Fix()` / `Int()` de VB6.

### 6. Quirk Histórico de Distancia Espacial en P2P

- **Diagnóstico en VB6**: En `PuedeSeguirComerciando` (`mdlCOmercioConUsuario.bas:286-352`), se comprueban rangos de índices, flags de conexión activa, reciprocidad en `DestUsu`, coincidencia mutua de cadenas en `DestNick` y vitalidad (`flags.Muerto == 0`). No obstante, **en ningún momento se comprueba la distancia euclidiana o de Chebyshev entre ambos personajes**. Una vez iniciada la sesión comercial, dos jugadores pueden alejarse a mapas opuestos y seguir agregando ofertas y aceptando el intercambio.
- **Decisión de Porting**: Se preserva la ausencia de validación espacial dentro de `PuedeSeguirComerciando`, limitando las comprobaciones estrictamente a las precondiciones originales de VB6.

### 7. Blindaje Léxico en PascalCase (Naming Policy)

- Conforme a la regla institucional establecida en [`docs/CONVENTIONS.md`](../CONVENTIONS.md), toda la superficie pública exportada conserva de forma exacta el nombre en PascalCase de Visual Basic 6.0: `Descuento`, `SalePrice`, `IniciarComercioNPC`, `SlotEnNPCInv`, `EnviarNpcInv`, `Comercio`, `IniciarComercioConUsuario`, `EnviarOferta`, `FinComerciarUsu`, `AgregarOferta`, `PuedeSeguirComerciando` y `AceptarComercioUsu`.

### 8. Desacoplamiento de E/S y Hooks Inyectables

Para garantizar determinismo absoluto en las suites unitarias sin acoplarse a mapas o subsistemas de alto nivel:
- `KeyPurchaseHook`: Intercepta la compra de ítems de tipo `otLlaves` para delegar la persistencia de cerrojos (`Comercio.cpp`).
- `BanUserHook`: Intercepta la detección de pedidos anómalos (> 10.000 unidades) para registrar la sanción sin invocar directamente a `Admin.bas` (`Comercio.cpp`).
- `SubirSkillHook`: Callback para el entrenamiento del skill `Comerciar` (`Comercio.cpp`).
- `QuitarObjetosHook`: Callback inyectable que emula la sustracción de ítems de `Trabajo.bas` (`mdlCOmercioConUsuario.cpp`).
- `TirarItemAlPisoHook`: Callback inyectable que emula la colocación de ítems en el suelo y simula condiciones de saturación espacial (`TileLibre` colmado) (`mdlCOmercioConUsuario.cpp`).

---

## Catálogo Canónico de Procedimientos Implementados

| Módulo Fuente | Procedimiento C++ | Firma Completa | Grupo | Estado |
| :--- | :--- | :--- | :---: | :---: |
| `Comercio.bas` | `Descuento` | `float Descuento(std::int16_t user_index)` | G1 | ✅ Implementado / Testeado |
| `Comercio.bas` | `SalePrice` | `float SalePrice(std::int16_t obj_index)` | G1 | ✅ Implementado / Testeado |
| `Comercio.bas` | `IniciarComercioNPC` | `void IniciarComercioNPC(std::int16_t user_index)` | G1 | ✅ Implementado / Testeado |
| `Comercio.bas` | `SlotEnNPCInv` | `std::int16_t SlotEnNPCInv(std::int16_t npc_index, std::int16_t obj_index, std::int16_t cantidad)` | G1 | ✅ Implementado / Testeado |
| `Comercio.bas` | `EnviarNpcInv` | `void EnviarNpcInv(std::int16_t user_index, std::int16_t npc_index)` | G1 | ✅ Implementado / Testeado |
| `Comercio.bas` | `Comercio` | `void Comercio(eModoComercio modo, std::int16_t user_index, std::int16_t npc_index, std::int16_t slot, std::int16_t cantidad, KeyPurchaseHook key_hook, BanUserHook ban_hook)` | G1 | ✅ Implementado / Testeado |
| `mdlCOmercioConUsuario.bas` | `IniciarComercioConUsuario` | `void IniciarComercioConUsuario(std::int16_t origen, std::int16_t destino)` | G2 | ✅ Implementado / Testeado |
| `mdlCOmercioConUsuario.bas` | `EnviarOferta` | `void EnviarOferta(std::int16_t user_index, std::uint8_t offer_slot)` | G2 | ✅ Implementado / Testeado |
| `mdlCOmercioConUsuario.bas` | `FinComerciarUsu` | `void FinComerciarUsu(std::int16_t user_index)` | G2 | ✅ Implementado / Testeado |
| `mdlCOmercioConUsuario.bas` | `AgregarOferta` | `void AgregarOferta(std::int16_t user_index, std::uint8_t offer_slot, std::int16_t obj_index, std::int64_t amount, bool is_gold)` | G2 | ✅ Implementado / Testeado |
| `mdlCOmercioConUsuario.bas` | `PuedeSeguirComerciando` | `bool PuedeSeguirComerciando(std::int16_t user_index)` | G2 | ✅ Implementado / Testeado |
| `mdlCOmercioConUsuario.bas` | `AceptarComercioUsu` | `void AceptarComercioUsu(std::int16_t user_index, QuitarObjetosHook quitar_obj_hook, TirarItemAlPisoHook tirar_piso_hook)` | G3 | ✅ Implementado / Testeado |

---

## Cobertura de Pruebas Unitarias

La suite de pruebas con **doctest** para el Módulo #21 alcanza **100% de cobertura funcional** a lo largo de sus tres grupos lógicos mediante dos archivos de prueba dedicados, totalizando **19 casos de prueba y 121 aserciones**:

### 1. `tests/test_comercio.cpp` (Comercio NPC — Grupo 1)
- `Calculo de descuento segun skill Comerciar`: Comprobación del divisor según nivel de habilidad (0 = 1.0, 50 = 1.5, 100 = 2.0).
- `Formulas de precios de compra y venta`: Validación de `SalePrice` (división por 3 y retorno 0 para novatos) y redondeo al techo (`+0.5`) en compra.
- `Comercio NPC - Asimetria de slots 20 vs 30 (Bug #33)`: Comprobación de que `EnviarNpcInv` despacha exactamente 20 paquetes de slot ocultando del 21 al 30.
- `Comercio NPC - Deteccion de autoban ante compras anómalas > 10000`: Verificación del guardián de autoban ante cantidades > 10.000 sin modificar oro ni inventario.
- `Comercio NPC - Compra con mochila llena aborta sin debitar oro`: Validación de aborto limpio cuando el inventario del jugador está colmado.
- `Comercio NPC - Venta con acreditacion clampleada a MAXORO`: Validación de que la venta a NPCs clamplea estrictamente a `MAXORO` (90.000.000).
- `Comercio NPC - Restricciones de sastres faccionarios`: Validación de venta restringida a "SR" (Sastre Real) y "SC" (Sastre Caótico).
- `Comercio NPC - Intercepcion de compra de llaves via KeyPurchaseHook`: Validación del callback de compra de llaves para cerrojos.

### 2. `tests/test_comercio_usuario.cpp` (Comercio Seguro P2P — Grupos 2 y 3)
- `IniciarComercioConUsuario - Primer intento solo notifica al destino`: Notificación de solicitud sin abrir ventana hasta reciprocidad.
- `IniciarComercioConUsuario - Reciprocidad abre ventana para ambos`: Despacho mutuo de `UserCommerceInit` y activación de banderas al coincidir `/COMERCIAR`.
- `AgregarOferta - Modificacion de oro e items`: Mutación y supresión de ítems y oro en la mesa comercial.
- `AgregarOferta - Bloqueo de modificacion cuando Confirmo es true`: Aborto inmediato de sesión vía `FinComerciarUsu` si se intenta alterar una oferta confirmada.
- `EnviarOferta - Despacho de ChangeUserTradeSlot`: Verificación del paquete de actualización de slot hacia el interlocutor.
- `PuedeSeguirComerciando - Guardianes de sesion y reset en falla`: Comprobación de cierre de sesión ante muerte, logout o desajuste de nicks.
- `FinComerciarUsu - Reseteo integral de estado`: Blanqueo exhaustivo de variables de comercio y emisión de `UserCommerceEnd`.
- `AceptarComercioUsu - Espera bilateral hasta confirmacion mutua`: Confirmación unidireccional marca `Acepto = true` sin alterar balances hasta el acuerdo mutuo.
- `AceptarComercioUsu - Omicion de MAXORO en transferencia (Bug #36)`: Validación de que el oro transferido entre usuarios supera libremente los 90.000.000 sin clampleo.
- `AceptarComercioUsu - Acreditacion previa al debito (Bug #35)`: Validación de que el receptor recibe el objeto antes de que se convoque la sustracción en el emisor.
- `AceptarComercioUsu - Evaporacion por piso saturado (Bug #34)`: Validación de que ante mochila llena y suelo saturado (`TileLibre` retorna 0,0), el ítem se sustrae del emisor y se evapora en el limbo.

---

## Propagación Cruzada y Notas de Integración

1. **Estado de los Archivos C++**:
   - `src/server/Comercio.hpp` y `src/server/Comercio.cpp` están **formalmente cerrados y completos**.
   - `src/server/mdlCOmercioConUsuario.hpp` y `src/server/mdlCOmercioConUsuario.cpp` están **formalmente cerrados y completos**.
2. **Integración con Protocolo (`Protocol.bas`, Módulo #16)**:
   - Los paquetes entrantes del cliente (`CommerceInit`, `CommerceBuy`, `CommerceSell`, `CommerceEnd`, `UserCommerceInit`, `UserCommerceOffer`, `UserCommerceConfirm`, `UserCommerceEnd`, `UserCommerceOk`) conectan directamente con los 11 procedimientos aquí formalizados.
3. **Conexión con Módulo de Oficios (`Trabajo.bas`, Módulo #22, Capa 7)**:
   - Proveerá la función de producción para `QuitarObjetos`, que satisface el contrato de `SetQuitarObjetosHook`.
4. **Conexión con Módulo de Usuarios (`Modulo_UsUaRiOs.bas`, Módulo #34, Capa 9)**:
   - Proveerá la implementación definitiva de `TileLibre`, conectando con el hook `SetTirarItemAlPisoHook` para resolver la baldosa libre en el mapa durante el desborde por mochila colmada.
   - Proveerá la implementación definitiva para `SetSubirSkillHook` y `SetBanUserHook`.
