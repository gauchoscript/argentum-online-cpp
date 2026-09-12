# FileIO Grupo 4 — Carga y Guardado de Mapas Binarios e INI

Este documento describe el porteo del **Grupo 4** del módulo legacy `legacy/server/Codigo/FileIO.bas`, correspondiente a las rutinas de serialización, deserialización, topología y precomputación de distancias de mapas (`CargarMapa`, `GrabarMapa`, `LoadMapData`, `generateMatrix`, `setDistance`, `getLimit`).

---

## 1. Alcance de Funciones Porteadas

Las siguientes 6 funciones fueron migradas a `src/server/FileIO.hpp` y `src/server/FileIO.cpp`:

1. `CargarMapa(map, mapFile)`: Deserializa un mapa completo a partir de su tríada `.map` (binario de capas/geometría), `.inf` (binario de triggers, traslados, NPCs y objetos) y `.dat` (metadatos INI de configuración del mapa).
2. `GrabarMapa(map, mapFile)`: Serializa un mapa completo desde memoria hacia su tríada de archivos `.map`, `.inf` y `.dat`.
3. `LoadMapData()`: Lee `Dat/Map.dat` para conocer la cantidad global de mapas (`NumMaps`), su ruta de almacenamiento (`MapPath`) y cargar secuencialmente cada uno de ellos.
4. `generateMatrix(mapa)`: Precomputa la matriz bidimensional de distancias mínimas en pasos cardinales (`distanceToCities`) desde cada mapa del mundo hacia cada una de las 5 ciudades principales.
5. `setDistance(mapa, city, side, x, y)`: Algoritmo de exploración recursiva en profundidad para propagar la distancia Manhattan entre mapas colindantes.
6. `getLimit(mapa, side)`: Examina los bordes del mapa (tiles de borde con traslados `TileExit`) en la dirección cardinal indicada (`eHeading`) para identificar el mapa vecino.

---

## 2. Decisiones de Diseño Arquitectónico

### A. Encapsulamiento Local de `tCabecera` y `MiCabecera`
- **Diagnóstico del Legacy**: En VB6, `MiCabecera As tCabecera` estaba declarada como variable global mutable en `Declares.bas`. Sin embargo, el servidor AO opera bajo un modelo monohilo y estrictamente secuencial donde los mapas se cargan o guardan uno a la vez.
- **Decisión C++**: Conforme a la regla de modularidad limpia y para evitar acarrear estado global mutable innecesario en `Declares.hpp` (mismo criterio aplicado a `Queue` y `PathFinding`), `tCabecera` y los datos de cabecera se encapsularon en una estructura interna (`MapHeaderInternal`) y una caché estática local en la unidad de traducción `FileIO.cpp`. `Declares.hpp` no fue contaminado con variables globales de cabecera de mapa.

### B. Convención de Espacios en `desc` (Trim-on-Read / Pad-on-Write)
- **Frontera de serialización**: El campo de descripción del mapa tiene una longitud fija de 255 bytes en el formato binario de VB6.
- **Memoria**: En memoria C++, se almacena estrictamente como un `std::string` limpio, eliminando espacios (`0x20`) y caracteres nulos (`\0`) sobrantes al leer.
- **Escritura**: Al persistir en disco, el `std::string` se copia a un búfer de 255 bytes rellenado previamente con espacios ASCII (`0x20`), logrando una reconstrucción byte a byte 100% fiel al formato original de VB6.

### C. Manejo de NPCs en Ausencia de `MODULO_NPCs` (Capa 8)
- En el archivo binario `.inf`, el registro de NPC almacena el número de NPC original (`NpcNumber`, ej. 536). En VB6, `CargarMapa` llama a `OpenNPC` para instanciarlo en el arreglo `Npclist`, y `GrabarMapa` escribe `Npclist(.NpcIndex).Numero`.
- Para garantizar el funcionamiento desacoplado en la Capa 2 (FileIO), `CargarMapa` asigna el número a `MapData[...].NpcIndex` y asegura que si `Npclist[npcIndex].Numero == 0`, se registre el número allí. Al momento de llamar a `GrabarMapa`, se recupera `Npclist[...].Numero` o el índice directamente, preservando el roundtrip binario idéntico tanto ahora como cuando `OpenNPC` sea portado en la Capa 8.

### D. Manejo y Reseteo de Fogatas (`otFogata`)
- **Regla de Negocio Original (`FileIO.bas:499-504`)**:
  ```vb
  If .ObjInfo.ObjIndex > 0 Then
     If ObjData(.ObjInfo.ObjIndex).OBJType = eOBJType.otFogata Then
          .ObjInfo.ObjIndex = 0
          .ObjInfo.Amount = 0
      End If
  End If
  ```
- **Justificación y Trazabilidad**: Las fogatas (`FOGATA = 63`, `eOBJType::otFogata = 15`) son objetos temporales del mundo encendidos dinámicamente por los jugadores mediante la habilidad de Supervivencia (`Acciones.bas:324-336`, `CrearFuego`). Su ubicación en el mapa es registrada en la colección global `TrashCollector` (`Collection` de objetos planos `cGarbage` de Capa 0) para su eliminación periódica durante el ciclo de mantenimiento `LimpiarMundo` (`General.bas:162-165`).
- **Comportamiento en Persistencia**: Dado que son elementos de juego efímeros que se consumen y apagan, `GrabarMapa` las filtra explícitamente reseteando `ObjIndex = 0` y `Amount = 0` para evitar que queden perpetuadas en los archivos `.inf` como objetos estáticos del mapa. Ver [`docs/audit/01a-clsdicc-cgarbage.md`](../audit/01a-clsdicc-cgarbage.md) y [`docs/implementation/05-cgarbage.md`](05-cgarbage.md).


---

## 3. Estructura de Formatos Binarios e INI

### A. Archivo `.map` (Geometría y Capas Gráficas)
- **Cabecera (273 bytes total)**:
  - `MapVersion`: `int16_t` (2 bytes, little-endian).
  - `tCabecera`:
    - `desc`: `char[255]` (rellenado con `0x20`).
    - `crc`: `int32_t` (4 bytes).
    - `MagicWord`: `int32_t` (4 bytes).
  - 4 valores `TempInt`: 4 $\times$ `int16_t` (8 bytes).
- **Cuerpo (10.000 tiles, bucle exterior Y de 1 a 100, interior X de 1 a 100)**:
  - `ByFlags`: `uint8_t` (1 byte con máscara de bits):
    - `Bit 0 (0x01)`: Tile bloqueado (`Blocked = 1`).
    - `Bit 1 (0x02)`: Capa gráfica 2 presente (`Graphic[2]`).
    - `Bit 2 (0x04)`: Capa gráfica 3 presente (`Graphic[3]`).
    - `Bit 3 (0x08)`: Capa gráfica 4 presente (`Graphic[4]`).
    - `Bit 4 (0x10)`: Trigger presente (`trigger`).
  - `Graphic[1]`: `int16_t` (2 bytes) — **siempre presente incondicionalmente**.
  - Si `Bit 1`: `Graphic[2]` (`int16_t`, 2 bytes).
  - Si `Bit 2`: `Graphic[3]` (`int16_t`, 2 bytes).
  - Si `Bit 3`: `Graphic[4]` (`int16_t`, 2 bytes).
  - Si `Bit 4`: `trigger` (`int16_t`, 2 bytes).

### B. Archivo `.inf` (Triggers, Salidas, NPCs y Objetos)
- **Cabecera (10 bytes total)**:
  - 5 valores `TempInt`: 5 $\times$ `int16_t` (10 bytes).
- **Cuerpo (10.000 tiles, bucle exterior Y de 1 a 100, interior X de 1 a 100)**:
  - `ByFlags`: `uint8_t` (1 byte con máscara de bits):
    - `Bit 0 (0x01)`: Salida de mapa presente (`TileExit`: Map, X, Y).
    - `Bit 1 (0x02)`: NPC presente (`NpcIndex`).
    - `Bit 2 (0x04)`: Objeto tirado presente (`ObjInfo`: ObjIndex, Amount).
  - Si `Bit 0`: `TileExit.Map` (`int16_t`), `TileExit.X` (`int16_t`), `TileExit.Y` (`int16_t`) (6 bytes).
  - Si `Bit 1`: `NpcNumber` (`int16_t`, 2 bytes).
  - Si `Bit 2`: `ObjInfo.ObjIndex` (`int16_t`), `ObjInfo.Amount` (`int16_t`) (4 bytes).

### C. Archivo `.dat` (Propiedades y Reglas de Mapa)
- Formato INI estándar en sección `[Mapa<N>]`:
  - `Name`, `MusicNum`, `StartPos` (`Map-X-Y`), `MagiaSinefecto`, `InviSinEfecto`, `ResuSinEfecto`, `NoEncriptarMP`, `RoboNpcsPermitido`, `Pk` (0 = seguro, 1 = zona de combate insegura), `Terreno`, `Zona`, `Restringir`, `BackUp`.

---

## 4. Verificación y Pruebas Unitarias

Se implementó la suite completa de pruebas en `tests/test_fileio_map.cpp` cubriendo:

1. **4 Mapas Reales de Producción (`tests/fixtures/maps/`)**:
   - **Mapa 1 (Ciudad de Ullathorpe)**: Ejercita las 4 capas gráficas completas (L2: 25 tiles, L3: 226 tiles, L4: 264 tiles), 940 triggers bajo techo, 36 NPCs, 159 objetos y 344 salidas. Verificación de consumo exacto de bytes hasta EOF (`33.183` bytes en `.map` y `12.782` bytes en `.inf`).
   - **Mapa 4 (Bosque exterior)**: Ejercita capas superiores densas, 164 triggers y 32 NPCs (`30.879` bytes en `.map`, `12.358` bytes en `.inf`).
   - **Mapa 8 (Llanura/Sparse baseline)**: Mapa base sin capas 2/4 ni triggers (`30.433` bytes en `.map`, `12.390` bytes en `.inf`).
   - **Mapa 15 (Dungeon)**: Mapa subterráneo con 285 tiles en capa 2 y 0 tiles en capas 3 y 4 (`30.843` bytes en `.map`, `12.074` bytes en `.inf`).
2. **Roundtrip Binario Byte-Exacto**:
   - Carga desde los fixtures autoritativos en `tests/fixtures/maps/`, guardado con `GrabarMapa` en directorio temporal (`tests/temp_maps_roundtrip/`) y comparación binaria byte a byte de `.map` e `.inf`. Ambos buffers resultaron 100% idénticos.
3. **Aserciones Específicas de Coordenadas**:
   - Mapa 1: (40, 30) con `Graphic[1] = 5500`, `Graphic[4] = 5601`, `trigger = BAJOTECHO`; (50, 26) bloqueado; (73, 16) con `NPC = 536`.
   - Mapa 4: (55, 20) con `NPC = 502`; (20, 27) con trigger.
   - Mapa 8: (50, 50) despejado sin capas superiores.
   - Mapa 15: (9, 1) con `Graphic[1] = 1505` y `Graphic[2] = 7307`.
4. **Algoritmos Topológicos y de Distancias**:
   - `getLimit`: Detección en los 4 puntos cardinales (`eHeading::NORTH`, `EAST`, `SOUTH`, `WEST`).
   - `setDistance` y `generateMatrix`: Propagación recursiva de pasos hacia ciudades y detección de mapas inalcanzables (`-1`).
   - `LoadMapData`: Carga y dimensionamiento a partir de `Map.dat`.
   - Resiliencia ante rutas inexistentes y coordenadas o IDs de mapa inválidos.
