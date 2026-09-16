#ifndef TRABAJO_HPP
#define TRABAJO_HPP

#include "Declares.hpp"
#include "Protocol.hpp"
#include <cstdint>
#include <functional>
#include <string_view>

namespace Trabajo {

constexpr std::int16_t GUANTE_HURTO = 873;
constexpr std::int16_t GASTO_ENERGIA_TRABAJADOR = 2;
constexpr std::int16_t GASTO_ENERGIA_NO_TRABAJADOR = 6;
constexpr std::int16_t MineralHierro = iMinerales::HierroCrudo;
constexpr std::int16_t MineralPlata = iMinerales::PlataCruda;
constexpr std::int16_t MineralOro = iMinerales::OroCrudo;

struct TrabajoHooks {
    std::function<void(std::int16_t user_index)> WriteUpdateSta;
    std::function<void(std::int16_t user_index, std::string_view msg, ao::net::protocol::FontTypeNames font_type)> WriteConsoleMsg;
    std::function<void(std::int16_t user_index, std::uint8_t slot)> Desequipar;
    std::function<std::int32_t(std::int32_t min, std::int32_t max)> RandomNumber;
    std::function<bool(std::int16_t user_index, const Obj& item)> MeterItemEnInventario;
    std::function<void(const WorldPos& pos, const Obj& item)> TirarItemAlPiso;
    std::function<void(std::int16_t user_index, std::int16_t skill_index, bool exito)> SubirSkill;
    std::function<void(std::int16_t user_index)> WriteStopWorking;
    std::function<void(std::int16_t user_index, std::uint8_t wave_number)> SendAreaWave;
    std::function<void(std::int16_t user_index)> WriteUpdateGold;
    std::function<void(std::int16_t user_index)> FlushBuffer;
    std::function<void(std::int16_t user_index)> VolverCriminal;
    std::function<void(std::int16_t user_index, std::int16_t victim_npc_index, std::int32_t daño)> CalcularDarExp;
    std::function<void(std::int16_t victim_user_index)> WriteParalizeOK;
    std::function<void(std::int16_t user_index, std::uint8_t slot, std::uint8_t amount)> QuitarUserInvItem;
    std::function<void(bool armaduras, std::int16_t user_index, std::uint8_t slot)> UpdateUserInv;
    std::function<void(std::int16_t npc_index)> FollowAmo;
    std::function<void(std::int16_t npc_index)> ReSpawnNpc;
    std::function<void(std::int16_t npc_index)> QuitarNPC;
    std::function<void(std::int16_t user_index)> CancelExit;
    std::function<std::uint8_t(std::int16_t user_index, std::int16_t victima_index)> TriggerZonaPelea;
    std::function<void(std::int16_t user_index)> ToogleBoatBody;
    std::function<void(std::int16_t user_index, std::int16_t body, std::int16_t head, std::uint8_t heading, std::int16_t weapon, std::int16_t shield, std::int16_t helmet)> ChangeUserChar;
    std::function<void(std::int16_t user_index, std::int16_t char_index, bool invisible)> SetInvisible;
    std::function<void(std::int16_t user_index)> WriteNavigateToggle;
    std::function<void(std::int16_t user_index)> WriteMeditateToggle;
    std::function<void(std::int16_t user_index)> WriteUpdateMana;
    std::function<void(const Obj& obj, std::int16_t map, std::int16_t x, std::int16_t y)> MakeObj;
    std::function<std::uint32_t()> GetTickCount;
};

void SetHooks(const TrabajoHooks& hooks);
void ResetHooks();

// Modificadores por clase
float ModNavegacion(eClass clase, std::int16_t user_index);
float ModFundicion(eClass clase);
std::int16_t ModCarpinteria(eClass clase);
float ModHerreriA(eClass clase);
std::int16_t ModDomar(eClass clase);

// Auxiliares de Estamina e Inventario
std::int16_t MaxItemsConstruibles(std::int16_t user_index);
void QuitarSta(std::int16_t user_index, std::int16_t cantidad);
bool TieneObjetos(std::int16_t item_index, std::int16_t cant, std::int16_t user_index);
void QuitarObjetos(std::int16_t item_index, std::int16_t cant, std::int16_t user_index);

// Verificación y Remoción de Materiales (Crafteo y Upgrades)
bool PuedeConstruir(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items);
bool PuedeConstruirHerreria(std::int16_t item_index);
bool HerreroTieneMateriales(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items);
void HerreroQuitarMateriales(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items);
bool PuedeConstruirCarpintero(std::int16_t item_index);
bool CarpinteroTieneMateriales(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad, bool show_msg = false);
void CarpinteroQuitarMateriales(std::int16_t user_index, std::int16_t item_index, std::int16_t cantidad_items);
bool TieneMaterialesUpgrade(std::int16_t user_index, std::int16_t item_index);
void QuitarMaterialesUpgrade(std::int16_t user_index, std::int16_t item_index);

// G2 — Recolección de Recursos
void DoTalar(std::int16_t user_index, bool dar_madera_elfica = false);
void DoMineria(std::int16_t user_index);
void DoPescar(std::int16_t user_index);
void DoPescarRed(std::int16_t user_index);

// G3 — Manufactura, Fundición y Upgrades
std::int16_t MineralesParaLingote(iMinerales lingote);
void DoLingotes(std::int16_t user_index);
void FundirMineral(std::int16_t user_index);
void FundirArmas(std::int16_t user_index);
void DoFundir(std::int16_t user_index);
void HerreroConstruirItem(std::int16_t user_index, std::int16_t item_index);
void CarpinteroConstruirItem(std::int16_t user_index, std::int16_t item_index);
void DoUpgrade(std::int16_t user_index, std::int16_t item_index);

// G4 — Habilidades de Combate, Hurto, Desarme y Domesticación
void DoApuñalar(std::int16_t user_index, std::int16_t victim_npc_index, std::int16_t victim_user_index, std::int16_t daño);
void DoAcuchillar(std::int16_t user_index, std::int16_t victim_npc_index, std::int16_t victim_user_index, std::int16_t daño);
void DoGolpeCritico(std::int16_t user_index, std::int16_t victim_npc_index, std::int16_t victim_user_index, std::int16_t daño);
void DoRobar(std::int16_t ladr_on_index, std::int16_t victima_index);
bool ObjEsRobable(std::int16_t victima_index, std::int16_t slot);
void RobarObjeto(std::int16_t ladr_on_index, std::int16_t victima_index);
void DoHurtar(std::int16_t user_index, std::int16_t victima_index);
void DoHandInmo(std::int16_t user_index, std::int16_t victima_index);
void Desarmar(std::int16_t user_index, std::int16_t victim_index);
void DoDesequipar(std::int16_t user_index, std::int16_t victim_index);
void DoDomar(std::int16_t user_index, std::int16_t npc_index);
bool PuedeDomarMascota(std::int16_t user_index, std::int16_t npc_index);
std::int16_t FreeMascotaIndex(std::int16_t user_index);

// G5 — Gestión de Estados y Entorno
void DoOcultarse(std::int16_t user_index);
void DoPermanecerOculto(std::int16_t user_index);
void DoNavega(std::int16_t user_index, const ObjData& barco, std::int16_t slot);
void DoAdminInvisible(std::int16_t user_index);
void TratarDeHacerFogata(std::int16_t map, std::int16_t x, std::int16_t y, std::int16_t user_index);
void DoMeditar(std::int16_t user_index);

} // namespace Trabajo

#endif // TRABAJO_HPP
