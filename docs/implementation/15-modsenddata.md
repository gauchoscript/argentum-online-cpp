---
area: infraestructura-de-red
status: completed
module: modSendData
layer: 4
legacy_source: legacy/server/Codigo/modSendData.bas
target_header: src/server/modSendData.hpp
target_source: src/server/modSendData.cpp
test_suite: tests/test_modsenddata.cpp
last_updated: 2026-09-12
---

# Módulo #15: modSendData — Despacho y Difusión de Paquetes de Red

## Resumen del Módulo

Este documento describe la arquitectura, decisiones de diseño y comportamiento verificado del módulo de Capa 4 **`modSendData`** ([`src/server/modSendData.hpp`](../../src/server/modSendData.hpp) y [`src/server/modSendData.cpp`](../../src/server/modSendData.cpp)), correspondiente al archivo legacy [`legacy/server/Codigo/modSendData.bas`](../../legacy/server/Codigo/modSendData.bas) (~734 líneas en Visual Basic 6.0).

`modSendData` constituye el motor de difusión (*broadcasting* y *multicasting*) del servidor de Argentum Online. Su responsabilidad es canalizar los paquetes binarios ya serializados hacia los sockets de los destinatarios pertinentes según filtros espaciales (mapas, áreas de visión de 9x9 tiles), sociales (miembros de clan, integrantes de party) y jerárquicos/faccionarios (administradores, consejeros, ciudadanos, criminales, armadas reales y legiones del caos).

---

## Decisiones Estratégicas de Arquitectura en C++20

### 1. Arquitectura Zero-Copy Broadcasting (`std::span<const uint8_t>`)

- **Diagnóstico del Legacy VB6**: En Visual Basic 6, las rutinas `SendData`, `SendToAll`, `SendToArea`, etc., recibían los paquetes serializados por valor mediante cadenas `ByVal sndData As String`. Al realizar un broadcast a cientos de jugadores, la máquina virtual de VB6 realizaba constantes reasignaciones dinámicas de descriptores `BSTR` y copias redundantes de memoria.
- **Diseño Moderno en C++20**:
  - Toda la API de despacho recibe vistas inmutables contiguas de memoria mediante `std::span<const std::uint8_t> data`.
  - Se proveen sobrecargas de conveniencia que aceptan `std::string_view` (reinterpretando los caracteres como bytes de solo lectura sin copiar memoria).
  - **Cero Copias Intermedias**: Un paquete preparado una única vez en memoria (sea en el stack o en un vector de trabajo) se despacha secuencialmente a través de los sockets de todos los destinatarios sin duplicar un solo byte en memoria intermedia.

### 2. Bypass Explícito de la Cola `outgoingData` de `clsByteQueue`

- **Confirmación Empírica**: La auditoría técnica ([`docs/audit/02d-modsenddata-detalle.md`](../audit/02d-modsenddata-detalle.md)) confirmó que `modSendData.bas` **jamás escribe sobre la cola `outgoingData` del usuario ni convoca a `FlushBuffer`**.
- **Despacho Directo a Transporte**: Las funciones de `modSendData` delegan de forma directa e inmediata en la primitiva de red `TCP::EnviarDatosASlot(user_index, data)`. Esto evita saturar las colas de paquetes pendientes de cada usuario con mensajes idénticos de difusión y permite que el subsistema de transporte gestione el backpressure y la transmisión asincrónica de Asio de forma centralizada.

### 3. Correcciones Matemáticas de Bitmasks (`constexpr` Bit Shifts)

- **Defecto Aritmético en VB6**: En `modSendData.bas:550-551`, el cálculo de pertenencia a área dentro de `SendToAreaByPos` utilizaba la expresión `2 ^ (AreaX \ 9)`. Dicha operación recurría a exponenciación en punto flotante de doble precisión (`Double`) mediante la función de runtime `__vbaPower`, introduciendo una penalización de rendimiento innecesaria en el camino crítico del game loop.
- **Implementación In-line `constexpr`**: Se sustituyó por operaciones enteras de desplazamiento de bits a nivel de compilador (`1 << (pos / 9)`):
  ```cpp
  [[nodiscard]] constexpr int area_pertenece_mask(int pos) noexcept {
      return 1 << (pos / 9);
  }

  [[nodiscard]] constexpr int area_recive_mask(int area_index) noexcept {
      int mask = (1 << area_index);
      if (area_index > 0) mask |= (1 << (area_index - 1));
      if (area_index < 11) mask |= (1 << (area_index + 1));
      return mask;
  }
  ```

### 4. Mitigación de Bugs y Quirks Legacy

1. **Eliminación de la Constante Huérfana `SendTarget.ToGM` (Bug #21)**:
   - En VB6, `ToGM = 6` estaba declarada formalmente en el Enum público, pero `SendData` carecía de una rama `Case SendTarget.ToGM`, cayendo al vacío sin emitir error ni despachar datos.
   - En C++20, se eliminó `ToGM` del `enum class SendTarget`. Esto garantiza que la comprobación exhaustiva del compilador en las sentencias `switch` alerte de inmediato ante cualquier enumerador no manejado. Ver [`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md#entrada-21--modsenddata-constante-sendtargettogm-huérfana-en-el-enumerador-de-ruteo).
2. **Parche de Seguridad ante Fuga de Paquetes en Pre-Login (Bug #22)**:
   - En el servidor original, los canales de administración (`ToAdmins`, `ToHigherAdmins`, `ToConsejo`, etc.) y de facción (`ToCiudadanos`, `ToCriminales`, `ToReal`, `ToCaos`) verificaban únicamente que el slot tuviera una conexión TCP activa (`ConnID <> -1`), omitiendo comprobar `flags.UserLogged`. Como consecuencia, si un usuario tenía el socket abierto pero aún no se había autenticado en el juego, recibía paquetes de difusión del mundo.
   - En C++20, todas las funciones de ruteo global y faccionario validan rigurosamente que el usuario se encuentre logueado (`UserList[i].flags.UserLogged`), mitigando filtraciones de información hacia clientes no autenticados. Ver [`docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md#entrada-22--modsenddata-omisión-de-verificación-userlogged-en-broadcasts-globales-administrativos-y-faccionarios).

### 5. Resguardo Geográfico y Frontera con `ModAreas` (Capa 5 Proxy)

- Para consultar los usuarios presentes en un mapa sin incurrir en dependencias circulares antes del porting completo de `ModAreas.bas`, `modSendData` accede a las estructuras centrales definidas en [`src/server/Declares.hpp`](../../src/server/Declares.hpp) (`ConnGroups`, `MapData`, `UserList`).
- Todas las rutinas incorporan la validación determinista de límites mediante `is_valid_map_index(map)` (asegurando `1 <= map <= NumMaps`), previniendo desbordamientos de vector ante IDs de mapas inválidos.

---

## Catálogo de Funciones Implementadas

Las funciones se encuentran bajo el espacio de nombres `ao::net::send_data`:

### 1. Tipado Fuerte y Helpers de Bitmasks
- `enum class SendTarget : std::uint8_t`: Enumerador strongly-typed con 29 destinos válidos.
- `area_pertenece_mask(int pos) noexcept -> int`: Bitmask para coordenada $X$ o $Y$ ($1..100$).
- `area_recive_mask(int area_index) noexcept -> int`: Cono de visión de 3 áreas contiguas ($0..11$).
- `is_valid_map_index(int map) noexcept -> bool`: Validación de cotas seguras contra `NumMaps`.

### 2. Ruteo Geográfico y Espacial
- `send_to_user_area(user_index, data)`: Despacha a todos los usuarios cuyas áreas de visión intersectan la posición del usuario emisor.
- `send_to_user_area_but_index(user_index, data)`: Igual a `send_to_user_area`, pero excluyendo al emisor.
- `send_to_dead_user_area(user_index, data)`: Despacha a usuarios vivos en el área y a los muertos que tengan el mapa en memoria.
- `send_to_area_by_pos(map, area_x, area_y, data)`: Despacha a usuarios del mapa cuyas áreas intersectan las coordenadas dadas.
- `send_to_map(map, data)`: Broadcast a todos los usuarios conectados en un mapa determinado.
- `send_to_map_but_index(user_index, data)`: Broadcast al mapa del usuario, omitiendo al propio usuario emisor.
- `send_to_npc_area(npc_index, data)`: Despacha a los usuarios que ven el área donde se ubica un NPC.

### 3. Ruteo Social (Clanes y Parties)
- `send_to_guild_members(guild_index, data)`: Transmite a todos los miembros online de un clan.
- `send_to_dioses_y_clan(guild_index, data)`: Transmite a los miembros del clan y a todos los Game Masters con privilegio de Dioses.
- `send_to_user_guild_area(user_index, data)`: Transmite únicamente a los miembros del mismo clan que se encuentren en el área visual del usuario.
- `send_to_user_party_area(user_index, data)`: Transmite a los compañeros de grupo (*party*) presentes en el área visual del emisor.

### 4. Ruteo Jerárquico y Faccionario (con Parche Pre-Login)
- `send_to_all(data)` / `send_to_all_but_index(user_index, data)`: Broadcast global a todos los usuarios logueados del servidor.
- `send_to_admins(data)` / `send_to_higher_admins(data)`: Transmisión a miembros del staff y administradores de alto rango.
- `send_to_consejo(data)` / `send_to_consejo_caos(data)`: Transmisión a los consejos faccionarios.
- `send_to_roles_masters(data)`: Transmisión a los Roles Masters.
- `send_to_ciudadanos(data)` / `send_to_criminales(data)`: Transmisión a ciudadanos y criminales.
- `send_to_real(data)` / `send_to_caos(data)`: Transmisión a miembros de la Armada Real o Legión Oscura.
- `send_to_ciudadanos_y_rms(data)`, `send_to_criminales_y_rms(data)`, `send_to_real_y_rms(data)`, `send_to_caos_y_rms(data)`: Canales combinados con soporte para Roles Masters.
- `send_to_admins_but_consejeros_area(user_index, data)`: Difusión local a GMs en el área excluyendo consejeros.
- `send_to_gms_area_but_rms_or_counselors(user_index, data)`: Difusión a administradores excluyendo RMs o consejeros.
- `send_to_users_area_but_gms(user_index, data)`: Difusión a usuarios mortales en el área ignorando GMs.
- `send_to_users_and_rms_and_counselors_area_but_gms(user_index, data)`: Difusión en el área para usuarios, consejeros y RMs, excluyendo GMs.

### 5. Despachador Maestro y Funcionalidades Especiales
- `send_data(route, index, data)`: Dispatcher centralizado que rutea la carga útil según la enumeración `SendTarget`.
- `alertar_faccionarios(user_index)`: Determina la orientación cardinal del usuario atacado y emite una alerta direccional a sus compañeros de facción en el mapa.

---

## Verificación y Pruebas Unitarias (`tests/test_modsenddata.cpp`)

La suite de pruebas exhaustiva en [`tests/test_modsenddata.cpp`](../../tests/test_modsenddata.cpp) cubre el 100% de los escenarios del módulo bajo el framework **doctest**:

1. **Bitmasks y Validación**: Verificación de corrimientos de bits para `area_pertenece_mask` y `area_recive_mask`, y límites de mapas válidos e inválidos.
2. **Ruteo Geográfico Zero-Copy**: Pruebas de difusión a áreas de mapa (`send_to_user_area`, `send_to_user_area_but_index`), verificando que los sockets receptores reciban los bytes idénticos sin mutación.
3. **Mitigación del Bug #22 en Pre-Login**: Se configuraron usuarios con socket activo (`ConnID != -1`) pero bandera `flags.UserLogged = false`. Se verificó que ninguna difusión global ni faccionaria envíe bytes a usuarios no autenticados, y que al cambiar `UserLogged = true` el despacho opere normalmente.
4. **Ruteo Social de Clanes y Parties**: Verificación de filtros por `GuildIndex` y miembros de party locales.
5. **Dispatcher Maestro `send_data`**: Validación exhaustiva de las distintas ramas de `SendTarget`, incluyendo el manejo seguro de casos borde.

---

## Referencias Cruzadas

- [Normas de Arquitectura y Convenciones — `docs/CONVENTIONS.md`](../CONVENTIONS.md)
- [Plan Maestro de Porting C++ — `docs/implementation/00-port-plan.md`](00-port-plan.md)
- [Desglose Metodológico de `modSendData.bas` — `docs/implementation/15-modsenddata-breakdown.md`](15-modsenddata-breakdown.md)
- [Registro Maestro de Bugs Históricos — `docs/implementation/KNOWN-LEGACY-BUGS.md`](KNOWN-LEGACY-BUGS.md) (Entradas #21 y #22)
- [Auditoría de Protocolo y Despacho de Red — `docs/audit/02d-modsenddata-detalle.md`](../audit/02d-modsenddata-detalle.md)
- [Especificación de Transporte TCP — `docs/implementation/14-tcp.md`](14-tcp.md)
