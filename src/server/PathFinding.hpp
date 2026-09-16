#pragma once

#include <cstdint>
#include <queue>
#include "Declares.hpp"

struct tIntermidiateWork {
    bool Known{false};
    std::int16_t DistV{1000};
    tVertice PrevV{};
};

bool Limites(std::int16_t vfila, std::int16_t vcolu);
bool IsWalkable(std::int16_t map, std::int16_t row, std::int16_t col, std::int16_t npc_index);

void InitializeTable(tIntermidiateWork (&T)[101][101], const tVertice& S, std::int16_t max_steps = 30);
void ProcessAdjacents(std::int16_t map_index, tIntermidiateWork (&T)[101][101], std::int16_t vfila, std::int16_t vcolu, std::int16_t npc_index, std::queue<tVertice>& queue);

void SeekPath(std::int16_t npc_index, std::int16_t max_steps = 30);
void MakePath(std::int16_t npc_index);
