# Módulo #23: modHechizos.bas (Capa 7 — Magia y Hechizos)

## 1. Estado del Módulo
**Completado (Aislado / Cableado Pendiente en Capas 7, 8, 9 y 11)**.

El subsistema de magia y hechizos fue portado integralmente a C++20 (src/server/modHechizos.hpp y src/server/modHechizos.cpp) implementando el 100% de los 22 procedimientos originales de legacy/server/Codigo/modHechizos.bas. La lógica de dominio se encuentra completamente aislada y desacoplada de la red y del ciclo de vida del servidor mediante la estructura de callbacks tipados SpellsCallbacks.

---

## 2. Resumen y Alcance

El subsistema administra todas las mecánicas mágicas del servidor de Argentum Online:
1. **Gestión del Libro de Hechizos**: Aprendizaje (AgregarHechizo), verificación de posesión (TieneHechizo), reordenamiento manual de slots (ChangeUserHechizo, DesplazarHechizo) y sincronización con el cliente (UpdateUserHechizos).
2. **Ciclo de Casteo y Palabras Mágicas**: Pronunciación pública de palabras mágicas con desocultamiento (DecirPalabrasMagicas), validación preliminar de requisitos de báculo, maná, estamina y skill (PuedeLanzar), y orquestación por objetivo (LanzarHechizo).
3. **Efectos Cuantitativos y Modificación de Atributos**: Fórmulas canónicas de daño mágico y curación escaladas por nivel, bonificaciones de báculos y laúdes, absorción antimagia (cascos y anillos), daño sobre criaturas con reparto de experiencia y alteración temporal de atributos primarios (HechizoPropUsuario, HechizoPropNPC).
4. **Estados Alterados, Metamorfosis e Invocaciones**: Control de envenenamiento, parálisis, inmovilización, ceguera, estupidez, invisibilidad, mimetismo físico y metamorfosis entre usuarios y criaturas; invocación de criaturas aliadas y transporte de mascotas vía Warp (HechizoEstadoUsuario, HechizoEstadoNPC, HechizoTerrenoEstado, HechizoInvocacion).
5. **Soporte Legal, Moralidad y Magia de NPCs**: Matriz de reglas legales para asistencia entre alineaciones (CanSupportUser), penalización de nobleza y aumento de bandido (DisNobAuBan), y lanzamiento de conjuros por parte de NPCs hostiles o entrenados (NpcLanzaSpellSobreUser, NpcLanzaSpellSobreNpc).

---

## 3. Decisiones de Diseño y Paridad Estricta

### 3.1. Replicación de Bugs Históricos (Master Bug Ledger)

- **Bug #38 — Omisión de RandomNumber en Maná y Estamina (HechizoPropUsuario)**:
  - *Comportamiento*: En VB6, las ramas SubeMana y SubeSta mutan los atributos usando la variable local daño sin invocar RandomNumber(MinHp, MaxHp).
  - *Efecto replicado*: Si el conjuro afecta exclusivamente maná o energía, daño ingresa con valor 0 y produce una mutación de 0 puntos. Si el conjuro modifica previamente HP o Fuerza, daño arrastra ese residuo numérico y lo traslada a la reserva de maná o estamina.
- **Bug #39 — Muerte Asimétrica en Resurrección (HechizoEstadoUsuario)**:
  - *Comportamiento*: Al resucitar a un objetivo, el coste vital puede reducir la vida del lanzador a 0 o menos. El código ejecuta UserDie(UserIndex) y marca HechizoCasteado = False, pero **omite la instrucción de salida** (Exit Sub).
  - *Efecto replicado*: La resurrección continúa su flujo normal y ejecuta RevivirUsuario(TargetIndex), resultando en que el objetivo revive exitosamente a pesar de que el lanzador perece en el acto.
- **Bug #40 — Colisión Ceguera vs Estupidez (HechizoEstadoUsuario)**:
  - *Comportamiento*: Ambos estados alterados comparten y sobreescriben la misma variable de contador .Counters.Ceguera (IntervaloParalizado / 3 en ceguera vs IntervaloParalizado en estupidez).
  - *Efecto replicado*: Si un usuario recibe ceguera y luego estupidez (o viceversa), el contador de duración del efecto previo queda completamente sobrescrito por el segundo.

### 3.2. Quirks de Dominio Preservados

- **Quirk 7.1 — Mensaje Invertido de Daño en Curación de Criaturas (NpcLanzaSpellSobreUser)**:
  - Cuando un NPC ejecuta un hechizo de curación (SubeHP == 1) sobre un usuario, la vida se incrementa efectivamente pero el mensaje emitido por consola de pelea dice: {npc.name} te ha quitado {daño} puntos de vida..
- **Quirk 7.2 — Temporizador Anómalo de Estupidez en Criaturas (NpcLanzaSpellSobreUser)**:
  - A diferencia del casteo entre usuarios (que usa IntervaloParalizado), cuando un NPC aplica estupidez a un personaje, el temporizador asignado es .Counters.Ceguera = IntervaloInvisible.
- **Quirk 7.3 — Validación Unidimensional en Eje Y (LanzarHechizo)**:
  - La distancia entre el casteador y el objetivo se evalúa exclusivamente sobre el eje vertical: std::abs(target_pos.Y - user.Pos.Y) <= RANGO_VISION_Y (con RANGO_VISION_Y = 6). Cualquier separación horizontal en el eje X es completamente ignorada por el motor.
- **Quirk 7.4 — Bonificaciones del Druida con Flauta Élfica**:
  - Descuento del 50% de maná en mimetismo (Mimetiza == 1).
  - Descuento del 30% de maná en invocaciones regulares (Tipo == uInvocacion).
  - Descuento del 10% de maná en magia general, **con excepción expresa de Apocalipsis** (APOCALIPSIS_SPELL_INDEX = 25).
  - Activación del flag .flags.Ignorado = true al mimetizarse con criaturas para evitar agresiones de NPCs.
- **Quirk 7.5 — Modificadores Canónicos de Báculo y Laúd**:
  - Mago con conjuro StaffAffected: penalización de 0.7x desarmado o multiplicador de daño con báculo equipado ((StaffBonus + 70) / 100).
  - Bardo con LAUDELFICO o FLAUTAELFICA: bonificador multiplicativo fijo de 1.04x sobre el daño mágico final.

---

## 4. Matriz de Soporte Legal y Moralidad

El procedimiento CanSupportUser(CasterIndex, TargetIndex, DoCriminal) formaliza las restricciones de ayuda e interferencia mágica:

| Casteador | Objetivo | Condiciones / Estado | Resultado | Penalización |
| :--- | :--- | :--- | :--- | :--- |
| Cualquiera | Mismo Usuario | Auto-casteo | **Permitido** | Ninguna |
| Cualquiera | Cualquiera | En Consulta (lags.EnConsulta) | **Bloqueado** | Mensaje informativo |
| Cualquiera | Cualquiera | Zona de Pelea / Arena (Trigger 6) | **Permitido** | Ninguna |
| Armada Real | Criminal | Siempre | **Bloqueado** | Mensaje informativo |
| Ciudadano | Criminal | Seguro Puesto (lags.Seguro == true) | **Bloqueado** | Mensaje informativo |
| Ciudadano | Criminal | Sin Seguro (DoCriminal == true) | **Permitido** | VolverCriminal(CasterIndex) |
| Ciudadano | Criminal | Sin Seguro (DoCriminal == false) | **Permitido** | DisNobAuBan (-50% Nobleza, +10.000 Bandido) |
| Legión Caos | Ciudadano | Siempre | **Bloqueado** | Mensaje informativo |
| Armada Real | Ciudadano | Objetivo Atacable por Tercero | **Bloqueado** | Mensaje informativo |
| Ciudadano | Ciudadano | Atacable por Tercero con Seguro | **Bloqueado** | Mensaje informativo |
| Ciudadano | Ciudadano | Atacable por Tercero sin Seguro | **Permitido** | DisNobAuBan (-50% Nobleza, +10.000 Bandido) |
| Ciudadano | Ciudadano | Atacable por el propio Casteador | **Permitido** | Ninguna (legítima defensa) |

### Procedimiento DisNobAuBan
- Inmune en zonas de arena o pelea (Trigger 6).
- Deduce 
oble_pts de .Reputacion.NobleRep (suelo en 0).
- Incrementa andido_pts en .Reputacion.BandidoRep (cap en MAXREP = 500000).
- Emite mensaje de red eMessages::NobilityLost.
- Si el usuario cae en criminalidad y pertenecía a la Armada Real (Faccion.ArmadaReal == 1), invoca de forma inmediata ExpulsarFaccionReal(UserIndex).
- Si el estado de criminalidad mutó, invoca RefreshCharStatus(UserIndex).

---

## 5. Catálogo de Inyección de Dependencias (SpellsCallbacks)

Para garantizar el desacoplamiento total en Capa 7, el módulo utiliza la siguiente estructura de hooks inyectables mediante SetSpellsCallbacks():

`cpp
struct SpellsCallbacks {
    // Ciclo Vital y Efectos Fatales
    std::function<void(std::int16_t user_index)> UserDie;
    std::function<void(std::int16_t target_index)> RevivirUsuario;
    std::function<void(std::int16_t npc_index, std::int16_t user_index)> MuereNpc;

    // Combate, Validación y Retaliación
    std::function<bool(std::int16_t user_index, std::int16_t target_index)> PuedeAtacar;
    std::function<bool(std::int16_t user_index, std::int16_t npc_index, bool paralisis)> PuedeAtacarNPC;
    std::function<void(std::int16_t attacker_index, std::int16_t victim_index)> UsuarioAtacadoPorUsuario;
    std::function<void(std::int16_t npc_index, std::int16_t user_index)> NpcAtacado;
    std::function<std::int16_t(std::int16_t user_index, std::int16_t target_index)> TriggerZonaPelea;
    std::function<void(std::int16_t user_index, std::int16_t npc_index, std::int32_t damage)> CalcularDarExp;

    // Facciones, Moralidad y Reputación
    std::function<bool(std::int16_t user_index)> Criminal;
    std::function<bool(std::int16_t user_index)> EsArmada;
    std::function<bool(std::int16_t user_index)> EsCaos;
    std::function<void(std::int16_t user_index)> VolverCriminal;
    std::function<void(std::int16_t user_index)> RestarCriminalidad;
    std::function<void(std::int16_t user_index)> ExpulsarFaccionReal;
    std::function<void(std::int16_t user_index)> RefreshCharStatus;

    // Criaturas y Mascotas
    std::function<std::int16_t(std::int16_t npc_num, const WorldPos& pos, bool backup, bool respawn)> SpawnNpc;
    std::function<void(std::int16_t npc_index)> FollowAmo;
    std::function<void(std::int16_t user_index, std::int16_t pet_index)> WarpMascota;
    std::function<std::int16_t(std::int16_t user_index)> FarthestPet;
    std::function<std::int16_t(std::int16_t user_index)> FreeMascotaIndex;

    // Inventario y Progresión
    std::function<void(std::int16_t user_index, std::uint8_t slot, std::int16_t amount)> QuitarUserInvItem;
    std::function<void(std::int16_t user_index, eSkill skill, bool sube)> SubirSkill;

    // Notificaciones, Apariencia y Estadísticas
    std::function<void(std::int16_t user_index, std::int16_t target_index)> StoreFrag;
    std::function<void(std::int16_t victim_index, std::int16_t killer_index)> ContarMuerte;
    std::function<void(std::int16_t victim_index, std::int16_t killer_index)> ActStats;
    std::function<void(std::int16_t user_index, std::int16_t char_index, bool invisible)> SetInvisible;
    std::function<void(std::int16_t user_index, std::int16_t body, std::int16_t head, std::uint8_t heading, std::int16_t weapon, std::int16_t shield, std::int16_t helmet)> ChangeUserChar;
    std::function<std::int16_t(std::int16_t user_index, std::int16_t weapon_obj_index)> GetWeaponAnim;

    // Soporte Legal y Penalizaciones Morales
    std::function<bool(std::int16_t caster_index, std::int16_t target_index, bool do_criminal)> CanSupportUser;
    std::function<void(std::int16_t user_index, double exp_restada, std::int32_t puntos_bandido)> DisNobAuBan;
};
`

---

## 6. Métricas de Cobertura y Verificación

La suite de pruebas en 	ests/test_modhechizos.cpp cuenta con **25 casos de prueba** divididos en las 4 fases modulares:

1. **Fase 1 (G1 — Libro y Utilidades, 6 tests)**:
   - TieneHechizo: consulta determinista en libro de 35 slots.
   - AgregarHechizo: slot libre, consumo de ítem y rechazo de duplicados.
   - DesplazarHechizo: desplazamiento arriba/abajo y límites en bordes 1 y 35.
   - ChangeUserHechizo y UpdateUserHechizos: actualización selectiva y total.
   - PuedeLanzar: chequeos de maná, estamina, báculo de mago y bonos de druida.
   - DecirPalabrasMagicas: desocultamiento de personajes no administradores.
2. **Fase 2 (G2 — Efectos Cuantitativos y Modificación de Atributos, 6 tests)**:
   - HechizoPropUsuario: curación canónica con escalado por nivel.
   - HechizoPropUsuario: daño mágico, mitigación antimagia de casco/anillo, penalización desarmado (0.7x), báculo y bardo (1.04x).
   - HechizoPropUsuario: **Replicación Bug #38** (omisión de RandomNumber en maná/estamina).
   - HechizoPropUsuario: modificación temporal de Fuerza y Agilidad con caps canónicos.
   - HechizoPropNPC: daño mágico, defensa mágica del NPC y reparto de experiencia.
   - HechizoPropNPC: muerte del NPC otorgando crédito al usuario.
3. **Fase 3 (G3 — Estados Alterados, Invocaciones y Metamorfosis, 7 tests)**:
   - HechizoEstadoUsuario: envenenamiento y curación de veneno.
   - HechizoEstadoUsuario: parálisis e inmovilización con inmunidad de SUPERANILLO.
   - HechizoEstadoUsuario: ceguera y estupidez con **Replicación Bug #40**.
   - HechizoEstadoUsuario: invisibilidad y mimetismo físico entre personajes.
   - HechizoEstadoUsuario: **Replicación Bug #39** (muerte del lanzador en resurrección).
   - HechizoEstadoNPC: metamorfosis exclusiva de Druida con criatura y flag ignorado.
   - HechizoInvocacion y HechizoTerrenoEstado: acotado a MAXMASCOTAS y desinvisibilización en área 17x17.
4. **Fase 4 (G4 — Orquestación, Moralidad y Magia de NPCs, 6 tests)**:
   - CanSupportUser: matriz completa moral (auto-soporte, arena, consulta, criminales, caóticos, armadas, atacables).
   - DisNobAuBan: reducción de nobleza, suma de bandido y expulsión de Armada Real al caer en criminalidad.
   - LanzarHechizo: **Quirk 7.3** (validación unidimensional en eje Y) y ruteo a usuario, criatura y terreno.
   - HandleHechizo*: consumo de maná, vaciado total en Warp y descuentos de Druida (**Quirk 7.4**).
   - NpcLanzaSpellSobreUser: **Quirk 7.1** (mensaje invertido en cura), **Quirk 7.2** (timer invisible en estupidez) y mitigaciones.
   - NpcLanzaSpellSobreNpc: combate entre criaturas y muerte asociada al amo atacante.

**Resultado Global de la Suite**: 285/285 tests pasados al 100% (5.177 aserciones).
