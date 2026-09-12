# FileIO Grupos 6 y 7 — Respaldos de Mundo (`WorldBackUp`) y Logging Administrativo

Este documento describe el porteo de los **Grupos 6 y 7** del módulo legacy `legacy/server/Codigo/FileIO.bas`, correspondientes a los subsistemas de respaldos del mundo de juego (`DoBackUp`, `CargarBackUp`, `BackUPnPc`, `CargarNpcBackUp`) y registro administrativo de sanciones (`LogBan`, `LogBanFromName`, `Ban`). Con la finalización de estos dos grupos, el módulo **FileIO queda 100% completado**.

---

## 1. Alcance de Funciones Porteadas

Las siguientes 7 funciones y variables de ruta auxiliares fueron implementadas en `src/server/FileIO.hpp` y `src/server/FileIO.cpp`:

### Grupo 7: Logging Administrativo y Sanciones
1. `LogBan(userIndex, bannerIndex, reason)`: Registra el detalle del ban en formato INI en `logs/BanDetail.log` y anexa el nombre del usuario en formato de texto plano a `logs/GenteBanned.log`.
2. `LogBanFromName(bannedName, bannerIndex, reason)`: Registra el ban de un personaje desconectado en `logs/BanDetail.dat` y anexa su nombre a `logs/GenteBanned.log`.
3. `Ban(bannedName, bannerName, reason)`: Registra una sanción administrativa textual directa en `logs/BanDetail.dat` y `logs/GenteBanned.log`, asociándola al autor del ban.

### Grupo 6: Sistema de Respaldos de Mundo (`WorldBackUp`)
4. `BackUPnPc(npcIndex)`: Serializa el estado completo de un NPC activo marcado con `flags.BackUp = 1` hacia `Dat/bkNPCs.dat` (o `WorldBackupPath`), preservando su número de plantilla, nombre, posición, atributos/estadísticas de combate, criaturas invocadas, banderas de comportamiento e inventario portado (`Obj1..N`).
5. `CargarNpcBackUp(npcIndex, npcNumber)`: Deserializa y restaura el estado de un NPC respaldado desde el archivo de backup INI hacia la grilla de memoria (`Npclist[npcIndex]` y `MapData`), restableciendo sus flags de actividad.
6. `CargarBackUp()`: Orquesta la restauración del mundo completo al reiniciar el servidor, cargando los mapas respaldados (`Mapa<N>`) con fallback a los mapas base de `Dat/Map.dat` si algún archivo no está presente en el directorio de respaldo.
7. `DoBackUp()`: Procedimiento de alto nivel ejecutado por los timers del servidor. Establece la bandera de bloqueo `haciendoBK = true`, invoca el guardado de mapas modificados y NPCs respaldados, registra la fecha y hora en `logs/BackUps.log` y libera la bandera `haciendoBK = false`.

### Rutas Auxiliares Configurables
- `FileIO::LogsPath`: Directorio de archivos de registro (`logs/`), configurable en runtime para permitir tests unitarios herméticos sin afectar el entorno del servidor.
- `FileIO::WorldBackupPath`: Directorio de respaldos de mapas y NPCs (`WorldBackUp/`), configurable para pruebas y entornos de despliegue.

---

## 2. Hallazgos Técnicos y Decisiones de Diseño

### A. Formatos de Registro y Quirk Histórico de Extensiones (`.log` vs `.dat`)
- **Estructurado (INI)**:
  - `LogBan` escribe en `logs/BanDetail.log` utilizando secciones por nombre de usuario (`[NombreUsuario]`), conteniendo las claves `BannedBy=<Admin>` y `Reason=<Motivo>`.
  - `LogBanFromName` y `Ban` escriben en `logs/BanDetail.dat`. Esto corresponde a un **quirk histórico explícito del código legacy** (`FileIO.bas:2158,2178`) donde el autor original utilizó la extensión `.dat` en lugar de `.log`. Se preservó fielmente para mantener total interoperabilidad con herramientas legacy de gestión externa.
- **Append de Texto Plano**:
  - Las tres rutinas anexan una línea de texto simple con el nombre del sancionado al final de `logs/GenteBanned.log`. Si el archivo no existe, se crea automáticamente.

### B. Propiedad y Ámbito de `BanIps.dat`
- **Diagnóstico del Legacy**: En el inventario de datos reales del servidor figura el archivo `legacy/server/Dat/BanIps.dat`.
- **Confirmación Arquitectónica**: Dicho archivo **NO** es gestionado por `FileIO.bas`. Su carga y persistencia corresponden exclusivamente a `Admin.bas:407,431` (`GuardarBanIps`, `CargarBanIps`), el cual administra la colección global en memoria `BanIps As New Collection` (`Declares.bas:1542`).
- **Conclusión**: La persistencia de `BanIps.dat` pertenece a la **Capa 10 (`Admin`)** del plan de porting. El módulo `FileIO` se limita estrictamente al baneo de personajes y sus respectivos logs.

### C. Trazabilidad de `DoBackUp`: Persistencia de Mapas/NPCs vs Personajes (`.chr`)
- Se realizó un seguimiento minucioso del flujo de ejecución de `DoBackUp` en `FileIO.bas:373` y `Admin.bas:134` (`WorldSave`):
  ```mermaid
  sequenceDiagram
      autonumber
      participant Timer as modNuevoTimer / GameLogic
      participant FIO as FileIO::DoBackUp
      participant WS as Admin::WorldSave
      participant Maps as FileIO::GrabarMapa
      participant NPCs as FileIO::BackUPnPc
      participant Log as logs/BackUps.log

      Timer->>FIO: DoBackUp()
      FIO->>FIO: haciendoBK = true
      FIO->>WS: WorldSave()
      loop Cada mapa con BackUp = 1
          WS->>Maps: GrabarMapa(map, WorldBackupPath)
      end
      loop Cada NPC con flags.BackUp = 1
          WS->>NPCs: BackUPnPc(npcIndex)
      end
      WS->>WS: SaveForums()
      FIO->>Log: Append timestamp (LastBackup)
      FIO->>FIO: haciendoBK = false
  ```
- **Confirmación Crítica**: `DoBackUp` **NO** guarda ni modifica los archivos de personaje (`.chr`).
  - La persistencia periódica de personajes se ejecuta mediante `GuardarUsuarios()` en `General.bas:1612`.
  - La persistencia individual de personajes ocurre ante el cierre de sesión (`Cerrar_Usuario`) en `Modulo_UsUaRiOs.bas` y `TCP.bas`.
  - Por lo tanto, `DoBackUp` se concentra pura y exclusivamente en el estado del mundo físico (mapas dinámicos, NPCs persistentes, foros y timestamps).

### D. Quirk de Serialización de NPCs en `BackUPnPc` (Inventario vs Drops)
- En `FileIO.bas:1980-2044`, `BackUPnPc` serializa las secciones `[NPC]`, atributos y los objetos de inventario actualmente cargados (`Obj1` a `ObjN` bajo la clave `[INV]`).
- **Comportamiento del Legacy**: No persiste el arreglo `Drop` del NPC. Los drops provienen de la definición estática de la criatura en `Dat/NPCs.dat` al momento del spawn inicial. C++ replica este comportamiento exacto sin serializar drops inexistentes en el backup.

### E. Auditoría de Estado Global en `Declares.hpp`
- En cumplimiento estricto de la regla del proyecto de pausar y auditar antes de alterar `Declares.hpp`:
  - Se confirmó que todas las variables globales requeridas (`haciendoBK`, `LastNPC`, `LastBackup`, `Npclist`, `UserList`, `MapData`, `MapInfoList`) ya estaban formalmente declaradas e inicializadas en `Declares.hpp` y `Declares.cpp`.
  - **No se requirió agregar ninguna nueva variable global**.
  - Se detectó y subsanó una omisión histórica menor en la estructura `struct NPCStats`: se restauró el campo `std::int16_t Alineacion{0};` correspondiente a la línea 1274 de `legacy/server/Codigo/Declares.bas`.

---

## 3. Propagación de Decisiones entre Módulos (*Cross-Module Decision Propagation*)

1. **Hacia `Admin` (Capa 10)**:
   - `WorldSave`: Al implementar `Admin.bas`, conectar `WorldSave` invocando directamente `FileIO::GrabarMapa` para mapas respaldados y `FileIO::BackUPnPc` para NPCs con flag de backup.
   - `BanIps.dat`: Gestionar la colección `BanIps` mediante las rutinas `GuardarBanIps` y `CargarBanIps` de `Admin.cpp`, consumiendo `GetVar`/`WriteVar` de `FileIO`.
   - Variable `haciendoBK`: Utilizar la variable global de `Declares.hpp` para desestimar o encolar operaciones conflictivas de administración mientras el backup está en curso.

2. **Hacia `MODULO_NPCs` (Capa 8)**:
   - Ciclo de Respawn y Backup: Al instanciar NPCs desde backups, verificar `flags.BackUp` y coordinar con `CargarNpcBackUp` asegurando que `MapData[map][x][y].NpcIndex` apunte al slot correspondiente en `Npclist`.

3. **Hacia `General` / `GameLogic` (Capa 11)**:
   - Timers de Mantenimiento: Separar limpiamente el disparo de `FileIO::DoBackUp()` respecto del ciclo de guardado de usuarios `GuardarUsuarios()`, preservando la cadencia original del servidor.

---

## 4. Fixtures de Prueba y Verificación

### Fixtures de Producción Reales
Se incorporaron 3 tríadas de respaldo autoritativas directamente desde `legacy/server/WorldBackup/` en `tests/fixtures/worldbackup/real/`:
- `Mapa1.map`, `Mapa1.inf`, `Mapa1.dat` (Ullathorpe).
- `Mapa10.map`, `Mapa10.inf`, `Mapa10.dat` (Zona boscosa de combate/caza).
- `Mapa11.map`, `Mapa11.inf`, `Mapa11.dat` (Zona de peligro con criaturas).

### Suites de Pruebas Unitarias
1. **`tests/test_fileio_adminlogs.cpp`** (3 casos de prueba, 15 aserciones):
   - Escritura estructurada en `BanDetail.log` y anexado en texto en `GenteBanned.log`.
   - Quirk legacy de guardado en `BanDetail.dat` mediante `LogBanFromName` y validación de nombres vacíos.
   - Ejecución de `Ban` textual y resistencia a índices fuera de rango.
2. **`tests/test_fileio_backup.cpp`** (3 casos de prueba, 35 aserciones):
   - Serialización y deserialización roundtrip fiel de NPCs con `BackUPnPc` y `CargarNpcBackUp` (validando preservación de atributos, posición, inventario y omisión de drops).
   - `CargarBackUp` restaurando mapas reales de `tests/fixtures/worldbackup/real/`.
   - `DoBackUp` orquestando mapas, NPCs y `BackUps.log`, validando que `haciendoBK` inicia y concluye en `false` y verificando formalmente que **los archivos `.chr` permanecen intactos**.

**Resultado global**: 63 casos de prueba ejecutados y aprobados con éxito (2.627 aserciones verificadas, 0 fallos).
