# FileIO Grupo 5 — Carga de Tablas de Datos del Juego

Este documento describe el porteo del **Grupo 5** del módulo legacy `legacy/server/Codigo/FileIO.bas`, correspondiente a las rutinas de carga de tablas de datos del juego (`Dat/Obj.dat`, `Dat/Hechizos.dat`, `Dat/Balance.dat`, `Dat/Invokar.dat`, `Dat/NombresInvalidos.txt`, `Dat/ArmasHerrero.dat`, `Dat/ArmadurasHerrero.dat`, `Dat/ObjCarpintero.dat`, `Dat/apuestas.dat`, `Dat/ArmadurasFaccionarias.dat`).

---

## 1. Alcance de Funciones Porteadas

Las siguientes 10 funciones fueron migradas a `src/server/FileIO.hpp` y `src/server/FileIO.cpp`:

1. `CargarSpawnList()`: Lee `Dat/Invokar.dat` en `SpawnList`.
2. `CargarForbidenWords()`: Lee `Dat/NombresInvalidos.txt` línea por línea en `ForbidenNames`.
3. `CargarHechizos()`: Lee `Dat/Hechizos.dat` en `Hechizos` usando `clsIniReader`.
4. `LoadArmasHerreria()`: Lee `Dat/ArmasHerrero.dat` en `ArmasHerrero`.
5. `LoadArmadurasHerreria()`: Lee `Dat/ArmadurasHerrero.dat` en `ArmadurasHerrero`.
6. `LoadBalance()`: Lee `Dat/Balance.dat` en `ModClaseList`, `ModRazaList`, `ModVida`, `DistribucionEnteraVida`, `DistribucionSemienteraVida`, `PorcentajeRecuperoMana`, `ExponenteNivelParty` y `RecompensaFacciones`.
7. `LoadObjCarpintero()`: Lee `Dat/ObjCarpintero.dat` en `ObjCarpintero`.
8. `LoadOBJData()`: Lee `Dat/Obj.dat` en `ObjDataList` usando `clsIniReader`.
9. `CargaApuestas()`: Lee `Dat/apuestas.dat` en `Apuestas`.
10. `LoadArmadurasFaccion()`: Lee `Dat/ArmadurasFaccionarias.dat` en `ArmadurasFaccion`.

---

## 2. Tipos de Datos y Estructuras de Memoria

- **Indexación 1-based de arreglos legacy**: Mantenida mediante dimensionamiento `size + 1` en vectores y arreglos globales (`SpawnList`, `ForbidenNames`, `Hechizos`, `ArmasHerrero`, `ArmadurasHerrero`, `ObjCarpintero`, `ObjDataList`, `ModClaseList`, `ModRazaList`, `ModVida`, `ArmadurasFaccion`).
- **Codificación**: Archivos INI y texto plano parseados en ANSI / Windows-1252.
- **Tipos C++**:
  - `Integer` de VB6 (16 bits) -> `int16_t` / `uint8_t` según rango.
  - `Long` de VB6 (32 bits) -> `int32_t`.
  - `Single` de VB6 -> `float`.
  - `Double` de VB6 -> `double`.

---

## 3. Análisis de Caso de Borde en `LoadOBJData` (`ClaseProhibida`)

En el código legacy de VB6 (`legacy/server/Codigo/FileIO.bas:895-903`):
```vb
            Dim i As Integer
            Dim N As Integer
            Dim S As String
            For i = 1 To NUMCLASES
                S = UCase$(Leer.GetValue("OBJ" & Object, "CP" & i))
                N = 1
                Do While LenB(S) > 0 And UCase$(ListaClases(N)) <> S
                    N = N + 1
                Loop
                .ClaseProhibida(i) = IIf(LenB(S) > 0, N, 0)
            Next i
```
- **Si `CP<N>` es una cadena válida** (ej. `"Guerrero"`): El bucle encuentra coincidencia en `ListaClases` y retorna el índice de la clase (ej. `3`).
- **Si `CP<N>` está vacío (`""`)**: `LenB(S) > 0` es falso, el bucle `Do While` no se ejecuta y `IIf` asigna `0` (`eClass` no especificada / sin restricción).
- **Si `CP<N>` contiene una cadena no coincidente / typo** (ej. `"ClaseInexistente"`): En el runtime legacy de VB6, `N` se incrementa hasta `NUMCLASES + 1` (13), lo que provoca un fallo de runtime `Subscript out of range` (Error 9 de VB6). Este fallo no era manejado de forma grácil por diseño, sino capturado de forma incidental por el controlador general de errores `On Error GoTo Errhandler`.
- **Desviación Deliberada en C++**: La asignación del valor `0` (`static_cast<eClass>(0)`, idéntico a una cadena vacía) ante un nombre no coincidente representa una **desviación deliberada respecto del comportamiento legacy real**. Esta decisión de diseño se tomó por ser la interpretación más segura y razonable frente al comentario explícito del autor legacy (`'CHECK: !!! Esto es provisorio hasta que los de Dateo cambien los valores de string a numerico`), y **no** porque replique el comportamiento de ejecución original en esta entrada. En C++, la búsqueda acotada de `1` a `NUMCLASES` evita accesos fuera de rango o fallos en runtime.

---

## 4. Propagación Cruzada de Variables Globales (Cross-Module Decision Propagation)

Las siguientes variables declaradas en `src/server/Declares.hpp` son pobladas por el Grupo 5 de `FileIO` y quedan listas para su consumo autónomo en futuros módulos:
1. **`ModFacciones` (Capa 7)**: `RecompensaFacciones`, `ArmadurasFaccion`, `tFaccionArmaduras`, `eTipoDefArmors`, `NUM_RANGOS_FACCION`.
2. **`mdParty` (Capa 9)**: `ExponenteNivelParty`.
3. **`Admin` (Capa 10)**: `PorcentajeRecuperoMana`, `tAPuestas`, `Apuestas`.

---

## 5. Verificación y Cobertura de Tests

Suite de pruebas unitarias implementada en `tests/test_fileio_gamedata.cpp` utilizando **doctest**. El módulo cuenta con validación dual completa:

1. **Validación contra Fixtures Sintéticos (Aislamiento y Bordes)**:
   - Cobertura de las 10 rutinas con archivos sintéticos generados en directorios temporales aislados (`tests/temp_g5_*/`).
   - Test de caso de borde para `ClaseProhibida`: valida el mapeo exacto de clase válida (`Guerrero` $\to 3$), cadena vacía (`""` $\to 0$) y nombre inexistente (`"ClaseInexistente"` $\to 0$, protegiendo contra el fallo de índice fuera de rango del VB6 legacy).
   - Test de inicialización y desborde en tablas de herrería, carpintería, spawns y apuestas.

2. **Validación contra Tablas de Datos REALES de Producción (`tests/fixtures/gamedata/real/`)**:
   - **`obj.dat` (1052 objetos)**: Verificación de carga masiva de todos los objetos reales (`NumObjs=1052`), validando tipos de objetos y nombres conocidos (`Manzana Roja`, `Espada Larga`, `Daga`, `Fogata`).
   - **`Hechizos.dat` (46 hechizos)**: Verificación de carga de todos los hechizos (`NumeroHechizos=46`), validando palabras mágicas (`NIHIL VED`), requerimientos de maná y skills.
   - **`Balance.dat`**: Verificación de modificadores de evasión, daño, escudo y atributos por raza y clase (`Humano` Fuerza=1, Constitución=2, `Elfo` Agilidad=3, `Guerrero` Evasión=1.0, `Cazador` Evasión=0.9).
   - **Tablas Complementarias**: Verificación de carga exitosa de `ArmasHerrero.dat`, `ArmadurasHerrero.dat`, `ObjCarpintero.dat`, `ArmadurasFaccionarias.dat`, `Invokar.dat`, `NombresInvalidos.txt` y `apuestas.dat`.

> [!IMPORTANT]
> **Estatus de Fixtures Duales**: Los tests sintéticos validan la robustez lógica y el manejo de entradas inválidas, mientras que los datos autoritativos en `tests/fixtures/gamedata/real/` confirman que las estructuras en memoria de C++ reproducen fielmente el universo de datos del servidor original.


