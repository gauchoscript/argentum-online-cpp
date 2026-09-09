#include <doctest/doctest.h>
#include "server/FileIO.hpp"
#include "server/Declares.hpp"
#include <filesystem>
#include <fstream>
#include <string>

TEST_SUITE("FileIO_Grupo7_AdminLogs") {

    TEST_CASE("LogBan escribe en BanDetail.log y apendea en GenteBanned.log") {
        std::string testDir = "tests/temp_g7_logs/";
        std::filesystem::create_directories(testDir);
        FileIO::LogsPath = testDir;

        // Configurar usuarios
        UserList.resize(10);
        UserList[1].name = "BetaTester";
        UserList[2].name = "DiosElio";

        FileIO::LogBan(1, 2, "Uso de macros no permitidos");

        // 1. Validar BanDetail.log (formato INI estructurado)
        std::string logIni = testDir + "BanDetail.log";
        CHECK(FileIO::GetVar(logIni, "BetaTester", "BannedBy") == "DiosElio");
        CHECK(FileIO::GetVar(logIni, "BetaTester", "Reason") == "Uso de macros no permitidos");

        // 2. Validar GenteBanned.log (formato plano append)
        std::string listLog = testDir + "GenteBanned.log";
        REQUIRE(std::filesystem::exists(listLog));
        {
            std::ifstream ifs(listLog, std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            CHECK(content.find("BetaTester") != std::string::npos);
        }

        // 3. Caso de borde: índices fuera de rango no deben provocar caídas
        CHECK_NOTHROW(FileIO::LogBan(0, 2, "Invalido"));
        CHECK_NOTHROW(FileIO::LogBan(1, 999, "Invalido"));

        std::filesystem::remove_all(testDir);
        FileIO::LogsPath.clear();
    }

    TEST_CASE("LogBanFromName escribe en BanDetail.dat (quirk legacy) y GenteBanned.log") {
        std::string testDir = "tests/temp_g7_logs_name/";
        std::filesystem::create_directories(testDir);
        FileIO::LogsPath = testDir;

        UserList.resize(10);
        UserList[3].name = "Consejero";

        FileIO::LogBanFromName("UsuarioDesconectado", 3, "Insultos masivos");

        // Validar que se crea BanDetail.dat (quirk histórico de FileIO.bas:2158)
        std::string datIni = testDir + "BanDetail.dat";
        CHECK(FileIO::GetVar(datIni, "UsuarioDesconectado", "BannedBy") == "Consejero");
        CHECK(FileIO::GetVar(datIni, "UsuarioDesconectado", "Reason") == "Insultos masivos");

        // Validar lista plana
        std::string listLog = testDir + "GenteBanned.log";
        REQUIRE(std::filesystem::exists(listLog));
        {
            std::ifstream ifs(listLog, std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            CHECK(content.find("UsuarioDesconectado") != std::string::npos);
        }

        // Caso de borde: nombre vacío
        CHECK_NOTHROW(FileIO::LogBanFromName("", 3, "Test"));

        std::filesystem::remove_all(testDir);
        FileIO::LogsPath.clear();
    }

    TEST_CASE("Ban escribe en BanDetail.dat con baneador textual") {
        std::string testDir = "tests/temp_g7_ban/";
        std::filesystem::create_directories(testDir);
        FileIO::LogsPath = testDir;

        FileIO::Ban("ClonIlegal", "SistemaAntiCheat", "Deteccion automatica de velocidad");

        std::string datIni = testDir + "BanDetail.dat";
        CHECK(FileIO::GetVar(datIni, "ClonIlegal", "BannedBy") == "SistemaAntiCheat");
        CHECK(FileIO::GetVar(datIni, "ClonIlegal", "Reason") == "Deteccion automatica de velocidad");

        std::string listLog = testDir + "GenteBanned.log";
        REQUIRE(std::filesystem::exists(listLog));
        {
            std::ifstream ifs(listLog, std::ios::binary);
            std::string content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
            CHECK(content.find("ClonIlegal") != std::string::npos);
        }

        std::filesystem::remove_all(testDir);
        FileIO::LogsPath.clear();
    }
}