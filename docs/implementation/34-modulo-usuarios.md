---
area: entidad-usuario-y-estado
source_files:
  - legacy/server/Codigo/Modulo_UsUaRiOs.bas
tags: [usuarios, userlist, sesion, nivel, exp, warp, casper, navegacion, mascotas, quirks]
last_updated: 2026-09-19
---

# Especificación Técnica C++: Módulo #34 (`Modulo_UsUaRiOs.bas`)

Este documento formaliza la especificación técnica definitiva para la transliteración del **Módulo #34 (`Modulo_UsUaRiOs.bas`)** a C++20, habiendo alcanzado el estado **Completado (Aislado / Hooks en Capas 6, 7 y 8)** con cobertura de pruebas del 100% sobre sus 57 rutinas y la réplica estricta de sus bugs históricos.

---

## 1. Estado y Ubicación en la Arquitectura

- **Estado**: **Completado (Aislado / Hooks en Capas 6, 7 y 8)**
- **Archivos de Producción**:
  - [`src/server/Modulo_UsUaRiOs.hpp`](../../src/server/Modulo_UsUaRiOs.hpp)
  - [`src/server/Modulo_UsUaRiOs.cpp`](../../src/server/Modulo_UsUaRiOs.cpp)
- **Suite de Pruebas**:
  - [`tests/test_modulo_usuarios.cpp`](../../tests/test_modulo_usuarios.cpp)

---

## 2. Cobertura Funcional por Fases (57 Rutinas)

La implementación se organizó en 5 fases lógicas completadas exhaustivamente:

### Fase G1: Ciclo de Vida y Gestión de Ranuras (5 Rutinas)
- `NextOpenUser()`: Busca slot desocupado delegando en `TCP::NextOpenUser()`.
- `NextOpenCharIndex()`: Busca ranura libre en `CharList` (1 a `MAXCHARS`).
- `Cerrar_Usuario(user_index)`: Ejecuta desconexión mediante `TCP::CerrarUsuario()`.
- `CancelExit(user_index)`: Cancela temporizador `/salir` (Replica Bug #52 si `!ConnIDValida`).
- `CambiarNick(user_index, target_user_index, new_nick)`: Modifica el nombre del personaje.

### Fase G2: Cinemática, Transporte Espacial y Mascotas (14 Rutinas)
- `MoveUserChar(user_index, heading)`: Desplazamiento grilla a grilla, permuta con caspers e interacción con Admin invisible (Replica Bug #53).
- `WarpUserChar(user_index, map, x, y, fx, teletransported)`: Teletransporte espacial, actualización de grilla y densidad poblacional (Replica Bug #51 en `NumUsers < 0 => 0`).
- `Tilelibre(pos, n_pos, obj, agua, tierra)`: Búsqueda de celda adyacente libre.
- `PuedeAtravesarAgua(user_index)`: Chequeo de tránsito sobre agua (navegando o muerto).
- `ToogleBoatBody(user_index)`: Alterna apariencia entre barco y terrestre.
- `BodyIsBoat(body)`: Evalúa si un sprite de cuerpo corresponde a embarcación.
- `ChangeUserChar(...)`: Actualización de equipamiento visual y notificación al área.
- `MakeUserChar(...)` y `EraseUserChar(...)`: Control de visibilidad en grilla visual.
- `WarpMascotas(user_index)` y `WarpMascota(user_index, pet_index)`: Traslado iterativo de criaturas invocadas.
- `InvertHeading(heading)` y `GetDireccion(user_index, other_user_index)`: Orientación cardinal.
- `FarthestPet(user_index)`: Retorna la mascota más distante del amo.

### Fase G3: Progresión de Nivel e Inspección (13 Rutinas)
- `CheckUserLevel(user_index)`: Cálculo de subida de nivel, atributos aleatorios, expulsión de clan faccionario a nivel 25 y **Bug #50** (lanza `ELUOverflowException` si `next_elu > INT32_MAX`).
- `ActStats(victim_index, attacker_index)`: Otorgamiento de experiencia por combate.
- `SubirSkill(user_index, skill, acerto)` y `CheckEluSkill(...)`: Asignación y progresión de habilidades.
- `EnviarFama(user_index)`: Envío de reputaciones al cliente.
- Rutinas de inspección online/offline: `SendUserStatsTxt`, `SendUserMiniStatsTxt`, `SendUserSkillsTxt`, `SendUserInvTxt`, `SendUserMiniStatsTxtFromChar`, `SendUserStatsTxtOFF`, `SendUserOROTxtFromChar` y `SendUserInvTxtFromChar` (desacopladas vía `CharReaderHook`).

### Fase G4: Muerte, Resurrección y Máquina de Estados (10 Rutinas)
- `UserDie(user_index)`: Proceso de deceso: adopta `iFragataFantasmal` en agua o `iCuerpoMuerto` en tierra (`OrigChar` intacto), tirado de objetos/oro, purga de estados alterados, seguro `SeguroResu = true` y penalización a la party.
- `RevivirUsuario(user_index)`: Restablece sprite original o embarcación, fija `Stats.MinHp = 1` y marca `flags.Muerto = 0`.
- `ContarMuerte(muerto_index, atacante_index)`: Conteo de frags respetando legítima defensa.
- `SetInvisible(user_index, user_char_index, invisible)` y `SetConsulatMode(user_index)`: Modos de visibilidad y consulta GM.
- `IsArena(user_index)`: Consulta si la posición actual está en zona de coliseo/pelea.
- `VolverCriminal(user_index)` y `VolverCiudadano(user_index)`: Mutación de alineación faccionaria y expulsión de facción opuesta.
- `RefreshCharStatus(user_index)`: Sincronización visual de nick, tag de clan y color faccionario.
- `EsMascotaCiudadano(npc_index, user_index)`: Validación de propiedad de criaturas ciudadanas.

### Fase G5: Reglas de Combate e Inventario (15 Rutinas)
- `ToogleToAtackable(user_index, owner_index, stealing_npc)`: Activa legítima defensa (`flags.AtacablePor`) por agresión.
- `setHome(user_index, new_home, npc_index)` y `goHome(user_index)`: Configuración y retorno a ciudad natal.
- `PerdioNpc(user_index)` y `ApropioNpc(user_index, npc_index)`: Vinculación y desvinculación de criaturas.
- `SameFaccion(user_index, other_user_index)`: Chequeo de alineación faccionaria idéntica.
- `NPCAtacado(npc_index, user_index)`: Sanciones al agredir guardias o criaturas aliadas.
- `PuedeApuñalar(user_index)` y `PuedeAcuchillar(user_index)`: Comprobación de clase (`Assasin`/`Pirat`), arma equipada y habilidad $\ge 10$.
- `GetWeaponAnim(user_index, obj_index)`: Animación de arma.
- `GetNickColor(user_index)`: Retorna color del nick (GM=0, Armada=1, Caos=2, Criminal=3, Ciudadano=4).
- `ChangeUserInv(...)`: Actualización de slot de inventario.
- `HasEnoughItems(...)` y `TotalOfferItems(...)`: Conteo acumulativo en inventario disperso.
- `getMaxInventorySlots(user_index)`: Retorna 30 con `MochilaEqpSlot > 0`, 20 en caso contrario.

---

## 3. Registro de Bugs y Quirks Históricos Replicados (#50 a #53)

1. **Bug #50 (`CheckUserLevel` / Excepción Tipada de ELU)**:
   - Se promueve el cálculo a `std::int64_t`. Si excede `INT32_MAX`, lanza `Modulo_UsUaRiOs::ELUOverflowException`. Preserva el Error 6 de VB6 traducido a una excepción segura de C++.
2. **Bug #51 (`WarpUserChar` / Clamping de `NumUsers`)**:
   - Preserva la guarda defensiva `MapInfoList[old_map].NumUsers--` con clamp `NumUsers < 0 => 0`.
3. **Bug #52 (`CancelExit` / Retención de Slot)**:
   - Reasigna el temporizador de salida si `!ConnIDValida` evitando la liberación inmediata del slot.
4. **Bug #53 (`MoveUserChar` / Admin Invisible sobre Casper)**:
   - Prohíbe la transmisión de paquetes de movimiento cuando un Admin invisible camina sobre un fantasma.

---

## 4. Verificación y Calidad de Código

- **Compilación C++20**: Exitosa con CMake/Ninja sin advertencias en `clang-tidy`.
- **Suite doctest**: 438 test cases pasados / 5.964 aserciones exitosas.
