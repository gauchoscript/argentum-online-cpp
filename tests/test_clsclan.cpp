#include <doctest/doctest.h>
#include "server/modGuilds.hpp"
#include "server/clsClan.hpp"
#include "server/cSolicitud.hpp"
#include "server/FileIO.hpp"
#include <filesystem>
#include <fstream>

using namespace FileIO;

TEST_SUITE("clsClan_modGuilds_SistemaClanes") {

TEST_CASE("Validación contra Fixtures Reales de Producción (tests/fixtures/guilds/real/)") {
    // Apuntamos temporalmente el path a las fixtures reales de producción
    std::string realPath = "tests/fixtures/guilds/real/";
    REQUIRE(std::filesystem::exists(realPath + "guildsinfo.inf"));
    REQUIRE(std::filesystem::exists(realPath + "Game Masters-members.mem"));
    REQUIRE(std::filesystem::exists(realPath + "Game Masters-solicitudes.sol"));

    modGuilds::SetGuildsPath(realPath);
    modGuilds::LoadGuildsDB();

    // Verificamos cabecera global [INIT] NroGuilds=1
    CHECK(modGuilds::CANTIDADDECLANES == 1);
    CHECK(modGuilds::GuildIndex("Game Masters") == 1);
    CHECK(modGuilds::GuildName(1) == "Game Masters");
    CHECK(modGuilds::GuildFounder(1) == "Elio");
    CHECK(modGuilds::GuildLeader(1) == "Elio");
    CHECK(modGuilds::GuildAlignment(1) == "Game Masters");

    // Instanciamos el clan para verificar padrón y solicitudes directamente
    clsClan gmClan;
    gmClan.Inicializar("Game Masters", 1, ALINEACION_GUILD::ALINEACION_MASTER);

    // 1. Padrón real: Member1=Elio -> GetMemberList debe retornar "ELIO" (UCase$)
    CHECK(gmClan.CantidadDeMiembros() == 1);
    auto members = gmClan.GetMemberList();
    REQUIRE(members.size() == 1);
    CHECK(members[0] == "ELIO");

    // 2. Solicitudes reales: CantSolicitudes=0
    CHECK(gmClan.CantidadAspirantes() == 0);
    auto aspirantes = gmClan.GetAspirantes();
    CHECK(aspirantes.empty());

    // 3. Codex real: a, b, c, d
    CHECK(gmClan.GetCodex(1) == "a");
    CHECK(gmClan.GetCodex(2) == "b");
    CHECK(gmClan.GetCodex(3) == "c");
    CHECK(gmClan.GetCodex(4) == "d");
    CHECK(gmClan.GetCodex(5).empty());

    // 4. Descripción y Noticias (en disco Windows-1252 ANSI, 'ó' es el byte 0xF3)
    CHECK(gmClan.GetDesc() == "GMS");
    CHECK(gmClan.GetGuildNews() == "Clan creado con alineaci\xF3n: Game Masters");

    // Restauramos path por defecto
    modGuilds::SetGuildsPath("guilds/");
}

TEST_CASE("Frontera de Serialización cSolicitud: Mapeo Nombre/Detalle vs UserName/desc") {
    // Este test verifica explícitamente el requerimiento arquitectónico de frontera:
    // En disco INI (.sol): Claves "Nombre" y "Detalle"
    // En struct C++ cSolicitud: Campos "UserName" y "desc"
    std::string solPath = "tests/fixtures/guilds/Legion de Honor-solicitudes.sol";
    REQUIRE(std::filesystem::exists(solPath));

    // Leemos directamente del INI para confirmar claves en disco
    std::string nombreDisco = GetVar(solPath, "SOLICITUD1", "Nombre");
    std::string detalleDisco = GetVar(solPath, "SOLICITUD1", "Detalle");
    CHECK(nombreDisco == "NOVATO");
    CHECK(detalleDisco == "Deseo unirme a la Legion de Honor para aprender a jugar.");

    // Mapeamos hacia el DTO cSolicitud preservando los nombres originales de campos
    cSolicitud solicitudDto;
    solicitudDto.UserName = nombreDisco;
    solicitudDto.desc = detalleDisco;

    CHECK(solicitudDto.UserName == "NOVATO");
    CHECK(solicitudDto.desc == "Deseo unirme a la Legion de Honor para aprender a jugar.");
}

TEST_CASE("Grupo 1: Conversiones y Validaciones Sintácticas") {
    // String2Alineacion y Alineacion2String bidireccional
    CHECK(modGuilds::String2Alineacion("Neutral") == ALINEACION_GUILD::ALINEACION_NEUTRO);
    CHECK(modGuilds::String2Alineacion("Del Mal") == ALINEACION_GUILD::ALINEACION_LEGION);
    CHECK(modGuilds::String2Alineacion("Real") == ALINEACION_GUILD::ALINEACION_ARMADA);
    CHECK(modGuilds::String2Alineacion("Game Masters") == ALINEACION_GUILD::ALINEACION_MASTER);
    CHECK(modGuilds::String2Alineacion("Legal") == ALINEACION_GUILD::ALINEACION_CIUDA);
    CHECK(modGuilds::String2Alineacion("Criminal") == ALINEACION_GUILD::ALINEACION_CRIMINAL);

    CHECK(modGuilds::Alineacion2String(ALINEACION_GUILD::ALINEACION_NEUTRO) == "Neutral");
    CHECK(modGuilds::Alineacion2String(ALINEACION_GUILD::ALINEACION_LEGION) == "Del Mal");
    CHECK(modGuilds::Alineacion2String(ALINEACION_GUILD::ALINEACION_ARMADA) == "Real");
    CHECK(modGuilds::Alineacion2String(ALINEACION_GUILD::ALINEACION_MASTER) == "Game Masters");
    CHECK(modGuilds::Alineacion2String(ALINEACION_GUILD::ALINEACION_CIUDA) == "Legal");
    CHECK(modGuilds::Alineacion2String(ALINEACION_GUILD::ALINEACION_CRIMINAL) == "Criminal");

    // String2Relacion y Relacion2String
    CHECK(modGuilds::Relacion2String(RELACIONES_GUILD::ALIADOS) == "A");
    CHECK(modGuilds::Relacion2String(RELACIONES_GUILD::GUERRA) == "G");
    CHECK(modGuilds::Relacion2String(RELACIONES_GUILD::PAZ) == "P");

    CHECK(modGuilds::String2Relacion("A") == RELACIONES_GUILD::ALIADOS);
    CHECK(modGuilds::String2Relacion("G") == RELACIONES_GUILD::GUERRA);
    CHECK(modGuilds::String2Relacion("P") == RELACIONES_GUILD::PAZ);
    CHECK(modGuilds::String2Relacion("") == RELACIONES_GUILD::PAZ);
    CHECK(modGuilds::String2Relacion("xyz") == RELACIONES_GUILD::PAZ);

    // GuildNameValido (solo letras a-z minúsculas, espacios y código 255)
    CHECK(modGuilds::GuildNameValido("Legion de Honor") == true);
    CHECK(modGuilds::GuildNameValido("Clan Real") == true);
    CHECK(modGuilds::GuildNameValido("Clan123") == false); // no admite números
    CHECK(modGuilds::GuildNameValido("Clan-Caos") == false); // no admite guiones
    CHECK(modGuilds::GuildNameValido("Clan_Caos") == false); // no admite guiones bajos
}

// ==============================================================================
// TEST CRÍTICO DE REPRODUCCIÓN DEL COMPORTAMIENTO EXACTO DEL LEGACY EN EMPATES
// ==============================================================================
// IMPORTANTE / CONVENCIONES DEL PROYECTO:
// Este caso de prueba verifica el comportamiento ante un empate electoral en clsClan.cls.
// En el código legacy original (clsClan.cls:589-616), cuando ocurre un empate:
// 1. SetLeader NO es llamado; el líder existente permanece en el cargo intacto.
// 2. CerrarElecciones se ejecuta obligatoriamente, eliminando el archivo .vot de disco.
// 3. RevisarElecciones retorna 'false'.
// 4. ERROR DEL AUTOR ORIGINAL PRESERVADO: El autor concatenó CantGanadores en la noticia:
//    "*Empate en la votación. " & Ganador & " con " & CantGanadores & " votos ganaron las elecciones del clan."
//    pero CantGanadores es la cantidad de candidatos empatados devuelta por clsdicc.MayorValor,
//    NO la cantidad de votos recibidos por cada uno.
//
// El éxito de este test consiste en reproducir EXACTAMENTE este error histórico.
// QUEDA TERMINANTEMENTE PROHIBIDO "corregir" el mensaje o nombrar a un ganador.
// ==============================================================================
TEST_CASE("Grupo 5: Elecciones - Reproducción End-to-End de Empate y Quirk Legacy Verbatim") {
    std::string sandboxDir = "tests/fixtures/scratch_elections_tie/";
    std::error_code ec;
    std::filesystem::remove_all(sandboxDir, ec);
    std::filesystem::create_directories(sandboxDir, ec);

    modGuilds::SetGuildsPath(sandboxDir);

    // Creamos un clan en el sandbox con líder inicial
    clsClan clan;
    clan.Inicializar("ClanTestEmpate", 1, ALINEACION_GUILD::ALINEACION_NEUTRO);
    clan.InicializarNuevoClan("LIDER_ACTUAL");
    clan.SetLeader("LIDER_ACTUAL");

    // Agregamos 4 miembros al padrón del clan
    clan.AceptarNuevoMiembro("MIEMBRO1");
    clan.AceptarNuevoMiembro("MIEMBRO2");
    clan.AceptarNuevoMiembro("MIEMBRO3");
    clan.AceptarNuevoMiembro("MIEMBRO4");

    CHECK(clan.GetLeader() == "LIDER_ACTUAL");
    CHECK(clan.CantidadDeMiembros() == 4);

    // Abrimos elecciones
    clan.AbrirElecciones();
    CHECK(clan.EleccionesAbiertas() == true);
    std::string votFile = clan.GetVotacionesFile();
    CHECK(std::filesystem::exists(votFile));

    // Votación con empate exacto 2 a 2:
    // MIEMBRO1 vota a CANDIDATO_A
    // MIEMBRO2 vota a CANDIDATO_A
    // MIEMBRO3 vota a CANDIDATO_B
    // MIEMBRO4 vota a CANDIDATO_B
    clan.ContabilizarVoto("MIEMBRO1", "CANDIDATO_A");
    clan.ContabilizarVoto("MIEMBRO2", "CANDIDATO_A");
    clan.ContabilizarVoto("MIEMBRO3", "CANDIDATO_B");
    clan.ContabilizarVoto("MIEMBRO4", "CANDIDATO_B");

    CHECK(clan.YaVoto("MIEMBRO1") == true);
    CHECK(clan.YaVoto("MIEMBRO4") == true);

    // Verificamos conteo crudo con clsdicc
    int cantGanadores = 0;
    std::string ganadoresStr = clan.ContarVotos(cantGanadores);
    CHECK(cantGanadores == 2); // Son 2 candidatos empatados (CANDIDATO_A y CANDIDATO_B)
    CHECK(ganadoresStr == "CANDIDATO_A,CANDIDATO_B"); // En orden de inserción y mayúsculas

    // Ejecutamos RevisarElecciones forzando el cierre del comicio
    bool resultadoRevision = clan.RevisarElecciones(true);

    // 1. RevisarElecciones debe devolver false ante un empate
    CHECK(resultadoRevision == false);

    // 2. SetLeader NO debe haber sido llamado: el líder actual continúa en funciones
    CHECK(clan.GetLeader() == "LIDER_ACTUAL");

    // 3. CerrarElecciones debe haberse ejecutado: elecciones cerradas y archivo .vot destruido
    CHECK(clan.EleccionesAbiertas() == false);
    CHECK(!std::filesystem::exists(votFile));

    // 4. QUIRK LEGACY VERBATIM: El anuncio de noticias debe decir "con 2 votos",
    // reportando el número de empatados (2) en lugar de la cantidad de votos que recibieron.
    std::string expectedNews = "*Empate en la votación. CANDIDATO_A,CANDIDATO_B con 2 votos ganaron las elecciones del clan.";
    CHECK(clan.GetGuildNews() == expectedNews);

    // Limpieza
    std::filesystem::remove_all(sandboxDir, ec);
    modGuilds::SetGuildsPath("guilds/");
}

TEST_CASE("Grupo 5: Elecciones - Ganador Único Legítimo") {
    std::string sandboxDir = "tests/fixtures/scratch_elections_winner/";
    std::error_code ec;
    std::filesystem::remove_all(sandboxDir, ec);
    std::filesystem::create_directories(sandboxDir, ec);

    modGuilds::SetGuildsPath(sandboxDir);

    clsClan clan;
    clan.Inicializar("ClanTestGanador", 1, ALINEACION_GUILD::ALINEACION_NEUTRO);
    clan.InicializarNuevoClan("VIEJO_LIDER");
    clan.SetLeader("VIEJO_LIDER");

    clan.AceptarNuevoMiembro("MIEMBRO1");
    clan.AceptarNuevoMiembro("MIEMBRO2");
    clan.AceptarNuevoMiembro("MIEMBRO3");

    clan.AbrirElecciones();

    // MIEMBRO1 y MIEMBRO2 votan a MIEMBRO1 (2 votos)
    // MIEMBRO3 vota a VIEJO_LIDER (1 voto)
    clan.ContabilizarVoto("MIEMBRO1", "MIEMBRO1");
    clan.ContabilizarVoto("MIEMBRO2", "MIEMBRO1");
    clan.ContabilizarVoto("MIEMBRO3", "VIEJO_LIDER");

    bool exito = clan.RevisarElecciones(true);
    CHECK(exito == true);

    // El nuevo líder debe ser MIEMBRO1
    CHECK(clan.GetLeader() == "MIEMBRO1");
    CHECK(clan.GetGuildNews() == "*MIEMBRO1 ganó la elección del clan*");
    CHECK(clan.EleccionesAbiertas() == false);
    CHECK(!std::filesystem::exists(clan.GetVotacionesFile()));

    std::filesystem::remove_all(sandboxDir, ec);
    modGuilds::SetGuildsPath("guilds/");
}

TEST_CASE("Grupo 3 & 6: Padrón, Expulsión y Relaciones Diplomáticas") {
    std::string sandboxDir = "tests/fixtures/scratch_guild_relations/";
    std::error_code ec;
    std::filesystem::remove_all(sandboxDir, ec);
    std::filesystem::create_directories(sandboxDir, ec);

    modGuilds::SetGuildsPath(sandboxDir);
    modGuilds::CANTIDADDECLANES = 2;

    clsClan clan1;
    clan1.Inicializar("ClanAlfa", 1, ALINEACION_GUILD::ALINEACION_ARMADA);
    clan1.InicializarNuevoClan("Rey");

    clsClan clan2;
    clan2.Inicializar("ClanBeta", 2, ALINEACION_GUILD::ALINEACION_LEGION);
    clan2.InicializarNuevoClan("Lord");

    // Diplomacia inicial en PAZ
    CHECK(clan1.GetRelacion(2) == RELACIONES_GUILD::PAZ);

    // Declaramos GUERRA
    clan1.SetRelacion(2, RELACIONES_GUILD::GUERRA);
    CHECK(clan1.GetRelacion(2) == RELACIONES_GUILD::GUERRA);
    CHECK(clan1.CantidadEnemys() == 1);

    // Propuesta de Paz
    clan2.SetPropuesta(RELACIONES_GUILD::PAZ, 1, "Pedimos tregua");
    CHECK(clan2.CantidadPropuestas(RELACIONES_GUILD::PAZ) == 1);
    CHECK(clan2.HayPropuesta(1, RELACIONES_GUILD::PAZ) == true);

    RELACIONES_GUILD tipoProp;
    std::string detalle = clan2.GetPropuesta(1, tipoProp);
    CHECK(detalle == "Pedimos tregua");
    CHECK(tipoProp == RELACIONES_GUILD::PAZ);

    clan2.AnularPropuestas(1);
    CHECK(clan2.HayPropuesta(1, RELACIONES_GUILD::PAZ) == false);

    // Padrón y expulsión
    clan1.AceptarNuevoMiembro("SOLDADO1");
    clan1.AceptarNuevoMiembro("SOLDADO2");
    CHECK(clan1.CantidadDeMiembros() == 2);

    clan1.ExpulsarMiembro("SOLDADO1");
    CHECK(clan1.CantidadDeMiembros() == 1);
    auto miembrosRestantes = clan1.GetMemberList();
    REQUIRE(miembrosRestantes.size() == 1);
    CHECK(miembrosRestantes[0] == "SOLDADO2");

    std::filesystem::remove_all(sandboxDir, ec);
    modGuilds::SetGuildsPath("guilds/");
}

}
