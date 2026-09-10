#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include "modGuilds.hpp"
#include "cSolicitud.hpp"
#include "clsdicc.hpp"

class clsClan {
private:
    std::string p_GuildName;
    ALINEACION_GUILD p_Alineacion{ALINEACION_GUILD::ALINEACION_NINGUNA};
    std::vector<std::int16_t> p_OnlineMembers;
    std::vector<std::int16_t> p_GMsOnline;
    std::vector<std::int16_t> p_PropuestasDePaz;
    std::vector<std::int16_t> p_PropuestasDeAlianza;

    std::size_t p_IteradorRelaciones{0};
    std::size_t p_IteradorOnlineMembers{0};
    std::size_t p_IteradorPropuesta{0};
    std::size_t p_IteradorOnlineGMs{0};

    std::int16_t p_GuildNumber{0};
    std::vector<RELACIONES_GUILD> p_Relaciones;

    std::string GUILDINFOFILE;
    std::string GUILDPATH;
    std::string MEMBERSFILE;
    std::string SOLICITUDESFILE;
    std::string PROPUESTASFILE;
    std::string RELACIONESFILE;
    std::string VOTACIONESFILE;

    void ReplaceInvalidChars(std::string& s);
    void CerrarElecciones();

public:
    clsClan();
    ~clsClan() = default;

    // Inicialización y ciclo de vida
    void Inicializar(const std::string& guildName, std::int16_t guildNumber, ALINEACION_GUILD alineacion);
    void InicializarNuevoClan(const std::string& fundador);
    void ProcesarFundacionDeOtroClan();

    // Propiedades
    std::string GuildName() const { return p_GuildName; }
    ALINEACION_GUILD Alineacion() const { return p_Alineacion; }
    std::int16_t PuntosAntifaccion() const;
    void PuntosAntifaccion(std::int16_t p);
    bool CambiarAlineacion(ALINEACION_GUILD nuevaAlineacion);

    std::string Fundador() const;
    std::int16_t CantidadDeMiembros() const;
    void SetLeader(const std::string& leader);
    std::string GetLeader() const;
    std::vector<std::string> GetMemberList() const;

    void ConectarMiembro(std::int16_t userIndex);
    void DesConectarMiembro(std::int16_t userIndex);
    void AceptarNuevoMiembro(const std::string& nombre);
    void ExpulsarMiembro(std::string nombre);

    // Aspirantes y Solicitudes
    std::vector<std::string> GetAspirantes() const;
    std::int16_t CantidadAspirantes() const;
    std::string DetallesSolicitudAspirante(std::int16_t nroAspirante) const;
    std::int16_t NumeroDeAspirante(const std::string& nombre) const;
    void NuevoAspirante(const std::string& nombre, const std::string& peticion);
    void RetirarAspirante(const std::string& nombre, std::int16_t nroAspirante);
    void InformarRechazoEnChar(const std::string& nombre, const std::string& detalles);

    // Metadata y Definición del clan
    std::string GetFechaFundacion() const;
    void SetCodex(std::int16_t codexNumber, std::string codex);
    std::string GetCodex(std::int16_t codexNumber) const;
    void SetURL(const std::string& url);
    std::string GetURL() const;
    void SetGuildNews(std::string news);
    std::string GetGuildNews() const;
    void SetDesc(std::string desc);
    std::string GetDesc() const;

    // Elecciones
    bool EleccionesAbiertas() const;
    void AbrirElecciones();
    void ContabilizarVoto(const std::string& votante, const std::string& votado);
    bool YaVoto(const std::string& votante) const;
    std::string ContarVotos(int& cantGanadores) const;
    bool RevisarElecciones(bool forzar = false);

    // Relaciones diplomáticas
    std::int16_t CantidadPropuestas(RELACIONES_GUILD tipo) const;
    std::int16_t CantidadEnemys() const;
    std::int16_t CantidadAllies() const;
    RELACIONES_GUILD GetRelacion(std::int16_t otroGuild) const;
    void SetRelacion(std::int16_t guildIndex, RELACIONES_GUILD relacion);
    void SetPropuesta(RELACIONES_GUILD tipo, std::int16_t otroGuild, const std::string& detalle);
    void AnularPropuestas(std::int16_t otroGuild);
    std::string GetPropuesta(std::int16_t otroGuild, RELACIONES_GUILD& tipo) const;
    bool HayPropuesta(std::int16_t otroGuild, RELACIONES_GUILD tipo) const;

    // Iteradores
    std::int16_t m_Iterador_ProximoUserIndex();
    std::int16_t Iterador_ProximoGM();
    std::int16_t r_Iterador_ProximaPropuesta(RELACIONES_GUILD tipo);
    void GMEscuchaClan(std::int16_t userIndex);
    void GMDejaDeEscucharClan(std::int16_t userIndex);

    // Getters auxiliares de rutas
    const std::string& GetGuildInfoFile() const { return GUILDINFOFILE; }
    const std::string& GetMembersFile() const { return MEMBERSFILE; }
    const std::string& GetSolicitudesFile() const { return SOLICITUDESFILE; }
    const std::string& GetVotacionesFile() const { return VOTACIONESFILE; }
    const std::string& GetRelacionesFile() const { return RELACIONESFILE; }
    const std::string& GetPropuestasFile() const { return PROPUESTASFILE; }
};
