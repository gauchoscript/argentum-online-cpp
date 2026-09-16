# Plan de Desglose Modular — Módulo #25: `ModFacciones`

> **Estado**: Planificación Aprobada — Pendiente de Ejecución  
> **Área**: Capa 7 (Lógica de Combate, Magia, Facciones y Oficios)  
> **Documentación Relacionada**: [`docs/audit/15b-facciones-detalle.md`](../audit/15b-facciones-detalle.md), [`docs/implementation/00-port-plan.md`](00-port-plan.md), [`docs/CONVENTIONS.md`](../CONVENTIONS.md)  
> **Archivos a Crear**: `src/server/ModFacciones.hpp`, `src/server/ModFacciones.cpp`, `tests/test_modfacciones.cpp`

---

## 1. Resumen de Objetivos y Arquitectura

El presente documento establece la hoja de ruta en **4 fases lógicas** para la transliteración y desacoplamiento de `ModFacciones.bas` a C++ (`src/server/ModFacciones.hpp` y `src/server/ModFacciones.cpp`).

### Principios Fundamentales de Diseño
1. **Preservación del Redondeo Bancario en `GetArmourAmount`**:
   Las fórmulas numéricas de asignación de armaduras faccionarias por rango ($1..15$) en VB6 utilizan divisiones y multiplicaciones flotantes con redondeo al entero más cercano (round-half-to-even). En C++ se implementarán mediante `std::nearbyint`:
   - `ieBaja`: `static_cast<std::int16_t>(std::nearbyint(20.0 / (rango + 1.0)))`
   - `ieMedia`: `static_cast<std::int16_t>(std::nearbyint((rango * 2.0) / std::max(rango - 4.0, 1.0)))`
   - `ieAlta`: `static_cast<std::int16_t>(std::nearbyint(rango * 1.35))`
   Queda estrictamente prohibida la división entera truncada para no alterar la paridad de unidades otorgadas.

2. **Reutilización de Símbolos Existentes de `Declares.hpp`**:
   No se redefinirán las matrices ni estructuras globales. Se consumirán directamente los tipos y variables exportados por `Declares.hpp`:
   - `ArmadurasFaccion(clase, raza)` (matriz de `tFaccionArmaduras`)
   - `RecompensaFacciones(rango)` (arreglo de `std::int32_t`)
   - `NUM_RANGOS_FACCION` ($15$) y `NUM_DEF_FACCION_ARMOURS` ($3$)
   - Variables globales de IDs de armaduras/vestimentas faccionarias (`ArmaduraImperial1..3`, `VestimentaImperialHumano`, etc.).

3. **Taxonomía de Quirks e Histórico Legacy (Regla 7 de CONVENTIONS.md)**:
   - **Quirk 7.1 (Bloqueo Irreversible a Legión Oscura)**: Si `user.Faccion.RecibioExpInicialReal == 1`, el enrolamiento en el Caos es rechazado permanentemente en `EnlistarCaos`.
   - **Quirk 7.2 (Exploit de Retención de Ítems Faccionarios)**: En `ExpulsarFaccionReal` y `ExpulsarFaccionCaos`, la llamada `PerderItemsFaccionarios` se mantiene omitida (preservando el comentario legacy `'Call PerderItemsFaccionarios`). Solo se desequipa lo vestido (`Desequipar`), permitiendo retener las armaduras de la mochila/bóveda bancaria.
   - **Quirk 7.3 (Marca Mágica `Reenlistadas = 200`)**: En `EnlistarCaos`, la condición `Reenlistadas == 200` emite el mensaje específico de rebelión militar sin permitir enrolarse.

---

## 2. Desglose Fase por Fase

---

### Fase 1: Infraestructura, Constantes, Callbacks, Fórmulas y Títulos

#### Objetivos
- Crear las cabeceras `ModFacciones.hpp` y `ModFacciones.cpp` dentro del namespace `ModFacciones`.
- Definir la estructura `FactionCallbacks` con inyección de funciones tipadas para desacoplar el módulo de la capa de red e inventario.
- Implementar `SetFactionCallbacks` y `ResetFactionCallbacks`.
- Implementar la función privada `GetArmourAmount` utilizando `std::nearbyint`.
- Implementar las funciones públicas `TituloReal` y `TituloCaos` mapeando los 15 escalafones.

#### Estructura de Callbacks (`FactionCallbacks`)
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
void ResetFactionCallbacks();

std::string TituloReal(std::int16_t user_index);
std::string TituloCaos(std::int16_t user_index);

} // namespace ModFacciones
```

#### Pruebas Unitarias de Fase 1 (`tests/test_modfacciones.cpp`)
1. **`GetArmourAmount`**: Verificar los valores computados de entrega para rangos 1, 4, 7, 10 y 14 en las tres categorías de defensa (`ieBaja`, `ieMedia`, `ieAlta`) confirmando la paridad exacta con el redondeo de VB6.
2. **`TituloReal` y `TituloCaos`**: Verificar la devolución de títulos para todos los índices de `RecompensasReal` y `RecompensasCaos` ($0..15$).

---

### Fase 2: Recompensas, Experiencia y Entrega de Armaduras

#### Objetivos
- Implementar `GiveFactionArmours(user_index, is_caos)`:
  - Recuperar el rango `Rango = (is_caos ? user.Faccion.RecompensasCaos : user.Faccion.RecompensasReal) + 1`.
  - Para cada categoría (`ieBaja`, `ieMedia`, `ieAlta`), calcular `GetArmourAmount(Rango, TipoDef)` y consultar `ArmadurasFaccion(user.clase, user.raza)`.
  - Intentar insertar con `MeterItemEnInventario`. En caso de fallo (mochila llena), arrojar al suelo con `TirarItemAlPiso(user.Pos, ObjArmour)`.
- Implementar `GiveExpReward(user_index, rango)`:
  - Acreditar `RecompensaFacciones(rango)` en `user.Stats.Exp` limitando a `MAXEXP`.
  - Emitir mensaje por consola mediante callback `WriteConsoleMsg`.
  - Evaluar posible ascenso de nivel invocando `CheckUserLevel(user_index)`.
- Implementar `RecompensaArmadaReal(user_index)` y `RecompensaCaos(user_index)`:
  - Verificar si `user.Faccion.CriminalesMatados` (o `CiudadanosMatados`) satisface la meta de `NextRecompensa`.
  - Evaluar las cotas adicionales de nivel (`user.Stats.ELV`) y puntos de nobleza (`user.Reputacion.NobleRep`).
  - Si falta requisito, emitir mensaje `WriteChatOverHead` sobre el NPC objetivo (`user.flags.TargetNPC`) y salir.
  - Al subir de rango, actualizar `RecompensasReal` / `RecompensasCaos`, setear la nueva meta de `NextRecompensa`, emitir mensaje overhead de felicitación, e invocar `GiveFactionArmours` y `GiveExpReward`.

#### Pruebas Unitarias de Fase 2 (`tests/test_modfacciones.cpp`)
1. **Entrega de Armaduras**: Simular inventario con espacio e inventario lleno (verificando el desborde a `TirarItemAlPiso`).
2. **Acreditación de Experiencia**: Verificar la suma de exp, el clamping a `MAXEXP` y la llamada al callback `CheckUserLevel`.
3. **Escalafón de Recompensas Armada**: Probar la progresión paso a paso de rangos 0 a 15, validando que el bloqueo por nobleza o nivel impida avanzar la meta.
4. **Escalafón de Recompensas Caos**: Probar la progresión paso a paso de rangos 0 a 15, verificando que sólo se exijan muertes de ciudadanos y nivel.

---

### Fase 3: Ciclo de Enrolamiento y Expulsión

#### Objetivos
- Implementar `EnlistarArmadaReal(user_index)`:
  - Validar exhaustivamente las 9 condiciones de ingreso (no pertenecer a facciones, no criminal, $\ge 30$ criminales matados, nivel $\ge 25$, $0$ ciudadanos matados, Reenlistadas $\le 4$, nobleza $\ge 1M$, no clan neutral).
  - Si se cumplen, marcar `ArmadaReal = 1`, incrementar `Reenlistadas`, entregar kit de Rango 0 si `RecibioArmaduraReal == 0`, fijar `NextRecompensa = 70`, actualizar barca con `RefreshCharStatus` si navega y registrar log con `LogEjercitoReal`.
- Implementar `EnlistarCaos(user_index)`:
  - Validar las 8 condiciones de ingreso (es criminal, no pertenecer a facciones, $\ge 70$ ciudadanos matados, nivel $\ge 25$, no clan neutral, Reenlistadas $\le 4$).
  - **Requisito Irreversible (Quirk 7.1)**: Verificar `user.Faccion.RecibioExpInicialReal == 0`. Si es 1, denegar ingreso.
  - **Rebelión (Quirk 7.3)**: Si `Reenlistadas == 200`, emitir mensaje específico de rechazo por rebelión.
  - Si se aprueba, marcar `FuerzasCaos = 1`, incrementar `Reenlistadas`, entregar kit de Rango 0 si `RecibioArmaduraCaos == 0`, fijar `NextRecompensa = 160`, actualizar barca y registrar log con `LogEjercitoCaos`.
- Implementar `ExpulsarFaccionReal(user_index, expulsado)` y `ExpulsarFaccionCaos(user_index, expulsado)`:
  - Blanquear el flag faccionario (`ArmadaReal = 0` / `FuerzasCaos = 0`).
  - **Preservación de Ítems en Inventario (Quirk 7.2)**: Mantener comentada la llamada `PerderItemsFaccionarios`. Solamente invocar `Desequipar` para desequipar la armadura o escudo faccionario si están vestidos en ese instante (`ArmourEqpObjIndex` / `EscudoEqpObjIndex`).
  - Notificar por consola (`WriteConsoleMsg`) distinguiendo retiro voluntario de expulsión forzosa.
  - Si `user.flags.Navegando` es `true`, actualizar el gráfico de la barca con `RefreshCharStatus`.

#### Pruebas Unitarias de Fase 3 (`tests/test_modfacciones.cpp`)
1. **Validaciones de Enrolamiento Armada**: Verificar el rechazo individual ante cada una de las 9 condiciones incumplidas y el éxito completo al cumplirlas todas.
2. **Bloqueo Irreversible Caos**: Probar que un personaje con `RecibioExpInicialReal = 1` sea rechazado incondicionalmente al intentar enrolarse en el Caos.
3. **Reenlistadas y Excepción 200**: Probar el rechazo con `Reenlistadas > 4` y la rama de `Reenlistadas = 200`.
4. **Expulsión y Desequipamiento**: Verificar que al expulsar un usuario solo se desequipen las piezas vestidas, conservando las copias adicionales en el inventario.

---

### Fase 4: Integración, Cierre y Verificación Global

#### Objetivos
- Registrar `src/server/ModFacciones.cpp` en `server_core` y `tests/test_modfacciones.cpp` en `unit_tests` dentro de `CMakeLists.txt`.
- Configurar callbacks por defecto hacia `InvUsuario`, `FileIO` y `Protocol`.
- Sincronizar la documentación en `docs/implementation/00-port-plan.md` marcando el módulo como completado.
- Ejecutar la compilación en Ninja / CMake y validar que la suite global pase con 0 fallos.

---

## 3. Matriz de Verificación y Cobertura de Tests

| Componente | Casos de Prueba Doctest (`tests/test_modfacciones.cpp`) | Criterio de Aprobación |
| :--- | :--- | :--- |
| **`GetArmourAmount`** | Cálculo de cantidades para rangos $1..15$ en baja/media/alta. | Coincidencia byte a byte con redondeo bancario de VB6 (`std::nearbyint`). |
| **`Titulos`** | Mapeo de `TituloReal` y `TituloCaos` para índices $0..15$. | Strings exactos sin alteraciones de casing ni caracteres. |
| **`Recompensas`** | Ascenso progresivo en Armada Real y Caos con verificación de bloqueos. | Avance de `NextRecompensa` y entrega de experiencia/armaduras. |
| **`Enrolamiento Armada`** | Evaluación de las 9 reglas de ingreso. | Rechazo selectivo ante cada condición no satisfecha. |
| **`Enrolamiento Caos`** | Evaluación de las 8 reglas + Bloqueo Irreversible + Rebelión 200. | Rechazo estricto ante ex-Armada o rebelde. |
| **`Expulsión`** | Reseteo de flags, desequipamiento y retención de inventario. | Desquipamiento exclusivo de ítems vestidos; resguardo de mochila. |

---

## 4. Estado de Autorización

Plan de desglose listo para su revisión final. Se procederá con la ejecución de la **Fase 1** una vez concedida la autorización del usuario.
