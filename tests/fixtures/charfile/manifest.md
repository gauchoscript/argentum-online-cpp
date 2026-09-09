# Manifiesto de Fixtures de Personajes (`charfile/`)

Este directorio contiene dos conjuntos de datos de prueba para la persistencia de personajes (.chr):

---

## 1. Fixtures Reales Autoritativos (`real/`)

> [!IMPORTANT]
> **DATOS REALES Y ORIGINALES DEL JUEGO**: Archivos auténticos extraídos de la distribución original del servidor (`legacy/server/Charfile/`). Validan el comportamiento real del motor frente a personajes históricos de producción.

- **`ELIO.chr`** (3.071 B):
  - Humano / Mago (Clase 1, Raza 1).
  - Nivel alto, miembro y fundador del clan *Game Masters* (`GuildIndex = 1`), registrado como Dios en `Server.ini`.
- **`USER.chr`** (3.030 B):
  - Humano / Clérigo (Clase 2, Raza 1).
  - Posición en mapa 1 (Ullathorpe) `(59, 45)`, equipamiento inicial de clérigo.
- **`BETATESTER.chr`** (3.046 B):
  - Personaje Game Master registrado como Dios en `Server.ini`.
- **`MASTER.chr`** (3.068 B):
  - Personaje administrador con atributos y configuraciones de prueba internas.

---

## 2. Fixtures Sintéticos (Directorio Raíz `charfile/`)

> [!NOTE]
> **Datos Diseñados para Casos de Borde**: Generados mediante `tests/generate_fixtures.ps1` para cubrir casos extremos y combinaciones no presentes en los archivos reales.

- **`PEPE.chr`**: Mago Humano (Nivel 25), clan neutral, slots semi-llenos.
- **`GONZALO.chr`**: Guerrero Elfo (Nivel 45), líder de clan, equipamiento completo.
- **`NOVATO.chr`**: Caso de borde con inventario completamente vacío (`CantidadItems = 0`) y estadísticas base mínimas.
- **`PODEROSO.chr`**: Caso de borde con límites máximos permitidos (Nivel 50, stats 21, inventario 30 slots, banco 30 slots, 35 hechizos).
- **`CAZADORFURTIVO.chr`**: Gnomo Cazador (Nivel 30), proyectiles y bandera `Escondido = 1`.
- **`SACERDOTEREAL.chr`**: Faccionario de la Armada Real (`EjercitoReal = 1`).
- **`ASESINOSOMBRIO.chr`**: Faccionario de las Fuerzas del Caos (`EjercitoCaos = 1`).
