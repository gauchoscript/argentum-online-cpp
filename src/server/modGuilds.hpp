#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <functional>
#include "Declares.hpp"

// Constantes de configuración de clanes (modGuilds.bas / clsClan.cls)
constexpr std::int16_t MAX_GUILDS = 1000;
constexpr std::uint8_t CANTIDADMAXIMACODEX = 8;
constexpr std::uint8_t MAXASPIRANTES = 10;
constexpr std::uint8_t MAXANTIFACCION = 5;

constexpr std::int32_t NEWSLENGTH = 1024;
constexpr std::int32_t DESCLENGTH = 256;
constexpr std::int32_t CODEXLENGTH = 256;

// Sonidos asociados a acciones de clanes (modGuilds.bas)
enum class SONIDOS_GUILD : std::int32_t {
    SND_CREACIONCLAN = 44,
    SND_ACEPTADOCLAN = 43,
    SND_DECLAREWAR = 45
};

// Relaciones diplomáticas inter-clan (modGuilds.bas)
enum class RELACIONES_GUILD : std::int32_t {
    GUERRA = -1,
    PAZ = 0,
    ALIADOS = 1
};

// Objetivo de envío de red para paquetes de clanes
enum class SendTarget : std::int32_t {
    ToAll = 0,
    ToDiosesYclan = 1,
    ToAdmins = 2
};

// Hook opcional para interceptar despachos de red en pruebas unitarias
using SendDataCallback = std::function<void(SendTarget target, int16_t guildIndex, const std::string& message)>;

namespace modGuilds {

// Variable pública con la cantidad de clanes en el servidor
extern std::int16_t CANTIDADDECLANES;

// Configuración de rutas (permite inyectar fixtures en tests unitarios)
void SetGuildsPath(const std::string& path);
std::string GetGuildsPath();
void SetSendDataHook(SendDataCallback hook);

// Grupo 1: Conversiones y validaciones base
ALINEACION_GUILD String2Alineacion(const std::string& s);
std::string Alineacion2String(ALINEACION_GUILD alineacion);
std::string Relacion2String(RELACIONES_GUILD relacion);
RELACIONES_GUILD String2Relacion(const std::string& s);
bool GuildNameValido(const std::string& cad);

// Grupo 7: Ciclo de vida y carga global
void LoadGuildsDB();
void ResetGuildsDB();

// Grupo 3 & 7: Miembros y reglas de negocio
bool m_ConectarMiembroAClan(std::int16_t userIndex, std::int16_t guildIndex);
bool m_ValidarPermanencia(std::int16_t userIndex, bool sumaAntifaccion, bool& cambioAlineacion);
void UpdateGuildMembers(std::int16_t guildIndex);
ALINEACION_GUILD BajarGrado(std::int16_t guildIndex);
void m_DesconectarMiembroDelClan(std::int16_t userIndex, std::int16_t guildIndex);
std::int16_t m_EcharMiembroDeClan(std::int16_t expulsador, const std::string& expulsado);
bool m_PuedeSalirDeClan(const std::string& nombre, std::int16_t guildIndex, std::int16_t quienLoEchaUI);
bool m_EstadoPermiteEntrar(std::int16_t userIndex, std::int16_t guildIndex);
bool m_EstadoPermiteEntrarChar(const std::string& personaje, std::int16_t guildIndex);

// Metadata y edición
void ActualizarWebSite(std::int16_t userIndex, const std::string& web);
void ChangeCodexAndDesc(const std::string& desc, const std::vector<std::string>& codex, std::int16_t guildIndex);
void ActualizarNoticias(std::int16_t userIndex, const std::string& datos);

// Fundación
bool PuedeFundarUnClan(std::int16_t userIndex, ALINEACION_GUILD alineacion, std::string& refError);
bool CrearNuevoClan(std::int16_t fundadorIndex, const std::string& desc, const std::string& guildName,
                   const std::string& url, const std::vector<std::string>& codex,
                   ALINEACION_GUILD alineacion, std::string& refError);
bool YaExiste(const std::string& guildName);
bool HasFound(const std::string& userName);

// Grupo 5: Elecciones
bool v_AbrirElecciones(std::int16_t userIndex, std::string& refError);
bool v_UsuarioVota(std::int16_t userIndex, const std::string& votado, std::string& refError);
void v_RutinaElecciones();

// Consultas públicas
std::int16_t GuildIndex(const std::string& guildName);
std::string GuildName(std::int16_t guildIndex);
std::string GuildLeader(std::int16_t guildIndex);
std::string GuildAlignment(std::int16_t guildIndex);
std::string GuildFounder(std::int16_t guildIndex);
std::string m_ListaDeMiembrosOnline(std::int16_t userIndex, std::int16_t guildIndex);
std::vector<std::string> PrepareGuildsList();

// Iteradores y monitoreo GM
std::int16_t m_Iterador_ProximoUserIndex(std::int16_t guildIndex);
std::int16_t Iterador_ProximoGM(std::int16_t guildIndex);
std::int16_t r_Iterador_ProximaPropuesta(std::int16_t guildIndex, RELACIONES_GUILD tipo);
std::int16_t GMEscuchaClan(std::int16_t userIndex, const std::string& guildName);
void GMDejaDeEscucharClan(std::int16_t userIndex, std::int16_t guildIndex);

// Grupo 6: Diplomacia
std::int16_t r_DeclararGuerra(std::int16_t userIndex, const std::string& guildGuerra, std::string& refError);
std::int16_t r_AceptarPropuestaDePaz(std::int16_t userIndex, const std::string& guildPaz, std::string& refError);
std::int16_t r_RechazarPropuestaDeAlianza(std::int16_t userIndex, const std::string& guildPro, std::string& refError);
std::int16_t r_RechazarPropuestaDePaz(std::int16_t userIndex, const std::string& guildPro, std::string& refError);
std::int16_t r_AceptarPropuestaDeAlianza(std::int16_t userIndex, const std::string& guildAllie, std::string& refError);
bool r_ClanGeneraPropuesta(std::int16_t userIndex, const std::string& otroClan, RELACIONES_GUILD tipo,
                          const std::string& detalle, std::string& refError);
std::string r_VerPropuesta(std::int16_t userIndex, const std::string& otroGuild, RELACIONES_GUILD tipo, std::string& refError);
std::vector<std::string> r_ListaDePropuestas(std::int16_t userIndex, RELACIONES_GUILD tipo);

// Grupo 4: Aspirantes
bool a_NuevoAspirante(std::int16_t userIndex, const std::string& clan, const std::string& solicitud, std::string& refError);
bool a_AceptarAspirante(std::int16_t userIndex, const std::string& aspirante, std::string& refError);
bool a_RechazarAspirante(std::int16_t userIndex, const std::string& nombre, std::string& refError);
void a_RechazarAspiranteChar(const std::string& aspirante, std::int16_t guild, const std::string& detalles);
std::string a_ObtenerRechazoDeChar(const std::string& aspirante);
std::string a_DetallesAspirante(std::int16_t userIndex, const std::string& nombre);

// Despacho de red / Protocolo (stubs seguros cuando Protocol.bas no está presente)
void SendGuildNews(std::int16_t userIndex);
void SendGuildDetails(std::int16_t userIndex, const std::string& guildName);
void SendGuildLeaderInfo(std::int16_t userIndex);
void SendDetallesPersonaje(std::int16_t userIndex, const std::string& personaje);

} // namespace modGuilds
