#include "Modulo_InventANDobj.hpp"
#include "Matematicas.hpp"
#include <algorithm>

namespace Modulo_InventANDobj {

namespace {

NPCTemplateLookupHook s_npc_template_lookup_hook{};
RandomGeneratorHook s_random_generator_hook{};
TirarItemAlPisoHook s_tirar_item_hook{};
TilelibreHook s_tilelibre_hook{};
MakeObjHook s_make_obj_hook{};

[[nodiscard]] inline bool IsValidNpcIndex(std::int16_t npc_index) noexcept {
    return npc_index >= 1 && static_cast<std::size_t>(npc_index) < Npclist.size();
}

[[nodiscard]] inline std::int32_t GetRandomNumber(std::int32_t min, std::int32_t max) {
    if (s_random_generator_hook) {
        return s_random_generator_hook(min, max);
    }
    return RandomNumber(min, max);
}

} // namespace

void SetNPCTemplateLookupHook(NPCTemplateLookupHook hook) noexcept {
    s_npc_template_lookup_hook = std::move(hook);
}

void SetRandomGeneratorHook(RandomGeneratorHook hook) noexcept {
    s_random_generator_hook = std::move(hook);
}

void SetTirarItemAlPisoHook(TirarItemAlPisoHook hook) noexcept {
    s_tirar_item_hook = std::move(hook);
}

void SetTilelibreHook(TilelibreHook hook) noexcept {
    s_tilelibre_hook = std::move(hook);
}

void SetMakeObjHook(MakeObjHook hook) noexcept {
    s_make_obj_hook = std::move(hook);
}

// ============================================================================
// Fase 1 - G1: Núcleo de Inventario de NPCs
// ============================================================================

void ResetNpcInv(std::int16_t npc_index) noexcept {
    if (!IsValidNpcIndex(npc_index)) {
        return;
    }

    auto& npc_inst = Npclist[npc_index];
    npc_inst.Invent.NroItems = 0;

    // Preservación estricta de indexación 1-based (1..MAX_INVENTORY_SLOTS)
    // alineada con la estructura legacy de VB6 y Declares.hpp:100.
    for (std::size_t i = 1; i <= MAX_INVENTORY_SLOTS; ++i) {
        npc_inst.Invent.Object[i].ObjIndex = 0;
        npc_inst.Invent.Object[i].Amount = 0;
    }

    npc_inst.InvReSpawn = 0;
}

bool QuedanItems(std::int16_t npc_index, std::int16_t obj_index) noexcept {
    if (!IsValidNpcIndex(npc_index) || obj_index <= 0) {
        return false;
    }

    const auto& invent = Npclist[npc_index].Invent;
    if (invent.NroItems > 0) {
        // Recorrido de slots 1-based (1..MAX_INVENTORY_SLOTS)
        for (std::size_t i = 1; i <= MAX_INVENTORY_SLOTS; ++i) {
            if (invent.Object[i].ObjIndex == obj_index) {
                return true;
            }
        }
    }

    return false;
}

std::int16_t EncontrarCant(std::int16_t npc_index, std::int16_t obj_index) noexcept {
    if (!IsValidNpcIndex(npc_index) || obj_index <= 0) {
        return 0;
    }

    // Erradicación de lecturas síncronas bloqueantes a "NPCs.dat" (GetVar):
    // Se consulta la plantilla en memoria mediante el hook desacoplado.
    if (!s_npc_template_lookup_hook) {
        return 0;
    }

    const auto slots = s_npc_template_lookup_hook(Npclist[npc_index].Numero);
    for (const auto& slot : slots) {
        if (slot.obj_index == obj_index) {
            return slot.amount;
        }
    }

    return 0;
}

void QuitarNpcInvItem(std::int16_t npc_index, std::uint8_t slot, std::int16_t cantidad) {
    // Validación de rangos respetando el slot 1-based (1..MAX_INVENTORY_SLOTS)
    if (!IsValidNpcIndex(npc_index) || slot < 1 || slot > MAX_INVENTORY_SLOTS || cantidad <= 0) {
        return;
    }

    auto& npc_inst = Npclist[npc_index];
    const std::int16_t obj_index = npc_inst.Invent.Object[slot].ObjIndex;
    if (obj_index <= 0) {
        return;
    }

    // Consulta de metadato crucial en ObjDataList (cargada en arranque por FileIO)
    const bool es_crucial = (static_cast<std::size_t>(obj_index) < ObjDataList.size())
        ? (ObjDataList[obj_index].Crucial != 0)
        : false;

    auto& slot_obj = npc_inst.Invent.Object[slot];
    slot_obj.Amount -= cantidad;

    if (slot_obj.Amount <= 0) {
        npc_inst.Invent.NroItems -= 1;
        slot_obj.ObjIndex = 0;
        slot_obj.Amount = 0;

        // Si el objeto es crucial, se debe reponer inmediatamente con el stock original de la plantilla
        if (es_crucial) {
            if (!QuedanItems(npc_index, obj_index)) {
                const std::int16_t cant_original = EncontrarCant(npc_index, obj_index);
                if (cant_original > 0) {
                    slot_obj.ObjIndex = obj_index;
                    slot_obj.Amount = cant_original;
                    npc_inst.Invent.NroItems += 1;
                }
            }
        }

        // Si el inventario quedó totalmente desierto y no tiene InvReSpawn = 1, reponer todo el inventario
        if (npc_inst.Invent.NroItems == 0 && npc_inst.InvReSpawn != 1) {
            CargarInvent(npc_index);
        }
    }
}

void CargarInvent(std::int16_t npc_index) {
    if (!IsValidNpcIndex(npc_index)) {
        return;
    }

    auto& npc_inst = Npclist[npc_index];

    // Erradicación de lecturas síncronas bloqueantes a "NPCs.dat" (GetVar):
    // La plantilla completa de ítems iniciales se obtiene desde memoria vía hook.
    if (!s_npc_template_lookup_hook) {
        npc_inst.Invent.NroItems = 0;
        return;
    }

    const auto slots = s_npc_template_lookup_hook(npc_inst.Numero);
    const std::size_t count = std::min(slots.size(), static_cast<std::size_t>(MAX_INVENTORY_SLOTS));
    npc_inst.Invent.NroItems = static_cast<std::int16_t>(count);

    // Carga en slots 1-based (Object[1..count])
    for (std::size_t i = 0; i < count; ++i) {
        npc_inst.Invent.Object[i + 1].ObjIndex = slots[i].obj_index;
        npc_inst.Invent.Object[i + 1].Amount = slots[i].amount;
    }

    // Blanqueo de slots residuales 1-based
    for (std::size_t i = count + 1; i <= MAX_INVENTORY_SLOTS; ++i) {
        npc_inst.Invent.Object[i + 1].ObjIndex = 0;
        npc_inst.Invent.Object[i + 1].Amount = 0;
    }
}

// ============================================================================
// Fase 2 - G2: Drops Probabilísticos y Fraccionamiento de Oro
// ============================================================================

void TirarOroNpc(std::int32_t cantidad, const WorldPos& pos) {
    if (cantidad <= 0) {
        return;
    }

    std::int32_t remaining_gold = cantidad;

    while (remaining_gold > 0) {
        Obj my_obj{};
        my_obj.ObjIndex = iORO;

        if (remaining_gold > MAX_INVENTORY_OBJS) {
            my_obj.Amount = MAX_INVENTORY_OBJS;
            remaining_gold -= MAX_INVENTORY_OBJS;
        } else {
            my_obj.Amount = static_cast<std::int16_t>(remaining_gold);
            remaining_gold = 0;
        }

        TirarItemAlPiso(pos, my_obj);
    }
}

void NPC_TIRAR_ITEMS(npc& current_npc, bool is_pretoriano) {
    // Tira todo el inventario si la criatura es pretoriana
    if (is_pretoriano) {
        for (std::size_t i = 1; i <= MAX_INVENTORY_SLOTS; ++i) {
            if (current_npc.Invent.Object[i].ObjIndex > 0) {
                Obj my_obj{};
                my_obj.ObjIndex = current_npc.Invent.Object[i].ObjIndex;
                my_obj.Amount = current_npc.Invent.Object[i].Amount;
                TirarItemAlPiso(current_npc.Pos, my_obj);
            }
        }

        // Bug #27: En el servidor legacy, los pretorianos son los ÚNICOS que evalúan y arrojan
        // el campo .GiveGLD fraccionado.
        if (current_npc.GiveGLD > 0) {
            TirarOroNpc(current_npc.GiveGLD, current_npc.Pos);
        }

        return;
    }

    // Criaturas regulares: Cascada geométrica de drops
    // Bug #27: Las criaturas regulares NUNCA arrojan .GiveGLD; se descarta por completo
    // y solo arrojan el botín probabilístico de la matriz .Drop.
    const std::int32_t roll = GetRandomNumber(1, 100);

    // 10% de probabilidad de drop nulo (roll > 90)
    if (roll <= 90) {
        std::size_t nro_drop = 1;

        // 10% de pasar a la etapa 2 (roll <= 10)
        if (roll <= 10) {
            nro_drop = 2;

            // 10% de ir pasando a etapas sucesivas 3, 4 y 5
            for (int i = 1; i <= 3; ++i) {
                if (GetRandomNumber(1, 100) <= 10) {
                    nro_drop += 1;
                } else {
                    break;
                }
            }
        }

        if (nro_drop >= 1 && nro_drop <= MAX_NPC_DROPS) {
            const std::int16_t obj_index = current_npc.Drop[nro_drop].ObjIndex;
            if (obj_index > 0) {
                if (obj_index == iORO) {
                    TirarOroNpc(static_cast<std::int32_t>(current_npc.Drop[nro_drop].Amount), current_npc.Pos);
                } else {
                    Obj my_obj{};
                    my_obj.ObjIndex = obj_index;
                    my_obj.Amount = static_cast<std::int16_t>(current_npc.Drop[nro_drop].Amount);
                    TirarItemAlPiso(current_npc.Pos, my_obj);
                }
            }
        }
    }
}

WorldPos TirarItemAlPiso(const WorldPos& pos, const Obj& obj, bool not_pirata) {
    if (s_tirar_item_hook) {
        return s_tirar_item_hook(pos, obj, not_pirata);
    }

    WorldPos nueva_pos{ .Map = pos.Map, .X = 0, .Y = 0 };

    if (s_tilelibre_hook) {
        s_tilelibre_hook(pos, nueva_pos, obj, not_pirata, true);
    }

    // Bug #28: Destrucción silenciosa de ítems si Tilelibre retorna celda nula (0, 0).
    // Si no hay celdas libres adyacentes o legales (mapa saturado), MakeObj se omite
    // incondicionalmente y el ítem se evapora sin arrojar excepciones ni alterar bucles llamadores.
    if (nueva_pos.X != 0 && nueva_pos.Y != 0) {
        if (s_make_obj_hook) {
            s_make_obj_hook(obj, pos.Map, nueva_pos.X, nueva_pos.Y);
        }
    }

    return nueva_pos;
}

} // namespace Modulo_InventANDobj
