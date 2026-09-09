# Manifiesto de Fixtures de Mapas — Argentum Online

> [!IMPORTANT]
> ## DATOS REALES Y ORIGINALES DEL JUEGO (ESTATUS AUTORITATIVO)
> A diferencia de las demás carpetas de fixtures del proyecto (`charfile/`, `guilds/`, `foros/`) que fueron generadas de forma sintética mediante plantillas de script para validar especificaciones documentales, **los archivos contenidos en esta carpeta representan DATOS REALES Y ORIGINALES DEL JUEGO**, copiados directamente de la distribución oficial del servidor legacy (`legacy/server/Maps/`).
>
> Estos mapas constituyen los **primeros fixtures del proyecto con estatus autoritativo de producción**. Permiten validar la deserialización binaria, el consumo exacto de bytes hasta EOF y la equivalencia binaria byte a byte del serializador contra contenido histórico real del juego.

---

## 1. Mapas Seleccionados y Casos de Prueba Asociados

No se copió la totalidad de los 290 mapas del mundo para mantener el repositorio liviano y enfocado. Se seleccionó un subconjunto representativo de **4 mapas** con perfiles geométricos y de entidades dispares que garantizan la cobertura integral de todas las ramas y condiciones lógicas del formato binario:

| Mapa | Nombre / Tipo | Archivos y Tamaños | Características Técnicas y Camino de Ejecución Ejercitado |
| :---: | :--- | :--- | :--- |
| **Mapa 1** | **Ciudad de Ullathorpe** *(Entorno Urbano / Denso)* | • `Mapa1.map`: **33.183 B**<br>• `Mapa1.inf`: **12.782 B**<br>• `Mapa1.dat`: **144 B** | **Ejercita todas las ramas condicionales posibles:**<br>• Utiliza simultáneamente las **4 capas gráficas** (L1 suelo, L2: 25 tiles, L3: 226 tiles, L4: 264 tiles).<br>• **940 triggers** bajo techo (`trigger = 1 / BAJOTECHO`).<br>• **36 NPCs** urbanos (comerciantes, guardias, sacerdotes).<br>• **159 objetos** en el mapa y **344 traslados** (`TileExit`).<br>• Valida la lectura de metadatos urbanos en `.dat` (`Zona=CIUDAD`, `Terreno=BOSQUE`, `Pk=1` -> seguro). |
| **Mapa 4** | **Bosque Exterior** *(Entorno Natural con Triggers)* | • `Mapa4.map`: **30.879 B**<br>• `Mapa4.inf`: **12.358 B**<br>• `Mapa4.dat`: **487 B** | **Ejercita capas superiores y fauna:**<br>• Capa 4 dispersa (3 tiles) y Capa 3 densa (136 tiles de copas de árboles).<br>• **164 triggers** en cabañas/estructuras boscosas.<br>• **32 NPCs** de criaturas y fauna hostil (ej. criatura 502).<br>• **67 objetos** en el suelo. |
| **Mapa 8** | **Llanura Despejada** *(Línea Base Simple / Sparse)* | • `Mapa8.map`: **30.433 B**<br>• `Mapa8.inf`: **12.390 B**<br>• `Mapa8.dat`: **561 B** | **Línea base mínima de verificación:**<br>• **0 tiles en Capa 2**, **0 tiles en Capa 4** y **0 triggers** en todo el mapa.<br>• Sólo capa base (L1) y 80 tiles de capa 3.<br>• Garantiza que las banderas de bits ausentes no introduzcan corrupciones de punteros ni desfases de lectura. |
| **Mapa 15** | **Dungeon Subterráneo** *(Morfología Inversa)* | • `Mapa15.map`: **30.843 B**<br>• `Mapa15.inf`: **12.074 B**<br>• `Mapa15.dat`: **347 B** | **Perfil inverso de capas:**<br>• Presencia significativa de **Capa 2 (285 tiles)** (paredes y grutas subterráneas).<br>• **0 tiles en Capa 3**, **0 tiles en Capa 4**, **0 triggers** y **0 objetos**.<br>• Fauna de criaturas de dungeon (ej. NPC 519). |

---

## 2. Garantías de Verificación en Pruebas Unitarias

Las suites de prueba en [`tests/test_fileio_map.cpp`](../../test_fileio_map.cpp) validan contra estos fixtures:
1. **Consumo Exacto de Bytes hasta EOF**: La suma de los 273 bytes de cabecera más los 10.000 registros variables de tile consume **exactamente** la longitud total del archivo `.map` e `.inf` sin que sobre ni falte un solo byte.
2. **Equivalencia Binaria Byte a Byte (Roundtrip)**: Al cargar cualquiera de estos 4 mapas con `FileIO::CargarMapa` y re-serializarlos con `FileIO::GrabarMapa`, los binarios resultantes son **100% idénticos byte a byte** a los originales aquí almacenados.
3. **Desacoplamiento de Entorno**: Los tests ejecutan sobre `tests/fixtures/maps/`, garantizando independencia absoluta respecto al directorio `legacy/server/Maps/`.
