#include "Characters.hpp"
#include "Declares.hpp"

std::int16_t CharIndexToUserIndex(std::int16_t char_index) noexcept {
    if (char_index < 1 || char_index > MAXCHARS) {
        return INVALID_INDEX;
    }

    std::int16_t user_index = CharList[char_index];

    if (user_index < 1 || user_index > MaxUsers) {
        return INVALID_INDEX;
    }

    if (UserList[user_index].char_appearance.CharIndex != char_index) {
        return INVALID_INDEX;
    }

    return user_index;
}
