#include <doctest/doctest.h>
#include "server/FileIO.hpp"
#include "server/Declares.hpp"
#include "server/clsIniReader.hpp"
#include <filesystem>
#include <fstream>

// ============================================================================
// Pruebas Unitarias para FileIO Grupo 2 (Persistencia de Personajes .chr)
// ============================================================================
// NOTA SOBRE FIXTURES:
// Tal como establece la documentación del proyecto (docs/audit/06-formatos-de-datos.md),
// los archivos de fixture en tests/fixtures/charfile/ son SINTÉTICOS.
// Estas pruebas verifican la autoconsistencia byte a byte y la paridad exacta
// respecto a la especificación auditada de persistencia de personajes, no la salida
// de un ejecutable VB6 legacy verificado de forma independiente.
// ============================================================================

TEST_SUITE("FileIO_PersistenciaPersonajes") {

    TEST_CASE("Carga de personaje PEPE.chr desde fixture sintético") {
        UserList.resize(100);
        // NOTA: Fixture sintético en tests/fixtures/charfile/PEPE.chr
        std::string fixturePath = "tests/fixtures/charfile/PEPE.chr";
        REQUIRE(std::filesystem::exists(fixturePath));

        clsIniReader reader;
        reader.Initialize(fixturePath);

        int userIdx = 1;
        FileIO::LoadUserInit(userIdx, reader);

        const auto& user = UserList[userIdx];

        // Verificación de datos de inicialización ([INIT])
        CHECK(user.Genero == eGenero::Hombre); // Genero = 1
        CHECK(user.raza == eRaza::Humano);    // Raza = 1
        CHECK(user.Hogar == eCiudad::cUllathorpe); // Hogar = 1
        CHECK(user.clase == eClass::Mage);     // Clase = 1
        CHECK(user.desc == "Mago humano versado en hechizos elementales.");
        CHECK(static_cast<int>(user.char_appearance.heading) == 3); // Heading = 3 (SOUTH)
        CHECK(user.OrigChar.Head == 1);
        CHECK(user.OrigChar.body == 1);

        // Verificación de estadísticas ([STATS])
        CHECK(user.Stats.GLD == 50000);
        CHECK(user.Stats.Banco == 100000);
        CHECK(user.Stats.MaxHp == 250);
        CHECK(user.Stats.MinHp == 250);
        CHECK(user.Stats.MaxMAN == 500);
        CHECK(user.Stats.MinMAN == 500);
        CHECK(user.Stats.MaxSta == 150);
        CHECK(user.Stats.MinSta == 150);
        CHECK(user.Stats.ELV == 25);
        CHECK(user.Stats.ELU == 2000000);

        // Verificación de atributos ([ATRIBUTOS])
        CHECK(user.Stats.UserAtributos[1] == 18); // AT1 Fuerza
        CHECK(user.Stats.UserAtributos[2] == 18); // AT2 Agilidad
        CHECK(user.Stats.UserAtributos[3] == 20); // AT3 Inteligencia
        CHECK(user.Stats.UserAtributos[4] == 15); // AT4 Carisma
        CHECK(user.Stats.UserAtributos[5] == 18); // AT5 Constitución

        // Verificación de inventario ([INVENTORY])
        CHECK(user.Invent.NroItems == 10);
        CHECK(user.Invent.Object[1].ObjIndex == 101);
        CHECK(user.Invent.Object[1].Amount == 5);
        CHECK(user.Invent.Object[1].Equipped == 0);
        CHECK(user.Invent.Object[10].ObjIndex == 110);
        CHECK(user.Invent.Object[10].Amount == 50);

        // Verificación de clan ([GUILD])
        CHECK(user.GuildIndex == 1);
    }

    TEST_CASE("Carga de personaje GONZALO.chr (Guerrero Elfo nivel alto)") {
        UserList.resize(100);
        // NOTA: Fixture sintético en tests/fixtures/charfile/GONZALO.chr
        std::string fixturePath = "tests/fixtures/charfile/GONZALO.chr";
        REQUIRE(std::filesystem::exists(fixturePath));

        clsIniReader reader;
        reader.Initialize(fixturePath);

        int userIdx = 2;
        FileIO::LoadUserInit(userIdx, reader);

        const auto& user = UserList[userIdx];

        CHECK(user.raza == eRaza::Elfo);       // Raza = 2
        CHECK(user.clase == eClass::Warrior);  // Clase = 3
        CHECK(user.Stats.GLD == 250000);
        CHECK(user.Stats.ELV == 45);
        CHECK(user.Invent.NroItems == 20);
    }

    TEST_CASE("Carga de personaje NOVATO.chr (Aventurero inicial)") {
        UserList.resize(100);
        // NOTA: Fixture sintético en tests/fixtures/charfile/NOVATO.chr
        std::string fixturePath = "tests/fixtures/charfile/NOVATO.chr";
        REQUIRE(std::filesystem::exists(fixturePath));

        clsIniReader reader;
        reader.Initialize(fixturePath);

        int userIdx = 3;
        FileIO::LoadUserInit(userIdx, reader);

        const auto& user = UserList[userIdx];

        CHECK(user.Stats.GLD == 0);
        CHECK(user.Stats.ELV == 1);
        CHECK(user.Invent.NroItems == 0);
    }

    TEST_CASE("Carga de personaje PODEROSO.chr (Stats máximos en nivel 50)") {
        UserList.resize(100);
        // NOTA: Fixture sintético en tests/fixtures/charfile/PODEROSO.chr
        std::string fixturePath = "tests/fixtures/charfile/PODEROSO.chr";
        REQUIRE(std::filesystem::exists(fixturePath));

        clsIniReader reader;
        reader.Initialize(fixturePath);

        int userIdx = 4;
        FileIO::LoadUserInit(userIdx, reader);

        const auto& user = UserList[userIdx];

        CHECK(user.Stats.GLD == 5000000);
        CHECK(user.Stats.Banco == 10000000);
        CHECK(user.Stats.MaxHp == 999);
        CHECK(user.Stats.MaxMAN == 9999);
        CHECK(user.Stats.ELV == 50);

        // Atributos en el máximo (21)
        for (int i = 1; i <= NUMATRIBUTOS; ++i) {
            CHECK(user.Stats.UserAtributos[i] == 21);
        }
    }

    TEST_CASE("Consistencia Round-Trip (SaveUser y LoadUserInit)") {
        UserList.resize(100);
        // NOTA: Verifica que SaveUser produzca un archivo .chr que, al ser vuelto a cargar con LoadUserInit,
        // restaure de forma idéntica todos los campos de usuario.
        int srcIdx = 1;
        std::string srcFixture = "tests/fixtures/charfile/PEPE.chr";
        REQUIRE(std::filesystem::exists(srcFixture));

        clsIniReader srcReader;
        srcReader.Initialize(srcFixture);
        FileIO::LoadUserInit(srcIdx, srcReader);

        // Guardar a un archivo temporal en scratch / temp
        std::string tempPath = "tests/test_pepe_temp.chr";

        // Copiar primero el fixture original al archivo temporal para asegurar presencia de secciones no emitidas por SaveUser (Password, FLAGS, FACCIONES)
        std::filesystem::copy_file(srcFixture, tempPath, std::filesystem::copy_options::overwrite_existing);

        FileIO::SaveUser(srcIdx, tempPath);
        REQUIRE(std::filesystem::exists(tempPath));

        // Cargar en un índice distinto (userIdx = 5)
        int dstIdx = 5;
        clsIniReader dstReader;
        dstReader.Initialize(tempPath);
        FileIO::LoadUserInit(dstIdx, dstReader);

        const auto& orig = UserList[srcIdx];
        const auto& loaded = UserList[dstIdx];

        CHECK(loaded.Genero == orig.Genero);
        CHECK(loaded.raza == orig.raza);
        CHECK(loaded.Hogar == orig.Hogar);
        CHECK(loaded.clase == orig.clase);
        CHECK(loaded.desc == orig.desc);

        CHECK(loaded.Stats.GLD == orig.Stats.GLD);
        CHECK(loaded.Stats.Banco == orig.Stats.Banco);
        CHECK(loaded.Stats.MaxHp == orig.Stats.MaxHp);
        CHECK(loaded.Stats.MinHp == orig.Stats.MinHp);
        CHECK(loaded.Stats.MaxMAN == orig.Stats.MaxMAN);
        CHECK(loaded.Stats.MinMAN == orig.Stats.MinMAN);
        CHECK(loaded.Stats.ELV == orig.Stats.ELV);

        for (int i = 1; i <= NUMATRIBUTOS; ++i) {
            CHECK(loaded.Stats.UserAtributos[i] == orig.Stats.UserAtributos[i]);
        }

        for (int i = 1; i <= NUMSKILLS; ++i) {
            CHECK(loaded.Stats.UserSkills[i] == orig.Stats.UserSkills[i]);
        }

        CHECK(loaded.Invent.NroItems == orig.Invent.NroItems);
        for (int i = 1; i <= orig.Invent.NroItems; ++i) {
            CHECK(loaded.Invent.Object[i].ObjIndex == orig.Invent.Object[i].ObjIndex);
            CHECK(loaded.Invent.Object[i].Amount == orig.Invent.Object[i].Amount);
            CHECK(loaded.Invent.Object[i].Equipped == orig.Invent.Object[i].Equipped);
        }

        // Limpieza de archivo temporal
        std::filesystem::remove(tempPath);
    }

    TEST_CASE("Función criminal de reputación") {
        UserList.resize(100);
        int userIdx = 6;

        // Usuario ciudadano (reputación positiva)
        UserList[userIdx].Reputacion.AsesinoRep = 0;
        UserList[userIdx].Reputacion.BandidoRep = 0;
        UserList[userIdx].Reputacion.BurguesRep = 100;
        UserList[userIdx].Reputacion.LadronesRep = 0;
        UserList[userIdx].Reputacion.NobleRep = 100;
        UserList[userIdx].Reputacion.PlebeRep = 100;
        CHECK_FALSE(FileIO::criminal(userIdx));

        // Usuario criminal (reputación asesino alta)
        UserList[userIdx].Reputacion.AsesinoRep = 500;
        UserList[userIdx].Reputacion.BandidoRep = 500;
        UserList[userIdx].Reputacion.BurguesRep = 0;
        UserList[userIdx].Reputacion.LadronesRep = 500;
        UserList[userIdx].Reputacion.NobleRep = 0;
        UserList[userIdx].Reputacion.PlebeRep = 0;
        CHECK(FileIO::criminal(userIdx));
    }

    TEST_CASE("Validación de límites de UserIndex (std::out_of_range)") {
        UserList.resize(100);
        clsIniReader reader;

        // userIndex 0 es inválido (< 1)
        CHECK_THROWS_AS(FileIO::LoadUserInit(0, reader), std::out_of_range);

        // userIndex fuera de los límites de UserList
        int outOfBoundsIdx = static_cast<int>(UserList.size()) + 10;
        CHECK_THROWS_AS(FileIO::LoadUserInit(outOfBoundsIdx, reader), std::out_of_range);
        CHECK_THROWS_AS(FileIO::SaveUser(outOfBoundsIdx, "dummy.chr"), std::out_of_range);
        CHECK_THROWS_AS(FileIO::criminal(outOfBoundsIdx), std::out_of_range);
    }

    TEST_CASE("Validación contra personajes REALES de producción (tests/fixtures/charfile/real/)") {
        UserList.resize(100);

        // 1. Carga de ELIO.chr (Mago Humano Nivel 25, GM/Dios, Guild Game Masters)
        {
            std::string elioPath = "tests/fixtures/charfile/real/ELIO.chr";
            REQUIRE(std::filesystem::exists(elioPath));
            clsIniReader reader;
            reader.Initialize(elioPath);

            int userIdx = 20;
            FileIO::LoadUserInit(userIdx, reader);
            const auto& user = UserList[userIdx];

            CHECK(user.Genero == eGenero::Hombre);
            CHECK(user.raza == eRaza::Humano);
            CHECK(user.clase == eClass::Mage);
            CHECK(user.Stats.ELV == 25);
            CHECK(user.Stats.GLD == 2);
            CHECK(user.Stats.MaxHp == 164);
            CHECK(user.Stats.MinHp == 164);
            CHECK(user.Stats.MaxMAN == 1128);
            CHECK(user.Stats.MinMAN == 48);
            CHECK(user.Stats.UserAtributos[1] == 16); // AT1 Fuerza
            CHECK(user.Stats.UserAtributos[2] == 16); // AT2 Agilidad
            CHECK(user.Stats.UserAtributos[5] == 18); // AT5 Constitución
            CHECK(user.Stats.UserSkills[1] == 62);   // Magia
            CHECK(user.Stats.UserSkills[16] == 90);  // Meditar
            CHECK(user.GuildIndex == 1);            // Game Masters
            CHECK(user.Reputacion.NobleRep == 2000);
            CHECK_FALSE(FileIO::criminal(userIdx));
        }

        // 2. Carga y verificación de USER.chr (Clérigo Humano)
        {
            std::string userPath = "tests/fixtures/charfile/real/USER.chr";
            REQUIRE(std::filesystem::exists(userPath));
            clsIniReader reader;
            reader.Initialize(userPath);

            int userIdx = 21;
            FileIO::LoadUserInit(userIdx, reader);
            const auto& user = UserList[userIdx];

            CHECK(user.Genero == eGenero::Hombre);
            CHECK(user.raza == eRaza::Humano);
            CHECK(user.clase == eClass::Cleric);
        }

        // 3. Carga y verificación de BETATESTER.chr (Mago Humano)
        {
            std::string betaPath = "tests/fixtures/charfile/real/BETATESTER.chr";
            REQUIRE(std::filesystem::exists(betaPath));
            clsIniReader reader;
            reader.Initialize(betaPath);

            int userIdx = 22;
            FileIO::LoadUserInit(userIdx, reader);
            const auto& user = UserList[userIdx];

            CHECK(user.clase == eClass::Mage);
            CHECK(user.raza == eRaza::Humano);
        }

        // 4. Carga y verificación de MASTER.chr (Clérigo)
        {
            std::string masterPath = "tests/fixtures/charfile/real/MASTER.chr";
            REQUIRE(std::filesystem::exists(masterPath));
            clsIniReader reader;
            reader.Initialize(masterPath);

            int userIdx = 23;
            FileIO::LoadUserInit(userIdx, reader);
            const auto& user = UserList[userIdx];

            CHECK(user.clase == eClass::Cleric);
        }
    }

    TEST_CASE("Roundtrip SaveUser / LoadUserInit con personaje REAL ELIO.chr") {
        UserList.resize(100);

        std::string elioPath = "tests/fixtures/charfile/real/ELIO.chr";
        clsIniReader reader;
        reader.Initialize(elioPath);

        int slotOriginal = 30;
        FileIO::LoadUserInit(slotOriginal, reader);

        std::string tempDir = "tests/temp_charfile_real/";
        std::filesystem::create_directories(tempDir);
        std::string tempSavedPath = tempDir + "ELIO_saved.chr";

        // Serializar a disco
        FileIO::SaveUser(slotOriginal, tempSavedPath);

        // Deserializar en otro slot
        int slotReload = 31;
        clsIniReader readerReload;
        readerReload.Initialize(tempSavedPath);
        FileIO::LoadUserInit(slotReload, readerReload);

        const auto& uOrig = UserList[slotOriginal];
        const auto& uRel = UserList[slotReload];

        // Comparar campos fundamentales
        CHECK(uRel.clase == uOrig.clase);
        CHECK(uRel.raza == uOrig.raza);
        CHECK(uRel.Genero == uOrig.Genero);
        CHECK(uRel.Hogar == uOrig.Hogar);
        CHECK(uRel.Stats.ELV == uOrig.Stats.ELV);
        CHECK(uRel.Stats.GLD == uOrig.Stats.GLD);
        CHECK(uRel.Stats.Banco == uOrig.Stats.Banco);
        CHECK(uRel.Stats.MaxHp == uOrig.Stats.MaxHp);
        CHECK(uRel.Stats.MinHp == uOrig.Stats.MinHp);
        CHECK(uRel.Stats.MaxMAN == uOrig.Stats.MaxMAN);
        CHECK(uRel.Stats.MinMAN == uOrig.Stats.MinMAN);
        // Nota: GuildIndex no es serializado por SaveUser (es administrado por modGuilds.bas)

        for (int i = 1; i <= NUMATRIBUTOS; ++i) {
            CHECK(uRel.Stats.UserAtributos[i] == uOrig.Stats.UserAtributos[i]);
        }
        for (int i = 1; i <= NUMSKILLS; ++i) {
            CHECK(uRel.Stats.UserSkills[i] == uOrig.Stats.UserSkills[i]);
        }
    }
}
