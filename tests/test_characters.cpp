#include <doctest/doctest.h>
#include "server/Characters.hpp"
#include "server/Declares.hpp"

TEST_SUITE("Characters - Mapeo CharIndex a UserIndex") {
    TEST_CASE("CharIndex fuera de rango valido (1 .. MAXCHARS)") {
        CHECK(CharIndexToUserIndex(0) == INVALID_INDEX);
        CHECK(CharIndexToUserIndex(-1) == INVALID_INDEX);
        CHECK(CharIndexToUserIndex(-32768) == INVALID_INDEX);
        CHECK(CharIndexToUserIndex(MAXCHARS + 1) == INVALID_INDEX);
    }

    TEST_CASE("UserIndex devuelto por CharList fuera de rango (1 .. MaxUsers)") {
        MaxUsers = 100;
        
        CharList[10] = 0;
        CHECK(CharIndexToUserIndex(10) == INVALID_INDEX);

        CharList[11] = -5;
        CHECK(CharIndexToUserIndex(11) == INVALID_INDEX);

        CharList[12] = 101;
        CHECK(CharIndexToUserIndex(12) == INVALID_INDEX);
    }

    TEST_CASE("Desincronizacion o slot sin correspondencia bidireccional") {
        MaxUsers = 100;

        // Redimensionar UserList si es necesario
        if (UserList.size() <= 100) {
            UserList.resize(101);
        }

        // Slot de usuario cuyo CharIndex no esta asignado (0)
        CharList[15] = 5;
        UserList[5].char_appearance.CharIndex = 0;
        CHECK(CharIndexToUserIndex(15) == INVALID_INDEX);

        // Slot de usuario cuyo CharIndex apunta a otro personaje diferente
        CharList[16] = 6;
        UserList[6].char_appearance.CharIndex = 99;
        CHECK(CharIndexToUserIndex(16) == INVALID_INDEX);
    }

    TEST_CASE("Mapeo bidireccional valido de CharIndex a UserIndex") {
        MaxUsers = 100;

        if (UserList.size() <= 100) {
            UserList.resize(101);
        }

        CharList[25] = 42;
        UserList[42].char_appearance.CharIndex = 25;
        CHECK(CharIndexToUserIndex(25) == 42);

        CharList[MAXCHARS] = 1;
        UserList[1].char_appearance.CharIndex = MAXCHARS;
        CHECK(CharIndexToUserIndex(MAXCHARS) == 1);

        // Cleanup
        CharList[25] = 0;
        UserList[42].char_appearance.CharIndex = 0;
        CharList[MAXCHARS] = 0;
        UserList[1].char_appearance.CharIndex = 0;
    }
}
