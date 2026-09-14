#include "SistemaCombate.hpp"
#include "Protocol.hpp"
#include "FileIO.hpp"
#include "TCP.hpp"
#include "modSendData.hpp"

#include <chrono>
#include <cmath>
#include <algorithm>
#include <utility>

namespace ao {

namespace {

CombatTimeProvider s_time_provider{nullptr};
CombatRandomProvider s_random_provider{nullptr};

QuitarObjetosHook s_quitar_objetos_hook{nullptr};
DoApuñalarHook s_apuñalar_hook{nullptr};
DoGolpeCriticoHook s_golpe_critico_hook{nullptr};
DoAcuchillarHook s_acuchillar_hook{nullptr};
UserDieHook s_user_die_hook{nullptr};
MuereNpcHook s_muere_npc_hook{nullptr};
SubirSkillHook s_subir_skill_hook{nullptr};
CheckUserLevelHook s_check_user_level_hook{nullptr};
PartyExpHook s_party_exp_hook{nullptr};
RefreshCharStatusHook s_refresh_char_status_hook{nullptr};

VolverCriminalHook s_volver_criminal_hook{nullptr};
CancelExitHook s_cancel_exit_hook{nullptr};
StoreFragHook s_store_frag_hook{nullptr};
ContarMuerteHook s_contar_muerte_hook{nullptr};

inline void HeadtoPos(eHeading head, WorldPos& pos) noexcept {
    switch (head) {
        case eHeading::NORTH: pos.Y--; break;
        case eHeading::SOUTH: pos.Y++; break;
        case eHeading::EAST:  pos.X++; break;
        case eHeading::WEST:  pos.X--; break;
    }
}

std::int32_t GetCurrentTick() noexcept {
    if (s_time_provider) {
        return s_time_provider();
    }
    const auto now = std::chrono::steady_clock::now();
    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return static_cast<std::int32_t>(ms & 0x7FFFFFFF);
}

std::int32_t GetCombatRandom(std::int32_t lower, std::int32_t upper) {
    if (s_random_provider) {
        return s_random_provider(lower, upper);
    }
    return RandomNumber(lower, upper);
}

inline std::size_t GetMapBlockIndex(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return (static_cast<std::size_t>(map) * 101 + static_cast<std::size_t>(x)) * 101 + static_cast<std::size_t>(y);
}

inline const MapBlock& GetMapBlock(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    static const MapBlock s_empty{};
    const auto idx = GetMapBlockIndex(map, x, y);
    if (idx < MapData.size()) {
        return MapData[idx];
    }
    return s_empty;
}

inline bool IsValidUser(std::int16_t user_index) noexcept {
    return user_index > 0 && static_cast<std::size_t>(user_index) < UserList.size();
}

inline bool IsValidNpc(std::int16_t npc_index) noexcept {
    return npc_index > 0 && static_cast<std::size_t>(npc_index) < Npclist.size();
}

inline bool IsValidObj(std::int16_t obj_index) noexcept {
    return obj_index > 0 && static_cast<std::size_t>(obj_index) < ObjDataList.size();
}

} // namespace

using ao::net::protocol::FontTypeNames;


void SetCombatTimeProvider(CombatTimeProvider provider) noexcept {
    s_time_provider = std::move(provider);
}

void SetCombatRandomProvider(CombatRandomProvider provider) noexcept {
    s_random_provider = std::move(provider);
}

std::int16_t MinimoInt(std::int16_t a, std::int16_t b) noexcept {
    return (a > b) ? b : a;
}

std::int16_t MaximoInt(std::int16_t a, std::int16_t b) noexcept {
    return (a > b) ? a : b;
}

std::int32_t PoderEvasionEscudo(std::int16_t UserIndex) {
    if (!IsValidUser(UserIndex)) {
        return 0;
    }
    const auto& user = UserList[UserIndex];
    const auto clase_idx = static_cast<std::size_t>(user.clase);
    if (clase_idx >= ModClaseList.size()) {
        return 0;
    }
    const double skill_defensa = static_cast<double>(user.Stats.UserSkills[eSkill::Defensa]);
    const double mod_escudo = ModClaseList[clase_idx].Escudo;
    return static_cast<std::int32_t>(std::floor((skill_defensa * mod_escudo) / 2.0));
}

std::int32_t PoderEvasion(std::int16_t UserIndex) {
    if (!IsValidUser(UserIndex)) {
        return 0;
    }
    const auto& user = UserList[UserIndex];
    const auto clase_idx = static_cast<std::size_t>(user.clase);
    if (clase_idx >= ModClaseList.size()) {
        return 0;
    }
    const double skill_tacticas = static_cast<double>(user.Stats.UserSkills[eSkill::Tacticas]);
    const double agilidad = static_cast<double>(user.Stats.UserAtributos[eAtributos::Agilidad]);
    const double mod_evasion = ModClaseList[clase_idx].Evasion;

    // VB6: lTemp = (.Stats.UserSkills(eSkill.Tacticas) + .Stats.UserSkills(eSkill.Tacticas) / 33 * .Stats.UserAtributos(eAtributos.Agilidad)) * ModClase(.clase).Evasion (SistemaCombate.bas:170)
    // Preservación de punto flotante (/ 33.0): en VB6 el operador '/' produce Double. Usar 33.0 evita truncamiento a cero cuando Tactics < 33.
    const double lTemp = (skill_tacticas + (skill_tacticas / 33.0) * agilidad) * mod_evasion;

    // VB6: PoderEvasion = (lTemp + (2.5 * MaximoInt(.Stats.ELV - 12, 0))) (SistemaCombate.bas:172)
    const double level_bonus = 2.5 * static_cast<double>(MaximoInt(static_cast<std::int16_t>(user.Stats.ELV) - 12, 0));
    return static_cast<std::int32_t>(std::floor(lTemp + level_bonus));
}

std::int32_t PoderAtaqueArma(std::int16_t UserIndex) {
    if (!IsValidUser(UserIndex)) {
        return 0;
    }
    const auto& user = UserList[UserIndex];
    const auto clase_idx = static_cast<std::size_t>(user.clase);
    if (clase_idx >= ModClaseList.size()) {
        return 0;
    }
    const auto skill = static_cast<std::int32_t>(user.Stats.UserSkills[eSkill::Armas]);
    const auto agilidad = static_cast<std::int32_t>(user.Stats.UserAtributos[eAtributos::Agilidad]);
    const double mod_ataque = ModClaseList[clase_idx].AtaqueArmas;

    double poder_ataque_temp = 0.0;
    if (skill < 31) {
        poder_ataque_temp = static_cast<double>(skill) * mod_ataque;
    } else if (skill < 61) {
        poder_ataque_temp = static_cast<double>(skill + agilidad) * mod_ataque;
    } else if (skill < 91) {
        poder_ataque_temp = static_cast<double>(skill + 2 * agilidad) * mod_ataque;
    } else {
        poder_ataque_temp = static_cast<double>(skill + 3 * agilidad) * mod_ataque;
    }

    const double level_bonus = 2.5 * static_cast<double>(MaximoInt(static_cast<std::int16_t>(user.Stats.ELV) - 12, 0));
    return static_cast<std::int32_t>(std::floor(poder_ataque_temp + level_bonus));
}

std::int32_t PoderAtaqueProyectil(std::int16_t UserIndex) {
    if (!IsValidUser(UserIndex)) {
        return 0;
    }
    const auto& user = UserList[UserIndex];
    const auto clase_idx = static_cast<std::size_t>(user.clase);
    if (clase_idx >= ModClaseList.size()) {
        return 0;
    }
    const auto skill = static_cast<std::int32_t>(user.Stats.UserSkills[eSkill::Proyectiles]);
    const auto agilidad = static_cast<std::int32_t>(user.Stats.UserAtributos[eAtributos::Agilidad]);
    const double mod_ataque = ModClaseList[clase_idx].AtaqueProyectiles;

    double poder_ataque_temp = 0.0;
    if (skill < 31) {
        poder_ataque_temp = static_cast<double>(skill) * mod_ataque;
    } else if (skill < 61) {
        poder_ataque_temp = static_cast<double>(skill + agilidad) * mod_ataque;
    } else if (skill < 91) {
        poder_ataque_temp = static_cast<double>(skill + 2 * agilidad) * mod_ataque;
    } else {
        poder_ataque_temp = static_cast<double>(skill + 3 * agilidad) * mod_ataque;
    }

    const double level_bonus = 2.5 * static_cast<double>(MaximoInt(static_cast<std::int16_t>(user.Stats.ELV) - 12, 0));
    return static_cast<std::int32_t>(std::floor(poder_ataque_temp + level_bonus));
}

std::int32_t PoderAtaqueWrestling(std::int16_t UserIndex) {
    if (!IsValidUser(UserIndex)) {
        return 0;
    }
    const auto& user = UserList[UserIndex];
    const auto clase_idx = static_cast<std::size_t>(user.clase);
    if (clase_idx >= ModClaseList.size()) {
        return 0;
    }
    const auto skill = static_cast<std::int32_t>(user.Stats.UserSkills[eSkill::Wrestling]);
    const auto agilidad = static_cast<std::int32_t>(user.Stats.UserAtributos[eAtributos::Agilidad]);
    const double mod_ataque = ModClaseList[clase_idx].AtaqueWrestling;

    double poder_ataque_temp = 0.0;
    if (skill < 31) {
        poder_ataque_temp = static_cast<double>(skill) * mod_ataque;
    } else if (skill < 61) {
        poder_ataque_temp = static_cast<double>(skill + agilidad) * mod_ataque;
    } else if (skill < 91) {
        poder_ataque_temp = static_cast<double>(skill + 2 * agilidad) * mod_ataque;
    } else {
        poder_ataque_temp = static_cast<double>(skill + 3 * agilidad) * mod_ataque;
    }

    const double level_bonus = 2.5 * static_cast<double>(MaximoInt(static_cast<std::int16_t>(user.Stats.ELV) - 12, 0));
    return static_cast<std::int32_t>(std::floor(poder_ataque_temp + level_bonus));
}

std::int32_t PoderAtaqueModificado(std::int16_t UserIndex) {
    if (!IsValidUser(UserIndex)) {
        return 0;
    }
    const auto weapon_idx = UserList[UserIndex].Invent.WeaponEqpObjIndex;
    if (IsValidObj(weapon_idx) && ObjDataList[weapon_idx].proyectil == 1) {
        return PoderAtaqueProyectil(UserIndex);
    }
    return PoderAtaqueArma(UserIndex);
}

bool CheckResistencia(std::int16_t UserIndex) {
    if (!IsValidUser(UserIndex)) {
        return false;
    }
    const auto skill_magia = static_cast<std::int32_t>(UserList[UserIndex].Stats.UserSkills[eSkill::Magia]);
    return GetCombatRandom(1, 100) <= skill_magia;
}

bool ArcoYFlecha(std::int16_t UserIndex) {
    if (!IsValidUser(UserIndex)) {
        return false;
    }
    const auto weapon_idx = UserList[UserIndex].Invent.WeaponEqpObjIndex;
    const auto ammo_idx = UserList[UserIndex].Invent.MunicionEqpObjIndex;

    if (!IsValidObj(weapon_idx) || ObjDataList[weapon_idx].proyectil != 1) {
        return false;
    }
    if (ObjDataList[weapon_idx].Municion == 1) {
        if (!IsValidObj(ammo_idx)) {
            return false;
        }
        return CheckArmasMuniciones(UserIndex, weapon_idx, ammo_idx);
    }
    return true;
}

std::int32_t AlcanzaEspacio(std::int16_t UserIndex, std::int16_t Arma) {
    if (IsValidObj(Arma) && ObjDataList[Arma].proyectil == 1) {
        return MAXDISTANCIAARCO;
    }
    return 1;
}

bool CheckArmasMuniciones(std::int16_t UserIndex, std::int16_t Arma, std::int16_t Municion) {
    if (!IsValidObj(Arma) || !IsValidObj(Municion)) {
        return false;
    }
    const auto& arma_data = ObjDataList[Arma];
    const auto& municion_data = ObjDataList[Municion];

    if (arma_data.Municion == 1) {
        return municion_data.OBJType == eOBJType::otFlechas || municion_data.proyectil == 1;
    }
    return true;
}

bool ArmaParaApuñalar(std::int16_t ArmaIndex) {
    if (!IsValidObj(ArmaIndex)) {
        return false;
    }
    return ObjDataList[ArmaIndex].Apuñala == 1;
}

bool SameClan(std::int16_t UserIndex, std::int16_t OtherUserIndex) {
    if (!IsValidUser(UserIndex) || !IsValidUser(OtherUserIndex)) {
        return false;
    }
    const auto clan_a = UserList[UserIndex].GuildIndex;
    const auto clan_b = UserList[OtherUserIndex].GuildIndex;
    return (clan_a == clan_b) && (clan_a != 0);
}

bool SameParty(std::int16_t UserIndex, std::int16_t OtherUserIndex) {
    if (!IsValidUser(UserIndex) || !IsValidUser(OtherUserIndex)) {
        return false;
    }
    const auto party_a = UserList[UserIndex].PartyIndex;
    const auto party_b = UserList[OtherUserIndex].PartyIndex;
    return (party_a == party_b) && (party_a != 0);
}

eTrigger6 TriggerZonaPelea(std::int16_t Origen, std::int16_t Destino) {
    if (!IsValidUser(Origen) || !IsValidUser(Destino)) {
        return eTrigger6::TRIGGER6_AUSENTE;
    }
    const auto& pos_org = UserList[Origen].Pos;
    const auto& pos_dst = UserList[Destino].Pos;

    if (pos_org.Map <= 0 || pos_dst.Map <= 0 ||
        pos_org.X < 1 || pos_org.X > 100 || pos_org.Y < 1 || pos_org.Y > 100 ||
        pos_dst.X < 1 || pos_dst.X > 100 || pos_dst.Y < 1 || pos_dst.Y > 100) {
        return eTrigger6::TRIGGER6_AUSENTE;
    }

    const auto idx_org = GetMapBlockIndex(pos_org.Map, pos_org.X, pos_org.Y);
    const auto idx_dst = GetMapBlockIndex(pos_dst.Map, pos_dst.X, pos_dst.Y);

    if (idx_org >= MapData.size() || idx_dst >= MapData.size()) {
        return eTrigger6::TRIGGER6_AUSENTE;
    }

    const auto t_org = MapData[idx_org].trigger;
    const auto t_dst = MapData[idx_dst].trigger;

    if (t_org == eTrigger::ZONAPELEA || t_dst == eTrigger::ZONAPELEA) {
        if (t_org == t_dst) {
            return eTrigger6::TRIGGER6_PERMITE;
        } else {
            return eTrigger6::TRIGGER6_PROHIBE;
        }
    }
    return eTrigger6::TRIGGER6_AUSENTE;
}

bool IntervaloPermiteAtacar(std::int16_t UserIndex, bool Avisar) {
    if (!IsValidUser(UserIndex)) {
        return false;
    }
    const auto t_actual = GetCurrentTick();
    auto& counters = UserList[UserIndex].Counters;

    if (t_actual - counters.TimerPuedeAtacar >= IntervaloUserPuedeAtacar) {
        counters.TimerPuedeAtacar = t_actual;
        counters.TimerGolpeUsar = t_actual;
        return true;
    }
    return false;
}

// ============================================================================
// Daño Bruto y Acierto RNG (Fase 2: G2)
// ============================================================================

std::int32_t CalcularDaño(std::int16_t UserIndex, std::int16_t NpcIndex) {
    if (!IsValidUser(UserIndex)) {
        return 1;
    }

    const auto& user = UserList[UserIndex];
    const auto clase_idx = static_cast<std::size_t>(user.clase);
    if (clase_idx >= ModClaseList.size()) {
        return 1;
    }

    std::int32_t daño_arma = 0;
    std::int32_t daño_max_arma = 0;
    double modif_clase = 1.0;
    bool mato_dragon = false;

    const auto weapon_idx = user.Invent.WeaponEqpObjIndex;
    if (weapon_idx > 0 && IsValidObj(weapon_idx)) {
        const auto& arma = ObjDataList[weapon_idx];

        if (NpcIndex > 0 && IsValidNpc(NpcIndex)) {
            // Ataca a un NPC
            if (arma.proyectil == 1) {
                modif_clase = ModClaseList[clase_idx].DañoProyectiles;
                daño_arma = GetCombatRandom(arma.MinHIT, arma.MaxHIT);
                daño_max_arma = arma.MaxHIT;

                if (arma.Municion == 1) {
                    const auto ammo_idx = user.Invent.MunicionEqpObjIndex;
                    if (ammo_idx > 0 && IsValidObj(ammo_idx)) {
                        const auto& proyectil = ObjDataList[ammo_idx];
                        daño_arma += GetCombatRandom(proyectil.MinHIT, proyectil.MaxHIT);
                        // BUG #37 HISTÓRICO REPLICADO (SistemaCombate.bas:384-388): proyectil.MaxHIT NO se suma a daño_max_arma.
                        // daño_max_arma permanece igual a arma.MaxHIT para el bono por fuerza.
                    }
                }
            } else {
                modif_clase = ModClaseList[clase_idx].DañoArmas;

                // Criterio 7.1 (SistemaCombate.bas:394-401): Espada Mata Dragones inflige daño normal contra DRAGON, pero 1 contra otros NPCs.
                if (weapon_idx == EspadaMataDragonesIndex) {
                    if (Npclist[NpcIndex].NPCtype == eNPCType::DRAGON) {
                        daño_arma = GetCombatRandom(arma.MinHIT, arma.MaxHIT);
                        daño_max_arma = arma.MaxHIT;
                        mato_dragon = true;
                    } else {
                        daño_arma = 1;
                        daño_max_arma = 1;
                    }
                } else {
                    daño_arma = GetCombatRandom(arma.MinHIT, arma.MaxHIT);
                    daño_max_arma = arma.MaxHIT;
                }
            }
        } else {
            // Ataca a un Usuario (PvP)
            if (arma.proyectil == 1) {
                modif_clase = ModClaseList[clase_idx].DañoProyectiles;
                daño_arma = GetCombatRandom(arma.MinHIT, arma.MaxHIT);
                daño_max_arma = arma.MaxHIT;

                if (arma.Municion == 1) {
                    const auto ammo_idx = user.Invent.MunicionEqpObjIndex;
                    if (ammo_idx > 0 && IsValidObj(ammo_idx)) {
                        const auto& proyectil = ObjDataList[ammo_idx];
                        daño_arma += GetCombatRandom(proyectil.MinHIT, proyectil.MaxHIT);
                        // BUG #37 HISTÓRICO REPLICADO (SistemaCombate.bas:384-388): proyectil.MaxHIT NO se suma a daño_max_arma.
                    }
                }
            } else {
                modif_clase = ModClaseList[clase_idx].DañoArmas;

                // Criterio 7.1 (SistemaCombate.bas:404-405): En PvP la Espada Mata Dragones inflige daño fijo 1.
                if (weapon_idx == EspadaMataDragonesIndex) {
                    daño_arma = 1;
                    daño_max_arma = 1;
                } else {
                    daño_arma = GetCombatRandom(arma.MinHIT, arma.MaxHIT);
                    daño_max_arma = arma.MaxHIT;
                }
            }
        }
    } else {
        // Combate Desarmado (Wrestling) o sin arma
        modif_clase = ModClaseList[clase_idx].DañoWrestling;

        std::int32_t daño_min_arma = 4;
        daño_max_arma = 9;

        const auto glove_idx = user.Invent.AnilloEqpObjIndex;
        bool tiene_guantes = false;
        if (glove_idx > 0 && IsValidObj(glove_idx)) {
            if (ObjDataList[glove_idx].Guante == 1) {
                daño_min_arma += ObjDataList[glove_idx].MinHIT;
                daño_max_arma += ObjDataList[glove_idx].MaxHIT;
                tiene_guantes = true;
            }
        }

        // Si no porta arma ni guante de combate, y la clase no tiene daño o se trata de ataque básico:
        daño_arma = GetCombatRandom(daño_min_arma, daño_max_arma);
    }

    const std::int32_t daño_usuario = GetCombatRandom(user.Stats.MinHIT, user.Stats.MaxHIT);

    if (mato_dragon && NpcIndex > 0 && IsValidNpc(NpcIndex)) {
        return Npclist[NpcIndex].Stats.MinHp + Npclist[NpcIndex].Stats.def;
    }

    const auto fuerza = static_cast<std::int32_t>(user.Stats.UserAtributos[eAtributos::Fuerza]);
    const auto fuerza_excedente = MaximoInt(0, static_cast<std::int16_t>(fuerza - 15));
    const double bono_fuerza = (static_cast<double>(daño_max_arma) / 5.0) * static_cast<double>(fuerza_excedente);

    const double total = (3.0 * static_cast<double>(daño_arma) + bono_fuerza + static_cast<double>(daño_usuario)) * modif_clase;
    const auto resultado = static_cast<std::int32_t>(std::floor(total));

    return (resultado < 1) ? 1 : resultado;
}

std::int32_t ProbExito(std::int32_t PoderAtacante, std::int32_t PoderDefensor) {
    // Chances are rounded: MaximoInt(10, MinimoInt(90, 50 + (PoderAtaque - UserPoderEvasion) * 0.4))
    const double diff = static_cast<double>(PoderAtacante - PoderDefensor);
    const auto calculo = static_cast<std::int32_t>(std::floor(50.0 + (diff * 0.4)));
    return MaximoInt(10, MinimoInt(90, static_cast<std::int16_t>(calculo)));
}

bool UserImpactoUser(std::int16_t AtacanteIndex, std::int16_t VictimaIndex) {
    if (!IsValidUser(AtacanteIndex) || !IsValidUser(VictimaIndex)) {
        return false;
    }

    const auto& atacante = UserList[AtacanteIndex];
    const auto& victima = UserList[VictimaIndex];

    std::int32_t user_poder_evasion = PoderEvasion(VictimaIndex);
    if (victima.Invent.EscudoEqpObjIndex > 0) {
        user_poder_evasion += PoderEvasionEscudo(VictimaIndex);
    }

    std::int32_t poder_ataque = 0;
    const auto weapon_idx = atacante.Invent.WeaponEqpObjIndex;
    if (weapon_idx > 0 && IsValidObj(weapon_idx)) {
        if (ObjDataList[weapon_idx].proyectil == 1) {
            poder_ataque = PoderAtaqueProyectil(AtacanteIndex);
        } else {
            poder_ataque = PoderAtaqueArma(AtacanteIndex);
        }
    } else {
        poder_ataque = PoderAtaqueWrestling(AtacanteIndex);
    }

    std::int32_t prob_exito = ProbExito(poder_ataque, user_poder_evasion);

    // Si la víctima está meditando, se reduce la evasión un 25% directo
    if (victima.flags.Meditando) {
        const double prob_evadir = static_cast<double>(100 - prob_exito) * 0.75;
        prob_exito = MinimoInt(90, static_cast<std::int16_t>(std::floor(100.0 - prob_evadir)));
    }

    const auto tirada = GetCombatRandom(1, 100);
    const bool impacto = (tirada <= prob_exito);

    // Si falló y la víctima usa escudo, se evalúa si fue rechazo activo
    if (!impacto && victima.Invent.EscudoEqpObjIndex > 0) {
        const auto skill_tacticas = static_cast<std::int32_t>(victima.Stats.UserSkills[eSkill::Tacticas]);
        const auto skill_defensa = static_cast<std::int32_t>(victima.Stats.UserSkills[eSkill::Defensa]);

        if (skill_defensa + skill_tacticas > 0) {
            const double ratio = (100.0 * static_cast<double>(skill_defensa)) / static_cast<double>(skill_defensa + skill_tacticas);
            const auto prob_rechazo = MaximoInt(10, MinimoInt(90, static_cast<std::int16_t>(std::floor(ratio))));
            const auto tirada_escudo = GetCombatRandom(1, 100);
            const bool rechazo = (tirada_escudo <= prob_rechazo);
            (void)rechazo; // En Fase 4 se despachará audio y mensajes
        }
    }

    return impacto;
}

bool UserImpactoNpc(std::int16_t UserIndex, std::int16_t NpcIndex) {
    if (!IsValidUser(UserIndex) || !IsValidNpc(NpcIndex)) {
        return false;
    }

    const auto& user = UserList[UserIndex];
    std::int32_t poder_ataque = 0;
    const auto weapon_idx = user.Invent.WeaponEqpObjIndex;
    if (weapon_idx > 0 && IsValidObj(weapon_idx)) {
        if (ObjDataList[weapon_idx].proyectil == 1) {
            poder_ataque = PoderAtaqueProyectil(UserIndex);
        } else {
            poder_ataque = PoderAtaqueArma(UserIndex);
        }
    } else {
        poder_ataque = PoderAtaqueWrestling(UserIndex);
    }

    const auto npc_evasion = Npclist[NpcIndex].PoderEvasion;
    const auto prob_exito = ProbExito(poder_ataque, npc_evasion);

    return GetCombatRandom(1, 100) <= prob_exito;
}

bool NpcImpactoUser(std::int16_t NpcIndex, std::int16_t UserIndex) {
    if (!IsValidNpc(NpcIndex) || !IsValidUser(UserIndex)) {
        return false;
    }

    const auto& user = UserList[UserIndex];
    std::int32_t user_evasion = PoderEvasion(UserIndex);
    if (user.Invent.EscudoEqpObjIndex > 0) {
        user_evasion += PoderEvasionEscudo(UserIndex);
    }

    const auto npc_poder_ataque = Npclist[NpcIndex].PoderAtaque;
    const auto prob_exito = ProbExito(npc_poder_ataque, user_evasion);

    const auto tirada = GetCombatRandom(1, 100);
    const bool impacto = (tirada <= prob_exito);

    if (!impacto && user.Invent.EscudoEqpObjIndex > 0) {
        const auto skill_tacticas = static_cast<std::int32_t>(user.Stats.UserSkills[eSkill::Tacticas]);
        const auto skill_defensa = static_cast<std::int32_t>(user.Stats.UserSkills[eSkill::Defensa]);

        if (skill_defensa + skill_tacticas > 0) {
            const double ratio = (100.0 * static_cast<double>(skill_defensa)) / static_cast<double>(skill_defensa + skill_tacticas);
            const auto prob_rechazo = MaximoInt(10, MinimoInt(90, static_cast<std::int16_t>(std::floor(ratio))));
            const auto tirada_escudo = GetCombatRandom(1, 100);
            const bool rechazo = (tirada_escudo <= prob_rechazo);
            (void)rechazo;
        }
    }

    return impacto;
}

bool NpcImpactoNpc(std::int16_t Atacante, std::int16_t Victima) {
    if (!IsValidNpc(Atacante) || !IsValidNpc(Victima)) {
        return false;
    }

    const auto poder_att = Npclist[Atacante].PoderAtaque;
    const auto poder_eva = Npclist[Victima].PoderEvasion;
    const auto prob_exito = ProbExito(poder_att, poder_eva);

    return GetCombatRandom(1, 100) <= prob_exito;
}

std::int32_t NpcDaño(std::int16_t NpcIndex) {
    if (!IsValidNpc(NpcIndex)) {
        return 0;
    }
    const auto& stats = Npclist[NpcIndex].Stats;
    return GetCombatRandom(stats.MinHIT, stats.MaxHIT);
}

std::int32_t UserImpacto(std::int16_t UserIndex) {
    return PoderAtaqueModificado(UserIndex);
}

std::int32_t NpcImpacto(std::int16_t NpcIndex) {
    if (!IsValidNpc(NpcIndex)) {
        return 0;
    }
    return Npclist[NpcIndex].PoderAtaque;
}

std::int32_t NpcEvasion(std::int16_t NpcIndex) {
    if (!IsValidNpc(NpcIndex)) {
        return 0;
    }
    return Npclist[NpcIndex].PoderEvasion;
}

// ============================================================================
// Deducción de Daño, Absorciones y Peculiaridades (Fase 3: G3)
// ============================================================================

void SetCombatQuitarObjetosHook(QuitarObjetosHook hook) noexcept {
    s_quitar_objetos_hook = std::move(hook);
}

void SetDoApuñalarHook(DoApuñalarHook hook) noexcept {
    s_apuñalar_hook = std::move(hook);
}

void SetDoGolpeCriticoHook(DoGolpeCriticoHook hook) noexcept {
    s_golpe_critico_hook = std::move(hook);
}

void SetDoAcuchillarHook(DoAcuchillarHook hook) noexcept {
    s_acuchillar_hook = std::move(hook);
}

void SetUserDieHook(UserDieHook hook) noexcept {
    s_user_die_hook = std::move(hook);
}

void SetMuereNpcHook(MuereNpcHook hook) noexcept {
    s_muere_npc_hook = std::move(hook);
}

void SetCombatSubirSkillHook(SubirSkillHook hook) noexcept {
    s_subir_skill_hook = std::move(hook);
}

void SetCheckUserLevelHook(CheckUserLevelHook hook) noexcept {
    s_check_user_level_hook = std::move(hook);
}

void SetPartyExpHook(PartyExpHook hook) noexcept {
    s_party_exp_hook = std::move(hook);
}

void SetRefreshCharStatusHook(RefreshCharStatusHook hook) noexcept {
    s_refresh_char_status_hook = std::move(hook);
}

void SetVolverCriminalHook(VolverCriminalHook hook) noexcept {
    s_volver_criminal_hook = std::move(hook);
}

void SetCancelExitHook(CancelExitHook hook) noexcept {
    s_cancel_exit_hook = std::move(hook);
}

void SetStoreFragHook(StoreFragHook hook) noexcept {
    s_store_frag_hook = std::move(hook);
}

void SetContarMuerteHook(ContarMuerteHook hook) noexcept {
    s_contar_muerte_hook = std::move(hook);
}

void ResetCombatHooks() noexcept {
    s_quitar_objetos_hook = nullptr;
    s_apuñalar_hook = nullptr;
    s_golpe_critico_hook = nullptr;
    s_acuchillar_hook = nullptr;
    s_user_die_hook = nullptr;
    s_muere_npc_hook = nullptr;
    s_subir_skill_hook = nullptr;
    s_check_user_level_hook = nullptr;
    s_party_exp_hook = nullptr;
    s_refresh_char_status_hook = nullptr;
    s_volver_criminal_hook = nullptr;
    s_cancel_exit_hook = nullptr;
    s_store_frag_hook = nullptr;
    s_contar_muerte_hook = nullptr;
    s_random_provider = nullptr;
    s_time_provider = nullptr;
}

bool PuedeApuñalar(std::int16_t user_index) {
    if (!IsValidUser(user_index)) {
        return false;
    }
    const auto weapon_idx = UserList[user_index].Invent.WeaponEqpObjIndex;
    if (weapon_idx > 0 && IsValidObj(weapon_idx)) {
        if (ObjDataList[weapon_idx].Apuñala == 1) {
            return UserList[user_index].Stats.UserSkills[eSkill::Apuñalar] >= MIN_APUÑALAR ||
                   UserList[user_index].clase == eClass::Assasin;
        }
    }
    return false;
}

bool PuedeAcuchillar(std::int16_t user_index) {
    if (!IsValidUser(user_index)) {
        return false;
    }
    if (UserList[user_index].clase == eClass::Pirat) {
        const auto weapon_idx = UserList[user_index].Invent.WeaponEqpObjIndex;
        if (weapon_idx > 0 && IsValidObj(weapon_idx)) {
            return ObjDataList[weapon_idx].Acuchilla == 1;
        }
    }
    return false;
}

std::int32_t UserDañoNpc(std::int16_t user_index, std::int16_t npc_index) {
    if (!IsValidUser(user_index) || !IsValidNpc(npc_index)) {
        return 0;
    }

    auto& user = UserList[user_index];
    auto& npc = Npclist[npc_index];

    std::int32_t daño_base = 0;
    std::int32_t daño = 0;
    const auto weapon_idx = user.Invent.WeaponEqpObjIndex;

    // Criterio 7.1 (SistemaCombate.bas:502-510):
    // Contra DRAGON: daño letal exacto (MinHp + def). Contra otro objetivo: daño fijo en 1.
    if (weapon_idx == EspadaMataDragonesIndex) {
        if (npc.NPCtype == eNPCType::DRAGON) {
            daño_base = npc.Stats.MinHp + npc.Stats.def;
            daño = daño_base;
        } else {
            daño_base = 1;
            daño = 1;
        }
    } else {
        daño_base = CalcularDaño(user_index, npc_index);

        if (user.flags.Navegando == 1 && user.Invent.BarcoObjIndex > 0 && IsValidObj(user.Invent.BarcoObjIndex)) {
            const auto& barco = ObjDataList[user.Invent.BarcoObjIndex];
            daño_base += GetCombatRandom(barco.MinHIT, barco.MaxHIT);
        }

        daño = daño_base - npc.Stats.def;
        if (daño < 0) {
            daño = 0;
        }
    }

    CalcularDarExp(user_index, npc_index, daño);
    npc.Stats.MinHp -= daño;

    if (npc.Stats.MinHp > 0) {
        if (PuedeApuñalar(user_index)) {
            // Criterio 7.2 (SistemaCombate.bas:549 — ZaMa 07/04/2010): En PvE apuñala acorde al daño base sin descontar defensa
            if (s_apuñalar_hook) {
                s_apuñalar_hook(user_index, 0, npc_index, daño_base);
            }
        }
        if (s_golpe_critico_hook) {
            s_golpe_critico_hook(user_index, 0, npc_index, daño);
        }
        if (PuedeAcuchillar(user_index)) {
            if (s_acuchillar_hook) {
                s_acuchillar_hook(user_index, 0, npc_index, daño);
            }
        }
    } else {
        // Criterio 7.1 (SistemaCombate.bas:560-562): Autodestrucción inmediata de la Espada Mata Dragones al liquidar al dragón
        if (npc.NPCtype == eNPCType::DRAGON && weapon_idx == EspadaMataDragonesIndex) {
            if (s_quitar_objetos_hook) {
                s_quitar_objetos_hook(EspadaMataDragonesIndex, 1, user_index);
            }
        }

        for (int j = 1; j <= MAXMASCOTAS; ++j) {
            const auto pet_idx = user.MascotasIndex[j];
            if (pet_idx > 0 && IsValidNpc(pet_idx)) {
                if (Npclist[pet_idx].TargetNPC == npc_index) {
                    Npclist[pet_idx].TargetNPC = 0;
                    Npclist[pet_idx].Movement = TipoAI::SigueAmo;
                }
            }
        }

        if (s_muere_npc_hook) {
            s_muere_npc_hook(npc_index, user_index);
        }
    }

    return daño;
}

std::int32_t UserDañoUser(std::int16_t atacante_index, std::int16_t victima_index) {
    if (!IsValidUser(atacante_index) || !IsValidUser(victima_index)) {
        return 0;
    }

    auto& atacante = UserList[atacante_index];
    auto& victima = UserList[victima_index];

    std::int32_t daño = 0;
    const auto weapon_idx = atacante.Invent.WeaponEqpObjIndex;

    // Criterio 7.1 en PvP: Si porta la Mata Dragones contra un usuario, el daño se fija exactamente en 1
    if (weapon_idx == EspadaMataDragonesIndex) {
        daño = 1;
    } else {
        daño = CalcularDaño(atacante_index, 0);

        if (atacante.flags.Navegando == 1 && atacante.Invent.BarcoObjIndex > 0 && IsValidObj(atacante.Invent.BarcoObjIndex)) {
            const auto& barco = ObjDataList[atacante.Invent.BarcoObjIndex];
            daño += GetCombatRandom(barco.MinHIT, barco.MaxHIT);
        }

        std::int32_t defbarco = 0;
        if (victima.flags.Navegando == 1 && victima.Invent.BarcoObjIndex > 0 && IsValidObj(victima.Invent.BarcoObjIndex)) {
            const auto& barco_def = ObjDataList[victima.Invent.BarcoObjIndex];
            defbarco = GetCombatRandom(barco_def.MinDef, barco_def.MaxDef);
        }

        std::int32_t resist = 0;
        if (weapon_idx > 0 && IsValidObj(weapon_idx)) {
            resist = ObjDataList[weapon_idx].Refuerzo;
        }

        const auto lugar = static_cast<PartesCuerpo>(GetCombatRandom(static_cast<std::int32_t>(PartesCuerpo::bCabeza),
                                                                    static_cast<std::int32_t>(PartesCuerpo::bTorso)));
        std::int32_t absorbido = 0;

        switch (lugar) {
            case PartesCuerpo::bCabeza: {
                const auto casco_idx = victima.Invent.CascoEqpObjIndex;
                if (casco_idx > 0 && IsValidObj(casco_idx)) {
                    const auto& casco = ObjDataList[casco_idx];
                    absorbido = GetCombatRandom(casco.MinDef, casco.MaxDef);
                    absorbido = absorbido + defbarco - resist;
                    if (absorbido < 0) absorbido = 0;
                    daño = daño - absorbido;
                }
                break;
            }
            default: { // bTorso
                const auto armour_idx = victima.Invent.ArmourEqpObjIndex;
                if (armour_idx > 0 && IsValidObj(armour_idx)) {
                    const auto& armour = ObjDataList[armour_idx];
                    const auto escudo_idx = victima.Invent.EscudoEqpObjIndex;
                    if (escudo_idx > 0 && IsValidObj(escudo_idx)) {
                        const auto& escudo = ObjDataList[escudo_idx];
                        absorbido = GetCombatRandom(armour.MinDef + escudo.MinDef, armour.MaxDef + escudo.MaxDef);
                    } else {
                        absorbido = GetCombatRandom(armour.MinDef, armour.MaxDef);
                    }
                    absorbido = absorbido + defbarco - resist;
                    if (absorbido < 0) absorbido = 0;
                    daño = daño - absorbido;
                }
                break;
            }
        }

        // Guarda contra absorción excesiva: el daño nunca puede ser menor a 0
        if (daño < 0) {
            daño = 0;
        }
    }

    UserEnvenena(atacante_index, victima_index);

    victima.Stats.MinHp -= daño;

    if (atacante.flags.Hambre == 0 && atacante.flags.Sed == 0) {
        if (weapon_idx > 0 && IsValidObj(weapon_idx)) {
            if (ObjDataList[weapon_idx].proyectil == 1) {
                if (s_subir_skill_hook) s_subir_skill_hook(atacante_index, eSkill::Proyectiles, true);
                if (ObjDataList[weapon_idx].Municion == 0 && ObjDataList[weapon_idx].Acuchilla == 1) {
                    if (s_acuchillar_hook) s_acuchillar_hook(atacante_index, victima_index, 0, daño);
                }
            } else {
                if (s_subir_skill_hook) s_subir_skill_hook(atacante_index, eSkill::Armas, true);
            }
        } else {
            if (s_subir_skill_hook) s_subir_skill_hook(atacante_index, eSkill::Wrestling, true);
        }

        if (PuedeApuñalar(atacante_index)) {
            // Criterio 7.2 (SistemaCombate.bas:1145 — ZaMa 07/04/2010): En PvP DoApuñalar recibe daño NETO post-absorción (daño)
            if (s_apuñalar_hook) {
                s_apuñalar_hook(atacante_index, victima_index, 0, daño);
            }
        }
        if (s_golpe_critico_hook) {
            s_golpe_critico_hook(atacante_index, victima_index, 0, daño);
        }
    }

    if (victima.Stats.MinHp <= 0) {
        // Criterio 7.3 (SistemaCombate.bas:1249-1254): Legítima Defensa — Si victima.flags.AtacablePor == atacante_index, no registra frag ni cuenta muerte penalizada
        if (victima.flags.AtacablePor != atacante_index) {
            if (s_store_frag_hook) {
                s_store_frag_hook(atacante_index, victima_index);
            }
            if (s_contar_muerte_hook) {
                s_contar_muerte_hook(victima_index, atacante_index);
            }
        }

        for (int j = 1; j <= MAXMASCOTAS; ++j) {
            const auto pet_idx = atacante.MascotasIndex[j];
            if (pet_idx > 0 && IsValidNpc(pet_idx)) {
                if (Npclist[pet_idx].Target == victima_index) {
                    Npclist[pet_idx].Target = 0;
                    AllFollowAmo(atacante_index);
                }
            }
        }

        if (s_user_die_hook) {
            s_user_die_hook(victima_index);
        }
    }

    if (s_check_user_level_hook) {
        s_check_user_level_hook(atacante_index);
    }

    return daño;
}

std::int32_t NpcDañoUser(std::int16_t npc_index, std::int16_t user_index) {
    if (!IsValidNpc(npc_index) || !IsValidUser(user_index)) {
        return 0;
    }

    const auto& npc = Npclist[npc_index];
    auto& user = UserList[user_index];

    std::int32_t daño = GetCombatRandom(npc.Stats.MinHIT, npc.Stats.MaxHIT);

    std::int32_t defbarco = 0;
    if (user.flags.Navegando == 1 && user.Invent.BarcoObjIndex > 0 && IsValidObj(user.Invent.BarcoObjIndex)) {
        const auto& barco = ObjDataList[user.Invent.BarcoObjIndex];
        defbarco = GetCombatRandom(barco.MinDef, barco.MaxDef);
    }

    const auto lugar = static_cast<PartesCuerpo>(GetCombatRandom(static_cast<std::int32_t>(PartesCuerpo::bCabeza),
                                                                static_cast<std::int32_t>(PartesCuerpo::bTorso)));
    std::int32_t absorbido = 0;

    switch (lugar) {
        case PartesCuerpo::bCabeza: {
            const auto casco_idx = user.Invent.CascoEqpObjIndex;
            if (casco_idx > 0 && IsValidObj(casco_idx)) {
                const auto& casco = ObjDataList[casco_idx];
                absorbido = GetCombatRandom(casco.MinDef, casco.MaxDef);
            }
            break;
        }
        default: { // bTorso
            const auto armour_idx = user.Invent.ArmourEqpObjIndex;
            if (armour_idx > 0 && IsValidObj(armour_idx)) {
                const auto& armour = ObjDataList[armour_idx];
                const auto escudo_idx = user.Invent.EscudoEqpObjIndex;
                if (escudo_idx > 0 && IsValidObj(escudo_idx)) {
                    const auto& escudo = ObjDataList[escudo_idx];
                    absorbido = GetCombatRandom(armour.MinDef + escudo.MinDef, armour.MaxDef + escudo.MaxDef);
                } else {
                    absorbido = GetCombatRandom(armour.MinDef, armour.MaxDef);
                }
            }
            break;
        }
    }

    absorbido += defbarco;
    daño -= absorbido;
    if (daño < 1) {
        daño = 1;
    }

    user.Stats.MinHp -= daño;

    if (user.flags.Meditando) {
        user.flags.Meditando = false;
    }

    if (user.Stats.MinHp <= 0) {
        if (FileIO::criminal(user_index) && npc.NPCtype == eNPCType::GuardiaReal) {
            RestarCriminalidad(user_index);
        }
        if (s_user_die_hook) {
            s_user_die_hook(user_index);
        }
    }

    return daño;
}

std::int32_t NpcDaño(std::int16_t npc_index, std::int16_t user_index) {
    return NpcDañoUser(npc_index, user_index);
}

std::int32_t NpcDañoNpc(std::int16_t atacante_index, std::int16_t victima_index) {
    if (!IsValidNpc(atacante_index) || !IsValidNpc(victima_index)) {
        return 0;
    }

    auto& atacante = Npclist[atacante_index];
    auto& victima = Npclist[victima_index];

    const auto daño = GetCombatRandom(atacante.Stats.MinHIT, atacante.Stats.MaxHIT);
    victima.Stats.MinHp -= daño;

    if (victima.Stats.MinHp < 1) {
        atacante.Movement = atacante.flags.OldMovement;
        if (!atacante.flags.AttackedBy.empty()) {
            atacante.Hostile = atacante.flags.OldHostil;
        }
        if (s_muere_npc_hook) {
            s_muere_npc_hook(victima_index, atacante.MaestroUser);
        }
    }

    return daño;
}

void CalcularDarExp(std::int16_t user_index, std::int16_t npc_index, std::int32_t daño) {
    if (!IsValidUser(user_index) || !IsValidNpc(npc_index)) {
        return;
    }
    if (daño <= 0) {
        return;
    }

    auto& npc = Npclist[npc_index];
    if (npc.Stats.MaxHp <= 0) {
        return;
    }

    if (daño > npc.Stats.MinHp) {
        daño = npc.Stats.MinHp;
    }

    // VB6: ExpaDar = CLng(ElDaño * (Npclist(NpcIndex).GiveEXP / Npclist(NpcIndex).Stats.MaxHp))
    const double exp_ratio = static_cast<double>(npc.GiveEXP) / static_cast<double>(npc.Stats.MaxHp);
    auto exp_a_dar = static_cast<std::int32_t>(std::floor(static_cast<double>(daño) * exp_ratio));
    if (exp_a_dar <= 0) {
        return;
    }

    if (exp_a_dar > npc.flags.ExpCount) {
        exp_a_dar = npc.flags.ExpCount;
        npc.flags.ExpCount = 0;
    } else {
        npc.flags.ExpCount -= exp_a_dar;
    }

    if (exp_a_dar > 0) {
        auto& user = UserList[user_index];
        if (user.PartyIndex > 0) {
            if (s_party_exp_hook) {
                s_party_exp_hook(user_index, exp_a_dar, npc.Pos.Map, npc.Pos.X, npc.Pos.Y);
            }
        } else {
            user.Stats.Exp += exp_a_dar;
            if (user.Stats.Exp > MAXEXP) {
                user.Stats.Exp = MAXEXP;
            }
        }

        if (s_check_user_level_hook) {
            s_check_user_level_hook(user_index);
        }
    }
}

void RestarCriminalidad(std::int16_t user_index) {
    if (!IsValidUser(user_index)) {
        return;
    }

    const bool era_criminal = FileIO::criminal(user_index);
    auto& rep = UserList[user_index].Reputacion;

    if (rep.BandidoRep > 0) {
        rep.BandidoRep -= vlASALTO;
        if (rep.BandidoRep < 0) {
            rep.BandidoRep = 0;
        }
    } else if (rep.LadronesRep > 0) {
        rep.LadronesRep -= (vlCAZADOR * 10);
        if (rep.LadronesRep < 0) {
            rep.LadronesRep = 0;
        }
    }

    if (era_criminal && !FileIO::criminal(user_index)) {
        if (s_refresh_char_status_hook) {
            s_refresh_char_status_hook(user_index);
        }
    }
}

void UserEnvenena(std::int16_t user_index, std::int16_t victima_index) {
    if (!IsValidUser(user_index) || !IsValidUser(victima_index)) {
        return;
    }

    auto obj_idx = UserList[user_index].Invent.WeaponEqpObjIndex;
    if (obj_idx > 0 && IsValidObj(obj_idx)) {
        if (ObjDataList[obj_idx].proyectil == 1) {
            obj_idx = UserList[user_index].Invent.MunicionEqpObjIndex;
        }

        if (obj_idx > 0 && IsValidObj(obj_idx)) {
            if (ObjDataList[obj_idx].Envenena == 1) {
                // Probabilidad 60%: tirada en [1, 100] <= 60
                if (GetCombatRandom(1, 100) <= 60) {
                    UserList[victima_index].flags.Envenenado = 1;
                }
            }
        }
    }
}

// ============================================================================
// Orquestación de Flujo de Ataque, Protocolo y Hooks (Fase 4: G4)
// ============================================================================

namespace {

void DispatchSwingSound(std::int16_t user_index, const WorldPos& pos) {
    auto wave = ao::net::protocol::PrepareMessagePlayWave(SND_SWING, pos.X, pos.Y);
    std::string_view wave_view(reinterpret_cast<const char*>(wave.data()), wave.size());
    // Criterio 7.4 (SistemaCombate.bas:1205-1210): Sigilo GM invisible — si es AdminInvisible se envía únicamente a su socket
    if (UserList[user_index].flags.AdminInvisible == 1) {
        TCP::EnviarDatosASlot(user_index, wave_view);
    } else {
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, user_index, wave);
    }
}

} // namespace

void UsuarioAtaca(std::int16_t user_index) {
    if (!IsValidUser(user_index)) return;

    auto& user = UserList[user_index];

    if (!IntervaloPermiteAtacar(user_index, true)) {
        return;
    }

    if (user.flags.Muerto == 1) {
        ao::net::protocol::WriteConsoleMsg(user_index, "¡Estás muerto!!", FontTypeNames::FONTTYPE_INFO);
        return;
    }

    // Quitamos stamina
    if (user.Stats.MinSta >= 10) {
        const auto sta_quitar = GetCombatRandom(1, 10);
        user.Stats.MinSta -= sta_quitar;
        if (user.Stats.MinSta < 0) user.Stats.MinSta = 0;
    } else {
        if (user.Genero == eGenero::Hombre) {
            ao::net::protocol::WriteConsoleMsg(user_index, "Estás muy cansado para luchar.", FontTypeNames::FONTTYPE_INFO);
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "Estás muy cansada para luchar.", FontTypeNames::FONTTYPE_INFO);
        }
        return;
    }

    WorldPos attack_pos = user.Pos;
    HeadtoPos(user.char_appearance.heading, attack_pos);

    if (attack_pos.X < XMinMapSize || attack_pos.X > XMaxMapSize || attack_pos.Y < YMinMapSize || attack_pos.Y > YMaxMapSize) {
        DispatchSwingSound(user_index, user.Pos);
        return;
    }

    const auto target_user = GetMapBlock(attack_pos.Map, attack_pos.X, attack_pos.Y).UserIndex;
    if (target_user > 0) {
        UsuarioAtacaUsuario(user_index, target_user);
        return;
    }

    const auto target_npc = GetMapBlock(attack_pos.Map, attack_pos.X, attack_pos.Y).NpcIndex;
    if (target_npc > 0) {
        if (Npclist[target_npc].Attackable != 0) {
            const auto map_idx = Npclist[target_npc].Pos.Map;
            if (Npclist[target_npc].MaestroUser > 0 && map_idx > 0 && static_cast<std::size_t>(map_idx) < MapInfoList.size() && !MapInfoList[map_idx].Pk) {
                ao::net::protocol::WriteConsoleMsg(user_index, "No puedes atacar mascotas en zona segura.", FontTypeNames::FONTTYPE_FIGHT);
                return;
            }
            UsuarioAtacaNpc(user_index, target_npc);
        } else {
            ao::net::protocol::WriteConsoleMsg(user_index, "No puedes atacar a este NPC.", FontTypeNames::FONTTYPE_FIGHT);
        }
        return;
    }

    // Ataque al aire
    DispatchSwingSound(user_index, user.Pos);
    if (user.Counters.Trabajando > 0) user.Counters.Trabajando--;
    if (user.Counters.Ocultando > 0) user.Counters.Ocultando--;
}

bool UsuarioAtacaUsuario(std::int16_t atacante_index, std::int16_t victima_index) {
    if (!IsValidUser(atacante_index) || !IsValidUser(victima_index)) {
        return false;
    }

    if (!PuedeAtacar(atacante_index, victima_index)) {
        return false;
    }

    auto& atacante = UserList[atacante_index];
    auto& victima = UserList[victima_index];

    if (Distancia(atacante.Pos, victima.Pos) > MAXDISTANCIAARCO) {
        ao::net::protocol::WriteConsoleMsg(atacante_index, "Estás muy lejos para disparar.", FontTypeNames::FONTTYPE_FIGHT);
        return false;
    }

    UsuarioAtacadoPorUsuario(atacante_index, victima_index);

    if (UserImpactoUser(atacante_index, victima_index)) {
        auto wave = ao::net::protocol::PrepareMessagePlayWave(SND_IMPACTO, atacante.Pos.X, atacante.Pos.Y);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, atacante_index, wave);

        if (victima.flags.Navegando == 0) {
            auto fx = ao::net::protocol::PrepareMessageCreateFX(victima.char_appearance.CharIndex, FXSANGRE, 0);
            ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, victima_index, fx);
        }

        UserDañoUser(atacante_index, victima_index);
    } else {
        DispatchSwingSound(atacante_index, atacante.Pos);
        ao::net::protocol::WriteMultiMessage(atacante_index, eMessages::UserSwing);
        ao::net::protocol::WriteMultiMessage(victima_index, eMessages::UserAttackedSwing, atacante_index);
    }

    return true;
}

void UsuarioAtacadoPorUsuario(std::int16_t atacante_index, std::int16_t victima_index) {
    if (!IsValidUser(atacante_index) || !IsValidUser(victima_index)) {
        return;
    }

    if (TriggerZonaPelea(atacante_index, victima_index) == eTrigger6::TRIGGER6_PERMITE) {
        return;
    }

    const bool era_criminal = FileIO::criminal(atacante_index);
    bool victima_es_atacable = false;

    if (!FileIO::criminal(atacante_index)) {
        if (!FileIO::criminal(victima_index)) {
            // Criterio 7.3 (SistemaCombate.bas:1456-1460): Legítima Defensa — Si la víctima está en estado atacable por el atacante, no se vuelve criminal
            victima_es_atacable = (UserList[victima_index].flags.AtacablePor == atacante_index);
            if (!victima_es_atacable) {
                if (s_volver_criminal_hook) {
                    s_volver_criminal_hook(atacante_index);
                }
            }
        }
    }

    auto& victima = UserList[victima_index];
    if (victima.flags.Meditando) {
        victima.flags.Meditando = false;
        ao::net::protocol::WriteMeditateToggle(victima_index);
        ao::net::protocol::WriteConsoleMsg(victima_index, "Dejas de meditar.", FontTypeNames::FONTTYPE_INFO);
        victima.char_appearance.FX = 0;
        victima.char_appearance.loops = 0;
        auto fx = ao::net::protocol::PrepareMessageCreateFX(victima.char_appearance.CharIndex, 0, 0);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, victima_index, fx);
    }

    // Criterio 7.3 (SistemaCombate.bas:1463-1473): Si ataca a alguien atacable, no suma puntos de bandido ni reduce nobleza
    if (!victima_es_atacable) {
        auto& rep = UserList[atacante_index].Reputacion;
        if (!FileIO::criminal(victima_index)) {
            rep.BandidoRep += vlASALTO;
            if (rep.BandidoRep > MAXREP) rep.BandidoRep = MAXREP;

            rep.NobleRep = static_cast<std::int32_t>(rep.NobleRep * 0.5);
            if (rep.NobleRep < 0) rep.NobleRep = 0;
        } else {
            rep.NobleRep += vlNoble;
            if (rep.NobleRep > MAXREP) rep.NobleRep = MAXREP;
        }
    }

    if (FileIO::criminal(atacante_index)) {
        if (!era_criminal) {
            if (s_refresh_char_status_hook) s_refresh_char_status_hook(atacante_index);
        }
    } else if (era_criminal) {
        if (s_refresh_char_status_hook) s_refresh_char_status_hook(atacante_index);
    }

    AllMascotasAtacanUser(atacante_index, victima_index);
    AllMascotasAtacanUser(victima_index, atacante_index);

    if (s_cancel_exit_hook) {
        s_cancel_exit_hook(victima_index);
    }
}

bool UsuarioAtacaNpc(std::int16_t user_index, std::int16_t npc_index) {
    if (!IsValidUser(user_index) || !IsValidNpc(npc_index)) {
        return false;
    }

    if (!PuedeAtacarNPC(user_index, npc_index)) {
        return false;
    }

    auto& user = UserList[user_index];
    auto& npc = Npclist[npc_index];

    if (UserImpactoNpc(user_index, npc_index)) {
        const auto snd = (npc.flags.Snd2 > 0) ? npc.flags.Snd2 : SND_IMPACTO2;
        auto wave = ao::net::protocol::PrepareMessagePlayWave(snd, npc.Pos.X, npc.Pos.Y);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, user_index, wave);

        UserDañoNpc(user_index, npc_index);
    } else {
        DispatchSwingSound(user_index, user.Pos);
        ao::net::protocol::WriteMultiMessage(user_index, eMessages::UserSwing);
    }

    user.flags.Ignorado = false;
    return true;
}

bool NpcAtacaUser(std::int16_t npc_index, std::int16_t user_index) {
    if (!IsValidNpc(npc_index) || !IsValidUser(user_index)) {
        return false;
    }

    auto& user = UserList[user_index];
    auto& npc = Npclist[npc_index];

    if (user.flags.AdminInvisible == 1) {
        return false;
    }

    if (npc.CanAttack == 1) {
        if (npc.Target == 0) npc.Target = user_index;
        if (user.flags.AtacadoPorNpc == 0 && user.flags.AtacadoPorUser == 0) {
            user.flags.AtacadoPorNpc = npc_index;
        }
    } else {
        return false;
    }

    npc.CanAttack = 0;

    if (npc.flags.Snd1 > 0) {
        auto wave = ao::net::protocol::PrepareMessagePlayWave(npc.flags.Snd1, npc.Pos.X, npc.Pos.Y);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToNPCArea, npc_index, wave);
    }

    if (NpcImpactoUser(npc_index, user_index)) {
        auto wave = ao::net::protocol::PrepareMessagePlayWave(SND_IMPACTO, user.Pos.X, user.Pos.Y);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, user_index, wave);

        if (!user.flags.Meditando && user.flags.Navegando == 0) {
            auto fx = ao::net::protocol::PrepareMessageCreateFX(user.char_appearance.CharIndex, FXSANGRE, 0);
            ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToPCArea, user_index, fx);
        }

        NpcDañoUser(npc_index, user_index);
    } else {
        ao::net::protocol::WriteMultiMessage(user_index, eMessages::NPCSwing);
    }

    return true;
}

void NpcAtacaNpc(std::int16_t atacante_index, std::int16_t victima_index, bool cambiarMovimiento) {
    if (!IsValidNpc(atacante_index) || !IsValidNpc(victima_index)) {
        return;
    }

    auto& atacante = Npclist[atacante_index];
    auto& victima = Npclist[victima_index];

    if (atacante.CanAttack == 1) {
        atacante.CanAttack = 0;
        if (cambiarMovimiento) {
            victima.TargetNPC = atacante_index;
            victima.Movement = TipoAI::NpcAtacaNpc;
        }
    } else {
        return;
    }

    if (atacante.flags.Snd1 > 0) {
        auto wave = ao::net::protocol::PrepareMessagePlayWave(atacante.flags.Snd1, atacante.Pos.X, atacante.Pos.Y);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToNPCArea, atacante_index, wave);
    }

    if (NpcImpactoNpc(atacante_index, victima_index)) {
        const auto snd2 = (victima.flags.Snd2 > 0) ? victima.flags.Snd2 : SND_IMPACTO2;
        auto wave = ao::net::protocol::PrepareMessagePlayWave(snd2, victima.Pos.X, victima.Pos.Y);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToNPCArea, victima_index, wave);

        NpcDañoNpc(atacante_index, victima_index);
    } else {
        auto wave = ao::net::protocol::PrepareMessagePlayWave(SND_SWING, victima.Pos.X, victima.Pos.Y);
        ao::net::send_data::SendData(ao::net::send_data::SendTarget::ToNPCArea, victima_index, wave);
    }
}

bool PuedeAtacar(std::int16_t atacante_index, std::int16_t victima_index) {
    if (!IsValidUser(atacante_index) || !IsValidUser(victima_index)) {
        return false;
    }

    auto& atacante = UserList[atacante_index];
    auto& victima = UserList[victima_index];

    if (atacante.flags.Muerto == 1) {
        ao::net::protocol::WriteConsoleMsg(atacante_index, "¡Estás muerto!!", FontTypeNames::FONTTYPE_INFO);
        return false;
    }

    if (victima.flags.Muerto == 1) {
        ao::net::protocol::WriteConsoleMsg(atacante_index, "No puedes atacar a un espíritu.", FontTypeNames::FONTTYPE_INFO);
        return false;
    }

    if (atacante.flags.EnConsulta) {
        ao::net::protocol::WriteConsoleMsg(atacante_index, "No puedes atacar usuarios mientras estas en consulta.", FontTypeNames::FONTTYPE_INFO);
        return false;
    }

    if (victima.flags.EnConsulta) {
        ao::net::protocol::WriteConsoleMsg(atacante_index, "No puedes atacar usuarios mientras estan en consulta.", FontTypeNames::FONTTYPE_INFO);
        return false;
    }

    switch (TriggerZonaPelea(atacante_index, victima_index)) {
        case eTrigger6::TRIGGER6_PERMITE:
            return (victima.flags.AdminInvisible == 0);
        case eTrigger6::TRIGGER6_PROHIBE:
            return false;
        case eTrigger6::TRIGGER6_AUSENTE:
            if ((victima.flags.Privilegios & PlayerType::UserPlayer) == 0) {
                if (victima.flags.AdminInvisible == 0) {
                    ao::net::protocol::WriteConsoleMsg(atacante_index, "El ser es demasiado poderoso.", FontTypeNames::FONTTYPE_WARNING);
                }
                return false;
            }
            break;
    }

    if (!FileIO::criminal(victima_index)) {
        if (!FileIO::criminal(atacante_index)) {
            if (atacante.Faccion.ArmadaReal == 1 && victima.Faccion.ArmadaReal == 1) {
                ao::net::protocol::WriteConsoleMsg(atacante_index, "Los soldados del ejército real tienen prohibido atacar ciudadanos.", FontTypeNames::FONTTYPE_WARNING);
                return false;
            }
            // Criterio 7.3 (SistemaCombate.bas:1360-1365): Legítima Defensa — Si victima.flags.AtacablePor == atacante_index, se permite sin chequear Seguro ni facción
            if (victima.flags.AtacablePor == atacante_index) {
                return true;
            }
        }
    } else {
        if (victima.Faccion.FuerzasCaos == 1 && atacante.Faccion.FuerzasCaos == 1) {
            ao::net::protocol::WriteConsoleMsg(atacante_index, "Los miembros de la legión oscura tienen prohibido atacarse entre sí.", FontTypeNames::FONTTYPE_WARNING);
            return false;
        }
    }

    if (atacante.flags.Seguro) {
        if (!FileIO::criminal(victima_index)) {
            ao::net::protocol::WriteConsoleMsg(atacante_index, "No puedes atacar ciudadanos, para hacerlo debes desactivar el seguro.", FontTypeNames::FONTTYPE_WARNING);
            return false;
        }
    } else {
        if (!FileIO::criminal(victima_index) && atacante.Faccion.ArmadaReal == 1) {
            ao::net::protocol::WriteConsoleMsg(atacante_index, "Los soldados del ejército real tienen prohibido atacar ciudadanos.", FontTypeNames::FONTTYPE_WARNING);
            return false;
        }
    }

    const auto victim_map = victima.Pos.Map;
    if (victim_map > 0 && static_cast<std::size_t>(victim_map) < MapInfoList.size()) {
        if (!MapInfoList[victim_map].Pk) {
            if (atacante.Faccion.ArmadaReal == 1 && atacante.Faccion.RecompensasReal > 11) {
                if (victim_map == 58 || victim_map == 59 || victim_map == 60) {
                    return true;
                }
            }
            if (atacante.Faccion.FuerzasCaos == 1 && atacante.Faccion.RecompensasCaos > 11) {
                if (victim_map == 151 || victim_map == 156) {
                    return true;
                }
            }
            ao::net::protocol::WriteConsoleMsg(atacante_index, "Esta es una zona segura, aquí no puedes atacar a otros usuarios.", FontTypeNames::FONTTYPE_WARNING);
            return false;
        }
    }

    if (GetMapBlock(victima.Pos.Map, victima.Pos.X, victima.Pos.Y).trigger == eTrigger::ZONASEGURA ||
        GetMapBlock(atacante.Pos.Map, atacante.Pos.X, atacante.Pos.Y).trigger == eTrigger::ZONASEGURA) {
        ao::net::protocol::WriteConsoleMsg(atacante_index, "No puedes pelear aquí.", FontTypeNames::FONTTYPE_WARNING);
        return false;
    }

    return true;
}

bool PuedeAtacarNPC(std::int16_t user_index, std::int16_t npc_index, bool paraliza, bool is_pet) {
    (void)paraliza;
    (void)is_pet;

    if (!IsValidUser(user_index) || !IsValidNpc(npc_index)) {
        return false;
    }

    const auto& user = UserList[user_index];
    const auto& npc = Npclist[npc_index];

    if (user.flags.Muerto == 1) {
        ao::net::protocol::WriteConsoleMsg(user_index, "¡Estás muerto!!", FontTypeNames::FONTTYPE_INFO);
        return false;
    }

    if ((user.flags.Privilegios & PlayerType::Consejero) != 0) {
        return false;
    }

    if (user.flags.EnConsulta) {
        ao::net::protocol::WriteConsoleMsg(user_index, "No puedes atacar npcs mientras estas en consulta.", FontTypeNames::FONTTYPE_INFO);
        return false;
    }

    if (npc.Attackable == 0) {
        ao::net::protocol::WriteConsoleMsg(user_index, "No puedes atacar esta criatura.", FontTypeNames::FONTTYPE_INFO);
        return false;
    }

    if (Distancia(user.Pos, npc.Pos) >= MAXDISTANCIAARCO) {
        ao::net::protocol::WriteConsoleMsg(user_index, "Estás muy lejos para disparar.", FontTypeNames::FONTTYPE_FIGHT);
        return false;
    }

    if (npc.Hostile == 0) {
        if (npc.NPCtype == eNPCType::Guardiascaos) {
            if (user.Faccion.FuerzasCaos == 1) {
                ao::net::protocol::WriteConsoleMsg(user_index, "No puedes atacar Guardias del Caos siendo de la legión oscura.", FontTypeNames::FONTTYPE_INFO);
                return false;
            }
        } else if (npc.NPCtype == eNPCType::GuardiaReal) {
            if (user.Faccion.ArmadaReal == 1) {
                ao::net::protocol::WriteConsoleMsg(user_index, "No puedes atacar Guardias Reales siendo del ejército real.", FontTypeNames::FONTTYPE_INFO);
                return false;
            }
            if (user.flags.Seguro) {
                ao::net::protocol::WriteConsoleMsg(user_index, "Para poder atacar Guardias Reales debes quitarte el seguro.", FontTypeNames::FONTTYPE_INFO);
                return false;
            } else {
                if (s_volver_criminal_hook) {
                    s_volver_criminal_hook(user_index);
                }
                return true;
            }
        }
    }

    return true;
}

void MuereNpc(std::int16_t npc_index, std::int16_t user_index) {
    if (!IsValidNpc(npc_index)) return;
    auto& npc = Npclist[npc_index];
    if (npc.MaestroUser > 0) {
        RestarCriaturasEntrenador(npc.MaestroUser, npc_index);
    }
    if (s_muere_npc_hook) {
        s_muere_npc_hook(npc_index, user_index);
    }
}

void RestarCriaturasEntrenador(std::int16_t user_index, std::int16_t npc_index) {
    if (!IsValidUser(user_index)) return;
    auto& user = UserList[user_index];
    for (int i = 1; i <= MAXMASCOTAS; ++i) {
        if (user.MascotasIndex[i] == npc_index) {
            user.MascotasIndex[i] = 0;
            user.MascotasType[i] = 0;
            if (user.NroMascotas > 0) {
                user.NroMascotas--;
            }
            break;
        }
    }
}

void CheckPets(std::int16_t npc_index, std::int16_t user_index, bool check_elementales) {
    (void)check_elementales;
    if (!IsValidUser(user_index) || !IsValidNpc(npc_index)) return;
    auto& user = UserList[user_index];
    if (user.NroMascotas == 0) return;
    if (!PuedeAtacarNPC(user_index, npc_index)) return;

    for (int j = 1; j <= MAXMASCOTAS; ++j) {
        const auto pet_idx = user.MascotasIndex[j];
        if (pet_idx > 0 && pet_idx != npc_index && IsValidNpc(pet_idx)) {
            if (Npclist[pet_idx].TargetNPC == 0) {
                Npclist[pet_idx].TargetNPC = npc_index;
                Npclist[pet_idx].Movement = TipoAI::NpcAtacaNpc;
            }
        }
    }
}

void AllFollowAmo(std::int16_t user_index) {
    if (!IsValidUser(user_index)) return;
    auto& user = UserList[user_index];
    for (int j = 1; j <= MAXMASCOTAS; ++j) {
        const auto pet_idx = user.MascotasIndex[j];
        if (pet_idx > 0 && IsValidNpc(pet_idx)) {
            Npclist[pet_idx].Target = 0;
            Npclist[pet_idx].TargetNPC = 0;
            Npclist[pet_idx].Movement = TipoAI::SigueAmo;
        }
    }
}

void AllMascotasAtacanUser(std::int16_t victim, std::int16_t maestro) {
    if (!IsValidUser(maestro) || !IsValidUser(victim)) return;
    auto& user = UserList[maestro];
    for (int i = 1; i <= MAXMASCOTAS; ++i) {
        const auto pet_idx = user.MascotasIndex[i];
        if (pet_idx > 0 && IsValidNpc(pet_idx)) {
            Npclist[pet_idx].flags.AttackedBy = UserList[victim].name;
            Npclist[pet_idx].Movement = TipoAI::NPCDEFENSA;
            Npclist[pet_idx].Hostile = 1;
            Npclist[pet_idx].Target = victim;
        }
    }
}

} // namespace ao
