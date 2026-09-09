#include "FileIO.hpp"
#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace FileIO {

// ============================================================================
// GRUPO 1: Utilidades Base de Archivos e INI (Low-Level INI & File Helpers)
// ============================================================================

std::string GetVar(const std::string& file, const std::string& main, const std::string& var, long emptySpaces) {
    (void)emptySpaces;
    clsIniReader reader;
    reader.Initialize(file);
    return reader.GetValue(main, var);
}

void WriteVar(const std::string& file, const std::string& main, const std::string& var, const std::string& value) {
    struct KeyValue {
        std::string key;
        std::string value;
    };
    struct Section {
        std::string name;
        std::vector<KeyValue> keys;
    };

    std::vector<Section> sections;

    auto equalsIgnoreCase = [](const std::string& a, const std::string& b) {
        if (a.size() != b.size()) return false;
        for (size_t i = 0; i < a.size(); ++i) {
            if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i])))
                return false;
        }
        return true;
    };

    // Cargar archivo existente si está presente para preservar secciones y claves previas
    std::ifstream inFile(file, std::ios::binary);
    if (inFile.is_open()) {
        std::string line;
        Section* currentSection = nullptr;
        while (std::getline(inFile, line)) {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            std::string trimmed = line.substr(start);
            if (trimmed.empty() || trimmed[0] == ';' || trimmed[0] == '\'') continue;

            if (trimmed.front() == '[' && trimmed.back() == ']') {
                std::string secName = trimmed.substr(1, trimmed.size() - 2);
                sections.push_back({secName, {}});
                currentSection = &sections.back();
            } else if (currentSection != nullptr) {
                size_t eqPos = trimmed.find('=');
                if (eqPos != std::string::npos) {
                    std::string k = trimmed.substr(0, eqPos);
                    std::string v = trimmed.substr(eqPos + 1);
                    currentSection->keys.push_back({k, v});
                }
            }
        }
        inFile.close();
    }

    Section* secTarget = nullptr;
    for (auto& sec : sections) {
        if (equalsIgnoreCase(sec.name, main)) {
            secTarget = &sec;
            break;
        }
    }
    if (!secTarget) {
        sections.push_back({main, {}});
        secTarget = &sections.back();
    }

    bool keyFound = false;
    for (auto& kv : secTarget->keys) {
        if (equalsIgnoreCase(kv.key, var)) {
            kv.value = value;
            keyFound = true;
            break;
        }
    }
    if (!keyFound) {
        secTarget->keys.push_back({var, value});
    }

    // Reescribir preservando codificación ANSI (Windows-1252) y terminación de línea CRLF (\r\n)
    std::ofstream outFile(file, std::ios::binary | std::ios::trunc);
    if (outFile.is_open()) {
        for (const auto& sec : sections) {
            outFile << "[" << sec.name << "]\r\n";
            for (const auto& kv : sec.keys) {
                outFile << kv.key << "=" << kv.value << "\r\n";
            }
            outFile << "\r\n";
        }
    }
}

long TxtDimension(const std::string& name) {
    std::error_code ec;
    auto size = std::filesystem::file_size(name, ec);
    if (ec) {
        return 0;
    }
    return static_cast<long>(size);
}

std::string ReadField(int pos, const std::string& text, int sepAscii) {
    if (pos <= 0 || text.empty()) return "";
    char delim = static_cast<char>(sepAscii);
    int currentField = 1;
    size_t start = 0;
    size_t end = text.find(delim);

    while (end != std::string::npos) {
        if (currentField == pos) {
            return text.substr(start, end - start);
        }
        start = end + 1;
        end = text.find(delim, start);
        currentField++;
    }

    if (currentField == pos) {
        return text.substr(start);
    }

    return "";
}


// ============================================================================
// GRUPO 2: Persistencia de Personajes (.chr) — 🚨 CATEGORÍA CRÍTICA 1
// ============================================================================

static void CheckUserIndexBounds(int userIndex) {
    if (userIndex < 1 || static_cast<size_t>(userIndex) >= UserList.size()) {
        throw std::out_of_range("FileIO: UserIndex fuera de rango (UserIndex = " + std::to_string(userIndex) +
                                ", UserList.size() = " + std::to_string(UserList.size()) + ")");
    }
}

bool criminal(int userIndex) {
    CheckUserIndexBounds(userIndex);
    const auto& rep = UserList[userIndex].Reputacion;
    long l = (-rep.AsesinoRep) +
             (-rep.BandidoRep) +
             rep.BurguesRep +
             (-rep.LadronesRep) +
             rep.NobleRep +
             rep.PlebeRep;
    l = l / 6;
    return (l < 0);
}

void LoadUserStats(int userIndex, clsIniReader& userFile) {
    CheckUserIndexBounds(userIndex);
    auto& user = UserList[userIndex];
    auto& stats = user.Stats;

    for (int i = 1; i <= NUMATRIBUTOS; ++i) {
        std::string valStr = userFile.GetValue("ATRIBUTOS", "AT" + std::to_string(i));
        stats.UserAtributos[i] = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
        stats.UserAtributosBackUP[i] = stats.UserAtributos[i];
    }

    for (int i = 1; i <= NUMSKILLS; ++i) {
        std::string sk = userFile.GetValue("SKILLS", "SK" + std::to_string(i));
        std::string elu = userFile.GetValue("SKILLS", "ELUSK" + std::to_string(i));
        std::string exp = userFile.GetValue("SKILLS", "EXPSK" + std::to_string(i));
        stats.UserSkills[i] = sk.empty() ? 0 : static_cast<uint8_t>(std::stoi(sk));
        stats.EluSkills[i] = elu.empty() ? 0 : std::stol(elu);
        stats.ExpSkills[i] = exp.empty() ? 0 : std::stol(exp);
    }

    for (int i = 1; i <= MAXUSERHECHIZOS; ++i) {
        std::string h = userFile.GetValue("Hechizos", "H" + std::to_string(i));
        stats.UserHechizos[i] = h.empty() ? 0 : static_cast<int16_t>(std::stoi(h));
    }

    std::string gld = userFile.GetValue("STATS", "GLD");
    stats.GLD = gld.empty() ? 0 : std::stol(gld);

    std::string banco = userFile.GetValue("STATS", "BANCO");
    stats.Banco = banco.empty() ? 0 : std::stol(banco);

    std::string maxHp = userFile.GetValue("STATS", "MaxHP");
    stats.MaxHp = maxHp.empty() ? 0 : static_cast<int16_t>(std::stoi(maxHp));

    std::string minHp = userFile.GetValue("STATS", "MinHP");
    stats.MinHp = minHp.empty() ? 0 : static_cast<int16_t>(std::stoi(minHp));

    std::string minSta = userFile.GetValue("STATS", "MinSTA");
    stats.MinSta = minSta.empty() ? 0 : static_cast<int16_t>(std::stoi(minSta));

    std::string maxSta = userFile.GetValue("STATS", "MaxSTA");
    stats.MaxSta = maxSta.empty() ? 0 : static_cast<int16_t>(std::stoi(maxSta));

    std::string maxMan = userFile.GetValue("STATS", "MaxMAN");
    stats.MaxMAN = maxMan.empty() ? 0 : static_cast<int16_t>(std::stoi(maxMan));

    std::string minMan = userFile.GetValue("STATS", "MinMAN");
    stats.MinMAN = minMan.empty() ? 0 : static_cast<int16_t>(std::stoi(minMan));

    std::string maxHit = userFile.GetValue("STATS", "MaxHIT");
    stats.MaxHIT = maxHit.empty() ? 0 : static_cast<int16_t>(std::stoi(maxHit));

    std::string minHit = userFile.GetValue("STATS", "MinHIT");
    stats.MinHIT = minHit.empty() ? 0 : static_cast<int16_t>(std::stoi(minHit));

    std::string maxAgu = userFile.GetValue("STATS", "MaxAGU");
    stats.MaxAGU = maxAgu.empty() ? 0 : static_cast<int16_t>(std::stoi(maxAgu));

    std::string minAgu = userFile.GetValue("STATS", "MinAGU");
    stats.MinAGU = minAgu.empty() ? 0 : static_cast<int16_t>(std::stoi(minAgu));

    std::string maxHam = userFile.GetValue("STATS", "MaxHAM");
    stats.MaxHam = maxHam.empty() ? 0 : static_cast<int16_t>(std::stoi(maxHam));

    std::string minHam = userFile.GetValue("STATS", "MinHAM");
    stats.MinHam = minHam.empty() ? 0 : static_cast<int16_t>(std::stoi(minHam));

    std::string skillPts = userFile.GetValue("STATS", "SkillPtsLibres");
    stats.SkillPts = skillPts.empty() ? 0 : static_cast<int16_t>(std::stoi(skillPts));

    std::string expVal = userFile.GetValue("STATS", "EXP");
    stats.Exp = expVal.empty() ? 0.0 : std::stod(expVal);

    std::string eluVal = userFile.GetValue("STATS", "ELU");
    stats.ELU = eluVal.empty() ? 0 : std::stol(eluVal);

    std::string elvVal = userFile.GetValue("STATS", "ELV");
    stats.ELV = elvVal.empty() ? 0 : static_cast<uint8_t>(std::stoi(elvVal));

    std::string userMuertes = userFile.GetValue("MUERTES", "UserMuertes");
    stats.UsuariosMatados = userMuertes.empty() ? 0 : std::stol(userMuertes);

    std::string npcsMuertes = userFile.GetValue("MUERTES", "NpcsMuertes");
    stats.NPCsMuertos = npcsMuertes.empty() ? 0 : static_cast<int16_t>(std::stoi(npcsMuertes));

    std::string consP = userFile.GetValue("CONSEJO", "PERTENECE");
    if (!consP.empty() && std::stoi(consP) != 0) {
        user.flags.Privilegios = static_cast<PlayerType>(static_cast<int>(user.flags.Privilegios) | static_cast<int>(PlayerType::RoyalCouncil));
    }

    std::string consCaos = userFile.GetValue("CONSEJO", "PERTENECECAOS");
    if (!consCaos.empty() && std::stoi(consCaos) != 0) {
        user.flags.Privilegios = static_cast<PlayerType>(static_cast<int>(user.flags.Privilegios) | static_cast<int>(PlayerType::ChaosCouncil));
    }
}

void LoadUserReputacion(int userIndex, clsIniReader& userFile) {
    CheckUserIndexBounds(userIndex);
    auto& rep = UserList[userIndex].Reputacion;
    std::string valStr;

    valStr = userFile.GetValue("REP", "Asesino");
    rep.AsesinoRep = valStr.empty() ? 0 : std::stol(valStr);

    valStr = userFile.GetValue("REP", "Bandido");
    rep.BandidoRep = valStr.empty() ? 0 : std::stol(valStr);

    valStr = userFile.GetValue("REP", "Burguesia");
    rep.BurguesRep = valStr.empty() ? 0 : std::stol(valStr);

    valStr = userFile.GetValue("REP", "Ladrones");
    rep.LadronesRep = valStr.empty() ? 0 : std::stol(valStr);

    valStr = userFile.GetValue("REP", "Nobles");
    rep.NobleRep = valStr.empty() ? 0 : std::stol(valStr);

    valStr = userFile.GetValue("REP", "Plebe");
    rep.PlebeRep = valStr.empty() ? 0 : std::stol(valStr);

    valStr = userFile.GetValue("REP", "Promedio");
    rep.Promedio = valStr.empty() ? 0 : std::stol(valStr);
}

void LoadUserInit(int userIndex, clsIniReader& userFile) {
    CheckUserIndexBounds(userIndex);
    auto& user = UserList[userIndex];

    LoadUserStats(userIndex, userFile);
    LoadUserReputacion(userIndex, userFile);

    std::string valStr;

    valStr = userFile.GetValue("FACCIONES", "EjercitoReal");
    user.Faccion.ArmadaReal = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FACCIONES", "EjercitoCaos");
    user.Faccion.FuerzasCaos = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FACCIONES", "CiudMatados");
    user.Faccion.CiudadanosMatados = valStr.empty() ? 0 : std::stol(valStr);

    valStr = userFile.GetValue("FACCIONES", "CrimMatados");
    user.Faccion.CriminalesMatados = valStr.empty() ? 0 : std::stol(valStr);

    valStr = userFile.GetValue("FACCIONES", "rArCaos");
    user.Faccion.RecibioArmaduraCaos = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FACCIONES", "rArReal");
    user.Faccion.RecibioArmaduraReal = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FACCIONES", "rExCaos");
    user.Faccion.RecibioExpInicialCaos = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FACCIONES", "rExReal");
    user.Faccion.RecibioExpInicialReal = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FACCIONES", "recCaos");
    user.Faccion.RecompensasCaos = valStr.empty() ? 0 : std::stol(valStr);

    valStr = userFile.GetValue("FACCIONES", "recReal");
    user.Faccion.RecompensasReal = valStr.empty() ? 0 : std::stol(valStr);

    valStr = userFile.GetValue("FACCIONES", "Reenlistadas");
    user.Faccion.Reenlistadas = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FACCIONES", "NivelIngreso");
    user.Faccion.NivelIngreso = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    user.Faccion.FechaIngreso = userFile.GetValue("FACCIONES", "FechaIngreso");

    valStr = userFile.GetValue("FACCIONES", "MatadosIngreso");
    user.Faccion.MatadosIngreso = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FACCIONES", "NextRecompensa");
    user.Faccion.NextRecompensa = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FLAGS", "Muerto");
    user.flags.Muerto = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FLAGS", "Escondido");
    user.flags.Escondido = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FLAGS", "Hambre");
    user.flags.Hambre = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FLAGS", "Sed");
    user.flags.Sed = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FLAGS", "Desnudo");
    user.flags.Desnudo = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FLAGS", "Navegando");
    user.flags.Navegando = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FLAGS", "Envenenado");
    user.flags.Envenenado = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FLAGS", "Paralizado");
    user.flags.Paralizado = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = userFile.GetValue("FLAGS", "LastMap");
    user.flags.lastMap = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    if (user.flags.Paralizado == 1) {
        user.Counters.Paralisis = IntervaloParalizado;
    }

    valStr = userFile.GetValue("COUNTERS", "Pena");
    user.Counters.Pena = valStr.empty() ? 0 : std::stol(valStr);

    valStr = userFile.GetValue("COUNTERS", "SkillsAsignados");
    user.Counters.AsignedSkills = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    user.email = userFile.GetValue("CONTACTO", "Email");

    valStr = userFile.GetValue("INIT", "Genero");
    user.Genero = valStr.empty() ? eGenero::Hombre : static_cast<eGenero>(std::stoi(valStr));

    valStr = userFile.GetValue("INIT", "Clase");
    user.clase = valStr.empty() ? eClass::Mage : static_cast<eClass>(std::stoi(valStr));

    valStr = userFile.GetValue("INIT", "Raza");
    user.raza = valStr.empty() ? eRaza::Humano : static_cast<eRaza>(std::stoi(valStr));

    valStr = userFile.GetValue("INIT", "Hogar");
    user.Hogar = valStr.empty() ? eCiudad::cUllathorpe : static_cast<eCiudad>(std::stoi(valStr));

    valStr = userFile.GetValue("INIT", "Heading");
    user.char_appearance.heading = valStr.empty() ? eHeading::SOUTH : static_cast<eHeading>(std::stoi(valStr));

    valStr = userFile.GetValue("INIT", "Head");
    user.OrigChar.Head = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = userFile.GetValue("INIT", "Body");
    user.OrigChar.body = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = userFile.GetValue("INIT", "Arma");
    user.OrigChar.WeaponAnim = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = userFile.GetValue("INIT", "Escudo");
    user.OrigChar.ShieldAnim = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = userFile.GetValue("INIT", "Casco");
    user.OrigChar.CascoAnim = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    user.OrigChar.heading = eHeading::SOUTH;

#if ConUpTime
    valStr = userFile.GetValue("INIT", "UpTime");
    user.UpTime = valStr.empty() ? 0 : std::stol(valStr);
#endif

    if (user.flags.Muerto == 0) {
        user.char_appearance = user.OrigChar;
    } else {
        user.char_appearance.body = iCuerpoMuerto;
        user.char_appearance.Head = iCabezaMuerto;
        user.char_appearance.WeaponAnim = NingunArma;
        user.char_appearance.ShieldAnim = NingunEscudo;
        user.char_appearance.CascoAnim = NingunCasco;
    }

    user.desc = userFile.GetValue("INIT", "Desc");

    std::string posStr = userFile.GetValue("INIT", "Position");
    if (!posStr.empty()) {
        std::string mapS = ReadField(1, posStr, 45);
        std::string xS = ReadField(2, posStr, 45);
        std::string yS = ReadField(3, posStr, 45);
        user.Pos.Map = mapS.empty() ? 0 : static_cast<int16_t>(std::stoi(mapS));
        user.Pos.X = xS.empty() ? 0 : static_cast<int16_t>(std::stoi(xS));
        user.Pos.Y = yS.empty() ? 0 : static_cast<int16_t>(std::stoi(yS));
    }

    valStr = userFile.GetValue("Inventory", "CantidadItems");
    user.Invent.NroItems = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = userFile.GetValue("BancoInventory", "CantidadItems");
    user.BancoInvent.NroItems = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    for (int i = 1; i <= MAX_BANCOINVENTORY_SLOTS; ++i) {
        std::string objStr = userFile.GetValue("BancoInventory", "Obj" + std::to_string(i));
        if (!objStr.empty()) {
            std::string idx = ReadField(1, objStr, 45);
            std::string amt = ReadField(2, objStr, 45);
            user.BancoInvent.Object[i].ObjIndex = idx.empty() ? 0 : static_cast<int16_t>(std::stoi(idx));
            user.BancoInvent.Object[i].Amount = amt.empty() ? 0 : static_cast<int16_t>(std::stoi(amt));
        } else {
            user.BancoInvent.Object[i].ObjIndex = 0;
            user.BancoInvent.Object[i].Amount = 0;
        }
    }

    for (int i = 1; i <= MAX_INVENTORY_SLOTS; ++i) {
        std::string objStr = userFile.GetValue("Inventory", "Obj" + std::to_string(i));
        if (!objStr.empty()) {
            std::string idx = ReadField(1, objStr, 45);
            std::string amt = ReadField(2, objStr, 45);
            std::string eqp = ReadField(3, objStr, 45);
            user.Invent.Object[i].ObjIndex = idx.empty() ? 0 : static_cast<int16_t>(std::stoi(idx));
            user.Invent.Object[i].Amount = amt.empty() ? 0 : static_cast<int16_t>(std::stoi(amt));
            user.Invent.Object[i].Equipped = eqp.empty() ? 0 : static_cast<uint8_t>(std::stoi(eqp));
        } else {
            user.Invent.Object[i].ObjIndex = 0;
            user.Invent.Object[i].Amount = 0;
            user.Invent.Object[i].Equipped = 0;
        }
    }

    valStr = userFile.GetValue("Inventory", "WeaponEqpSlot");
    user.Invent.WeaponEqpSlot = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
    if (user.Invent.WeaponEqpSlot > 0 && user.Invent.WeaponEqpSlot <= MAX_INVENTORY_SLOTS) {
        user.Invent.WeaponEqpObjIndex = user.Invent.Object[user.Invent.WeaponEqpSlot].ObjIndex;
    }

    valStr = userFile.GetValue("Inventory", "ArmourEqpSlot");
    user.Invent.ArmourEqpSlot = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
    if (user.Invent.ArmourEqpSlot > 0 && user.Invent.ArmourEqpSlot <= MAX_INVENTORY_SLOTS) {
        user.Invent.ArmourEqpObjIndex = user.Invent.Object[user.Invent.ArmourEqpSlot].ObjIndex;
        user.flags.Desnudo = 0;
    } else {
        user.flags.Desnudo = 1;
    }

    valStr = userFile.GetValue("Inventory", "EscudoEqpSlot");
    user.Invent.EscudoEqpSlot = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
    if (user.Invent.EscudoEqpSlot > 0 && user.Invent.EscudoEqpSlot <= MAX_INVENTORY_SLOTS) {
        user.Invent.EscudoEqpObjIndex = user.Invent.Object[user.Invent.EscudoEqpSlot].ObjIndex;
    }

    valStr = userFile.GetValue("Inventory", "CascoEqpSlot");
    user.Invent.CascoEqpSlot = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
    if (user.Invent.CascoEqpSlot > 0 && user.Invent.CascoEqpSlot <= MAX_INVENTORY_SLOTS) {
        user.Invent.CascoEqpObjIndex = user.Invent.Object[user.Invent.CascoEqpSlot].ObjIndex;
    }

    valStr = userFile.GetValue("Inventory", "BarcoSlot");
    user.Invent.BarcoSlot = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
    if (user.Invent.BarcoSlot > 0 && user.Invent.BarcoSlot <= MAX_INVENTORY_SLOTS) {
        user.Invent.BarcoObjIndex = user.Invent.Object[user.Invent.BarcoSlot].ObjIndex;
    }

    valStr = userFile.GetValue("Inventory", "MunicionSlot");
    user.Invent.MunicionEqpSlot = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
    if (user.Invent.MunicionEqpSlot > 0 && user.Invent.MunicionEqpSlot <= MAX_INVENTORY_SLOTS) {
        user.Invent.MunicionEqpObjIndex = user.Invent.Object[user.Invent.MunicionEqpSlot].ObjIndex;
    }

    valStr = userFile.GetValue("Inventory", "AnilloSlot");
    user.Invent.AnilloEqpSlot = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
    if (user.Invent.AnilloEqpSlot > 0 && user.Invent.AnilloEqpSlot <= MAX_INVENTORY_SLOTS) {
        user.Invent.AnilloEqpObjIndex = user.Invent.Object[user.Invent.AnilloEqpSlot].ObjIndex;
    }

    valStr = userFile.GetValue("Inventory", "MochilaSlot");
    user.Invent.MochilaEqpSlot = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
    if (user.Invent.MochilaEqpSlot > 0 && user.Invent.MochilaEqpSlot <= MAX_INVENTORY_SLOTS) {
        user.Invent.MochilaEqpObjIndex = user.Invent.Object[user.Invent.MochilaEqpSlot].ObjIndex;
    }

    valStr = userFile.GetValue("MASCOTAS", "NroMascotas");
    user.NroMascotas = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    for (int i = 1; i <= MAXMASCOTAS; ++i) {
        std::string masStr = userFile.GetValue("MASCOTAS", "MAS" + std::to_string(i));
        user.MascotasType[i] = masStr.empty() ? 0 : static_cast<int16_t>(std::stoi(masStr));
    }

    std::string guildIndexStr = userFile.GetValue("Guild", "GUILDINDEX");
    if (!guildIndexStr.empty()) {
        try {
            user.GuildIndex = static_cast<int16_t>(std::stoi(guildIndexStr));
        } catch (...) {
            user.GuildIndex = 0;
        }
    } else {
        user.GuildIndex = 0;
    }
}

void SaveUser(int userIndex, const std::string& userFile) {
    CheckUserIndexBounds(userIndex);
    const auto& user = UserList[userIndex];

    for (int loopC = 1; loopC <= NUMATRIBUTOS; ++loopC) {
        WriteVar(userFile, "ATRIBUTOS", "AT" + std::to_string(loopC), std::to_string(user.Stats.UserAtributos[loopC]));
    }

    for (int loopC = 1; loopC <= NUMSKILLS; ++loopC) {
        WriteVar(userFile, "SKILLS", "SK" + std::to_string(loopC), std::to_string(user.Stats.UserSkills[loopC]));
        WriteVar(userFile, "SKILLS", "ELUSK" + std::to_string(loopC), std::to_string(user.Stats.EluSkills[loopC]));
        WriteVar(userFile, "SKILLS", "EXPSK" + std::to_string(loopC), std::to_string(user.Stats.ExpSkills[loopC]));
    }

    WriteVar(userFile, "CONTACTO", "Email", user.email);

    WriteVar(userFile, "INIT", "Genero", std::to_string(static_cast<int>(user.Genero)));
    WriteVar(userFile, "INIT", "Raza", std::to_string(static_cast<int>(user.raza)));
    WriteVar(userFile, "INIT", "Hogar", std::to_string(static_cast<int>(user.Hogar)));
    WriteVar(userFile, "INIT", "Clase", std::to_string(static_cast<int>(user.clase)));
    WriteVar(userFile, "INIT", "Desc", user.desc);

    WriteVar(userFile, "INIT", "Heading", std::to_string(static_cast<int>(user.char_appearance.heading)));
    WriteVar(userFile, "INIT", "Head", std::to_string(user.OrigChar.Head));

    if (user.flags.Muerto == 0) {
        WriteVar(userFile, "INIT", "Body", std::to_string(user.char_appearance.body));
    }

    WriteVar(userFile, "INIT", "Arma", std::to_string(user.char_appearance.WeaponAnim));
    WriteVar(userFile, "INIT", "Escudo", std::to_string(user.char_appearance.ShieldAnim));
    WriteVar(userFile, "INIT", "Casco", std::to_string(user.char_appearance.CascoAnim));

#if ConUpTime
    WriteVar(userFile, "INIT", "UpTime", std::to_string(user.UpTime));
#endif

    std::string lastIP1 = GetVar(userFile, "INIT", "LastIP1");
    if (lastIP1.empty()) {
        WriteVar(userFile, "INIT", "LastIP1", user.ip + " - 01/01/2026:00:00");
    } else {
        size_t spacePos = lastIP1.find(' ');
        std::string prevIp = (spacePos != std::string::npos) ? lastIP1.substr(0, spacePos) : lastIP1;
        if (user.ip != prevIp) {
            for (int i = 5; i >= 2; --i) {
                std::string prevVal = GetVar(userFile, "INIT", "LastIP" + std::to_string(i - 1));
                if (!prevVal.empty()) {
                    WriteVar(userFile, "INIT", "LastIP" + std::to_string(i), prevVal);
                }
            }
            WriteVar(userFile, "INIT", "LastIP1", user.ip + " - 01/01/2026:00:00");
        } else {
            WriteVar(userFile, "INIT", "LastIP1", user.ip + " - 01/01/2026:00:00");
        }
    }

    WriteVar(userFile, "INIT", "Position", std::to_string(user.Pos.Map) + "-" + std::to_string(user.Pos.X) + "-" + std::to_string(user.Pos.Y));

    WriteVar(userFile, "STATS", "GLD", std::to_string(user.Stats.GLD));
    WriteVar(userFile, "STATS", "BANCO", std::to_string(user.Stats.Banco));

    WriteVar(userFile, "STATS", "MaxHP", std::to_string(user.Stats.MaxHp));
    WriteVar(userFile, "STATS", "MinHP", std::to_string(user.Stats.MinHp));

    WriteVar(userFile, "STATS", "MaxSTA", std::to_string(user.Stats.MaxSta));
    WriteVar(userFile, "STATS", "MinSTA", std::to_string(user.Stats.MinSta));

    WriteVar(userFile, "STATS", "MaxMAN", std::to_string(user.Stats.MaxMAN));
    WriteVar(userFile, "STATS", "MinMAN", std::to_string(user.Stats.MinMAN));

    WriteVar(userFile, "STATS", "MaxHIT", std::to_string(user.Stats.MaxHIT));
    WriteVar(userFile, "STATS", "MinHIT", std::to_string(user.Stats.MinHIT));

    WriteVar(userFile, "STATS", "MaxAGU", std::to_string(user.Stats.MaxAGU));
    WriteVar(userFile, "STATS", "MinAGU", std::to_string(user.Stats.MinAGU));

    WriteVar(userFile, "STATS", "MaxHAM", std::to_string(user.Stats.MaxHam));
    WriteVar(userFile, "STATS", "MinHAM", std::to_string(user.Stats.MinHam));

    WriteVar(userFile, "STATS", "SkillPtsLibres", std::to_string(user.Stats.SkillPts));

    WriteVar(userFile, "STATS", "EXP", std::to_string(static_cast<long long>(user.Stats.Exp)));
    WriteVar(userFile, "STATS", "ELV", std::to_string(static_cast<int>(user.Stats.ELV)));
    WriteVar(userFile, "STATS", "ELU", std::to_string(user.Stats.ELU));

    WriteVar(userFile, "MUERTES", "UserMuertes", std::to_string(user.Stats.UsuariosMatados));
    WriteVar(userFile, "MUERTES", "NpcsMuertes", std::to_string(user.Stats.NPCsMuertos));

    WriteVar(userFile, "BancoInventory", "CantidadItems", std::to_string(user.BancoInvent.NroItems));
    for (int loopd = 1; loopd <= MAX_BANCOINVENTORY_SLOTS; ++loopd) {
        WriteVar(userFile, "BancoInventory", "Obj" + std::to_string(loopd),
                 std::to_string(user.BancoInvent.Object[loopd].ObjIndex) + "-" + std::to_string(user.BancoInvent.Object[loopd].Amount));
    }

    WriteVar(userFile, "Inventory", "CantidadItems", std::to_string(user.Invent.NroItems));
    for (int loopC = 1; loopC <= MAX_INVENTORY_SLOTS; ++loopC) {
        WriteVar(userFile, "Inventory", "Obj" + std::to_string(loopC),
                 std::to_string(user.Invent.Object[loopC].ObjIndex) + "-" + std::to_string(user.Invent.Object[loopC].Amount) + "-" + std::to_string(static_cast<int>(user.Invent.Object[loopC].Equipped)));
    }

    WriteVar(userFile, "Inventory", "WeaponEqpSlot", std::to_string(static_cast<int>(user.Invent.WeaponEqpSlot)));
    WriteVar(userFile, "Inventory", "ArmourEqpSlot", std::to_string(static_cast<int>(user.Invent.ArmourEqpSlot)));
    WriteVar(userFile, "Inventory", "CascoEqpSlot", std::to_string(static_cast<int>(user.Invent.CascoEqpSlot)));
    WriteVar(userFile, "Inventory", "EscudoEqpSlot", std::to_string(static_cast<int>(user.Invent.EscudoEqpSlot)));
    WriteVar(userFile, "Inventory", "BarcoSlot", std::to_string(static_cast<int>(user.Invent.BarcoSlot)));
    WriteVar(userFile, "Inventory", "MunicionSlot", std::to_string(static_cast<int>(user.Invent.MunicionEqpSlot)));
    WriteVar(userFile, "Inventory", "MochilaSlot", std::to_string(static_cast<int>(user.Invent.MochilaEqpSlot)));
    WriteVar(userFile, "Inventory", "AnilloSlot", std::to_string(static_cast<int>(user.Invent.AnilloEqpSlot)));

    WriteVar(userFile, "REP", "Asesino", std::to_string(user.Reputacion.AsesinoRep));
    WriteVar(userFile, "REP", "Bandido", std::to_string(user.Reputacion.BandidoRep));
    WriteVar(userFile, "REP", "Burguesia", std::to_string(user.Reputacion.BurguesRep));
    WriteVar(userFile, "REP", "Ladrones", std::to_string(user.Reputacion.LadronesRep));
    WriteVar(userFile, "REP", "Nobles", std::to_string(user.Reputacion.NobleRep));
    WriteVar(userFile, "REP", "Plebe", std::to_string(user.Reputacion.PlebeRep));

    long L = (-user.Reputacion.AsesinoRep) +
             (-user.Reputacion.BandidoRep) +
             user.Reputacion.BurguesRep +
             (-user.Reputacion.LadronesRep) +
             user.Reputacion.NobleRep +
             user.Reputacion.PlebeRep;
    L = L / 6;
    WriteVar(userFile, "REP", "Promedio", std::to_string(L));

    for (int loopC = 1; loopC <= MAXUSERHECHIZOS; ++loopC) {
        WriteVar(userFile, "HECHIZOS", "H" + std::to_string(loopC), std::to_string(user.Stats.UserHechizos[loopC]));
    }

    long nroMascotas = user.NroMascotas;
    for (int loopC = 1; loopC <= MAXMASCOTAS; ++loopC) {
        std::string cad = std::to_string(user.MascotasType[loopC]);
        WriteVar(userFile, "MASCOTAS", "MAS" + std::to_string(loopC), cad);
    }
    WriteVar(userFile, "MASCOTAS", "NroMascotas", std::to_string(nroMascotas));
}


// ============================================================================
// GRUPO 3: Configuración del Servidor y Roles Administrativos (Server.ini & Roles)
// ============================================================================

static std::string ToUpperCopy(const std::string& str) {
    std::string copy = str;
    for (char& c : copy) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return copy;
}

static bool CheckRole(const std::string& name, const std::string& sectionName, const std::string& keyPrefix) {
    std::string iniFile = IniPath.empty() ? "Server.ini" : (IniPath + "Server.ini");
    std::string numWizsStr = GetVar(iniFile, "INIT", sectionName);
    if (numWizsStr.empty()) return false;
    int numWizs = 0;
    try {
        numWizs = std::stoi(numWizsStr);
    } catch (...) {
        return false;
    }

    std::string targetUpper = ToUpperCopy(name);
    for (int wizNum = 1; wizNum <= numWizs; ++wizNum) {
        std::string nomB = ToUpperCopy(GetVar(iniFile, sectionName, keyPrefix + std::to_string(wizNum)));
        if (!nomB.empty() && (nomB.front() == '*' || nomB.front() == '+')) {
            nomB = nomB.substr(1);
        }
        if (targetUpper == nomB) {
            return true;
        }
    }
    return false;
}

bool EsAdmin(const std::string& name) {
    return CheckRole(name, "Admines", "Admin");
}

bool EsDios(const std::string& name) {
    return CheckRole(name, "Dioses", "Dios");
}

bool EsSemiDios(const std::string& name) {
    return CheckRole(name, "SemiDioses", "SemiDios");
}

bool EsConsejero(const std::string& name) {
    return CheckRole(name, "Consejeros", "Consejero");
}

bool EsRolesMaster(const std::string& name) {
    return CheckRole(name, "RolesMasters", "RM");
}

void LoadMotd() {
    std::string motdFile = DatPath.empty() ? "Dat/Motd.ini" : (DatPath + "Motd.ini");
    std::string linesStr = GetVar(motdFile, "INIT", "NumLines");
    int numLines = 0;
    if (!linesStr.empty()) {
        try {
            numLines = std::stoi(linesStr);
        } catch (...) {
            numLines = 0;
        }
    }
    if (numLines < 0) numLines = 0;

    MaxLines = static_cast<int16_t>(numLines);
    MOTD.clear();
    MOTD.resize(MaxLines + 1);

    for (int i = 1; i <= MaxLines; ++i) {
        MOTD[i].texto = GetVar(motdFile, "Motd", "Line" + std::to_string(i));
        MOTD[i].Formato.clear();
    }
}

void LoadSini() {
    std::string serverIni = IniPath.empty() ? "Server.ini" : (IniPath + "Server.ini");
    std::string valStr;

    valStr = GetVar(serverIni, "INIT", "IniciarDesdeBackUp");
    BootDelBackUp = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "StartPort");
    Puerto = valStr.empty() ? 7666 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "Hide");
    HideMe = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "AllowMultiLogins");
    AllowMultiLogins = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "IdleLimit");
    IdleLimit = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    ULTIMAVERSION = GetVar(serverIni, "INIT", "Version");

    valStr = GetVar(serverIni, "INIT", "PuedeCrearPersonajes");
    PuedeCrearPersonajes = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "ServerSoloGMs");
    ServerSoloGMs = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "ArmaduraImperial1");
    ArmaduraImperial1 = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "ArmaduraImperial2");
    ArmaduraImperial2 = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "ArmaduraImperial3");
    ArmaduraImperial3 = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "TunicaMagoImperial");
    TunicaMagoImperial = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "TunicaMagoImperialEnanos");
    TunicaMagoImperialEnanos = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "ArmaduraCaos1");
    ArmaduraCaos1 = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "ArmaduraCaos2");
    ArmaduraCaos2 = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "ArmaduraCaos3");
    ArmaduraCaos3 = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "TunicaMagoCaos");
    TunicaMagoCaos = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "TunicaMagoCaosEnanos");
    TunicaMagoCaosEnanos = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "VestimentaImperialHumano");
    VestimentaImperialHumano = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "VestimentaImperialEnano");
    VestimentaImperialEnano = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "TunicaConspicuaHumano");
    TunicaConspicuaHumano = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "TunicaConspicuaEnano");
    TunicaConspicuaEnano = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "ArmaduraNobilisimaHumano");
    ArmaduraNobilisimaHumano = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "ArmaduraNobilisimaEnano");
    ArmaduraNobilisimaEnano = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "ArmaduraGranSacerdote");
    ArmaduraGranSacerdote = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "VestimentaLegionHumano");
    VestimentaLegionHumano = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "VestimentaLegionEnano");
    VestimentaLegionEnano = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "TunicaLobregaHumano");
    TunicaLobregaHumano = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "TunicaLobregaEnano");
    TunicaLobregaEnano = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "TunicaEgregiaHumano");
    TunicaEgregiaHumano = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "TunicaEgregiaEnano");
    TunicaEgregiaEnano = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "SacerdoteDemoniaco");
    SacerdoteDemoniaco = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "MapaPretoriano");
    MAPA_PRETORIANO = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "Testing");
    EnTesting = valStr.empty() ? false : (std::stoi(valStr) != 0);

    // Intervalos
    valStr = GetVar(serverIni, "INTERVALOS", "SanaIntervaloSinDescansar");
    SanaIntervaloSinDescansar = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "StaminaIntervaloSinDescansar");
    StaminaIntervaloSinDescansar = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "SanaIntervaloDescansar");
    SanaIntervaloDescansar = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "StaminaIntervaloDescansar");
    StaminaIntervaloDescansar = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloSed");
    IntervaloSed = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloHambre");
    IntervaloHambre = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloVeneno");
    IntervaloVeneno = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloParalizado");
    IntervaloParalizado = valStr.empty() ? 500 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloInvisible");
    IntervaloInvisible = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloFrio");
    IntervaloFrio = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloWAVFX");
    IntervaloWavFx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloInvocacion");
    IntervaloInvocacion = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloParaConexion");
    IntervaloParaConexion = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    IntervaloPuedeSerAtacado = 5000;
    IntervaloAtacable = 60000;
    IntervaloOwnedNpc = 18000;

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloLanzaHechizo");
    IntervaloUserPuedeCastear = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloTrabajo");
    IntervaloUserPuedeTrabajar = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloUserPuedeAtacar");
    IntervaloUserPuedeAtacar = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloMagiaGolpe");
    IntervaloMagiaGolpe = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloGolpeMagia");
    IntervaloGolpeMagia = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloGolpeUsar");
    IntervaloGolpeUsar = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloWS");
    MinutosWs = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    if (MinutosWs < 60) MinutosWs = 180;

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloCerrarConexion");
    IntervaloCerrarConexion = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloUserPuedeUsar");
    IntervaloUserPuedeUsar = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloFlechasCazadores");
    IntervaloFlechasCazadores = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INTERVALOS", "IntervaloOculto");
    IntervaloOculto = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(serverIni, "INIT", "Record");
    recordusuarios = valStr.empty() ? 0 : std::stol(valStr);

    valStr = GetVar(serverIni, "INIT", "MaxUsers");
    int temporal = valStr.empty() ? 0 : std::stoi(valStr);
    if (MaxUsers == 0 && temporal > 0) {
        MaxUsers = static_cast<int16_t>(temporal);
        UserList.resize(MaxUsers + 1);
    }

    std::string ciudadesDat = DatPath.empty() ? "Dat/Ciudades.dat" : (DatPath + "Ciudades.dat");

    valStr = GetVar(ciudadesDat, "Ullathorpe", "Mapa");
    Ullathorpe.Map = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(ciudadesDat, "Ullathorpe", "X");
    Ullathorpe.X = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(ciudadesDat, "Ullathorpe", "Y");
    Ullathorpe.Y = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(ciudadesDat, "Nix", "Mapa");
    Nix.Map = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(ciudadesDat, "Nix", "X");
    Nix.X = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(ciudadesDat, "Nix", "Y");
    Nix.Y = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(ciudadesDat, "Banderbill", "Mapa");
    Banderbill.Map = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(ciudadesDat, "Banderbill", "X");
    Banderbill.X = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(ciudadesDat, "Banderbill", "Y");
    Banderbill.Y = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(ciudadesDat, "Lindos", "Mapa");
    Lindos.Map = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(ciudadesDat, "Lindos", "X");
    Lindos.X = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(ciudadesDat, "Lindos", "Y");
    Lindos.Y = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(ciudadesDat, "Arghal", "Mapa");
    Arghal.Map = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(ciudadesDat, "Arghal", "X");
    Arghal.X = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(ciudadesDat, "Arghal", "Y");
    Arghal.Y = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
}


// ============================================================================
// GRUPO 4: Carga y Guardado de Mapas Binarios e INI (.map, .inf, .dat)
// ============================================================================

namespace {

struct MapHeaderInternal {
    int16_t mapVersion{0};
    std::string desc;
    int32_t crc{0};
    int32_t magicWord{0};
    int16_t extraInts[4]{0, 0, 0, 0};
    int16_t infExtraInts[5]{0, 0, 0, 0, 0};
};

static std::unordered_map<long, MapHeaderInternal> s_mapHeaders;

inline int16_t readInt16LE(const uint8_t* ptr) {
    int16_t val;
    std::memcpy(&val, ptr, sizeof(val));
    return val;
}

inline int32_t readInt32LE(const uint8_t* ptr) {
    int32_t val;
    std::memcpy(&val, ptr, sizeof(val));
    return val;
}

inline void writeInt16LE(std::vector<uint8_t>& buf, int16_t val) {
    uint8_t bytes[sizeof(val)];
    std::memcpy(bytes, &val, sizeof(val));
    buf.insert(buf.end(), bytes, bytes + sizeof(val));
}

inline void writeInt32LE(std::vector<uint8_t>& buf, int32_t val) {
    uint8_t bytes[sizeof(val)];
    std::memcpy(bytes, &val, sizeof(val));
    buf.insert(buf.end(), bytes, bytes + sizeof(val));
}

inline size_t mapBlockIndex(long map, int x, int y) {
    return (static_cast<size_t>(map) * 101 + static_cast<size_t>(x)) * 101 + static_cast<size_t>(y);
}

} // anonymous namespace

void CargarMapa(long map, const std::string& mapFile) {
    if (map <= 0) return;

    std::string mapPath = mapFile + ".map";
    std::ifstream mapFileStream(mapPath, std::ios::binary);
    if (!mapFileStream.is_open()) return;

    std::vector<uint8_t> mapBuf((std::istreambuf_iterator<char>(mapFileStream)), std::istreambuf_iterator<char>());
    mapFileStream.close();

    std::string infPath = mapFile + ".inf";
    std::ifstream infFileStream(infPath, std::ios::binary);
    if (!infFileStream.is_open()) return;

    std::vector<uint8_t> infBuf((std::istreambuf_iterator<char>(infFileStream)), std::istreambuf_iterator<char>());
    infFileStream.close();

    if (mapBuf.size() < 273 || infBuf.size() < 10) return;

    size_t mapOffset = 0;
    int16_t mapVer = readInt16LE(mapBuf.data() + mapOffset);
    mapOffset += 2;

    const char* rawDesc = reinterpret_cast<const char*>(mapBuf.data() + mapOffset);
    mapOffset += 255;

    size_t lastPos = 255;
    while (lastPos > 0 && (rawDesc[lastPos - 1] == ' ' || rawDesc[lastPos - 1] == '\0')) {
        --lastPos;
    }
    std::string desc(rawDesc, lastPos);

    int32_t crc = readInt32LE(mapBuf.data() + mapOffset);
    mapOffset += 4;
    int32_t magic = readInt32LE(mapBuf.data() + mapOffset);
    mapOffset += 4;

    int16_t extraInts[4];
    for (int i = 0; i < 4; ++i) {
        extraInts[i] = readInt16LE(mapBuf.data() + mapOffset);
        mapOffset += 2;
    }

    size_t infOffset = 0;
    int16_t infExtraInts[5];
    for (int i = 0; i < 5; ++i) {
        infExtraInts[i] = readInt16LE(infBuf.data() + infOffset);
        infOffset += 2;
    }

    auto& hdr = s_mapHeaders[map];
    hdr.mapVersion = mapVer;
    hdr.desc = desc;
    hdr.crc = crc;
    hdr.magicWord = magic;
    for (int i = 0; i < 4; ++i) hdr.extraInts[i] = extraInts[i];
    for (int i = 0; i < 5; ++i) hdr.infExtraInts[i] = infExtraInts[i];

    if (MapInfoList.size() <= static_cast<size_t>(map)) {
        MapInfoList.resize(static_cast<size_t>(map) + 1);
    }
    MapInfoList[map].MapVersion = mapVer;

    size_t requiredSize = (static_cast<size_t>(map) + 1) * 101 * 101;
    if (MapData.size() < requiredSize) {
        MapData.resize(requiredSize);
    }

    for (int y = YMinMapSize; y <= YMaxMapSize; ++y) {
        for (int x = XMinMapSize; x <= XMaxMapSize; ++x) {
            size_t idx = mapBlockIndex(map, x, y);
            auto& block = MapData[idx];
            block = MapBlock{};

            if (mapOffset < mapBuf.size()) {
                uint8_t byFlags = mapBuf[mapOffset++];
                if (byFlags & 1) {
                    block.Blocked = 1;
                }
                if (mapOffset + 2 <= mapBuf.size()) {
                    block.Graphic[1] = readInt16LE(mapBuf.data() + mapOffset);
                    mapOffset += 2;
                }
                if (byFlags & 2) {
                    if (mapOffset + 2 <= mapBuf.size()) {
                        block.Graphic[2] = readInt16LE(mapBuf.data() + mapOffset);
                        mapOffset += 2;
                    }
                }
                if (byFlags & 4) {
                    if (mapOffset + 2 <= mapBuf.size()) {
                        block.Graphic[3] = readInt16LE(mapBuf.data() + mapOffset);
                        mapOffset += 2;
                    }
                }
                if (byFlags & 8) {
                    if (mapOffset + 2 <= mapBuf.size()) {
                        block.Graphic[4] = readInt16LE(mapBuf.data() + mapOffset);
                        mapOffset += 2;
                    }
                }
                if (byFlags & 16) {
                    if (mapOffset + 2 <= mapBuf.size()) {
                        int16_t tr = readInt16LE(mapBuf.data() + mapOffset);
                        block.trigger = static_cast<eTrigger>(tr);
                        mapOffset += 2;
                    }
                }
            }

            if (infOffset < infBuf.size()) {
                uint8_t byFlags = infBuf[infOffset++];
                if (byFlags & 1) {
                    if (infOffset + 6 <= infBuf.size()) {
                        block.TileExit.Map = readInt16LE(infBuf.data() + infOffset); infOffset += 2;
                        block.TileExit.X = readInt16LE(infBuf.data() + infOffset); infOffset += 2;
                        block.TileExit.Y = readInt16LE(infBuf.data() + infOffset); infOffset += 2;
                    }
                }
                if (byFlags & 2) {
                    if (infOffset + 2 <= infBuf.size()) {
                        int16_t npcNum = readInt16LE(infBuf.data() + infOffset); infOffset += 2;
                        block.NpcIndex = npcNum;
                        if (npcNum > 0 && npcNum <= MAXNPCS && Npclist[npcNum].Numero == 0) {
                            Npclist[npcNum].Numero = npcNum;
                        }
                    }
                }
                if (byFlags & 4) {
                    if (infOffset + 4 <= infBuf.size()) {
                        block.ObjInfo.ObjIndex = readInt16LE(infBuf.data() + infOffset); infOffset += 2;
                        block.ObjInfo.Amount = readInt16LE(infBuf.data() + infOffset); infOffset += 2;
                    }
                }
            }
        }
    }

    std::string datFile = mapFile + ".dat";
    std::string sec = "Mapa" + std::to_string(map);
    auto& info = MapInfoList[map];

    info.name = GetVar(datFile, sec, "Name");
    info.Music = GetVar(datFile, sec, "MusicNum");

    std::string startPosStr = GetVar(datFile, sec, "StartPos");
    if (!startPosStr.empty()) {
        info.StartPos.Map = static_cast<int16_t>(std::atoi(ReadField(1, startPosStr, '-').c_str()));
        info.StartPos.X = static_cast<int16_t>(std::atoi(ReadField(2, startPosStr, '-').c_str()));
        info.StartPos.Y = static_cast<int16_t>(std::atoi(ReadField(3, startPosStr, '-').c_str()));
    }

    std::string valStr;
    valStr = GetVar(datFile, sec, "MagiaSinEfecto");
    if (valStr.empty()) valStr = GetVar(datFile, "mapa" + std::to_string(map), "MagiaSinEfecto");
    info.MagiaSinEfecto = valStr.empty() ? 0 : static_cast<uint8_t>(std::atoi(valStr.c_str()));

    valStr = GetVar(datFile, sec, "InviSinEfecto");
    if (valStr.empty()) valStr = GetVar(datFile, "mapa" + std::to_string(map), "InviSinEfecto");
    info.InviSinEfecto = valStr.empty() ? 0 : static_cast<uint8_t>(std::atoi(valStr.c_str()));

    valStr = GetVar(datFile, sec, "ResuSinEfecto");
    if (valStr.empty()) valStr = GetVar(datFile, "mapa" + std::to_string(map), "ResuSinEfecto");
    info.ResuSinEfecto = valStr.empty() ? 0 : static_cast<uint8_t>(std::atoi(valStr.c_str()));

    valStr = GetVar(datFile, sec, "NoEncriptarMP");
    info.NoEncriptarMP = valStr.empty() ? 0 : static_cast<uint8_t>(std::atoi(valStr.c_str()));

    valStr = GetVar(datFile, sec, "RoboNpcsPermitido");
    info.RoboNpcsPermitido = valStr.empty() ? 0 : static_cast<uint8_t>(std::atoi(valStr.c_str()));

    valStr = GetVar(datFile, sec, "Pk");
    if (!valStr.empty()) {
        info.Pk = (std::atoi(valStr.c_str()) == 0);
    } else {
        info.Pk = true;
    }

    info.Terreno = GetVar(datFile, sec, "Terreno");
    info.Zona = GetVar(datFile, sec, "Zona");
    info.Restringir = GetVar(datFile, sec, "Restringir");

    valStr = GetVar(datFile, sec, "BackUp");
    info.BackUp = valStr.empty() ? 0 : static_cast<uint8_t>(std::atoi(valStr.c_str()));
}

void GrabarMapa(long map, const std::string& mapFile) {
    if (map <= 0) return;
    if (MapData.size() < (static_cast<size_t>(map) + 1) * 101 * 101) return;

    MapHeaderInternal hdr;
    auto it = s_mapHeaders.find(map);
    if (it != s_mapHeaders.end()) {
        hdr = it->second;
    }
    if (MapInfoList.size() > static_cast<size_t>(map)) {
        hdr.mapVersion = MapInfoList[map].MapVersion;
    }

    std::vector<uint8_t> mapBuf;
    mapBuf.reserve(35000);

    writeInt16LE(mapBuf, hdr.mapVersion);

    char paddedDesc[255];
    std::memset(paddedDesc, ' ', 255);
    size_t copyLen = std::min(hdr.desc.size(), static_cast<size_t>(255));
    std::memcpy(paddedDesc, hdr.desc.data(), copyLen);
    mapBuf.insert(mapBuf.end(), paddedDesc, paddedDesc + 255);

    writeInt32LE(mapBuf, hdr.crc);
    writeInt32LE(mapBuf, hdr.magicWord);

    for (int i = 0; i < 4; ++i) {
        writeInt16LE(mapBuf, hdr.extraInts[i]);
    }

    for (int y = YMinMapSize; y <= YMaxMapSize; ++y) {
        for (int x = XMinMapSize; x <= XMaxMapSize; ++x) {
            size_t idx = mapBlockIndex(map, x, y);
            const auto& block = MapData[idx];

            uint8_t byFlags = 0;
            if (block.Blocked) byFlags |= 1;
            if (block.Graphic[2] != 0) byFlags |= 2;
            if (block.Graphic[3] != 0) byFlags |= 4;
            if (block.Graphic[4] != 0) byFlags |= 8;
            if (static_cast<int16_t>(block.trigger) != 0) byFlags |= 16;

            mapBuf.push_back(byFlags);
            writeInt16LE(mapBuf, block.Graphic[1]);

            for (int c = 2; c <= 4; ++c) {
                if (block.Graphic[c] != 0) {
                    writeInt16LE(mapBuf, block.Graphic[c]);
                }
            }

            if (static_cast<int16_t>(block.trigger) != 0) {
                writeInt16LE(mapBuf, static_cast<int16_t>(block.trigger));
            }
        }
    }

    std::string mapFilePath = mapFile + ".map";
    std::ofstream mapOfs(mapFilePath, std::ios::binary | std::ios::trunc);
    if (mapOfs.is_open()) {
        mapOfs.write(reinterpret_cast<const char*>(mapBuf.data()), mapBuf.size());
        mapOfs.close();
    }

    std::vector<uint8_t> infBuf;
    infBuf.reserve(15000);

    for (int i = 0; i < 5; ++i) {
        writeInt16LE(infBuf, hdr.infExtraInts[i]);
    }

    for (int y = YMinMapSize; y <= YMaxMapSize; ++y) {
        for (int x = XMinMapSize; x <= XMaxMapSize; ++x) {
            size_t idx = mapBlockIndex(map, x, y);
            auto& block = MapData[idx];

            if (block.ObjInfo.ObjIndex > 0 && static_cast<size_t>(block.ObjInfo.ObjIndex) < ObjDataList.size()) {
                if (ObjDataList[block.ObjInfo.ObjIndex].OBJType == eOBJType::otFogata) {
                    block.ObjInfo.ObjIndex = 0;
                    block.ObjInfo.Amount = 0;
                }
            }

            uint8_t byFlags = 0;
            if (block.TileExit.Map != 0) byFlags |= 1;
            if (block.NpcIndex != 0) byFlags |= 2;
            if (block.ObjInfo.ObjIndex != 0) byFlags |= 4;

            infBuf.push_back(byFlags);

            if (block.TileExit.Map != 0) {
                writeInt16LE(infBuf, block.TileExit.Map);
                writeInt16LE(infBuf, block.TileExit.X);
                writeInt16LE(infBuf, block.TileExit.Y);
            }

            if (block.NpcIndex != 0) {
                int16_t npcNum = block.NpcIndex;
                if (block.NpcIndex > 0 && block.NpcIndex <= MAXNPCS && Npclist[block.NpcIndex].Numero != 0) {
                    npcNum = Npclist[block.NpcIndex].Numero;
                }
                writeInt16LE(infBuf, npcNum);
            }

            if (block.ObjInfo.ObjIndex != 0) {
                writeInt16LE(infBuf, block.ObjInfo.ObjIndex);
                writeInt16LE(infBuf, block.ObjInfo.Amount);
            }
        }
    }

    std::string infFilePath = mapFile + ".inf";
    std::ofstream infOfs(infFilePath, std::ios::binary | std::ios::trunc);
    if (infOfs.is_open()) {
        infOfs.write(reinterpret_cast<const char*>(infBuf.data()), infBuf.size());
        infOfs.close();
    }

    if (MapInfoList.size() > static_cast<size_t>(map)) {
        const auto& info = MapInfoList[map];
        std::string datPath = mapFile + ".dat";
        std::string sec = "Mapa" + std::to_string(map);

        WriteVar(datPath, sec, "Name", info.name);
        WriteVar(datPath, sec, "MusicNum", info.Music);
        WriteVar(datPath, "mapa" + std::to_string(map), "MagiaSinefecto", std::to_string(info.MagiaSinEfecto));
        WriteVar(datPath, "mapa" + std::to_string(map), "InviSinEfecto", std::to_string(info.InviSinEfecto));
        WriteVar(datPath, "mapa" + std::to_string(map), "ResuSinEfecto", std::to_string(info.ResuSinEfecto));
        WriteVar(datPath, sec, "StartPos", std::to_string(info.StartPos.Map) + "-" + std::to_string(info.StartPos.X) + "-" + std::to_string(info.StartPos.Y));
        WriteVar(datPath, sec, "Terreno", info.Terreno);
        WriteVar(datPath, sec, "Zona", info.Zona);
        WriteVar(datPath, sec, "Restringir", info.Restringir);
        WriteVar(datPath, sec, "BackUp", std::to_string(info.BackUp));
        WriteVar(datPath, sec, "Pk", info.Pk ? "0" : "1");
    }
}

int getLimit(int mapa, uint8_t side) {
    if (mapa <= 0 || mapa > NumMaps) return 0;
    if (MapData.size() < (static_cast<size_t>(mapa) + 1) * 101 * 101) return 0;

    for (int x = 15; x <= 87; ++x) {
        for (int y = 0; y <= 3; ++y) {
            int limit = 0;
            switch (side) {
                case eHeading::NORTH:
                    limit = MapData[mapBlockIndex(mapa, x, 7 + y)].TileExit.Map;
                    break;
                case eHeading::EAST:
                    limit = MapData[mapBlockIndex(mapa, 92 - y, x)].TileExit.Map;
                    break;
                case eHeading::SOUTH:
                    limit = MapData[mapBlockIndex(mapa, x, 94 - y)].TileExit.Map;
                    break;
                case eHeading::WEST:
                    limit = MapData[mapBlockIndex(mapa, 9 + y, x)].TileExit.Map;
                    break;
                default:
                    break;
            }
            if (limit > 0) return limit;
        }
    }
    return 0;
}

void setDistance(int mapa, uint8_t city, int side, int x, int y) {
    if (mapa <= 0 || mapa > NumMaps) return;
    if (city == 0 || city > NUMCIUDADES) return;
    if (distanceToCities.size() <= static_cast<size_t>(mapa)) return;

    if (distanceToCities[mapa].distanceToCity[city] >= 0) return;

    if (mapa == Ciudades[city].Map) {
        distanceToCities[mapa].distanceToCity[city] = 0;
    } else {
        distanceToCities[mapa].distanceToCity[city] = static_cast<int16_t>(std::abs(x) + std::abs(y));
    }

    for (int i = 1; i <= 4; ++i) {
        int lim = getLimit(mapa, static_cast<uint8_t>(i));
        if (lim > 0) {
            switch (i) {
                case eHeading::NORTH:
                    setDistance(lim, city, i, x, y + 1);
                    break;
                case eHeading::EAST:
                    setDistance(lim, city, i, x + 1, y);
                    break;
                case eHeading::SOUTH:
                    setDistance(lim, city, i, x, y - 1);
                    break;
                case eHeading::WEST:
                    setDistance(lim, city, i, x - 1, y);
                    break;
                default:
                    break;
            }
        }
    }
}

void generateMatrix(int /*mapa*/) {
    if (NumMaps <= 0) return;

    distanceToCities.assign(static_cast<size_t>(NumMaps) + 1, HomeDistance{});

    for (int j = 1; j <= NUMCIUDADES; ++j) {
        for (int i = 1; i <= NumMaps; ++i) {
            distanceToCities[i].distanceToCity[j] = -1;
        }
    }

    for (int j = 1; j <= NUMCIUDADES; ++j) {
        for (int i = 1; i <= 4; ++i) {
            switch (i) {
                case eHeading::NORTH:
                    setDistance(getLimit(Ciudades[j].Map, eHeading::NORTH), static_cast<uint8_t>(j), i, 0, 1);
                    break;
                case eHeading::EAST:
                    setDistance(getLimit(Ciudades[j].Map, eHeading::EAST), static_cast<uint8_t>(j), i, 1, 0);
                    break;
                case eHeading::SOUTH:
                    setDistance(getLimit(Ciudades[j].Map, eHeading::SOUTH), static_cast<uint8_t>(j), i, 0, 1);
                    break;
                case eHeading::WEST:
                    setDistance(getLimit(Ciudades[j].Map, eHeading::WEST), static_cast<uint8_t>(j), i, -1, 0);
                    break;
                default:
                    break;
            }
        }
    }
}

void LoadMapData() {
    std::string iniFile = DatPath.empty() ? "Dat/Map.dat" : (DatPath + "Map.dat");
    std::string valStr = GetVar(iniFile, "INIT", "NumMaps");
    NumMaps = valStr.empty() ? 0 : static_cast<int16_t>(std::atoi(valStr.c_str()));
    MapPath = GetVar(iniFile, "INIT", "MapPath");

    MapData.assign((static_cast<size_t>(NumMaps) + 1) * 101 * 101, MapBlock{});
    MapInfoList.resize(static_cast<size_t>(NumMaps) + 1);

    for (int map = 1; map <= NumMaps; ++map) {
        std::string tFileName;
        if (!MapPath.empty() && (MapPath.back() == '/' || MapPath.back() == '\\')) {
            tFileName = MapPath + "Mapa" + std::to_string(map);
        } else if (!MapPath.empty()) {
            tFileName = MapPath + "/Mapa" + std::to_string(map);
        } else {
            tFileName = "Mapa" + std::to_string(map);
        }
        CargarMapa(map, tFileName);
    }
}


// ============================================================================
// GRUPO 5: Carga de Tablas de Datos del Juego (Data-Table Loaders)
// ============================================================================

void CargarSpawnList() {
    std::string file = DatPath.empty() ? "Dat/Invokar.dat" : (DatPath + "Invokar.dat");
    std::string valStr = GetVar(file, "INIT", "NumNpcs");
    int n = valStr.empty() ? 0 : std::stoi(valStr);
    SpawnList.assign(n + 1, tCriaturasEntrenador{});
    for (int lc = 1; lc <= n; ++lc) {
        valStr = GetVar(file, "NPC" + std::to_string(lc), "NpcIndex");
        SpawnList[lc].NpcIndex = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        SpawnList[lc].NpcName = GetVar(file, "NPC" + std::to_string(lc), "NpcName");
    }
}

void CargarForbidenWords() {
    std::string file = DatPath.empty() ? "Dat/NombresInvalidos.txt" : (DatPath + "NombresInvalidos.txt");
    std::ifstream ifs(file, std::ios::binary);
    if (!ifs.is_open()) {
        ForbidenNames.clear();
        return;
    }
    std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    ifs.close();

    std::vector<std::string> lines;
    std::string current;
    for (char c : content) {
        if (c == '\r') continue;
        if (c == '\n') {
            lines.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    if (!current.empty()) {
        lines.push_back(current);
    }

    ForbidenNames.assign(lines.size() + 1, "");
    for (size_t i = 0; i < lines.size(); ++i) {
        ForbidenNames[i + 1] = lines[i];
    }
}

void CargarHechizos() {
    std::string file = DatPath.empty() ? "Dat/Hechizos.dat" : (DatPath + "Hechizos.dat");
    clsIniReader reader;
    reader.Initialize(file);
    std::string valStr = reader.GetValue("INIT", "NumeroHechizos");
    NumeroHechizos = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    Hechizos.assign(NumeroHechizos + 1, tHechizo{});

    for (int i = 1; i <= NumeroHechizos; ++i) {
        std::string sec = "HECHIZO" + std::to_string(i);
        auto& spell = Hechizos[i];
        spell.Nombre = reader.GetValue(sec, "Nombre");
        spell.desc = reader.GetValue(sec, "Desc");
        spell.PalabrasMagicas = reader.GetValue(sec, "PalabrasMagicas");
        spell.HechizeroMsg = reader.GetValue(sec, "HechizeroMsg");
        spell.TargetMsg = reader.GetValue(sec, "TargetMsg");
        spell.PropioMsg = reader.GetValue(sec, "PropioMsg");

        valStr = reader.GetValue(sec, "Tipo");
        spell.Tipo = valStr.empty() ? static_cast<TipoHechizo>(0) : static_cast<TipoHechizo>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "WAV");
        spell.WAV = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "FXgrh");
        spell.FXgrh = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Loops");
        spell.loops = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "SubeHP");
        spell.SubeHP = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MinHP");
        spell.MinHp = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MaxHP");
        spell.MaxHp = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "SubeMana");
        spell.SubeMana = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MinMana");
        spell.MiMana = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MaxMana");
        spell.MaMana = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "SubeSta");
        spell.SubeSta = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MinSta");
        spell.MinSta = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MaxSta");
        spell.MaxSta = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MinSkill");
        spell.MinSkill = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "ManaRequerido");
        spell.ManaRequerido = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "StaRequerido");
        spell.StaRequerido = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Target");
        spell.Target = valStr.empty() ? static_cast<TargetType>(0) : static_cast<TargetType>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "NeedStaff");
        spell.NeedStaff = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Ceguera");
        spell.Ceguera = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Estupidez");
        spell.Estupidez = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Invisibilidad");
        spell.Invisibilidad = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Paraliza");
        spell.Paraliza = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Inmoviliza");
        spell.Inmoviliza = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "RemoverParalisis");
        spell.RemoverParalisis = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "RemoverEstupidez");
        spell.RemoverEstupidez = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "CuraVeneno");
        spell.CuraVeneno = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Envenena");
        spell.Envenena = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Morph");
        spell.Morph = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Cant");
        spell.cant = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "NumNpc");
        spell.NumNpc = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "CantMascotas");
        spell.Invoca = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
    }
}

void LoadArmasHerreria() {
    std::string file = DatPath.empty() ? "Dat/ArmasHerrero.dat" : (DatPath + "ArmasHerrero.dat");
    std::string valStr = GetVar(file, "INIT", "NumArmas");
    int n = valStr.empty() ? 0 : std::stoi(valStr);
    ArmasHerrero.assign(n + 1, 0);
    for (int lc = 1; lc <= n; ++lc) {
        valStr = GetVar(file, "Arma" + std::to_string(lc), "Index");
        ArmasHerrero[lc] = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    }
}

void LoadArmadurasHerreria() {
    std::string file = DatPath.empty() ? "Dat/ArmadurasHerrero.dat" : (DatPath + "ArmadurasHerrero.dat");
    std::string valStr = GetVar(file, "INIT", "NumArmaduras");
    int n = valStr.empty() ? 0 : std::stoi(valStr);
    ArmadurasHerrero.assign(n + 1, 0);
    for (int lc = 1; lc <= n; ++lc) {
        valStr = GetVar(file, "Armadura" + std::to_string(lc), "Index");
        ArmadurasHerrero[lc] = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    }
}

void LoadBalance() {
    std::string file = DatPath.empty() ? "Dat/Balance.dat" : (DatPath + "Balance.dat");

    for (int i = 1; i <= NUMCLASES; ++i) {
        std::string claseName = (i >= 1 && i <= NUMCLASES) ? ListaClases[i] : "";
        if (claseName.empty()) continue;
        auto& modC = ModClaseList[i];
        std::string valStr = GetVar(file, "MODEVASION", claseName);
        modC.Evasion = valStr.empty() ? 0.0 : std::stod(valStr);
        valStr = GetVar(file, "MODATAQUEARMAS", claseName);
        modC.AtaqueArmas = valStr.empty() ? 0.0 : std::stod(valStr);
        valStr = GetVar(file, "MODATAQUEPROYECTILES", claseName);
        modC.AtaqueProyectiles = valStr.empty() ? 0.0 : std::stod(valStr);
        valStr = GetVar(file, "MODATAQUEWRESTLING", claseName);
        modC.AtaqueWrestling = valStr.empty() ? 0.0 : std::stod(valStr);
        valStr = GetVar(file, "MODDAÑOARMAS", claseName);
        modC.DañoArmas = valStr.empty() ? 0.0 : std::stod(valStr);
        valStr = GetVar(file, "MODDAÑOPROYECTILES", claseName);
        modC.DañoProyectiles = valStr.empty() ? 0.0 : std::stod(valStr);
        valStr = GetVar(file, "MODDAÑOWRESTLING", claseName);
        modC.DañoWrestling = valStr.empty() ? 0.0 : std::stod(valStr);
        valStr = GetVar(file, "MODESCUDO", claseName);
        modC.Escudo = valStr.empty() ? 0.0 : std::stod(valStr);
    }

    for (int i = 1; i <= NUMRAZAS; ++i) {
        std::string razaName = (i >= 1 && i <= NUMRAZAS) ? ListaRazas[i] : "";
        if (razaName.empty()) continue;
        auto& modR = ModRazaList[i];
        std::string valStr = GetVar(file, "MODRAZA", razaName + "Fuerza");
        modR.Fuerza = valStr.empty() ? 0.0f : std::stof(valStr);
        valStr = GetVar(file, "MODRAZA", razaName + "Agilidad");
        modR.Agilidad = valStr.empty() ? 0.0f : std::stof(valStr);
        valStr = GetVar(file, "MODRAZA", razaName + "Inteligencia");
        modR.Inteligencia = valStr.empty() ? 0.0f : std::stof(valStr);
        valStr = GetVar(file, "MODRAZA", razaName + "Carisma");
        modR.Carisma = valStr.empty() ? 0.0f : std::stof(valStr);
        valStr = GetVar(file, "MODRAZA", razaName + "Constitucion");
        modR.Constitucion = valStr.empty() ? 0.0f : std::stof(valStr);
    }

    for (int i = 1; i <= NUMCLASES; ++i) {
        std::string claseName = (i >= 1 && i <= NUMCLASES) ? ListaClases[i] : "";
        if (claseName.empty()) continue;
        std::string valStr = GetVar(file, "MODVIDA", claseName);
        ModVida[i] = valStr.empty() ? 0.0 : std::stod(valStr);
    }

    for (int i = 1; i <= 5; ++i) {
        std::string valStr = GetVar(file, "DISTRIBUCION", "E" + std::to_string(i));
        DistribucionEnteraVida[i] = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    }
    for (int i = 1; i <= 4; ++i) {
        std::string valStr = GetVar(file, "DISTRIBUCION", "S" + std::to_string(i));
        DistribucionSemienteraVida[i] = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    }

    std::string valStr = GetVar(file, "EXTRA", "PorcentajeRecuperoMana");
    PorcentajeRecuperoMana = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(file, "PARTY", "ExponenteNivelParty");
    ExponenteNivelParty = valStr.empty() ? 0.0f : std::stof(valStr);

    for (int i = 1; i <= NUM_RANGOS_FACCION; ++i) {
        valStr = GetVar(file, "RECOMPENSAFACCION", "Rango" + std::to_string(i));
        RecompensaFacciones[i - 1] = valStr.empty() ? 0 : std::stol(valStr);
    }
}

void LoadObjCarpintero() {
    std::string file = DatPath.empty() ? "Dat/ObjCarpintero.dat" : (DatPath + "ObjCarpintero.dat");
    std::string valStr = GetVar(file, "INIT", "NumObjs");
    int n = valStr.empty() ? 0 : std::stoi(valStr);
    ObjCarpintero.assign(n + 1, 0);
    for (int lc = 1; lc <= n; ++lc) {
        valStr = GetVar(file, "Obj" + std::to_string(lc), "Index");
        ObjCarpintero[lc] = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    }
}

void LoadOBJData() {
    std::string file = DatPath.empty() ? "Dat/Obj.dat" : (DatPath + "Obj.dat");
    clsIniReader reader;
    reader.Initialize(file);

    std::string valStr = reader.GetValue("INIT", "NumObjs");
    NumObjDatas = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    ObjDataList.assign(NumObjDatas + 1, ObjData{});

    for (int object = 1; object <= NumObjDatas; ++object) {
        std::string sec = "OBJ" + std::to_string(object);
        auto& obj = ObjDataList[object];

        obj.name = reader.GetValue(sec, "Name");

        valStr = reader.GetValue(sec, "Log");
        obj.Log = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "NoLog");
        obj.NoLog = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "GrhIndex");
        obj.GrhIndex = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "ObjType");
        obj.OBJType = valStr.empty() ? static_cast<eOBJType>(0) : static_cast<eOBJType>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Newbie");
        obj.Newbie = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        switch (obj.OBJType) {
            case eOBJType::otArmadura:
                valStr = reader.GetValue(sec, "Real");
                obj.Real = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Caos");
                obj.Caos = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingH");
                obj.LingH = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingP");
                obj.LingP = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingO");
                obj.LingO = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "SkHerreria");
                obj.SkHerreria = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                break;

            case eOBJType::otESCUDO:
                valStr = reader.GetValue(sec, "Anim");
                obj.ShieldAnim = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingH");
                obj.LingH = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingP");
                obj.LingP = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingO");
                obj.LingO = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "SkHerreria");
                obj.SkHerreria = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Real");
                obj.Real = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Caos");
                obj.Caos = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                break;

            case eOBJType::otCASCO:
                valStr = reader.GetValue(sec, "Anim");
                obj.CascoAnim = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingH");
                obj.LingH = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingP");
                obj.LingP = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingO");
                obj.LingO = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "SkHerreria");
                obj.SkHerreria = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Real");
                obj.Real = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Caos");
                obj.Caos = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                break;

            case eOBJType::otWeapon:
                valStr = reader.GetValue(sec, "Anim");
                obj.WeaponAnim = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Apuñala");
                obj.Apuñala = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Envenena");
                obj.Envenena = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "MaxHIT");
                obj.MaxHIT = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "MinHIT");
                obj.MinHIT = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Proyectil");
                obj.proyectil = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Municiones");
                obj.Municion = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "StaffPower");
                obj.StaffPower = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "StaffDamageBonus");
                obj.StaffDamageBonus = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Refuerzo");
                obj.Refuerzo = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingH");
                obj.LingH = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingP");
                obj.LingP = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingO");
                obj.LingO = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "SkHerreria");
                obj.SkHerreria = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Real");
                obj.Real = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Caos");
                obj.Caos = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "RazaEnanaAnim");
                obj.WeaponRazaEnanaAnim = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                break;

            case eOBJType::otInstrumentos:
                valStr = reader.GetValue(sec, "SND1");
                obj.Snd1 = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "SND2");
                obj.Snd2 = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "SND3");
                obj.Snd3 = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Real");
                obj.Real = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Caos");
                obj.Caos = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                break;

            case eOBJType::otMinerales:
                valStr = reader.GetValue(sec, "MinSkill");
                obj.MinSkill = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                break;

            case eOBJType::otPuertas:
            case eOBJType::otBotellaVacia:
            case eOBJType::otBotellaLlena:
                valStr = reader.GetValue(sec, "IndexAbierta");
                obj.IndexAbierta = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "IndexCerrada");
                obj.IndexCerrada = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "IndexCerradaLlave");
                obj.IndexCerradaLlave = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                break;

            case eOBJType::otPociones:
                valStr = reader.GetValue(sec, "TipoPocion");
                obj.TipoPocion = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "MaxModificador");
                obj.MaxModificador = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "MinModificador");
                obj.MinModificador = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "DuracionEfecto");
                obj.DuracionEfecto = valStr.empty() ? 0 : static_cast<int32_t>(std::stoi(valStr));
                break;

            case eOBJType::otBarcos:
                valStr = reader.GetValue(sec, "MinSkill");
                obj.MinSkill = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "MaxHIT");
                obj.MaxHIT = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "MinHIT");
                obj.MinHIT = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                break;

            case eOBJType::otFlechas:
                valStr = reader.GetValue(sec, "MaxHIT");
                obj.MaxHIT = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "MinHIT");
                obj.MinHIT = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Envenena");
                obj.Envenena = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "Paraliza");
                obj.Paraliza = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
                break;

            case eOBJType::otAnillo:
                valStr = reader.GetValue(sec, "LingH");
                obj.LingH = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingP");
                obj.LingP = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "LingO");
                obj.LingO = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "SkHerreria");
                obj.SkHerreria = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "MaxHIT");
                obj.MaxHIT = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                valStr = reader.GetValue(sec, "MinHIT");
                obj.MinHIT = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
                break;

            case eOBJType::otTeleport:
                valStr = reader.GetValue(sec, "Radio");
                obj.Radio = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
                break;

            case eOBJType::otMochilas:
                valStr = reader.GetValue(sec, "MochilaType");
                obj.MochilaType = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
                break;

            case eOBJType::otForos:
                break;

            default:
                break;
        }

        valStr = reader.GetValue(sec, "NumRopaje");
        obj.Ropaje = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "HechizoIndex");
        obj.HechizoIndex = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "LingoteIndex");
        obj.LingoteIndex = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MineralIndex");
        obj.MineralIndex = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MaxHP");
        obj.MaxHp = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MinHP");
        obj.MinHp = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Mujer");
        obj.Mujer = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Hombre");
        obj.Hombre = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MinHam");
        obj.MinHam = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MinAgu");
        obj.MinSed = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MINDEF");
        obj.MinDef = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "MAXDEF");
        obj.MaxDef = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        obj.def = (obj.MinDef + obj.MaxDef) / 2.0f;

        valStr = reader.GetValue(sec, "RazaEnana");
        obj.RazaEnana = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "RazaDrow");
        obj.RazaDrow = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "RazaElfa");
        obj.RazaElfa = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "RazaGnoma");
        obj.RazaGnoma = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "RazaHumana");
        obj.RazaHumana = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Valor");
        obj.Valor = valStr.empty() ? 0 : static_cast<int32_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Crucial");
        obj.Crucial = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "abierta");
        obj.Cerrada = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
        if (obj.Cerrada == 1) {
            valStr = reader.GetValue(sec, "Llave");
            obj.Llave = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
            valStr = reader.GetValue(sec, "Clave");
            obj.clave = valStr.empty() ? 0 : static_cast<int32_t>(std::stoi(valStr));
        }

        valStr = reader.GetValue(sec, "Clave");
        if (!valStr.empty()) {
            obj.clave = static_cast<int32_t>(std::stoi(valStr));
        }

        obj.texto = reader.GetValue(sec, "Texto");

        valStr = reader.GetValue(sec, "VGrande");
        obj.GrhSecundario = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Agarrable");
        obj.Agarrable = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        obj.ForoID = reader.GetValue(sec, "ID");

        valStr = reader.GetValue(sec, "Acuchilla");
        obj.Acuchilla = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Guante");
        obj.Guante = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        for (int i = 1; i <= NUMCLASES; ++i) {
            std::string cpStr = reader.GetValue(sec, "CP" + std::to_string(i));
            if (cpStr.empty()) {
                obj.ClaseProhibida[i] = static_cast<eClass>(0);
            } else {
                std::string cpUpper = cpStr;
                for (auto& c : cpUpper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                int found = 0;
                for (int n = 1; n <= NUMCLASES; ++n) {
                    std::string claseName = (n >= 1 && n <= NUMCLASES) ? ListaClases[n] : "";
                    for (auto& c : claseName) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
                    if (!claseName.empty() && claseName == cpUpper) {
                        found = n;
                        break;
                    }
                }
                obj.ClaseProhibida[i] = static_cast<eClass>(found);
            }
        }

        valStr = reader.GetValue(sec, "DefensaMagicaMax");
        obj.DefensaMagicaMax = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "DefensaMagicaMin");
        obj.DefensaMagicaMin = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "SkCarpinteria");
        obj.SkCarpinteria = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        if (obj.SkCarpinteria > 0) {
            valStr = reader.GetValue(sec, "Madera");
            obj.Madera = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

            valStr = reader.GetValue(sec, "MaderaElfica");
            obj.MaderaElfica = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        }

        valStr = reader.GetValue(sec, "MinST");
        obj.MinSta = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "NoSeCae");
        obj.NoSeCae = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

        valStr = reader.GetValue(sec, "Upgrade");
        obj.Upgrade = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    }
}

void CargaApuestas() {
    std::string file = DatPath.empty() ? "Dat/apuestas.dat" : (DatPath + "apuestas.dat");
    std::string valStr = GetVar(file, "Main", "Ganancias");
    Apuestas.Ganancias = valStr.empty() ? 0 : std::stol(valStr);
    valStr = GetVar(file, "Main", "Perdidas");
    Apuestas.Perdidas = valStr.empty() ? 0 : std::stol(valStr);
    valStr = GetVar(file, "Main", "Jugadas");
    Apuestas.Jugadas = valStr.empty() ? 0 : std::stol(valStr);
}

void LoadArmadurasFaccion() {
    std::string file = DatPath.empty() ? "Dat/ArmadurasFaccionarias.dat" : (DatPath + "ArmadurasFaccionarias.dat");
    for (int classIndex = 1; classIndex <= NUMCLASES; ++classIndex) {
        std::string sec = "CLASE" + std::to_string(classIndex);

        std::string valStr = GetVar(file, sec, "DefMinArmyAlto");
        int16_t idx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Drow)].Armada[static_cast<size_t>(eTipoDefArmors::ieBaja)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Elfo)].Armada[static_cast<size_t>(eTipoDefArmors::ieBaja)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Humano)].Armada[static_cast<size_t>(eTipoDefArmors::ieBaja)] = idx;

        valStr = GetVar(file, sec, "DefMinArmyBajo");
        idx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Enano)].Armada[static_cast<size_t>(eTipoDefArmors::ieBaja)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Gnomo)].Armada[static_cast<size_t>(eTipoDefArmors::ieBaja)] = idx;

        valStr = GetVar(file, sec, "DefMinCaosAlto");
        idx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Drow)].Caos[static_cast<size_t>(eTipoDefArmors::ieBaja)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Elfo)].Caos[static_cast<size_t>(eTipoDefArmors::ieBaja)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Humano)].Caos[static_cast<size_t>(eTipoDefArmors::ieBaja)] = idx;

        valStr = GetVar(file, sec, "DefMinCaosBajo");
        idx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Enano)].Caos[static_cast<size_t>(eTipoDefArmors::ieBaja)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Gnomo)].Caos[static_cast<size_t>(eTipoDefArmors::ieBaja)] = idx;

        valStr = GetVar(file, sec, "DefMedArmyAlto");
        idx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Drow)].Armada[static_cast<size_t>(eTipoDefArmors::ieMedia)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Elfo)].Armada[static_cast<size_t>(eTipoDefArmors::ieMedia)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Humano)].Armada[static_cast<size_t>(eTipoDefArmors::ieMedia)] = idx;

        valStr = GetVar(file, sec, "DefMedArmyBajo");
        idx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Enano)].Armada[static_cast<size_t>(eTipoDefArmors::ieMedia)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Gnomo)].Armada[static_cast<size_t>(eTipoDefArmors::ieMedia)] = idx;

        valStr = GetVar(file, sec, "DefMedCaosAlto");
        idx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Drow)].Caos[static_cast<size_t>(eTipoDefArmors::ieMedia)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Elfo)].Caos[static_cast<size_t>(eTipoDefArmors::ieMedia)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Humano)].Caos[static_cast<size_t>(eTipoDefArmors::ieMedia)] = idx;

        valStr = GetVar(file, sec, "DefMedCaosBajo");
        idx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Enano)].Caos[static_cast<size_t>(eTipoDefArmors::ieMedia)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Gnomo)].Caos[static_cast<size_t>(eTipoDefArmors::ieMedia)] = idx;

        valStr = GetVar(file, sec, "DefAltaArmyAlto");
        idx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Drow)].Armada[static_cast<size_t>(eTipoDefArmors::ieAlta)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Elfo)].Armada[static_cast<size_t>(eTipoDefArmors::ieAlta)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Humano)].Armada[static_cast<size_t>(eTipoDefArmors::ieAlta)] = idx;

        valStr = GetVar(file, sec, "DefAltaArmyBajo");
        idx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Enano)].Armada[static_cast<size_t>(eTipoDefArmors::ieAlta)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Gnomo)].Armada[static_cast<size_t>(eTipoDefArmors::ieAlta)] = idx;

        valStr = GetVar(file, sec, "DefAltaCaosAlto");
        idx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Drow)].Caos[static_cast<size_t>(eTipoDefArmors::ieAlta)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Elfo)].Caos[static_cast<size_t>(eTipoDefArmors::ieAlta)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Humano)].Caos[static_cast<size_t>(eTipoDefArmors::ieAlta)] = idx;

        valStr = GetVar(file, sec, "DefAltaCaosBajo");
        idx = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Enano)].Caos[static_cast<size_t>(eTipoDefArmors::ieAlta)] = idx;
        ArmadurasFaccion[classIndex][static_cast<size_t>(eRaza::Gnomo)].Caos[static_cast<size_t>(eTipoDefArmors::ieAlta)] = idx;
    }
}

// ============================================================================
// GRUPO 7: Logging Administrativo y Sanciones (Admin Logging & Bans)
// ============================================================================

std::string LogsPath = "";

static std::string getLogFile(const std::string& filename) {
    std::string base = LogsPath.empty() ? "logs/" : LogsPath;
    if (!base.empty() && base.back() != '/' && base.back() != '\\') {
        base += "/";
    }
    return base + filename;
}

static void appendLogLine(const std::string& filepath, const std::string& line) {
    std::filesystem::path p(filepath);
    if (p.has_parent_path()) {
        std::error_code ec;
        std::filesystem::create_directories(p.parent_path(), ec);
    }
    std::ofstream ofs(filepath, std::ios::app | std::ios::binary);
    if (ofs.is_open()) {
        ofs << line << "\r\n";
    }
}

void LogBan(int16_t bannedIndex, int16_t userIndex, const std::string& motivo) {
    if (bannedIndex <= 0 || static_cast<size_t>(bannedIndex) >= UserList.size()) return;
    if (userIndex <= 0 || static_cast<size_t>(userIndex) >= UserList.size()) return;

    std::string bannedName = UserList[bannedIndex].name;
    std::string adminName = UserList[userIndex].name;

    std::string banDetailFile = getLogFile("BanDetail.log");
    WriteVar(banDetailFile, bannedName, "BannedBy", adminName);
    WriteVar(banDetailFile, bannedName, "Reason", motivo);

    std::string genteBannedFile = getLogFile("GenteBanned.log");
    appendLogLine(genteBannedFile, bannedName);
}

void LogBanFromName(const std::string& bannedName, int16_t userIndex, const std::string& motivo) {
    if (bannedName.empty()) return;
    if (userIndex <= 0 || static_cast<size_t>(userIndex) >= UserList.size()) return;

    std::string adminName = UserList[userIndex].name;

    // Quirk legacy: LogBanFromName persiste en BanDetail.dat (a diferencia de LogBan que usa BanDetail.log)
    std::string banDetailFile = getLogFile("BanDetail.dat");
    WriteVar(banDetailFile, bannedName, "BannedBy", adminName);
    WriteVar(banDetailFile, bannedName, "Reason", motivo);

    std::string genteBannedFile = getLogFile("GenteBanned.log");
    appendLogLine(genteBannedFile, bannedName);
}

void Ban(const std::string& bannedName, const std::string& baneador, const std::string& motivo) {
    if (bannedName.empty()) return;

    std::string banDetailFile = getLogFile("BanDetail.dat");
    WriteVar(banDetailFile, bannedName, "BannedBy", baneador);
    WriteVar(banDetailFile, bannedName, "Reason", motivo);

    std::string genteBannedFile = getLogFile("GenteBanned.log");
    appendLogLine(genteBannedFile, bannedName);
}

// ============================================================================
// GRUPO 6: Sistema de Respaldos de Mundo (World Backup System)
// ============================================================================

std::string WorldBackupPath = "";

static std::string getWorldBackupFile(const std::string& filename) {
    std::string base = WorldBackupPath.empty() ? "WorldBackUp/" : WorldBackupPath;
    if (!base.empty() && base.back() != '/' && base.back() != '\\') {
        base += "/";
    }
    return base + filename;
}

void BackUPnPc(int16_t npcIndex) {
    if (npcIndex <= 0 || npcIndex > MAXNPCS) return;

    std::string file = DatPath.empty() ? "Dat/bkNPCs.dat" : (DatPath + "bkNPCs.dat");
    const auto& n = Npclist[npcIndex];
    int npcNumero = n.Numero;
    std::string sec = "NPC" + std::to_string(npcNumero);

    WriteVar(file, sec, "Name", n.name);
    WriteVar(file, sec, "Desc", n.desc);
    WriteVar(file, sec, "Head", std::to_string(n.char_appearance.Head));
    WriteVar(file, sec, "Body", std::to_string(n.char_appearance.body));
    WriteVar(file, sec, "Heading", std::to_string(n.char_appearance.heading));
    WriteVar(file, sec, "Movement", std::to_string(static_cast<int>(n.Movement)));
    WriteVar(file, sec, "Attackable", std::to_string(n.Attackable));
    WriteVar(file, sec, "Comercia", std::to_string(n.Comercia));
    WriteVar(file, sec, "TipoItems", std::to_string(n.TipoItems));
    WriteVar(file, sec, "Hostil", std::to_string(n.Hostile));
    WriteVar(file, sec, "GiveEXP", std::to_string(n.GiveEXP));
    WriteVar(file, sec, "GiveGLD", std::to_string(n.GiveGLD));
    WriteVar(file, sec, "InvReSpawn", std::to_string(n.InvReSpawn));
    WriteVar(file, sec, "NpcType", std::to_string(static_cast<int>(n.NPCtype)));

    WriteVar(file, sec, "Alineacion", std::to_string(n.Stats.Alineacion));
    WriteVar(file, sec, "DEF", std::to_string(n.Stats.def));
    WriteVar(file, sec, "MaxHit", std::to_string(n.Stats.MaxHIT));
    WriteVar(file, sec, "MaxHp", std::to_string(n.Stats.MaxHp));
    WriteVar(file, sec, "MinHit", std::to_string(n.Stats.MinHIT));
    WriteVar(file, sec, "MinHp", std::to_string(n.Stats.MinHp));

    WriteVar(file, sec, "ReSpawn", std::to_string(n.flags.Respawn));
    WriteVar(file, sec, "BackUp", std::to_string(n.flags.BackUp));
    WriteVar(file, sec, "Domable", std::to_string(n.flags.Domable));

    WriteVar(file, sec, "NroItems", std::to_string(n.Invent.NroItems));
    if (n.Invent.NroItems > 0) {
        for (int loopC = 1; loopC <= MAX_INVENTORY_SLOTS; ++loopC) {
            std::string itemVal = std::to_string(n.Invent.Object[loopC].ObjIndex) + "-" + std::to_string(n.Invent.Object[loopC].Amount);
            WriteVar(file, sec, "Obj" + std::to_string(loopC), itemVal);
        }
    }
}

void CargarNpcBackUp(int16_t npcIndex, int16_t npcNumber) {
    if (npcIndex <= 0 || npcIndex > MAXNPCS) return;

    std::string file = DatPath.empty() ? "Dat/bkNPCs.dat" : (DatPath + "bkNPCs.dat");
    std::string sec = "NPC" + std::to_string(npcNumber);
    auto& n = Npclist[npcIndex];

    n.Numero = npcNumber;
    n.name = GetVar(file, sec, "Name");
    n.desc = GetVar(file, sec, "Desc");

    std::string valStr;
    valStr = GetVar(file, sec, "Movement");
    n.Movement = valStr.empty() ? TipoAI::StaticNPC : static_cast<TipoAI>(std::stoi(valStr));

    valStr = GetVar(file, sec, "NpcType");
    n.NPCtype = valStr.empty() ? eNPCType::Comun : static_cast<eNPCType>(std::stoi(valStr));

    valStr = GetVar(file, sec, "Body");
    n.char_appearance.body = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(file, sec, "Head");
    n.char_appearance.Head = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(file, sec, "Heading");
    n.char_appearance.heading = valStr.empty() ? eHeading::NORTH : static_cast<eHeading>(std::stoi(valStr));

    valStr = GetVar(file, sec, "Attackable");
    n.Attackable = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
    valStr = GetVar(file, sec, "Comercia");
    n.Comercia = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(file, sec, "Hostile");
    if (valStr.empty()) valStr = GetVar(file, sec, "Hostil");
    n.Hostile = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = GetVar(file, sec, "GiveEXP");
    n.GiveEXP = valStr.empty() ? 0 : std::stol(valStr);
    valStr = GetVar(file, sec, "GiveGLD");
    n.GiveGLD = valStr.empty() ? 0 : std::stol(valStr);
    valStr = GetVar(file, sec, "InvReSpawn");
    n.InvReSpawn = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = GetVar(file, sec, "MaxHP");
    n.Stats.MaxHp = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(file, sec, "MinHP");
    n.Stats.MinHp = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(file, sec, "MaxHIT");
    n.Stats.MaxHIT = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(file, sec, "MinHIT");
    n.Stats.MinHIT = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(file, sec, "DEF");
    n.Stats.def = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(file, sec, "Alineacion");
    n.Stats.Alineacion = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    valStr = GetVar(file, sec, "NROITEMS");
    if (valStr.empty()) valStr = GetVar(file, sec, "NroItems");
    n.Invent.NroItems = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));

    if (n.Invent.NroItems > 0) {
        for (int loopC = 1; loopC <= MAX_INVENTORY_SLOTS; ++loopC) {
            std::string ln = GetVar(file, sec, "Obj" + std::to_string(loopC));
            if (!ln.empty()) {
                n.Invent.Object[loopC].ObjIndex = static_cast<int16_t>(std::atoi(ReadField(1, ln, 45).c_str()));
                n.Invent.Object[loopC].Amount = static_cast<int16_t>(std::atoi(ReadField(2, ln, 45).c_str()));
            } else {
                n.Invent.Object[loopC].ObjIndex = 0;
                n.Invent.Object[loopC].Amount = 0;
            }
        }
    } else {
        for (int loopC = 1; loopC <= MAX_INVENTORY_SLOTS; ++loopC) {
            n.Invent.Object[loopC].ObjIndex = 0;
            n.Invent.Object[loopC].Amount = 0;
        }
    }

    for (int loopC = 1; loopC <= MAX_NPC_DROPS; ++loopC) {
        std::string ln = GetVar(file, sec, "Drop" + std::to_string(loopC));
        if (!ln.empty()) {
            n.Drop[loopC].ObjIndex = static_cast<int16_t>(std::atoi(ReadField(1, ln, 45).c_str()));
            n.Drop[loopC].Amount = static_cast<int32_t>(std::atoi(ReadField(2, ln, 45).c_str()));
        } else {
            n.Drop[loopC].ObjIndex = 0;
            n.Drop[loopC].Amount = 0;
        }
    }

    n.flags.NPCActive = true;
    valStr = GetVar(file, sec, "ReSpawn");
    n.flags.Respawn = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
    valStr = GetVar(file, sec, "BackUp");
    n.flags.BackUp = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));
    valStr = GetVar(file, sec, "Domable");
    n.flags.Domable = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
    valStr = GetVar(file, sec, "OrigPos");
    n.flags.RespawnOrigPos = valStr.empty() ? 0 : static_cast<uint8_t>(std::stoi(valStr));

    valStr = GetVar(file, sec, "TipoItems");
    n.TipoItems = valStr.empty() ? 0 : static_cast<int16_t>(std::stoi(valStr));
}

void CargarBackUp() {
    std::string mapDatFile = DatPath.empty() ? "Dat/Map.dat" : (DatPath + "Map.dat");
    std::string numMapsStr = GetVar(mapDatFile, "INIT", "NumMaps");
    if (numMapsStr.empty()) return;

    NumMaps = static_cast<int16_t>(std::stoi(numMapsStr));
    std::string mapPathCfg = GetVar(mapDatFile, "INIT", "MapPath");
    if (!mapPathCfg.empty()) {
        MapPath = mapPathCfg;
    }
    if (!MapPath.empty() && MapPath.back() != '/' && MapPath.back() != '\\') {
        MapPath += "/";
    }

    if (MapInfoList.size() <= static_cast<size_t>(NumMaps)) {
        MapInfoList.resize(static_cast<size_t>(NumMaps) + 1);
    }
    size_t reqSize = (static_cast<size_t>(NumMaps) + 1) * 101 * 101;
    if (MapData.size() < reqSize) {
        MapData.resize(reqSize);
    }

    for (int map = 1; map <= NumMaps; ++map) {
        std::string mapDat = MapPath + "Mapa" + std::to_string(map) + ".dat";
        std::string backupVal = GetVar(mapDat, "Mapa" + std::to_string(map), "BackUp");
        int backupFlag = backupVal.empty() ? 0 : std::stoi(backupVal);

        std::string tFileName;
        if (backupFlag != 0) {
            tFileName = getWorldBackupFile("Mapa" + std::to_string(map));
            // Si no existe el .map en WorldBackup, fallback a la carpeta base
            if (!std::filesystem::exists(tFileName + ".map") && !std::filesystem::exists(tFileName + ".Map")) {
                tFileName = MapPath + "Mapa" + std::to_string(map);
            }
        } else {
            tFileName = MapPath + "Mapa" + std::to_string(map);
        }

        CargarMapa(map, tFileName);
    }
}

void DoBackUp() {
    haciendoBK = true;

    // 1. Persistir mapas que tengan flag BackUp = 1
    std::string bkDir = WorldBackupPath.empty() ? "WorldBackUp/" : WorldBackupPath;
    if (!bkDir.empty() && bkDir.back() != '/' && bkDir.back() != '\\') {
        bkDir += "/";
    }
    std::error_code ec;
    std::filesystem::create_directories(bkDir, ec);

    for (int map = 1; map <= NumMaps; ++map) {
        if (static_cast<size_t>(map) < MapInfoList.size() && MapInfoList[map].BackUp == 1) {
            std::string outPrefix = bkDir + "Mapa" + std::to_string(map);
            GrabarMapa(map, outPrefix);
        }
    }

    // 2. Persistir NPCs con flag BackUp = 1
    std::string npcDat = DatPath.empty() ? "Dat/bkNPCs.dat" : (DatPath + "bkNPCs.dat");
    std::filesystem::remove(npcDat, ec);

    for (int loopX = 1; loopX <= LastNPC && loopX <= MAXNPCS; ++loopX) {
        if (Npclist[loopX].flags.BackUp == 1) {
            BackUPnPc(static_cast<int16_t>(loopX));
        }
    }

    // 3. Registrar fecha y hora en logs/BackUps.log
    auto now = std::chrono::system_clock::now();
    std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
    std::tm tmBuf{};
#if defined(_WIN32)
    localtime_s(&tmBuf, &nowTime);
#else
    localtime_r(&nowTime, &tmBuf);
#endif
    char timeStr[64];
    std::strftime(timeStr, sizeof(timeStr), "%d/%m/%Y %H:%M:%S", &tmBuf);
    LastBackup = timeStr;

    std::string bkLog = getLogFile("BackUps.log");
    appendLogLine(bkLog, LastBackup);

    haciendoBK = false;
}

} // namespace FileIO
