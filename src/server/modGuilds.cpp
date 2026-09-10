#include "modGuilds.hpp"
#include "clsClan.hpp"
#include "FileIO.hpp"
#include <array>
#include <algorithm>
#include <cctype>
#include <filesystem>

using namespace FileIO;

namespace modGuilds {

std::int16_t CANTIDADDECLANES = 0;

static std::string s_guildsPath = "guilds/";
static std::array<std::unique_ptr<clsClan>, MAX_GUILDS + 1> s_guilds{};
static SendDataCallback s_sendDataHook = nullptr;

void SetGuildsPath(const std::string& path) {
    s_guildsPath = path;
    if (!s_guildsPath.empty() && s_guildsPath.back() != '/' && s_guildsPath.back() != '\\') {
        s_guildsPath += "/";
    }
}

std::string GetGuildsPath() {
    return s_guildsPath;
}

void SetSendDataHook(SendDataCallback hook) {
    s_sendDataHook = std::move(hook);
}

static void DispatchSendData(SendTarget target, int16_t guildIndex, const std::string& msg) {
    if (s_sendDataHook) {
        s_sendDataHook(target, guildIndex, msg);
    }
}

// Helpers de casing
static std::string ToUpper(std::string s) {
    for (char& c : s) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return s;
}

static std::string Trim(std::string s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
    return s;
}

//
// GRUPO 1: Conversiones y validaciones
//

ALINEACION_GUILD String2Alineacion(const std::string& s) {
    if (s == "Neutral") return ALINEACION_GUILD::ALINEACION_NEUTRO;
    if (s == "Del Mal") return ALINEACION_GUILD::ALINEACION_LEGION;
    if (s == "Real") return ALINEACION_GUILD::ALINEACION_ARMADA;
    if (s == "Game Masters") return ALINEACION_GUILD::ALINEACION_MASTER;
    if (s == "Legal") return ALINEACION_GUILD::ALINEACION_CIUDA;
    if (s == "Criminal") return ALINEACION_GUILD::ALINEACION_CRIMINAL;
    return ALINEACION_GUILD::ALINEACION_NINGUNA;
}

std::string Alineacion2String(ALINEACION_GUILD alineacion) {
    switch (alineacion) {
        case ALINEACION_GUILD::ALINEACION_NEUTRO: return "Neutral";
        case ALINEACION_GUILD::ALINEACION_LEGION: return "Del Mal";
        case ALINEACION_GUILD::ALINEACION_ARMADA: return "Real";
        case ALINEACION_GUILD::ALINEACION_MASTER: return "Game Masters";
        case ALINEACION_GUILD::ALINEACION_CIUDA: return "Legal";
        case ALINEACION_GUILD::ALINEACION_CRIMINAL: return "Criminal";
        default: return "";
    }
}

std::string Relacion2String(RELACIONES_GUILD relacion) {
    switch (relacion) {
        case RELACIONES_GUILD::ALIADOS: return "A";
        case RELACIONES_GUILD::GUERRA: return "G";
        case RELACIONES_GUILD::PAZ: return "P";
        default: return "P";
    }
}

RELACIONES_GUILD String2Relacion(const std::string& s) {
    std::string clean = ToUpper(Trim(s));
    if (clean.empty() || clean == "P") return RELACIONES_GUILD::PAZ;
    if (clean == "G") return RELACIONES_GUILD::GUERRA;
    if (clean == "A") return RELACIONES_GUILD::ALIADOS;
    return RELACIONES_GUILD::PAZ;
}

bool GuildNameValido(const std::string& cad) {
    for (char ch : cad) {
        unsigned char c = static_cast<unsigned char>(std::tolower(static_cast<unsigned char>(ch)));
        if ((c < 'a' || c > 'z') && (c != 255) && (c != ' ')) {
            return false;
        }
    }
    return true;
}

//
// GRUPO 7: Carga global y ciclo de vida
//

void ResetGuildsDB() {
    for (int i = 1; i <= MAX_GUILDS; ++i) {
        s_guilds[i].reset();
    }
    CANTIDADDECLANES = 0;
}

void LoadGuildsDB() {
    ResetGuildsDB();
    std::string guildInfoFile = GetGuildsPath() + "guildsinfo.inf";
    std::string cantClanes = GetVar(guildInfoFile, "INIT", "nroGuilds");
    try {
        if (!cantClanes.empty()) {
            CANTIDADDECLANES = static_cast<int16_t>(std::stoi(cantClanes));
        }
    } catch (...) {
        CANTIDADDECLANES = 0;
    }

    if (CANTIDADDECLANES > MAX_GUILDS) CANTIDADDECLANES = MAX_GUILDS;

    for (int i = 1; i <= CANTIDADDECLANES; ++i) {
        s_guilds[i] = std::make_unique<clsClan>();
        std::string name = GetVar(guildInfoFile, "GUILD" + std::to_string(i), "GUILDNAME");
        std::string alinStr = GetVar(guildInfoFile, "GUILD" + std::to_string(i), "Alineacion");
        s_guilds[i]->Inicializar(name, static_cast<int16_t>(i), String2Alineacion(alinStr));
    }
}

//
// Consultas globales
//

std::int16_t GuildIndex(const std::string& guildName) {
    std::string target = ToUpper(Trim(guildName));
    for (int i = 1; i <= CANTIDADDECLANES; ++i) {
        if (s_guilds[i] && ToUpper(s_guilds[i]->GuildName()) == target) {
            return static_cast<int16_t>(i);
        }
    }
    return 0;
}

std::string GuildName(std::int16_t guildIndex) {
    if (guildIndex > 0 && guildIndex <= CANTIDADDECLANES && s_guilds[guildIndex]) {
        return s_guilds[guildIndex]->GuildName();
    }
    return "";
}

std::string GuildLeader(std::int16_t guildIndex) {
    if (guildIndex > 0 && guildIndex <= CANTIDADDECLANES && s_guilds[guildIndex]) {
        return s_guilds[guildIndex]->GetLeader();
    }
    return "";
}

std::string GuildAlignment(std::int16_t guildIndex) {
    if (guildIndex > 0 && guildIndex <= CANTIDADDECLANES && s_guilds[guildIndex]) {
        return Alineacion2String(s_guilds[guildIndex]->Alineacion());
    }
    return "";
}

std::string GuildFounder(std::int16_t guildIndex) {
    if (guildIndex > 0 && guildIndex <= CANTIDADDECLANES && s_guilds[guildIndex]) {
        return s_guilds[guildIndex]->Fundador();
    }
    return "";
}

bool YaExiste(const std::string& guildName) {
    return (GuildIndex(guildName) != 0);
}

bool HasFound(const std::string& userName) {
    std::string target = ToUpper(Trim(userName));
    for (int i = 1; i <= CANTIDADDECLANES; ++i) {
        if (s_guilds[i] && ToUpper(s_guilds[i]->Fundador()) == target) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> PrepareGuildsList() {
    std::vector<std::string> res;
    if (CANTIDADDECLANES <= 0) return res;
    res.reserve(CANTIDADDECLANES);
    for (int i = 1; i <= CANTIDADDECLANES; ++i) {
        if (s_guilds[i]) {
            res.push_back(s_guilds[i]->GuildName());
        }
    }
    return res;
}

//
// Fundación de clanes
//

bool PuedeFundarUnClan(std::int16_t userIndex, ALINEACION_GUILD alineacion, std::string& refError) {
    if (userIndex <= 0 || static_cast<size_t>(userIndex) >= UserList.size()) {
        refError = "Índice de usuario inválido.";
        return false;
    }

    const auto& user = UserList[userIndex];
    if (user.GuildIndex > 0) {
        refError = "Ya perteneces a un clan, no puedes fundar otro";
        return false;
    }

    if (user.Stats.ELV < 25 || user.Stats.UserSkills[eSkill::Liderazgo] < 90) {
        refError = "Para fundar un clan debes ser nivel 25 y tener 90 skills en liderazgo.";
        return false;
    }

    switch (alineacion) {
        case ALINEACION_GUILD::ALINEACION_ARMADA:
            if (user.Faccion.ArmadaReal != 1) {
                refError = "Para fundar un clan real debes ser miembro del ejército real.";
                return false;
            }
            break;
        case ALINEACION_GUILD::ALINEACION_CIUDA:
            if (criminal(userIndex)) {
                refError = "Para fundar un clan de ciudadanos no debes ser criminal.";
                return false;
            }
            break;
        case ALINEACION_GUILD::ALINEACION_CRIMINAL:
            if (!criminal(userIndex)) {
                refError = "Para fundar un clan de criminales no debes ser ciudadano.";
                return false;
            }
            break;
        case ALINEACION_GUILD::ALINEACION_LEGION:
            if (user.Faccion.FuerzasCaos != 1) {
                refError = "Para fundar un clan del mal debes pertenecer a la legión oscura.";
                return false;
            }
            break;
        case ALINEACION_GUILD::ALINEACION_MASTER:
            if (user.flags.Privilegios & (PlayerType::UserPlayer | PlayerType::Consejero | PlayerType::SemiDios)) {
                refError = "Para fundar un clan sin alineación debes ser un dios.";
                return false;
            }
            break;
        case ALINEACION_GUILD::ALINEACION_NEUTRO:
            if (user.Faccion.ArmadaReal != 0 || user.Faccion.FuerzasCaos != 0) {
                refError = "Para fundar un clan neutro no debes pertenecer a ninguna facción.";
                return false;
            }
            break;
        default:
            break;
    }

    return true;
}

bool CrearNuevoClan(std::int16_t fundadorIndex, const std::string& desc, const std::string& guildName,
                   const std::string& url, const std::vector<std::string>& codex,
                   ALINEACION_GUILD alineacion, std::string& refError) {
    if (!PuedeFundarUnClan(fundadorIndex, alineacion, refError)) {
        return false;
    }

    if (guildName.empty() || !GuildNameValido(guildName)) {
        refError = "Nombre de clan inválido.";
        return false;
    }

    if (YaExiste(guildName)) {
        refError = "Ya existe un clan con ese nombre.";
        return false;
    }

    if (CANTIDADDECLANES < MAX_GUILDS) {
        ++CANTIDADDECLANES;
        s_guilds[CANTIDADDECLANES] = std::make_unique<clsClan>();
        auto& g = s_guilds[CANTIDADDECLANES];
        g->Inicializar(guildName, CANTIDADDECLANES, alineacion);
        
        std::string fName = UserList[fundadorIndex].name;
        g->InicializarNuevoClan(fName);

        for (size_t i = 0; i < codex.size() && i < CANTIDADMAXIMACODEX; ++i) {
            g->SetCodex(static_cast<int16_t>(i + 1), codex[i]);
        }
        g->SetDesc(desc);
        g->SetGuildNews("Clan creado con alineación: " + Alineacion2String(alineacion));
        g->SetLeader(fName);
        g->SetURL(url);

        g->AceptarNuevoMiembro(fName);
        g->ConectarMiembro(fundadorIndex);

        UserList[fundadorIndex].GuildIndex = CANTIDADDECLANES;

        for (int i = 1; i < CANTIDADDECLANES; ++i) {
            if (s_guilds[i]) {
                s_guilds[i]->ProcesarFundacionDeOtroClan();
            }
        }
        return true;
    } else {
        refError = "No hay más slots para fundar clanes. Consulte a un administrador.";
        return false;
    }
}

//
// Membresías y reglas faccionarias
//

bool m_EstadoPermiteEntrar(std::int16_t userIndex, std::int16_t guildIndex) {
    if (guildIndex <= 0 || guildIndex > CANTIDADDECLANES || !s_guilds[guildIndex]) return false;
    if (userIndex <= 0 || static_cast<size_t>(userIndex) >= UserList.size()) return false;

    const auto& user = UserList[userIndex];
    switch (s_guilds[guildIndex]->Alineacion()) {
        case ALINEACION_GUILD::ALINEACION_ARMADA:
            return (user.Faccion.ArmadaReal == 1);
        case ALINEACION_GUILD::ALINEACION_CIUDA:
            return !criminal(userIndex);
        case ALINEACION_GUILD::ALINEACION_CRIMINAL:
            return criminal(userIndex);
        case ALINEACION_GUILD::ALINEACION_LEGION:
            return (user.Faccion.FuerzasCaos == 1);
        case ALINEACION_GUILD::ALINEACION_MASTER:
            return !(user.flags.Privilegios & (PlayerType::UserPlayer | PlayerType::Consejero | PlayerType::SemiDios));
        case ALINEACION_GUILD::ALINEACION_NEUTRO:
            return (user.Faccion.ArmadaReal == 0 && user.Faccion.FuerzasCaos == 0);
        default:
            return true;
    }
}

bool m_EstadoPermiteEntrarChar(const std::string& personaje, std::int16_t guildIndex) {
    if (guildIndex <= 0 || guildIndex > CANTIDADDECLANES || !s_guilds[guildIndex]) return false;
    std::string ruta = CharPath.empty() ? ("charfile/" + personaje + ".chr") : (CharPath + personaje + ".chr");
    if (!std::filesystem::exists(std::filesystem::u8path(ruta))) return false;

    std::string esCrimStr = GetVar(ruta, "COUNTERS", "Pena");
    std::string ejercitoReal = GetVar(ruta, "FACCIONES", "EjercitoReal");
    std::string ejercitoCaos = GetVar(ruta, "FACCIONES", "EjercitoCaos");

    switch (s_guilds[guildIndex]->Alineacion()) {
        case ALINEACION_GUILD::ALINEACION_ARMADA:
            return (ejercitoReal == "1");
        case ALINEACION_GUILD::ALINEACION_CIUDA:
            return (esCrimStr != "1");
        case ALINEACION_GUILD::ALINEACION_CRIMINAL:
            return (esCrimStr == "1");
        case ALINEACION_GUILD::ALINEACION_LEGION:
            return (ejercitoCaos == "1");
        case ALINEACION_GUILD::ALINEACION_NEUTRO:
            return (ejercitoReal == "0" && ejercitoCaos == "0");
        default:
            return true;
    }
}

bool m_ConectarMiembroAClan(std::int16_t userIndex, std::int16_t guildIndex) {
    if (guildIndex <= 0 || guildIndex > CANTIDADDECLANES || !s_guilds[guildIndex]) return false;
    if (m_EstadoPermiteEntrar(userIndex, guildIndex)) {
        s_guilds[guildIndex]->ConectarMiembro(userIndex);
        UserList[userIndex].GuildIndex = guildIndex;
        return true;
    }
    return false;
}

void m_DesconectarMiembroDelClan(std::int16_t userIndex, std::int16_t guildIndex) {
    if (guildIndex <= 0 || guildIndex > CANTIDADDECLANES || !s_guilds[guildIndex]) return;
    s_guilds[guildIndex]->DesConectarMiembro(userIndex);
}

ALINEACION_GUILD BajarGrado(std::int16_t guildIndex) {
    if (guildIndex <= 0 || guildIndex > CANTIDADDECLANES || !s_guilds[guildIndex]) {
        return ALINEACION_GUILD::ALINEACION_NEUTRO;
    }
    switch (s_guilds[guildIndex]->Alineacion()) {
        case ALINEACION_GUILD::ALINEACION_ARMADA: return ALINEACION_GUILD::ALINEACION_CIUDA;
        case ALINEACION_GUILD::ALINEACION_LEGION: return ALINEACION_GUILD::ALINEACION_CRIMINAL;
        case ALINEACION_GUILD::ALINEACION_CIUDA:
        case ALINEACION_GUILD::ALINEACION_CRIMINAL: return ALINEACION_GUILD::ALINEACION_NEUTRO;
        default: return ALINEACION_GUILD::ALINEACION_NEUTRO;
    }
}

void UpdateGuildMembers(std::int16_t guildIndex) {
    if (guildIndex <= 0 || guildIndex > CANTIDADDECLANES || !s_guilds[guildIndex]) return;
    auto& g = s_guilds[guildIndex];
    if (g->CambiarAlineacion(BajarGrado(guildIndex))) {
        // Se cambió la alineación
    }
    auto members = g->GetMemberList();
    for (const auto& mem : members) {
        if (!m_EstadoPermiteEntrarChar(mem, guildIndex)) {
            g->ExpulsarMiembro(mem);
        }
    }
    g->PuntosAntifaccion(0);
}

bool m_ValidarPermanencia(std::int16_t userIndex, bool sumaAntifaccion, bool& cambioAlineacion) {
    cambioAlineacion = false;
    if (userIndex <= 0 || static_cast<size_t>(userIndex) >= UserList.size()) return false;
    int16_t gi = UserList[userIndex].GuildIndex;
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi]) return false;

    if (!m_EstadoPermiteEntrar(userIndex, gi)) {
        if (GuildLeader(gi) == UserList[userIndex].name) {
            do {
                UpdateGuildMembers(gi);
            } while (!m_EstadoPermiteEntrar(userIndex, gi) && s_guilds[gi]->Alineacion() != ALINEACION_GUILD::ALINEACION_NEUTRO);
        } else {
            if (sumaAntifaccion) {
                int16_t pts = s_guilds[gi]->PuntosAntifaccion() + 1;
                s_guilds[gi]->PuntosAntifaccion(pts);
            }
            cambioAlineacion = (s_guilds[gi]->PuntosAntifaccion() >= MAXANTIFACCION);
            s_guilds[gi]->ExpulsarMiembro(UserList[userIndex].name);
            UserList[userIndex].GuildIndex = 0;
            if (cambioAlineacion) {
                UpdateGuildMembers(gi);
            }
        }
        return false;
    }
    return true;
}

std::int16_t m_EcharMiembroDeClan(std::int16_t expulsador, const std::string& expulsado) {
    int16_t gi = UserList[expulsador].GuildIndex;
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi]) return 0;
    if (ToUpper(GuildLeader(gi)) != ToUpper(UserList[expulsador].name)) return 0;

    s_guilds[gi]->ExpulsarMiembro(expulsado);
    return gi;
}

bool m_PuedeSalirDeClan(const std::string& nombre, std::int16_t guildIndex, std::int16_t) {
    if (guildIndex <= 0 || guildIndex > CANTIDADDECLANES || !s_guilds[guildIndex]) return false;
    return (ToUpper(s_guilds[guildIndex]->GetLeader()) != ToUpper(nombre));
}

void ActualizarWebSite(std::int16_t userIndex, const std::string& web) {
    int16_t gi = UserList[userIndex].GuildIndex;
    if (gi > 0 && gi <= CANTIDADDECLANES && s_guilds[gi]) {
        s_guilds[gi]->SetURL(web);
    }
}

void ChangeCodexAndDesc(const std::string& desc, const std::vector<std::string>& codex, std::int16_t guildIndex) {
    if (guildIndex > 0 && guildIndex <= CANTIDADDECLANES && s_guilds[guildIndex]) {
        s_guilds[guildIndex]->SetDesc(desc);
        for (size_t i = 0; i < codex.size() && i < CANTIDADMAXIMACODEX; ++i) {
            s_guilds[guildIndex]->SetCodex(static_cast<int16_t>(i + 1), codex[i]);
        }
    }
}

void ActualizarNoticias(std::int16_t userIndex, const std::string& datos) {
    int16_t gi = UserList[userIndex].GuildIndex;
    if (gi > 0 && gi <= CANTIDADDECLANES && s_guilds[gi]) {
        s_guilds[gi]->SetGuildNews(datos);
    }
}

//
// GRUPO 5: Elecciones
//

bool v_AbrirElecciones(std::int16_t userIndex, std::string& refError) {
    int16_t gi = UserList[userIndex].GuildIndex;
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi]) {
        refError = "No perteneces a ningún clan.";
        return false;
    }
    if (s_guilds[gi]->EleccionesAbiertas()) {
        refError = "Las elecciones ya están abiertas.";
        return false;
    }
    s_guilds[gi]->AbrirElecciones();
    return true;
}

bool v_UsuarioVota(std::int16_t userIndex, const std::string& votado, std::string& refError) {
    int16_t gi = UserList[userIndex].GuildIndex;
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi]) {
        refError = "No perteneces a ningún clan.";
        return false;
    }
    if (!s_guilds[gi]->EleccionesAbiertas()) {
        refError = "No hay elecciones abiertas en tu clan.";
        return false;
    }
    if (s_guilds[gi]->YaVoto(UserList[userIndex].name)) {
        refError = "Ya has votado en estas elecciones.";
        return false;
    }
    s_guilds[gi]->ContabilizarVoto(UserList[userIndex].name, votado);
    return true;
}

void v_RutinaElecciones() {
    for (int i = 1; i <= CANTIDADDECLANES; ++i) {
        if (s_guilds[i]) {
            if (s_guilds[i]->RevisarElecciones(false)) {
                DispatchSendData(SendTarget::ToAll, 0,
                    "Servidor> " + s_guilds[i]->GetLeader() + " es el nuevo líder de " + s_guilds[i]->GuildName() + ".");
            }
        }
    }
}

//
// GRUPO 4: Aspirantes
//

bool a_NuevoAspirante(std::int16_t userIndex, const std::string& clan, const std::string& solicitud, std::string& refError) {
    int16_t gi = GuildIndex(clan);
    if (gi <= 0 || !s_guilds[gi]) {
        refError = "El clan especificado no existe.";
        return false;
    }
    if (s_guilds[gi]->CantidadAspirantes() >= MAXASPIRANTES) {
        refError = "El clan no acepta más solicitudes en este momento.";
        return false;
    }
    s_guilds[gi]->NuevoAspirante(UserList[userIndex].name, solicitud);
    return true;
}

bool a_AceptarAspirante(std::int16_t userIndex, const std::string& aspirante, std::string& refError) {
    int16_t gi = UserList[userIndex].GuildIndex;
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi]) {
        refError = "No perteneces a ningún clan.";
        return false;
    }
    int16_t nroAsp = s_guilds[gi]->NumeroDeAspirante(aspirante);
    if (nroAsp <= 0) {
        refError = "El aspirante indicado no existe.";
        return false;
    }
    s_guilds[gi]->RetirarAspirante(aspirante, nroAsp);
    s_guilds[gi]->AceptarNuevoMiembro(aspirante);
    return true;
}

bool a_RechazarAspirante(std::int16_t userIndex, const std::string& nombre, std::string& refError) {
    int16_t gi = UserList[userIndex].GuildIndex;
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi]) {
        refError = "No perteneces a ningún clan.";
        return false;
    }
    int16_t nroAsp = s_guilds[gi]->NumeroDeAspirante(nombre);
    if (nroAsp <= 0) {
        refError = "El aspirante indicado no existe.";
        return false;
    }
    s_guilds[gi]->RetirarAspirante(nombre, nroAsp);
    return true;
}

void a_RechazarAspiranteChar(const std::string& aspirante, std::int16_t guild, const std::string& detalles) {
    if (guild > 0 && guild <= CANTIDADDECLANES && s_guilds[guild]) {
        s_guilds[guild]->InformarRechazoEnChar(aspirante, detalles);
    }
}

std::string a_ObtenerRechazoDeChar(const std::string& aspirante) {
    std::string ruta = CharPath.empty() ? ("charfile/" + aspirante + ".chr") : (CharPath + aspirante + ".chr");
    return GetVar(ruta, "GUILD", "MotivoRechazo");
}

std::string a_DetallesAspirante(std::int16_t userIndex, const std::string& nombre) {
    int16_t gi = UserList[userIndex].GuildIndex;
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi]) return "";
    int16_t nroAsp = s_guilds[gi]->NumeroDeAspirante(nombre);
    if (nroAsp <= 0) return "";
    return s_guilds[gi]->DetallesSolicitudAspirante(nroAsp);
}

//
// GRUPO 6: Diplomacia
//

std::int16_t r_DeclararGuerra(std::int16_t userIndex, const std::string& guildGuerra, std::string& refError) {
    int16_t gi = UserList[userIndex].GuildIndex;
    int16_t gig = GuildIndex(guildGuerra);
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi] || gig <= 0 || !s_guilds[gig]) {
        refError = "Clan inválido.";
        return 0;
    }
    s_guilds[gi]->SetRelacion(gig, RELACIONES_GUILD::GUERRA);
    s_guilds[gig]->SetRelacion(gi, RELACIONES_GUILD::GUERRA);
    return gig;
}

std::int16_t r_AceptarPropuestaDePaz(std::int16_t userIndex, const std::string& guildPaz, std::string& refError) {
    int16_t gi = UserList[userIndex].GuildIndex;
    int16_t gig = GuildIndex(guildPaz);
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi] || gig <= 0 || !s_guilds[gig]) {
        refError = "Clan inválido.";
        return 0;
    }
    s_guilds[gi]->SetRelacion(gig, RELACIONES_GUILD::PAZ);
    s_guilds[gig]->SetRelacion(gi, RELACIONES_GUILD::PAZ);
    s_guilds[gi]->AnularPropuestas(gig);
    return gig;
}

std::int16_t r_RechazarPropuestaDeAlianza(std::int16_t userIndex, const std::string& guildPro, std::string& refError) {
    int16_t gi = UserList[userIndex].GuildIndex;
    int16_t gig = GuildIndex(guildPro);
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi] || gig <= 0 || !s_guilds[gig]) {
        refError = "Clan inválido.";
        return 0;
    }
    s_guilds[gi]->AnularPropuestas(gig);
    return gig;
}

std::int16_t r_RechazarPropuestaDePaz(std::int16_t userIndex, const std::string& guildPro, std::string& refError) {
    int16_t gi = UserList[userIndex].GuildIndex;
    int16_t gig = GuildIndex(guildPro);
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi] || gig <= 0 || !s_guilds[gig]) {
        refError = "Clan inválido.";
        return 0;
    }
    s_guilds[gi]->AnularPropuestas(gig);
    return gig;
}

std::int16_t r_AceptarPropuestaDeAlianza(std::int16_t userIndex, const std::string& guildAllie, std::string& refError) {
    int16_t gi = UserList[userIndex].GuildIndex;
    int16_t gig = GuildIndex(guildAllie);
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi] || gig <= 0 || !s_guilds[gig]) {
        refError = "Clan inválido.";
        return 0;
    }
    s_guilds[gi]->SetRelacion(gig, RELACIONES_GUILD::ALIADOS);
    s_guilds[gig]->SetRelacion(gi, RELACIONES_GUILD::ALIADOS);
    s_guilds[gi]->AnularPropuestas(gig);
    return gig;
}

bool r_ClanGeneraPropuesta(std::int16_t userIndex, const std::string& otroClan, RELACIONES_GUILD tipo,
                          const std::string& detalle, std::string& refError) {
    int16_t gi = UserList[userIndex].GuildIndex;
    int16_t gig = GuildIndex(otroClan);
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi] || gig <= 0 || !s_guilds[gig]) {
        refError = "Clan inválido.";
        return false;
    }
    s_guilds[gig]->SetPropuesta(tipo, gi, detalle);
    return true;
}

std::string r_VerPropuesta(std::int16_t userIndex, const std::string& otroGuild, RELACIONES_GUILD tipo, std::string& refError) {
    int16_t gi = UserList[userIndex].GuildIndex;
    int16_t gig = GuildIndex(otroGuild);
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi] || gig <= 0 || !s_guilds[gig]) {
        refError = "Clan inválido.";
        return "";
    }
    RELACIONES_GUILD t;
    return s_guilds[gi]->GetPropuesta(gig, t);
}

std::vector<std::string> r_ListaDePropuestas(std::int16_t userIndex, RELACIONES_GUILD tipo) {
    std::vector<std::string> res;
    int16_t gi = UserList[userIndex].GuildIndex;
    if (gi <= 0 || gi > CANTIDADDECLANES || !s_guilds[gi]) return res;

    int16_t cant = s_guilds[gi]->CantidadPropuestas(tipo);
    for (int16_t i = 0; i < cant; ++i) {
        int16_t otroG = s_guilds[gi]->r_Iterador_ProximaPropuesta(tipo);
        if (otroG > 0 && otroG <= CANTIDADDECLANES && s_guilds[otroG]) {
            res.push_back(s_guilds[otroG]->GuildName());
        }
    }
    return res;
}

//
// Iteradores y monitoreo GM
//

std::int16_t m_Iterador_ProximoUserIndex(std::int16_t guildIndex) {
    if (guildIndex > 0 && guildIndex <= CANTIDADDECLANES && s_guilds[guildIndex]) {
        return s_guilds[guildIndex]->m_Iterador_ProximoUserIndex();
    }
    return 0;
}

std::int16_t Iterador_ProximoGM(std::int16_t guildIndex) {
    if (guildIndex > 0 && guildIndex <= CANTIDADDECLANES && s_guilds[guildIndex]) {
        return s_guilds[guildIndex]->Iterador_ProximoGM();
    }
    return 0;
}

std::int16_t r_Iterador_ProximaPropuesta(std::int16_t guildIndex, RELACIONES_GUILD tipo) {
    if (guildIndex > 0 && guildIndex <= CANTIDADDECLANES && s_guilds[guildIndex]) {
        return s_guilds[guildIndex]->r_Iterador_ProximaPropuesta(tipo);
    }
    return 0;
}

std::int16_t GMEscuchaClan(std::int16_t userIndex, const std::string& guildName) {
    int16_t gi = GuildIndex(guildName);
    if (gi > 0 && gi <= CANTIDADDECLANES && s_guilds[gi]) {
        s_guilds[gi]->GMEscuchaClan(userIndex);
        UserList[userIndex].EscucheClan = gi;
        return gi;
    }
    return 0;
}

void GMDejaDeEscucharClan(std::int16_t userIndex, std::int16_t guildIndex) {
    if (guildIndex > 0 && guildIndex <= CANTIDADDECLANES && s_guilds[guildIndex]) {
        s_guilds[guildIndex]->GMDejaDeEscucharClan(userIndex);
    }
    if (userIndex > 0 && static_cast<size_t>(userIndex) < UserList.size()) {
        UserList[userIndex].EscucheClan = 0;
    }
}

std::string m_ListaDeMiembrosOnline(std::int16_t, std::int16_t guildIndex) {
    if (guildIndex <= 0 || guildIndex > CANTIDADDECLANES || !s_guilds[guildIndex]) return "";
    std::string res;
    int16_t u = s_guilds[guildIndex]->m_Iterador_ProximoUserIndex();
    while (u != 0) {
        if (static_cast<size_t>(u) < UserList.size()) {
            if (!res.empty()) res += ",";
            res += UserList[u].name;
        }
        u = s_guilds[guildIndex]->m_Iterador_ProximoUserIndex();
    }
    return res;
}

// Stubs de envío de protocolo
void SendGuildNews(std::int16_t) {}
void SendGuildDetails(std::int16_t, const std::string&) {}
void SendGuildLeaderInfo(std::int16_t) {}
void SendDetallesPersonaje(std::int16_t, const std::string&) {}

} // namespace modGuilds
