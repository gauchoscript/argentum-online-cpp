# Manifiesto de Datos de Prueba (Fixtures) - Argentum Online

> **Nota de Implementación**: El generador de estos datos (	ests/generate_fixtures.ps1) replica de forma **100% fiel** los métodos de escritura y formato exacto producidos por las rutinas reales de guardado de VB6 (SaveUser en FileIO.bas y gestión de clanes en modGuilds.bas / clsClan.cls), utilizando codificación Windows-1252 (ANSI). Todos los nombres en charfile/ y guilds/ cumplen con la restricción de caracteres de AsciiValidos y GuildNameValido.

---

## 1. Personajes Válidos (charfile/)

Se generaron **7 personajes válidos** cubriendo todas las razas, múltiples clases, alineaciones faccionarias y variedad de estados de inventario, hechizos y estadísticas:

1. **PEPE.chr**
   - **Raza / Clase**: Humano / Mago (Nivel 25)
   - **Estado**: 50.000 oro en billetera, 100.000 en banco, inventario semi-lleno (10 slots ocupados), 5 hechizos aprendidos. Miembro del clan Legion de Honor.
2. **GONZALO.chr**
   - **Raza / Clase**: Elfo / Guerrero (Nivel 45)
   - **Caso de Borde**: Personaje de nivel alto con equipamiento completo (arma, armadura, escudo, casco equipados).
   - **Estado**: 250.000 oro, 20 slots de inventario. Fundador y Líder del clan Legion de Honor.
3. **NOVATO.chr**
   - **Raza / Clase**: Humano / Aventurero (Nivel 1)
   - **Caso de Borde**: Personaje inicial con stats mínimos (20 HP, 0 Mana, 0 oro) e inventario totalmente vacío (CantidadItems = 0).
4. **PODEROSO.chr**
   - **Raza / Clase**: Alto Elfo / Mago (Nivel 50 Max)
   - **Caso de Borde**: Atributos en el máximo permitido (21 en Fuerza, Agilidad, Inteligencia, Carisma, Constitución), HP 999, Mana 9999, 5.000.000 oro en billetera, 10.000.000 en banco, inventario completo (30 slots), banco completo (30 slots) y lista máxima de hechizos (35 slots).
5. **CAZADORFURTIVO.chr**
   - **Raza / Clase**: Gnomo / Cazador (Nivel 30)
   - **Estado**: 80.000 oro, arcos y flechas equipados, bandera Escondido = 1.
6. **SACERDOTEREAL.chr**
   - **Raza / Clase**: Elfo Oscuro / Clérigo (Nivel 40)
   - **Alineación**: Faccionario de la Armada Real (EjercitoReal = 1, 50 criminales matados).
   - **Estado**: 150.000 oro, 10 hechizos aprendidos, equipado completo. Fundador y Líder del clan Armada Real.
7. **ASESINOSOMBRIO.chr**
   - **Raza / Clase**: Elfo Oscuro / Asesino (Nivel 42)
   - **Alineación**: Faccionario de las Fuerzas del Caos (EjercitoCaos = 1, 80 ciudadanos matados).
   - **Estado**: 200.000 oro, dagas y vestimenta oscura, bandera Escondido = 1. Fundador y Líder del clan Fuerzas del Caos.

---

## 2. Clanes Válidos (guilds/)

Se generaron **3 clanes válidos** representando las tres alineaciones del juego:

1. **Legion de Honor** (Alineación Neutral)
   - **Archivos**: guildsinfo.inf, Legion de Honor-members.mem, Legion de Honor-solicitudes.sol, Legion de Honor-relaciones.rel.
   - **Integrantes**: 2 miembros (Gonzalo como fundador/líder y PEPE como integrante). Solicitud pendiente de NOVATO.
2. **Armada Real** (Alineación Real / Armada)
   - **Archivos**: guildsinfo.inf, Armada Real-members.mem, Armada Real-solicitudes.sol, Armada Real-relaciones.rel.
   - **Integrantes**: SACERDOTEREAL como líder. Relación de guerra abierta contra Fuerzas del Caos.
3. **Fuerzas del Caos** (Alineación Caos)
   - **Archivos**: guildsinfo.inf, Fuerzas del Caos-members.mem, Fuerzas del Caos-solicitudes.sol, Fuerzas del Caos-relaciones.rel.
   - **Integrantes**: ASESINOSOMBRIO como líder. Relación de guerra abierta contra Armada Real.

---

## 3. Datos Inválidos / Casos de Borde de Codificación (invalid_chars/ e invalid_guilds/)

1. **invalid_chars/ÑANDÚPEÑA.chr**
   - Personaje con caracteres especiales (Ñ, Ú, ñ) que fallan la validación de AsciiValidos en VB6.
2. **invalid_guilds/Legión de Ñandúes-members.mem** y guildsinfo.inf
   - Clan con caracteres especiales (ó, Ñ, ú) que fallan la validación de GuildNameValido en VB6.