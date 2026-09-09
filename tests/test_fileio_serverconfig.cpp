#include <doctest/doctest.h>
#include "server/FileIO.hpp"
#include "server/Declares.hpp"
#include <filesystem>
#include <fstream>

// ============================================================================
// Pruebas Unitarias para FileIO Grupo 3 (Configuración Servidor y Roles)
// ============================================================================

TEST_SUITE("FileIO_ConfiguracionServidor") {

    TEST_CASE("Verificación de Roles: Insensibilidad a mayúsculas/minúsculas y Prefijos '*' / '+'") {
        std::string testDir = "tests/temp_g3_roles/";
        std::filesystem::create_directories(testDir);
        std::string serverIniPath = testDir + "Server.ini";

        IniPath = testDir;

        // Crear un Server.ini sintético con Admines, Dioses, SemiDioses, Consejeros y RolesMasters
        std::ofstream out(serverIniPath, std::ios::binary);
        out << "[INIT]\r\n";
        out << "Admines=2\r\n";
        out << "Dioses=1\r\n";
        out << "SemiDioses=1\r\n";
        out << "Consejeros=1\r\n";
        out << "RolesMasters=1\r\n";
        out << "\r\n";
        out << "[Admines]\r\n";
        out << "Admin1=*PepeAdmin\r\n";
        out << "Admin2=+JuanAdmin\r\n";
        out << "\r\n";
        out << "[Dioses]\r\n";
        out << "Dios1=ZeusGod\r\n";
        out << "\r\n";
        out << "[SemiDioses]\r\n";
        out << "SemiDios1=*HerculesSemi\r\n";
        out << "\r\n";
        out << "[Consejeros]\r\n";
        out << "Consejero1=Sabio\r\n";
        out << "\r\n";
        out << "[RolesMasters]\r\n";
        out << "RM1=MasterRole\r\n";
        out.close();

        // 1. Verificación de insensibilidad a mayúsculas/minúsculas y descarte de prefijo '*' y '+'
        CHECK(FileIO::EsAdmin("pepeadmin"));
        CHECK(FileIO::EsAdmin("PEPEADMIN"));
        CHECK(FileIO::EsAdmin("PepeAdmin"));

        CHECK(FileIO::EsAdmin("juanadmin"));
        CHECK(FileIO::EsAdmin("JUANADMIN"));

        CHECK(FileIO::EsDios("zeusgod"));
        CHECK(FileIO::EsDios("ZEUSGOD"));

        CHECK(FileIO::EsSemiDios("herculessemi"));
        CHECK(FileIO::EsConsejero("sabio"));
        CHECK(FileIO::EsRolesMaster("masterrole"));

        // Limpieza de directorio de prueba
        IniPath.clear();
        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("Verificación de Independencia No-Jerárquica entre Roles") {
        std::string testDir = "tests/temp_g3_indep/";
        std::filesystem::create_directories(testDir);
        std::string serverIniPath = testDir + "Server.ini";

        IniPath = testDir;

        std::ofstream out(serverIniPath, std::ios::binary);
        out << "[INIT]\r\n";
        out << "Admines=1\r\n";
        out << "Dioses=1\r\n";
        out << "SemiDioses=0\r\n";
        out << "Consejeros=0\r\n";
        out << "RolesMasters=0\r\n";
        out << "\r\n";
        out << "[Admines]\r\n";
        out << "Admin1=ExclusivoAdmin\r\n";
        out << "\r\n";
        out << "[Dioses]\r\n";
        out << "Dios1=ExclusivoDios\r\n";
        out.close();

        // Un Admin NO debe ser considerado Dios automáticamente (ausencia de jerarquía)
        CHECK(FileIO::EsAdmin("ExclusivoAdmin"));
        CHECK_FALSE(FileIO::EsDios("ExclusivoAdmin"));
        CHECK_FALSE(FileIO::EsSemiDios("ExclusivoAdmin"));
        CHECK_FALSE(FileIO::EsConsejero("ExclusivoAdmin"));
        CHECK_FALSE(FileIO::EsRolesMaster("ExclusivoAdmin"));

        // Un Dios NO debe ser considerado Admin automáticamente
        CHECK(FileIO::EsDios("ExclusivoDios"));
        CHECK_FALSE(FileIO::EsAdmin("ExclusivoDios"));

        IniPath.clear();
        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("Carga de MOTD (Límites y Arreglo MOTD)") {
        std::string testDir = "tests/temp_g3_motd/";
        std::filesystem::create_directories(testDir + "Dat/");
        std::string motdIniPath = testDir + "Dat/Motd.ini";

        DatPath = testDir + "Dat/";

        // 1. Caso de MOTD Vacío (NumLines = 0)
        {
            std::ofstream out(motdIniPath, std::ios::binary);
            out << "[INIT]\r\nNumLines=0\r\n";
            out.close();

            FileIO::LoadMotd();
            CHECK(MaxLines == 0);
            CHECK(MOTD.size() == 1);
        }

        // 2. Caso de MOTD con 3 Líneas
        {
            std::ofstream out(motdIniPath, std::ios::binary);
            out << "[INIT]\r\nNumLines=3\r\n\r\n";
            out << "[Motd]\r\n";
            out << "Line1=Bienvenidos a Argentum Online C++\r\n";
            out << "Line2=Disfruten del juego sin lag.\r\n";
            out << "Line3=Reglas: No hacer trampas.\r\n";
            out.close();

            FileIO::LoadMotd();
            CHECK(MaxLines == 3);
            REQUIRE(MOTD.size() >= 4);
            CHECK(MOTD[1].texto == "Bienvenidos a Argentum Online C++");
            CHECK(MOTD[2].texto == "Disfruten del juego sin lag.");
            CHECK(MOTD[3].texto == "Reglas: No hacer trampas.");
        }

        DatPath.clear();
        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("Carga Global con LoadSini (Server.ini y Ciudades.dat)") {
        std::string testDir = "tests/temp_g3_sini/";
        std::filesystem::create_directories(testDir + "Dat/");

        IniPath = testDir;
        DatPath = testDir + "Dat/";

        std::ofstream serverOut(testDir + "Server.ini", std::ios::binary);
        serverOut << "[INIT]\r\n";
        serverOut << "StartPort=7666\r\n";
        serverOut << "Hide=1\r\n";
        serverOut << "AllowMultiLogins=1\r\n";
        serverOut << "IdleLimit=5\r\n";
        serverOut << "Version=0.13.0\r\n";
        serverOut << "Testing=1\r\n";
        serverOut << "Record=150\r\n";
        serverOut << "MaxUsers=100\r\n";
        serverOut << "\r\n[INTERVALOS]\r\n";
        serverOut << "SanaIntervaloSinDescansar=1600\r\n";
        serverOut << "IntervaloParalizado=500\r\n";
        serverOut.close();

        std::ofstream ciudadesOut(testDir + "Dat/Ciudades.dat", std::ios::binary);
        ciudadesOut << "[Ullathorpe]\r\nMapa=1\r\nX=50\r\nY=50\r\n";
        ciudadesOut << "[Nix]\r\nMapa=34\r\nX=60\r\nY=40\r\n";
        ciudadesOut.close();

        FileIO::LoadSini();

        CHECK(Puerto == 7666);
        CHECK(HideMe == 1);
        CHECK(AllowMultiLogins == 1);
        CHECK(IdleLimit == 5);
        CHECK(ULTIMAVERSION == "0.13.0");
        CHECK(EnTesting == true);
        CHECK(recordusuarios == 150);
        CHECK(SanaIntervaloSinDescansar == 1600);
        CHECK(IntervaloParalizado == 500);

        CHECK(Ullathorpe.Map == 1);
        CHECK(Ullathorpe.X == 50);
        CHECK(Ullathorpe.Y == 50);

        CHECK(Nix.Map == 34);
        CHECK(Nix.X == 60);
        CHECK(Nix.Y == 40);

        IniPath.clear();
        DatPath.clear();
        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("Validación contra Server.ini y Ciudades.Dat REALES de producción (tests/fixtures/serverconfig/real/)") {
        IniPath = "tests/fixtures/serverconfig/real/";
        DatPath = "tests/fixtures/serverconfig/real/";

        // 1. Verificación de roles administrativos reales en Server.ini
        // En el archivo de producción: Dioses=1 (Elio), SemiDioses=0, Consejeros=0, RolesMasters=0
        CHECK(FileIO::EsDios("Elio"));
        CHECK(FileIO::EsDios("ELIO"));
        CHECK_FALSE(FileIO::EsDios("BetaTester")); // Dioses=1 restringe la búsqueda al primer slot (Dios1)
        CHECK_FALSE(FileIO::EsSemiDios("RESISTENCIA")); // SemiDioses=0 en producción
        CHECK_FALSE(FileIO::EsConsejero("zaveth"));     // Consejeros=0 en producción
        CHECK_FALSE(FileIO::EsRolesMaster("ELVIO GANNET")); // RolesMasters=0 en producción

        CHECK_FALSE(FileIO::EsDios("UsuarioAleatorio"));
        CHECK_FALSE(FileIO::EsAdmin("Elio")); // No jerárquico: Elio está en [Dioses], no en [Admines]

        // 2. Verificación de carga de configuración global con LoadSini
        FileIO::LoadSini();

        CHECK(Puerto == 7666);
        CHECK(IdleLimit == 5);
        CHECK(ULTIMAVERSION == "0.13.0");
        CHECK(ArmaduraImperial1 == 370);
        CHECK(ArmaduraCaos1 == 379);

        // Coordenadas reales de Ciudades.Dat (Ullathorpe: 1, Nix: 34, Banderbill: 59, Lindos: 62, Arghal: 196)
        CHECK(Ullathorpe.Map == 1);
        CHECK(Nix.Map == 34);
        CHECK(Banderbill.Map == 59);
        CHECK(Lindos.Map == 62);
        CHECK(Arghal.Map == 196);

        IniPath.clear();
        DatPath.clear();
    }

    TEST_CASE("Validación de LoadMotd contra Motd.ini REAL de producción") {
        DatPath = "tests/fixtures/serverconfig/real/";

        FileIO::LoadMotd();

        CHECK(MaxLines == 2);
        REQUIRE(MOTD.size() > 2);
        CHECK(MOTD[1].texto.find("Bienvenidos a la") != std::string::npos);
        CHECK(MOTD[1].texto.find("Argentum Online") != std::string::npos);
        CHECK(MOTD[2].texto.find("gs-zone.org") != std::string::npos);

        DatPath.clear();
    }
}
