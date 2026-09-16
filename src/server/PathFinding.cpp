#include "PathFinding.hpp"

static tIntermidiateWork TmpArray[101][101];

bool Limites(std::int16_t vfila, std::int16_t vcolu) {
    return (vcolu >= 1 && vcolu <= 100 && vfila >= 1 && vfila <= 100);
}

bool IsWalkable(std::int16_t map, std::int16_t row, std::int16_t col, std::int16_t npc_index) {
    if (map <= 0 || row < 1 || row > 100 || col < 1 || col > 100) {
        return false;
    }

    std::size_t block_idx = (static_cast<std::size_t>(map) * 101 + static_cast<std::size_t>(row)) * 101 + static_cast<std::size_t>(col);
    if (block_idx >= MapData.size()) {
        return false;
    }

    const auto& cell = MapData[block_idx];
    bool walkable = (cell.Blocked == 0 && cell.NpcIndex == 0);

    if (cell.UserIndex != 0) {
        if (static_cast<std::size_t>(npc_index) < Npclist.size()) {
            if (cell.UserIndex != Npclist[npc_index].PFINFO.TargetUser) {
                walkable = false;
            }
        } else {
            walkable = false;
        }
    }

    return walkable;
}

void InitializeTable(tIntermidiateWork (&T)[101][101], const tVertice& S, std::int16_t max_steps) {
    for (std::int16_t j = static_cast<std::int16_t>(S.Y - max_steps); j <= static_cast<std::int16_t>(S.Y + max_steps); ++j) {
        for (std::int16_t k = static_cast<std::int16_t>(S.X - max_steps); k <= static_cast<std::int16_t>(S.X + max_steps); ++k) {
            if (Limites(j, k)) {
                T[j][k].Known = false;
                T[j][k].DistV = 1000;
                T[j][k].PrevV.X = 0;
                T[j][k].PrevV.Y = 0;
            }
        }
    }

    if (Limites(S.Y, S.X)) {
        T[S.Y][S.X].Known = false;
        T[S.Y][S.X].DistV = 0;
    }
}

void ProcessAdjacents(std::int16_t map_index, tIntermidiateWork (&T)[101][101], std::int16_t vfila, std::int16_t vcolu, std::int16_t npc_index, std::queue<tVertice>& queue) {
    tVertice V;

    // 1. Norte: (vfila - 1, vcolu)
    std::int16_t j_north = static_cast<std::int16_t>(vfila - 1);
    if (Limites(j_north, vcolu)) {
        if (IsWalkable(map_index, j_north, vcolu, npc_index)) {
            if (T[j_north][vcolu].DistV == 1000) {
                T[j_north][vcolu].DistV = static_cast<std::int16_t>(T[vfila][vcolu].DistV + 1);
                T[j_north][vcolu].PrevV.X = vcolu;
                T[j_north][vcolu].PrevV.Y = vfila;
                V.X = vcolu;
                V.Y = j_north;
                queue.push(V);
            }
        }
    }

    // 2. Sur: (vfila + 1, vcolu)
    std::int16_t j_south = static_cast<std::int16_t>(vfila + 1);
    if (Limites(j_south, vcolu)) {
        if (IsWalkable(map_index, j_south, vcolu, npc_index)) {
            if (T[j_south][vcolu].DistV == 1000) {
                T[j_south][vcolu].DistV = static_cast<std::int16_t>(T[vfila][vcolu].DistV + 1);
                T[j_south][vcolu].PrevV.X = vcolu;
                T[j_south][vcolu].PrevV.Y = vfila;
                V.X = vcolu;
                V.Y = j_south;
                queue.push(V);
            }
        }
    }

    // 3. Oeste: (vfila, vcolu - 1)
    std::int16_t k_west = static_cast<std::int16_t>(vcolu - 1);
    if (Limites(vfila, k_west)) {
        if (IsWalkable(map_index, vfila, k_west, npc_index)) {
            if (T[vfila][k_west].DistV == 1000) {
                T[vfila][k_west].DistV = static_cast<std::int16_t>(T[vfila][vcolu].DistV + 1);
                T[vfila][k_west].PrevV.X = vcolu;
                T[vfila][k_west].PrevV.Y = vfila;
                V.X = k_west;
                V.Y = vfila;
                queue.push(V);
            }
        }
    }

    // 4. Este: (vfila, vcolu + 1)
    std::int16_t k_east = static_cast<std::int16_t>(vcolu + 1);
    if (Limites(vfila, k_east)) {
        if (IsWalkable(map_index, vfila, k_east, npc_index)) {
            if (T[vfila][k_east].DistV == 1000) {
                T[vfila][k_east].DistV = static_cast<std::int16_t>(T[vfila][vcolu].DistV + 1);
                T[vfila][k_east].PrevV.X = vcolu;
                T[vfila][k_east].PrevV.Y = vfila;
                V.X = k_east;
                V.Y = vfila;
                queue.push(V);
            }
        }
    }
}

void MakePath(std::int16_t npc_index) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& pf = Npclist[npc_index].PFINFO;
    tVertice target;
    target.X = static_cast<std::int16_t>(pf.Target.X);
    target.Y = static_cast<std::int16_t>(pf.Target.Y);

    if (!Limites(target.Y, target.X)) {
        pf.NoPath = true;
        pf.PathLenght = 0;
        return;
    }

    std::int16_t pasos = TmpArray[target.Y][target.X].DistV;
    pf.PathLenght = pasos;

    if (pasos == 1000) {
        pf.NoPath = true;
        pf.PathLenght = 0;
        return;
    }

    pf.Path.resize(static_cast<std::size_t>(pasos) + 1);
    pf.CurPos = 1;
    pf.NoPath = false;

    tVertice V = target;
    for (std::int16_t i = pasos; i >= 1; --i) {
        pf.Path[i].X = V.Y;
        pf.Path[i].Y = V.X;
        V = TmpArray[V.Y][V.X].PrevV;
    }
}

void SeekPath(std::int16_t npc_index, std::int16_t max_steps) {
    if (npc_index <= 0 || static_cast<std::size_t>(npc_index) >= Npclist.size()) {
        return;
    }

    auto& npc = Npclist[npc_index];
    tVertice cur_npc_pos;
    cur_npc_pos.X = npc.Pos.Y;
    cur_npc_pos.Y = npc.Pos.X;

    tVertice tar_npc_pos;
    tar_npc_pos.X = static_cast<std::int16_t>(npc.PFINFO.Target.X);
    tar_npc_pos.Y = static_cast<std::int16_t>(npc.PFINFO.Target.Y);

    // En VB6 L226: Call InitializeTable(TmpArray, cur_npc_pos) -> se usa el MaxSteps por defecto (30) de InitializeTable
    InitializeTable(TmpArray, cur_npc_pos);

    std::queue<tVertice> queue;
    queue.push(cur_npc_pos);

    int steps = 0;

    while (!queue.empty()) {
        if (steps > max_steps) {
            break;
        }

        tVertice V = queue.front();
        queue.pop();

        if (V.X == tar_npc_pos.X && V.Y == tar_npc_pos.Y) {
            break;
        }

        ProcessAdjacents(npc.Pos.Map, TmpArray, V.Y, V.X, npc_index, queue);
    }

    MakePath(npc_index);
}
