---
area: subsistema-combate
module_id: 22
source_files:
  - src/server/SistemaCombate.hpp
  - src/server/SistemaCombate.cpp
  - legacy/server/Codigo/SistemaCombate.bas
  - docs/audit/04a-sistemacombate-detalle.md
  - docs/implementation/22-sistemacombate-breakdown.md
  - docs/implementation/KNOWN-LEGACY-BUGS.md
  - docs/CONVENTIONS.md
tags: [combate, pvp, pve, formulas, daño, evasion, escudos, apuñalar, mata-dragones, mascotas, bugs-replicated, cpp20, specification]
last_updated: 2026-09-15
---

# Módulo de Combate: `SistemaCombate` (Capa 7, Módulo #22)

## Resumen del Módulo

Este documento formaliza la arquitectura, especificación técnica definitiva, decisiones de bajo nivel, replicación de defectos históricos, peculiaridades de balance y cobertura de pruebas del módulo de Capa 7 **SistemaCombate** (`src/server/SistemaCombate.hpp` y `src/server/SistemaCombate.cpp`), transliterado a partir del módulo original [`legacy/server/Codigo/SistemaCombate.bas`](../../legacy/server/Codigo/SistemaCombate.bas) (1.922 líneas en Visual Basic 6.0).

El módulo conforma el núcleo motor de resolución de enfrentamientos físicos (cuerpo a cuerpo, a distancia con proyectiles y combate desarmado/wrestling) en el mundo de Argentum Online:
1. **Fórmulas Aritméticas de Evasión y Poder Ofensivo (Fase 1: G1)**:
   - Utilidades matemáticas seguras (`MinimoInt`, `MaximoInt`).
   - Poder de evasión corporal (`PoderEvasion`), con tramos de habilidad y preservación estricta de división flotante `/ 33.0`.
   - Poder de evasión con escudo (`PoderEvasionEscudo`).
   - Poder de ataque cuerpo a cuerpo (`PoderAtaqueArma`), con proyectiles (`PoderAtaqueProyectil`), desarmado (`PoderAtaqueWrestling`) y compuesto (`PoderAtaqueModificado`).
   - Consultas de contexto y municiones (`CheckResistencia`, `ArcoYFlecha`, `AlcanzaEspacio`, `CheckArmasMuniciones`, `ArmaParaApuñalar`).
   - Determinación de relaciones sociales y de grupo (`SameClan`, `SameParty`).
   - Evaluación de cuadrículas de combate en mapa (`TriggerZonaPelea` con `eTrigger6`).
   - Control determinista de cadencia física (`IntervaloPermiteAtacar` y `CombatTimeProvider`).
2. **Resolución de Impacto y Daño Bruto RNG (Fase 2: G2)**:
   - Sorteo de acierto/fallo probabilístico (`ProbExito`, curva con piso del 10% y techo del 90%).
   - Chequeos de acierto para todos los emparejamientos: `UserImpactoUser`, `UserImpactoNpc`, `NpcImpactoUser`, `NpcImpactoNpc`, `UserImpacto`, `NpcImpacto`, `NpcEvasion`.
   - Generación de daño bruto físico (`CalcularDaño` y `NpcDaño`), incluyendo replicación estricta del **Bug #37** en proyectiles.
3. **Deducción de Daño, Absorciones, Desgaste y Modificadores (Fase 3: G3)**:
   - Absorción diferenciada por zona anatómica (cabeza vs torso/escudo).
   - Combate sobre embarcaciones navales (`defbarco`).
   - Deducción neta en salud, envenenamiento pasivo por armas (`UserEnvenena`), bonificaciones de nivel y experiencia compartida (`CalcularDarExp`, `RestarCriminalidad`).
   - Peculiaridades de dominio: letalidad instantánea y rotura de la Espada Mata Dragones (**Criterio 7.1**), asimetría ZaMa en apuñalamiento (**Criterio 7.2**).
4. **Orquestación de Flujo, Protocolo y Mascotas (Fase 4: G4)**:
   - Despacho de ataques con validaciones completas de cooldown, estados y energía: `UsuarioAtaca`, `UsuarioAtacaUsuario`, `UsuarioAtacadoPorUsuario`, `UsuarioAtacaNpc`, `NpcAtacaUser`, `NpcAtacaNpc`.
   - Sigilo de Administradores invisibles en swing al aire (**Criterio 7.4**).
   - Legítima defensa con exención penal y de frags (**Criterio 7.3**).
   - Matrices legales de combate (`PuedeAtacar`, `PuedeAtacarNPC`).
   - Control táctico de criaturas y defunciones de mascotas (`MuereNpc`, `RestarCriaturasEntrenador`, `CheckPets`, `AllFollowAmo`, `AllMascotasAtacanUser`).

---

### Estado de Cierre Formal: Completado (Aislado / Cableado Pendiente en Capas 7, 8, 9 y 11)

Conforme a la taxonomía definida en [`docs/CONVENTIONS.md`](../CONVENTIONS.md) (Lista de Chequeo de Finalización de Módulos), este módulo se clasifica en estado **Completado (Aislado / Cableado Pendiente en Capas 7, 8, 9 y 11)**:
- **Código C++ Cerrado**: La implementación de los 33 procedimientos públicos y sus estructuras auxiliares en `src/server/SistemaCombate.hpp` y `src/server/SistemaCombate.cpp` está 100% finalizada, compilada en `server_core` y verificada con 26 casos de prueba unitaria y 162 aserciones doctest específicas.
- **Aislamiento por Inyección de Hooks**: Todos los puntos de contacto con subsistemas de capas concurrentes o superiores fueron desacoplados mediante callbacks funcionales fuertemente tipados (`QuitarObjetosHook`, `DoApuñalarHook`, `DoGolpeCriticoHook`, `DoAcuchillarHook`, `UserDieHook`, `MuereNpcHook`, `SubirSkillHook`, `CheckUserLevelHook`, `PartyExpHook`, `RefreshCharStatusHook`, `VolverCriminalHook`, `CancelExitHook`, `StoreFragHook`, `ContarMuerteHook`).
- **Cableado Pendiente en Capas Futuras**:
  - **Capa 7 (`modHechizos.bas`, Módulo #23)**: Reutilización de verificaciones de parálisis/muerte, alcance máximo de lanzamiento (`MAXDISTANCIAMAGIA = 18`) y ruteo a hooks comunes de muerte.
  - **Capa 8 (`MODULO_NPCs.bas` #28 y `AI_NPC.bas` #29)**: Despacho de rutinas de combate para criaturas autónomas (`NpcAtacaUser`, `NpcAtacaNpc`) y conexión del hook `MuereNpcHook` para drops y reaparición de NPCs.
  - **Capa 9 (`clsParty` / `mdParty`, Módulo #32)**: Conexión de `PartyExpHook` para reparto proporcional de experiencia entre miembros del grupo en mapa y rango visible.
  - **Capa 9 (`Modulo_UsUaRiOs.bas`, Módulo #34)**: Conexión definitiva de `UserDieHook`, `SubirSkillHook`, `CheckUserLevelHook`, `VolverCriminalHook` y `CancelExitHook`.
  - **Capa 11 (`TCP.bas`, Módulo #14 / Loop Principal)**: Invocación de `UsuarioAtaca` desde el despachador de paquetes entrantes (`ClientPacketID::Attack`).

---

## Decisiones Estratégicas de Arquitectura y Paridad en C++20

### 1. Replicación del Bug #37: Omisión de Daño Máximo de Proyectiles en Bono de Fuerza

- **Diagnóstico en VB6**: En `SistemaCombate.bas:384-388`, al calcular el daño físico para armas con proyectiles (arcos/ballestas), el motor suma el daño aleatorio del proyectil a la variable de daño acumulado (`daño_arma += RandomNumber(proyectil.MinHIT, proyectil.MaxHIT)`). Sin embargo, **omite deliberadamente** acumular el valor superior `proyectil.MaxHIT` en la variable `daño_max_arma` (`daño_max_arma` permanece igual a `arma.MaxHIT`). Como consecuencia, al calcular el bono por fuerza del personaje:
  $$\text{BonoFuerza} = \left\lfloor (\text{Fuerza} - 15) \times \left( \frac{\text{daño\_max\_arma}}{20} \right) \right\rfloor$$
  el multiplicador depende pura y exclusivamente de la cota máxima del arco, ignorando el calibre de las flechas pesadas.
- **Decisión de Porting**: En cumplimiento estricto de la regla vinculante de replicación de defectos de `CONVENTIONS.md`, se replica idénticamente este comportamiento histórico. `daño_max_arma` no suma `proyectil.MaxHIT`. Documentado en el Master Bug Ledger ([`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md#entrada-37--sistemacombate-omisión-del-daño-máximo-del-proyectil-en-el-bono-por-fuerza-en-arcos), Entrada #37).

---

### 2. Peculiaridades de Dominio y Balance (Regla 7 de `CONVENTIONS.md`)

De acuerdo con la Taxonomía Global de Hallazgos (Sección 5 de `docs/CONVENTIONS.md`), se clasifican formalmente como peculiaridades intencionales de diseño y balance las siguientes cuatro mecánicas:

#### Criterio 7.1: Espada Mata Dragones (`EspadaMataDragonesIndex = 402`)
- **Comportamiento en VB6 (`SistemaCombate.bas:394-401, 502-510, 560-562`)**:
  - Si el blanco es un NPC de tipo dragón (`npc.NPCtype == eNPCType.DRAGON`), el daño infligido es letal e instantáneo:
    $$\text{Daño} = \text{npc.Stats.MinHp} + \text{npc.Stats.def}$$
    liquidando al dragón de un único impacto.
  - Inmediatamente después de asestar el golpe mortal, la espada se autodestruye del inventario del personaje vía `s_quitar_objetos_hook(EspadaMataDragonesIndex, 1, user_index)`.
  - Si la espada es utilizada contra cualquier otro blanco (otro tipo de NPC o en PvP contra otro usuario), su daño se fuerza rígidamente a **1 punto fijo**, evitando que funcione como arma convencional.
- **Implementación C++**: Replicado 1:1 en `CalcularDaño`, `UserDañoNpc` y `UserDañoUser`.

#### Criterio 7.2: Asimetría ZaMa en Apuñalamiento (PvE vs PvP)
- **Comportamiento en VB6 (`SistemaCombate.bas:549` vs `SistemaCombate.bas:1145`)**:
  - En combate contra criaturas (PvE), la invocación a `DoApuñalar` envía el daño bruto previo a la sustracción de la armadura (`daño_base`):
    `Call DoApuñalar(UserIndex, 0, NpcIndex, daño_base)`
  - En combate entre usuarios (PvP), tras el parche del desarrollador ZaMa del 07/04/2010, `DoApuñalar` recibe el daño **neto post-absorción** de cascos, armaduras y escudos (`daño`):
    `Call DoApuñalar(AtacanteIndex, VictimaIndex, 0, daño)`
- **Implementación C++**: Preservada la divergencia semántica exacta entre `UserDañoNpc` y `UserDañoUser`.

#### Criterio 7.3: Legítima Defensa (`flags.AtacablePor`)
- **Comportamiento en VB6 (`SistemaCombate.bas:1360-1365, 1456-1473, 1249-1254`)**:
  - Cuando un ciudadano ataca primero ilegítimamente a otro ciudadano, el agredido adquiere la marca temporal de legítima defensa: `victima.flags.AtacablePor = atacante`.
  - Si el agredido contraataca:
    1. `PuedeAtacar` autoriza el combate de forma inmediata sin comprobar el estado de `flags.Seguro` ni facciones reales.
    2. El contraataque **no incrementa `BandidoRep`** ni invoca `s_volver_criminal_hook`.
    3. Si el agresor inicial muere a manos del defensor en legítima defensa, el servidor **omite la penalización de `NobleRep`**, **no llama a `s_store_frag_hook`** y **no convoca a `s_contar_muerte_hook`**.
- **Implementación C++**: Replicado de forma estricta en `PuedeAtacar`, `UsuarioAtacadoPorUsuario` y `UserDañoUser`.

#### Criterio 7.4: Sigilo de Administrador Invisible en Swing al Aire
- **Comportamiento en VB6 (`SistemaCombate.bas:1205-1210`)**:
  - Cuando un personaje falla un golpe o ataca al aire, el servidor genera el sonido de abanico (`SND_SWING = 10`).
  - Si el atacante es un Game Master con invisibilidad administrativa activada (`flags.AdminInvisible == 1`), el paquete de audio se transmite en unicast **únicamente a su propio socket** (`TCP::EnviarDatosASlot`), suprimiendo el broadcast a los demás clientes del área (`SendTarget::ToPCArea`).
- **Implementación C++**: Replicado en el helper estático `DispatchSwingSound`.

---

### 3. Paridad Aritmética Segura y Preservación de División Flotante

1. **División en Punto Flotante en `PoderEvasion`**:
   - En VB6 (`SistemaCombate.bas:170`), la fórmula reza:
     $$lTemp = \left( \text{Tacticas} + \frac{\text{Tacticas}}{33} \times \text{Agilidad} \right) \times \text{ModClase.Evasion}$$
   - En VB6 el operador `/` es división de punto flotante de doble precisión (`Double`). Una división entera en C++ truncaría `Tacticas / 33` a `0` para cualquier habilidad inferior a 33 puntos, anulando por completo el aporte de la agilidad en personajes de nivel inicial y medio. Se implementa con literal de punto flotante `33.0`:
     ```cpp
     const double lTemp = (skill_tacticas + (skill_tacticas / 33.0) * agilidad) * mod_evasion;
     ```
2. **Uso de Enteros con Signo (`std::int32_t`)**:
   - Todas las deducciones de daño, cotas de absorción y penalizaciones de reputación operan con tipos enteros con signo (`std::int32_t` y `std::int16_t`) con clampleo inferior explícito a `0` para garantizar ausencia absoluta de subflujos aritméticos (*underflow*).

---

### 4. Inventario de Callbacks y Hooks de Desacoplamiento

Para mantener la independencia de compilación de Capa 7 respecto a módulos de capas superiores, se expone el siguiente catálogo de hooks configurables:

| Hook | Firma | Propósito y Módulo Destino |
| :--- | :--- | :--- |
| `QuitarObjetosHook` | `void(int16_t obj, int32_t cant, int16_t user)` | Consumo de la Espada Mata Dragones (`Trabajo.bas` #24 / `InvUsuario.bas` #19). |
| `DoApuñalarHook` | `void(int16_t user, int16_t v_user, int16_t v_npc, int32_t daño)` | Cálculo de daño crítico por apuñalamiento (`Trabajo.bas` #24). |
| `DoGolpeCriticoHook` | `void(int16_t user, int16_t v_user, int16_t v_npc, int32_t daño)` | Probabilidad de golpe crítico de clases guerreras (`Trabajo.bas` #24). |
| `DoAcuchillarHook` | `void(int16_t user, int16_t v_user, int16_t v_npc, int32_t daño)` | Especialidad de acuchillado para piratas (`Trabajo.bas` #24). |
| `UserDieHook` | `void(int16_t user_index)` | Procesamiento de defunción de usuarios (`Modulo_UsUaRiOs.bas` #34). |
| `MuereNpcHook` | `void(int16_t npc_index, int16_t user_index)` | Muerte de NPC, drops y reaparición (`MODULO_NPCs.bas` #28). |
| `SubirSkillHook` | `void(int16_t user, eSkill skill, bool exito)` | Aumento natural de habilidades en combate (`Modulo_UsUaRiOs.bas` #34). |
| `CheckUserLevelHook` | `void(int16_t user_index)` | Comprobación de subida de nivel por experiencia (`Modulo_UsUaRiOs.bas` #34). |
| `PartyExpHook` | `void(int16_t user, int32_t exp, int16_t map, int16_t x, int16_t y)` | Reparto de experiencia grupal (`clsParty` / `mdParty` #32). |
| `RefreshCharStatusHook` | `void(int16_t user_index)` | Actualización de tag de criminalidad / estado visual (`Modulo_UsUaRiOs.bas` #34). |
| `VolverCriminalHook` | `void(int16_t user_index)` | Transición de estado ciudadano a criminal (`Modulo_UsUaRiOs.bas` #34). |
| `CancelExitHook` | `void(int16_t user_index)` | Cancelación de cuenta regresiva de `/SALIR` (`Modulo_UsUaRiOs.bas` #34). |
| `StoreFragHook` | `void(int16_t killer, int16_t victim)` | Registro estadístico de frags PvP (`Modulo_UsUaRiOs.bas` #34). |
| `ContarMuerteHook` | `void(int16_t victim, int16_t killer)` | Registro de muertes del personaje (`Modulo_UsUaRiOs.bas` #34). |

---

## Cobertura de Pruebas Unitarias

La suite de pruebas en `tests/test_sistemacombate.cpp` verifica exhaustivamente cada una de las 4 fases del módulo mediante **26 casos de prueba unitaria** y **162 aserciones**:

1. **Fase 1 (G1 — Evasión, Poder Ofensivo y Utilidades)**:
   - `MinimoInt` y `MaximoInt` en extremos y signos mixtos.
   - Fórmulas de evasión con escudo y corporal, verificando tramos por habilidad y el bono de nivel.
   - Fórmulas de ataque cuerpo a cuerpo, a distancia y combate desarmado con guantes.
   - Restricciones de munición (arcos requieren flechas), distancias máximas (`MAXDISTANCIAARCO = 18`) y afinidad de clan/party.
   - Evaluación de triggers de zona de combate (`TriggerZonaPelea` con `eTrigger6`).
   - Cooldown físico determinista mediante `CombatTimeProvider`.
2. **Fase 2 (G2 — Daño Bruto y Acierto RNG)**:
   - Resolución de probabilidades de impacto (`ProbExito` clampleado entre 10% y 90%).
   - Impactos exitosos y fallidos en todos los emparejamientos con inyección determinista de `CombatRandomProvider`.
   - Cálculo de daño bruto físico en combate desarmado, con armas melee y proyectiles.
   - Verificación estricta de la replicación del **Bug #37** (omisión de `proyectil.MaxHIT` en `daño_max_arma`).
3. **Fase 3 (G3 — Deducción de Daño, Absorciones y Peculiaridades)**:
   - Verificación del **Criterio 7.1** (Espada Mata Dragones: letalidad instantánea y rotura contra dragón, daño 1 contra otros blancos).
   - Verificación del **Criterio 7.2** (Apuñalamiento: asimetría daño bruto en PvE vs daño neto en PvP).
   - Absorción anatómica: casco en cabeza vs armadura y escudo en torso.
   - Clamping de absorción excesiva (daño cero, no cura).
   - Cálculo proporcional de experiencia (`CalcularDarExp`) y mitigación de criminalidad (`RestarCriminalidad`).
   - Envenenamiento pasivo (`UserEnvenena`).
4. **Fase 4 (G4 — Orquestación de Ataque, Protocolo y Mascotas)**:
   - `UsuarioAtaca`: cooldowns, consumo de stamina por género y ruteo a celda frontal.
   - Verificación del **Criterio 7.4** (Sigilo de GM invisible: audio `SND_SWING` unicast privado).
   - Verificación del **Criterio 7.3** (Legítima defensa: bypass total de karma, frags, muertes penalizadas y criminalidad).
   - Ruptura de meditación y cancelación de salida pendiente (`CancelExitHook`).
   - Matriz legal de combate `PuedeAtacar` (seguro, zonas no-PK, Trigger 6 y facciones).
   - Reglas de facción en `PuedeAtacarNPC` (Guardias Reales y del Caos).
   - Gestión de mascotas: `MuereNpc`, `RestarCriaturasEntrenador`, `CheckPets`, `AllFollowAmo`, `AllMascotasAtacanUser`.
