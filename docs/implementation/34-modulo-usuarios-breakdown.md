---
area: entidad-usuario-y-estado
source_files:
  - legacy/server/Codigo/Modulo_UsUaRiOs.bas
tags: [plan, usuarios, userlist, sesion, nivel, exp, warp, casper, navegacion, mascotas, quirks]
last_updated: 2026-09-18
---

# Plan de Implementación C++: Módulo #34 (`Modulo_UsUaRiOs.bas`)

Este documento define el desglose técnico formal para la transliteración del **Módulo #34 (`Modulo_UsUaRiOs.bas`)** a C++17/C++20, estructurado en 5 fases lógicas de implementación (G1 a G5) de acuerdo con [`00-port-plan.md`](00-port-plan.md) y las normas de paridad comportamental de [`../CONVENTIONS.md`](../CONVENTIONS.md).

Para el informe de auditoría previa y análisis del módulo legacy en VB6, consultá [`../audit/15d-usuarios-detalle.md`](../audit/15d-usuarios-detalle.md). El registro de bugs replicados se encuentra en [`KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md).

---

## 1. Alcance Arquitectónico y Estructura del Módulo C++

El módulo se implementará respetando estrictamente la convención canónica de nombres (`Modulo_UsUaRiOs`) dentro del namespace `Modulo_UsUaRiOs` y en los archivos del servidor C++:

```
src/server/
├── Modulo_UsUaRiOs.hpp   # Declaración de funciones, excepciones tipadas y hooks de la API de usuarios
└── Modulo_UsUaRiOs.cpp   # Implementación de lógica espacial, nivel, sesión y estados de la entidad usuario
```

### Integración y Coordinación con Módulos Preexistentes
- **`TCP` (`TCP.hpp`)**: Coordinación de ranuras y sesiones (`TCP::NextOpenUser`, `TCP::CerrarUsuario`). `Modulo_UsUaRiOs` delega la validación de sockets y la terminación de slots en `TCP`.
- **`UserList` / `User` (`Declares.hpp`)**: Acceso y mutación directa sobre el vector global `UserList`.
- **`MapData` / `MapInfo` (`Declares.hpp`)**: Lectura y actualización de grilla espacial y densidad poblacional por mapa.
- **`Protocol` (`Protocol.hpp`)**: Emisión de paquetes binarios serializados (`WriteConsoleMsg`, `WritePosUpdate`, `WriteForceCharMove`, etc.).
- **`ModAreas` (`ModAreas.hpp`)**: Notificaciones de actualización de área visual (`CheckUpdateNeededUser`).
- **`mdParty` (`mdParty.hpp`)**: Actualización de niveles y penalizaciones por muerte en party.
- **`modGuilds` (`modGuilds.hpp`)**: Expulsión automática de clanes faccionarios al alcanzar nivel 25.

---

## 2. Estrategia de Preservación de Bugs y Reglas de Dominio

### 2.1 Preservación de Bugs Históricos (#50 a #53)

1. **Bug #50 (Desbordamiento Aritmético por Excepción Tipada en `CheckUserLevel`)**:
   - *Estrategia C++*: Se promueve el cálculo de `.Stats.ELU` a `std::int64_t`. Si el nuevo ELU excede el límite superior de entero de 32 bits con signo (`INT32_MAX`), se lanza la excepción tipada `Modulo_UsUaRiOs::ELUOverflowException`:
     ```cpp
     std::int64_t next_elu = static_cast<std::int64_t>(user.Stats.ELU * factor);
     if (next_elu > std::numeric_limits<std::int32_t>::max()) {
         throw ELUOverflowException("Overflow en calculo de ELU para usuario: " + std::to_string(user_index));
     }
     user.Stats.ELU = static_cast<std::int32_t>(next_elu);
     ```
   - *Justificación*: Preserva exactamente el comportamiento del Error 6 ("Overflow") de VB6 traduciéndolo a una excepción segura de C++, jamás aplicando clamping ni mutando silenciosamente las reglas de juego.

2. **Bug #51 (Underflow Poblacional `NumUsers` en `WarpUserChar`)**:
   - *Estrategia C++*: Preservar la guarda defensiva explícita:
     ```cpp
     MapInfo[old_map].NumUsers--;
     if (MapInfo[old_map].NumUsers < 0) {
         MapInfo[old_map].NumUsers = 0;
     }
     ```

3. **Bug #52 (Retención Anómala de Slot en `CancelExit`)**:
   - *Estrategia C++*: Preservar la rama `Else` en `CancelExit`:
     ```cpp
     if (!user.ConnIDValida) {
         user.Counters.Salir = (is_pk_map) ? IntervaloCerrarConexion : 0;
     }
     ```

4. **Bug #53 (Desincronización de Admin Invisible sobre Casper)**:
   - *Estrategia C++*: Saltear el despacho de `PrepareMessageCharacterMove` si el ejecutor es `AdminInvisible == 1` y la celda destino está ocupada por un fantasma.

### 2.2 Preservación de Reglas de Dominio

1. **Expulsión a Nivel 25 de Clanes Faccionarios**: En `CheckUserLevel`, si `ELV == 25` y `GuildIndex > 0` con alineación faccionaria ("Real" o "Del Mal"), invocar `modGuilds::m_EcharMiembroDeClan(-1, name)`.
2. **Fragata Fantasmal al Morir Navegando**: En `UserDie`, si `flags.Navegando == 1`, asignar `Char.body = iFragataFantasmal` en lugar de `iCuerpoMuerto`.
3. **Intercambio Espacial con Caspers**: En `MoveUserChar`, al caminar sobre un casper, relocalizar al casper a la celda que acaba de dejar libre el jugador vivo e invertir la dirección del casper con `InvertHeading`.
4. **Activación de Seguro de Resurrección (`SeguroResu`)**: Asignar `flags.SeguroResu = true` al morir o salir de un mapa de coliseo/arena.
5. **Penalización a la Party por Muerte**: En `UserDie`, invocar `mdParty::ObtenerExito(UserIndex, user.Stats.ELV * -10 * cant_miembros)`.

---

## 3. Plan de Desglose Exhaustivo por Fases (57 Rutinas)

El plan distribuye la totalidad de las 57 rutinas del catálogo de auditoría en 5 fases lógicas:

### Fase 1 (G1): Ciclo de Vida, Coordinación con `TCP.bas` y Gestión de Ranuras (5 Rutinas)
- **Firmas a Implementar**:
  ```cpp
  namespace Modulo_UsUaRiOs {
      int NextOpenUser();
      int NextOpenCharIndex();
      void Cerrar_Usuario(int user_index);
      void CancelExit(int user_index);
      void CambiarNick(int user_index, int target_user_index, const std::string& new_nick);
  }
  ```
- **Coordinación Arquitectónica con `TCP.bas`**: `NextOpenUser` y `Cerrar_Usuario` delegan directamente en las primitivas de infraestructura `TCP::NextOpenUser` y `TCP::CerrarUsuario`, manteniendo sincronizado el estado del socket con la ranura en `UserList`.
- **Pruebas doctest (`tests/test_modulo_usuarios.cpp`)**:
  - `G1_NextOpenUser_FindsFirstEmptySlot`: Verifica asignación de slot desocupado.
  - `G1_CerrarUsuario_SetsSaliendoCounter`: Comprueba temporizador de 10 segundos en zonas de combate.
  - `G1_CancelExit_ResetsCounterWhenValidConn`: Reestablece temporizador ante cancelación activa.
  - `G1_CancelExit_RetainsSlotWhenConnInvalid_Bug52`: Replica la retención del Bug #52.

### Fase 2 (G2): Cinemática, Transporte Espacial y Mascotas (14 Rutinas)
- **Firmas a Implementar**:
  ```cpp
  namespace Modulo_UsUaRiOs {
      void MoveUserChar(int user_index, eHeading heading);
      void WarpUserChar(int user_index, int map, int x, int y, bool fx, bool teletransported = false);
      void Tilelibre(const WorldPos& pos, WorldPos& n_pos, Obj& obj, bool& agua, bool& tierra);
      bool PuedeAtravesarAgua(int user_index);
      void ToogleBoatBody(int user_index);
      bool BodyIsBoat(int body);
      void ChangeUserChar(int user_index, int body, int head, uint8_t heading, int weapon, int shield, int helmet);
      void MakeUserChar(bool to_map, int snd_index, int user_index, int map, int x, int y);
      void EraseUserChar(int user_index, bool is_admin_invisible);
      void WarpMascotas(int user_index);
      void WarpMascota(int user_index, int pet_index);
      eHeading InvertHeading(eHeading heading);
      std::string GetDireccion(int user_index, int other_user_index);
      int FarthestPet(int user_index);
  }
  ```
- **Pruebas doctest**:
  - `G2_MoveUserChar_SwapsPositionWithCasper`: Verifica permuta de celdas al pisar un fantasma.
  - `G2_MoveUserChar_AdminInvisibleDesync_Bug53`: Replica desincronización de paquetes del Bug #53.
  - `G2_WarpUserChar_NumUsersClamping_Bug51`: Revisa el clamp de `NumUsers < 0 => 0` (Bug #51).
  - `G2_WarpMascotas_RelocatesAllPetsOnMapChange`: Confirma el traslado iterativo de criaturas invocadas.

### Fase 3 (G3): Progresión de Nivel, Experiencia e Inspección Administrativa (13 Rutinas)
- **Firmas a Implementar**:
  ```cpp
  namespace Modulo_UsUaRiOs {
      struct ELUOverflowException : public std::runtime_error {
          explicit ELUOverflowException(const std::string& msg) : std::runtime_error(msg) {}
      };

      using CharReaderHook = std::function<std::string(const std::string& char_name, const std::string& section, const std::string& key)>;

      void CheckUserLevel(int user_index);
      void ActStats(int victim_index, int attacker_index);
      void SubirSkill(int user_index, int skill, bool acerto);
      void CheckEluSkill(int user_index, uint8_t skill, bool allocation);
      void EnviarFama(int user_index);
      void SendUserStatsTxt(int send_index, int user_index);
      void SendUserMiniStatsTxt(int send_index, int user_index);
      void SendUserMiniStatsTxtFromChar(int send_index, const std::string& char_name, CharReaderHook reader = nullptr);
      void SendUserStatsTxtOFF(int send_index, const std::string& nombre, CharReaderHook reader = nullptr);
      void SendUserOROTxtFromChar(int send_index, const std::string& char_name, CharReaderHook reader = nullptr);
      void SendUserInvTxt(int send_index, int user_index);
      void SendUserInvTxtFromChar(int send_index, const std::string& char_name, CharReaderHook reader = nullptr);
      void SendUserSkillsTxt(int send_index, int user_index);
  }
  ```
- **Desacoplamiento de E/S**: Las rutinas de inspección de usuarios offline (`*FromChar` y `*OFF`) utilizan un hook inyectable `CharReaderHook` que desacopla el acceso a disco/INI facilitando pruebas unitarias aisladas sin requerir archivos `.chr` físicos.
- **Pruebas doctest**:
  - `G3_CheckUserLevel_ThrowsELUOverflowException_Bug50`: Lanza `ELUOverflowException` si `ELU > INT32_MAX` (Bug #50).
  - `G3_CheckUserLevel_ExpulsesFactionGuildAtLvl25`: Valida expulsión automática de clanes faccionarios al nivel 25.
  - `G3_SendUserStatsTxtOFF_DecoupledReader`: Verifica la lectura de estadísticas offline mediante `CharReaderHook`.

### Fase 4 (G4): Muerte, Resurrección y Máquina de Estados (10 Rutinas)
- **Firmas a Implementar**:
  ```cpp
  namespace Modulo_UsUaRiOs {
      void UserDie(int user_index);
      void RevivirUsuario(int user_index);
      void ContarMuerte(int muerto_index, int atacante_index);
      void SetInvisible(int user_index, int user_char_index, bool invisible);
      void SetConsulatMode(int user_index);
      bool IsArena(int user_index);
      void VolverCriminal(int user_index);
      void VolverCiudadano(int user_index);
      void RefreshCharStatus(int user_index);
      bool EsMascotaCiudadano(int npc_index, int user_index);
  }
  ```
- **Contrato de `RefreshCharStatus`**: Sincroniza la representación del sprite y el bando faccionario tras mutaciones en `InvUsuario` (equipo/armas) o `SistemaCombate`.
- **Pruebas doctest**:
  - `G4_UserDie_AdoptsGhostShipInWater`: Valida la transformación a `iFragataFantasmal` en agua.
  - `G4_UserDie_DropsInventoryAndActivatesResuSafe`: Comprueba desequipado masivo, vaciado a grilla y seguro de resurrección.
  - `G4_RefreshCharStatus_UpdatesSpriteAndFactionTag`: Revisa recálculo de alineación y cuerpo.

### Fase 5 (G5): Reglas de Combate e Interacción con Inventario (15 Rutinas)
- **Firmas a Implementar**:
  ```cpp
  namespace Modulo_UsUaRiOs {
      bool ToogleToAtackable(int user_index, int owner_index, bool stealing_npc = true);
      void setHome(int user_index, eCiudad new_home, int npc_index);
      void goHome(int user_index);
      void PerdioNpc(int user_index);
      void ApropioNpc(int user_index, int npc_index);
      bool SameFaccion(int user_index, int other_user_index);
      void NPCAtacado(int npc_index, int user_index);
      bool PuedeApuñalar(int user_index);
      bool PuedeAcuchillar(int user_index);
      int GetWeaponAnim(int user_index, int obj_index);
      uint8_t GetNickColor(int user_index);
      void ChangeUserInv(int user_index, uint8_t slot, const UserOBJ& obj);
      bool HasEnoughItems(int user_index, int obj_index, int32_t amount);
      int32_t TotalOfferItems(int obj_index, int user_index);
      uint8_t getMaxInventorySlots(int user_index);
  }
  ```
- **Pruebas doctest**:
  - `G5_ToogleToAtackable_SetsAttackableState`: Verifica estado de agresión legítima.
  - `G5_ApropioNpc_AssignsOwner`: Asigna la pertenencia del NPC al primer atacante.
  - `G5_PuedeApuñalar_ValidatesClassAndSkill`: Comprueba requisitos de clase/habilidad para apuñalar.

---

## 4. Criterios de Aceptación

1. **Paridad Comportamental y Totalidad**: Las 57 rutinas deben estar implementadas dentro del namespace `Modulo_UsUaRiOs` y mutar las estructuras globales en estricta concordancia con el servidor legacy VB6.
2. **Preservación Fiel de Quirks**: Los Bugs #50 a #53 deben contar con tests unitarios dedicados en `tests/test_modulo_usuarios.cpp`. `Bug #50` debe comprobarse mediante `CHECK_THROWS_AS(CheckUserLevel(idx), Modulo_UsUaRiOs::ELUOverflowException)`.
3. **Cobertura de Pruebas Unitarias**: Alcanzar un mínimo del 90% de cobertura en doctest sobre las 5 fases.
4. **Verificación Estática**: Ausencia total de advertencias en `clang-tidy`, sin fugas de memoria (RAII) ni accesos fuera de límites (*out-of-bounds*).

---

## 5. Referencias y Enlaces Relativos

- **Auditoría Técnica Detallada**: [`../audit/15d-usuarios-detalle.md`](../audit/15d-usuarios-detalle.md)
- **Registro Centralizado de Bugs**: [`KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md)
- **Convenciones del Proyecto**: [`../CONVENTIONS.md`](../CONVENTIONS.md)
- **Plan General de Portabilidad**: [`00-port-plan.md`](00-port-plan.md)
