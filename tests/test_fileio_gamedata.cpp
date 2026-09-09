#include <doctest/doctest.h>
#include "server/FileIO.hpp"
#include "server/Declares.hpp"
#include <filesystem>
#include <fstream>

TEST_SUITE("FileIO_GameData") {

    TEST_CASE("CargarSpawnList desd Invokar.dat") {
        std::string testDir = "tests/temp_g5_spawn/";
        std::filesystem::create_directories(testDir);
        DatPath = testDir;

        std::ofstream out(testDir + "Invokar.dat", std::ios::binary);
        out << "[INIT]\r\n";
        out << "NumNpcs=2\r\n";
        out << "[NPC1]\r\n";
        out << "NpcIndex=50\r\n";
        out << "NpcName=Dragon\r\n";
        out << "[NPC2]\r\n";
        out << "NpcIndex=102\r\n";
        out << "NpcName=Goblin\r\n";
        out.close();

        FileIO::CargarSpawnList();

        REQUIRE(SpawnList.size() >= 3);
        CHECK(SpawnList[1].NpcIndex == 50);
        CHECK(SpawnList[1].NpcName == "Dragon");
        CHECK(SpawnList[2].NpcIndex == 102);
        CHECK(SpawnList[2].NpcName == "Goblin");

        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("CargarForbidenWords desde NombresInvalidos.txt") {
        std::string testDir = "tests/temp_g5_words/";
        std::filesystem::create_directories(testDir);
        DatPath = testDir;

        std::ofstream out(testDir + "NombresInvalidos.txt", std::ios::binary);
        out << "ADMIN\r\nGOD\r\nPUTO\r\n";
        out.close();

        FileIO::CargarForbidenWords();

        REQUIRE(ForbidenNames.size() >= 4);
        CHECK(ForbidenNames[1] == "ADMIN");
        CHECK(ForbidenNames[2] == "GOD");
        CHECK(ForbidenNames[3] == "PUTO");

        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("CargarHechizos desde Hechizos.dat") {
        std::string testDir = "tests/temp_g5_hechizos/";
        std::filesystem::create_directories(testDir);
        DatPath = testDir;

        std::ofstream out(testDir + "Hechizos.dat", std::ios::binary);
        out << "[INIT]\r\n";
        out << "NumeroHechizos=1\r\n";
        out << "[HECHIZO1]\r\n";
        out << "Nombre=Apocalipsis\r\n";
        out << "Desc=Destruccion masiva\r\n";
        out << "PalabrasMagicas=Tenebrae\r\n";
        out << "Tipo=1\r\n";
        out << "WAV=5\r\n";
        out << "ManaRequerido=800\r\n";
        out.close();

        FileIO::CargarHechizos();

        CHECK(NumeroHechizos == 1);
        REQUIRE(Hechizos.size() >= 2);
        CHECK(Hechizos[1].Nombre == "Apocalipsis");
        CHECK(Hechizos[1].desc == "Destruccion masiva");
        CHECK(Hechizos[1].PalabrasMagicas == "Tenebrae");
        CHECK(Hechizos[1].WAV == 5);
        CHECK(Hechizos[1].ManaRequerido == 800);

        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("LoadArmasHerreria y LoadArmadurasHerreria") {
        std::string testDir = "tests/temp_g5_herreria/";
        std::filesystem::create_directories(testDir);
        DatPath = testDir;

        std::ofstream outArmas(testDir + "ArmasHerrero.dat", std::ios::binary);
        outArmas << "[INIT]\r\nNumArmas=2\r\n[Arma1]\r\nIndex=15\r\n[Arma2]\r\nIndex=20\r\n";
        outArmas.close();

        std::ofstream outArmaduras(testDir + "ArmadurasHerrero.dat", std::ios::binary);
        outArmaduras << "[INIT]\r\nNumArmaduras=1\r\n[Armadura1]\r\nIndex=101\r\n";
        outArmaduras.close();

        FileIO::LoadArmasHerreria();
        FileIO::LoadArmadurasHerreria();

        REQUIRE(ArmasHerrero.size() >= 3);
        CHECK(ArmasHerrero[1] == 15);
        CHECK(ArmasHerrero[2] == 20);

        REQUIRE(ArmadurasHerrero.size() >= 2);
        CHECK(ArmadurasHerrero[1] == 101);

        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("LoadBalance desde Balance.dat") {
        std::string testDir = "tests/temp_g5_balance/";
        std::filesystem::create_directories(testDir);
        DatPath = testDir;

        ListaClases[1] = "Mago";
        ListaRazas[1] = "Humano";

        std::ofstream out(testDir + "Balance.dat", std::ios::binary);
        out << "[MODEVASION]\r\nMago=1.2\r\n";
        out << "[MODRAZA]\r\nHumanoFuerza=10\r\nHumanoAgilidad=12\r\n";
        out << "[MODVIDA]\r\nMago=8.5\r\n";
        out << "[DISTRIBUCION]\r\nE1=2\r\nS1=4\r\n";
        out << "[EXTRA]\r\nPorcentajeRecuperoMana=15\r\n";
        out << "[PARTY]\r\nExponenteNivelParty=1.5\r\n";
        out << "[RECOMPENSAFACCION]\r\nRango1=500\r\n";
        out.close();

        FileIO::LoadBalance();

        CHECK(ModClaseList[1].Evasion == doctest::Approx(1.2));
        CHECK(ModRazaList[1].Fuerza == doctest::Approx(10.0f));
        CHECK(ModRazaList[1].Agilidad == doctest::Approx(12.0f));
        CHECK(ModVida[1] == doctest::Approx(8.5));
        CHECK(DistribucionEnteraVida[1] == 2);
        CHECK(DistribucionSemienteraVida[1] == 4);
        CHECK(PorcentajeRecuperoMana == 15);
        CHECK(ExponenteNivelParty == doctest::Approx(1.5f));
        CHECK(RecompensaFacciones[0] == 500);

        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("LoadObjCarpintero desde ObjCarpintero.dat") {
        std::string testDir = "tests/temp_g5_carpintero/";
        std::filesystem::create_directories(testDir);
        DatPath = testDir;

        std::ofstream out(testDir + "ObjCarpintero.dat", std::ios::binary);
        out << "[INIT]\r\nNumObjs=1\r\n[Obj1]\r\nIndex=300\r\n";
        out.close();

        FileIO::LoadObjCarpintero();

        REQUIRE(ObjCarpintero.size() >= 2);
        CHECK(ObjCarpintero[1] == 300);

        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("LoadOBJData desde Obj.dat") {
        std::string testDir = "tests/temp_g5_objdata/";
        std::filesystem::create_directories(testDir);
        DatPath = testDir;

        std::ofstream out(testDir + "Obj.dat", std::ios::binary);
        out << "[INIT]\r\nNumObjs=1\r\n";
        out << "[OBJ1]\r\nName=Espada Corta\r\nObjType=3\r\nGrhIndex=12\r\nMINDEF=5\r\nMAXDEF=15\r\nValor=100\r\n";
        out.close();

        FileIO::LoadOBJData();

        CHECK(NumObjDatas == 1);
        REQUIRE(ObjDataList.size() >= 2);
        CHECK(ObjDataList[1].name == "Espada Corta");
        CHECK(static_cast<int>(ObjDataList[1].OBJType) == 3);
        CHECK(ObjDataList[1].GrhIndex == 12);
        CHECK(ObjDataList[1].MinDef == 5);
        CHECK(ObjDataList[1].MaxDef == 15);
        CHECK(ObjDataList[1].def == doctest::Approx(10.0f));
        CHECK(ObjDataList[1].Valor == 100);

        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("LoadOBJData: Mapeo de ClaseProhibida (Válida, Vacía, Cadena Inexistente/Typo)") {
        std::string testDir = "tests/temp_g5_cp_edge/";
        std::filesystem::create_directories(testDir);
        DatPath = testDir;

        ListaClases[1] = "Mago";
        ListaClases[2] = "Clerigo";
        ListaClases[3] = "Guerrero";

        std::ofstream out(testDir + "Obj.dat", std::ios::binary);
        out << "[INIT]\r\nNumObjs=1\r\n";
        out << "[OBJ1]\r\nName=Tunica Mágica\r\nObjType=2\r\n";
        out << "CP1=Guerrero\r\n";       // Clase válida -> Mapea a 3 (Guerrero)
        out << "CP2=\r\n";               // Cadena vacía -> Mapea a 0
        out << "CP3=ClaseInexistente\r\n"; // Cadena inexistente -> Mapea a 0 (sin error fuera de rango)
        out.close();

        FileIO::LoadOBJData();

        REQUIRE(ObjDataList.size() >= 2);
        CHECK(static_cast<int>(ObjDataList[1].ClaseProhibida[1]) == 3);
        CHECK(static_cast<int>(ObjDataList[1].ClaseProhibida[2]) == 0);
        CHECK(static_cast<int>(ObjDataList[1].ClaseProhibida[3]) == 0);

        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("CargaApuestas desde apuestas.dat") {
        std::string testDir = "tests/temp_g5_apuestas/";
        std::filesystem::create_directories(testDir);
        DatPath = testDir;

        std::ofstream out(testDir + "apuestas.dat", std::ios::binary);
        out << "[Main]\r\nGanancias=5000\r\nPerdidas=2000\r\nJugadas=15\r\n";
        out.close();

        FileIO::CargaApuestas();

        CHECK(Apuestas.Ganancias == 5000);
        CHECK(Apuestas.Perdidas == 2000);
        CHECK(Apuestas.Jugadas == 15);

        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("LoadArmadurasFaccion desde ArmadurasFaccionarias.dat") {
        std::string testDir = "tests/temp_g5_armfacc/";
        std::filesystem::create_directories(testDir);
        DatPath = testDir;

        std::ofstream out(testDir + "ArmadurasFaccionarias.dat", std::ios::binary);
        out << "[CLASE1]\r\nDefMinArmyAlto=50\r\nDefMinArmyBajo=40\r\nDefMinCaosAlto=60\r\nDefMinCaosBajo=35\r\n";
        out.close();

        FileIO::LoadArmadurasFaccion();

        CHECK(ArmadurasFaccion[1][static_cast<size_t>(eRaza::Humano)].Armada[static_cast<size_t>(eTipoDefArmors::ieBaja)] == 50);
        CHECK(ArmadurasFaccion[1][static_cast<size_t>(eRaza::Enano)].Armada[static_cast<size_t>(eTipoDefArmors::ieBaja)] == 40);
        CHECK(ArmadurasFaccion[1][static_cast<size_t>(eRaza::Drow)].Caos[static_cast<size_t>(eTipoDefArmors::ieBaja)] == 60);

        std::filesystem::remove_all(testDir);
    }

    TEST_CASE("Validación contra Tablas de Datos REALES de producción (tests/fixtures/gamedata/real/)") {
        DatPath = "tests/fixtures/gamedata/real/";

        // 1. Validar LoadOBJData contra obj.dat real
        FileIO::LoadOBJData();

        CHECK(NumObjDatas == 1052);
        REQUIRE(ObjDataList.size() > 1052);
        CHECK(ObjDataList[1].name == "Manzana Roja");
        CHECK(ObjDataList[1].OBJType == eOBJType::otUseOnce);
        CHECK(ObjDataList[2].name == "Espada Larga");
        CHECK(ObjDataList[2].OBJType == eOBJType::otWeapon);
        CHECK(ObjDataList[15].name == "Daga");
        CHECK(ObjDataList[15].OBJType == eOBJType::otWeapon);
        CHECK(ObjDataList[63].name == "Fogata");
        CHECK(ObjDataList[63].OBJType == eOBJType::otFogata);

        // 2. Validar CargarHechizos contra Hechizos.dat real
        FileIO::CargarHechizos();

        CHECK(NumeroHechizos == 46);
        REQUIRE(Hechizos.size() > 46);
        CHECK(!Hechizos[1].Nombre.empty());
        CHECK(Hechizos[1].PalabrasMagicas == "NIHIL VED");
        CHECK(Hechizos[1].MinSkill == 10);
        CHECK(Hechizos[1].ManaRequerido == 12);

        // 3. Validar LoadBalance contra Balance.dat real
        ListaRazas[static_cast<size_t>(eRaza::Humano)] = "Humano";
        ListaRazas[static_cast<size_t>(eRaza::Elfo)] = "Elfo";
        ListaRazas[static_cast<size_t>(eRaza::Drow)] = "Drow";
        ListaRazas[static_cast<size_t>(eRaza::Gnomo)] = "Gnomo";
        ListaRazas[static_cast<size_t>(eRaza::Enano)] = "Enano";

        ListaClases[static_cast<size_t>(eClass::Mage)] = "Mago";
        ListaClases[static_cast<size_t>(eClass::Cleric)] = "Clerigo";
        ListaClases[static_cast<size_t>(eClass::Warrior)] = "Guerrero";
        ListaClases[static_cast<size_t>(eClass::Assasin)] = "Asesino";
        ListaClases[static_cast<size_t>(eClass::Thief)] = "Ladron";
        ListaClases[static_cast<size_t>(eClass::Bard)] = "Bardo";
        ListaClases[static_cast<size_t>(eClass::Druid)] = "Druida";
        ListaClases[static_cast<size_t>(eClass::Bandit)] = "Bandido";
        ListaClases[static_cast<size_t>(eClass::Paladin)] = "Paladin";
        ListaClases[static_cast<size_t>(eClass::Hunter)] = "Cazador";
        ListaClases[static_cast<size_t>(eClass::Worker)] = "Trabajador";
        ListaClases[static_cast<size_t>(eClass::Pirat)] = "Pirata";

        FileIO::LoadBalance();

        CHECK(ModRazaList[static_cast<size_t>(eRaza::Humano)].Fuerza == 1);
        CHECK(ModRazaList[static_cast<size_t>(eRaza::Humano)].Constitucion == 2);
        CHECK(ModRazaList[static_cast<size_t>(eRaza::Elfo)].Agilidad == 3);
        CHECK(ModClaseList[static_cast<size_t>(eClass::Warrior)].Evasion == 1.0);
        CHECK(ModClaseList[static_cast<size_t>(eClass::Hunter)].Evasion == 0.9);

        // 4. Validar cargas complementarias contra tablas reales
        FileIO::CargarSpawnList();
        CHECK(SpawnList.size() > 1);

        FileIO::CargarForbidenWords();
        CHECK(ForbidenNames.size() > 1);

        FileIO::LoadArmasHerreria();
        CHECK(ArmasHerrero.size() > 1);

        FileIO::LoadArmadurasHerreria();
        CHECK(ArmadurasHerrero.size() > 1);

        FileIO::LoadObjCarpintero();
        CHECK(ObjCarpintero.size() > 1);

        FileIO::CargaApuestas();
        // apuestas.dat real: Ganancias=0, Perdidas=0, Jugadas=0
        CHECK(Apuestas.Ganancias >= 0);

        FileIO::LoadArmadurasFaccion();

        DatPath.clear();
    }
}
