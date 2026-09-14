#include <doctest/doctest.h>
#include "server/Comercio.hpp"
#include "server/InvUsuario.hpp"
#include "server/Modulo_InventANDobj.hpp"
#include "server/Declares.hpp"
#include "server/Protocol.hpp"
#include "server/clsByteQueue.hpp"
#include <vector>
#include <string>
#include <memory>

namespace {

using namespace ao;
using namespace ao::net::protocol;

struct TestEnv {
    TestEnv() {
        MaxUsers = 5;
        LastUser = 5;
        UserList.clear();
        UserList.resize(10);
        ObjDataList.assign(100, ObjData{});
        NumObjDatas = 99;

        // Resetear hooks de dependencias
        ao::SetSubirSkillHook(nullptr);
        ao::SetKeyPurchaseHook(nullptr);
        ao::SetBanUserHook(nullptr);
        InvUsuario::SetQuitarUserInvItemHook(nullptr);
        InvUsuario::SetUpdateUserInvHook(nullptr);
        InvUsuario::SetMeterItemEnInventarioHook(nullptr);

        // Ítems de prueba
        ObjDataList[1].name = "Poción Roja";
        ObjDataList[1].OBJType = eOBJType::otPociones;
        ObjDataList[1].Valor = 100;

        ObjDataList[2].name = "Espada de Hierro";
        ObjDataList[2].OBJType = eOBJType::otWeapon;
        ObjDataList[2].Valor = 300;

        ObjDataList[3].name = "Daga Newbie";
        ObjDataList[3].OBJType = eOBJType::otWeapon;
        ObjDataList[3].Valor = 150;
        ObjDataList[3].Newbie = 1;

        ObjDataList[4].name = "Armadura Real";
        ObjDataList[4].OBJType = eOBJType::otArmadura;
        ObjDataList[4].Valor = 1000;
        ObjDataList[4].Real = 1;

        ObjDataList[5].name = "Armadura del Caos";
        ObjDataList[5].OBJType = eOBJType::otArmadura;
        ObjDataList[5].Valor = 1200;
        ObjDataList[5].Caos = 1;

        ObjDataList[6].name = "Llave de Mansión";
        ObjDataList[6].OBJType = eOBJType::otLlaves;
        ObjDataList[6].Valor = 5000;

        ObjDataList[iORO].name = "Oro";
        ObjDataList[iORO].OBJType = eOBJType::otPociones; // Distinto de cualquiera
        ObjDataList[iORO].Valor = 1;

        // Setup usuarios
        for (std::int16_t u = 1; u <= 5; ++u) {
            UserList[u].outgoingData = std::make_unique<clsByteQueue>();
            UserList[u].flags.UserLogged = true;
            UserList[u].flags.Comerciando = false;
            UserList[u].flags.TargetNPC = 1;
            UserList[u].flags.Privilegios = PlayerType::UserPlayer;
            UserList[u].Stats.GLD = 10000;
            UserList[u].Stats.UserSkills[eSkill::Comerciar] = 0;
            UserList[u].CurrentInventorySlots = 30;
            UserList[u].Invent.NroItems = 0;
            UserList[u].name = "Heroe" + std::to_string(u);

            for (std::uint8_t s = 1; s <= MAX_INVENTORY_SLOTS; ++s) {
                UserList[u].Invent.Object[s] = UserOBJ{};
            }
        }

        // Setup NPCs
        for (std::int16_t n = 1; n <= 5; ++n) {
            Npclist[n].Comercia = 1;
            Npclist[n].TipoItems = static_cast<std::int16_t>(eOBJType::otCualquiera);
            Npclist[n].name = "Mercader";
            Npclist[n].Invent.NroItems = 0;
            for (std::uint8_t s = 1; s <= MAX_INVENTORY_SLOTS; ++s) {
                Npclist[n].Invent.Object[s] = UserOBJ{};
            }
        }
    }
};

} // namespace

TEST_SUITE("Comercio NPC - G1") {

    TEST_CASE("Calculo de descuento segun skill Comerciar") {
        TestEnv env;
        UserList[1].Stats.UserSkills[eSkill::Comerciar] = 0;
        CHECK(Descuento(1) == doctest::Approx(1.0f));

        UserList[1].Stats.UserSkills[eSkill::Comerciar] = 50;
        CHECK(Descuento(1) == doctest::Approx(1.5f));

        UserList[1].Stats.UserSkills[eSkill::Comerciar] = 100;
        CHECK(Descuento(1) == doctest::Approx(2.0f));
    }

    TEST_CASE("Formulas de precios de compra y venta") {
        TestEnv env;
        // SalePrice: división por 3
        CHECK(SalePrice(2) == doctest::Approx(100.0f)); // 300 / 3

        // SalePrice: 0 para ítems Newbie
        CHECK(SalePrice(3) == 0.0f);

        // Compra: cálculo con skill 0 (descuento 1.0)
        UserList[1].Stats.UserSkills[eSkill::Comerciar] = 0;
        // Valor 100 / 1.0 * 2 = 200 + 0.5 = 200.5 -> 200
        double unit_val = static_cast<double>(ObjDataList[1].Valor) / static_cast<double>(Descuento(1));
        std::int64_t precio = static_cast<std::int64_t>((unit_val * 2.0) + 0.5);
        CHECK(precio == 200);

        // Compra: redondeo al techo (+0.5f) cuando da decimal
        UserList[1].Stats.UserSkills[eSkill::Comerciar] = 50; // Descuento 1.5
        // Valor 100 / 1.5 = 66.666... + 0.5 = 67.166... -> 67
        unit_val = static_cast<double>(ObjDataList[1].Valor) / static_cast<double>(Descuento(1));
        precio = static_cast<std::int64_t>((unit_val * 1.0) + 0.5);
        CHECK(precio == 67);
    }

    TEST_CASE("Comercio NPC - Asimetria de slots 20 vs 30 (Bug #33)") {
        TestEnv env;
        // Llenar los 30 slots del inventario del NPC 1
        for (std::uint8_t s = 1; s <= MAX_INVENTORY_SLOTS; ++s) {
            Npclist[1].Invent.Object[s].ObjIndex = 1; // Poción roja
            Npclist[1].Invent.Object[s].Amount = 100;
        }
        Npclist[1].Invent.NroItems = 30;

        // Despachar inventario al cliente
        EnviarNpcInv(1, 1);

        // Leer paquetes del outgoingData de UserList[1]
        auto* q = UserList[1].outgoingData.get();
        std::int32_t packet_count = 0;

        while (q->length() > 0) {
            auto opcode = q->ReadByte();
            if (opcode == static_cast<std::uint8_t>(ServerPacketID::ChangeNPCInventorySlot)) {
                packet_count++;
                q->ReadByte();        // Slot
                q->ReadASCIIString(); // Name
                q->ReadInteger();     // Amount
                q->ReadSingle();      // Price
                q->ReadInteger();     // GrhIndex
                q->ReadInteger();     // ObjIndex
                q->ReadByte();        // ObjType
                q->ReadInteger();     // MaxHit
                q->ReadInteger();     // MinHit
                q->ReadInteger();     // MaxDef
                q->ReadInteger();     // MinDef
            } else {
                break;
            }
        }

        // En estricta paridad con el Bug #33, solo se deben haber enviado exactamente 20 paquetes
        CHECK(packet_count == MAX_NORMAL_INVENTORY_SLOTS);
        CHECK(packet_count == 20);
    }

    TEST_CASE("Comercio NPC - Deteccion de autoban ante compras anómalas > 10000") {
        TestEnv env;
        Npclist[1].Invent.Object[1].ObjIndex = 1;
        Npclist[1].Invent.Object[1].Amount = 10000;
        UserList[1].Stats.GLD = 1000000;

        bool ban_disparado = false;
        std::string ban_reason;
        auto ban_hook = [&](std::int16_t u, std::string_view reason) {
            ban_disparado = true;
            ban_reason = reason;
        };

        Comercio(eModoComercio::Compra, 1, 1, 1, 10001, nullptr, ban_hook);

        CHECK(ban_disparado);
        CHECK(ban_reason.find("10001") != std::string::npos);
        CHECK(UserList[1].Stats.GLD == 1000000); // Sin cobro
        CHECK(Npclist[1].Invent.Object[1].Amount == 10000); // Sin descuento
    }

    TEST_CASE("Comercio NPC - Compra con mochila llena aborta sin debitar oro") {
        TestEnv env;
        Npclist[1].Invent.Object[1].ObjIndex = 2; // Espada
        Npclist[1].Invent.Object[1].Amount = 10;
        UserList[1].Stats.GLD = 10000;

        // Llenar la mochila del usuario con 30 ítems distintos
        for (std::uint8_t s = 1; s <= MAX_INVENTORY_SLOTS; ++s) {
            UserList[1].Invent.Object[s].ObjIndex = static_cast<std::int16_t>(10 + s);
            UserList[1].Invent.Object[s].Amount = 1;
        }
        UserList[1].Invent.NroItems = 30;

        Comercio(eModoComercio::Compra, 1, 1, 1, 1);

        CHECK(UserList[1].Stats.GLD == 10000); // No se debitó oro
        CHECK(Npclist[1].Invent.Object[1].Amount == 10); // No se descontó del NPC
    }

    TEST_CASE("Comercio NPC - Venta con acreditacion clampleada a MAXORO") {
        TestEnv env;
        // Usuario a 50 monedas del tope máximo MAXORO (90.000.000)
        UserList[1].Stats.GLD = 89999950;
        // Colocar espada de hierro (Valor 300, precio de venta = 100)
        UserList[1].Invent.Object[1].ObjIndex = 2;
        UserList[1].Invent.Object[1].Amount = 1;
        UserList[1].Invent.NroItems = 1;

        Comercio(eModoComercio::Venta, 1, 1, 1, 1);

        // El oro se incrementó de 89.999.950 + 100 = 90.000.050 -> Clamped a 90.000.000
        CHECK(UserList[1].Stats.GLD == MAXORO);
        CHECK(UserList[1].Invent.Object[1].Amount == 0);
        CHECK(UserList[1].Invent.Object[1].ObjIndex == 0);
    }

    TEST_CASE("Comercio NPC - Restricciones de sastres faccionarios") {
        TestEnv env;
        Npclist[1].name = "Mercader";
        Npclist[2].name = "SR"; // Sastre Real
        Npclist[3].name = "SC"; // Sastre Caótico

        // Usuario con Armadura Real (Obj 4) en slot 1 y Armadura Caos (Obj 5) en slot 2
        UserList[1].Invent.Object[1].ObjIndex = 4;
        UserList[1].Invent.Object[1].Amount = 1;
        UserList[1].Invent.Object[2].ObjIndex = 5;
        UserList[1].Invent.Object[2].Amount = 1;
        UserList[1].Invent.NroItems = 2;

        // 1. Intentar vender Armadura Real al mercader genérico -> Rechazado
        Comercio(eModoComercio::Venta, 1, 1, 1, 1);
        CHECK(UserList[1].Invent.Object[1].Amount == 1);

        // 2. Vender Armadura Real al Sastre Real ("SR") -> Aceptado
        Comercio(eModoComercio::Venta, 1, 2, 1, 1);
        CHECK(UserList[1].Invent.Object[1].Amount == 0);

        // 3. Intentar vender Armadura del Caos al Sastre Real ("SR") -> Rechazado
        Comercio(eModoComercio::Venta, 1, 2, 2, 1);
        CHECK(UserList[1].Invent.Object[2].Amount == 1);

        // 4. Vender Armadura del Caos al Sastre Caótico ("SC") -> Aceptado
        Comercio(eModoComercio::Venta, 1, 3, 2, 1);
        CHECK(UserList[1].Invent.Object[2].Amount == 0);
    }

    TEST_CASE("Comercio NPC - Intercepcion de compra de llaves via KeyPurchaseHook") {
        TestEnv env;
        Npclist[1].Invent.Object[1].ObjIndex = 6; // Llave de mansión (otLlaves)
        Npclist[1].Invent.Object[1].Amount = 1;
        UserList[1].Stats.GLD = 20000;

        bool key_hook_invoked = false;
        std::int16_t hooked_user = 0;
        std::int16_t hooked_npc = 0;
        std::uint8_t hooked_slot = 0;
        std::int16_t hooked_obj = 0;

        auto key_hook = [&](std::int16_t u, std::int16_t n, std::uint8_t s, std::int16_t o) {
            key_hook_invoked = true;
            hooked_user = u;
            hooked_npc = n;
            hooked_slot = s;
            hooked_obj = o;
        };

        Comercio(eModoComercio::Compra, 1, 1, 1, 1, key_hook);

        CHECK(key_hook_invoked);
        CHECK(hooked_user == 1);
        CHECK(hooked_npc == 1);
        CHECK(hooked_slot == 1);
        CHECK(hooked_obj == 6);
        CHECK(UserList[1].Invent.Object[1].ObjIndex == 6);
    }

}
