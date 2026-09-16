#include "MODULO_NPCs.hpp"
#include "Declares.hpp"
#include "Modulo_InventANDobj.hpp"
#include "clsIniReader.hpp"
#include "FileIO.hpp"
#include "Protocol.hpp"
#include "Matematicas.hpp"
#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace MODULO_NPCs {

namespace {
NpcCallbacks s_callbacks{};
std::int16_t pretorianosVivos = 0;

std::int16_t NextOpenCharIndex() noexcept {
    for (std::int16_t loop_c = 1; loop_c <= MAXCHARS; ++loop_c) {
        if (CharList[loop_c] == 0) {
            return loop_c;
        }
    }
    return static_cast<std::int16_t>(MAXCHARS + 1);
}

constexpr int MAXSPAWNATTEMPS = 100;

bool InMapBounds(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return (map > 0 && x >= 1 && x <= 100 && y >= 1 && y <= 100);
}

std::size_t MapBlockIndex(std::int16_t map, std::int16_t x, std::int16_t y) noexcept {
    return (static_cast<std::size_t>(map) * 101 + static_cast<std::size_t>(x)) * 101 + static_cast<std::size_t>(y);
}

void HeadtoPos(eHeading heading, WorldPos& pos) noexcept {
    switch (heading) {
        case eHeading::NORTH:
            pos.Y--;
            break;
        case eHeading::SOUTH:
            pos.Y++;
            break;
        case eHeading::EAST:
            pos.X++;
            break;
        case eHeading::WEST:
            pos.X--;
            break;
    }
}

eHeading InvertHeading(eHeading heading) noexcept {
    switch (heading) {
        case eHeading::NORTH:
            return eHeading::SOUTH;
        case eHeading::SOUTH:
            return eHeading::NORTH;
        case eHeading::EAST:
            return eHeading::WEST;
        case eHeading::WEST:
            return eHeading::EAST;
    }
    return eHeading::NORTH;
}

int esPretoriano(std::int16_t npc_index) noexcept {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return 0;
    }
    switch (Npclist[npc_index].Numero) {
        case 900: return 1; // PRCLER_NPC
        case 902: return 2; // PRMAGO_NPC
        case 903: return 3; // PRCAZA_NPC
        case 904: return 4; // PRKING_NPC
        case 901: return 5; // PRGUER_NPC
        default: return 0;
    }
}

void NPCTirarOro(npc& mi_npc) {
    // Código inerte / inactivo en 0.13.0 (Bug #27)
}
}

void SetCallbacks(const NpcCallbacks& cb) noexcept {
    s_callbacks = cb;
}

void ResetCallbacks() noexcept {
    s_callbacks = NpcCallbacks{};
}

void ResetNpcFlags(std::int16_t npc_index) noexcept {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& flags = Npclist[npc_index].flags;
    flags.AfectaParalisis = 0;
    flags.AguaValida = 0;
    flags.AttackedBy.clear();
    flags.AttackedFirstBy.clear();
    flags.BackUp = 0;
    flags.Bendicion = 0;
    flags.Domable = 0;
    flags.Envenenado = 0;
    flags.Faccion = 0;
    flags.Follow = false;
    flags.AtacaDoble = 0;
    flags.LanzaSpells = 0;
    flags.invisible = 0;
    flags.Maldicion = 0;
    flags.OldHostil = 0;
    flags.OldMovement = TipoAI::StaticNPC;
    flags.Paralizado = 0;
    flags.Inmovilizado = 0;
    flags.Respawn = 0;
    flags.RespawnOrigPos = 0;
    flags.Snd1 = 0;
    flags.Snd2 = 0;
    flags.Snd3 = 0;
    flags.TierraInvalida = 0;
}

void ResetNpcCounters(std::int16_t npc_index) noexcept {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& contadores = Npclist[npc_index].Contadores;
    contadores.Paralisis = 0;
    contadores.TiempoExistencia = 0;
}

void ResetNpcCharInfo(std::int16_t npc_index) noexcept {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& ch = Npclist[npc_index].char_appearance;
    ch.body = 0;
    ch.CascoAnim = 0;
    ch.CharIndex = 0;
    ch.FX = 0;
    ch.Head = 0;
    ch.heading = eHeading::NORTH;
    ch.loops = 0;
    ch.ShieldAnim = 0;
    ch.WeaponAnim = 0;
}

void ResetNpcCriatures(std::int16_t npc_index) noexcept {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& npc_ref = Npclist[npc_index];
    npc_ref.Criaturas.clear();
    npc_ref.NroCriaturas = 0;
}

void ResetExpresiones(std::int16_t npc_index) noexcept {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& npc_ref = Npclist[npc_index];
    npc_ref.Expresiones.clear();
    npc_ref.NroExpresiones = 0;
}

void ResetNpcMainInfo(std::int16_t npc_index) noexcept {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& npc_ref = Npclist[npc_index];
    npc_ref.Attackable = 0;
    npc_ref.CanAttack = 0;
    npc_ref.Comercia = 0;
    npc_ref.GiveEXP = 0;
    npc_ref.GiveGLD = 0;
    npc_ref.Hostile = 0;
    npc_ref.InvReSpawn = 0;

    if (npc_ref.MaestroUser > 0 && s_callbacks.PerdioNpc) {
        s_callbacks.PerdioNpc(npc_ref.MaestroUser);
    }
    npc_ref.MaestroUser = 0;
    npc_ref.MaestroNpc = 0;
    npc_ref.Mascotas = 0;
    npc_ref.Movement = TipoAI::StaticNPC;
    npc_ref.name.clear();
    npc_ref.NPCtype = eNPCType::Comun;
    npc_ref.Numero = 0;
    npc_ref.Orig = WorldPos{};
    npc_ref.PoderAtaque = 0;
    npc_ref.PoderEvasion = 0;
    npc_ref.Pos = WorldPos{};
    npc_ref.SkillDomar = 0;
    npc_ref.Target = 0;
    npc_ref.TargetNPC = 0;
    npc_ref.TipoItems = 0;
    npc_ref.Veneno = 0;
    npc_ref.desc.clear();
    npc_ref.Ciudad = 0;

    npc_ref.Spells.clear();
    npc_ref.NroSpells = 0;

    ResetNpcCharInfo(npc_index);
    ResetNpcCriatures(npc_index);
    ResetExpresiones(npc_index);
}

std::int16_t NextOpenNPC() noexcept {
    for (std::int16_t loop_c = 1; loop_c <= MAXNPCS; ++loop_c) {
        if (!Npclist[loop_c].flags.NPCActive) {
            return loop_c;
        }
    }
    return static_cast<std::int16_t>(MAXNPCS + 1);
}

std::int16_t OpenNPC(std::int16_t npc_number, bool respawn) {
    clsIniReader reader;
    std::string file_path = DatPath.empty() ? "Dat/NPCs.dat" : (DatPath + "NPCs.dat");
    reader.Initialize(file_path);

    std::string key = "NPC" + std::to_string(npc_number);
    if (!reader.KeyExists(key)) {
        return static_cast<std::int16_t>(MAXNPCS + 1);
    }

    std::int16_t npc_index = NextOpenNPC();
    if (npc_index > MAXNPCS) {
        return npc_index;
    }

    auto& npc_ref = Npclist[npc_index];
    npc_ref.Numero = npc_number;
    npc_ref.name = reader.GetValue(key, "Name");
    npc_ref.desc = reader.GetValue(key, "Desc");

    std::string val_str = reader.GetValue(key, "Movement");
    npc_ref.Movement = val_str.empty() ? TipoAI::StaticNPC : static_cast<TipoAI>(std::stoi(val_str));
    npc_ref.flags.OldMovement = npc_ref.Movement;

    val_str = reader.GetValue(key, "AguaValida");
    npc_ref.flags.AguaValida = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "TierraInValida");
    npc_ref.flags.TierraInvalida = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Faccion");
    npc_ref.flags.Faccion = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "AtacaDoble");
    npc_ref.flags.AtacaDoble = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "NpcType");
    npc_ref.NPCtype = val_str.empty() ? eNPCType::Comun : static_cast<eNPCType>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Body");
    npc_ref.char_appearance.body = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Head");
    npc_ref.char_appearance.Head = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Heading");
    npc_ref.char_appearance.heading = val_str.empty() ? eHeading::NORTH : static_cast<eHeading>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Attackable");
    npc_ref.Attackable = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Comercia");
    npc_ref.Comercia = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Hostile");
    npc_ref.Hostile = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));
    npc_ref.flags.OldHostil = npc_ref.Hostile;

    val_str = reader.GetValue(key, "GiveEXP");
    npc_ref.GiveEXP = val_str.empty() ? 0 : std::stoi(val_str);
    npc_ref.flags.ExpCount = npc_ref.GiveEXP;

    val_str = reader.GetValue(key, "Veneno");
    npc_ref.Veneno = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Domable");
    npc_ref.flags.Domable = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "GiveGLD");
    npc_ref.GiveGLD = val_str.empty() ? 0 : std::stoi(val_str);

    val_str = reader.GetValue(key, "PoderAtaque");
    npc_ref.PoderAtaque = val_str.empty() ? 0 : std::stoi(val_str);

    val_str = reader.GetValue(key, "PoderEvasion");
    npc_ref.PoderEvasion = val_str.empty() ? 0 : std::stoi(val_str);

    val_str = reader.GetValue(key, "InvReSpawn");
    npc_ref.InvReSpawn = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "MaxHP");
    npc_ref.Stats.MaxHp = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "MinHP");
    npc_ref.Stats.MinHp = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "MaxHIT");
    npc_ref.Stats.MaxHIT = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "MinHIT");
    npc_ref.Stats.MinHIT = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "DEF");
    npc_ref.Stats.def = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "DEFm");
    npc_ref.Stats.defM = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Alineacion");
    npc_ref.Stats.Alineacion = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "NROITEMS");
    npc_ref.Invent.NroItems = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    for (std::int16_t i = 1; i <= npc_ref.Invent.NroItems && i <= MAX_INVENTORY_SLOTS; ++i) {
        std::string ln = reader.GetValue(key, "Obj" + std::to_string(i));
        std::string obj_idx_str = FileIO::ReadField(1, ln, 45);
        std::string amt_str = FileIO::ReadField(2, ln, 45);
        npc_ref.Invent.Object[i].ObjIndex = obj_idx_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(obj_idx_str));
        npc_ref.Invent.Object[i].Amount = amt_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(amt_str));
    }

    for (std::int16_t i = 1; i <= MAX_NPC_DROPS; ++i) {
        std::string ln = reader.GetValue(key, "Drop" + std::to_string(i));
        std::string obj_idx_str = FileIO::ReadField(1, ln, 45);
        std::string amt_str = FileIO::ReadField(2, ln, 45);
        npc_ref.Drop[i].ObjIndex = obj_idx_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(obj_idx_str));
        npc_ref.Drop[i].Amount = amt_str.empty() ? 0 : std::stoi(amt_str);
    }

    val_str = reader.GetValue(key, "LanzaSpells");
    npc_ref.flags.LanzaSpells = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));
    if (npc_ref.flags.LanzaSpells > 0) {
        npc_ref.Spells.resize(npc_ref.flags.LanzaSpells + 1, 0);
        for (std::size_t i = 1; i <= npc_ref.flags.LanzaSpells; ++i) {
            std::string sp_val = reader.GetValue(key, "Sp" + std::to_string(i));
            npc_ref.Spells[i] = sp_val.empty() ? 0 : static_cast<std::int16_t>(std::stoi(sp_val));
        }
    }

    if (npc_ref.NPCtype == eNPCType::Entrenador) {
        val_str = reader.GetValue(key, "NroCriaturas");
        npc_ref.NroCriaturas = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));
        if (npc_ref.NroCriaturas > 0) {
            npc_ref.Criaturas.resize(npc_ref.NroCriaturas + 1);
            for (std::size_t i = 1; i <= static_cast<std::size_t>(npc_ref.NroCriaturas); ++i) {
                std::string ci_val = reader.GetValue(key, "CI" + std::to_string(i));
                std::string cn_val = reader.GetValue(key, "CN" + std::to_string(i));
                npc_ref.Criaturas[i].NpcIndex = ci_val.empty() ? 0 : static_cast<std::int16_t>(std::stoi(ci_val));
                npc_ref.Criaturas[i].NpcName = cn_val;
            }
        }
    }

    npc_ref.flags.NPCActive = true;
    if (respawn) {
        val_str = reader.GetValue(key, "ReSpawn");
        npc_ref.flags.Respawn = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));
    } else {
        npc_ref.flags.Respawn = 1;
    }

    val_str = reader.GetValue(key, "BackUp");
    npc_ref.flags.BackUp = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "OrigPos");
    npc_ref.flags.RespawnOrigPos = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "AfectaParalisis");
    npc_ref.flags.AfectaParalisis = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Snd1");
    npc_ref.flags.Snd1 = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Snd2");
    npc_ref.flags.Snd2 = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Snd3");
    npc_ref.flags.Snd3 = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "NROEXP");
    npc_ref.NroExpresiones = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));
    if (npc_ref.NroExpresiones > 0) {
        npc_ref.Expresiones.resize(npc_ref.NroExpresiones + 1);
        for (std::size_t i = 1; i <= npc_ref.NroExpresiones; ++i) {
            npc_ref.Expresiones[i] = reader.GetValue(key, "Exp" + std::to_string(i));
        }
    }

    val_str = reader.GetValue(key, "TipoItems");
    npc_ref.TipoItems = val_str.empty() ? 0 : static_cast<std::int16_t>(std::stoi(val_str));

    val_str = reader.GetValue(key, "Ciudad");
    npc_ref.Ciudad = val_str.empty() ? 0 : static_cast<std::uint8_t>(std::stoi(val_str));

    if (npc_index > LastNPC) {
        LastNPC = npc_index;
    }
    NumNPCs++;

    return npc_index;
}

void EraseNPCChar(std::int16_t npc_index) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& ch = Npclist[npc_index].char_appearance;
    if (ch.CharIndex != 0) {
        CharList[ch.CharIndex] = 0;
    }

    if (ch.CharIndex == LastChar) {
        do {
            LastChar--;
            if (LastChar <= 1) {
                break;
            }
        } while (CharList[LastChar] > 0);
    }

    const auto& pos = Npclist[npc_index].Pos;
    if (InMapBounds(pos.Map, pos.X, pos.Y)) {
        std::size_t idx = MapBlockIndex(pos.Map, pos.X, pos.Y);
        if (idx < MapData.size()) {
            MapData[idx].NpcIndex = 0;
        }
    }

    if (s_callbacks.SendData && ch.CharIndex != 0) {
        auto data = ao::net::protocol::PrepareMessageCharacterRemove(ch.CharIndex);
        s_callbacks.SendData(ao::net::send_data::SendTarget::ToNPCArea, npc_index, data);
    }

    ch.CharIndex = 0;

    if (NumChars > 0) {
        NumChars--;
    }
}

void QuitarNPC(std::int16_t npc_index) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& npc_ref = Npclist[npc_index];
    npc_ref.flags.NPCActive = false;
    npc_ref.Owner = 0;

    if (InMapBounds(npc_ref.Pos.Map, npc_ref.Pos.X, npc_ref.Pos.Y)) {
        EraseNPCChar(npc_index);
    }

    Modulo_InventANDobj::ResetNpcInv(npc_index);
    ResetNpcFlags(npc_index);
    ResetNpcCounters(npc_index);
    ResetNpcMainInfo(npc_index);

    if (npc_index == LastNPC) {
        do {
            LastNPC--;
            if (LastNPC < 1) {
                break;
            }
        } while (!Npclist[LastNPC].flags.NPCActive);
    }

    if (NumNPCs != 0) {
        NumNPCs--;
    }
}

bool TestSpawnTrigger(const WorldPos& pos, bool puede_agua) noexcept {
    bool is_legal = false;
    if (s_callbacks.LegalPos) {
        is_legal = s_callbacks.LegalPos(pos.Map, pos.X, pos.Y, puede_agua);
    } else {
        is_legal = InMapBounds(pos.Map, pos.X, pos.Y);
    }

    if (is_legal) {
        std::size_t idx = MapBlockIndex(pos.Map, pos.X, pos.Y);
        if (idx < MapData.size()) {
            auto trg = static_cast<std::int32_t>(MapData[idx].trigger);
            return trg != 3 && trg != 2 && trg != 1;
        }
    }
    return false;
}

void MakeNPCChar(bool to_map, std::int16_t snd_index, std::int16_t npc_index, std::int16_t map, std::int16_t x, std::int16_t y) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    std::int16_t char_index = 0;
    auto& ch = Npclist[npc_index].char_appearance;
    if (ch.CharIndex == 0) {
        char_index = NextOpenCharIndex();
        ch.CharIndex = char_index;
        if (char_index <= MAXCHARS) {
            CharList[char_index] = npc_index;
            if (char_index > LastChar) {
                LastChar = char_index;
            }
            NumChars++;
        }
    }

    if (InMapBounds(map, x, y)) {
        std::size_t idx = MapBlockIndex(map, x, y);
        if (idx < MapData.size()) {
            MapData[idx].NpcIndex = npc_index;
        }
    }

    if (!to_map) {
        if (s_callbacks.WriteCharacterCreate) {
            s_callbacks.WriteCharacterCreate(snd_index, ch.body, ch.Head, ch.heading, ch.CharIndex, x, y, 0, 0, 0, 0, 0, "", 0, 0);
        }
        if (s_callbacks.FlushBuffer) {
            s_callbacks.FlushBuffer(snd_index);
        }
    } else {
        if (s_callbacks.AgregarNpc) {
            s_callbacks.AgregarNpc(npc_index);
        }
    }
}

void ChangeNPCChar(std::int16_t npc_index, std::int16_t body, std::int16_t head, eHeading heading) {
    if (npc_index > 0 && static_cast<std::size_t>(npc_index) < Npclist.size()) {
        auto& ch = Npclist[npc_index].char_appearance;
        ch.body = body;
        ch.Head = head;
        ch.heading = heading;

        if (s_callbacks.SendData) {
            auto data = ao::net::protocol::PrepareMessageCharacterChange(body, head, heading, ch.CharIndex, 0, 0, 0, 0, 0);
            s_callbacks.SendData(ao::net::send_data::SendTarget::ToNPCArea, npc_index, data);
        }
    }
}

std::int16_t SpawnNpc(std::int16_t npc_index, const WorldPos& pos, bool fx, bool respawn) {
    std::int16_t nIndex = OpenNPC(npc_index, respawn);
    if (nIndex > MAXNPCS) {
        return 0;
    }

    bool puede_agua = Npclist[nIndex].flags.AguaValida != 0;
    bool puede_tierra = Npclist[nIndex].flags.TierraInvalida != 1;
    WorldPos newpos{}, altpos{};
    bool posicion_valida = false;

    if (s_callbacks.ClosestLegalPos) {
        s_callbacks.ClosestLegalPos(pos, newpos, puede_agua, puede_tierra);
        s_callbacks.ClosestLegalPos(pos, altpos, puede_agua, true);
    }

    if (newpos.X != 0 && newpos.Y != 0) {
        Npclist[nIndex].Pos.Map = newpos.Map;
        Npclist[nIndex].Pos.X = newpos.X;
        Npclist[nIndex].Pos.Y = newpos.Y;
        posicion_valida = true;
    } else {
        if (altpos.X != 0 && altpos.Y != 0) {
            Npclist[nIndex].Pos.Map = altpos.Map;
            Npclist[nIndex].Pos.X = altpos.X;
            Npclist[nIndex].Pos.Y = altpos.Y;
            posicion_valida = true;
        } else {
            posicion_valida = false;
        }
    }

    if (!posicion_valida) {
        QuitarNPC(nIndex);
        return 0;
    }

    std::int16_t map = Npclist[nIndex].Pos.Map;
    std::int16_t x = Npclist[nIndex].Pos.X;
    std::int16_t y = Npclist[nIndex].Pos.Y;

    MakeNPCChar(true, map, nIndex, map, x, y);

    if (fx && s_callbacks.SendData) {
        auto wave = ao::net::protocol::PrepareMessagePlayWave(SND_WARP, x, y);
        s_callbacks.SendData(ao::net::send_data::SendTarget::ToNPCArea, nIndex, wave);
        auto fx_msg = ao::net::protocol::PrepareMessageCreateFX(Npclist[nIndex].char_appearance.CharIndex, FXIDs::FXWARP, 0);
        s_callbacks.SendData(ao::net::send_data::SendTarget::ToNPCArea, nIndex, fx_msg);
    }

    return nIndex;
}

std::int16_t CrearNPC(std::int16_t nro_npc, std::int16_t mapa, const WorldPos& orig_pos, const WorldPos& alt_pos_arg) {
    std::int16_t nIndex = OpenNPC(nro_npc);
    if (nIndex > MAXNPCS) {
        return 0;
    }

    Npclist[nIndex].Orig = orig_pos;

    WorldPos pos = orig_pos;
    WorldPos newpos{}, altpos = alt_pos_arg;
    bool puede_agua = Npclist[nIndex].flags.AguaValida != 0;
    bool puede_tierra = Npclist[nIndex].flags.TierraInvalida != 1;
    bool posicion_valida = false;
    int iteraciones = 0;
    // Cascada de instanciación espacial en CrearNPC:
    // 1. Si pos (orig_pos) es (0,0), intenta buscar posición válida cerca de (50,50) vía ClosestLegalPos.
    // 2. Si pos es diferente de (0,0), realiza hasta MAXSPAWNATTEMPS (100) iteraciones al azar en pos +/- 3 tiles.
    // 3. Si se agotan los 100 intentos, realiza fallback a altpos si es válida.
    // 4. Si altpos no es válida, realiza fallback a (50,50) vía ClosestLegalPos. Si falla, invoca QuitarNPC y aborta.
    if (pos.X == 0 || pos.Y == 0) {
        pos.Map = mapa;
        pos.X = 50;
        pos.Y = 50;
        if (s_callbacks.ClosestLegalPos) {
            s_callbacks.ClosestLegalPos(pos, newpos, puede_agua, puede_tierra);
        }
        if (newpos.X != 0 && newpos.Y != 0) {
            Npclist[nIndex].Pos = newpos;
            MakeNPCChar(true, mapa, nIndex, mapa, newpos.X, newpos.Y);
            return nIndex;
        } else {
            QuitarNPC(nIndex);
            return 0;
        }
    } else {
        do {
            newpos.Map = mapa;
            newpos.X = static_cast<std::int16_t>(RandomNumber(pos.X - 3, pos.X + 3));
            newpos.Y = static_cast<std::int16_t>(RandomNumber(pos.Y - 3, pos.Y + 3));

            if (newpos.X != 0 && newpos.Y != 0) {
                altpos.X = newpos.X;
                altpos.Y = newpos.Y;
                altpos.Map = newpos.Map;
            }

            bool legal_npc = false;
            if (s_callbacks.LegalPosNPC) {
                legal_npc = s_callbacks.LegalPosNPC(newpos.Map, newpos.X, newpos.Y, puede_agua, false);
            } else {
                legal_npc = InMapBounds(newpos.Map, newpos.X, newpos.Y);
            }

            bool hay_pc = false;
            if (s_callbacks.HayPCarea) {
                hay_pc = s_callbacks.HayPCarea(newpos);
            }

            if (legal_npc && !hay_pc && TestSpawnTrigger(newpos, puede_agua)) {
                Npclist[nIndex].Pos = newpos;
                posicion_valida = true;
                break;
            } else {
                newpos.X = 0;
                newpos.Y = 0;
            }

            iteraciones++;
            if (iteraciones > MAXSPAWNATTEMPS) {
                if (altpos.X != 0 && altpos.Y != 0) {
                    Npclist[nIndex].Pos.Map = mapa;
                    Npclist[nIndex].Pos.X = altpos.X;
                    Npclist[nIndex].Pos.Y = altpos.Y;
                    MakeNPCChar(true, mapa, nIndex, mapa, altpos.X, altpos.Y);
                    return nIndex;
                } else {
                    altpos.X = 50;
                    altpos.Y = 50;
                    altpos.Map = mapa;
                    if (s_callbacks.ClosestLegalPos) {
                        s_callbacks.ClosestLegalPos(altpos, newpos, puede_agua, puede_tierra);
                    }
                    if (newpos.X != 0 && newpos.Y != 0) {
                        Npclist[nIndex].Pos = newpos;
                        MakeNPCChar(true, newpos.Map, nIndex, newpos.Map, newpos.X, newpos.Y);
                        return nIndex;
                    } else {
                        QuitarNPC(nIndex);
                        return 0;
                    }
                }
            }
        } while (!posicion_valida);

        MakeNPCChar(true, mapa, nIndex, mapa, Npclist[nIndex].Pos.X, Npclist[nIndex].Pos.Y);
        return nIndex;
    }
}

void ReSpawnNpc(npc& mi_npc) {
    if (mi_npc.flags.Respawn == 0) {
        CrearNPC(mi_npc.Numero, mi_npc.Pos.Map, mi_npc.Orig);
    }
}

void MoveNPCChar(std::int16_t npc_index, eHeading n_heading) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& npc_ref = Npclist[npc_index];
    WorldPos nPos = npc_ref.Pos;
    HeadtoPos(n_heading, nPos);

    bool legal = false;
    if (s_callbacks.LegalPosNPC) {
        legal = s_callbacks.LegalPosNPC(npc_ref.Pos.Map, nPos.X, nPos.Y, npc_ref.flags.AguaValida == 1, npc_ref.MaestroUser != 0);
    } else {
        legal = InMapBounds(nPos.Map, nPos.X, nPos.Y);
    }

    if (legal) {
        bool dest_agua = s_callbacks.HayAgua ? s_callbacks.HayAgua(npc_ref.Pos.Map, nPos.X, nPos.Y) : false;
        if (npc_ref.flags.AguaValida == 0 && dest_agua) return;
        if (npc_ref.flags.TierraInvalida == 1 && !dest_agua) return;

        std::size_t npos_idx = MapBlockIndex(npc_ref.Pos.Map, nPos.X, nPos.Y);
        std::int16_t user_index = (npos_idx < MapData.size()) ? MapData[npos_idx].UserIndex : static_cast<std::int16_t>(0);

        // Permuta de posición y desalojo forzado de usuarios muertos (caspers) en MoveNPCChar:
        // Si la casilla destino contiene a un casper (UserIndex > 0), el servidor mueve al casper a la posición previa
        // que ocupaba el NPC (X, Y previa), limpia la casilla previa del casper y actualiza la grilla de MapData.
        // Notifica el desplazamiento forzado mediante WriteForceCharMove y verifica la frontera de superficie (HayAgua).
        if (user_index > 0 && static_cast<std::size_t>(user_index) < UserList.size()) {
            bool orig_agua = s_callbacks.HayAgua ? s_callbacks.HayAgua(npc_ref.Pos.Map, npc_ref.Pos.X, npc_ref.Pos.Y) : false;
            if (dest_agua && !orig_agua) return;
            if (!dest_agua && orig_agua) return;

            auto& user = UserList[user_index];
            std::size_t user_old_idx = MapBlockIndex(user.Pos.Map, user.Pos.X, user.Pos.Y);
            if (user_old_idx < MapData.size()) {
                MapData[user_old_idx].UserIndex = 0;
            }
            user.Pos.X = npc_ref.Pos.X;
            user.Pos.Y = npc_ref.Pos.Y;
            std::size_t user_new_idx = MapBlockIndex(user.Pos.Map, user.Pos.X, user.Pos.Y);
            if (user_new_idx < MapData.size()) {
                MapData[user_new_idx].UserIndex = user_index;
            }

            if (s_callbacks.SendData) {
                auto move_user = ao::net::protocol::PrepareMessageCharacterMove(user.char_appearance.CharIndex, user.Pos.X, user.Pos.Y);
                s_callbacks.SendData(ao::net::send_data::SendTarget::ToPCAreaButIndex, user_index, move_user);
            }

            if (s_callbacks.WriteForceCharMove) {
                s_callbacks.WriteForceCharMove(user_index, InvertHeading(n_heading));
            } else {
                ao::net::protocol::WriteForceCharMove(user_index, InvertHeading(n_heading));
            }

            if (s_callbacks.FlushBuffer) {
                s_callbacks.FlushBuffer(user_index);
            }
        }

        if (s_callbacks.SendData) {
            auto move_npc = ao::net::protocol::PrepareMessageCharacterMove(npc_ref.char_appearance.CharIndex, nPos.X, nPos.Y);
            s_callbacks.SendData(ao::net::send_data::SendTarget::ToNPCArea, npc_index, move_npc);
        }

        std::size_t npc_old_idx = MapBlockIndex(npc_ref.Pos.Map, npc_ref.Pos.X, npc_ref.Pos.Y);
        if (npc_old_idx < MapData.size()) {
            MapData[npc_old_idx].NpcIndex = 0;
        }

        npc_ref.Pos = nPos;
        npc_ref.char_appearance.heading = n_heading;

        std::size_t npc_new_idx = MapBlockIndex(nPos.Map, nPos.X, nPos.Y);
        if (npc_new_idx < MapData.size()) {
            MapData[npc_new_idx].NpcIndex = npc_index;
        }

        if (s_callbacks.CheckUpdateNeededNpc) {
            s_callbacks.CheckUpdateNeededNpc(npc_index, n_heading);
        }
    } else if (npc_ref.MaestroUser == 0) {
        if (npc_ref.Movement == TipoAI::NpcPathfinding) {
            npc_ref.PFINFO.PathLenght = 0;
            npc_ref.PFINFO.CurPos = 0;
            npc_ref.PFINFO.NoPath = true;
        }
    }
}

void NpcEnvenenarUser(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }
    int n = RandomNumber(1, 100);
    if (n < 30) {
        UserList[user_index].flags.Envenenado = 1;
        if (s_callbacks.WriteConsoleMsg) {
            s_callbacks.WriteConsoleMsg(user_index, "¡La criatura te ha envenenado!!", static_cast<std::int16_t>(ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT));
        }
    }
}

void QuitarMascota(std::int16_t user_index, std::int16_t npc_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }
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

void QuitarMascotaNpc(std::int16_t maestro_npc_index) {
    if (maestro_npc_index <= 0 || static_cast<std::size_t>(maestro_npc_index) >= Npclist.size()) {
        return;
    }
    if (Npclist[maestro_npc_index].Mascotas > 0) {
        Npclist[maestro_npc_index].Mascotas--;
    }
}

void QuitarPet(std::int16_t user_index, std::int16_t npc_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }
    auto& user = UserList[user_index];
    int pet_index = 0;
    for (int i = 1; i <= MAXMASCOTAS; ++i) {
        if (user.MascotasIndex[i] == npc_index) {
            pet_index = i;
            break;
        }
    }
    if (pet_index == 0) {
        return;
    }
    if (user.NroMascotas > 0) {
        user.NroMascotas--;
    }
    user.MascotasIndex[pet_index] = 0;
    user.MascotasType[pet_index] = 0;

    QuitarNPC(npc_index);
}

void ValidarPermanenciaNpc(std::int16_t npc_index) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }
    std::int16_t owner = Npclist[npc_index].Owner;
    if (owner > 0 && s_callbacks.IntervaloPerdioNpc && s_callbacks.IntervaloPerdioNpc(owner)) {
        if (s_callbacks.PerdioNpc) {
            s_callbacks.PerdioNpc(owner);
        }
    }
}

void MuereNpc(std::int16_t npc_index, std::int16_t user_index) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    npc mi_npc = Npclist[npc_index];
    bool era_criminal = false;
    bool is_pretoriano = false;

    if (esPretoriano(npc_index) == 4) { // Rey Pretoriano
        is_pretoriano = true;
        if (Npclist[npc_index].Pos.Map == MAPA_PRETORIANO) {
            // Barrido cuadrático literal legacy 8 To 90 para verificar la supervivencia de subordinados pretorianos
            for (int i = 8; i <= 90; ++i) {
                for (int j = 8; j <= 90; ++j) {
                    std::size_t b_idx = MapBlockIndex(MAPA_PRETORIANO, static_cast<std::int16_t>(i), static_cast<std::int16_t>(j));
                    std::int16_t npci = (b_idx < MapData.size()) ? MapData[b_idx].NpcIndex : static_cast<std::int16_t>(0);
                    if (npci > 0 && esPretoriano(npci) > 0 && npci != npc_index) {
                        if (Npclist[npc_index].Pos.X > 50) {
                            if (Npclist[npci].Pos.X > 50) Npclist[npci].Invent.ArmourEqpSlot = 1;
                        } else {
                            if (Npclist[npci].Pos.X <= 50) Npclist[npci].Invent.ArmourEqpSlot = 5;
                        }
                    }
                }
            }
            if (s_callbacks.CrearClanPretoriano) {
                s_callbacks.CrearClanPretoriano(Npclist[npc_index].Pos.X);
            }
        }
    } else if (esPretoriano(npc_index) > 0) {
        is_pretoriano = true;
        if (Npclist[npc_index].Pos.Map == MAPA_PRETORIANO) {
            Npclist[npc_index].Invent.ArmourEqpSlot = 0;
            if (pretorianosVivos > 0) {
                pretorianosVivos--;
            }
        }
    }

    QuitarNPC(npc_index);

    if (user_index > 0 && static_cast<std::size_t>(user_index) < UserList.size()) {
        auto& user = UserList[user_index];

        if (mi_npc.flags.Snd3 > 0 && s_callbacks.SendData) {
            auto sound_msg = ao::net::protocol::PrepareMessagePlayWave(static_cast<std::uint8_t>(mi_npc.flags.Snd3), mi_npc.Pos.X, mi_npc.Pos.Y);
            s_callbacks.SendData(ao::net::send_data::SendTarget::ToPCArea, user_index, sound_msg);
        }

        user.flags.TargetNPC = 0;
        user.flags.TargetNpcTipo = eNPCType::Comun;

        if (user.NroMascotas > 0) {
            for (int t = 1; t <= MAXMASCOTAS; ++t) {
                std::int16_t pet_idx = user.MascotasIndex[t];
                if (pet_idx > 0 && static_cast<std::size_t>(pet_idx) < Npclist.size()) {
                    if (Npclist[pet_idx].TargetNPC == npc_index) {
                        if (s_callbacks.FollowAmo) {
                            s_callbacks.FollowAmo(pet_idx);
                        }
                    }
                }
            }
        }

        if (mi_npc.flags.ExpCount > 0) {
            if (user.PartyIndex > 0) {
                if (s_callbacks.PartyObtenerExito) {
                    s_callbacks.PartyObtenerExito(user_index, mi_npc.flags.ExpCount, mi_npc.Pos.Map, mi_npc.Pos.X, mi_npc.Pos.Y);
                }
            } else {
                user.Stats.Exp = user.Stats.Exp + mi_npc.flags.ExpCount;
                if (user.Stats.Exp > MAXEXP) {
                    user.Stats.Exp = MAXEXP;
                }
                if (s_callbacks.WriteConsoleMsg) {
                    s_callbacks.WriteConsoleMsg(user_index, "Has ganado " + std::to_string(mi_npc.flags.ExpCount) + " puntos de experiencia.", static_cast<std::int16_t>(ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT));
                }
            }
            mi_npc.flags.ExpCount = 0;
        }

        if (s_callbacks.WriteConsoleMsg) {
            s_callbacks.WriteConsoleMsg(user_index, "¡Has matado a la criatura!", static_cast<std::int16_t>(ao::net::protocol::FontTypeNames::FONTTYPE_FIGHT));
        }

        if (user.Stats.NPCsMuertos < 32000) {
            user.Stats.NPCsMuertos++;
        }

        era_criminal = FileIO::criminal(user_index);

        if (mi_npc.Stats.Alineacion == 0) {
            if (mi_npc.Numero == Guardias) {
                user.Reputacion.NobleRep = 0;
                user.Reputacion.PlebeRep = 0;
                user.Reputacion.AsesinoRep += 500;
                if (user.Reputacion.AsesinoRep > MAXREP) user.Reputacion.AsesinoRep = MAXREP;
            }
            if (mi_npc.MaestroUser == 0) {
                user.Reputacion.AsesinoRep += vlASESINO;
                if (user.Reputacion.AsesinoRep > MAXREP) user.Reputacion.AsesinoRep = MAXREP;
            }
        } else if (mi_npc.Stats.Alineacion == 1) {
            user.Reputacion.PlebeRep += vlCAZADOR;
            if (user.Reputacion.PlebeRep > MAXREP) user.Reputacion.PlebeRep = MAXREP;
        } else if (mi_npc.Stats.Alineacion == 2) {
            user.Reputacion.NobleRep += vlASESINO / 2;
            if (user.Reputacion.NobleRep > MAXREP) user.Reputacion.NobleRep = MAXREP;
        } else if (mi_npc.Stats.Alineacion == 4) {
            user.Reputacion.PlebeRep += vlCAZADOR;
            if (user.Reputacion.PlebeRep > MAXREP) user.Reputacion.PlebeRep = MAXREP;
        }

        bool es_crim_ahora = FileIO::criminal(user_index);
        if (es_crim_ahora && user.Faccion.ArmadaReal == 1) {
            if (s_callbacks.ExpulsarFaccionReal) s_callbacks.ExpulsarFaccionReal(user_index);
        }
        if (!es_crim_ahora && user.Faccion.FuerzasCaos == 1) {
            if (s_callbacks.ExpulsarFaccionCaos) s_callbacks.ExpulsarFaccionCaos(user_index);
        }

        if (era_criminal != es_crim_ahora) {
            if (s_callbacks.RefreshCharStatus) s_callbacks.RefreshCharStatus(user_index);
        }

        if (s_callbacks.CheckUserLevel) {
            s_callbacks.CheckUserLevel(user_index);
        }
    }

    if (mi_npc.MaestroUser == 0) {
        // NPCTirarOro(mi_npc); <-- BUG #27 REPLICADO: Comentado e inerte!
        if (s_callbacks.NPC_TIRAR_ITEMS) {
            s_callbacks.NPC_TIRAR_ITEMS(mi_npc, is_pretoriano);
        }
        ReSpawnNpc(mi_npc);
    }
}

void DoFollow(std::int16_t npc_index, const std::string& user_name) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& npc_ref = Npclist[npc_index];
    if (npc_ref.flags.Follow) {
        npc_ref.flags.Follow = false;
        npc_ref.Target = 0;
        npc_ref.Movement = TipoAI::NPCMAmbula;
    } else {
        npc_ref.flags.Follow = true;
        if (s_callbacks.FindUser) {
            npc_ref.Target = s_callbacks.FindUser(user_name);
        }
        npc_ref.Movement = TipoAI::SigueAmo;
    }
}

void FollowAmo(std::int16_t npc_index) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& npc_ref = Npclist[npc_index];
    npc_ref.flags.Follow = true;
    npc_ref.Movement = TipoAI::SigueAmo;
}

} // namespace MODULO_NPCs
