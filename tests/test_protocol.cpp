#include <doctest/doctest.h>
#include "server/Protocol.hpp"

#include <type_traits>
#include <cstdint>

using namespace ao::net::protocol;

TEST_SUITE("Protocol - Paso 1: Definición de Opcodes y Tipos Fuertes") {

    TEST_CASE("Alineación binaria estricta a 1 byte (uint8_t)") {
        // Validación estática de tipo subyacente
        static_assert(std::is_same_v<std::underlying_type_t<ServerPacketID>, std::uint8_t>,
                      "ServerPacketID debe tener como tipo subyacente uint8_t");
        static_assert(std::is_same_v<std::underlying_type_t<ClientPacketID>, std::uint8_t>,
                      "ClientPacketID debe tener como tipo subyacente uint8_t");
        static_assert(std::is_same_v<std::underlying_type_t<FontTypeNames>, std::uint8_t>,
                      "FontTypeNames debe tener como tipo subyacente uint8_t");
        static_assert(std::is_same_v<std::underlying_type_t<eEditOptions>, std::uint8_t>,
                      "eEditOptions debe tener como tipo subyacente uint8_t");
        static_assert(std::is_same_v<std::underlying_type_t<PacketParseResult>, std::uint8_t>,
                      "PacketParseResult debe tener como tipo subyacente uint8_t");

        // Validación estática de tamaño en memoria (sizeof == 1)
        static_assert(sizeof(ServerPacketID) == 1, "ServerPacketID debe ocupar exactamente 1 byte");
        static_assert(sizeof(ClientPacketID) == 1, "ClientPacketID debe ocupar exactamente 1 byte");
        static_assert(sizeof(FontTypeNames) == 1, "FontTypeNames debe ocupar exactamente 1 byte");
        static_assert(sizeof(eEditOptions) == 1, "eEditOptions debe ocupar exactamente 1 byte");
        static_assert(sizeof(PacketParseResult) == 1, "PacketParseResult debe ocupar exactamente 1 byte");

        // Validación estática de valores absolutos (paridad binaria estricta con VB6)
        static_assert(static_cast<std::uint8_t>(ServerPacketID::Logged) == 0, "Logged debe ser 0 en base 0 de VB6");
        static_assert(static_cast<std::uint8_t>(ServerPacketID::CancelOfferItem) == 103, "CancelOfferItem debe ser 103");
        static_assert(static_cast<std::uint8_t>(ClientPacketID::LoginExistingChar) == 0, "LoginExistingChar debe ser 0");
        static_assert(static_cast<std::uint8_t>(ClientPacketID::ThrowDices) == 1, "ThrowDices debe ser 1");
        static_assert(static_cast<std::uint8_t>(ClientPacketID::LoginNewChar) == 2, "LoginNewChar debe ser 2");
        static_assert(static_cast<std::uint8_t>(ClientPacketID::Talk) == 3, "Talk debe ser 3");
        static_assert(static_cast<std::uint8_t>(ClientPacketID::Walk) == 6, "Walk debe ser 6");
        static_assert(static_cast<std::uint8_t>(ClientPacketID::Attack) == 8, "Attack debe ser 8");
        static_assert(static_cast<std::uint8_t>(ClientPacketID::PickUp) == 9, "PickUp debe ser 9");
        static_assert(static_cast<std::uint8_t>(ClientPacketID::Ping) == 119, "Ping debe ser 119");
        static_assert(static_cast<std::uint8_t>(ClientPacketID::Consultation) == 128, "Consultation debe ser 128");
        static_assert(LAST_CLIENT_PACKET_ID == 128, "LAST_CLIENT_PACKET_ID debe ser 128");
        static_assert(static_cast<std::uint8_t>(eEditOptions::eo_Gold) == 1, "eo_Gold debe iniciar en 1");
        static_assert(static_cast<std::uint8_t>(eEditOptions::eo_addGold) == 15, "eo_addGold debe ser 15");

        // Chequeos en tiempo de ejecución
        CHECK(sizeof(ServerPacketID) == 1);
        CHECK(sizeof(ClientPacketID) == 1);
        CHECK(sizeof(FontTypeNames) == 1);
        CHECK(sizeof(eEditOptions) == 1);
        CHECK(sizeof(PacketParseResult) == 1);
    }

    TEST_CASE("ServerPacketID - Mapeo numérico y cotas exactas") {
        // En VB6 los enumeradores sin inicializador explícito inician en 0
        CHECK(static_cast<std::uint8_t>(ServerPacketID::Logged) == 0);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::RemoveDialogs) == 1);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::RemoveCharDialog) == 2);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::NavigateToggle) == 3);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::Disconnect) == 4);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::CommerceEnd) == 5);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::BankEnd) == 6);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::CommerceInit) == 7);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::BankInit) == 8);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::UserCommerceInit) == 9);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::UserCommerceEnd) == 10);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::UserOfferConfirm) == 11);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::CommerceChat) == 12);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::ShowBlacksmithForm) == 13);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::ShowCarpenterForm) == 14);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::UpdateSta) == 15);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::UpdateMana) == 16);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::UpdateHP) == 17);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::UpdateGold) == 18);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::UpdateBankGold) == 19);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::UpdateExp) == 20);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::ChangeMap) == 21);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::PosUpdate) == 22);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::ChatOverHead) == 23);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::ConsoleMsg) == 24);

        // Opcodes intermedios y de cierre
        CHECK(static_cast<std::uint8_t>(ServerPacketID::GuildChat) == 25);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::ShowMessageBox) == 26);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::CharacterCreate) == 29);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::CharacterRemove) == 30);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::CharacterMove) == 32);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::ObjectCreate) == 35);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::ObjectDelete) == 36);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::PlayMidi) == 38);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::PlayWave) == 39);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::PauseToggle) == 42);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::RainToggle) == 43);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::CreateFX) == 44);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::UpdateUserStats) == 45);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::ErrorMsg) == 55);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::Blind) == 56);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::Dumb) == 57);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::Pong) == 88);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::MultiMessage) == 101);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::StopWorking) == 102);
        CHECK(static_cast<std::uint8_t>(ServerPacketID::CancelOfferItem) == 103);
    }

    TEST_CASE("ClientPacketID - Mapeo numérico y constante centinela") {
        // En VB6 inicia en 0 (LoginExistingChar) y concluye en 128 (Consultation)
        CHECK(static_cast<std::uint8_t>(ClientPacketID::LoginExistingChar) == 0);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::ThrowDices) == 1);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::LoginNewChar) == 2);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Talk) == 3);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Yell) == 4);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Whisper) == 5);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Walk) == 6);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::RequestPositionUpdate) == 7);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Attack) == 8);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::PickUp) == 9);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::SafeToggle) == 10);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::ResuscitationSafeToggle) == 11);

        // Opcodes clave de interacción y comercio
        CHECK(static_cast<std::uint8_t>(ClientPacketID::CommerceEnd) == 17);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::BankEnd) == 21);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Drop) == 24);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::CastSpell) == 25);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::LeftClick) == 26);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::DoubleClick) == 27);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Work) == 28);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::UseItem) == 30);

        // Comandos de consola y estado
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Online) == 70);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Quit) == 71);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Rest) == 78);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Meditate) == 79);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Resucitate) == 80);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Heal) == 81);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Help) == 82);

        // Opcodes finales
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Ping) == 119);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::RequestPartyForm) == 120);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::ItemUpgrade) == 121);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::GMCommands) == 122);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::InitCrafting) == 123);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Home) == 124);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::ShowGuildNews) == 125);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::ShareNpc) == 126);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::StopSharingNpc) == 127);
        CHECK(static_cast<std::uint8_t>(ClientPacketID::Consultation) == 128);

        // Validación estricta de la constante centinela del protocolo
        static_assert(LAST_CLIENT_PACKET_ID == 128, "LAST_CLIENT_PACKET_ID debe ser 128");
        CHECK(LAST_CLIENT_PACKET_ID == static_cast<std::uint8_t>(ClientPacketID::Consultation));
    }

    TEST_CASE("FontTypeNames - Enumerador de fuentes de consola") {
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_TALK) == 0);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_FIGHT) == 1);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_WARNING) == 2);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_INFO) == 3);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_SERVER) == 9);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_GUILDMSG) == 10);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_CONSEJO) == 11);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_CONSEJOCAOS) == 12);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_CENTINELA) == 15);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_GMMSG) == 16);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_GM) == 17);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_CITIZEN) == 18);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_CONSE) == 19);
        CHECK(static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_DIOS) == 20);
    }

    TEST_CASE("eEditOptions - Opciones de edición con offset base 1") {
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_Gold) == 1);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_Experience) == 2);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_Body) == 3);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_Head) == 4);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_CiticensKilled) == 5);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_CriminalsKilled) == 6);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_Level) == 7);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_Class) == 8);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_Skills) == 9);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_SkillPointsLeft) == 10);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_Nobleza) == 11);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_Asesino) == 12);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_Sex) == 13);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_Raza) == 14);
        CHECK(static_cast<std::uint8_t>(eEditOptions::eo_addGold) == 15);
    }

    TEST_CASE("PacketParseResult - Resultados del despachador binario") {
        CHECK(static_cast<std::uint8_t>(PacketParseResult::Ok) == 0);
        CHECK(static_cast<std::uint8_t>(PacketParseResult::NeedMoreData) == 1);
        CHECK(static_cast<std::uint8_t>(PacketParseResult::InvalidSession) == 2);
        CHECK(static_cast<std::uint8_t>(PacketParseResult::MalformedData) == 3);
        CHECK(static_cast<std::uint8_t>(PacketParseResult::UnknownPacket) == 4);
    }
}

TEST_SUITE("Protocol - Paso 2: Generadores Multicast PrepareMessage...") {

    TEST_CASE("PrepareMessagePauseToggle - Paquete sin payload") {
        auto buf = PrepareMessagePauseToggle();
        CHECK(buf.size() == 1);
        CHECK(buf[0] == static_cast<std::uint8_t>(ServerPacketID::PauseToggle));
    }

    TEST_CASE("PrepareMessageRainToggle - Paquete sin payload") {
        auto buf = PrepareMessageRainToggle();
        CHECK(buf.size() == 1);
        CHECK(buf[0] == static_cast<std::uint8_t>(ServerPacketID::RainToggle));
    }

    TEST_CASE("PrepareMessagePlayWave - Serialización de audio 3D y coordenadas") {
        // Opcode (1) + wave (1) + X (1) + Y (1) = 4 bytes
        auto buf = PrepareMessagePlayWave(44, 50, 60);
        CHECK(buf.size() == 4);
        CHECK(buf[0] == static_cast<std::uint8_t>(ServerPacketID::PlayWave));
        CHECK(buf[1] == 44);
        CHECK(buf[2] == 50);
        CHECK(buf[3] == 60);
    }

    TEST_CASE("PrepareMessagePlayMidi - Midi y loops Little-Endian") {
        // Opcode (1) + midi (1) + loops (2, int16_t) = 4 bytes
        auto buf = PrepareMessagePlayMidi(7, -1);
        CHECK(buf.size() == 4);
        CHECK(buf[0] == static_cast<std::uint8_t>(ServerPacketID::PlayMidi));
        CHECK(buf[1] == 7);
        // -1 en int16_t Little-Endian es 0xFF, 0xFF
        CHECK(buf[2] == 0xFF);
        CHECK(buf[3] == 0xFF);
    }

    TEST_CASE("PrepareMessageSetInvisible - Manejo de CharIndex y booleano a 1 byte") {
        // Opcode (1) + CharIndex (2, int16_t LE) + invisible (1, uint8_t 1/0) = 4 bytes
        auto bufTrue = PrepareMessageSetInvisible(514, true); // 514 = 0x0202 -> LE: 0x02, 0x02
        CHECK(bufTrue.size() == 4);
        CHECK(bufTrue[0] == static_cast<std::uint8_t>(ServerPacketID::SetInvisible));
        CHECK(bufTrue[1] == 0x02);
        CHECK(bufTrue[2] == 0x02);
        CHECK(bufTrue[3] == 1); // Exactamente 1 byte (true = 1)

        auto bufFalse = PrepareMessageSetInvisible(1, false);
        CHECK(bufFalse.size() == 4);
        CHECK(bufFalse[0] == static_cast<std::uint8_t>(ServerPacketID::SetInvisible));
        CHECK(bufFalse[1] == 1);
        CHECK(bufFalse[2] == 0);
        CHECK(bufFalse[3] == 0); // Exactamente 1 byte (false = 0)
    }

    TEST_CASE("PrepareMessageBlockPosition - Coordenadas y estado booleano") {
        // Opcode (1) + X (1) + Y (1) + Blocked (1) = 4 bytes
        auto buf = PrepareMessageBlockPosition(12, 34, true);
        CHECK(buf.size() == 4);
        CHECK(buf[0] == static_cast<std::uint8_t>(ServerPacketID::BlockPosition));
        CHECK(buf[1] == 12);
        CHECK(buf[2] == 34);
        CHECK(buf[3] == 1);
    }

    TEST_CASE("PrepareMessageObjectCreate y ObjectDelete") {
        // ObjectCreate: Opcode (1) + X (1) + Y (1) + GrhIndex (2, LE) = 5 bytes
        auto bufCreate = PrepareMessageObjectCreate(1050, 45, 67); // 1050 = 0x041A -> LE: 0x1A, 0x04
        CHECK(bufCreate.size() == 5);
        CHECK(bufCreate[0] == static_cast<std::uint8_t>(ServerPacketID::ObjectCreate));
        CHECK(bufCreate[1] == 45);
        CHECK(bufCreate[2] == 67);
        CHECK(bufCreate[3] == 0x1A);
        CHECK(bufCreate[4] == 0x04);

        // ObjectDelete: Opcode (1) + X (1) + Y (1) = 3 bytes
        auto bufDelete = PrepareMessageObjectDelete(45, 67);
        CHECK(bufDelete.size() == 3);
        CHECK(bufDelete[0] == static_cast<std::uint8_t>(ServerPacketID::ObjectDelete));
        CHECK(bufDelete[1] == 45);
        CHECK(bufDelete[2] == 67);
    }

    TEST_CASE("PrepareMessageCharacterRemove y RemoveCharDialog") {
        // CharacterRemove: Opcode (1) + CharIndex (2) = 3 bytes
        auto bufRemove = PrepareMessageCharacterRemove(300); // 300 = 0x012C -> LE: 0x2C, 0x01
        CHECK(bufRemove.size() == 3);
        CHECK(bufRemove[0] == static_cast<std::uint8_t>(ServerPacketID::CharacterRemove));
        CHECK(bufRemove[1] == 0x2C);
        CHECK(bufRemove[2] == 0x01);

        // RemoveCharDialog: Opcode (1) + CharIndex (2) = 3 bytes
        auto bufDialog = PrepareMessageRemoveCharDialog(300);
        CHECK(bufDialog.size() == 3);
        CHECK(bufDialog[0] == static_cast<std::uint8_t>(ServerPacketID::RemoveCharDialog));
        CHECK(bufDialog[1] == 0x2C);
        CHECK(bufDialog[2] == 0x01);
    }

    TEST_CASE("PrepareMessageCreateFX - Identificador de char, FX y repeticiones") {
        // Opcode (1) + CharIndex (2) + FX (2) + loops (2) = 7 bytes
        auto buf = PrepareMessageCreateFX(10, 5, 2);
        CHECK(buf.size() == 7);
        CHECK(buf[0] == static_cast<std::uint8_t>(ServerPacketID::CreateFX));
        CHECK(buf[1] == 10);
        CHECK(buf[2] == 0);
        CHECK(buf[3] == 5);
        CHECK(buf[4] == 0);
        CHECK(buf[5] == 2);
        CHECK(buf[6] == 0);
    }

    TEST_CASE("PrepareMessageCharacterMove y ForceCharMove") {
        // CharacterMove: Opcode (1) + CharIndex (2) + X (1) + Y (1) = 5 bytes
        auto bufMove = PrepareMessageCharacterMove(20, 55, 66);
        CHECK(bufMove.size() == 5);
        CHECK(bufMove[0] == static_cast<std::uint8_t>(ServerPacketID::CharacterMove));
        CHECK(bufMove[1] == 20);
        CHECK(bufMove[2] == 0);
        CHECK(bufMove[3] == 55);
        CHECK(bufMove[4] == 66);

        // ForceCharMove: Opcode (1) + Direccion (1) = 2 bytes
        auto bufForce = PrepareMessageForceCharMove(eHeading::EAST);
        CHECK(bufForce.size() == 2);
        CHECK(bufForce[0] == static_cast<std::uint8_t>(ServerPacketID::ForceCharMove));
        CHECK(bufForce[1] == static_cast<std::uint8_t>(eHeading::EAST));
    }

    TEST_CASE("PrepareMessageChatOverHead - RGB de 3 bytes (ahorro respecto a Long)") {
        // Opcode (1) + StrLen (2) + Str ("Hola", 4) + CharIndex (2) + R (1) + G (1) + B (1) = 12 bytes
        // color = 0x00332211 -> R = 0x11, G = 0x22, B = 0x33
        const std::int32_t color = 0x00332211;
        auto buf = PrepareMessageChatOverHead("Hola", 15, color);
        CHECK(buf.size() == 12);
        CHECK(buf[0] == static_cast<std::uint8_t>(ServerPacketID::ChatOverHead));
        CHECK(buf[1] == 4); // StrLen LE: 4
        CHECK(buf[2] == 0);
        CHECK(buf[3] == 'H');
        CHECK(buf[4] == 'o');
        CHECK(buf[5] == 'l');
        CHECK(buf[6] == 'a');
        CHECK(buf[7] == 15); // CharIndex LE: 15
        CHECK(buf[8] == 0);
        CHECK(buf[9] == 0x11);  // R
        CHECK(buf[10] == 0x22); // G
        CHECK(buf[11] == 0x33); // B
    }

    TEST_CASE("PrepareMessageConsoleMsg - Prefijo dinámico y FontIndex") {
        // Opcode (1) + StrLen (2) + Str ("Test", 4) + FontIndex (1) = 8 bytes
        auto buf = PrepareMessageConsoleMsg("Test", FontTypeNames::FONTTYPE_INFO);
        CHECK(buf.size() == 8);
        CHECK(buf[0] == static_cast<std::uint8_t>(ServerPacketID::ConsoleMsg));
        CHECK(buf[1] == 4);
        CHECK(buf[2] == 0);
        CHECK(buf[3] == 'T');
        CHECK(buf[4] == 'e');
        CHECK(buf[5] == 's');
        CHECK(buf[6] == 't');
        CHECK(buf[7] == static_cast<std::uint8_t>(FontTypeNames::FONTTYPE_INFO));
    }

    TEST_CASE("PrepareMessageCharacterChangeNick, GuildChat, ShowMessageBox y ErrorMsg") {
        // CharacterChangeNick: Opcode (1) + CharIndex (2) + StrLen (2) + Str ("Moro", 4) = 9 bytes
        auto bufNick = PrepareMessageCharacterChangeNick(100, "Moro");
        CHECK(bufNick.size() == 9);
        CHECK(bufNick[0] == static_cast<std::uint8_t>(ServerPacketID::CharacterChangeNick));
        CHECK(bufNick[1] == 100);
        CHECK(bufNick[2] == 0);
        CHECK(bufNick[3] == 4);
        CHECK(bufNick[4] == 0);

        // GuildChat: Opcode (1) + StrLen (2) + Str ("Clan", 4) = 7 bytes
        auto bufGuild = PrepareMessageGuildChat("Clan");
        CHECK(bufGuild.size() == 7);
        CHECK(bufGuild[0] == static_cast<std::uint8_t>(ServerPacketID::GuildChat));
        CHECK(bufGuild[1] == 4);
        CHECK(bufGuild[2] == 0);

        // ShowMessageBox: Opcode (1) + StrLen (2) + Str ("Aviso", 5) = 8 bytes
        auto bufBox = PrepareMessageShowMessageBox("Aviso");
        CHECK(bufBox.size() == 8);
        CHECK(bufBox[0] == static_cast<std::uint8_t>(ServerPacketID::ShowMessageBox));
        CHECK(bufBox[1] == 5);
        CHECK(bufBox[2] == 0);

        // ErrorMsg: Opcode (1) + StrLen (2) + Str ("Error", 5) = 8 bytes
        auto bufErr = PrepareMessageErrorMsg("Error");
        CHECK(bufErr.size() == 8);
        CHECK(bufErr[0] == static_cast<std::uint8_t>(ServerPacketID::ErrorMsg));
        CHECK(bufErr[1] == 5);
        CHECK(bufErr[2] == 0);
    }

    TEST_CASE("PrepareMessageCharacterCreate - Serialización binaria compleja y orden estricto de campos") {
        // En VB6: CharIndex, body, Head, heading, X, Y, weapon, shield, helmet, FX, FXLoops, name, NickColor, Privileges
        // Tamaños:
        // Opcode (1) + CharIndex (2) + body (2) + Head (2) + heading (1) + X (1) + Y (1)
        // + weapon (2) + shield (2) + helmet (2) + FX (2) + FXLoops (2)
        // + StrLen (2) + name ("Heroe", 5) + NickColor (1) + Privileges (1)
        // = 1 + 2 + 2 + 2 + 1 + 1 + 1 + 2 + 2 + 2 + 2 + 2 + 2 + 5 + 1 + 1 = 29 bytes
        auto buf = PrepareMessageCharacterCreate(101, 202, eHeading::SOUTH, 50, 25, 75,
                                                301, 401, 501, 1, 601, "Heroe", 2, 1);
        CHECK(buf.size() == 29);
        CHECK(buf[0] == static_cast<std::uint8_t>(ServerPacketID::CharacterCreate));
        CHECK(buf[1] == 50);  // CharIndex (LE)
        CHECK(buf[2] == 0);
        CHECK(buf[3] == 101); // body (LE)
        CHECK(buf[4] == 0);
        CHECK(buf[5] == static_cast<std::uint8_t>(202)); // Head (LE)
        CHECK(buf[6] == 0);
        CHECK(buf[7] == static_cast<std::uint8_t>(eHeading::SOUTH)); // heading (1 byte)
        CHECK(buf[8] == 25);  // X
        CHECK(buf[9] == 75);  // Y
        CHECK(buf[10] == static_cast<std::uint8_t>(301 & 0xFF)); // weapon
        CHECK(buf[11] == static_cast<std::uint8_t>(301 >> 8));
        CHECK(buf[12] == static_cast<std::uint8_t>(401 & 0xFF)); // shield
        CHECK(buf[13] == static_cast<std::uint8_t>(401 >> 8));
        CHECK(buf[14] == static_cast<std::uint8_t>(601 & 0xFF)); // helmet
        CHECK(buf[15] == static_cast<std::uint8_t>(601 >> 8));
        CHECK(buf[16] == static_cast<std::uint8_t>(501 & 0xFF)); // FX
        CHECK(buf[17] == static_cast<std::uint8_t>(501 >> 8));
        CHECK(buf[18] == 1);  // FXLoops
        CHECK(buf[19] == 0);
        CHECK(buf[20] == 5);  // Name len (LE)
        CHECK(buf[21] == 0);
        CHECK(buf[22] == 'H');
        CHECK(buf[23] == 'e');
        CHECK(buf[24] == 'r');
        CHECK(buf[25] == 'o');
        CHECK(buf[26] == 'e');
        CHECK(buf[27] == 2);  // NickColor
        CHECK(buf[28] == 1);  // Privileges
    }

    TEST_CASE("PrepareMessageCharacterChange - Serialización de cambio de apariencia") {
        // Opcode (1) + CharIndex (2) + body (2) + Head (2) + heading (1) + weapon (2)
        // + shield (2) + helmet (2) + FX (2) + FXLoops (2) = 18 bytes
        auto buf = PrepareMessageCharacterChange(10, 20, eHeading::WEST, 1, 30, 40, 50, 2, 60);
        CHECK(buf.size() == 18);
        CHECK(buf[0] == static_cast<std::uint8_t>(ServerPacketID::CharacterChange));
        CHECK(buf[1] == 1);  // CharIndex (LE)
        CHECK(buf[2] == 0);
        CHECK(buf[3] == 10); // body (LE)
        CHECK(buf[4] == 0);
        CHECK(buf[5] == 20); // Head (LE)
        CHECK(buf[6] == 0);
        CHECK(buf[7] == static_cast<std::uint8_t>(eHeading::WEST)); // heading (1 byte)
        CHECK(buf[8] == 30); // weapon (LE)
        CHECK(buf[9] == 0);
        CHECK(buf[10] == 40); // shield (LE)
        CHECK(buf[11] == 0);
        CHECK(buf[12] == 60); // helmet (LE)
        CHECK(buf[13] == 0);
        CHECK(buf[14] == 50); // FX (LE)
        CHECK(buf[15] == 0);
        CHECK(buf[16] == 2);  // FXLoops (LE)
        CHECK(buf[17] == 0);
    }

    TEST_CASE("PrepareMessageUpdateTagAndStatus - Resolución de UserIndex y Tag") {
        // Opcode (1) + CharIndex (2) + NickColor (1) + StrLen (2) + Tag ("<Admin>", 7) = 13 bytes
        // Con UserList vacía o fuera de rango, CharIndex es 0
        auto buf = PrepareMessageUpdateTagAndStatus(999, 3, "<Admin>");
        CHECK(buf.size() == 13);
        CHECK(buf[0] == static_cast<std::uint8_t>(ServerPacketID::UpdateTagAndStatus));
        CHECK(buf[1] == 0);  // CharIndex = 0
        CHECK(buf[2] == 0);
        CHECK(buf[3] == 3);  // NickColor
        CHECK(buf[4] == 7);  // Tag length
        CHECK(buf[5] == 0);
        CHECK(buf[6] == '<');
        CHECK(buf[12] == '>');
    }
}

TEST_SUITE("Protocol - Paso 3: Primitivas de Salida Unicast (Write...) y FlushBuffer") {

    TEST_CASE("Acumulación consecutiva de paquetes unicast en outgoingData") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 1;
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].Stats.MinHp = 150;
        UserList[userIndex].Stats.GLD = 100000;
        UserList[userIndex].Pos.X = 50;
        UserList[userIndex].Pos.Y = 45;

        // 1. WriteUpdateHP: 1 byte opcode + 2 bytes MinHp = 3 bytes
        WriteUpdateHP(userIndex);
        CHECK(UserList[userIndex].outgoingData->length() == 3);

        // 2. WriteUpdateGold: 1 byte opcode + 4 bytes GLD = 5 bytes (Total 8 bytes)
        WriteUpdateGold(userIndex);
        CHECK(UserList[userIndex].outgoingData->length() == 8);

        // 3. WritePosUpdate: 1 byte opcode + 1 byte X + 1 byte Y = 3 bytes (Total 11 bytes)
        WritePosUpdate(userIndex);
        CHECK(UserList[userIndex].outgoingData->length() == 11);

        // Inspeccionar contenido sin consumirlo
        auto* q = UserList[userIndex].outgoingData.get();
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::UpdateHP));
        CHECK(q->ReadInteger() == 150);
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::UpdateGold));
        CHECK(q->ReadLong() == 100000);
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::PosUpdate));
        CHECK(q->ReadByte() == 50);
        CHECK(q->ReadByte() == 45);
        CHECK(q->length() == 0);
    }

    TEST_CASE("FlushBuffer vacía outgoingData de forma transaccional") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 2;
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].Stats.MinSta = 200;

        WriteUpdateSta(userIndex);
        CHECK(UserList[userIndex].outgoingData->length() == 3);

        // Invocación a FlushBuffer: vacía completamente la cola del usuario
        FlushBuffer(userIndex);
        CHECK(UserList[userIndex].outgoingData->length() == 0);

        // FlushBuffer sobre cola vacía no debe provocar excepciones ni fallos
        CHECK_NOTHROW(FlushBuffer(userIndex));
    }

    TEST_CASE("Wrappers de PrepareMessage... encolan el payload exacto") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 3;
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();

        // WriteRemoveCharDialog(userIndex, 77)
        WriteRemoveCharDialog(userIndex, 77);
        auto* q = UserList[userIndex].outgoingData.get();
        CHECK(q->length() == 3);
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::RemoveCharDialog));
        CHECK(q->ReadInteger() == 77);

        // WriteSetInvisible(userIndex, 10, true)
        WriteSetInvisible(userIndex, 10, true);
        CHECK(q->length() == 4);
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::SetInvisible));
        CHECK(q->ReadInteger() == 10);
        CHECK(q->ReadBoolean() == true);
    }

    TEST_CASE("WriteMultiMessage serializa variantes complejas de combate y hogar") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 4;
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        auto* q = UserList[userIndex].outgoingData.get();

        // Caso NPCHitUser: Opcode + MsgIndex + Target (Byte) + Damage (Integer) = 1 + 1 + 1 + 2 = 5 bytes
        WriteMultiMessage(userIndex, static_cast<std::int16_t>(eMessages::NPCHitUser), 1, 45);
        CHECK(q->length() == 5);
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::MultiMessage));
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(eMessages::NPCHitUser));
        CHECK(q->ReadByte() == 1);
        CHECK(q->ReadInteger() == 45);

        // Caso Home: Opcode + MsgIndex + Arg1 (Byte) + Arg2 (Integer) + StringArg1 (ASCII)
        WriteMultiMessage(userIndex, static_cast<std::int16_t>(eMessages::Home), 2, 10, 0, "Ullathorpe");
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::MultiMessage));
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(eMessages::Home));
        CHECK(q->ReadByte() == 2);
        CHECK(q->ReadInteger() == 10);
        CHECK(q->ReadASCIIString() == "Ullathorpe");
    }

    TEST_CASE("WriteChangeInventorySlot serializa item existente o vacío según ObjIndex") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }
        if (ObjDataList.size() < 10) {
            ObjDataList.resize(10);
        }

        const std::int16_t userIndex = 5;
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        auto* q = UserList[userIndex].outgoingData.get();

        // 1. Con ObjIndex > 0
        ObjDataList[1].name = "Espada Larga";
        ObjDataList[1].GrhIndex = 1234;
        ObjDataList[1].OBJType = eOBJType::otWeapon;
        ObjDataList[1].MaxHIT = 20;
        ObjDataList[1].MinHIT = 10;
        ObjDataList[1].MaxDef = 5;
        ObjDataList[1].MinDef = 2;
        ObjDataList[1].Valor = 300;

        UserList[userIndex].Invent.Object[1].ObjIndex = 1;
        UserList[userIndex].Invent.Object[1].Amount = 1;
        UserList[userIndex].Invent.Object[1].Equipped = 1;

        WriteChangeInventorySlot(userIndex, 1);
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::ChangeInventorySlot));
        CHECK(q->ReadByte() == 1);
        CHECK(q->ReadInteger() == 1);
        CHECK(q->ReadASCIIString() == "Espada Larga");
        CHECK(q->ReadInteger() == 1); // Amount
        CHECK(q->ReadBoolean() == true); // Equipped
        CHECK(q->ReadInteger() == 1234); // GrhIndex
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(eOBJType::otWeapon));
        CHECK(q->ReadInteger() == 20); // MaxHIT
        CHECK(q->ReadInteger() == 10); // MinHIT
        CHECK(q->ReadInteger() == 5);  // MaxDef
        CHECK(q->ReadInteger() == 2);  // MinDef
        CHECK(q->ReadSingle() == 100.0f);   // SalePrice = 300 / 3

        // 2. Con ObjIndex = 0 (Borrado del slot)
        UserList[userIndex].Invent.Object[1].ObjIndex = 0;
        UserList[userIndex].Invent.Object[1].Amount = 0;
        UserList[userIndex].Invent.Object[1].Equipped = 0;

        WriteChangeInventorySlot(userIndex, 1);
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::ChangeInventorySlot));
        CHECK(q->ReadByte() == 1);
        CHECK(q->ReadInteger() == 0);
        CHECK(q->ReadASCIIString() == "");
        CHECK(q->ReadInteger() == 0);
        CHECK(q->ReadBoolean() == false);
        CHECK(q->ReadInteger() == 0);
        CHECK(q->ReadByte() == 0);
        CHECK(q->ReadInteger() == 0);
        CHECK(q->ReadInteger() == 0);
        CHECK(q->ReadInteger() == 0);
        CHECK(q->ReadInteger() == 0);
        CHECK(q->ReadSingle() == 0.0f);
    }

    TEST_CASE("WriteShowForumForm calcula visibilidad según estatus de facción y GM") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 6;
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        auto* q = UserList[userIndex].outgoingData.get();

        // Usuario Real (ArmadaReal = 1)
        UserList[userIndex].Faccion.ArmadaReal = 1;
        UserList[userIndex].Faccion.FuerzasCaos = 0;
        UserList[userIndex].flags.Privilegios = static_cast<PlayerType>(0);
        WriteShowForumForm(userIndex);

        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::ShowForumForm));
        CHECK(q->ReadByte() == 1); // Visibilidad Real = 1
        CHECK(q->ReadBoolean() == false); // canSticky = false

        // Usuario GM Dios
        UserList[userIndex].Faccion.ArmadaReal = 0;
        UserList[userIndex].flags.Privilegios = PlayerType::Dios;
        WriteShowForumForm(userIndex);

        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::ShowForumForm));
        CHECK(q->ReadByte() == 3); // Visibilidad Armada | Caos = 3
        CHECK(q->ReadBoolean() == true); // canSticky = true
    }

    TEST_CASE("WriteSendSkills calcula porcentajes exactos de experiencia en habilidades") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 7;
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        auto* q = UserList[userIndex].outgoingData.get();

        // Skill 1 con 50 puntos y 50% de progreso
        UserList[userIndex].Stats.UserSkills[1] = 50;
        UserList[userIndex].Stats.ExpSkills[1] = 500;
        UserList[userIndex].Stats.EluSkills[1] = 1000;

        WriteSendSkills(userIndex);
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::SendSkills));
        CHECK(q->ReadByte() == 50);  // Puntos Skill 1
        CHECK(q->ReadByte() == 50);  // 500 * 100 / 1000 = 50%
    }
}


TEST_SUITE("Protocol - Paso 4: Recepcion, Desfragmentacion y Despachador Monolitico") {
    using namespace ao::net::protocol;

    TEST_CASE("Desfragmentacion TCP en paquete de longitud fija (Walk)") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 1;
        UserList[userIndex].incomingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].flags.UserLogged = true;

        auto* inQ = UserList[userIndex].incomingData.get();

        // 1. Llega unicamente el opcode de Walk (1 byte) sin el heading (requiere 2 bytes en total)
        inQ->WriteByte(static_cast<std::uint8_t>(ClientPacketID::Walk));
        CHECK(inQ->length() == 1);

        HandleIncomingData(userIndex);

        // La transaccion sufrio rollback por NotEnoughDataException: el opcode permanece intacto
        CHECK(inQ->length() == 1);
        CHECK(inQ->PeekByte() == static_cast<std::uint8_t>(ClientPacketID::Walk));

        // 2. Llega el byte de heading restante por la red (heading = 2, Sur)
        inQ->WriteByte(2);
        CHECK(inQ->length() == 2);

        HandleIncomingData(userIndex);

        // Paquete consumido en su totalidad tras completarse los bytes requeridos
        CHECK(inQ->length() == 0);
    }

    TEST_CASE("Desfragmentacion TCP en paquete de longitud dinamica con string (Talk)") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 2;
        UserList[userIndex].incomingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].flags.UserLogged = true;

        auto* inQ = UserList[userIndex].incomingData.get();

        // Paquete Talk completo: Opcode (1 byte) + Prefijo longitud 10 (2 bytes) + 10 bytes de texto ("Hola mundo") = 13 bytes
        const std::string fullMsg = "Hola mundo";

        // Fase 1: Llega solo opcode (1 byte) + primer byte de la longitud de cadena (total 2 bytes)
        inQ->WriteByte(static_cast<std::uint8_t>(ClientPacketID::Talk));
        inQ->WriteByte(10); // LSB de int16
        CHECK(inQ->length() == 2);

        HandleIncomingData(userIndex);
        CHECK(inQ->length() == 2); // Rollback: cola intacta

        // Fase 2: Llega el segundo byte de longitud (MSB = 0) y parte del mensaje (4 bytes: "Hola")
        inQ->WriteByte(0); // MSB de int16
        inQ->WriteASCIIStringFixed("Hola");
        // Total en cola: 1 opcode + 2 len + 4 texto = 7 bytes (faltan 6 bytes de texto)
        CHECK(inQ->length() == 7);

        HandleIncomingData(userIndex);
        CHECK(inQ->length() == 7); // Rollback: transaccion preserva los 7 bytes sin desfasar el stream

        // Fase 3: Llega el remanente de 6 bytes (" mundo")
        inQ->WriteASCIIStringFixed(" mundo");
        CHECK(inQ->length() == 13);

        HandleIncomingData(userIndex);
        // Paquete completado y consumido integramente
        CHECK(inQ->length() == 0);
    }

    TEST_CASE("Rafaga concatenada de multiples paquetes en una sola lectura TCP") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 3;
        UserList[userIndex].incomingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].flags.UserLogged = true;

        auto* inQ = UserList[userIndex].incomingData.get();

        // Paquete 1: Walk (opcode 6 + heading 1 = 2 bytes)
        inQ->WriteByte(static_cast<std::uint8_t>(ClientPacketID::Walk));
        inQ->WriteByte(1);

        // Paquete 2: SafeToggle (opcode 10 = 1 byte)
        inQ->WriteByte(static_cast<std::uint8_t>(ClientPacketID::SafeToggle));

        // Paquete 3: Talk (opcode 3 + string "Ok" = 1 + 2 + 2 = 5 bytes)
        inQ->WriteByte(static_cast<std::uint8_t>(ClientPacketID::Talk));
        inQ->WriteASCIIString("Ok");

        // Total en el buffer: 2 + 1 + 5 = 8 bytes
        CHECK(inQ->length() == 8);

        HandleIncomingData(userIndex);

        // El bucle iterativo debe drenar los tres paquetes sucesivamente
        CHECK(inQ->length() == 0);
    }

    TEST_CASE("Validacion de sesion: rechaza paquetes de juego si el usuario no esta logueado") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 4;
        UserList[userIndex].incomingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].flags.UserLogged = false; // NO logueado

        auto* inQ = UserList[userIndex].incomingData.get();

        // Intenta enviar Attack (requiere usuario logueado)
        inQ->WriteByte(static_cast<std::uint8_t>(ClientPacketID::Attack));

        HandleIncomingData(userIndex);

        // Debe cerrar la conexion de inmediato
        CHECK(UserList[userIndex].flags.UserLogged == false);
        // Socket cerrado resetea incomingData
        CHECK((UserList[userIndex].incomingData == nullptr || UserList[userIndex].incomingData->length() == 0));
    }

    TEST_CASE("Validacion de sesion: rechaza paquetes de login si el usuario ya esta logueado") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 5;
        UserList[userIndex].incomingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].flags.UserLogged = true; // YA logueado

        auto* inQ = UserList[userIndex].incomingData.get();

        // Intenta enviar LoginExistingChar estando ya autenticado
        inQ->WriteByte(static_cast<std::uint8_t>(ClientPacketID::LoginExistingChar));
        inQ->WriteASCIIString("MiUser");
        inQ->WriteASCIIString("MiPass");
        inQ->WriteByte(0);
        inQ->WriteByte(13);
        inQ->WriteByte(0);

        HandleIncomingData(userIndex);

        // Violacion de sesion: debe forzar CloseSocket
        CHECK(UserList[userIndex].flags.UserLogged == false);
    }

    TEST_CASE("Reseteo de IdleCount y NoPuedeSerAtacado al recibir paquetes validos") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 6;
        UserList[userIndex].incomingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].flags.UserLogged = true;
        UserList[userIndex].Counters.IdleCount = 350;
        UserList[userIndex].flags.NoPuedeSerAtacado = true;

        auto* inQ = UserList[userIndex].incomingData.get();
        inQ->WriteByte(static_cast<std::uint8_t>(ClientPacketID::Walk));
        inQ->WriteByte(3); // Este

        HandleIncomingData(userIndex);

        CHECK(UserList[userIndex].Counters.IdleCount == 0);
        CHECK(UserList[userIndex].flags.NoPuedeSerAtacado == false);
        CHECK(inQ->length() == 0);
    }

    TEST_CASE("Rechazo y cierre de socket ante opcode desconocido o fuera de rango") {
        if (UserList.size() < 10) {
            UserList.resize(10);
        }

        const std::int16_t userIndex = 8;
        UserList[userIndex].incomingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].outgoingData = std::make_unique<clsByteQueue>();
        UserList[userIndex].flags.UserLogged = true;

        auto* inQ = UserList[userIndex].incomingData.get();
        // 250 esta fuera del rango de ClientPacketID (0 a 128)
        inQ->WriteByte(250);

        HandleIncomingData(userIndex);

        // Desconexion inmediata
        CHECK(UserList[userIndex].flags.UserLogged == false);
    }
}
