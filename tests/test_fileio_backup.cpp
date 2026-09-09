#include <doctest/doctest.h>
#include "server/FileIO.hpp"
#include "server/Declares.hpp"
#include <filesystem>
#include <fstream>
#include <string>

TEST_SUITE("FileIO_Grupo6_WorldBackup") {

    TEST_CASE("BackUPnPc y CargarNpcBackUp: persistencia y restauraciÃ³n roundtrip de NPC") {
        std::string testDir = "tests/temp_g6_npcbackup/";
        std::filesystem::create_directories(testDir);
        DatPath = testDir;

        // Configurar NPC de prueba en slot 5
        int16_t npcSlot = 5;
        int16_t npcNumero = 502;

        Npclist[npcSlot] = npc{};
        auto& n = Npclist[npcSlot];
        n.Numero = npcNumero;
        n.name = "Criatura de Prueba";
        n.desc = "Un temible monstruo de las cavernas";
        n.Movement = TipoAI::NPCMAmbula;
        n.NPCtype = eNPCType::Comun;
        n.char_appearance.body = 120;
        n.char_appearance.Head = 50;
        n.char_appearance.heading = eHeading::SOUTH;
        n.Attackable = 1;
        n.Comercia = 0;
        n.Hostile = 1;
        n.GiveEXP = 2500;
        n.GiveGLD = 1200;
        n.InvReSpawn = 1;
        n.Stats.MaxHp = 800;
        n.Stats.MinHp = 750;
        n.Stats.MaxHIT = 60;
        n.Stats.MinHIT = 40;
        n.Stats.def = 35;
        n.Stats.Alineacion = 2;

        n.flags.Respawn = 1;
        n.flags.BackUp = 1;
        n.flags.Domable = 100;
        n.flags.RespawnOrigPos = 1;
        n.TipoItems = 3;

        n.Invent.NroItems = 2;
        n.Invent.Object[1].ObjIndex = 15;
        n.Invent.Object[1].Amount = 2;
        n.Invent.Object[2].ObjIndex = 2;
        n.Invent.Object[2].Amount = 1;

        n.Drop[1].ObjIndex = 63;
        n.Drop[1].Amount = 5;

        // 1. Guardar con BackUPnPc
        FileIO::BackUPnPc(npcSlot);

        std::string bkFile = testDir + "bkNPCs.dat";
        REQUIRE(std::filesystem::exists(bkFile));
        CHECK(FileIO::GetVar(bkFile, "NPC502", "Name") == "Criatura de Prueba");
        CHECK(FileIO::GetVar(bkFile, "NPC502", "MaxHp") == "800");
        CHECK(FileIO::GetVar(bkFile, "NPC502", "Obj1") == "15-2");

        // 2. Limpiar NPC en memoria
        Npclist[npcSlot] = npc{};

        // 3. Restaurar con CargarNpcBackUp
        FileIO::CargarNpcBackUp(npcSlot, npcNumero);

        CHECK(Npclist[npcSlot].Numero == 502);
        CHECK(Npclist[npcSlot].name == "Criatura de Prueba");
        CHECK(Npclist[npcSlot].desc == "Un temible monstruo de las cavernas");
        CHECK(Npclist[npcSlot].char_appearance.body == 120);
        CHECK(Npclist[npcSlot].char_appearance.Head == 50);
        CHECK(Npclist[npcSlot].char_appearance.heading == eHeading::SOUTH);
        CHECK(Npclist[npcSlot].Stats.MaxHp == 800);
        CHECK(Npclist[npcSlot].Stats.MinHp == 750);
        CHECK(Npclist[npcSlot].Stats.def == 35);
        CHECK(Npclist[npcSlot].Stats.Alineacion == 2);
        CHECK(Npclist[npcSlot].flags.BackUp == 1);
        CHECK(Npclist[npcSlot].flags.NPCActive == true);
        CHECK(Npclist[npcSlot].Invent.NroItems == 2);
        CHECK(Npclist[npcSlot].Invent.Object[1].ObjIndex == 15);
        CHECK(Npclist[npcSlot].Invent.Object[1].Amount == 2);
        // Quirk legacy: BackUPnPc solo serializa Invent.Object, no persiste Drop
        CHECK(Npclist[npcSlot].Drop[1].ObjIndex == 0);

        Npclist[npcSlot] = npc{};
        std::filesystem::remove_all(testDir);
        DatPath.clear();
    }

    TEST_CASE("CargarBackUp: restaura mapas respaldados y aplica fallback a mapas base") {
        std::string testDir = "tests/temp_g6_loadbackup/";
        std::filesystem::create_directories(testDir);
        DatPath = testDir;

        // Crear Map.dat apuntando a los fixtures autoritativos
        std::ofstream mapDat(testDir + "Map.dat", std::ios::binary);
        mapDat << "[INIT]\r\nNumMaps=1\r\nMapPath=tests/fixtures/maps/\r\n";
        mapDat.close();

        // Configurar WorldBackupPath apuntando a los respaldos reales
        FileIO::WorldBackupPath = "tests/fixtures/worldbackup/real/";

        FileIO::CargarBackUp();

        CHECK(NumMaps == 1);
        REQUIRE(MapInfoList.size() > 1);
        // Mapa 1 proviene de tests/fixtures/worldbackup/real/Mapa1
        CHECK(MapInfoList[1].MapVersion > 0);
        CHECK(!MapInfoList[1].name.empty());

        std::filesystem::remove_all(testDir);
        DatPath.clear();
        FileIO::WorldBackupPath.clear();
    }

    TEST_CASE("DoBackUp: orquestaciÃ³n de respaldo de mapas, NPCs y log, sin tocar .chr de personajes") {
        std::string testDir = "tests/temp_g6_dobackup/";
        std::string bkDir = testDir + "WorldBackUp/";
        std::string datDir = testDir + "Dat/";
        std::string logsDir = testDir + "logs/";
        std::string charsDir = testDir + "Charfile/";

        std::filesystem::create_directories(bkDir);
        std::filesystem::create_directories(datDir);
        std::filesystem::create_directories(logsDir);
        std::filesystem::create_directories(charsDir);

        DatPath = datDir;
        FileIO::WorldBackupPath = bkDir;
        FileIO::LogsPath = logsDir;
        CharPath = charsDir;

        // Cargar un mapa real en memoria (Mapa 1) para respaldarlo
        FileIO::CargarMapa(1, "tests/fixtures/maps/Mapa1");
        NumMaps = 1;
        MapInfoList[1].BackUp = 1; // Marcar para backup

        // Configurar NPC con BackUp = 1
        LastNPC = 1;
        Npclist[1].Numero = 536;
        Npclist[1].name = "Guardia Imperial";
        Npclist[1].flags.BackUp = 1;

        // Crear un archivo .chr ficticio para verificar que DoBackUp NO lo toca ni sobreescribe
        std::string dummyChr = charsDir + "TESTUSER.chr";
        {
            std::ofstream chrOut(dummyChr);
            chrOut << "[INIT]\r\nName=TESTUSER\r\nOriginal=1\r\n";
        }
        auto chrModTimeBefore = std::filesystem::last_write_time(dummyChr);

        CHECK(haciendoBK == false);

        // Ejecutar DoBackUp
        FileIO::DoBackUp();

        // Verificar bandera de backup restablecida a false
        CHECK(haciendoBK == false);

        // 1. Validar que se creÃ³ el respaldo de mapa en WorldBackUp/
        CHECK(std::filesystem::exists(bkDir + "Mapa1.map"));
        CHECK(std::filesystem::exists(bkDir + "Mapa1.inf"));
        CHECK(std::filesystem::file_size(bkDir + "Mapa1.map") > 0);

        // 2. Validar que se creÃ³ bkNPCs.dat con el NPC marcado
        CHECK(std::filesystem::exists(datDir + "bkNPCs.dat"));
        CHECK(FileIO::GetVar(datDir + "bkNPCs.dat", "NPC536", "Name") == "Guardia Imperial");

        // 3. Validar que se escribió la línea de tiempo en BackUps.log
        std::string logFile = logsDir + "BackUps.log";
        REQUIRE(std::filesystem::exists(logFile));
        CHECK(!LastBackup.empty());
        {
            std::ifstream logIfs(logFile);
            std::string logContent((std::istreambuf_iterator<char>(logIfs)), std::istreambuf_iterator<char>());
            CHECK(logContent.find(LastBackup) != std::string::npos);
        }

        // 4. Verificación de regla de negocio: DoBackUp NO guarda archivos .chr de personajes
        auto chrModTimeAfter = std::filesystem::last_write_time(dummyChr);
        CHECK(chrModTimeBefore == chrModTimeAfter);

        std::filesystem::remove_all(testDir);
        DatPath.clear();
        FileIO::WorldBackupPath.clear();
        FileIO::LogsPath.clear();
        CharPath.clear();
        Npclist[1] = npc{};
        LastNPC = 0;
        MapData.clear();
        MapInfoList.clear();
    }
}