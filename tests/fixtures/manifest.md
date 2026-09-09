# Manifiesto de Datos de Prueba (Fixtures) - Argentum Online

> **Nota de Implementación**: El generador de estos datos (`tests/generate_fixtures.ps1`) replica los métodos de escritura e interfaz en disco documentados en las auditorías (`FileIO.bas`, `modGuilds.bas` y `modForum.bas`), utilizando codificación Windows-1252 (ANSI). Todos los nombres en `charfile/`, `guilds/` y `foros/` cumplen con las reglas del motor legacy.
> 
> **ADVERTENCIA DE VERIFICACIÓN (CRÍTICO)**: El script de generación (`tests/generate_fixtures.ps1`) **NO invoca ejecutable ni código ejecutable VB6 real** (vía COM o binarios compilados), sino que construye de forma independiente las cadenas e INIs usando plantillas de PowerShell basándose en la especificación documentada en `docs/audit/06-formatos-de-datos.md` y `docs/audit/11a-modforum-detalle.md`. Por lo tanto, estos fixtures **validan nuestra propia especificación documental**, y no el comportamiento en tiempo de ejecución de un binario VB6 legacy real.
> 
> > [!CAUTION]
> > **Corrección Registrada en el Generador Sintético (`generate_fixtures.ps1`)**:
> > Durante la migración del Grupo 2 de `FileIO.bas`, se descubrió un fallo en el propio generador sintético: escribía las claves de inventario como `Item<N>=` en lugar de `Obj<N>=`. La inspección directa del código fuente legacy VB6 (`FileIO.bas:1136` y `1882`) confirmó que la implementación original utiliza únicamente `Obj<N>=` (`Obj1`, `Obj2`, ...). Se corrigió `tests/generate_fixtures.ps1` y se regeneraron todos los fixtures sintéticos en `tests/fixtures/charfile/`. Este hallazgo deja registro explícito de que la limitación aceptada de los fixtures sintéticos conlleva riesgos reales de autoconsistencia.

---

## 1. Personajes Válidos (`charfile/`)

Se generaron **7 personajes válidos** cubriendo todas las razas, múltiples clases, alineaciones faccionarias y variedad de estados de inventario, hechizos y estadísticas:

1. **`PEPE.chr`**
   - **Raza / Clase**: Humano / Mago (Nivel 25)
   - **Estado**: 50.000 oro en billetera, 100.000 en banco, inventario semi-lleno (10 slots ocupados), 5 hechizos aprendidos. Miembro del clan `Legion de Honor`.
2. **`GONZALO.chr`**
   - **Raza / Clase**: Elfo / Guerrero (Nivel 45)
   - **Caso de Borde**: Personaje de nivel alto con equipamiento completo (arma, armadura, escudo, casco equipados).
   - **Estado**: 250.000 oro, 20 slots de inventario. Fundador y Líder del clan `Legion de Honor`.
3. **`NOVATO.chr`**
   - **Raza / Clase**: Humano / Aventurero (Nivel 1)
   - **Caso de Borde**: Personaje inicial con stats mínimos (20 HP, 0 Mana, 0 oro) e inventario totalmente vacío (`CantidadItems = 0`).
4. **`PODEROSO.chr`**
   - **Raza / Clase**: Alto Elfo / Mago (Nivel 50 Max)
   - **Caso de Borde**: Atributos en el máximo permitido (21 en Fuerza, Agilidad, Inteligencia, Carisma, Constitución), HP 999, Mana 9999, 5.000.000 oro en billetera, 10.000.000 en banco, inventario completo (30 slots), banco completo (30 slots) y lista máxima de hechizos (35 slots).
5. **`CAZADORFURTIVO.chr`**
   - **Raza / Clase**: Gnomo / Cazador (Nivel 30)
   - **Estado**: 80.000 oro, arcos y flechas equipados, bandera `Escondido = 1`.
6. **`SACERDOTEREAL.chr`**
   - **Raza / Clase**: Elfo Oscuro / Clérigo (Nivel 40)
   - **Alineación**: Faccionario de la Armada Real (`EjercitoReal = 1`, 50 criminales matados).
   - **Estado**: 150.000 oro, 10 hechizos aprendidos, equipado completo. Fundador y Líder del clan `Armada Real`.
7. **`ASESINOSOMBRIO.chr`**
   - **Raza / Clase**: Elfo Oscuro / Asesino (Nivel 42)
   - **Alineación**: Faccionario de las Fuerzas del Caos (`EjercitoCaos = 1`, 80 ciudadanos matados).
   - **Estado**: 200.000 oro, dagas y vestimenta oscura, bandera `Escondido = 1`. Fundador y Líder del clan `Fuerzas del Caos`.

---

## 2. Clanes Válidos (`guilds/`)

Se generaron **3 clanes válidos** representando las tres alineaciones del juego:

1. **`Legion de Honor`** (Alineación Neutral)
   - **Archivos**: `guildsinfo.inf`, `Legion de Honor-members.mem`, `Legion de Honor-solicitudes.sol`, `Legion de Honor-relaciones.rel`.
   - **Integrantes**: 2 miembros (`Gonzalo` como fundador/líder y `PEPE` como integrante). Solicitud pendiente de `NOVATO`.
2. **`Armada Real`** (Alineación Real / Armada)
   - **Archivos**: `guildsinfo.inf`, `Armada Real-members.mem`, `Armada Real-solicitudes.sol`, `Armada Real-relaciones.rel`.
   - **Integrantes**: `SACERDOTEREAL` como líder. Relación de guerra abierta contra `Fuerzas del Caos`.
3. **`Fuerzas del Caos`** (Alineación Caos)
   - **Archivos**: `guildsinfo.inf`, `Fuerzas del Caos-members.mem`, `Fuerzas del Caos-solicitudes.sol`, `Fuerzas del Caos-relaciones.rel`.
   - **Integrantes**: `ASESINOSOMBRIO` como líder. Relación de guerra abierta contra `Armada Real`.

---

## 3. Foros Válidos (`foros/`)

Se generaron **3 foros válidos** cubriendo mezclas de mensajes, caracteres especiales acentuados y límites máximos de capacidad:

1. **`General.for`** (Foro Mixto)
   - **Archivos**: `General.for` (Índice INI: `CantMSG=3`, `CantAnuncios=2`), `General1.for`, `General2.for`, `General3.for`, `General1a.for`, `General2a.for`.
   - **Caso de Borde de Codificación**: Contiene publicaciones con autor y texto acentuados (`SEÑORÍO_REAL`, `PEÑA_DE_ORO`, `Búsqueda`, `Cacería`, `mágicos`) para verificar la compatibilidad de caracteres ANSI/Windows-1252 (ñ, á, é, í, ó, ú).
2. **`Mercado.for`** (Límite Máximo de Posts Generales)
   - **Archivos**: `Mercado.for` (Índice INI: `CantMSG=30`, `CantAnuncios=1`), `Mercado1.for` a `Mercado30.for` (30 posts generales), `Mercado1a.for`.
   - **Caso de Borde**: Cobertura del límite máximo estricto de 30 posts por foro (`MAX_MENSAJES_FORO = 30`).
3. **`Noticias.for`** (Límite Máximo de Anuncios Fijados)
   - **Archivos**: `Noticias.for` (Índice INI: `CantMSG=5`, `CantAnuncios=5`), `Noticias1.for` a `Noticias5.for`, `Noticias1a.for` a `Noticias5a.for` (5 anuncios fijados).
   - **Caso de Borde**: Cobertura del límite máximo estricto de 5 anuncios fijados por foro (`MAX_ANUNCIOS_FORO = 5`).

---

## 4. Datos Inválidos / Casos de Borde de Codificación (`invalid_chars/` e `invalid_guilds/`)

1. **`invalid_chars/ÑANDÚPEÑA.chr`**
   - Personaje con caracteres especiales (`Ñ`, `Ú`, `ñ`) que fallan la validación de `AsciiValidos` en VB6.
2. **`invalid_guilds/Legión de Ñandúes-members.mem`** y `guildsinfo.inf`
   - Clan con caracteres especiales (`ó`, `Ñ`, `ú`) que fallan la validación de `GuildNameValido` en VB6.

---

## 5. Mapas Reales y Autoritativos del Juego (`maps/`)

> [!IMPORTANT]
> **Datos Originales de Producción**: A diferencia de las secciones sintéticas 1 a 4, los fixtures en `maps/` son **datos reales y originales del juego**, copiados directamente desde `legacy/server/Maps/` para validar la deserialización binaria y el roundtrip byte a byte de `FileIO` (Grupo 4). Ver detalle completo en [`maps/manifest.md`](maps/manifest.md).

- **`Mapa1` (Ciudad de Ullathorpe)**: Ejercita las 4 capas gráficas completas, 940 triggers, 36 NPCs, 159 objetos y 344 traslados.
- **`Mapa4` (Bosque Exterior)**: Capas superiores densas, 164 triggers y 32 NPCs.
- **`Mapa8` (Llanura Despejada)**: Línea base simple sin capas 2/4 ni triggers.
- **`Mapa15` (Dungeon Subterráneo)**: Perfil inverso con capa 2 densa pero sin capas 3/4 ni triggers.

---

## 6. Fixtures Reales Autoritativos Adicionales (`charfile/real/`, `guilds/real/`, `serverconfig/real/`, `gamedata/real/`)

> [!IMPORTANT]
> **Actualización de Estatus Autoritativo**: Se incorporaron a la suite de pruebas los datos históricos de producción hallados en el árbol legacy. Los fixtures sintéticos se preservan intactos para evaluar casos de borde específicos (valores extremos, banco lleno, caracteres acentuados).

1. **Personajes Reales (`charfile/real/`)**: `ELIO.chr`, `USER.chr`, `BETATESTER.chr`, `MASTER.chr`. Ver [`charfile/manifest.md`](charfile/manifest.md).
2. **Clanes Reales (`guilds/real/`)**: `guildsinfo.inf`, `Game Masters-members.mem`, `Game Masters-solicitudes.sol`. Ver [`guilds/manifest.md`](guilds/manifest.md).
3. **Configuración del Servidor Real (`serverconfig/real/`)**: `Server.ini`, `Motd.ini`, `Ciudades.Dat`. Ver [`serverconfig/manifest.md`](serverconfig/manifest.md).
4. **Tablas de Datos Maestras Reales (`gamedata/real/`)**: `obj.dat`, `Hechizos.dat`, `Balance.dat`, `ArmasHerrero.dat`, `ArmadurasHerrero.dat`, `ObjCarpintero.dat`, `ArmadurasFaccionarias.dat`, `Invokar.dat`, `NombresInvalidos.txt`, `apuestas.dat`. Ver [`gamedata/manifest.md`](gamedata/manifest.md).
5. **Respaldos de Mundo Reales (`worldbackup/real/`)**: `Mapa1`, `Mapa10`, `Mapa11` (`.Map`, `.Inf`, `.dat`). Ver [`worldbackup/manifest.md`](worldbackup/manifest.md).

