#include "mdlCOmercioConUsuario.hpp"
#include "Protocol.hpp"
#include "InvUsuario.hpp"
#include "Modulo_InventANDobj.hpp"

namespace ao {

using namespace ao::net::protocol;

namespace {
QuitarObjetosHook g_quitar_objetos_hook{nullptr};
TirarItemAlPisoHook g_tirar_piso_hook{nullptr};

void QuitarObjetosDefault(std::int16_t obj_index, std::int32_t amount, std::int16_t user_index) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    std::int32_t cant = amount;

    for (std::uint8_t i = 1; i <= user.CurrentInventorySlots; ++i) {
        auto& item = user.Invent.Object[i];
        if (item.ObjIndex == obj_index) {
            if (item.Amount <= cant && item.Equipped == 1) {
                InvUsuario::Desequipar(user_index, i);
            }

            item.Amount -= static_cast<std::int16_t>(cant);
            if (item.Amount <= 0) {
                cant = -item.Amount;
                user.Invent.NroItems--;
                item.Amount = 0;
                item.ObjIndex = 0;
            } else {
                cant = 0;
            }

            InvUsuario::UpdateUserInv(false, user_index, i);

            if (cant == 0) {
                return;
            }
        }
    }
}

WorldPos TirarItemAlPisoDefault(const WorldPos& pos, const Obj& obj) {
    return Modulo_InventANDobj::TirarItemAlPiso(pos, obj, true);
}

} // namespace

void SetQuitarObjetosHook(QuitarObjetosHook hook) noexcept {
    g_quitar_objetos_hook = std::move(hook);
}

void SetTirarItemAlPisoHook(TirarItemAlPisoHook hook) noexcept {
    g_tirar_piso_hook = std::move(hook);
}

void IniciarComercioConUsuario(std::int16_t origen, std::int16_t destino) {
    if (origen < 1 || static_cast<std::size_t>(origen) >= UserList.size() ||
        destino < 1 || static_cast<std::size_t>(destino) >= UserList.size()) {
        return;
    }

    // Si ambos pusieron /comerciar entonces
    if (UserList[origen].ComUsu.DestUsu == destino &&
        UserList[destino].ComUsu.DestUsu == origen) {
        // Actualiza el inventario del usuario origen
        InvUsuario::UpdateUserInv(true, origen, 0);
        // Decirle al origen que abra la ventanita.
        WriteUserCommerceInit(origen);
        UserList[origen].flags.Comerciando = true;

        // Actualiza el inventario del usuario destino
        InvUsuario::UpdateUserInv(true, destino, 0);
        // Decirle al destino que abra la ventanita.
        WriteUserCommerceInit(destino);
        UserList[destino].flags.Comerciando = true;
    } else {
        // Es el primero que comercia
        WriteConsoleMsg(destino, UserList[origen].name + " desea comerciar. Si deseas aceptar, escribe /COMERCIAR.", FontTypeNames::FONTTYPE_TALK);
        UserList[destino].flags.TargetUser = origen;
    }

    FlushBuffer(destino);
}

void EnviarOferta(std::int16_t user_index, std::uint8_t offer_slot) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    const auto dest_usu = UserList[user_index].ComUsu.DestUsu;
    if (dest_usu < 1 || static_cast<std::size_t>(dest_usu) >= UserList.size()) {
        return;
    }

    std::int16_t obj_index = 0;
    std::int32_t obj_amount = 0;

    if (offer_slot == GOLD_OFFER_SLOT) {
        obj_index = iORO;
        obj_amount = UserList[dest_usu].ComUsu.GoldAmount;
    } else if (offer_slot >= 1 && offer_slot <= MAX_OFFER_SLOTS) {
        obj_index = UserList[dest_usu].ComUsu.Objeto[offer_slot];
        obj_amount = UserList[dest_usu].ComUsu.cant[offer_slot];
    }

    WriteChangeUserTradeSlot(user_index, offer_slot, obj_index, obj_amount);
    FlushBuffer(user_index);
}

void FinComerciarUsu(std::int16_t user_index) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    if (user.ComUsu.DestUsu > 0) {
        WriteUserCommerceEnd(user_index);
    }

    user.ComUsu.Acepto = false;
    user.ComUsu.Confirmo = false;
    user.ComUsu.DestUsu = 0;

    for (std::int16_t i = 1; i <= MAX_OFFER_SLOTS; ++i) {
        user.ComUsu.cant[i] = 0;
        user.ComUsu.Objeto[i] = 0;
    }

    user.ComUsu.GoldAmount = 0;
    user.ComUsu.DestNick.clear();
    user.flags.Comerciando = false;
}

void AgregarOferta(std::int16_t user_index, std::uint8_t offer_slot, std::int16_t obj_index, std::int64_t amount, bool is_gold) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    if (PuedeSeguirComerciando(user_index)) {
        auto& com_usu = UserList[user_index].ComUsu;
        // Si ya confirmo su oferta, no puede cambiarla! (intento de trampa -> aborta la sesion)
        if (com_usu.Confirmo) {
            const auto otro_user = com_usu.DestUsu;
            FinComerciarUsu(user_index);
            if (otro_user > 0 && otro_user <= MaxUsers && static_cast<std::size_t>(otro_user) < UserList.size()) {
                FinComerciarUsu(otro_user);
                FlushBuffer(otro_user);
            }
            return;
        }

        if (is_gold) {
            // Agregamos (o quitamos) mas oro a la oferta promoviendo a 64-bit
            std::int64_t nuevo_oro = static_cast<std::int64_t>(com_usu.GoldAmount) + amount;
            if (nuevo_oro < 0) {
                nuevo_oro = 0;
            }
            com_usu.GoldAmount = static_cast<std::int32_t>(nuevo_oro);
        } else {
            if (offer_slot >= 1 && offer_slot <= MAX_OFFER_SLOTS) {
                // Si es 0 estoy modificando la cantidad, no agregando
                if (obj_index > 0) {
                    com_usu.Objeto[offer_slot] = obj_index;
                }
                std::int64_t nueva_cant = static_cast<std::int64_t>(com_usu.cant[offer_slot]) + amount;
                if (nueva_cant <= 0) {
                    // Removemos el objeto para evitar conflictos
                    com_usu.Objeto[offer_slot] = 0;
                    com_usu.cant[offer_slot] = 0;
                } else {
                    com_usu.cant[offer_slot] = static_cast<std::int32_t>(nueva_cant);
                }
            }
        }
    }
}

bool PuedeSeguirComerciando(std::int16_t user_index) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return false;
    }

    bool comercio_invalido = false;
    const auto dest_usu = UserList[user_index].ComUsu.DestUsu;

    // Usuario valido?
    if (dest_usu <= 0 || dest_usu > MaxUsers || static_cast<std::size_t>(dest_usu) >= UserList.size()) {
        comercio_invalido = true;
    }

    const auto otro_user_index = dest_usu;

    if (!comercio_invalido) {
        // Estan logueados?
        if (!UserList[otro_user_index].flags.UserLogged || !UserList[user_index].flags.UserLogged) {
            comercio_invalido = true;
        }
    }

    if (!comercio_invalido) {
        // Se estan comerciando el uno al otro?
        if (UserList[otro_user_index].ComUsu.DestUsu != user_index) {
            comercio_invalido = true;
        }
    }

    if (!comercio_invalido) {
        // El nombre del otro es el mismo que al que le comercio?
        if (UserList[otro_user_index].name != UserList[user_index].ComUsu.DestNick) {
            comercio_invalido = true;
        }
    }

    if (!comercio_invalido) {
        // Mi nombre es el mismo que al que el le comercia?
        if (UserList[user_index].name != UserList[otro_user_index].ComUsu.DestNick) {
            comercio_invalido = true;
        }
    }

    if (!comercio_invalido) {
        // Esta vivo?
        if (UserList[otro_user_index].flags.Muerto == 1) {
            comercio_invalido = true;
        }
    }

    // Fin del comercio si fue invalido
    if (comercio_invalido) {
        FinComerciarUsu(user_index);

        if (otro_user_index > 0 && otro_user_index <= MaxUsers && static_cast<std::size_t>(otro_user_index) < UserList.size()) {
            FinComerciarUsu(otro_user_index);
            FlushBuffer(otro_user_index);
        }

        return false;
    }

    return true;
}

void AceptarComercioUsu(std::int16_t user_index,
                        QuitarObjetosHook quitar_obj_hook,
                        TirarItemAlPisoHook tirar_piso_hook) {
    if (user_index < 1 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    if (!PuedeSeguirComerciando(user_index)) {
        return;
    }

    UserList[user_index].ComUsu.Acepto = true;
    const auto otro_user_index = UserList[user_index].ComUsu.DestUsu;

    if (otro_user_index < 1 || static_cast<std::size_t>(otro_user_index) >= UserList.size()) {
        return;
    }

    if (!UserList[otro_user_index].ComUsu.Acepto) {
        return;
    }

    auto effective_quitar_obj_hook = quitar_obj_hook ? quitar_obj_hook : (g_quitar_objetos_hook ? g_quitar_objetos_hook : QuitarObjetosDefault);
    auto effective_tirar_piso_hook = tirar_piso_hook ? tirar_piso_hook : (g_tirar_piso_hook ? g_tirar_piso_hook : TirarItemAlPisoDefault);

    // Envio los items a quien corresponde (slots 1 a 31)
    for (std::int16_t offer_slot = 1; offer_slot <= MAX_OFFER_SLOTS + 1; ++offer_slot) {
        // Items del 1er usuario hacia el 2do usuario
        {
            auto& user1 = UserList[user_index];
            auto& user2 = UserList[otro_user_index];

            if (offer_slot == GOLD_OFFER_SLOT) {
                if (user1.ComUsu.GoldAmount > 0) {
                    user1.Stats.GLD -= user1.ComUsu.GoldAmount;
                    WriteUpdateUserStats(user_index);

                    // Replicación Bug #36: Sin clampleo contra MAXORO, usando int64_t
                    std::int64_t nuevo_oro = static_cast<std::int64_t>(user2.Stats.GLD) + user1.ComUsu.GoldAmount;
                    user2.Stats.GLD = static_cast<std::int32_t>(nuevo_oro);
                    WriteUpdateUserStats(otro_user_index);
                }
            } else if (offer_slot >= 1 && offer_slot <= MAX_OFFER_SLOTS) {
                if (user1.ComUsu.Objeto[offer_slot] > 0) {
                    Obj trading_obj{user1.ComUsu.Objeto[offer_slot], static_cast<std::int16_t>(user1.ComUsu.cant[offer_slot])};

                    // Replicación Bug #35: Se acredita en el receptor antes de debitar en el emisor
                    if (!InvUsuario::MeterItemEnInventario(otro_user_index, trading_obj)) {
                        WorldPos pos_creada = effective_tirar_piso_hook(user2.Pos, trading_obj);
                        // Replicación Bug #34: Si pos_creada.X == 0 && Y == 0, no se materializó en el piso
                    }

                    // Se sustrae incondicionalmente del emisor (consuma evaporación en Bug #34)
                    effective_quitar_obj_hook(trading_obj.ObjIndex, user1.ComUsu.cant[offer_slot], user_index);
                }
            }
        }

        // Items del 2do usuario hacia el 1er usuario
        {
            auto& user1 = UserList[user_index];
            auto& user2 = UserList[otro_user_index];

            if (offer_slot == GOLD_OFFER_SLOT) {
                if (user2.ComUsu.GoldAmount > 0) {
                    user2.Stats.GLD -= user2.ComUsu.GoldAmount;
                    WriteUpdateUserStats(otro_user_index);

                    // Replicación Bug #36: Sin clampleo contra MAXORO, usando int64_t
                    std::int64_t nuevo_oro = static_cast<std::int64_t>(user1.Stats.GLD) + user2.ComUsu.GoldAmount;
                    user1.Stats.GLD = static_cast<std::int32_t>(nuevo_oro);
                    WriteUpdateUserStats(user_index);
                }
            } else if (offer_slot >= 1 && offer_slot <= MAX_OFFER_SLOTS) {
                if (user2.ComUsu.Objeto[offer_slot] > 0) {
                    Obj trading_obj{user2.ComUsu.Objeto[offer_slot], static_cast<std::int16_t>(user2.ComUsu.cant[offer_slot])};

                    // Replicación Bug #35: Se acredita en el receptor antes de debitar en el emisor
                    if (!InvUsuario::MeterItemEnInventario(user_index, trading_obj)) {
                        WorldPos pos_creada = effective_tirar_piso_hook(user1.Pos, trading_obj);
                        // Replicación Bug #34: Si pos_creada.X == 0 && Y == 0, no se materializó en el piso
                    }

                    // Se sustrae incondicionalmente del emisor (consuma evaporación en Bug #34)
                    effective_quitar_obj_hook(trading_obj.ObjIndex, user2.ComUsu.cant[offer_slot], otro_user_index);
                }
            }
        }
    }

    // End Trade
    FinComerciarUsu(user_index);
    FinComerciarUsu(otro_user_index);
}

} // namespace ao
