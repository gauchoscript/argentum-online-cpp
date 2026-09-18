# Especificación Técnica de Portabilidad — Módulo #33: `Acciones`

> **Estado**: En Ejecución  
> **Área**: Capa 9 (Interacción con Mundo y Entidades)  
> **Documentación Relacionada**: [`docs/audit/15c-acciones-detalle.md`](../audit/15c-acciones-detalle.md), [`docs/implementation/33-acciones-breakdown.md`](33-acciones-breakdown.md), [`docs/CONVENTIONS.md`](../CONVENTIONS.md)  
> **Archivos C++**: `src/server/Acciones.hpp`, `src/server/Acciones.cpp`, `tests/test_acciones.cpp`

---

## 1. Introducción y Arquitectura

El módulo `Acciones` gestiona el despacho de clics del usuario sobre coordenadas del mundo (`Map`, `X`, `Y`), coordinando la respuesta del servidor según el tipo de entidad u objeto presente en la celda cliqueada.

La implementación en C++ (`src/server/Acciones.hpp` y `src/server/Acciones.cpp`) preserva la firma y orden exacto de parámetros de los 5 procedimientos de VB6, encapsulando las llamadas a subsistemas externos mediante la estructura de callbacks tipados `AccionesCallbacks`.

---

## 2. Definición del Espacio de Nombres y Callbacks

```cpp
namespace Acciones {

struct AccionesCallbacks {
    // Red y Notificaciones
    std::function<void(int16_t user_index, const std::string& msg, FontType font_type)> WriteConsoleMsg;
    std::function<void(int16_t user_index, int16_t obj_index)> WriteShowSignal;
    std::function<void(int16_t user_index)> WriteShowForumForm;
    std::function<void(int16_t user_index)> WriteUpdateUserStats;
    std::function<void(int16_t map, int16_t x, int16_t y, uint16_t grh_index)> SendObjectCreateToArea;
    std::function<void(int16_t map, int16_t x, int16_t y, uint16_t wave_index)> SendPlayWaveToArea;

    // Negocio y Usuarios
    std::function<void(int16_t user_index)> IniciarComercioNPC;
    std::function<void(int16_t user_index)> IniciarDepositoBanco;
    std::function<void(int16_t user_index)> RevivirUsuario;
    std::function<bool(int16_t user_index)> EsNewbie;
    std::function<void(int16_t user_index, eSkill skill, bool exito)> SubirSkill;
    std::function<bool(int16_t user_index, int16_t foro_id)> SendPosts;

    // Entorno y Basura
    std::function<void(const Obj& obj, int16_t map, int16_t x, int16_t y)> MakeObj;
    std::function<void(bool bloquear, int16_t map, int16_t x, int16_t y, uint8_t flag)> SetTileBlocked;
    std::function<void(int16_t map, int16_t x, int16_t y)> RegisterGarbageFire;
};

void SetAccionesCallbacks(const AccionesCallbacks& callbacks);
void InitDefaultAccionesCallbacks();

void Accion(int16_t user_index, int16_t map, int16_t x, int16_t y);
void AccionParaForo(int16_t map, int16_t x, int16_t y, int16_t user_index);
void AccionParaPuerta(int16_t map, int16_t x, int16_t y, int16_t user_index);
void AccionParaCartel(int16_t map, int16_t x, int16_t y, int16_t user_index);
void AccionParaRamita(int16_t map, int16_t x, int16_t y, int16_t user_index);

} // namespace Acciones
```

---

## 3. Registro de Quirks y Prevención de UB

1. **Bug #47 (Protección de bordes en puertas)**:
   ```cpp
   MapData[map][x][y].Blocked = (abrir ? 0 : 1);
   if (x > 1) {
       MapData[map][x - 1][y].Blocked = (abrir ? 0 : 1);
       s_callbacks.SetTileBlocked(true, map, x - 1, y, abrir ? 0 : 1);
   }
   ```
2. **Bug #48 (Tirada de Supervivencia)**:
   ```cpp
   if (skill > 1 && skill < 6) suerte = 3;
   else if (skill >= 6 && skill <= 10) suerte = 2;
   else if (skill >= 10) suerte = 1; // Bug #48 replica la condición >= 10
   ```
3. **Bug #49 (Targeting incondicional)**:
   Al cliquear sobre celda con NPC u Objeto, se asigna `user.flags.TargetNPC` o `user.flags.TargetObj` inmediatamente.
