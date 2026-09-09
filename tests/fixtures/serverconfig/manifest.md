# Manifiesto de Fixtures de Configuración del Servidor (`serverconfig/`)

Este directorio contiene los archivos de configuración y roles administrativos del servidor AO:

---

## Fixtures Reales Autoritativos (`real/`)

> [!IMPORTANT]
> **DATOS REALES Y ORIGINALES DEL JUEGO**: Copiados directamente desde `legacy/server/Server.ini` y `legacy/server/Dat/`.

- **`Server.ini`** (3.311 B):
  - Configuración operativa oficial del servidor AO 0.13.0.
  - Parámetros de red (`ServerIp`, `StartPort=7666`), límites de inactividad (`IdleLimit=5`), versión (`0.13.0`).
  - Índices de armaduras imperiales y del caos.
  - Listas canónicas de Game Masters:
    - **`[Dioses]`**: *Elio*, *BetaTester*, *KUD LEBAZ*, *ELVIO GANNET*, *INJU*.
    - **`[SemiDioses]`**: *RESISTENCIA*, *PERFORMANCE*, *SEGMA*, *REIBEN*, *LONEWOLF*.
    - **`[Consejeros]`**: *ZAVETH*, *TIHUN NEBIYU*, *BARBOL*, *LAINADAN*, *GAME DESIGN*.
    - **`[RolesMasters]`**: *ELVIO GANNET*, *INJU*, *LEGL*, *KROGAN*, *MISS LUCHITAZ*.
  - Configuración completa de intervalos de combate, maná, descanso y hambre/sed.
- **`Motd.ini`** (1.079 B):
  - Mensaje del día multilínea transmitido al usuario al ingresar al juego.
- **`Ciudades.Dat`** (166 B):
  - Posiciones de respawn y teleport de las 5 ciudades principales (*Ullathorpe*, *Nix*, *Banderbill*, *Lindos*, *Arghal*).
