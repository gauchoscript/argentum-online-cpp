# Manifiesto de Fixtures de WorldBackup — Argentum Online

> [!IMPORTANT]
> ## DATOS REALES Y ORIGINALES DEL JUEGO (ESTATUS AUTORITATIVO)
> A diferencia de las carpetas de fixtures sintéticas, **los archivos contenidos en esta carpeta representan DATOS REALES Y ORIGINALES DEL JUEGO**, copiados directamente desde la carpeta de respaldos del servidor legacy oficial (legacy/server/WorldBackup/).
>
> Estos respaldos constituyen fixtures autoritativos de producción para el **Grupo 6 de FileIO** (CargarBackUp y DoBackUp), validando la restauración de mapas respaldados por el servidor histórico.

---

## 1. Respaldos Seleccionados y Archivos

De la colección total de 118 tríadas de mapas respaldados (354 archivos), se seleccionaron **3 tríadas representativas** para mantener el repositorio ágil:

| Mapa | Nombre / Tipo | Archivos y Tamaños | Características Técnicas |
| :---: | :--- | :--- | :--- |
| **Mapa 1** | **Ciudad de Ullathorpe** *(Urbano / Denso)* | • Mapa1.Map: **33.183 B**<br>• Mapa1.Inf: **12.782 B**<br>• Mapa1.dat: **177 B** | Respaldo del mapa urbano principal con 4 capas, 940 triggers y múltiples NPCs/objetos. |
| **Mapa 10** | **Paso de Río y Puentes** *(Topología Fluvial)* | • Mapa10.Map: **30.991 B**<br>• Mapa10.Inf: **11.846 B**<br>• Mapa10.dat: **183 B** | Respaldo de mapa natural con pasajes de puentes, transiciones de terreno y fauna. |
| **Mapa 11** | **Bosque / Pradera** *(Exterior)* | • Mapa11.Map: **30.667 B**<br>• Mapa11.Inf: **12.848 B**<br>• Mapa11.dat: **182 B** | Respaldo de mapa de planicie boscosa con spawns de criaturas y objetos dinámicos. |

---

## 2. Garantías de Verificación

Las suites de prueba en 	ests/test_fileio_backup.cpp validan contra estos fixtures:
1. **Restauración en CargarBackUp**: Carga exitosa de mapas respaldados desde el directorio de WorldBackup cuando BackUp = 1.
2. **Fallback Transparente**: Comprobación de que si un mapa no posee respaldo o tiene BackUp = 0, el motor carga correctamente desde la carpeta base de mapas.
3. **Consistencia de NPCs y Objetos**: Los metadatos de entidades en los archivos .Inf y .dat de respaldo se deserializan manteniendo total paridad con el formato legacy.
