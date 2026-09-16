# Módulo #25: ModFacciones.bas (Capa 7 — Facciones y Jerarquías)

## 1. Estado del Módulo
**Completado (Aislado / Cableado Pendiente en Capa 9)**.

El subsistema de facciones fue portado integralmente a C++20 (`src/server/ModFacciones.hpp` y `src/server/ModFacciones.cpp`) implementando el 100% de la lógica original de `legacy/server/Codigo/ModFacciones.bas`. La lógica de dominio se encuentra completamente aislada y desacoplada de la red y del ciclo de vida del servidor mediante la estructura de callbacks tipados `FactionCallbacks`. Además, se proveyó la función `InitDefaultFactionCallbacks()` para cablear por defecto las llamadas hacia los subsistemas ya migrados (`InvUsuario`, `Modulo_InventANDobj`, `FileIO` y `modGuilds`).

---

## 2. Resumen y Alcance

El subsistema administra todas las mecánicas faccionarias de Argentum Online:
1. **Fórmulas de Entrega de Armaduras Faccionarias (`GetArmourAmount`)**: Cálculo de la cantidad de unidades de armadura y cascos/escudos según el rango del personaje y el tipo de defensa (baja, media, alta), utilizando `std::nearbyint` para garantizar la paridad exacta de redondeo con VB6 (round-half-to-even).
2. **Jerarquías y Títulos Faccionarios (`TituloReal`, `TituloCaos`)**: Retorno del título nobiliario u oscuro según la escala de recompensas (rangos 0 a 14+).
3. **Mecánica de Recompensas y Rangos (`RecompensaArmadaReal`, `RecompensaCaos`, `GiveFactionArmours`, `GiveExpReward`)**: Promoción de rango en base a muertes de criminales/ciudadanos, nivel del personaje y reputación de nobleza; acreditación de experiencia y entrega de armaduras faccionarias al inventario o al suelo si no hay espacio.
4. **Ciclo de Alta y Enrolamiento (`EnlistarArmadaReal`, `EnlistarCaos`)**: Validación de requisitos (nivel >= 25, muertes mínimas, nobleza, no pertenecer a la facción opuesta ni ser criminal/ciudadano asesino, clan neutro) y registro del enrolamiento con entrega inicial de equipo.
5. **Ciclo de Baja y Expulsión (`ExpulsarFaccionReal`, `ExpulsarFaccionCaos`)**: Remoción del estado faccionario, desequipamiento automático de armaduras/escudos faccionarios en uso y notificación por consola.

---

## 3. Decisiones de Diseño y Paridad Estricta

### 3.1. Preservación del Redondeo de VB6 en `GetArmourAmount`

Para evitar que la división entera de C++ trunce decimales e invalide la cantidad exacta de unidades otorgadas, se utiliza aritmética en punto flotante combinada con `std::nearbyint`:
- `ieBaja`: `std::nearbyint(20.0 / (rango + 1.0))`
- `ieMedia`: `std::nearbyint((rango * 2.0) / std::max(rango - 4.0, 1.0))`
- `ieAlta`: `std::nearbyint(rango * 1.35)`

### 3.2. Quirks de Dominio Preservados

- **Quirk 7.1 — Bloqueo Irreversible de Caos por Experiencia Inicial Real**:
  - Si un personaje recibió alguna vez la experiencia inicial de la Armada Real (`RecibioExpInicialReal == 1`), no puede enrolarse jamás en la Legión Oscura en `EnlistarCaos`, aunque haya sido expulsado o se haya retirado de la Armada.
- **Quirk 7.2 — Desarme Selectivo al Expulsar**:
  - VB6 tenía comentada la llamada a `PerderItemsFaccionarios`. Los ítems faccionarios en el inventario **NO se pierden ni se tiran al piso**. Únicamente se desequipan la armadura (`ArmourEqpSlot`) y el escudo (`EscudoEqpSlot`) si tienen `Real == 1` o `Caos == 1` en la tabla `ObjDataList`.
- **Quirk 7.3 — Mensaje de Rebelión por Reenlistadas == 200**:
  - Si un usuario tiene `Reenlistadas > 4` e intenta enrolarse en Caos, pero su valor exacto de `Reenlistadas` es `200`, `EnlistarCaos` emite el mensaje específico de rebelión: *"Has sido expulsado de las fuerzas oscuras y durante tu rebeldía has atacado a mi ejército. ¡Vete de aquí!"*.
- **Quirk 7.4 — Escalón Ciego de Nivel en Promociones Reales y Caóticas**:
  - En la promoción de rangos (ej. rango 6, 9, 11-14), si las muertes requeridas se alcanzaron pero el nivel o nobleza no cumple el mínimo, se interrumpe la promoción sin avanzar `RecompensasReal` ni `NextRecompensa`.
- **Quirk 7.5 — Recompensa Máxima Alcanzada**:
  - Al llegar a `NextRecompensa == 10000` (Armada) o `23000` (Caos), el NPC comunica que se alcanzó el rango máximo y no otorga más armaduras ni experiencia adicional.

---

## 4. Catálogo de Procedimientos y Visibilidad

| Función | Parámetros | Retorno | Propósito |
| :--- | :--- | :--- | :--- |
| `GetArmourAmount` | `(int16_t rango, eTipoDefArmors tipo_def)` | `int16_t` | Calcula la cantidad de unidades de armadura a entregar según el rango y tipo de defensa. |
| `TituloReal` | `(int16_t user_index)` | `std::string` | Retorna el título de la Armada Real del personaje según `Faccion.RecompensasReal`. |
| `TituloCaos` | `(int16_t user_index)` | `std::string` | Retorna el título de la Legión Oscura del personaje según `Faccion.RecompensasCaos`. |
| `GiveFactionArmours` | `(int16_t user_index, bool is_caos)` | `void` | Entrega los 3 tipos de defensa (baja, media, alta) al inventario o al suelo. |
| `GiveExpReward` | `(int16_t user_index, int32_t rango)` | `void` | Otorga los puntos de experiencia del rango según la tabla `RecompensaFacciones`. |
| `RecompensaArmadaReal` | `(int16_t user_index)` | `void` | Evalúa requisitos y otorga ascensos de rango y equipo en la Armada Real. |
| `RecompensaCaos` | `(int16_t user_index)` | `void` | Evalúa requisitos y otorga ascensos de rango y equipo en la Legión Oscura. |
| `EnlistarArmadaReal` | `(int16_t user_index)` | `void` | Procesa el alta de enrolamiento en las tropas reales. |
| `EnlistarCaos` | `(int16_t user_index)` | `void` | Procesa el alta de enrolamiento en la Legión Oscura. |
| `ExpulsarFaccionReal` | `(int16_t user_index, bool expulsado = true)` | `void` | Remueve al personaje de la Armada Real y desequipa vestimenta faccionaria. |
| `ExpulsarFaccionCaos` | `(int16_t user_index, bool expulsado = true)` | `void` | Remueve al personaje de la Legión Oscura y desequipa vestimenta faccionaria. |

---

## 5. Catálogo de Callbacks e Integración (`FactionCallbacks`)

El módulo utiliza la estructura de callbacks `FactionCallbacks` declarada en `src/server/ModFacciones.hpp`:

```cpp
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
```

La función `InitDefaultFactionCallbacks()` enlaza estos punteros con las implementaciones reales del servidor:
- `MeterItemEnInventario` -> `InvUsuario::MeterItemEnInventario`
- `TirarItemAlPiso` -> `Modulo_InventANDobj::TirarItemAlPiso`
- `Desequipar` -> `InvUsuario::Desequipar`
- `GetGuildAlignment` -> `modGuilds::GuildAlignment`
- `LogEjercitoReal` -> `FileIO::LogEjercitoReal`
- `LogEjercitoCaos` -> `FileIO::LogEjercitoCaos`

---

## 6. Cobertura de Pruebas Unitarias

La suite de pruebas en `tests/test_modfacciones.cpp` valida minuciosamente todos los aspectos del módulo en 7 casos de prueba (149 aserciones):
1. **`GetArmourAmount formulas and rounding`**: Paridad exacta de las 3 fórmulas y redondeo half-to-even.
2. **`TituloReal and TituloCaos string maps`**: Retorno correcto de todos los títulos por rango (0 a 14).
3. **`GiveFactionArmours inventory and floor fallback`**: Acreditación en inventario y desborde al suelo si el inventario está lleno.
4. **`GiveExpReward cap and level check`**: Incremento de experiencia, límite `MAXEXP` e invocación de `CheckUserLevel`.
5. **`RecompensaArmadaReal & RecompensaCaos progression`**: Ascenso de rangos, barreras de nivel y nobleza, e interrupción si no se cumple el mínimo.
6. **`EnlistarArmadaReal & EnlistarCaos requirements`**: Validación de criminalidad, nivel, muertes, clanes neutros, `RecibioExpInicialReal` (Quirk 7.1) y mensaje de rebelión `Reenlistadas == 200` (Quirk 7.3).
7. **`ExpulsarFaccionReal & ExpulsarFaccionCaos unequip`**: Desequipamiento exclusivo de prendas faccionarias y desestimación del inventario no equipado (Quirk 7.2).
