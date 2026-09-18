#include <doctest/doctest.h>
#include "server/mdParty.hpp"
#include "server/Declares.hpp"
#include <cmath>

TEST_SUITE("[Party_G1]") {

    TEST_CASE("G1_NextParty_FindsFirstEmptySlot") {
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) {
            Parties[i] = nullptr;
        }

        CHECK(ModParty::NextParty() == 1);

        Parties[1] = std::make_shared<clsParty>();
        CHECK(ModParty::NextParty() == 2);

        for (std::int16_t i = 2; i <= ModParty::MAX_PARTIES; ++i) {
            Parties[i] = std::make_shared<clsParty>();
        }
        CHECK(ModParty::NextParty() == -1);

        // Reset
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) {
            Parties[i] = nullptr;
        }
    }

    TEST_CASE("G1_PuedeCrearParty_ValidatesAttributes") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;

        std::int16_t user_index = 1;
        UserList[user_index] = User{};

        UserList[user_index].Stats.UserAtributos[eAtributos::Carisma] = 10;
        UserList[user_index].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[user_index].flags.Muerto = 0;

        CHECK(ModParty::PuedeCrearParty(user_index) == true);

        // Fail due to low carisma/liderazgo product (< 100)
        UserList[user_index].Stats.UserAtributos[eAtributos::Carisma] = 9;
        CHECK(ModParty::PuedeCrearParty(user_index) == false);

        // Fail due to being dead
        UserList[user_index].Stats.UserAtributos[eAtributos::Carisma] = 10;
        UserList[user_index].flags.Muerto = 1;
        CHECK(ModParty::PuedeCrearParty(user_index) == false);

        UserList[user_index] = User{};
    }

    TEST_CASE("G1_CrearParty_InstantiatesLeader") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;

        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) {
            Parties[i] = nullptr;
        }

        ExponenteNivelParty = 1.4f;

        std::int16_t user_index = 1;
        UserList[user_index] = User{};
        UserList[user_index].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[user_index].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[user_index].Stats.ELV = 20;
        UserList[user_index].flags.Muerto = 0;
        UserList[user_index].PartyIndex = 0;

        ModParty::CrearParty(user_index);

        CHECK(UserList[user_index].PartyIndex == 1);
        CHECK(Parties[1] != nullptr);
        CHECK(Parties[1]->Fundador() == user_index);
        CHECK(Parties[1]->CantMiembros() == 1);
        CHECK(Parties[1]->EsPartyLeader(user_index) == true);

        float expected_sum = std::pow(20.0f, ExponenteNivelParty);
        CHECK(doctest::Approx(Parties[1]->SumaNivelesElevados()).epsilon(0.001) == expected_sum);

        // Cleanup
        Parties[1] = nullptr;
        UserList[user_index] = User{};
    }

}

TEST_SUITE("[Party_G2]") {

    TEST_CASE("G2_SolicitarIngreso") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        std::int16_t leader = 1;
        std::int16_t applicant = 2;

        UserList[leader] = User{};
        UserList[applicant] = User{};

        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20;
        UserList[leader].flags.Muerto = 0;

        ModParty::CrearParty(leader);

        UserList[applicant].flags.Muerto = 0;
        UserList[applicant].PartyIndex = 0;
        UserList[applicant].flags.TargetUser = leader;

        ModParty::SolicitarIngresoAParty(applicant);

        CHECK(UserList[applicant].PartySolicitud == UserList[leader].PartyIndex);

        // Cleanup
        Parties[1] = nullptr;
        UserList[leader] = User{};
        UserList[applicant] = User{};
    }

    TEST_CASE("G2_PuedeEntrar_Distance") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        std::int16_t leader = 1;
        std::int16_t applicant = 2;

        UserList[leader] = User{};
        UserList[applicant] = User{};

        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20;
        UserList[leader].Pos = WorldPos{1, 50, 50};
        ModParty::CrearParty(leader);

        // Distance = 1 tile (valid <= 2)
        UserList[applicant].Pos = WorldPos{1, 50, 51};
        std::string razon;
        CHECK(Parties[1]->PuedeEntrar(applicant, razon) == true);

        // Distance = 3 tiles (exceeds max 2 tiles)
        UserList[applicant].Pos = WorldPos{1, 50, 53};
        CHECK(Parties[1]->PuedeEntrar(applicant, razon) == false);

        // Cleanup
        Parties[1] = nullptr;
        UserList[leader] = User{};
        UserList[applicant] = User{};
    }

    TEST_CASE("G2_PuedeEntrar_Cupo") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        std::int16_t leader = 1;
        UserList[leader] = User{};
        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20;
        UserList[leader].Pos = WorldPos{1, 50, 50};
        ModParty::CrearParty(leader);

        // Add 4 members so total is 5 (full capacity)
        for (std::int16_t u = 2; u <= 5; ++u) {
            UserList[u] = User{};
            UserList[u].Pos = WorldPos{1, 50, 50};
            Parties[1]->NuevoMiembro(u);
        }

        CHECK(Parties[1]->CantMiembros() == 5);

        // Try adding 6th member
        std::int16_t sixth = 6;
        UserList[sixth] = User{};
        UserList[sixth].Pos = WorldPos{1, 50, 50};

        std::string razon;
        CHECK(Parties[1]->PuedeEntrar(sixth, razon) == false);
        CHECK(razon == "La party está llena.");

        // Cleanup
        Parties[1] = nullptr;
        for (std::int16_t i = 1; i <= 6; ++i) UserList[i] = User{};
    }

    TEST_CASE("G2_PuedeEntrar_FactionMatrix") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        // Case 1: Applicant is Armada Real, active member is Criminal
        {
            std::int16_t leader = 1;
            std::int16_t applicant = 2;
            UserList[leader] = User{};
            UserList[applicant] = User{};
            UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
            UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
            UserList[leader].Pos = WorldPos{1, 50, 50};
            UserList[leader].Reputacion.NobleRep = 0;
            UserList[leader].Reputacion.PlebeRep = -100; // criminal

            ModParty::CrearParty(leader);

            UserList[applicant].Pos = WorldPos{1, 50, 50};
            UserList[applicant].Faccion.ArmadaReal = 1;
            UserList[applicant].Reputacion.NobleRep = 1000;

            std::string razon;
            CHECK(Parties[1]->PuedeEntrar(applicant, razon) == false);

            Parties[1] = nullptr;
        }

        // Case 2: Applicant is Legión Oscura, active member is Citizen
        {
            std::int16_t leader = 1;
            std::int16_t applicant = 2;
            UserList[leader] = User{};
            UserList[applicant] = User{};
            UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
            UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
            UserList[leader].Pos = WorldPos{1, 50, 50};
            UserList[leader].Reputacion.NobleRep = 50; // citizen

            ModParty::CrearParty(leader);

            UserList[applicant].Pos = WorldPos{1, 50, 50};
            UserList[applicant].Faccion.FuerzasCaos = 1;
            UserList[applicant].Reputacion.PlebeRep = -500;

            std::string razon;
            CHECK(Parties[1]->PuedeEntrar(applicant, razon) == false);

            Parties[1] = nullptr;
        }

        // Case 3: Applicant is Criminal, active member is Armada Real
        {
            std::int16_t leader = 1;
            std::int16_t applicant = 2;
            UserList[leader] = User{};
            UserList[applicant] = User{};
            UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
            UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
            UserList[leader].Pos = WorldPos{1, 50, 50};
            UserList[leader].Faccion.ArmadaReal = 1;
            UserList[leader].Reputacion.NobleRep = 500;

            ModParty::CrearParty(leader);

            UserList[applicant].Pos = WorldPos{1, 50, 50};
            UserList[applicant].Reputacion.PlebeRep = -100; // criminal

            std::string razon;
            CHECK(Parties[1]->PuedeEntrar(applicant, razon) == false);

            Parties[1] = nullptr;
        }

        // Case 4: Applicant is Citizen, active member is Legión Oscura
        {
            std::int16_t leader = 1;
            std::int16_t applicant = 2;
            UserList[leader] = User{};
            UserList[applicant] = User{};
            UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
            UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
            UserList[leader].Pos = WorldPos{1, 50, 50};
            UserList[leader].Faccion.FuerzasCaos = 1;
            UserList[leader].Reputacion.PlebeRep = -500;

            ModParty::CrearParty(leader);

            UserList[applicant].Pos = WorldPos{1, 50, 50};
            UserList[applicant].Reputacion.NobleRep = 100; // citizen

            std::string razon;
            CHECK(Parties[1]->PuedeEntrar(applicant, razon) == false);

            Parties[1] = nullptr;
        }

        // Cleanup
        for (std::int16_t i = 1; i <= 2; ++i) UserList[i] = User{};
    }

    TEST_CASE("G2_AprobarIngreso_Exito") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        ExponenteNivelParty = 1.4f;

        std::int16_t leader = 1;
        std::int16_t applicant = 2;

        UserList[leader] = User{};
        UserList[applicant] = User{};

        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20;
        UserList[leader].Pos = WorldPos{1, 50, 50};
        UserList[leader].flags.Muerto = 0;

        ModParty::CrearParty(leader);

        UserList[applicant].Stats.ELV = 22;
        UserList[applicant].Pos = WorldPos{1, 50, 51};
        UserList[applicant].flags.Muerto = 0;
        UserList[applicant].PartyIndex = 0;
        UserList[applicant].flags.TargetUser = leader;

        ModParty::SolicitarIngresoAParty(applicant);
        CHECK(UserList[applicant].PartySolicitud == 1);

        ModParty::AprobarIngresoAParty(leader, applicant);

        CHECK(UserList[applicant].PartyIndex == 1);
        CHECK(UserList[applicant].PartySolicitud == 0);
        CHECK(Parties[1]->CantMiembros() == 2);

        float expected_sum = std::pow(20.0f, ExponenteNivelParty) + std::pow(22.0f, ExponenteNivelParty);
        CHECK(doctest::Approx(Parties[1]->SumaNivelesElevados()).epsilon(0.001) == expected_sum);

        // Cleanup
        Parties[1] = nullptr;
        UserList[leader] = User{};
        UserList[applicant] = User{};
    }

}

TEST_SUITE("[Party_G3]") {

    TEST_CASE("G3_SaleMiembro_Comun") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        ExponenteNivelParty = 1.4f;

        std::int16_t leader = 1;
        std::int16_t member2 = 2;
        std::int16_t member3 = 3;

        UserList[leader] = User{};
        UserList[member2] = User{};
        UserList[member3] = User{};

        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20;
        UserList[leader].Pos = WorldPos{1, 50, 50};

        ModParty::CrearParty(leader);

        UserList[member2].Stats.ELV = 22;
        UserList[member2].Pos = WorldPos{1, 50, 50};
        UserList[member2].PartySolicitud = 1;
        ModParty::AprobarIngresoAParty(leader, member2);

        UserList[member3].Stats.ELV = 25;
        UserList[member3].Pos = WorldPos{1, 50, 50};
        UserList[member3].PartySolicitud = 1;
        ModParty::AprobarIngresoAParty(leader, member3);

        CHECK(Parties[1]->CantMiembros() == 3);

        bool dissolved = Parties[1]->SaleMiembro(member2);
        CHECK(dissolved == false);
        CHECK(Parties[1]->CantMiembros() == 2);
        CHECK(UserList[member2].PartyIndex == 0);

        float expected_sum = std::pow(20.0f, ExponenteNivelParty) + std::pow(25.0f, ExponenteNivelParty);
        CHECK(doctest::Approx(Parties[1]->SumaNivelesElevados()).epsilon(0.001) == expected_sum);

        // Verify compaction: leader is slot 0, member3 is slot 1
        const auto& members = Parties[1]->GetMembers();
        CHECK(members[0].UserIndex == leader);
        CHECK(members[1].UserIndex == member3);
        CHECK(members[2].UserIndex == 0);

        // Cleanup
        Parties[1] = nullptr;
        for (std::int16_t i = 1; i <= 3; ++i) UserList[i] = User{};
    }

    TEST_CASE("G3_SaleMiembro_Lider_Disolucion") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        ExponenteNivelParty = 1.4f;

        std::int16_t leader = 1;
        std::int16_t member2 = 2;

        UserList[leader] = User{};
        UserList[member2] = User{};

        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20; // Leader level = 20
        UserList[leader].Pos = WorldPos{1, 50, 50};

        ModParty::CrearParty(leader);

        UserList[member2].Stats.ELV = 25; // Member level = 25
        UserList[member2].Pos = WorldPos{1, 50, 50};
        UserList[member2].PartySolicitud = 1;
        ModParty::AprobarIngresoAParty(leader, member2);

        float initial_sum = Parties[1]->SumaNivelesElevados();

        // Leader leaves -> returns true
        bool dissolved = Parties[1]->SaleMiembro(leader);
        CHECK(dissolved == true);
        CHECK(UserList[leader].PartyIndex == 0);
        CHECK(UserList[member2].PartyIndex == 0);

        // Replicating KNOWN BUG #46:
        // SumaNivelesElevados subtracted 2 * (leader_level ^ 1.4) instead of (leader_level^1.4 + member2_level^1.4)
        float expected_sum = initial_sum - (2.0f * std::pow(20.0f, ExponenteNivelParty));
        CHECK(doctest::Approx(Parties[1]->SumaNivelesElevados()).epsilon(0.001) == expected_sum);

        // Cleanup
        Parties[1] = nullptr;
        for (std::int16_t i = 1; i <= 2; ++i) UserList[i] = User{};
    }

    TEST_CASE("G3_SalirDeParty_DestruccionInstancia") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        std::int16_t leader = 1;
        UserList[leader] = User{};
        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20;

        ModParty::CrearParty(leader);
        CHECK(Parties[1] != nullptr);

        ModParty::SalirDeParty(leader);

        CHECK(Parties[1] == nullptr);
        CHECK(UserList[leader].PartyIndex == 0);

        // Cleanup
        UserList[leader] = User{};
    }

    TEST_CASE("G3_ExpulsarDeParty") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        std::int16_t leader = 1;
        std::int16_t member2 = 2;

        UserList[leader] = User{};
        UserList[member2] = User{};

        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20;
        UserList[leader].Pos = WorldPos{1, 50, 50};

        ModParty::CrearParty(leader);

        UserList[member2].Stats.ELV = 22;
        UserList[member2].Pos = WorldPos{1, 50, 50};
        UserList[member2].PartySolicitud = 1;
        ModParty::AprobarIngresoAParty(leader, member2);

        // Leader expels member2
        ModParty::ExpulsarDeParty(leader, member2);

        CHECK(Parties[1] != nullptr);
        CHECK(Parties[1]->CantMiembros() == 1);
        CHECK(UserList[member2].PartyIndex == 0);

        // Non-leader attempts to expel -> rejected
        UserList[member2].PartyIndex = 1; // fake join
        ModParty::ExpulsarDeParty(member2, leader);
        CHECK(Parties[1]->CantMiembros() == 1); // no effect

        // Cleanup
        Parties[1] = nullptr;
        for (std::int16_t i = 1; i <= 2; ++i) UserList[i] = User{};
    }

    TEST_CASE("G3_TransformarEnLider") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        std::int16_t leader = 1;
        std::int16_t member2 = 2;

        UserList[leader] = User{};
        UserList[member2] = User{};

        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20;
        UserList[leader].Pos = WorldPos{1, 50, 50};

        ModParty::CrearParty(leader);

        UserList[member2].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[member2].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[member2].Stats.ELV = 22;
        UserList[member2].Pos = WorldPos{1, 50, 50};
        UserList[member2].PartySolicitud = 1;
        ModParty::AprobarIngresoAParty(leader, member2);

        // Transfer leadership to member2
        ModParty::TransformarEnLider(leader, member2);

        CHECK(Parties[1]->Fundador() == member2);
        CHECK(Parties[1]->EsPartyLeader(member2) == true);
        CHECK(Parties[1]->EsPartyLeader(leader) == false);

        // Attempt transfer to dead member -> rejected
        std::int16_t dead_member = 3;
        UserList[dead_member] = User{};
        UserList[dead_member].flags.Muerto = 1;
        UserList[dead_member].PartySolicitud = 1;
        Parties[1]->NuevoMiembro(dead_member);
        UserList[dead_member].PartyIndex = 1;

        ModParty::TransformarEnLider(member2, dead_member);
        CHECK(Parties[1]->Fundador() == member2); // leadership unchanged

        // Cleanup
        Parties[1] = nullptr;
        for (std::int16_t i = 1; i <= 3; ++i) UserList[i] = User{};
    }

}

TEST_SUITE("[Party_G4]") {

    TEST_CASE("G4_ObtenerExito_RepartoPonderado") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        ExponenteNivelParty = 1.4f;

        std::int16_t leader = 1;
        std::int16_t member2 = 2;

        UserList[leader] = User{};
        UserList[member2] = User{};

        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20; // lvl 20
        UserList[leader].Pos = WorldPos{1, 50, 50};

        ModParty::CrearParty(leader);

        UserList[member2].Stats.ELV = 30; // lvl 30
        UserList[member2].Pos = WorldPos{1, 50, 50};
        UserList[member2].PartySolicitud = 1;
        ModParty::AprobarIngresoAParty(leader, member2);

        // Calculate expected proportions
        float elv1_pow = std::pow(20.0f, 1.4f);
        float elv2_pow = std::pow(30.0f, 1.4f);
        float total_pow = elv1_pow + elv2_pow;

        std::int32_t total_exp = 10000;
        double expected_share1 = static_cast<double>(total_exp) * (elv1_pow / total_pow);
        double expected_share2 = static_cast<double>(total_exp) * (elv2_pow / total_pow);

        ModParty::ObtenerExito(leader, total_exp, 1, 50, 50);

        CHECK(Parties[1]->ObtenerExperienciaTotal() == total_exp);
        CHECK(Parties[1]->MiExperiencia(leader) == static_cast<std::int32_t>(std::trunc(expected_share1)));
        CHECK(Parties[1]->MiExperiencia(member2) == static_cast<std::int32_t>(std::trunc(expected_share2)));

        // Cleanup
        Parties[1] = nullptr;
        for (std::int16_t i = 1; i <= 2; ++i) UserList[i] = User{};
    }

    TEST_CASE("G4_ObtenerExito_FiltrosEspaciales") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        ExponenteNivelParty = 1.4f;

        std::int16_t leader = 1;
        std::int16_t dead_member = 2;
        std::int16_t far_member = 3;
        std::int16_t other_map_member = 4;

        UserList[leader] = User{};
        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20;
        UserList[leader].Pos = WorldPos{1, 50, 50};

        ModParty::CrearParty(leader);

        // Setup members
        UserList[dead_member] = User{};
        UserList[dead_member].Stats.ELV = 20;
        UserList[dead_member].Pos = WorldPos{1, 50, 50};
        UserList[dead_member].flags.Muerto = 1; // Dead
        Parties[1]->NuevoMiembro(dead_member);
        UserList[dead_member].PartyIndex = 1;

        UserList[far_member] = User{};
        UserList[far_member].Stats.ELV = 20;
        UserList[far_member].Pos = WorldPos{1, 50, 75}; // Distance = 25 tiles (> 18)
        Parties[1]->NuevoMiembro(far_member);
        UserList[far_member].PartyIndex = 1;

        UserList[other_map_member] = User{};
        UserList[other_map_member].Stats.ELV = 20;
        UserList[other_map_member].Pos = WorldPos{2, 50, 50}; // Map 2 != Map 1
        Parties[1]->NuevoMiembro(other_map_member);
        UserList[other_map_member].PartyIndex = 1;

        ModParty::ObtenerExito(leader, 10000, 1, 50, 50);

        // Only leader is eligible, so dead, far, and other map members get 0
        CHECK(Parties[1]->MiExperiencia(dead_member) == 0);
        CHECK(Parties[1]->MiExperiencia(far_member) == 0);
        CHECK(Parties[1]->MiExperiencia(other_map_member) == 0);
        CHECK(Parties[1]->MiExperiencia(leader) > 0);

        // Cleanup
        Parties[1] = nullptr;
        for (std::int16_t i = 1; i <= 4; ++i) UserList[i] = User{};
    }

    TEST_CASE("G4_FlushExperiencia_ResiduoDecimal") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        ExponenteNivelParty = 1.4f;

        std::int16_t leader = 1;
        std::int16_t member2 = 2;

        UserList[leader] = User{};
        UserList[member2] = User{};

        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20;
        UserList[leader].Pos = WorldPos{1, 50, 50};

        ModParty::CrearParty(leader);

        UserList[member2].Stats.ELV = 22;
        UserList[member2].Pos = WorldPos{1, 50, 50};
        UserList[member2].PartySolicitud = 1;
        ModParty::AprobarIngresoAParty(leader, member2);

        // Give fractional experience by granting an odd exp total
        ModParty::ObtenerExito(leader, 1005, 1, 50, 50);

        // Check experience before flush
        auto initial_exp_leader = UserList[leader].Stats.Exp;
        auto initial_exp_member2 = UserList[member2].Stats.Exp;

        ModParty::ActualizaExperiencias();

        // Exp in UserList.Stats.Exp should increase by integer part
        CHECK(UserList[leader].Stats.Exp > initial_exp_leader);
        CHECK(UserList[member2].Stats.Exp > initial_exp_member2);

        // Remaining experience in members array should be < 1.0 (decimal remainder retained!)
        const auto& members = Parties[1]->GetMembers();
        CHECK(members[0].Experiencia >= 0.0);
        CHECK(members[0].Experiencia < 1.0);
        CHECK(members[1].Experiencia >= 0.0);
        CHECK(members[1].Experiencia < 1.0);

        // Cleanup
        Parties[1] = nullptr;
        for (std::int16_t i = 1; i <= 2; ++i) UserList[i] = User{};
    }

    TEST_CASE("G4_UpdateSumaNivelesElevados") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        ExponenteNivelParty = 1.4f;

        std::int16_t leader = 1;
        UserList[leader] = User{};
        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20;

        ModParty::CrearParty(leader);

        float initial_sum = Parties[1]->SumaNivelesElevados();
        CHECK(doctest::Approx(initial_sum).epsilon(0.001) == std::pow(20.0f, 1.4f));

        // Leader levels up from 20 to 21
        UserList[leader].Stats.ELV = 21;
        ModParty::ActualizarSumaNivelesElevados(leader);

        float new_sum = Parties[1]->SumaNivelesElevados();
        CHECK(doctest::Approx(new_sum).epsilon(0.001) == std::pow(21.0f, 1.4f));

        // Cleanup
        Parties[1] = nullptr;
        UserList[leader] = User{};
    }

    TEST_CASE("G4_BroadCastParty_OnlineParty") {
        if (UserList.size() < 10) UserList.resize(10);
        MaxUsers = 10;
        for (std::int16_t i = 1; i <= ModParty::MAX_PARTIES; ++i) Parties[i] = nullptr;

        std::int16_t leader = 1;
        UserList[leader] = User{};
        UserList[leader].Stats.UserAtributos[eAtributos::Carisma] = 18;
        UserList[leader].Stats.UserSkills[eSkill::Liderazgo] = 10;
        UserList[leader].Stats.ELV = 20;
        UserList[leader].name = "LiderHeroe";

        ModParty::CrearParty(leader);

        std::string last_broadcast_msg;
        std::uint8_t last_font = 0;
        ModParty::PartyCallbacks cb{};
        cb.WriteConsoleMsg = [&](std::int16_t u, std::string_view msg, std::uint8_t font) {
            last_broadcast_msg = msg;
            last_font = font;
        };
        ModParty::SetPartyCallbacks(cb);

        ModParty::BroadCastParty(leader, "Hola a la party!");
        CHECK(last_broadcast_msg == " [LiderHeroe] Hola a la party!");

        ModParty::OnlineParty(leader);
        CHECK(last_broadcast_msg.find("LiderHeroe") != std::string::npos);
        CHECK(last_broadcast_msg.find("Experiencia total:") != std::string::npos);

        // Reset callbacks
        ModParty::SetPartyCallbacks(ModParty::PartyCallbacks{});

        // Cleanup
        Parties[1] = nullptr;
        UserList[leader] = User{};
    }

}
