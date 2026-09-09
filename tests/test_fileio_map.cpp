#include <doctest/doctest.h>
#include "server/FileIO.hpp"
#include "server/Declares.hpp"
#include <filesystem>
#include <fstream>
#include <vector>
#include <string>

// Helper para leer un archivo completo en un vector de bytes
static std::vector<uint8_t> ReadBinaryFile(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) return {};
    return std::vector<uint8_t>((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
}

TEST_SUITE("FileIO_Mapas") {

    TEST_CASE("Carga y deserialización de mapas reales: Mapa 1 (Ullathorpe) con 4 capas, triggers y NPCs") {
        std::string mapPath = "tests/fixtures/maps/Mapa1";
        FileIO::CargarMapa(1, mapPath);

        // 1. Verificación de metadatos del archivo .dat
        REQUIRE(MapInfoList.size() > 1);
        CHECK(MapInfoList[1].name == "Ciudad de Ullathorpe");
        CHECK(MapInfoList[1].Music == "4");
        CHECK(MapInfoList[1].Terreno == "BOSQUE");
        CHECK(MapInfoList[1].Zona == "CIUDAD");
        CHECK(MapInfoList[1].Pk == false); // Pk=1 en .dat significa zona segura / no PK

        // 2. Consumo exacto de bytes comprobado contra el archivo original
        auto origMapBytes = ReadBinaryFile(mapPath + ".map");
        auto origInfBytes = ReadBinaryFile(mapPath + ".inf");
        REQUIRE(origMapBytes.size() == 33183);
        REQUIRE(origInfBytes.size() == 12782);

        // 3. Aserciones de tiles específicos analizados empíricamente
        size_t idx40_30 = (static_cast<size_t>(1) * 101 + 40) * 101 + 30;
        CHECK(MapData[idx40_30].Blocked == 0);
        CHECK(MapData[idx40_30].Graphic[1] == 5500);
        CHECK(MapData[idx40_30].Graphic[4] == 5601);
        CHECK(MapData[idx40_30].trigger == eTrigger::BAJOTECHO);

        size_t idx50_26 = (static_cast<size_t>(1) * 101 + 50) * 101 + 26;
        CHECK(MapData[idx50_26].Blocked == 1);
        CHECK(MapData[idx50_26].Graphic[1] == 6005);
        CHECK(MapData[idx50_26].trigger == eTrigger::BAJOTECHO);

        size_t idx73_16 = (static_cast<size_t>(1) * 101 + 73) * 101 + 16;
        CHECK(MapData[idx73_16].NpcIndex == 536);
    }

    TEST_CASE("Carga de mapas reales con diferentes perfiles: Mapa 4, Mapa 8, Mapa 15") {
        // Mapa 4: Bosque con triggers y capa 4
        std::string mapPath4 = "tests/fixtures/maps/Mapa4";
        FileIO::CargarMapa(4, mapPath4);
        REQUIRE(MapInfoList.size() > 4);
        CHECK_FALSE(MapInfoList[4].name.empty());

        auto bytesMap4 = ReadBinaryFile(mapPath4 + ".map");
        auto bytesInf4 = ReadBinaryFile(mapPath4 + ".inf");
        CHECK(bytesMap4.size() == 30879);
        CHECK(bytesInf4.size() == 12358);

        size_t idx55_20 = (static_cast<size_t>(4) * 101 + 55) * 101 + 20;
        CHECK(MapData[idx55_20].NpcIndex == 502);

        size_t idx20_27 = (static_cast<size_t>(4) * 101 + 20) * 101 + 27;
        CHECK(MapData[idx20_27].Blocked == 0);
        CHECK(MapData[idx20_27].Graphic[1] == 6012);
        CHECK(MapData[idx20_27].trigger == eTrigger::BAJOTECHO);

        // Mapa 8: Baseline sparse
        std::string mapPath8 = "tests/fixtures/maps/Mapa8";
        FileIO::CargarMapa(8, mapPath8);
        auto bytesMap8 = ReadBinaryFile(mapPath8 + ".map");
        auto bytesInf8 = ReadBinaryFile(mapPath8 + ".inf");
        CHECK(bytesMap8.size() == 30433);
        CHECK(bytesInf8.size() == 12390);

        size_t idx50_50_8 = (static_cast<size_t>(8) * 101 + 50) * 101 + 50;
        CHECK(MapData[idx50_50_8].Blocked == 0);
        CHECK(MapData[idx50_50_8].Graphic[1] == 6010);
        CHECK(MapData[idx50_50_8].Graphic[2] == 0);
        CHECK(MapData[idx50_50_8].Graphic[3] == 0);
        CHECK(MapData[idx50_50_8].Graphic[4] == 0);
        CHECK(MapData[idx50_50_8].trigger == eTrigger::NADA);

        // Mapa 15: Dungeon con capa 2 pero sin L3 ni L4 ni triggers
        std::string mapPath15 = "tests/fixtures/maps/Mapa15";
        FileIO::CargarMapa(15, mapPath15);
        auto bytesMap15 = ReadBinaryFile(mapPath15 + ".map");
        auto bytesInf15 = ReadBinaryFile(mapPath15 + ".inf");
        CHECK(bytesMap15.size() == 30843);
        CHECK(bytesInf15.size() == 12074);

        size_t idx9_1_15 = (static_cast<size_t>(15) * 101 + 9) * 101 + 1;
        CHECK(MapData[idx9_1_15].Graphic[1] == 1505);
        CHECK(MapData[idx9_1_15].Graphic[2] == 7307);

        size_t idx14_10_15 = (static_cast<size_t>(15) * 101 + 14) * 101 + 10;
        CHECK(MapData[idx14_10_15].NpcIndex == 519);
    }

    TEST_CASE("Roundtrip binario byte-exacto para los 4 mapas reales (GrabarMapa -> binary diff)") {
        std::string tempDir = "tests/temp_maps_roundtrip/";
        std::filesystem::create_directories(tempDir);

        std::vector<int> testMaps = {1, 4, 8, 15};

        for (int mapId : testMaps) {
            std::string origPath = "tests/fixtures/maps/Mapa" + std::to_string(mapId);
            std::string tempPath = tempDir + "Mapa" + std::to_string(mapId);

            // 1. Cargar desde original
            FileIO::CargarMapa(mapId, origPath);

            // 2. Grabar a directorio temporal
            FileIO::GrabarMapa(mapId, tempPath);

            // 3. Comparar .map byte a byte
            auto origMap = ReadBinaryFile(origPath + ".map");
            auto savedMap = ReadBinaryFile(tempPath + ".map");
            REQUIRE(savedMap.size() == origMap.size());
            CHECK(savedMap == origMap);

            // 4. Comparar .inf byte a byte
            auto origInf = ReadBinaryFile(origPath + ".inf");
            auto savedInf = ReadBinaryFile(tempPath + ".inf");
            REQUIRE(savedInf.size() == origInf.size());
            CHECK(savedInf == origInf);

            // 5. Verificar que el archivo .dat grabado retiene metadatos
            std::string savedName = FileIO::GetVar(tempPath + ".dat", "Mapa" + std::to_string(mapId), "Name");
            CHECK(savedName == MapInfoList[mapId].name);
        }
    }

    TEST_CASE("getLimit: detección de bordes y mapas colindantes") {
        NumMaps = 10;
        MapData.assign((static_cast<size_t>(NumMaps) + 1) * 101 * 101, MapBlock{});

        int testMap = 5;

        // Configurar salidas en los 4 bordes
        // Norte: Y entre 7 y 10, X entre 15 y 87
        size_t idxNorth = (static_cast<size_t>(testMap) * 101 + 50) * 101 + 7;
        MapData[idxNorth].TileExit.Map = 2;

        // Este: X entre 89 y 92, Y entre 15 y 87 (en getLimit: X del tile es 92 - y, Y del tile es x)
        size_t idxEast = (static_cast<size_t>(testMap) * 101 + 92) * 101 + 50;
        MapData[idxEast].TileExit.Map = 3;

        // Sur: Y entre 91 y 94, X entre 15 y 87
        size_t idxSouth = (static_cast<size_t>(testMap) * 101 + 50) * 101 + 94;
        MapData[idxSouth].TileExit.Map = 4;

        // Oeste: X entre 9 y 12, Y entre 15 y 87 (en getLimit: X del tile es 9 + y, Y del tile es x)
        size_t idxWest = (static_cast<size_t>(testMap) * 101 + 9) * 101 + 50;
        MapData[idxWest].TileExit.Map = 6;

        CHECK(FileIO::getLimit(testMap, eHeading::NORTH) == 2);
        CHECK(FileIO::getLimit(testMap, eHeading::EAST) == 3);
        CHECK(FileIO::getLimit(testMap, eHeading::SOUTH) == 4);
        CHECK(FileIO::getLimit(testMap, eHeading::WEST) == 6);

        // Mapa sin transiciones
        CHECK(FileIO::getLimit(1, eHeading::NORTH) == 0);

        // Argumentos fuera de rango
        CHECK(FileIO::getLimit(0, eHeading::NORTH) == 0);
        CHECK(FileIO::getLimit(-1, eHeading::NORTH) == 0);
        CHECK(FileIO::getLimit(999, eHeading::NORTH) == 0);
    }

    TEST_CASE("setDistance y generateMatrix: cálculo y propagación de distancias a ciudades") {
        NumMaps = 4;
        MapData.assign((static_cast<size_t>(NumMaps) + 1) * 101 * 101, MapBlock{});

        // Configurar Ullathorpe en mapa 1
        Ciudades[cUllathorpe].Map = 1;
        Ciudades[cNix].Map = 3;

        // Mapa 1 conecta al Norte con Mapa 2
        size_t idxN1 = (static_cast<size_t>(1) * 101 + 50) * 101 + 7;
        MapData[idxN1].TileExit.Map = 2;

        // Mapa 2 conecta al Sur con Mapa 1
        size_t idxS2 = (static_cast<size_t>(2) * 101 + 50) * 101 + 94;
        MapData[idxS2].TileExit.Map = 1;

        FileIO::generateMatrix(1);

        // Mapa 2 está colindante a Mapa 1 (distancia 1 a Ullathorpe).
        CHECK(distanceToCities[2].distanceToCity[cUllathorpe] == 1);

        // Mapa 4 no tiene conexión -> queda en -1
        CHECK(distanceToCities[4].distanceToCity[cUllathorpe] == -1);
    }

    TEST_CASE("Manejo de errores y resiliencia ante mapas o rutas inválidas") {
        // IDs inválidos no deben causar crash
        FileIO::CargarMapa(-1, "ruta_invalida");
        FileIO::CargarMapa(0, "ruta_invalida");
        FileIO::CargarMapa(1, "ruta_que_no_existe_seguramente");

        FileIO::GrabarMapa(-1, "temp/invalido");
        FileIO::GrabarMapa(0, "temp/invalido");

        FileIO::setDistance(-1, 1, 1);
        FileIO::setDistance(1, 0, 1);
        FileIO::setDistance(1, 99, 1);

        CHECK(FileIO::getLimit(-5, 1) == 0);
    }

    TEST_CASE("LoadMapData: carga de múltiples mapas desde archivo Map.dat") {
        std::string tempDir = "tests/temp_loadmapdata/";
        std::filesystem::create_directories(tempDir);

        std::string mapDat = tempDir + "Map.dat";
        std::ofstream datOut(mapDat, std::ios::binary);
        datOut << "[INIT]\r\n";
        datOut << "NumMaps=2\r\n";
        datOut << "MapPath=" << tempDir << "\r\n";
        datOut.close();

        // Creamos mapas sintéticos mínimos 1 y 2 en tempDir
        for (int m = 1; m <= 2; ++m) {
            std::string base = tempDir + "Mapa" + std::to_string(m);

            // .map: versión (2) + cabecera (263) + 4 ints (8) + 10000 tiles (mínimo 3 bytes c/u: flag + g1) = 30273
            std::vector<uint8_t> mb(273 + 10000 * 3, 0);
            std::ofstream mOut(base + ".map", std::ios::binary);
            mOut.write(reinterpret_cast<const char*>(mb.data()), mb.size());
            mOut.close();

            // .inf: 5 ints (10) + 10000 tiles (1 byte flag c/u) = 10010
            std::vector<uint8_t> ib(10 + 10000 * 1, 0);
            std::ofstream iOut(base + ".inf", std::ios::binary);
            iOut.write(reinterpret_cast<const char*>(ib.data()), ib.size());
            iOut.close();

            // .dat
            std::ofstream dOut(base + ".dat", std::ios::binary);
            dOut << "[Mapa" << m << "]\r\nName=MapaDePrueba" << m << "\r\n";
            dOut.close();
        }

        DatPath = tempDir;
        FileIO::LoadMapData();

        CHECK(NumMaps == 2);
        REQUIRE(MapInfoList.size() > 2);
        CHECK(MapInfoList[1].name == "MapaDePrueba1");
        CHECK(MapInfoList[2].name == "MapaDePrueba2");
    }
}
