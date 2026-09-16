# Plan de Desglose Modular — Módulo #26: `Trabajo.bas` (Capa 7)

> **Estado**: Planificación Aprobada — Pendiente de Ejecución  
> **Área**: Capa 7 (Lógica de Combate, Magia, Facciones y Oficios)  
> **Documentación Relacionada**: [`docs/audit/12e-trabajo-detalle.md`](../audit/12e-trabajo-detalle.md), [`docs/implementation/00-port-plan.md`](00-port-plan.md), [`docs/CONVENTIONS.md`](../CONVENTIONS.md), [`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md)  
> **Archivos a Crear**: `src/server/Trabajo.hpp`, `src/server/Trabajo.cpp`, `tests/test_trabajo.cpp`, `docs/implementation/26-trabajo.md`

---

## 1. Resumen de Objetivos y Arquitectura

El presente documento define el plan de implementación modular en **6 fases lógicas (G1 a G6)** para la traslación y desacoplamiento del módulo `Trabajo.bas` (~2.350 líneas de VB6) a C++20 (`src/server/Trabajo.hpp` y `src/server/Trabajo.cpp`).

### Decisiones Arquitectónicas Vinculantes

1. **Preservación 1:1 de Firmas de Funciones en PascalCase Exacto**:
   Queda totalmente descartada cualquier estructura intermedia con lambdas o contexto de extracción (ej. `ExtractionContext`).
   Todas las funciones públicas y privadas conservarán su nombre original en PascalCase y sus parámetros exactos indexados sobre `UserList` y `MapData`:
   - `void DoTalar(std::int16_t user_index, bool dar_madera_elfica = false);`
   - `void DoMineria(std::int16_t user_index);`
   - `void HerreroConstruirItem(std::int16_t user_index, std::int16_t item_index);`
   - `void DoApuñalar(std::int16_t user_index, std::int16_t victim_npc_index, std::int16_t victim_user_index, std::int16_t daño);`
   - `bool PuedeConstruir(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items);`

2. **Desacoplamiento mediante Hooks Inyectables (`Trabajo::SetHooks`)**:
   Siguiendo el estándar establecido en `SistemaCombate` e `InvUsuario`, el módulo utilizará una estructura de hooks inyectables dentro del namespace `Trabajo` para permitir pruebas unitarias totalmente desacopladas de red, I/O y temporizadores del sistema.

3. **Replicación Estricta de Quirks Históricos**:
   - **Bug #42 (`DoGolpeCritico`)**: Preservación del reductor accidental al 75% (`daño = static_cast<std::int16_t>(daño * 0.75)`).
   - **Overflow en Fundición (`DoFundir`)**: Preservación de la multiplicación en `std::int16_t` de `(ObjData(index).LingH * num)` antes del escalado.
   - **Quirk Extracción Nivel 1**: Redondeo `CInt` de niveles 1 a 3 que otorga entre 1 y 2 recursos a trabajadores novatos.

---

## 2. Catálogo Detallado de Hooks Tipados (`TrabajoHooks`)

Para aislar las mutaciones de red, efectos de sonido y tiradas aleatorias en la suite de pruebas unitarias `tests/test_trabajo.cpp`, se define la siguiente estructura de inyección de callbacks:

```cpp
namespace Trabajo {

struct TrabajoHooks {
    std::function<std::int32_t(std::int32_t min, std::int32_t max)> RandomNumber;
    std::function<bool(std::int16_t user_index, const Obj& item)> MeterItemEnInventario;
    std::function<void(const WorldPos& pos, const Obj& item)> TirarItemAlPiso;
    std::function<void(std::int16_t user_index, std::string_view msg, std::uint8_t font_type)> WriteConsoleMsg;
    std::function<void(std::int16_t user_index)> WriteUpdateSta;
    std::function<void(std::int16_t user_index)> WriteUpdateGold;
    std::function<void(std::int16_t user_index)> WriteStopWorking;
    std::function<void(std::int16_t user_index)> WriteNavigateToggle;
    std::function<void(std::int16_t user_index)> WriteMeditateToggle;
    std::function<void(std::int16_t user_index)> WriteParalizeOK;
    std::function<void(SendTarget target, std::int32_t index, std::string_view msg)> SendData;
    std::function<void(std::int16_t user_index, std::int16_t skill_index, bool exito)> SubirSkill;
    std::function<void(std::int16_t user_index)> VolverCriminal;
    std::function<void(std::int16_t user_index, std::int16_t victim_npc_index, std::int32_t daño)> CalcularDarExp;
    std::function<void(std::int16_t user_index, std::uint8_t slot)> Desequipar;
    std::function<void(std::int16_t user_index, std::int16_t body, std::int16_t head, std::uint8_t heading, std::int16_t weapon, std::int16_t shield, std::int16_t helmet)> ChangeUserChar;
    std::function<void(std::int16_t user_index, std::int16_t char_index, bool invisible)> SetInvisible;
    std::function<void(std::int16_t user_index)> ToogleBoatBody;
    std::function<void(std::int16_t user_index)> FlushBuffer;
    std::function<std::uint32_t()> GetTickCount;
};

void SetHooks(const TrabajoHooks& hooks);
void ResetHooks();

} // namespace Trabajo
```

---

## 3. Desglose Fase por Fase

---

### Fase 1 (G1 — Núcleo Auxiliar, Modificadores y Verificación de Materiales)

#### Objetivos
- Crear los archivos de cabecera `src/server/Trabajo.hpp` e implementación `src/server/Trabajo.cpp`.
- Implementar las funciones de consulta de modificadores por clase:
  - `float ModNavegacion(eClass clase, std::int16_t user_index);`
  - `float ModFundicion(eClass clase);`
  - `std::int16_t ModCarpinteria(eClass clase);`
  - `float ModHerreriA(eClass clase);`
  - `std::int16_t ModDomar(eClass clase);`
- Implementar las funciones de auxilio de estamina e inventario:
  - `std::int16_t MaxItemsConstruibles(std::int16_t user_index);`
  - `void QuitarSta(std::int16_t user_index, std::int16_t cantidad);`
  - `bool TieneObjetos(std::int16_t item_index, std::int16_t cant, std::int16_t user_index);`
  - `void QuitarObjetos(std::int16_t item_index, std::int16_t cant, std::int16_t user_index);`
- Implementar las funciones de verificación y remoción de materiales de crafteo y upgrades:
  - `bool HerreroTieneMateriales(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items);`
  - `void HerreroQuitarMateriales(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items);`
  - `bool PuedeConstruir(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items);`
  - `bool PuedeConstruirHerreria(std::int16_t item_index);`
  - `bool CarpinteroTieneMateriales(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad, bool show_msg = false);`
  - `void CarpinteroQuitarMateriales(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items);`
  - `bool PuedeConstruirCarpintero(std::int16_t item_index);`
  - `bool TieneMaterialesUpgrade(std::int16_t user_index, std::int16_t item_index);`
  - `void QuitarMaterialesUpgrade(std::int16_t user_index, std::int16_t item_index);`

#### Criterios de Aceptación de Fase 1
- `ModNavegacion` devuelve `1.0` para Pirata, `1.71` para Pescador 100 skill y `2.0` para el resto.
- `TieneObjetos` y `QuitarObjetos` suman y descuentan cantidades correctamente distribuidas en múltiples ranuras de inventario.
- `PuedeConstruir` y `HerreroTieneMateriales` validan con exactitud los requerimientos de lingotes (Hierro, Plata, Oro) y nivel de habilidad.

---

### Fase 2 (G2 — Recolección de Recursos)

#### Objetivos
- Implementar las 4 rutinas principales de extracción de materias primas:
  - `void DoTalar(std::int16_t user_index, bool dar_madera_elfica = false);`
  - `void DoMineria(std::int16_t user_index);`
  - `void DoPescar(std::int16_t user_index);`
  - `void DoPescarRed(std::int16_t user_index);`
- Aplicar la fórmula cuadrática de suerte para la probabilidad de éxito:
  $$\text{Suerte} = \text{static_cast<int>}(-0.00125 \cdot S^2 - 0.3 \cdot S + 49)$$
- Calcular la estamina descontada según sea clase Trabajador o General (`EsfuerzoTalarLeñador` vs `EsfuerzoTalarGeneral`, etc.).
- Calcular la cantidad recolectada para la clase Trabajador:
  $$\text{CantidadItems} = 1 + \text{MaximoInt}\left(1, \text{static_cast<int>}((\text{ELV} - 4) / 5)\right)$$
- Enviar objetos producidos a mochila con `MeterItemEnInventario`; si la mochila está saturada, arrojar a suelo mediante `TirarItemAlPiso`.

#### Criterios de Aceptación de Fase 2
- Verificación unitaria de la curva de probabilidad según skill ($0$, $50$, $100$ puntos de Talar/Minería/Pesca).
- Verificación del volumen de extracción otorgado a Trabajadores de nivel 1..3 vs niveles superiores.
- Confirmación de la caída a suelo al colmar los 30 slots de inventario.

---

### Fase 3 (G3 — Manufactura, Fundición y Upgrades)

#### Objetivos
- Implementar las rutinas de fundición y crafteo:
  - `std::int16_t MineralesParaLingote(iMinerales lingote);` (función privada)
  - `void DoLingotes(std::int16_t user_index);`
  - `void FundirMineral(std::int16_t user_index);`
  - `void FundirArmas(std::int16_t user_index);`
  - `void DoFundir(std::int16_t user_index);`
  - `void HerreroConstruirItem(std::int16_t user_index, std::int16_t item_index);`
  - `void CarpinteroConstruirItem(std::int16_t user_index, std::int16_t item_index);`
  - `void DoUpgrade(std::int16_t user_index, std::int16_t item_index);`
- Preservar la recuperación entre 10% y 25% de lingotes en `DoFundir` manteniendo la multiplicación `(LingH * num)` en `std::int16_t`.
- Procesar el ciclo masivo de construcción reduciendo `.Construir.Cantidad` y emitiendo los efectos de sonido de área (`MARTILLOHERRERO` y `LABUROCARPINTERO`).

#### Criterios de Aceptación de Fase 3
- Fundición exitosa de 50 minerales crudos a 1 lingote.
- Recuperación precisa de porcentaje de lingotes al fundir armas/armaduras.
- Fabricación masiva en ciclo de herrero y carpintero respetando estamina y remoción exacta de lingotes/madera.

---

### Fase 4 (G4 — Habilidades de Combate, Hurto, Desarme y Domesticación)

#### Objetivos
- Implementar las habilidades activas de combate:
  - `void DoApuñalar(std::int16_t user_index, std::int16_t victim_npc_index, std::int16_t victim_user_index, std::int16_t daño);`
  - `void DoAcuchillar(std::int16_t user_index, std::int16_t victim_npc_index, std::int16_t victim_user_index, std::int16_t daño);`
  - `void DoGolpeCritico(std::int16_t user_index, std::int16_t victim_npc_index, std::int16_t victim_user_index, std::int16_t daño);`
- Implementar las habilidades de despojo y desensamble:
  - `void DoRobar(std::int16_t ladr_on_index, std::int16_t victima_index);`
  - `bool ObjEsRobable(std::int16_t victima_index, std::int16_t slot);`
  - `void RobarObjeto(std::int16_t ladr_on_index, std::int16_t victima_index);`
  - `void DoHurtar(std::int16_t user_index, std::int16_t victima_index);`
  - `void DoHandInmo(std::int16_t user_index, std::int16_t victima_index);`
  - `void Desarmar(std::int16_t user_index, std::int16_t victim_index);`
  - `void DoDesequipar(std::int16_t user_index, std::int16_t victim_index);`
- Implementar la mecánica de domesticación de criaturas:
  - `void DoDomar(std::int16_t user_index, std::int16_t npc_index);`
  - `bool PuedeDomarMascota(std::int16_t user_index, std::int16_t npc_index);` (función privada)
  - `std::int16_t FreeMascotaIndex(std::int16_t user_index);`
- **Replicación del Bug #42**: En `DoGolpeCritico`, reducir el daño a 75% (`daño = static_cast<std::int16_t>(daño * 0.75)`).

#### Criterios de Aceptación de Fase 4
- `DoApuñalar` aplica multiplicador 1.5x con daga en Asesino y 1.4x en otras variantes.
- `DoGolpeCritico` ejecuta la reducción a 75% respetando la paridad del Bug #42.
- `DoRobar` y `DoHurtar` respetan la exclusión de ítems no robables (llaves, equipados, barcos, faccionarios) y actualizan el estado criminal.
- `DoDomar` inscribe la mascota en `MascotasIndex` respetando el límite máximo de 2 criaturas del mismo tipo.

---

### Fase 5 (G5 — Gestión de Estados y Entorno)

#### Objetivos
- Implementar las rutinas de ocultamiento, navegación, invisibilidad administrativa y fogatas:
  - `void DoOcultarse(std::int16_t user_index);`
  - `void DoPermanecerOculto(std::int16_t user_index);`
  - `void DoNavega(std::int16_t user_index, const ObjData& barco, std::int16_t slot);`
  - `void DoAdminInvisible(std::int16_t user_index);`
  - `void TratarDeHacerFogata(std::int16_t map, std::int16_t x, std::int16_t y, std::int16_t user_index);`
  - `void DoMeditar(std::int16_t user_index);`
- En `TratarDeHacerFogata`, validar la presencia de al menos 3 unidades de leña (ítem 58) en la celda de `MapData` y transformarlas en `FOGATA_APAG`.
- En `DoMeditar`, evaluar la diferencia de tiempo con `GetTickCount` y regenerar maná progresivamente.

#### Criterios de Aceptación de Fase 5
- Piratas en agua transformándose a galeón fantasmal al ocultarse o navegar.
- `TratarDeHacerFogata` consumiendo paquetes de a 3 troncos de leña en el piso y creando fogatas.
- `DoMeditar` restaurando maná hasta colmar `MaxMAN`.

---

### Fase 6 (G6 — Integración, Cobertura Doctest y Cierre Formal)

#### Objetivos
- Construir la suite de pruebas unitarias en `tests/test_trabajo.cpp` cubriendo exhaustivamente cada oficio y habilidad con `doctest`.
- Crear la documentación técnica definitiva `docs/implementation/26-trabajo.md`.
- Sincronizar los registros en `docs/implementation/KNOWN-LEGACY-BUGS.md` y `docs/implementation/00-port-plan.md`.

#### Cobertura en Pruebas Unitarias (`tests/test_trabajo.cpp`)
1. **Pruebas de Recolección (G2)**: Curva de azar, consumo de estamina y desborde a suelo.
2. **Pruebas de Crafteo (G3)**: Verificación de materiales, consumo de lingotes/madera, sonido de área y paridad de `DoFundir`.
3. **Pruebas de Habilidades de Combate (G4)**: Multiplicador de apuñalar, verificación del Bug #42 en `DoGolpeCritico`, transferencia de oro en robo y límite de mascotas en domar.
4. **Pruebas de Entorno (G5)**: Transformación de leña a fogata y regeneración de maná por meditación.

---

## 4. Matriz de Trazabilidad y Paridad con VB6

| Procedimiento VB6 | Líneas VB6 | Fase G | Hook Inyectado Involucrado |
| :--- | :--- | :---: | :--- |
| `ModNavegacion`, `ModFundicion`, `ModCarpinteria`, `ModHerreriA`, `ModDomar` | 1014-1100 | G1 | N/A |
| `MaxItemsConstruibles`, `QuitarSta`, `TieneObjetos`, `QuitarObjetos` | 311-363, 1906, 2343 | G1 | `WriteUpdateSta` |
| `HerreroTieneMateriales`, `HerreroQuitarMateriales`, `PuedeConstruir`, `PuedeConstruirHerreria` | 365, 420, 528, 539 | G1 | N/A |
| `CarpinteroTieneMateriales`, `CarpinteroQuitarMateriales`, `PuedeConstruirCarpintero` | 378, 390, 661 | G1 | N/A |
| `TieneMaterialesUpgrade`, `QuitarMaterialesUpgrade` | 452, 507 | G1 | N/A |
| `DoTalar`, `DoMineria`, `DoPescar`, `DoPescarRed` | 1379-1521, 1926-2067 | G2 | `RandomNumber`, `MeterItemEnInventario`, `TirarItemAlPiso`, `WriteConsoleMsg`, `SubirSkill` |
| `MineralesParaLingote`, `DoLingotes`, `FundirMineral`, `FundirArmas`, `DoFundir` | 257-309, 779-900 | G3 | `RandomNumber`, `MeterItemEnInventario`, `TirarItemAlPiso`, `UpdateUserInv` |
| `HerreroConstruirItem`, `CarpinteroConstruirItem`, `DoUpgrade` | 562-777, 902-1012 | G3 | `MeterItemEnInventario`, `TirarItemAlPiso`, `SendData`, `SubirSkill` |
| `DoApuñalar`, `DoAcuchillar`, `DoGolpeCritico` | 1793-1904 | G4 | `RandomNumber`, `CalcularDarExp`, `WriteConsoleMsg` |
| `DoRobar`, `ObjEsRobable`, `RobarObjeto`, `DoHurtar`, `DoHandInmo`, `Desarmar`, `DoDesequipar` | 1523-1791, 2155-2340 | G4 | `RandomNumber`, `VolverCriminal`, `WriteUpdateGold`, `FlushBuffer` |
| `DoDomar`, `PuedeDomarMascota`, `FreeMascotaIndex` | 1102-1242 | G4 | `RandomNumber`, `SubirSkill`, `WriteConsoleMsg` |
| `DoOcultarse`, `DoPermanecerOculto`, `DoNavega`, `DoAdminInvisible` | 36-255, 1244-1298 | G5 | `ChangeUserChar`, `SetInvisible`, `ToogleBoatBody` |
| `TratarDeHacerFogata`, `DoMeditar` | 1300-1377, 2069-2153 | G5 | `RandomNumber`, `SubirSkill`, `GetTickCount`, `WriteMeditateToggle` |
