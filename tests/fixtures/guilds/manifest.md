# Manifiesto de Fixtures de Clanes (`guilds/`)

Este directorio contiene dos conjuntos de datos de prueba para la administración y persistencia de clanes:

---

## 1. Fixtures Reales Autoritativos (`real/`)

> [!IMPORTANT]
> **DATOS REALES Y ORIGINALES DEL JUEGO**: Archivos auténticos extraídos de la distribución original del servidor (`legacy/server/guilds/`).

- **`guildsinfo.inf`** (282 B):
  - Catálogo global de clanes con `NroGuilds = 1`.
  - Define el clan ID 1: *Game Masters*, fundador y líder *Elio*, fecha 25/4/2025, alineación Game Masters, codex de 4 líneas y noticias de clan.
- **`Game Masters-members.mem`** (47 B):
  - Padrón oficial de miembros (`NroMembers = 1`, `Member1 = Elio`).
- **`Game Masters-solicitudes.sol`** (27 B):
  - Registro de solicitudes del clan (`CantSolicitudes = 0`).

---

## 2. Fixtures Sintéticos (Directorio Raíz `guilds/`)

> [!NOTE]
> **Datos Diseñados para Casos de Borde**: Generados mediante `tests/generate_fixtures.ps1` para cubrir las tres alineaciones simultáneas y relaciones diplomáticas entre clanes.

- **`Legion de Honor`**: Clan neutral con múltiples miembros y solicitudes pendientes.
- **`Armada Real`**: Clan de alineación real con estado de guerra contra el Caos.
- **`Fuerzas del Caos`**: Clan de alineación caótica con estado de guerra contra la Armada.
- **Archivos asociados**: `guildsinfo.inf` (con 3 clanes), `*-members.mem`, `*-solicitudes.sol`, y `*-relaciones.rel` (archivos de diplomacia ausentes en la muestra real de un único clan).
