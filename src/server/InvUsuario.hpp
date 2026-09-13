#pragma once

#include <cstdint>
#include <functional>
#include "Declares.hpp"

namespace InvUsuario {

// Hooks inyectables para desacoplar dependencias externas (Capa 9, G3 y espacio)
using QuitarUserInvItemHook = std::function<void(std::int16_t user_index, std::uint8_t slot, std::int16_t cantidad)>;
using UpdateUserInvHook = std::function<void(bool update_all, std::int16_t user_index, std::uint8_t slot)>;
using MeterItemEnInventarioHook = std::function<bool(std::int16_t user_index, Obj& obj)>;
using ChangeUserInvHook = std::function<void(std::int16_t user_index, std::uint8_t slot, const UserOBJ& item)>;
using DesequiparHook = std::function<void(std::int16_t user_index, std::uint8_t slot)>;
using TilelibreHook = std::function<void(const WorldPos& pos, WorldPos& n_pos, const Obj& obj, bool agua, bool tierra)>;

void SetQuitarUserInvItemHook(QuitarUserInvItemHook hook) noexcept;
void SetUpdateUserInvHook(UpdateUserInvHook hook) noexcept;
void SetMeterItemEnInventarioHook(MeterItemEnInventarioHook hook) noexcept;
void SetChangeUserInvHook(ChangeUserInvHook hook) noexcept;
void SetDesequiparHook(DesequiparHook hook) noexcept;
void SetTilelibreHook(TilelibreHook hook) noexcept;

// ==========================================
// Fase 1: G1 — Mutaciones en el Mundo y Suelo
// ==========================================

/**
 * @brief Crea un objeto en el suelo en la posición (map, x, y).
 * Si la celda ya contiene el mismo ítem, acumula cantidades promoviendo a std::int32_t
 * antes de la asignación para erradicar Undefined Behavior (overflow con signo en C++).
 * Si estaba vacía, asigna el objeto y emite paquete de creación al área circundante.
 */
void MakeObj(const Obj& obj, std::int16_t map, std::int16_t x, std::int16_t y);

/**
 * @brief Descuenta una cantidad de objetos de la celda (map, x, y).
 * Si la cantidad remanente es <= 0, elimina el objeto y emite paquete de borrado al área.
 */
void EraseObj(std::int16_t num, std::int16_t map, std::int16_t x, std::int16_t y);

/**
 * @brief Arroja un objeto del inventario del usuario hacia la celda del suelo.
 * Comprueba ocupación de celda leyendo MapData[UserList[user_index].Pos.Map, x, y],
 * preservando la incoherencia de parámetro legacy donde la coordenada de mapa es la del usuario.
 * Preserva estrictamente el Bug histórico #29: exploit de duplicación de ítems por recorte
 * asimétrico de cantidades (Obj.Amount mantiene el valor original pretendido al convocar a
 * MakeObj, mientras que QuitarUserInvItem descuenta únicamente la variable escalar num recortada).
 */
void DropObj(std::int16_t user_index, std::uint8_t slot, std::int16_t num, std::int16_t map, std::int16_t x, std::int16_t y);

/**
 * @brief Levanta el objeto ubicado en la celda donde está parado el usuario.
 * Si es oro, se acredita directamente en la billetera; si es ítem, ingresa al inventario.
 */
void GetObj(std::int16_t user_index);

// ====================================================
// Fase 2: G2 — Gestión Base de Inventario y Descarte
// ====================================================

/**
 * @brief Ingresa un objeto en la mochila del usuario respetando slots activos y apilamiento.
 */
bool MeterItemEnInventario(std::int16_t user_index, Obj& mi_obj);

/**
 * @brief Descuenta cantidad de un slot del usuario y desequipa si llega a 0.
 * El inventario opera como una grilla fija y dispersa (sparse) de 30 ranuras.
 * Al vaciarse un slot, se blanquea en su posición exacta sin corrimiento de elementos (no vector::erase).
 */
void QuitarUserInvItem(std::int16_t user_index, std::uint8_t slot, std::int16_t cantidad);

/**
 * @brief Sincroniza visualmente uno o todos los slots de inventario del usuario.
 */
void UpdateUserInv(bool update_all, std::int16_t user_index, std::uint8_t slot);

/**
 * @brief Blanquea completamente las ranuras de inventario y punteros de equipamiento rápido.
 */
void LimpiarInventario(std::int16_t user_index);

/**
 * @brief Evalúa si el usuario posee objetos susceptibles de ser robados (no llaves ni barcos).
 */
bool TieneObjetosRobables(std::int16_t user_index);

/**
 * @brief Remueve todos los ítems marcados como Newbie del inventario del usuario.
 * Preserva el quirk de ejecución incondicional de UpdateUserInv.
 */
void QuitarNewbieObj(std::int16_t user_index);

/**
 * @brief Determina si un objeto cae al morir o descartarse (evalúa facción y no-caíble).
 */
bool ItemSeCae(std::int16_t index);

/**
 * @brief Consulta si el objeto es de uso exclusivo para novatos (Newbie = 1).
 */
bool ItemNewbie(std::int16_t item_index);

/**
 * @brief Retorna el tipo de objeto según la tabla ObjDataList.
 */
eOBJType getObjType(std::int16_t obj_index);

/**
 * @brief Arroja todos los ítems y el excedente de oro del usuario al morir.
 */
void TirarTodo(std::int16_t user_index);

/**
 * @brief Arroja todos los ítems del inventario a celdas adyacentes transitables.
 */
void TirarTodosLosItems(std::int16_t user_index);

/**
 * @brief Arroja únicamente los ítems no newbies hacia celdas libres del mapa.
 */
void TirarTodosLosItemsNoNewbies(std::int16_t user_index);

/**
 * @brief Arroja los ítems ubicados en las ranuras expandidas de la mochila.
 */
void TirarTodosLosItemsEnMochila(std::int16_t user_index);

/**
 * @brief Arroja oro de la billetera en pilas de hasta 10.000 monedas al suelo.
 * Preserva estrictamente el Bug histórico #30: si la cantidad supera 500.000 monedas
 * y se logra arrojar al menos una pila, la variable 'extra' (> 500k) se deduce de forma
 * incondicional del saldo de la billetera sin crearse en el suelo (evaporación de oro).
 */
void TirarOro(std::int32_t cantidad, std::int16_t user_index);

// ====================================================
// Fase 3: G3 — Restricciones y Sistema de Equipamiento
// ====================================================

using ChangeUserCharHook = std::function<void(std::int16_t user_index, std::int16_t body, std::int16_t head, std::int16_t heading, std::int16_t weapon, std::int16_t shield, std::int16_t helmet)>;
using DarCuerpoDesnudoHook = std::function<std::int16_t(std::int16_t user_index, bool mantener_raza)>;

void SetChangeUserCharHook(ChangeUserCharHook hook) noexcept;
void SetDarCuerpoDesnudoHook(DarCuerpoDesnudoHook hook) noexcept;

/**
 * @brief Valida si la clase del personaje está habilitada para usar el ítem (considera bypass de GM).
 */
bool ClasePuedeUsarItem(std::int16_t user_index, std::int16_t obj_index, std::string* motivo = nullptr);

/**
 * @brief Valida si el género del personaje concuerda con las restricciones del ítem (Hombre / Mujer).
 */
bool SexoPuedeUsarItem(std::int16_t user_index, std::int16_t obj_index, std::string* motivo = nullptr);

/**
 * @brief Valida si la alineación/facción del usuario concuerda con los requisitos (Armada Real / Fuerzas del Caos).
 */
bool FaccionPuedeUsarItem(std::int16_t user_index, std::int16_t obj_index, std::string* motivo = nullptr);

/**
 * @brief Valida compatibilidad racial de la prenda (humanos/elfos/drows vs. enanos/gnomos y exclusivo drow).
 */
bool CheckRazaUsaRopa(std::int16_t user_index, std::int16_t item_index, std::string* motivo = nullptr);

/**
 * @brief Equipa un ítem del inventario (actúa como toggle si ya estaba equipado, desequipa preexistente).
 */
void EquiparInvItem(std::int16_t user_index, std::uint8_t slot);

/**
 * @brief Desequipa el ítem de la ranura, restaura apariencias por defecto y descarta excedente de mochila si aplica.
 */
void Desequipar(std::int16_t user_index, std::uint8_t slot);

// ====================================================
// Fase 4: G4 — Uso de Ítems, Interacción y Verificación
// ====================================================

using LearnSpellHook = std::function<bool(std::int16_t user_index, std::int16_t spell_index)>;
using NavegaHook = std::function<void(std::int16_t user_index, const Obj& barco_obj)>;
using WorkRequestTargetHook = std::function<void(std::int16_t user_index, eSkill skill)>;

void SetLearnSpellHook(LearnSpellHook hook) noexcept;
void SetNavegaHook(NavegaHook hook) noexcept;
void SetWorkRequestTargetHook(WorkRequestTargetHook hook) noexcept;

/**
 * @brief Procesa el uso de un ítem del inventario (alimentos, bebidas, pociones, herramientas, etc.).
 */
void UseInvItem(std::int16_t user_index, std::uint8_t slot);

/**
 * @brief Envía la lista de armas construibles de herrería al usuario.
 * Preserva el typo legacy verbatim ("EnivarArmasConstruibles") por convención de porting.
 */
void EnivarArmasConstruibles(std::int16_t user_index);

/**
 * @brief Envía la lista de objetos construibles de carpintería al usuario.
 * Preserva el typo legacy verbatim ("EnivarObjConstruibles") por convención de porting.
 */
void EnivarObjConstruibles(std::int16_t user_index);

/**
 * @brief Envía la lista de armaduras construibles de herrería al usuario.
 * Preserva el typo legacy verbatim ("EnivarArmadurasConstruibles") por convención de porting.
 */
void EnivarArmadurasConstruibles(std::int16_t user_index);

} // namespace InvUsuario

