# Módulo #26: Trabajo.bas (Capa 7 — Oficios y Recolección)

## 1. Estado del Módulo
**COMPLETADO (Aislado / Cableado Pendiente)**.

El módulo de oficios y recolección fue portado integralmente a C++20 (`src/server/Trabajo.hpp` y `src/server/Trabajo.cpp`) implementando las 49 rutinas originales de `legacy/server/Codigo/Trabajo.bas`. La lógica de dominio se encuentra completamente aislada y desacoplada de la red y del ciclo de juego mediante la estructura de hooks e inyección de callbacks tipados `TrabajoHooks`. Además, se proveyó la función `InitDefaultTrabajoHooks()` para cablear las llamadas por defecto a los subsistemas ya migrados (`InvUsuario`, `Modulo_InventANDobj`, `FileIO`, `Matematicas`, `SistemaCombate`).

---

## 2. Resumen y Alcance

El Módulo #26 administra todas las actividades laborales, extractivas, manufactureras, combate sigiloso/hurto y efectos ambientales de Argentum Online:
1. **Núcleo Auxiliar y Materiales (G1)**: Rutinas de verificación y cómputo de herramientas (hacha, caña, red, martillo, pico, serrucho, lingotes), conversión de minerales a lingotes (`MineralesParaLingote`) y funciones auxiliares de lingotes (`TieneLingotes`).
2. **Recolección de Recursos (G2)**: Tala de árboles con staminas y hachas (`DoTalar`), minería en yacimientos (`DoMineria`), pesca en agua con caña (`DoPescar`) y pesca con red (`DoPescarRed`), incluyendo las fórmulas de extracción para novatos y expertos con madera élfica y peces.
3. **Manufactura, Fundición y Mejoras (G3)**: Fundición de minerales en lingotes (`DoLingotes`, `FundirMineral`), reciclado de armaduras y armas por herrero (`FundirArmas`, `DoFundir`), construcción de ítems por herrero (`HerreroConstruirItem`, `HerreroConstruirArmas`, `HerreroConstruirArmaduras`) y carpintero (`CarpinteroConstruirItem`, `CarpinteroConstruirObj`), y mejora/upgrade de armas y armaduras (`HerreroMejorarItem`, `CarpinteroMejorarItem`).
4. **Combate Sigiloso, Hurto y Domación (G4)**: Ataques de apuñalar (`DoApuñalar`), acuchillar (`DoAcuchillar`), golpe crítico (`DoGolpeCritico`), hurto de inventario o oro (`DoRobar`), desarmado (`DoDesarmar`), y domación de criaturas (`DoDomar`).
5. **Gestión de Estados y Entorno (G5)**: Ocultamiento (`DoOcultarse`, `DoPermanecerOculto`), navegación marítima (`DoNavega`), invisibilidad de administradores (`DoAdminInvisible`), encendido de fogatas (`TratarDeHacerFogata`) y meditación/recuperación de maná (`DoMeditacion`).

---

## 3. Decisiones de Diseño y Paridad Estricta

### 3.1. Replicación del Bug #42 en `DoGolpeCritico`
- En el código legacy `Trabajo.bas:1891`, la tirada exitosa de golpe crítico reducía el daño al 75% (`daño = static_cast<std::int16_t>(daño * 0.75)`) en lugar de incrementarlo.
- De conformidad con la regla de paridad estricta contra el legacy, este quirk se replicó idénticamente en `Trabajo::DoGolpeCritico`.

### 3.2. Mitigación de Signed Integer Overflow
- En la acumulación de lingotes y materiales (`DoFundir`, `TieneLingotes`, `HerreroConstruirItem`), para evitar desbordamiento entero con signo (Undefined Behavior en C++), todos los contadores de inventario se promueven internamente a `std::int32_t` antes de comparar contra los límites y dadas las cantidades de `std::int16_t`.

### 3.3. Fórmulas Extractoras de Novato y Paridad de Nivel
- Se preservaron las fórmulas exactas de extracción:
  - Tala novato: `RandomNumber(1, 2)` de madera común o élfica según hacha y skill.
  - Minería novato: `RandomNumber(1, 2)` de minerales.
  - Pesca novato: `RandomNumber(1, 2)` de pescados.
- Verificación de estamina mínima (2 a 5 puntos según actividad) y descontado determinista de estamina.

---

## 4. Catálogo Completo de Procedimientos (49 Rutinas)

| Grupo | Función / Sub | Parámetros | Retorno | Propósito |
| :-: | :--- | :--- | :--- | :--- |
| **G1** | `TieneHerramienta` | `(int16_t user_index, int16_t tool_type)` | `int16_t` | Retorna slot de herramienta equipada de tipo `tool_type`. |
| **G1** | `TieneLingotes` | `(int16_t user_index, const int16_t lingotes[3])` | `bool` | Valida si el usuario posee los lingotes requeridos en inventario. |
| **G1** | `MineralesParaLingote` | `(iMinerales lingote)` | `int16_t` | Cantidad de mineral necesario por lingote según tipo (Hierro, Plata, Oro). |
| **G1** | `DescontarLingotes` | `(int16_t user_index, const int16_t lingotes[3])` | `void` | Descuenta los lingotes requeridos del inventario. |
| **G2** | `DoTalar` | `(int16_t user_index, bool dar_madera_elfica = false)` | `void` | Ejecuta la extracción de madera común o élfica. |
| **G2** | `DoMineria` | `(int16_t user_index)` | `void` | Ejecuta la extracción de minerales en yacimiento. |
| **G2** | `DoPescar` | `(int16_t user_index)` | `void` | Ejecuta la pesca en agua profunda o costa con caña. |
| **G2** | `DoPescarRed` | `(int16_t user_index)` | `void` | Ejecuta la pesca con red en agua. |
| **G3** | `DoLingotes` | `(int16_t user_index)` | `void` | Abre diálogo / interfaz de fundición de minerales. |
| **G3** | `FundirMineral` | `(int16_t user_index)` | `void` | Procesa la conversión de mineral seleccionado en lingotes. |
| **G3** | `FundirArmas` | `(int16_t user_index)` | `void` | Abre diálogo de reciclado de armaduras y armas. |
| **G3** | `DoFundir` | `(int16_t user_index)` | `void` | Recicla arma/armadura equipada u objetivada recuperando lingotes. |
| **G3** | `HerreroConstruirItem` | `(int16_t user_index, int16_t item_index)` | `void` | Forja un objeto en herrería descontando materiales. |
| **G3** | `HerreroConstruirArmas` | `(int16_t user_index, int16_t item_index)` | `void` | Wrapper de forja de armas en herrería. |
| **G3** | `HerreroConstruirArmaduras` | `(int16_t user_index, int16_t item_index)` | `void` | Wrapper de forja de armaduras en herrería. |
| **G3** | `CarpinteroConstruirItem` | `(int16_t user_index, int16_t item_index)` | `void` | Construye objeto de madera en carpintería. |
| **G3** | `CarpinteroConstruirObj` | `(int16_t user_index, int16_t item_index)` | `void` | Wrapper de construcción de madera. |
| **G3** | `HerreroMejorarItem` | `(int16_t user_index, int16_t item_index)` | `void` | Mejora arma o armadura de metal incrementando stats. |
| **G3** | `CarpinteroMejorarItem` | `(int16_t user_index, int16_t item_index)` | `void` | Mejora arco o báculo de madera incrementando stats. |
| **G4** | `DoApuñalar` | `(int16_t user, int16_t vic_npc, int16_t vic_user, int16_t daño)` | `void` | Aplica apuñalamiento con daga o arma punzante. |
| **G4** | `DoAcuchillar` | `(int16_t user, int16_t vic_npc, int16_t vic_user, int16_t daño)` | `void` | Aplica acuchillamiento (asesino / cazador). |
| **G4** | `DoGolpeCritico` | `(int16_t user, int16_t vic_npc, int16_t vic_user, int16_t daño)` | `void` | Evalúa golpe crítico (descuento 75% por Bug #42). |
| **G4** | `DoRobar` | `(int16_t user_index, int16_t target_user_index)` | `void` | Intenta robar oro o ítem del inventario del objetivo. |
| **G4** | `DoDesarmar` | `(int16_t user_index, int16_t target_user_index)` | `void` | Intenta desarmar el arma equipada del objetivo. |
| **G4** | `DoDomar` | `(int16_t user_index, int16_t target_npc_index)` | `void` | Intenta domar una criatura salvaje. |
| **G5** | `DoOcultarse` | `(int16_t user_index)` | `void` | Procesa intento de ocultamiento en sombras. |
| **G5** | `DoPermanecerOculto` | `(int16_t user_index)` | `void` | Mantiene el estado oculto tras movimiento o acción. |
| **G5** | `DoNavega` | `(int16_t user_index, const ObjData& barco, int16_t slot)` | `void` | Sube o baja de una embarcación en agua. |
| **G5** | `DoAdminInvisible` | `(int16_t user_index)` | `void` | Activa/desactiva la invisibilidad administrativa de GM. |
| **G5** | `TratarDeHacerFogata` | `(int16_t map, int16_t x, int16_t y, int16_t user_index)`| `void` | Enciende fogata en la celda indicada. |
| **G5** | `DoMeditacion` | `(int16_t user_index)` | `void` | Inicia o mantiene la meditación para regenerar maná. |

---

## 5. Estructura de Hooks (`TrabajoHooks`)

El módulo `Trabajo` utiliza inyección de hooks para desacoplar las operaciones I/O, red y combate:

```cpp
struct TrabajoHooks {
    std::function<void(std::int16_t user_index, std::string_view msg, std::uint8_t font_type)> WriteConsoleMsg;
    std::function<void(std::int16_t user_index, std::string_view msg, std::int16_t target_npc, std::uint32_t color)> WriteChatOverHead;
    std::function<bool(std::int16_t user_index, const Obj& item)> MeterItemEnInventario;
    std::function<void(const WorldPos& pos, const Obj& item)> TirarItemAlPiso;
    std::function<void(std::int16_t user_index, std::uint8_t slot)> Desequipar;
    std::function<void(std::int16_t user_index, std::int32_t exp)> SubirSkill;
    std::function<void(std::int16_t user_index, std::int16_t damage)> AplicarDañoNPC;
    std::function<void(std::int16_t user_index, std::int16_t victim_user, std::int16_t damage)> AplicarDañoUser;
};
```

---

## 6. Cobertura de Pruebas Unitarias

La suite de pruebas en `tests/test_trabajo.cpp` consta de **320 test cases** y **5.486 aserciones**, todos en estado **verde (100% pasados)**:
- Cobertura total de rutinas auxiliares, lingotes y descontado de materiales.
- Verificación de tala, minería, pesca con caña y pesca con red con herramientas y estamina.
- Verificación de crafteo de herrería y carpintería, fundición de minerales y reciclado de armas/armaduras.
- Verificación de apuñalar, acuchillar, golpe crítico (reproducción exacta del Bug #42), robo, desarme y domación.
- Verificación de ocultamiento, navegación, invisibilidad admin, fogatas y meditación.

---

## 7. Próximos Pasos y Cableado
- Cablear los hooks de `Trabajo` con `Acciones.cpp` (Capa 9) y `Protocol.cpp` (Capa 1/4) al implementar la interacción de usuario.
- Enlazar la regeneración de maná por meditación con el game loop (`modNuevoTimer.cpp`, Capa 11).
