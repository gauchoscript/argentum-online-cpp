#include "server/ModFacciones.hpp"
#include "server/FileIO.hpp"
#include "server/InvUsuario.hpp"
#include "server/Modulo_InventANDobj.hpp"
#include "server/modGuilds.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace ModFacciones {

namespace {
FactionCallbacks g_callbacks{};
}

void SetFactionCallbacks(const FactionCallbacks& callbacks) {
    g_callbacks = callbacks;
}

void ResetFactionCallbacks() {
    g_callbacks = FactionCallbacks{};
}

void InitDefaultFactionCallbacks() {
    FactionCallbacks cb{};

    cb.MeterItemEnInventario = [](std::int16_t user_index, const Obj& item) -> bool {
        Obj tmp = item;
        return InvUsuario::MeterItemEnInventario(user_index, tmp);
    };

    cb.TirarItemAlPiso = [](const WorldPos& pos, const Obj& item) {
        Modulo_InventANDobj::TirarItemAlPiso(pos, item);
    };

    cb.Desequipar = [](std::int16_t user_index, std::uint8_t slot) {
        InvUsuario::Desequipar(user_index, slot);
    };

    cb.GetGuildAlignment = [](std::int16_t guild_index) -> std::string {
        return modGuilds::GuildAlignment(guild_index);
    };

    cb.LogEjercitoReal = [](std::string_view text) {
        FileIO::LogEjercitoReal(std::string(text));
    };

    cb.LogEjercitoCaos = [](std::string_view text) {
        FileIO::LogEjercitoCaos(std::string(text));
    };

    SetFactionCallbacks(cb);
}

std::int16_t GetArmourAmount(std::int16_t rango, eTipoDefArmors tipo_def) {
    const double r = static_cast<double>(rango);
    switch (tipo_def) {
    case eTipoDefArmors::ieBaja:
        return static_cast<std::int16_t>(std::nearbyint(20.0 / (r + 1.0)));

    case eTipoDefArmors::ieMedia: {
        const double denom = std::max(r - 4.0, 1.0);
        return static_cast<std::int16_t>(std::nearbyint((r * 2.0) / denom));
    }

    case eTipoDefArmors::ieAlta:
        return static_cast<std::int16_t>(std::nearbyint(r * 1.35));

    default:
        return 0;
    }
}

std::string TituloReal(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return "";
    }

    const auto rango = UserList[user_index].Faccion.RecompensasReal;
    switch (rango) {
    case 0:
        return "Aprendiz";
    case 1:
        return "Escudero";
    case 2:
        return "Soldado";
    case 3:
        return "Sargento";
    case 4:
        return "Teniente";
    case 5:
        return "Comandante";
    case 6:
        return "Capitán";
    case 7:
        return "Senescal";
    case 8:
        return "Mariscal";
    case 9:
        return "Condestable";
    case 10:
        return "Ejecutor Imperial";
    case 11:
        return "Protector del Reino";
    case 12:
        return "Avatar de la Justicia";
    case 13:
        return "Guardián del Bien";
    default:
        return "Campeón de la Luz";
    }
}

std::string TituloCaos(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return "";
    }

    const auto rango = UserList[user_index].Faccion.RecompensasCaos;
    switch (rango) {
    case 0:
        return "Acólito";
    case 1:
        return "Alma Corrupta";
    case 2:
        return "Paria";
    case 3:
        return "Condenado";
    case 4:
        return "Esbirro";
    case 5:
        return "Sanguinario";
    case 6:
        return "Corruptor";
    case 7:
        return "Heraldo Impío";
    case 8:
        return "Caballero de la Oscuridad";
    case 9:
        return "Señor del Miedo";
    case 10:
        return "Ejecutor Infernal";
    case 11:
        return "Protector del Averno";
    case 12:
        return "Avatar de la Destrucción";
    case 13:
        return "Guardián del Mal";
    default:
        return "Campeón de la Oscuridad";
    }
}

void GiveFactionArmours(std::int16_t user_index, bool is_caos) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    const std::int16_t rango = static_cast<std::int16_t>((is_caos ? user.Faccion.RecompensasCaos : user.Faccion.RecompensasReal) + 1);

    const std::size_t clase = static_cast<std::size_t>(user.clase);
    const std::size_t raza = static_cast<std::size_t>(user.raza);

    const std::array<eTipoDefArmors, 3> tipos_def = {
        eTipoDefArmors::ieBaja,
        eTipoDefArmors::ieMedia,
        eTipoDefArmors::ieAlta
    };

    for (auto tipo_def : tipos_def) {
        Obj item{};
        item.Amount = GetArmourAmount(rango, tipo_def);

        const auto def_idx = static_cast<std::size_t>(tipo_def);
        if (clase < ArmadurasFaccion.size() && raza < ArmadurasFaccion[clase].size()) {
            if (is_caos) {
                item.ObjIndex = ArmadurasFaccion[clase][raza].Caos[def_idx];
            } else {
                item.ObjIndex = ArmadurasFaccion[clase][raza].Armada[def_idx];
            }
        }

        bool inserted = false;
        if (g_callbacks.MeterItemEnInventario) {
            inserted = g_callbacks.MeterItemEnInventario(user_index, item);
        }

        if (!inserted && g_callbacks.TirarItemAlPiso) {
            g_callbacks.TirarItemAlPiso(user.Pos, item);
        }
    }
}

void GiveExpReward(std::int16_t user_index, std::int32_t rango) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    if (rango < 0 || static_cast<std::size_t>(rango) >= RecompensaFacciones.size()) {
        return;
    }

    auto& user = UserList[user_index];
    const std::int32_t given_exp = RecompensaFacciones[static_cast<std::size_t>(rango)];

    user.Stats.Exp += given_exp;
    if (user.Stats.Exp > MAXEXP) {
        user.Stats.Exp = MAXEXP;
    }

    if (g_callbacks.WriteConsoleMsg) {
        g_callbacks.WriteConsoleMsg(user_index, "Has sido recompensado con " + std::to_string(given_exp) + " puntos de experiencia.", 14);
    }

    if (g_callbacks.CheckUserLevel) {
        g_callbacks.CheckUserLevel(user_index);
    }
}

void RecompensaArmadaReal(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    const auto lvl = user.Stats.ELV;
    const auto crimis = user.Faccion.CriminalesMatados;
    const auto next_recom = user.Faccion.NextRecompensa;
    const auto nobleza = user.Reputacion.NobleRep;

    const auto target_npc = user.flags.TargetNPC;
    const std::int16_t target_char_index = (target_npc > 0 && static_cast<std::size_t>(target_npc) < Npclist.size()) ? Npclist[target_npc].char_appearance.CharIndex : static_cast<std::int16_t>(0);

    constexpr std::uint32_t vbWhite = 0x00FFFFFF;

    if (crimis < next_recom) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "Mata " + std::to_string(next_recom - crimis) + " criminales más para recibir la próxima recompensa.", target_char_index, vbWhite);
        }
        return;
    }

    switch (next_recom) {
    case 70:
        user.Faccion.RecompensasReal = 1;
        user.Faccion.NextRecompensa = 130;
        break;
    case 130:
        user.Faccion.RecompensasReal = 2;
        user.Faccion.NextRecompensa = 210;
        break;
    case 210:
        user.Faccion.RecompensasReal = 3;
        user.Faccion.NextRecompensa = 320;
        break;
    case 320:
        user.Faccion.RecompensasReal = 4;
        user.Faccion.NextRecompensa = 460;
        break;
    case 460:
        user.Faccion.RecompensasReal = 5;
        user.Faccion.NextRecompensa = 640;
        break;
    case 640:
        if (lvl < 27) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes criminales, pero te faltan " + std::to_string(27 - lvl) + " niveles para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasReal = 6;
        user.Faccion.NextRecompensa = 870;
        break;
    case 870:
        user.Faccion.RecompensasReal = 7;
        user.Faccion.NextRecompensa = 1160;
        break;
    case 1160:
        user.Faccion.RecompensasReal = 8;
        user.Faccion.NextRecompensa = 2000;
        break;
    case 2000:
        if (lvl < 30) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes criminales, pero te faltan " + std::to_string(30 - lvl) + " niveles para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasReal = 9;
        user.Faccion.NextRecompensa = 2500;
        break;
    case 2500:
        if (nobleza < 2000000) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes criminales, pero te faltan " + std::to_string(2000000 - nobleza) + " puntos de nobleza para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasReal = 10;
        user.Faccion.NextRecompensa = 3000;
        break;
    case 3000:
        if (nobleza < 3000000) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes criminales, pero te faltan " + std::to_string(3000000 - nobleza) + " puntos de nobleza para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasReal = 11;
        user.Faccion.NextRecompensa = 3500;
        break;
    case 3500:
        if (lvl < 35) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes criminales, pero te faltan " + std::to_string(35 - lvl) + " niveles para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        if (nobleza < 4000000) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes criminales, pero te faltan " + std::to_string(4000000 - nobleza) + " puntos de nobleza para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasReal = 12;
        user.Faccion.NextRecompensa = 4000;
        break;
    case 4000:
        if (lvl < 36) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes criminales, pero te faltan " + std::to_string(36 - lvl) + " niveles para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        if (nobleza < 5000000) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes criminales, pero te faltan " + std::to_string(5000000 - nobleza) + " puntos de nobleza para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasReal = 13;
        user.Faccion.NextRecompensa = 5000;
        break;
    case 5000:
        if (lvl < 37) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes criminales, pero te faltan " + std::to_string(37 - lvl) + " niveles para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        if (nobleza < 6000000) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes criminales, pero te faltan " + std::to_string(6000000 - nobleza) + " puntos de nobleza para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasReal = 14;
        user.Faccion.NextRecompensa = 10000;
        break;
    case 10000:
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "Eres uno de mis mejores soldados. Mataste " + std::to_string(crimis) + " criminales, sigue así. Ya no tengo más recompensa para darte que mi agradecimiento. ¡Felicidades!", target_char_index, vbWhite);
        }
        return;
    default:
        return;
    }

    if (g_callbacks.WriteChatOverHead) {
        g_callbacks.WriteChatOverHead(user_index, "¡Aquí tienes tu recompensa " + TituloReal(user_index) + "!", target_char_index, vbWhite);
    }

    GiveFactionArmours(user_index, false);
    GiveExpReward(user_index, user.Faccion.RecompensasReal);
}

void RecompensaCaos(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    const auto lvl = user.Stats.ELV;
    const auto ciudas = user.Faccion.CiudadanosMatados;
    const auto next_recom = user.Faccion.NextRecompensa;

    const auto target_npc = user.flags.TargetNPC;
    const std::int16_t target_char_index = (target_npc > 0 && static_cast<std::size_t>(target_npc) < Npclist.size()) ? Npclist[target_npc].char_appearance.CharIndex : static_cast<std::int16_t>(0);

    constexpr std::uint32_t vbWhite = 0x00FFFFFF;

    if (ciudas < next_recom) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "Mata " + std::to_string(next_recom - ciudas) + " ciudadanos más para recibir la próxima recompensa.", target_char_index, vbWhite);
        }
        return;
    }

    switch (next_recom) {
    case 160:
        user.Faccion.RecompensasCaos = 1;
        user.Faccion.NextRecompensa = 300;
        break;
    case 300:
        user.Faccion.RecompensasCaos = 2;
        user.Faccion.NextRecompensa = 490;
        break;
    case 490:
        user.Faccion.RecompensasCaos = 3;
        user.Faccion.NextRecompensa = 740;
        break;
    case 740:
        user.Faccion.RecompensasCaos = 4;
        user.Faccion.NextRecompensa = 1100;
        break;
    case 1100:
        user.Faccion.RecompensasCaos = 5;
        user.Faccion.NextRecompensa = 1500;
        break;
    case 1500:
        if (lvl < 27) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes ciudadanos, pero te faltan " + std::to_string(27 - lvl) + " niveles para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasCaos = 6;
        user.Faccion.NextRecompensa = 2010;
        break;
    case 2010:
        user.Faccion.RecompensasCaos = 7;
        user.Faccion.NextRecompensa = 2700;
        break;
    case 2700:
        user.Faccion.RecompensasCaos = 8;
        user.Faccion.NextRecompensa = 4600;
        break;
    case 4600:
        if (lvl < 30) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes ciudadanos, pero te faltan " + std::to_string(30 - lvl) + " niveles para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasCaos = 9;
        user.Faccion.NextRecompensa = 5800;
        break;
    case 5800:
        if (lvl < 31) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes ciudadanos, pero te faltan " + std::to_string(31 - lvl) + " niveles para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasCaos = 10;
        user.Faccion.NextRecompensa = 6990;
        break;
    case 6990:
        if (lvl < 33) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes ciudadanos, pero te faltan " + std::to_string(33 - lvl) + " niveles para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasCaos = 11;
        user.Faccion.NextRecompensa = 8100;
        break;
    case 8100:
        if (lvl < 35) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes ciudadanos, pero te faltan " + std::to_string(35 - lvl) + " niveles para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasCaos = 12;
        user.Faccion.NextRecompensa = 9300;
        break;
    case 9300:
        if (lvl < 36) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes ciudadanos, pero te faltan " + std::to_string(36 - lvl) + " niveles para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasCaos = 13;
        user.Faccion.NextRecompensa = 11500;
        break;
    case 11500:
        if (lvl < 37) {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "Mataste suficientes ciudadanos, pero te faltan " + std::to_string(37 - lvl) + " niveles para poder recibir la próxima recompensa.", target_char_index, vbWhite);
            }
            return;
        }
        user.Faccion.RecompensasCaos = 14;
        user.Faccion.NextRecompensa = 23000;
        break;
    case 23000:
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "Eres uno de mis mejores soldados. Mataste " + std::to_string(ciudas) + " ciudadanos . Tu única recompensa será la sangre derramada. ¡Continúa así!", target_char_index, vbWhite);
        }
        return;
    default:
        return;
    }

    if (g_callbacks.WriteChatOverHead) {
        g_callbacks.WriteChatOverHead(user_index, "¡Bien hecho " + TituloCaos(user_index) + ", aquí tienes tu recompensa!", target_char_index, vbWhite);
    }

    GiveFactionArmours(user_index, true);
    GiveExpReward(user_index, user.Faccion.RecompensasCaos);
}

void EnlistarArmadaReal(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    const auto target_npc = user.flags.TargetNPC;
    const std::int16_t target_char_index = (target_npc > 0 && static_cast<std::size_t>(target_npc) < Npclist.size()) ? Npclist[target_npc].char_appearance.CharIndex : static_cast<std::int16_t>(0);

    constexpr std::uint32_t vbWhite = 0x00FFFFFF;

    if (user.Faccion.ArmadaReal == 1) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "¡Ya perteneces a las tropas reales!!! Ve a combatir criminales.", target_char_index, vbWhite);
        }
        return;
    }

    if (user.Faccion.FuerzasCaos == 1) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "¡Maldito insolente!!! Vete de aquí seguidor de las sombras.", target_char_index, vbWhite);
        }
        return;
    }

    if (g_callbacks.IsCriminal && g_callbacks.IsCriminal(user_index)) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "¡No se permiten criminales en el ejército real!!!", target_char_index, vbWhite);
        }
        return;
    }

    if (user.Faccion.CriminalesMatados < 30) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "Para unirte a nuestras fuerzas debes matar al menos 30 criminales, sólo has matado " + std::to_string(user.Faccion.CriminalesMatados) + ".", target_char_index, vbWhite);
        }
        return;
    }

    if (user.Stats.ELV < 25) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "¡Para unirte a nuestras fuerzas debes ser al menos de nivel 25!!!", target_char_index, vbWhite);
        }
        return;
    }

    if (user.Faccion.CiudadanosMatados > 0) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "¡Has asesinado gente inocente, no aceptamos asesinos en las tropas reales!", target_char_index, vbWhite);
        }
        return;
    }

    if (user.Faccion.Reenlistadas > 4) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "¡Has sido expulsado de las fuerzas reales demasiadas veces!", target_char_index, vbWhite);
        }
        return;
    }

    if (user.Reputacion.NobleRep < 1000000) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "Necesitas ser aún más noble para integrar el ejército real, sólo tienes " + std::to_string(user.Reputacion.NobleRep) + "/1.000.000 puntos de nobleza", target_char_index, vbWhite);
        }
        return;
    }

    if (user.GuildIndex > 0 && g_callbacks.GetGuildAlignment) {
        if (g_callbacks.GetGuildAlignment(user.GuildIndex) == "Neutral") {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "¡Perteneces a un clan neutro, sal de él si quieres unirte a nuestras fuerzas!!!", target_char_index, vbWhite);
            }
            return;
        }
    }

    user.Faccion.ArmadaReal = 1;
    user.Faccion.Reenlistadas += 1;

    if (g_callbacks.WriteChatOverHead) {
        g_callbacks.WriteChatOverHead(user_index, "¡Bienvenido al ejército real!!! Aquí tienes tus vestimentas. Cumple bien tu labor exterminando criminales y me encargaré de recompensarte.", target_char_index, vbWhite);
    }

    if (user.Faccion.RecibioArmaduraReal == 0) {
        GiveFactionArmours(user_index, false);
        GiveExpReward(user_index, 0);

        user.Faccion.RecibioArmaduraReal = 1;
        user.Faccion.NivelIngreso = user.Stats.ELV;
        user.Faccion.MatadosIngreso = user.Faccion.CiudadanosMatados;

        user.Faccion.RecibioExpInicialReal = 1;
        user.Faccion.RecompensasReal = 0;
        user.Faccion.NextRecompensa = 70;
    }

    if (user.flags.Navegando && g_callbacks.RefreshCharStatus) {
        g_callbacks.RefreshCharStatus(user_index);
    }

    if (g_callbacks.LogEjercitoReal) {
        g_callbacks.LogEjercitoReal(user.name + " ingresó cuando era nivel " + std::to_string(user.Stats.ELV));
    }
}

void EnlistarCaos(std::int16_t user_index) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    const auto target_npc = user.flags.TargetNPC;
    const std::int16_t target_char_index = (target_npc > 0 && static_cast<std::size_t>(target_npc) < Npclist.size()) ? Npclist[target_npc].char_appearance.CharIndex : static_cast<std::int16_t>(0);

    constexpr std::uint32_t vbWhite = 0x00FFFFFF;

    if (g_callbacks.IsCriminal && !g_callbacks.IsCriminal(user_index)) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "¡Lárgate de aquí, bufón!!!", target_char_index, vbWhite);
        }
        return;
    }

    if (user.Faccion.FuerzasCaos == 1) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "¡Ya perteneces a la legión oscura!!!", target_char_index, vbWhite);
        }
        return;
    }

    if (user.Faccion.ArmadaReal == 1) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "Las sombras reinarán en Argentum. ¡Fuera de aquí insecto real!!!", target_char_index, vbWhite);
        }
        return;
    }

    // Quirk 7.1: Bloqueo Irreversible si alguna vez recibió la experiencia inicial de la Armada Real
    if (user.Faccion.RecibioExpInicialReal == 1) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "No permitiré que ningún insecto real ingrese a mis tropas.", target_char_index, vbWhite);
        }
        return;
    }

    if (user.Faccion.CiudadanosMatados < 70) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "Para unirte a nuestras fuerzas debes matar al menos 70 ciudadanos, sólo has matado " + std::to_string(user.Faccion.CiudadanosMatados) + ".", target_char_index, vbWhite);
        }
        return;
    }

    if (user.Stats.ELV < 25) {
        if (g_callbacks.WriteChatOverHead) {
            g_callbacks.WriteChatOverHead(user_index, "¡Para unirte a nuestras fuerzas debes ser al menos nivel 25!!!", target_char_index, vbWhite);
        }
        return;
    }

    if (user.GuildIndex > 0 && g_callbacks.GetGuildAlignment) {
        if (g_callbacks.GetGuildAlignment(user.GuildIndex) == "Neutral") {
            if (g_callbacks.WriteChatOverHead) {
                g_callbacks.WriteChatOverHead(user_index, "¡Perteneces a un clan neutro, sal de él si quieres unirte a nuestras fuerzas!!!", target_char_index, vbWhite);
            }
            return;
        }
    }

    if (user.Faccion.Reenlistadas > 4) {
        if (g_callbacks.WriteChatOverHead) {
            // Quirk 7.3: Rebelión por Reenlistadas == 200
            if (user.Faccion.Reenlistadas == 200) {
                g_callbacks.WriteChatOverHead(user_index, "Has sido expulsado de las fuerzas oscuras y durante tu rebeldía has atacado a mi ejército. ¡Vete de aquí!", target_char_index, vbWhite);
            } else {
                g_callbacks.WriteChatOverHead(user_index, "¡Has sido expulsado de las fuerzas oscuras demasiadas veces!", target_char_index, vbWhite);
            }
        }
        return;
    }

    user.Faccion.Reenlistadas += 1;
    user.Faccion.FuerzasCaos = 1;

    if (g_callbacks.WriteChatOverHead) {
        g_callbacks.WriteChatOverHead(user_index, "¡Bienvenido al lado oscuro!!! Aquí tienes tus armaduras. Derrama sangre ciudadana y real, y serás recompensado, lo prometo.", target_char_index, vbWhite);
    }

    if (user.Faccion.RecibioArmaduraCaos == 0) {
        GiveFactionArmours(user_index, true);
        GiveExpReward(user_index, 0);

        user.Faccion.RecibioArmaduraCaos = 1;
        user.Faccion.NivelIngreso = user.Stats.ELV;

        user.Faccion.RecibioExpInicialCaos = 1;
        user.Faccion.RecompensasCaos = 0;
        user.Faccion.NextRecompensa = 160;
    }

    if (user.flags.Navegando && g_callbacks.RefreshCharStatus) {
        g_callbacks.RefreshCharStatus(user_index);
    }

    if (g_callbacks.LogEjercitoCaos) {
        g_callbacks.LogEjercitoCaos(user.name + " ingresó cuando era nivel " + std::to_string(user.Stats.ELV));
    }
}

void ExpulsarFaccionReal(std::int16_t user_index, bool expulsado) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    user.Faccion.ArmadaReal = 0;
    // 'Call PerderItemsFaccionarios(UserIndex) - Preservado por Quirk 7.2

    if (expulsado) {
        if (g_callbacks.WriteConsoleMsg) {
            g_callbacks.WriteConsoleMsg(user_index, "¡Has sido expulsado del ejército real!!!", 14);
        }
    } else {
        if (g_callbacks.WriteConsoleMsg) {
            g_callbacks.WriteConsoleMsg(user_index, "¡Te has retirado del ejército real!!!", 14);
        }
    }

    if (user.Invent.ArmourEqpObjIndex != 0 && static_cast<std::size_t>(user.Invent.ArmourEqpObjIndex) < ObjDataList.size()) {
        if (ObjDataList[user.Invent.ArmourEqpObjIndex].Real == 1) {
            if (g_callbacks.Desequipar) {
                g_callbacks.Desequipar(user_index, user.Invent.ArmourEqpSlot);
            }
        }
    }

    if (user.Invent.EscudoEqpObjIndex != 0 && static_cast<std::size_t>(user.Invent.EscudoEqpObjIndex) < ObjDataList.size()) {
        if (ObjDataList[user.Invent.EscudoEqpObjIndex].Real == 1) {
            if (g_callbacks.Desequipar) {
                g_callbacks.Desequipar(user_index, user.Invent.EscudoEqpSlot);
            }
        }
    }

    if (user.flags.Navegando && g_callbacks.RefreshCharStatus) {
        g_callbacks.RefreshCharStatus(user_index);
    }
}

void ExpulsarFaccionCaos(std::int16_t user_index, bool expulsado) {
    if (user_index <= 0 || static_cast<std::size_t>(user_index) >= UserList.size()) {
        return;
    }

    auto& user = UserList[user_index];
    user.Faccion.FuerzasCaos = 0;
    // 'Call PerderItemsFaccionarios(UserIndex) - Preservado por Quirk 7.2

    if (expulsado) {
        if (g_callbacks.WriteConsoleMsg) {
            g_callbacks.WriteConsoleMsg(user_index, "¡Has sido expulsado de la Legión Oscura!!!", 14);
        }
    } else {
        if (g_callbacks.WriteConsoleMsg) {
            g_callbacks.WriteConsoleMsg(user_index, "¡Te has retirado de la Legión Oscura!!!", 14);
        }
    }

    if (user.Invent.ArmourEqpObjIndex != 0 && static_cast<std::size_t>(user.Invent.ArmourEqpObjIndex) < ObjDataList.size()) {
        if (ObjDataList[user.Invent.ArmourEqpObjIndex].Caos == 1) {
            if (g_callbacks.Desequipar) {
                g_callbacks.Desequipar(user_index, user.Invent.ArmourEqpSlot);
            }
        }
    }

    if (user.Invent.EscudoEqpObjIndex != 0 && static_cast<std::size_t>(user.Invent.EscudoEqpObjIndex) < ObjDataList.size()) {
        if (ObjDataList[user.Invent.EscudoEqpObjIndex].Caos == 1) {
            if (g_callbacks.Desequipar) {
                g_callbacks.Desequipar(user_index, user.Invent.EscudoEqpSlot);
            }
        }
    }

    if (user.flags.Navegando && g_callbacks.RefreshCharStatus) {
        g_callbacks.RefreshCharStatus(user_index);
    }
}

} // namespace ModFacciones
