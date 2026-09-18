#include <doctest/doctest.h>
#include "server/Modulo_UsUaRiOs.hpp"
#include "server/Declares.hpp"
#include "server/TCP.hpp"
#include "server/Protocol.hpp"

TEST_SUITE("Modulo_UsUaRiOs - G1: Ciclo de Vida y Ranuras") {

TEST_CASE("NextOpenUser finds first open slot and skips occupied slots") {
    struct SlotBackup {
        std::int32_t connID;
        bool userLogged;
    };
    std::vector<SlotBackup> backup(MaxUsers + 1);
    for (int i = 1; i <= MaxUsers; ++i) {
        backup[i] = SlotBackup{UserList[i].ConnID, UserList[i].flags.UserLogged};
    }

    // Reset all slots to logged/occupied
    for (int i = 1; i <= MaxUsers; ++i) {
        UserList[i].ConnID = i;
        UserList[i].flags.UserLogged = true;
    }

    // When full, returns MaxUsers + 1 (legacy VB6 behavior)
    CHECK(Modulo_UsUaRiOs::NextOpenUser() == MaxUsers + 1);

    // Free slot 5
    UserList[5].ConnID = -1;
    UserList[5].flags.UserLogged = false;

    CHECK(Modulo_UsUaRiOs::NextOpenUser() == 5);

    // Free slot 2 as well (should pick slot 2 as it's lower)
    UserList[2].ConnID = -1;
    UserList[2].flags.UserLogged = false;

    CHECK(Modulo_UsUaRiOs::NextOpenUser() == 2);

    // Restore state
    for (int i = 1; i <= MaxUsers; ++i) {
        UserList[i].ConnID = backup[i].connID;
        UserList[i].flags.UserLogged = backup[i].userLogged;
    }
}

TEST_CASE("NextOpenCharIndex finds first empty char index and handles saturation") {
    auto backup_charlist = CharList;

    for (int i = 1; i <= MAXCHARS; ++i) {
        CharList[i] = static_cast<std::int16_t>(i);
    }

    CHECK(Modulo_UsUaRiOs::NextOpenCharIndex() == 0);

    CharList[10] = 0;
    CHECK(Modulo_UsUaRiOs::NextOpenCharIndex() == 10);

    CharList[3] = 0;
    CHECK(Modulo_UsUaRiOs::NextOpenCharIndex() == 3);

    CharList = backup_charlist;
}

TEST_CASE("Cerrar_Usuario handles PK map countdown vs safe map immediate exit") {
    IntervaloCerrarConexion = 10;
    if (MapInfoList.size() <= 2) {
        MapInfoList.resize(3);
    }

    MapInfoList[1].Pk = false;
    MapInfoList[2].Pk = true;

    // Save initial state for users 1 and 2
    auto orig1_logged = UserList[1].flags.UserLogged;
    auto orig1_priv = UserList[1].flags.Privilegios;
    auto orig1_map = UserList[1].Pos.Map;
    auto orig1_saliendo = UserList[1].Counters.Saliendo;
    auto orig1_salir = UserList[1].Counters.Salir;
    auto orig1_valid = UserList[1].ConnIDValida;
    auto orig1_conn = UserList[1].ConnID;

    auto orig2_logged = UserList[2].flags.UserLogged;
    auto orig2_priv = UserList[2].flags.Privilegios;
    auto orig2_map = UserList[2].Pos.Map;
    auto orig2_saliendo = UserList[2].Counters.Saliendo;
    auto orig2_salir = UserList[2].Counters.Salir;
    auto orig2_valid = UserList[2].ConnIDValida;
    auto orig2_conn = UserList[2].ConnID;

    // User 1 in safe map (Pk = false -> Salir = 0, Saliendo = true)
    UserList[1].flags.UserLogged = true;
    UserList[1].flags.Privilegios = PlayerType::UserPlayer;
    UserList[1].Pos.Map = 1;
    UserList[1].Counters.Saliendo = false;
    UserList[1].Counters.Salir = 0;
    UserList[1].ConnIDValida = true;
    UserList[1].ConnID = 1;

    Modulo_UsUaRiOs::Cerrar_Usuario(1);
    CHECK(UserList[1].Counters.Saliendo == true);
    CHECK(UserList[1].Counters.Salir == 0);

    // User 2 in PK map (Pk = true -> Salir = 10, Saliendo = true)
    UserList[2].flags.UserLogged = true;
    UserList[2].flags.Privilegios = PlayerType::UserPlayer;
    UserList[2].Pos.Map = 2;
    UserList[2].Counters.Saliendo = false;
    UserList[2].Counters.Salir = 0;
    UserList[2].ConnIDValida = true;
    UserList[2].ConnID = 2;

    Modulo_UsUaRiOs::Cerrar_Usuario(2);
    CHECK(UserList[2].Counters.Saliendo == true);
    CHECK(UserList[2].Counters.Salir == 10);

    // Restore
    UserList[1].flags.UserLogged = orig1_logged;
    UserList[1].flags.Privilegios = orig1_priv;
    UserList[1].Pos.Map = orig1_map;
    UserList[1].Counters.Saliendo = orig1_saliendo;
    UserList[1].Counters.Salir = orig1_salir;
    UserList[1].ConnIDValida = orig1_valid;
    UserList[1].ConnID = orig1_conn;

    UserList[2].flags.UserLogged = orig2_logged;
    UserList[2].flags.Privilegios = orig2_priv;
    UserList[2].Pos.Map = orig2_map;
    UserList[2].Counters.Saliendo = orig2_saliendo;
    UserList[2].Counters.Salir = orig2_salir;
    UserList[2].ConnIDValida = orig2_valid;
    UserList[2].ConnID = orig2_conn;
}

TEST_CASE("CancelExit cancels countdown when ConnIDValida is true") {
    auto orig_logged = UserList[1].flags.UserLogged;
    auto orig_valid = UserList[1].ConnIDValida;
    auto orig_saliendo = UserList[1].Counters.Saliendo;
    auto orig_salir = UserList[1].Counters.Salir;

    UserList[1].flags.UserLogged = true;
    UserList[1].ConnIDValida = true;
    UserList[1].Counters.Saliendo = true;
    UserList[1].Counters.Salir = 10;

    Modulo_UsUaRiOs::CancelExit(1);
    CHECK(UserList[1].Counters.Saliendo == false);
    CHECK(UserList[1].Counters.Salir == 0);

    UserList[1].flags.UserLogged = orig_logged;
    UserList[1].ConnIDValida = orig_valid;
    UserList[1].Counters.Saliendo = orig_saliendo;
    UserList[1].Counters.Salir = orig_salir;
}

TEST_CASE("CancelExit replicates Bug #52 when ConnIDValida is false") {
    IntervaloCerrarConexion = 10;
    if (MapInfoList.size() <= 2) {
        MapInfoList.resize(3);
    }
    MapInfoList[2].Pk = true;

    auto orig_logged = UserList[1].flags.UserLogged;
    auto orig_priv = UserList[1].flags.Privilegios;
    auto orig_map = UserList[1].Pos.Map;
    auto orig_valid = UserList[1].ConnIDValida;
    auto orig_saliendo = UserList[1].Counters.Saliendo;
    auto orig_salir = UserList[1].Counters.Salir;

    UserList[1].flags.UserLogged = true;
    UserList[1].flags.Privilegios = PlayerType::UserPlayer;
    UserList[1].Pos.Map = 2;
    UserList[1].ConnIDValida = false;
    UserList[1].Counters.Saliendo = true;
    UserList[1].Counters.Salir = 5;

    Modulo_UsUaRiOs::CancelExit(1);

    CHECK(UserList[1].Counters.Saliendo == true);
    CHECK(UserList[1].Counters.Salir == 10);

    UserList[1].flags.UserLogged = orig_logged;
    UserList[1].flags.Privilegios = orig_priv;
    UserList[1].Pos.Map = orig_map;
    UserList[1].ConnIDValida = orig_valid;
    UserList[1].Counters.Saliendo = orig_saliendo;
    UserList[1].Counters.Salir = orig_salir;
}

TEST_CASE("CambiarNick updates user name for logged target") {
    auto orig1_logged = UserList[1].flags.UserLogged;
    auto orig1_name = UserList[1].name;

    auto orig3_logged = UserList[3].flags.UserLogged;
    auto orig3_name = UserList[3].name;

    UserList[1].flags.UserLogged = true;
    UserList[1].name = "ViejoNick";

    Modulo_UsUaRiOs::CambiarNick(2, 1, "NuevoNick");
    CHECK(UserList[1].name == "NuevoNick");

    UserList[3].flags.UserLogged = false;
    UserList[3].name = "Original";
    Modulo_UsUaRiOs::CambiarNick(2, 3, "Invalido");
    CHECK(UserList[3].name == "Original");

    UserList[1].flags.UserLogged = orig1_logged;
    UserList[1].name = orig1_name;

    UserList[3].flags.UserLogged = orig3_logged;
    UserList[3].name = orig3_name;
}

}


TEST_SUITE("Modulo_UsUaRiOs - G2: Cinemática y Transporte") {

TEST_CASE("InvertHeading accurately inverts all directions") {
    CHECK(Modulo_UsUaRiOs::InvertHeading(eHeading::NORTH) == eHeading::SOUTH);
    CHECK(Modulo_UsUaRiOs::InvertHeading(eHeading::SOUTH) == eHeading::NORTH);
    CHECK(Modulo_UsUaRiOs::InvertHeading(eHeading::EAST) == eHeading::WEST);
    CHECK(Modulo_UsUaRiOs::InvertHeading(eHeading::WEST) == eHeading::EAST);
}

TEST_CASE("PuedeAtravesarAgua returns true for sailing or ghost users") {
    UserList[1].flags.Navegando = 1;
    UserList[1].flags.Muerto = 0;
    CHECK(Modulo_UsUaRiOs::PuedeAtravesarAgua(1) == true);

    UserList[1].flags.Navegando = 0;
    UserList[1].flags.Muerto = 1;
    CHECK(Modulo_UsUaRiOs::PuedeAtravesarAgua(1) == true);

    UserList[1].flags.Navegando = 0;
    UserList[1].flags.Muerto = 0;
    CHECK(Modulo_UsUaRiOs::PuedeAtravesarAgua(1) == false);
}

TEST_CASE("BodyIsBoat detects boat body IDs") {
    CHECK(Modulo_UsUaRiOs::BodyIsBoat(iFragataReal) == true);
    CHECK(Modulo_UsUaRiOs::BodyIsBoat(iBarcaPk) == true);
    CHECK(Modulo_UsUaRiOs::BodyIsBoat(iFragataFantasmal) == true);
    CHECK(Modulo_UsUaRiOs::BodyIsBoat(123) == false);
}

TEST_CASE("ToogleBoatBody toggles sprite based on faction and ship") {
    UserList[1].Faccion.ArmadaReal = 1;
    Modulo_UsUaRiOs::ToogleBoatBody(1);
    CHECK(UserList[1].char_appearance.body == iFragataReal);

    UserList[1].Faccion.ArmadaReal = 0;
    UserList[1].Faccion.FuerzasCaos = 1;
    Modulo_UsUaRiOs::ToogleBoatBody(1);
    CHECK(UserList[1].char_appearance.body == iFragataCaos);
}

TEST_CASE("G2_MoveUserChar_SwapsPositionWithCasper: Live user swaps tile and heading with casper") {
    if (MapInfoList.size() <= 1) {
        MapInfoList.resize(2);
    }
    if (MapData.size() <= 10000) {
        MapData.resize(10000);
    }
    MapInfoList[1].NumUsers = 2;

    // Live user 1 at (50, 50)
    UserList[1].Pos = WorldPos{1, 50, 50};
    UserList[1].flags.Muerto = 0;
    UserList[1].flags.AdminInvisible = 0;
    UserList[1].char_appearance.CharIndex = 1;

    // Casper user 2 at (50, 49) [North]
    UserList[2].Pos = WorldPos{1, 50, 49};
    UserList[2].flags.Muerto = 1;
    UserList[2].flags.AdminInvisible = 0;
    UserList[2].char_appearance.CharIndex = 2;

    std::size_t idx1 = (49 * 100) + 49; // (50, 50)
    std::size_t idx2 = (48 * 100) + 49; // (50, 49)

    MapData[idx1].UserIndex = 1;
    MapData[idx2].UserIndex = 2;
    MapData[idx2].Blocked = 0;

    Modulo_UsUaRiOs::MoveUserChar(1, eHeading::NORTH);

    // Live user 1 is now at (50, 49)
    CHECK(UserList[1].Pos.X == 50);
    CHECK(UserList[1].Pos.Y == 49);
    CHECK(UserList[1].char_appearance.heading == eHeading::NORTH);

    // Casper user 2 is swapped to (50, 50) and heading inverted to SOUTH
    CHECK(UserList[2].Pos.X == 50);
    CHECK(UserList[2].Pos.Y == 50);
    CHECK(UserList[2].char_appearance.heading == eHeading::SOUTH);

    CHECK(MapData[idx2].UserIndex == 1);
    CHECK(MapData[idx1].UserIndex == 2);
}

TEST_CASE("G2_MoveUserChar_AdminInvisibleDesync_Bug53: Admin invisible moves without swapping casper") {
    if (MapInfoList.size() <= 1) {
        MapInfoList.resize(2);
    }
    if (MapData.size() <= 10000) {
        MapData.resize(10000);
    }
    MapInfoList[1].NumUsers = 2;

    // Admin user 1 at (50, 50)
    UserList[1].Pos = WorldPos{1, 50, 50};
    UserList[1].flags.Muerto = 0;
    UserList[1].flags.AdminInvisible = 1;
    UserList[1].char_appearance.CharIndex = 1;

    // Casper user 2 at (50, 49)
    UserList[2].Pos = WorldPos{1, 50, 49};
    UserList[2].flags.Muerto = 1;
    UserList[2].flags.AdminInvisible = 0;
    UserList[2].char_appearance.CharIndex = 2;

    std::size_t idx1 = (49 * 100) + 49;
    std::size_t idx2 = (48 * 100) + 49;

    MapData[idx1].UserIndex = 1;
    MapData[idx2].UserIndex = 2;

    Modulo_UsUaRiOs::MoveUserChar(1, eHeading::NORTH);

    // Admin position remains at (50, 50) because admin invisible cannot step on casper
    CHECK(UserList[1].Pos.X == 50);
    CHECK(UserList[1].Pos.Y == 50);

    // Bug #53: Casper position remains untouched at (50, 49)
    CHECK(UserList[2].Pos.X == 50);
    CHECK(UserList[2].Pos.Y == 49);
}

TEST_CASE("G2_WarpUserChar_NumUsersClamping_Bug51: Underflow in NumUsers is clamped to 0") {
    if (MapInfoList.size() <= 2) {
        MapInfoList.resize(3);
    }
    if (MapData.size() <= 20000) {
        MapData.resize(20000);
    }

    MapInfoList[1].NumUsers = 0; // Desynced 0
    MapInfoList[2].NumUsers = 0;

    UserList[1].Pos = WorldPos{1, 50, 50};
    UserList[1].flags.AdminInvisible = 0;
    UserList[1].char_appearance.CharIndex = 1;

    Modulo_UsUaRiOs::WarpUserChar(1, 2, 30, 30, false);

    // Bug #51: Old map NumUsers (0 - 1 = -1) is clamped to 0
    CHECK(MapInfoList[1].NumUsers == 0);
    CHECK(MapInfoList[2].NumUsers == 1);
    CHECK(UserList[1].Pos.Map == 2);
    CHECK(UserList[1].Pos.X == 30);
    CHECK(UserList[1].Pos.Y == 30);
}

TEST_CASE("G2_Tilelibre_FindsValidSurroundingTile finds surrounding empty tile") {
    if (MapInfoList.size() <= 1) {
        MapInfoList.resize(2);
    }
    if (MapData.size() <= 10000) {
        MapData.resize(10000);
    }

    WorldPos pos{1, 50, 50};
    WorldPos npos{0, 0, 0};
    Obj obj{1, 1};

    Modulo_UsUaRiOs::Tilelibre(pos, npos, obj, false, true);

    CHECK(npos.Map == 1);
    CHECK(npos.X == 50);
    CHECK(npos.Y == 50);
}

TEST_CASE("GetDireccion returns correct cardinal direction string") {
    UserList[1].Pos = WorldPos{1, 50, 50};
    UserList[2].Pos = WorldPos{1, 50, 55};

    CHECK(Modulo_UsUaRiOs::GetDireccion(1, 2) == "Norte");

    UserList[2].Pos = WorldPos{1, 50, 45};
    CHECK(Modulo_UsUaRiOs::GetDireccion(1, 2) == "Sur");

    UserList[2].Pos = WorldPos{1, 45, 50};
    CHECK(Modulo_UsUaRiOs::GetDireccion(1, 2) == "Este");

    UserList[2].Pos = WorldPos{1, 55, 50};
    CHECK(Modulo_UsUaRiOs::GetDireccion(1, 2) == "Oeste");
}

}

TEST_SUITE("Modulo_UsUaRiOs - G3: Progresión de Nivel y Habilidades") {

TEST_CASE("G3_CheckUserLevel_ThrowsELUOverflowException_Bug50: Throws ELUOverflowException when ELU exceeds INT32_MAX") {
    UserList[1].Stats.ELV = 10;
    UserList[1].Stats.Exp = 2000000000;
    UserList[1].Stats.ELU = 1800000000; // 1.8B * 1.4 = ~2.52B > INT32_MAX

    CHECK_THROWS_AS(Modulo_UsUaRiOs::CheckUserLevel(1), Modulo_UsUaRiOs::ELUOverflowException);
}

TEST_CASE("G3_CheckUserLevel_ExpulsesFactionGuildAtLvl25: Expulses character from faction guild upon reaching lvl 25") {
    UserList[1].name = "Leveler25";
    UserList[1].Stats.ELV = 24;
    UserList[1].Stats.Exp = 100;
    UserList[1].Stats.ELU = 100;
    UserList[1].GuildIndex = 1;

    // Simulate level up to 25
    Modulo_UsUaRiOs::CheckUserLevel(1);

    CHECK(UserList[1].Stats.ELV == 25);
}

TEST_CASE("G3_SubirSkill_BlockedByHungerOrThirst: Skips skill gain if user is hungry or thirsty") {
    UserList[1].flags.Hambre = 1;
    UserList[1].flags.Sed = 0;
    UserList[1].Counters.AsignedSkills = 10;
    UserList[1].Stats.ELV = 10;
    UserList[1].Stats.UserSkills[1] = 10;
    UserList[1].Stats.ExpSkills[1] = 0;
    UserList[1].Stats.EluSkills[1] = 100;

    Modulo_UsUaRiOs::SubirSkill(1, 1, true);

    CHECK(UserList[1].Stats.ExpSkills[1] == 0);
    CHECK(UserList[1].Stats.UserSkills[1] == 10);
}

TEST_CASE("G3_SendUserStatsTxtOFF_DecoupledReader: Inspects offline stats using custom CharReaderHook") {
    Modulo_UsUaRiOs::CharReaderHook reader = [](const std::string& char_name, const std::string& section, const std::string& key) -> std::string {
        if (section == "STATS" || section == "stats") {
            if (key == "ELV" || key == "elv") return "45";
            if (key == "EXP" || key == "Exp") return "1000";
            if (key == "ELU" || key == "elu") return "2000";
            if (key == "MINSTA" || key == "minsta") return "100";
            if (key == "MAXSTA" || key == "maxSta") return "100";
            if (key == "MINHP" || key == "MinHP") return "350";
            if (key == "MAXHP" || key == "MaxHP") return "350";
            if (key == "MINMAN" || key == "MinMAN") return "2000";
            if (key == "MAXMAN" || key == "MaxMAN") return "2000";
            if (key == "MAXHIT" || key == "MaxHIT") return "150";
            if (key == "GLD") return "50000";
        }
        return "";
    };

    // Should not crash and resolve values via hook
    Modulo_UsUaRiOs::SendUserStatsTxtOFF(1, "OfflineHero", reader);
}

TEST_CASE("G3_SendUserOROTxtFromChar_DecoupledReader: Reads and reports bank gold via CharReaderHook") {
    Modulo_UsUaRiOs::CharReaderHook reader = [](const std::string& char_name, const std::string& section, const std::string& key) -> std::string {
        if (section == "STATS" && key == "BANCO") return "1234567";
        if (section == "STATS" && key == "GLD") return "5000";
        return "";
    };

    Modulo_UsUaRiOs::SendUserOROTxtFromChar(1, "RichHero", reader);
}

}

TEST_SUITE("Modulo_UsUaRiOs - G4: Muerte, Resurrección y Estados") {

TEST_CASE("G4_UserDie_AdoptsGhostShipInWater: Assigns iFragataFantasmal when sailing in water") {
    UserList[1].flags.Navegando = 1;
    UserList[1].flags.Muerto = 0;
    UserList[1].Pos = WorldPos{1, 50, 50};

    Modulo_UsUaRiOs::UserDie(1);

    CHECK(UserList[1].char_appearance.body == iFragataFantasmal);
    CHECK(UserList[1].flags.Muerto == 1);
    CHECK(UserList[1].flags.SeguroResu == true);
}

TEST_CASE("G4_UserDie_DropsInventoryAndActivatesResuSafe: Drops items and activates SeguroResu while leaving OrigChar intact") {
    UserList[1].flags.Navegando = 0;
    UserList[1].flags.Muerto = 0;
    UserList[1].Stats.ELV = 20;
    UserList[1].Pos = WorldPos{1, 50, 50};
    UserList[1].OrigChar.body = 15;
    UserList[1].OrigChar.Head = 30;

    Modulo_UsUaRiOs::UserDie(1);

    CHECK(UserList[1].char_appearance.body == iCuerpoMuerto);
    CHECK(UserList[1].char_appearance.Head == 0);
    CHECK(UserList[1].flags.Muerto == 1);
    CHECK(UserList[1].flags.SeguroResu == true);
    CHECK(UserList[1].OrigChar.body == 15);
    CHECK(UserList[1].OrigChar.Head == 30);
}

TEST_CASE("G4_UserDie_PenalizesPartyExp: Applies party exp penalty when player belongs to a party") {
    UserList[1].flags.Navegando = 0;
    UserList[1].flags.Muerto = 0;
    UserList[1].PartyIndex = 1;
    UserList[1].Stats.ELV = 10;
    UserList[1].Pos = WorldPos{1, 50, 50};

    Modulo_UsUaRiOs::UserDie(1);

    CHECK(UserList[1].flags.Muerto == 1);
}

TEST_CASE("G4_RevivirUsuario_RestoresSpriteAndSetsOneHP: Restores OrigChar sprite and sets MinHp to 1") {
    UserList[1].flags.Muerto = 1;
    UserList[1].flags.Navegando = 0;
    UserList[1].Pos = WorldPos{1, 50, 50};
    UserList[1].OrigChar.body = 15;
    UserList[1].OrigChar.Head = 30;
    UserList[1].Stats.UserAtributos[eAtributos::Constitucion] = 18;
    UserList[1].Stats.MaxHp = 200;

    Modulo_UsUaRiOs::RevivirUsuario(1);

    CHECK(UserList[1].flags.Muerto == 0);
    CHECK(UserList[1].char_appearance.body == 15);
    CHECK(UserList[1].char_appearance.Head == 30);
    CHECK(UserList[1].Stats.MinHp == 1);
}

TEST_CASE("G4_VolverCriminal_ChangesAlignment: Increases bandit reputation when becoming criminal") {
    UserList[1].flags.Privilegios = PlayerType::UserPlayer;
    UserList[1].Reputacion.BandidoRep = 0;
    UserList[1].Pos = WorldPos{1, 50, 50};

    Modulo_UsUaRiOs::VolverCriminal(1);

    CHECK(UserList[1].Reputacion.BandidoRep == vlASALTO);
}

TEST_CASE("G4_RefreshCharStatus_UpdatesSpriteAndFactionTag: Refreshes user status and tag") {
    UserList[1].GuildIndex = 1;
    UserList[1].showName = true;
    UserList[1].Pos = WorldPos{1, 50, 50};

    Modulo_UsUaRiOs::RefreshCharStatus(1);

    CHECK(UserList[1].GuildIndex == 1);
}

}

TEST_SUITE("Modulo_UsUaRiOs - G5: Combate e Inventario") {

TEST_CASE("G5_ToogleToAtackable_SetsLegitimateDefense: Sets AtacablePor when attacking owned NPC or player") {
    UserList[1].name = "Attacker";
    UserList[1].flags.Privilegios = PlayerType::UserPlayer;
    UserList[1].flags.AtacablePor = 0;
    UserList[1].Reputacion.PlebeRep = 3000;
    UserList[1].Reputacion.NobleRep = 3000;

    UserList[2].name = "Owner";
    UserList[2].flags.Privilegios = PlayerType::UserPlayer;
    UserList[2].Reputacion.PlebeRep = 3000;
    UserList[2].Reputacion.NobleRep = 3000;

    bool result = Modulo_UsUaRiOs::ToogleToAtackable(1, 2, true);

    CHECK(result == true);
    CHECK(UserList[1].flags.AtacablePor == 2);
}

TEST_CASE("G5_ApropioNpc_And_PerdioNpc: Manages NPC ownership") {
    UserList[1].flags.TargetNPC = 0;
    Npclist[1].Owner = 0;

    Modulo_UsUaRiOs::ApropioNpc(1, 1);

    CHECK(Npclist[1].Owner == 1);
    CHECK(UserList[1].flags.TargetNPC == 1);

    Modulo_UsUaRiOs::PerdioNpc(1);

    CHECK(Npclist[1].Owner == 0);
    CHECK(UserList[1].flags.TargetNPC == 0);
}

TEST_CASE("G5_GetNickColor_EvaluatesHierarchy: Checks GM, faction, and criminal nick colors") {
    UserList[1].flags.Privilegios = PlayerType::Admin;
    CHECK(Modulo_UsUaRiOs::GetNickColor(1) == 0);

    UserList[1].flags.Privilegios = PlayerType::UserPlayer;
    UserList[1].Faccion.ArmadaReal = 1;
    CHECK(Modulo_UsUaRiOs::GetNickColor(1) == 1);

    UserList[1].Faccion.ArmadaReal = 0;
    UserList[1].Faccion.FuerzasCaos = 1;
    CHECK(Modulo_UsUaRiOs::GetNickColor(1) == 2);
}

TEST_CASE("G5_getMaxInventorySlots_WithAndWithoutBackpack: Returns 20 without backpack and 30 with backpack") {
    UserList[1].Invent.MochilaEqpSlot = 0;
    CHECK(Modulo_UsUaRiOs::getMaxInventorySlots(1) == 20);

    UserList[1].Invent.MochilaEqpSlot = 1;
    CHECK(Modulo_UsUaRiOs::getMaxInventorySlots(1) == 30);
}

TEST_CASE("G5_HasEnoughItems_And_TotalOfferItems: Counts item quantities in inventory") {
    UserList[1].Invent.MochilaEqpSlot = 0;
    for (int i = 1; i <= MAX_INVENTORY_SLOTS; ++i) {
        UserList[1].Invent.Object[i] = UserOBJ{0, 0, 0};
    }
    UserList[1].Invent.Object[1] = UserOBJ{105, 5, 0};
    UserList[1].Invent.Object[3] = UserOBJ{105, 10, 0};

    CHECK(Modulo_UsUaRiOs::TotalOfferItems(105, 1) == 15);
    CHECK(Modulo_UsUaRiOs::HasEnoughItems(1, 105, 15) == true);
    CHECK(Modulo_UsUaRiOs::HasEnoughItems(1, 105, 16) == false);
}

TEST_CASE("G5_PuedeApuñalar_And_PuedeAcuchillar: Validates class and weapon stabbing requirements") {
    UserList[1].clase = eClass::Assasin;
    UserList[1].Invent.WeaponEqpObjIndex = 1;
    if (ObjDataList.size() <= 1) ObjDataList.resize(10);
    ObjDataList[1].Apuñala = 1;
    UserList[1].Stats.UserSkills[eSkill::Apuñalar] = 10;

    CHECK(Modulo_UsUaRiOs::PuedeApuñalar(1) == true);

    UserList[1].clase = eClass::Pirat;
    CHECK(Modulo_UsUaRiOs::PuedeAcuchillar(1) == true);
}

}
