#include "clsClan.hpp"
#include "FileIO.hpp"
#include <chrono>
#include <ctime>
#include <filesystem>
#include <cctype>
#include <cstdio>
#include <algorithm>

using namespace FileIO;

clsClan::clsClan() {
    GUILDPATH = modGuilds::GetGuildsPath();
    if (!GUILDPATH.empty() && GUILDPATH.back() != '/' && GUILDPATH.back() != '\\') {
        GUILDPATH += "/";
    }
    GUILDINFOFILE = GUILDPATH + "guildsinfo.inf";
}

void clsClan::ReplaceInvalidChars(std::string& s) {
    std::string res;
    res.reserve(s.size());
    for (unsigned char c : s) {
        if (c != '\r' && c != '\n' && c != static_cast<unsigned char>(0xAC)) {
            res += static_cast<char>(c);
        }
    }
    s = std::move(res);
}

void clsClan::Inicializar(const std::string& guildName, std::int16_t guildNumber, ALINEACION_GUILD alineacion) {
    p_GuildName = guildName;
    p_GuildNumber = guildNumber;
    p_Alineacion = alineacion;

    p_OnlineMembers.clear();
    p_GMsOnline.clear();
    p_PropuestasDePaz.clear();
    p_PropuestasDeAlianza.clear();

    GUILDPATH = modGuilds::GetGuildsPath();
    if (!GUILDPATH.empty() && GUILDPATH.back() != '/' && GUILDPATH.back() != '\\') {
        GUILDPATH += "/";
    }
    GUILDINFOFILE = GUILDPATH + "guildsinfo.inf";
    RELACIONESFILE = GUILDPATH + p_GuildName + "-relaciones.rel";
    MEMBERSFILE = GUILDPATH + p_GuildName + "-members.mem";
    PROPUESTASFILE = GUILDPATH + p_GuildName + "-propositions.pro";
    SOLICITUDESFILE = GUILDPATH + p_GuildName + "-solicitudes.sol";
    VOTACIONESFILE = GUILDPATH + p_GuildName + "-votaciones.vot";

    p_IteradorOnlineMembers = 0;
    p_IteradorPropuesta = 0;
    p_IteradorOnlineGMs = 0;
    p_IteradorRelaciones = 0;

    p_Relaciones.assign(modGuilds::CANTIDADDECLANES + 1, RELACIONES_GUILD::PAZ);
    for (int i = 1; i <= modGuilds::CANTIDADDECLANES; ++i) {
        std::string relStr = GetVar(RELACIONESFILE, "RELACIONES", std::to_string(i));
        p_Relaciones[i] = modGuilds::String2Relacion(relStr);
    }

    for (int i = 1; i <= modGuilds::CANTIDADDECLANES; ++i) {
        std::string penStr = GetVar(PROPUESTASFILE, std::to_string(i), "Pendiente");
        while (!penStr.empty() && (penStr.front() == ' ' || penStr.front() == '\t')) penStr.erase(penStr.begin());
        while (!penStr.empty() && (penStr.back() == ' ' || penStr.back() == '\t' || penStr.back() == '\r' || penStr.back() == '\n')) penStr.pop_back();

        if (penStr == "1") {
            std::string tipoStr = GetVar(PROPUESTASFILE, std::to_string(i), "Tipo");
            while (!tipoStr.empty() && (tipoStr.front() == ' ' || tipoStr.front() == '\t')) tipoStr.erase(tipoStr.begin());
            while (!tipoStr.empty() && (tipoStr.back() == ' ' || tipoStr.back() == '\t' || tipoStr.back() == '\r' || tipoStr.back() == '\n')) tipoStr.pop_back();

            auto tipo = modGuilds::String2Relacion(tipoStr);
            if (tipo == RELACIONES_GUILD::ALIADOS) {
                p_PropuestasDeAlianza.push_back(static_cast<int16_t>(i));
            } else if (tipo == RELACIONES_GUILD::PAZ) {
                p_PropuestasDePaz.push_back(static_cast<int16_t>(i));
            }
        }
    }
}

void clsClan::InicializarNuevoClan(const std::string& fundador) {
    WriteVar(MEMBERSFILE, "INIT", "NroMembers", "0");
    WriteVar(SOLICITUDESFILE, "INIT", "CantSolicitudes", "0");

    std::string oldQ = GetVar(GUILDINFOFILE, "INIT", "nroguilds");
    int newQ = 1;
    try {
        if (!oldQ.empty()) newQ = std::stoi(oldQ) + 1;
    } catch (...) {
        newQ = 1;
    }

    WriteVar(GUILDINFOFILE, "INIT", "NroGuilds", std::to_string(newQ));
    std::string sec = "GUILD" + std::to_string(newQ);
    WriteVar(GUILDINFOFILE, sec, "Founder", fundador);
    WriteVar(GUILDINFOFILE, sec, "GuildName", p_GuildName);

    auto now = std::chrono::system_clock::now();
    std::time_t tt = std::chrono::system_clock::to_time_t(now);
    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &tt);
#else
    localtime_r(&tt, &localTm);
#endif
    char dateBuf[32];
    std::snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d", localTm.tm_mday, localTm.tm_mon + 1, localTm.tm_year + 1900);
    WriteVar(GUILDINFOFILE, sec, "Date", dateBuf);
    WriteVar(GUILDINFOFILE, sec, "Antifaccion", "0");
    WriteVar(GUILDINFOFILE, sec, "Alineacion", modGuilds::Alineacion2String(p_Alineacion));
}

void clsClan::ProcesarFundacionDeOtroClan() {
    p_Relaciones.push_back(RELACIONES_GUILD::PAZ);
}

std::int16_t clsClan::PuntosAntifaccion() const {
    std::string valStr = GetVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "Antifaccion");
    try {
        if (!valStr.empty()) return static_cast<int16_t>(std::stoi(valStr));
    } catch (...) {}
    return 0;
}

void clsClan::PuntosAntifaccion(std::int16_t p) {
    WriteVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "Antifaccion", std::to_string(p));
}

bool clsClan::CambiarAlineacion(ALINEACION_GUILD nuevaAlineacion) {
    p_Alineacion = nuevaAlineacion;
    WriteVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "Alineacion", modGuilds::Alineacion2String(p_Alineacion));
    return (p_Alineacion == ALINEACION_GUILD::ALINEACION_NEUTRO);
}

std::string clsClan::Fundador() const {
    return GetVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "Founder");
}

std::int16_t clsClan::CantidadDeMiembros() const {
    std::string oldQ = GetVar(MEMBERSFILE, "INIT", "NroMembers");
    try {
        if (!oldQ.empty()) return static_cast<int16_t>(std::stoi(oldQ));
    } catch (...) {}
    return 0;
}

void clsClan::SetLeader(const std::string& leader) {
    WriteVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "Leader", leader);
}

std::string clsClan::GetLeader() const {
    return GetVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "Leader");
}

std::vector<std::string> clsClan::GetMemberList() const {
    int oldQ = CantidadDeMiembros();
    std::vector<std::string> list;
    if (oldQ <= 0) return list;
    list.reserve(oldQ);
    for (int i = 1; i <= oldQ; ++i) {
        std::string mem = GetVar(MEMBERSFILE, "Members", "Member" + std::to_string(i));
        for (char& c : mem) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        list.push_back(std::move(mem));
    }
    return list;
}

void clsClan::ConectarMiembro(std::int16_t userIndex) {
    p_OnlineMembers.push_back(userIndex);
}

void clsClan::DesConectarMiembro(std::int16_t userIndex) {
    for (auto it = p_OnlineMembers.begin(); it != p_OnlineMembers.end(); ++it) {
        if (*it == userIndex) {
            p_OnlineMembers.erase(it);
            return;
        }
    }
}

void clsClan::AceptarNuevoMiembro(const std::string& nombre) {
    std::string ruta = CharPath.empty() ? ("charfile/" + nombre + ".chr") : (CharPath + nombre + ".chr");
    if (std::filesystem::exists(std::filesystem::u8path(ruta))) {
        WriteVar(ruta, "GUILD", "GUILDINDEX", std::to_string(p_GuildNumber));
        WriteVar(ruta, "GUILD", "AspiranteA", "0");
    }
    int oldQ = CantidadDeMiembros();
    WriteVar(MEMBERSFILE, "INIT", "NroMembers", std::to_string(oldQ + 1));
    WriteVar(MEMBERSFILE, "Members", "Member" + std::to_string(oldQ + 1), nombre);
}

void clsClan::ExpulsarMiembro(std::string nombre) {
    int oldQ = CantidadDeMiembros();
    if (oldQ <= 0) return;

    std::string nombreUpper = nombre;
    for (char& c : nombreUpper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

    int foundIndex = -1;
    for (int i = 1; i <= oldQ; ++i) {
        std::string mem = GetVar(MEMBERSFILE, "Members", "Member" + std::to_string(i));
        while (!mem.empty() && (mem.front() == ' ' || mem.front() == '\t')) mem.erase(mem.begin());
        while (!mem.empty() && (mem.back() == ' ' || mem.back() == '\t' || mem.back() == '\r' || mem.back() == '\n')) mem.pop_back();
        std::string memUpper = mem;
        for (char& c : memUpper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (memUpper == nombreUpper) {
            foundIndex = i;
            break;
        }
    }

    if (foundIndex != -1) {
        std::string ruta = CharPath.empty() ? ("charfile/" + nombre + ".chr") : (CharPath + nombre + ".chr");
        if (std::filesystem::exists(std::filesystem::u8path(ruta))) {
            WriteVar(ruta, "GUILD", "GuildIndex", "");
            std::string miembroDe = GetVar(ruta, "GUILD", "Miembro");
            std::string pUpper = p_GuildName;
            for (char& c : pUpper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            std::string mUpper = miembroDe;
            for (char& c : mUpper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
            if (mUpper.find(pUpper) == std::string::npos) {
                if (!miembroDe.empty()) miembroDe += ",";
                miembroDe += p_GuildName;
                WriteVar(ruta, "GUILD", "Miembro", miembroDe);
            }
        }

        for (int i = foundIndex; i < oldQ; ++i) {
            std::string nextMem = GetVar(MEMBERSFILE, "Members", "Member" + std::to_string(i + 1));
            WriteVar(MEMBERSFILE, "Members", "Member" + std::to_string(i), nextMem);
        }
        WriteVar(MEMBERSFILE, "Members", "Member" + std::to_string(oldQ), "");
        WriteVar(MEMBERSFILE, "INIT", "NroMembers", std::to_string(oldQ - 1));
    }
}

std::vector<std::string> clsClan::GetAspirantes() const {
    int oldQ = CantidadAspirantes();
    std::vector<std::string> list;
    if (oldQ <= 0) return list;
    list.reserve(oldQ);
    for (int i = 1; i <= oldQ; ++i) {
        list.push_back(GetVar(SOLICITUDESFILE, "SOLICITUD" + std::to_string(i), "Nombre"));
    }
    return list;
}

std::int16_t clsClan::CantidadAspirantes() const {
    std::string temps = GetVar(SOLICITUDESFILE, "INIT", "CantSolicitudes");
    try {
        if (!temps.empty()) return static_cast<int16_t>(std::stoi(temps));
    } catch (...) {}
    return 0;
}

std::string clsClan::DetallesSolicitudAspirante(std::int16_t nroAspirante) const {
    return GetVar(SOLICITUDESFILE, "SOLICITUD" + std::to_string(nroAspirante), "Detalle");
}

std::int16_t clsClan::NumeroDeAspirante(const std::string& nombre) const {
    std::string nameUpper = nombre;
    for (char& c : nameUpper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    for (int i = 1; i <= MAXASPIRANTES; ++i) {
        std::string n = GetVar(SOLICITUDESFILE, "SOLICITUD" + std::to_string(i), "Nombre");
        while (!n.empty() && (n.front() == ' ' || n.front() == '\t')) n.erase(n.begin());
        while (!n.empty() && (n.back() == ' ' || n.back() == '\t' || n.back() == '\r' || n.back() == '\n')) n.pop_back();
        for (char& c : n) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (n == nameUpper) return static_cast<int16_t>(i);
    }
    return 0;
}

void clsClan::NuevoAspirante(const std::string& nombre, const std::string& peticion) {
    int oldQI = CantidadAspirantes();
    for (int i = 1; i <= MAXASPIRANTES; ++i) {
        std::string sec = "SOLICITUD" + std::to_string(i);
        if (GetVar(SOLICITUDESFILE, sec, "Nombre").empty()) {
            WriteVar(SOLICITUDESFILE, sec, "Nombre", nombre);
            std::string det = peticion;
            while (!det.empty() && (det.front() == ' ' || det.front() == '\t')) det.erase(det.begin());
            while (!det.empty() && (det.back() == ' ' || det.back() == '\t' || det.back() == '\r' || det.back() == '\n')) det.pop_back();
            WriteVar(SOLICITUDESFILE, sec, "Detalle", det.empty() ? "Peticion vacia" : det);
            WriteVar(SOLICITUDESFILE, "INIT", "CantSolicitudes", std::to_string(oldQI + 1));

            std::string chrPath = CharPath.empty() ? ("charfile/" + nombre + ".chr") : (CharPath + nombre + ".chr");
            if (std::filesystem::exists(std::filesystem::u8path(chrPath))) {
                WriteVar(chrPath, "GUILD", "ASPIRANTEA", std::to_string(p_GuildNumber));
            }
            return;
        }
    }
}

void clsClan::RetirarAspirante(const std::string& nombre, std::int16_t nroAspirante) {
    int oldQI = CantidadAspirantes();
    std::string chrPath = CharPath.empty() ? ("charfile/" + nombre + ".chr") : (CharPath + nombre + ".chr");
    if (std::filesystem::exists(std::filesystem::u8path(chrPath))) {
        WriteVar(chrPath, "GUILD", "ASPIRANTEA", "0");
        std::string pedidos = GetVar(chrPath, "GUILD", "Pedidos");
        std::string pUpper = p_GuildName;
        for (char& c : pUpper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        std::string pedUpper = pedidos;
        for (char& c : pedUpper) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        if (pedUpper.find(pUpper) == std::string::npos) {
            if (!pedidos.empty()) pedidos += ",";
            pedidos += p_GuildName;
            WriteVar(chrPath, "GUILD", "Pedidos", pedidos);
        }
    }

    int newCount = oldQI > 0 ? (oldQI - 1) : 0;
    WriteVar(SOLICITUDESFILE, "INIT", "CantSolicitudes", std::to_string(newCount));

    for (int i = nroAspirante; i < MAXASPIRANTES; ++i) {
        std::string nextNom = GetVar(SOLICITUDESFILE, "SOLICITUD" + std::to_string(i + 1), "Nombre");
        std::string nextDet = GetVar(SOLICITUDESFILE, "SOLICITUD" + std::to_string(i + 1), "Detalle");
        WriteVar(SOLICITUDESFILE, "SOLICITUD" + std::to_string(i), "Nombre", nextNom);
        WriteVar(SOLICITUDESFILE, "SOLICITUD" + std::to_string(i), "Detalle", nextDet);
    }
    WriteVar(SOLICITUDESFILE, "SOLICITUD" + std::to_string(MAXASPIRANTES), "Nombre", "");
    WriteVar(SOLICITUDESFILE, "SOLICITUD" + std::to_string(MAXASPIRANTES), "Detalle", "");
}

void clsClan::InformarRechazoEnChar(const std::string& nombre, const std::string& detalles) {
    std::string chrPath = CharPath.empty() ? ("charfile/" + nombre + ".chr") : (CharPath + nombre + ".chr");
    WriteVar(chrPath, "GUILD", "MotivoRechazo", detalles);
}

std::string clsClan::GetFechaFundacion() const {
    return GetVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "Date");
}

void clsClan::SetCodex(std::int16_t codexNumber, std::string codex) {
    ReplaceInvalidChars(codex);
    if (codex.size() > CODEXLENGTH) codex = codex.substr(0, CODEXLENGTH);
    WriteVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "Codex" + std::to_string(codexNumber), codex);
}

std::string clsClan::GetCodex(std::int16_t codexNumber) const {
    return GetVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "Codex" + std::to_string(codexNumber));
}

void clsClan::SetURL(const std::string& url) {
    std::string u = url;
    if (u.size() > 40) u = u.substr(0, 40);
    WriteVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "URL", u);
}

std::string clsClan::GetURL() const {
    return GetVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "URL");
}

void clsClan::SetGuildNews(std::string news) {
    ReplaceInvalidChars(news);
    if (news.size() > NEWSLENGTH) news = news.substr(0, NEWSLENGTH);
    WriteVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "GuildNews", news);
}

std::string clsClan::GetGuildNews() const {
    return GetVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "GuildNews");
}

void clsClan::SetDesc(std::string desc) {
    ReplaceInvalidChars(desc);
    if (desc.size() > DESCLENGTH) desc = desc.substr(0, DESCLENGTH);
    WriteVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "Desc", desc);
}

std::string clsClan::GetDesc() const {
    return GetVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "Desc");
}

bool clsClan::EleccionesAbiertas() const {
    std::string ee = GetVar(GUILDINFOFILE, "GUILD" + std::to_string(p_GuildNumber), "EleccionesAbiertas");
    return (ee == "1");
}

void clsClan::AbrirElecciones() {
    std::string sec = "GUILD" + std::to_string(p_GuildNumber);
    WriteVar(GUILDINFOFILE, sec, "EleccionesAbiertas", "1");

    auto tomorrow = std::chrono::system_clock::now() + std::chrono::hours(24);
    std::time_t tt = std::chrono::system_clock::to_time_t(tomorrow);
    std::tm localTm{};
#if defined(_WIN32)
    localtime_s(&localTm, &tt);
#else
    localtime_r(&tt, &localTm);
#endif
    char dateBuf[64];
    std::snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%04d %02d:%02d:%02d",
                  localTm.tm_mday, localTm.tm_mon + 1, localTm.tm_year + 1900,
                  localTm.tm_hour, localTm.tm_min, localTm.tm_sec);
    WriteVar(GUILDINFOFILE, sec, "EleccionesFinalizan", dateBuf);
    WriteVar(VOTACIONESFILE, "INIT", "NumVotos", "0");
}

void clsClan::CerrarElecciones() {
    std::string sec = "GUILD" + std::to_string(p_GuildNumber);
    WriteVar(GUILDINFOFILE, sec, "EleccionesAbiertas", "0");
    WriteVar(GUILDINFOFILE, sec, "EleccionesFinalizan", "");

    std::error_code ec;
    std::filesystem::remove(std::filesystem::u8path(VOTACIONESFILE), ec);
}

void clsClan::ContabilizarVoto(const std::string& votante, const std::string& votado) {
    std::string temps = GetVar(VOTACIONESFILE, "INIT", "NumVotos");
    int q = 0;
    try {
        if (!temps.empty()) q = std::stoi(temps);
    } catch (...) {}
    WriteVar(VOTACIONESFILE, "VOTOS", votante, votado);
    WriteVar(VOTACIONESFILE, "INIT", "NumVotos", std::to_string(q + 1));
}

bool clsClan::YaVoto(const std::string& votante) const {
    std::string voto = GetVar(VOTACIONESFILE, "VOTOS", votante);
    while (!voto.empty() && (voto.front() == ' ' || voto.front() == '\t')) voto.erase(voto.begin());
    while (!voto.empty() && (voto.back() == ' ' || voto.back() == '\t' || voto.back() == '\r' || voto.back() == '\n')) voto.pop_back();
    return !voto.empty();
}

std::string clsClan::ContarVotos(int& cantGanadores) const {
    cantGanadores = 0;
    std::string temps = GetVar(MEMBERSFILE, "INIT", "NroMembers");
    int q = 0;
    try {
        if (!temps.empty()) q = std::stoi(temps);
    } catch (...) {
        q = 0;
    }

    if (q <= 0) return "";

    clsdicc d;
    for (int i = 1; i <= q; ++i) {
        std::string member = GetVar(MEMBERSFILE, "MEMBERS", "Member" + std::to_string(i));
        std::string tempV = GetVar(VOTACIONESFILE, "VOTOS", member);
        if (!tempV.empty()) {
            std::string cur = d.At(tempV);
            if (!cur.empty()) {
                int votes = std::stoi(cur);
                d.AtPut(tempV, votes + 1);
            } else {
                d.AtPut(tempV, 1);
            }
        }
    }

    return d.MayorValor(cantGanadores);
}

bool clsClan::RevisarElecciones(bool forzar) {
    std::string sec = "GUILD" + std::to_string(p_GuildNumber);
    std::string temps = GetVar(GUILDINFOFILE, sec, "EleccionesFinalizan");
    while (!temps.empty() && (temps.front() == ' ' || temps.front() == '\t')) temps.erase(temps.begin());
    while (!temps.empty() && (temps.back() == ' ' || temps.back() == '\t' || temps.back() == '\r' || temps.back() == '\n')) temps.pop_back();

    if (temps.empty() && !forzar) return false;

    bool toca = forzar;
    if (!toca && !temps.empty()) {
        std::tm tmVal{};
        int d = 0, m = 0, y = 0, hr = 0, min = 0, secVal = 0;
        if (std::sscanf(temps.c_str(), "%d/%d/%d %d:%d:%d", &d, &m, &y, &hr, &min, &secVal) >= 3) {
            tmVal.tm_mday = d;
            tmVal.tm_mon = m - 1;
            tmVal.tm_year = y - 1900;
            tmVal.tm_hour = hr;
            tmVal.tm_min = min;
            tmVal.tm_sec = secVal;
            std::time_t sufragioTime = std::mktime(&tmVal);
            std::time_t nowTime = std::time(nullptr);
            if (sufragioTime <= nowTime) {
                toca = true;
            }
        } else if (std::sscanf(temps.c_str(), "%d/%d/%d", &d, &m, &y) == 3) {
            tmVal.tm_mday = d;
            tmVal.tm_mon = m - 1;
            tmVal.tm_year = y - 1900;
            std::time_t sufragioTime = std::mktime(&tmVal);
            std::time_t nowTime = std::time(nullptr);
            if (sufragioTime <= nowTime) {
                toca = true;
            }
        }
    }

    if (!toca) return false;

    int cantGanadores = 0;
    std::string ganador = ContarVotos(cantGanadores);
    bool revisadoExito = false;

    if (cantGanadores > 1) {
        // Empate en la votación:
        // Quirk legacy exacto: CantGanadores contiene el número de empatados,
        // pero el texto legacy dice "... con <CantGanadores> votos..."
        SetGuildNews("*Empate en la votación. " + ganador + " con " + std::to_string(cantGanadores) + " votos ganaron las elecciones del clan.");
    } else if (cantGanadores == 1) {
        auto list = GetMemberList();
        bool found = false;
        for (const auto& mem : list) {
            if (mem == ganador) {
                found = true;
                break;
            }
        }
        if (found) {
            SetGuildNews("*" + ganador + " ganó la elección del clan*");
            SetLeader(ganador);
            revisadoExito = true;
        } else {
            SetGuildNews("*" + ganador + " ganó la elección del clan pero abandonó las filas por lo que la votación queda desierta*");
        }
    } else {
        SetGuildNews("*El período de votación se cerró sin votos*");
    }

    CerrarElecciones();
    return revisadoExito;
}

std::int16_t clsClan::CantidadPropuestas(RELACIONES_GUILD tipo) const {
    if (tipo == RELACIONES_GUILD::ALIADOS) return static_cast<int16_t>(p_PropuestasDeAlianza.size());
    if (tipo == RELACIONES_GUILD::PAZ) return static_cast<int16_t>(p_PropuestasDePaz.size());
    return 0;
}

std::int16_t clsClan::CantidadEnemys() const {
    int16_t cnt = 0;
    for (size_t i = 1; i < p_Relaciones.size(); ++i) {
        if (p_Relaciones[i] == RELACIONES_GUILD::GUERRA) ++cnt;
    }
    return cnt;
}

std::int16_t clsClan::CantidadAllies() const {
    int16_t cnt = 0;
    for (size_t i = 1; i < p_Relaciones.size(); ++i) {
        if (p_Relaciones[i] == RELACIONES_GUILD::ALIADOS) ++cnt;
    }
    return cnt;
}

RELACIONES_GUILD clsClan::GetRelacion(std::int16_t otroGuild) const {
    if (otroGuild >= 1 && static_cast<size_t>(otroGuild) < p_Relaciones.size()) {
        return p_Relaciones[otroGuild];
    }
    return RELACIONES_GUILD::PAZ;
}

void clsClan::SetRelacion(std::int16_t guildIndex, RELACIONES_GUILD relacion) {
    if (guildIndex >= 1 && static_cast<size_t>(guildIndex) < p_Relaciones.size()) {
        p_Relaciones[guildIndex] = relacion;
    }
    WriteVar(RELACIONESFILE, "RELACIONES", std::to_string(guildIndex), modGuilds::Relacion2String(relacion));
}

void clsClan::SetPropuesta(RELACIONES_GUILD tipo, std::int16_t otroGuild, const std::string& detalle) {
    std::string sec = std::to_string(otroGuild);
    WriteVar(PROPUESTASFILE, sec, "Detalle", detalle);
    WriteVar(PROPUESTASFILE, sec, "Tipo", modGuilds::Relacion2String(tipo));
    WriteVar(PROPUESTASFILE, sec, "Pendiente", "1");
    if (tipo == RELACIONES_GUILD::ALIADOS) {
        p_PropuestasDeAlianza.push_back(otroGuild);
    } else if (tipo == RELACIONES_GUILD::PAZ) {
        p_PropuestasDePaz.push_back(otroGuild);
    }
}

void clsClan::AnularPropuestas(std::int16_t otroGuild) {
    std::string sec = std::to_string(otroGuild);
    WriteVar(PROPUESTASFILE, sec, "Detalle", "");
    WriteVar(PROPUESTASFILE, sec, "Pendiente", "0");
    for (auto it = p_PropuestasDePaz.begin(); it != p_PropuestasDePaz.end(); ++it) {
        if (*it == otroGuild) {
            p_PropuestasDePaz.erase(it);
            return;
        }
    }
    for (auto it = p_PropuestasDeAlianza.begin(); it != p_PropuestasDeAlianza.end(); ++it) {
        if (*it == otroGuild) {
            p_PropuestasDeAlianza.erase(it);
            return;
        }
    }
}

std::string clsClan::GetPropuesta(std::int16_t otroGuild, RELACIONES_GUILD& tipo) const {
    std::string sec = std::to_string(otroGuild);
    std::string det = GetVar(PROPUESTASFILE, sec, "Detalle");
    tipo = modGuilds::String2Relacion(GetVar(PROPUESTASFILE, sec, "Tipo"));
    return det;
}

bool clsClan::HayPropuesta(std::int16_t otroGuild, RELACIONES_GUILD tipo) const {
    if (tipo == RELACIONES_GUILD::ALIADOS) {
        for (int16_t g : p_PropuestasDeAlianza) if (g == otroGuild) return true;
    } else if (tipo == RELACIONES_GUILD::PAZ) {
        for (int16_t g : p_PropuestasDePaz) if (g == otroGuild) return true;
    }
    return false;
}

std::int16_t clsClan::m_Iterador_ProximoUserIndex() {
    if (p_OnlineMembers.empty()) return 0;
    if (p_IteradorOnlineMembers >= p_OnlineMembers.size()) p_IteradorOnlineMembers = 0;
    return p_OnlineMembers[p_IteradorOnlineMembers++];
}

std::int16_t clsClan::Iterador_ProximoGM() {
    if (p_GMsOnline.empty()) return 0;
    if (p_IteradorOnlineGMs >= p_GMsOnline.size()) p_IteradorOnlineGMs = 0;
    return p_GMsOnline[p_IteradorOnlineGMs++];
}

std::int16_t clsClan::r_Iterador_ProximaPropuesta(RELACIONES_GUILD tipo) {
    if (tipo == RELACIONES_GUILD::ALIADOS) {
        if (p_PropuestasDeAlianza.empty()) return 0;
        if (p_IteradorPropuesta >= p_PropuestasDeAlianza.size()) p_IteradorPropuesta = 0;
        return p_PropuestasDeAlianza[p_IteradorPropuesta++];
    } else if (tipo == RELACIONES_GUILD::PAZ) {
        if (p_PropuestasDePaz.empty()) return 0;
        if (p_IteradorPropuesta >= p_PropuestasDePaz.size()) p_IteradorPropuesta = 0;
        return p_PropuestasDePaz[p_IteradorPropuesta++];
    }
    return 0;
}

void clsClan::GMEscuchaClan(std::int16_t userIndex) {
    for (int16_t u : p_GMsOnline) if (u == userIndex) return;
    p_GMsOnline.push_back(userIndex);
}

void clsClan::GMDejaDeEscucharClan(std::int16_t userIndex) {
    for (auto it = p_GMsOnline.begin(); it != p_GMsOnline.end(); ++it) {
        if (*it == userIndex) {
            p_GMsOnline.erase(it);
            return;
        }
    }
}
