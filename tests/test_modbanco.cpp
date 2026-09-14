#include <doctest/doctest.h>
#include "server/modBanco.hpp"
#include "server/InvUsuario.hpp"
#include "server/Declares.hpp"
#include "server/Protocol.hpp"
#include "server/clsByteQueue.hpp"
#include <vector>
#include <string>
#include <map>

namespace {

using namespace ao::net::protocol;

struct TestEnv {
    TestEnv() {
        MaxUsers = 5;
        LastUser = 5;
        UserList.clear();
        UserList.resize(10);
        ObjDataList.assign(100, ObjData{});
        NumObjDatas = 99;

        // Resetear hooks
        InvUsuario::SetQuitarUserInvItemHook(nullptr);
        InvUsuario::SetUpdateUserInvHook(nullptr);
        modBanco::SetQuitarUserInvItemHook(nullptr);
        modBanco::SetQuitarBancoInvItemHook(nullptr);
        modBanco::SetCharReaderHook(nullptr);

        // Configurar ítems de prueba
        ObjDataList[1].name = "Poción Roja";
        ObjDataList[1].OBJType = eOBJType::otPociones;
        ObjDataList[1].GrhIndex = 10;

        ObjDataList[2].name = "Espada Larga";
        ObjDataList[2].OBJType = eOBJType::otWeapon;
        ObjDataList[2].GrhIndex = 20;

        ObjDataList[3].name = "Barca Pescador";
        ObjDataList[3].OBJType = eOBJType::otBarcos;
        ObjDataList[3].GrhIndex = 30;

        ObjDataList[4].name = "Armadura Real";
        ObjDataList[4].OBJType = eOBJType::otArmadura;
        ObjDataList[4].Real = 1;
        ObjDataList[4].GrhIndex = 40;

        ObjDataList[5].name = "Daga Newbie";
        ObjDataList[5].OBJType = eOBJType::otWeapon;
        ObjDataList[5].Newbie = 1;
        ObjDataList[5].GrhIndex = 50;

        for (std::int16_t u = 1; u <= 5; ++u) {
            UserList[u].outgoingData = std::make_unique<clsByteQueue>();
            UserList[u].flags.UserLogged = true;
            UserList[u].flags.Comerciando = false;
            UserList[u].Stats.Banco = 50000;
            UserList[u].CurrentInventorySlots = 30;
            UserList[u].Invent.NroItems = 0;
            UserList[u].BancoInvent.NroItems = 0;
            UserList[u].name = "Heroe" + std::to_string(u);

            for (std::uint8_t s = 1; s <= MAX_INVENTORY_SLOTS; ++s) {
                UserList[u].Invent.Object[s] = UserOBJ{};
            }
            for (std::uint8_t s = 1; s <= MAX_BANCOINVENTORY_SLOTS; ++s) {
                UserList[u].BancoInvent.Object[s] = UserOBJ{};
            }
        }
    }
};

std::vector<std::string> read_console_messages(clsByteQueue* q) {
    std::vector<std::string> messages;
    while (q && q->length() > 0) {
        const auto opcode = q->ReadByte();
        if (opcode == static_cast<std::uint8_t>(ServerPacketID::ConsoleMsg)) {
            messages.push_back(q->ReadASCIIString());
            q->ReadByte(); // FontIndex
        } else {
            break;
        }
    }
    return messages;
}

} // namespace

TEST_SUITE("modBanco - G1") {

TEST_CASE("IniciarDeposito configura comercio y despacha ráfaga completa de paquetes") {
    TestEnv env;
    const std::int16_t user_index = 1;

    UserList[user_index].BancoInvent.Object[1].ObjIndex = 1;
    UserList[user_index].BancoInvent.Object[1].Amount = 50;
    UserList[user_index].BancoInvent.NroItems = 1;

    CHECK(UserList[user_index].flags.Comerciando == false);
    CHECK(UserList[user_index].outgoingData->length() == 0);

    modBanco::IniciarDeposito(user_index);

    CHECK(UserList[user_index].flags.Comerciando == true);
    CHECK(UserList[user_index].outgoingData->length() > 0);

    auto* q = UserList[user_index].outgoingData.get();

    for (std::uint8_t s = 1; s <= MAX_BANCOINVENTORY_SLOTS; ++s) {
        const auto opcode = q->ReadByte();
        CHECK(opcode == static_cast<std::uint8_t>(ServerPacketID::ChangeBankSlot));
        const auto slot_read = q->ReadByte();
        CHECK(slot_read == s);
        const auto obj_index_read = q->ReadInteger();
        if (s == 1) {
            CHECK(obj_index_read == 1);
            CHECK(q->ReadASCIIString() == "Poción Roja");
            CHECK(q->ReadInteger() == 50);
        } else {
            CHECK(obj_index_read == 0);
            CHECK(q->ReadASCIIString().empty());
            CHECK(q->ReadInteger() == 0);
        }
        q->ReadInteger(); q->ReadByte(); q->ReadInteger(); q->ReadInteger(); q->ReadInteger(); q->ReadInteger(); q->ReadLong();
    }

    // UpdateUserStats
    CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::UpdateUserStats));
    q->ReadInteger(); q->ReadInteger(); q->ReadInteger(); q->ReadInteger(); q->ReadInteger(); q->ReadInteger();
    q->ReadLong(); q->ReadByte(); q->ReadLong(); q->ReadLong();

    // BankInit
    CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::BankInit));
    CHECK(q->ReadLong() == 50000);
    CHECK(q->length() == 0);
}

TEST_CASE("SendBanObj asigna en BancoInvent y emite ChangeBankSlot") {
    TestEnv env;
    const std::int16_t user_index = 2;
    const std::uint8_t slot = 15;

    UserOBJ obj{};
    obj.ObjIndex = 2;
    obj.Amount = 5;
    obj.Equipped = 0;

    CHECK(UserList[user_index].BancoInvent.Object[slot].ObjIndex == 0);

    modBanco::SendBanObj(user_index, slot, obj);

    CHECK(UserList[user_index].BancoInvent.Object[slot].ObjIndex == 2);
    CHECK(UserList[user_index].BancoInvent.Object[slot].Amount == 5);

    auto* q = UserList[user_index].outgoingData.get();
    CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::ChangeBankSlot));
    CHECK(q->ReadByte() == slot);
    CHECK(q->ReadInteger() == 2);
    CHECK(q->ReadASCIIString() == "Espada Larga");
    CHECK(q->ReadInteger() == 5);
}

TEST_CASE("SendBanObj ignora índices y slots fuera de rango") {
    TestEnv env;
    UserOBJ obj{};
    obj.ObjIndex = 1;
    obj.Amount = 1;

    modBanco::SendBanObj(0, 1, obj);
    modBanco::SendBanObj(999, 1, obj);
    modBanco::SendBanObj(1, 0, obj);
    modBanco::SendBanObj(1, 41, obj);

    CHECK(UserList[1].outgoingData->length() == 0);
}

TEST_CASE("UpdateBanUserInv unitario despacha sólo el slot especificado sin tocar los demás") {
    TestEnv env;
    const std::int16_t user_index = 3;
    const std::uint8_t slot = 7;

    UserList[user_index].BancoInvent.Object[slot].ObjIndex = 1;
    UserList[user_index].BancoInvent.Object[slot].Amount = 25;

    modBanco::UpdateBanUserInv(false, user_index, slot);

    auto* q = UserList[user_index].outgoingData.get();
    CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::ChangeBankSlot));
    CHECK(q->ReadByte() == slot);
    CHECK(q->ReadInteger() == 1);
    CHECK(q->ReadASCIIString() == "Poción Roja");
    CHECK(q->ReadInteger() == 25);
    q->ReadInteger(); q->ReadByte(); q->ReadInteger(); q->ReadInteger(); q->ReadInteger(); q->ReadInteger(); q->ReadLong();
    CHECK(q->length() == 0);
}

TEST_CASE("UpdateBanUserInv completo despacha los 40 slots") {
    TestEnv env;
    const std::int16_t user_index = 4;

    UserList[user_index].BancoInvent.Object[5].ObjIndex = 2;
    UserList[user_index].BancoInvent.Object[5].Amount = 1;

    modBanco::UpdateBanUserInv(true, user_index, 0);

    auto* q = UserList[user_index].outgoingData.get();

    for (std::uint8_t s = 1; s <= MAX_BANCOINVENTORY_SLOTS; ++s) {
        CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::ChangeBankSlot));
        CHECK(q->ReadByte() == s);
        const auto obj_idx = q->ReadInteger();
        if (s == 5) {
            CHECK(obj_idx == 2);
            CHECK(q->ReadASCIIString() == "Espada Larga");
            CHECK(q->ReadInteger() == 1);
        } else {
            CHECK(obj_idx == 0);
            CHECK(q->ReadASCIIString().empty());
            CHECK(q->ReadInteger() == 0);
        }
        q->ReadInteger(); q->ReadByte(); q->ReadInteger(); q->ReadInteger(); q->ReadInteger(); q->ReadInteger(); q->ReadLong();
    }
    CHECK(q->length() == 0);
}

TEST_CASE("UpdateVentanaBanco emite WriteBankOK") {
    TestEnv env;
    const std::int16_t user_index = 5;

    modBanco::UpdateVentanaBanco(user_index);

    auto* q = UserList[user_index].outgoingData.get();
    CHECK(q->length() == 1);
    CHECK(q->ReadByte() == static_cast<std::uint8_t>(ServerPacketID::BankOK));
}

TEST_CASE("UpdateVentanaBanco e IniciarDeposito con índices de usuario inválidos no fallan") {
    modBanco::IniciarDeposito(0);
    modBanco::IniciarDeposito(999);
    modBanco::UpdateVentanaBanco(0);
    modBanco::UpdateVentanaBanco(999);
    modBanco::UpdateBanUserInv(true, 0, 0);
    modBanco::UpdateBanUserInv(false, 999, 1);
}

}

TEST_SUITE("modBanco - G2") {

TEST_CASE("UserRetiraItem con apilamiento en slot existente de mochila") {
    TestEnv env;
    const std::int16_t user_index = 1;

    UserList[user_index].BancoInvent.Object[1].ObjIndex = 1;
    UserList[user_index].BancoInvent.Object[1].Amount = 50;
    UserList[user_index].BancoInvent.NroItems = 1;

    UserList[user_index].Invent.Object[1].ObjIndex = 1;
    UserList[user_index].Invent.Object[1].Amount = 20;
    UserList[user_index].Invent.NroItems = 1;

    modBanco::UserRetiraItem(user_index, 1, 30);

    CHECK(UserList[user_index].Invent.Object[1].ObjIndex == 1);
    CHECK(UserList[user_index].Invent.Object[1].Amount == 50);
    CHECK(UserList[user_index].Invent.NroItems == 1);

    CHECK(UserList[user_index].BancoInvent.Object[1].ObjIndex == 1);
    CHECK(UserList[user_index].BancoInvent.Object[1].Amount == 20);
    CHECK(UserList[user_index].BancoInvent.NroItems == 1);
}

TEST_CASE("UserRetiraItem ocupando nueva ranura libre en mochila") {
    TestEnv env;
    const std::int16_t user_index = 1;

    UserList[user_index].BancoInvent.Object[1].ObjIndex = 2;
    UserList[user_index].BancoInvent.Object[1].Amount = 1;
    UserList[user_index].BancoInvent.NroItems = 1;

    CHECK(UserList[user_index].Invent.NroItems == 0);

    modBanco::UserRetiraItem(user_index, 1, 1);

    CHECK(UserList[user_index].Invent.Object[1].ObjIndex == 2);
    CHECK(UserList[user_index].Invent.Object[1].Amount == 1);
    CHECK(UserList[user_index].Invent.NroItems == 1);

    CHECK(UserList[user_index].BancoInvent.Object[1].ObjIndex == 0);
    CHECK(UserList[user_index].BancoInvent.Object[1].Amount == 0);
    CHECK(UserList[user_index].BancoInvent.NroItems == 0);
}

TEST_CASE("UserRetiraItem rechazado por mochila llena emite mensaje sin alterar inventarios") {
    TestEnv env;
    const std::int16_t user_index = 1;

    for (std::uint8_t s = 1; s <= 30; ++s) {
        UserList[user_index].Invent.Object[s].ObjIndex = 2;
        UserList[user_index].Invent.Object[s].Amount = 1;
    }
    UserList[user_index].Invent.NroItems = 30;

    UserList[user_index].BancoInvent.Object[1].ObjIndex = 1;
    UserList[user_index].BancoInvent.Object[1].Amount = 10;
    UserList[user_index].BancoInvent.NroItems = 1;

    UserList[user_index].outgoingData = std::make_unique<clsByteQueue>();

    modBanco::UserRetiraItem(user_index, 1, 5);

    CHECK(UserList[user_index].BancoInvent.Object[1].Amount == 10);
    CHECK(UserList[user_index].BancoInvent.NroItems == 1);
    CHECK(UserList[user_index].Invent.NroItems == 30);
}

TEST_CASE("UserRetiraItem clamp de cantidad si supera saldo de banco") {
    TestEnv env;
    const std::int16_t user_index = 1;

    UserList[user_index].BancoInvent.Object[1].ObjIndex = 1;
    UserList[user_index].BancoInvent.Object[1].Amount = 7;
    UserList[user_index].BancoInvent.NroItems = 1;

    modBanco::UserRetiraItem(user_index, 1, 100);

    CHECK(UserList[user_index].Invent.Object[1].ObjIndex == 1);
    CHECK(UserList[user_index].Invent.Object[1].Amount == 7);
    CHECK(UserList[user_index].BancoInvent.Object[1].Amount == 0);
    CHECK(UserList[user_index].BancoInvent.NroItems == 0);
}

TEST_CASE("UserDepositaItem con apilamiento en slot existente de bóveda") {
    TestEnv env;
    const std::int16_t user_index = 1;

    UserList[user_index].BancoInvent.Object[1].ObjIndex = 1;
    UserList[user_index].BancoInvent.Object[1].Amount = 10;
    UserList[user_index].BancoInvent.NroItems = 1;

    UserList[user_index].Invent.Object[1].ObjIndex = 1;
    UserList[user_index].Invent.Object[1].Amount = 15;
    UserList[user_index].Invent.NroItems = 1;

    modBanco::UserDepositaItem(user_index, 1, 5);

    CHECK(UserList[user_index].BancoInvent.Object[1].Amount == 15);
    CHECK(UserList[user_index].BancoInvent.NroItems == 1);

    CHECK(UserList[user_index].Invent.Object[1].Amount == 10);
    CHECK(UserList[user_index].Invent.NroItems == 1);
}

TEST_CASE("UserDepositaItem ocupando nueva ranura libre en bóveda") {
    TestEnv env;
    const std::int16_t user_index = 1;

    UserList[user_index].Invent.Object[1].ObjIndex = 2;
    UserList[user_index].Invent.Object[1].Amount = 1;
    UserList[user_index].Invent.NroItems = 1;

    modBanco::UserDepositaItem(user_index, 1, 1);

    CHECK(UserList[user_index].BancoInvent.Object[1].ObjIndex == 2);
    CHECK(UserList[user_index].BancoInvent.Object[1].Amount == 1);
    CHECK(UserList[user_index].BancoInvent.NroItems == 1);

    CHECK(UserList[user_index].Invent.Object[1].ObjIndex == 0);
    CHECK(UserList[user_index].Invent.Object[1].Amount == 0);
    CHECK(UserList[user_index].Invent.NroItems == 0);
}

TEST_CASE("UserDepositaItem rechazado por bóveda llena") {
    TestEnv env;
    const std::int16_t user_index = 1;

    for (std::uint8_t s = 1; s <= MAX_BANCOINVENTORY_SLOTS; ++s) {
        UserList[user_index].BancoInvent.Object[s].ObjIndex = 2;
        UserList[user_index].BancoInvent.Object[s].Amount = 1;
    }
    UserList[user_index].BancoInvent.NroItems = 40;

    UserList[user_index].Invent.Object[1].ObjIndex = 1;
    UserList[user_index].Invent.Object[1].Amount = 10;
    UserList[user_index].Invent.NroItems = 1;

    modBanco::UserDepositaItem(user_index, 1, 5);

    CHECK(UserList[user_index].Invent.Object[1].Amount == 10);
    CHECK(UserList[user_index].BancoInvent.NroItems == 40);
}

TEST_CASE("QuitarBancoInvItem preserva modelo de grilla fija dispersa sin corrimiento") {
    TestEnv env;
    const std::int16_t user_index = 1;

    UserList[user_index].BancoInvent.Object[1].ObjIndex = 1;
    UserList[user_index].BancoInvent.Object[1].Amount = 10;

    UserList[user_index].BancoInvent.Object[2].ObjIndex = 2;
    UserList[user_index].BancoInvent.Object[2].Amount = 5;

    UserList[user_index].BancoInvent.Object[3].ObjIndex = 1;
    UserList[user_index].BancoInvent.Object[3].Amount = 20;

    UserList[user_index].BancoInvent.NroItems = 3;

    modBanco::QuitarBancoInvItem(user_index, 2, 5);

    CHECK(UserList[user_index].BancoInvent.Object[1].ObjIndex == 1);
    CHECK(UserList[user_index].BancoInvent.Object[1].Amount == 10);

    CHECK(UserList[user_index].BancoInvent.Object[2].ObjIndex == 0);
    CHECK(UserList[user_index].BancoInvent.Object[2].Amount == 0);

    CHECK(UserList[user_index].BancoInvent.Object[3].ObjIndex == 1);
    CHECK(UserList[user_index].BancoInvent.Object[3].Amount == 20);

    CHECK(UserList[user_index].BancoInvent.NroItems == 2);
}

TEST_CASE("Bug #31: Acreditación previa al débito en UserDejaObj provoca duplicación si el débito falla") {
    TestEnv env;
    const std::int16_t user_index = 1;

    UserList[user_index].Invent.Object[1].ObjIndex = 1;
    UserList[user_index].Invent.Object[1].Amount = 10;
    UserList[user_index].Invent.NroItems = 1;

    bool hook_invoked = false;
    modBanco::SetQuitarUserInvItemHook([&](std::int16_t, std::uint8_t, std::int16_t) {
        hook_invoked = true;
    });

    modBanco::UserDejaObj(user_index, 1, 5);

    CHECK(hook_invoked == true);

    CHECK(UserList[user_index].BancoInvent.Object[1].ObjIndex == 1);
    CHECK(UserList[user_index].BancoInvent.Object[1].Amount == 5);
    CHECK(UserList[user_index].BancoInvent.NroItems == 1);

    CHECK(UserList[user_index].Invent.Object[1].Amount == 10);

    modBanco::SetQuitarUserInvItemHook(nullptr);
}

TEST_CASE("Bug #32: modBanco permite almacenar barcos, objetos faccionarios e ítems newbie sin restricciones") {
    TestEnv env;
    const std::int16_t user_index = 1;

    UserList[user_index].Invent.Object[1].ObjIndex = 3;
    UserList[user_index].Invent.Object[1].Amount = 1;

    UserList[user_index].Invent.Object[2].ObjIndex = 4;
    UserList[user_index].Invent.Object[2].Amount = 1;

    UserList[user_index].Invent.Object[3].ObjIndex = 5;
    UserList[user_index].Invent.Object[3].Amount = 1;

    UserList[user_index].Invent.NroItems = 3;

    modBanco::UserDepositaItem(user_index, 1, 1);
    modBanco::UserDepositaItem(user_index, 2, 1);
    modBanco::UserDepositaItem(user_index, 3, 1);

    CHECK(UserList[user_index].BancoInvent.Object[1].ObjIndex == 3);
    CHECK(UserList[user_index].BancoInvent.Object[1].Amount == 1);

    CHECK(UserList[user_index].BancoInvent.Object[2].ObjIndex == 4);
    CHECK(UserList[user_index].BancoInvent.Object[2].Amount == 1);

    CHECK(UserList[user_index].BancoInvent.Object[3].ObjIndex == 5);
    CHECK(UserList[user_index].BancoInvent.Object[3].Amount == 1);

    CHECK(UserList[user_index].BancoInvent.NroItems == 3);
}

}

TEST_SUITE("modBanco - G3") {

TEST_CASE("SendUserBovedaTxt lista en consola sólo los slots ocupados de usuario online") {
    TestEnv env;
    const std::int16_t gm_index = 1;
    const std::int16_t target_user = 2;

    UserList[target_user].name = "Paladin";
    UserList[target_user].BancoInvent.Object[1].ObjIndex = 1; // Poción Roja
    UserList[target_user].BancoInvent.Object[1].Amount = 50;

    UserList[target_user].BancoInvent.Object[4].ObjIndex = 2; // Espada Larga
    UserList[target_user].BancoInvent.Object[4].Amount = 1;

    UserList[target_user].BancoInvent.NroItems = 2;

    modBanco::SendUserBovedaTxt(gm_index, target_user);

    auto msgs = read_console_messages(UserList[gm_index].outgoingData.get());
    REQUIRE(msgs.size() == 4);
    CHECK(msgs[0] == "Paladin");
    CHECK(msgs[1] == "Tiene 2 objetos.");
    CHECK(msgs[2] == "Objeto 1 Poción Roja Cantidad:50");
    CHECK(msgs[3] == "Objeto 4 Espada Larga Cantidad:1");
}

TEST_CASE("SendUserBovedaTxtFromChar lee perfiles sintéticos mediante CharReaderHook") {
    TestEnv env;
    const std::int16_t gm_index = 1;
    const std::string target_char = "MagoOscuro";

    std::map<std::string, std::string> synthetic_ini = {
        {"BancoInventory_CantidadItems", "2"},
        {"BancoInventory_Obj1", "1-30"}, // Poción Roja x30
        {"BancoInventory_Obj10", "4-1"}   // Armadura Real x1
    };

    modBanco::SetCharReaderHook([&](const std::string&, const std::string& section, const std::string& key) -> std::string {
        const std::string full_key = section + "_" + key;
        auto it = synthetic_ini.find(full_key);
        if (it != synthetic_ini.end()) {
            return it->second;
        }
        return "";
    });

    modBanco::SendUserBovedaTxtFromChar(gm_index, target_char);

    auto msgs = read_console_messages(UserList[gm_index].outgoingData.get());
    REQUIRE(msgs.size() == 4);
    CHECK(msgs[0] == "MagoOscuro");
    CHECK(msgs[1] == "Tiene 2 objetos.");
    CHECK(msgs[2] == "Objeto 1 Poción Roja Cantidad:30");
    CHECK(msgs[3] == "Objeto 10 Armadura Real Cantidad:1");

    modBanco::SetCharReaderHook(nullptr);
}

TEST_CASE("SendUserBovedaTxtFromChar reporta usuario inexistente") {
    TestEnv env;
    const std::int16_t gm_index = 1;
    const std::string ghost_char = "Fantasma";

    modBanco::SetCharReaderHook([](const std::string&, const std::string&, const std::string&) -> std::string {
        return ""; // archivo o sección inexistente
    });

    modBanco::SendUserBovedaTxtFromChar(gm_index, ghost_char);

    auto msgs = read_console_messages(UserList[gm_index].outgoingData.get());
    REQUIRE(msgs.size() == 1);
    CHECK(msgs[0] == "Usuario inexistente: Fantasma");

    modBanco::SetCharReaderHook(nullptr);
}

TEST_CASE("SendUserBovedaTxt y SendUserBovedaTxtFromChar con índices inválidos no fallan") {
    modBanco::SendUserBovedaTxt(0, 1);
    modBanco::SendUserBovedaTxt(1, 0);
    modBanco::SendUserBovedaTxt(999, 1);
    modBanco::SendUserBovedaTxt(1, 999);
    modBanco::SendUserBovedaTxtFromChar(0, "Nadie");
    modBanco::SendUserBovedaTxtFromChar(999, "Nadie");
}

}
