#ifndef CLSPARTY_HPP
#define CLSPARTY_HPP

#include <cstdint>
#include <array>
#include <string>
#include <string_view>
#include <memory>

namespace ModParty {

constexpr std::int16_t MAX_PARTIES = 300;
constexpr std::uint8_t PARTY_MAXMEMBERS = 5;
constexpr std::uint8_t MAXDISTANCIAINGRESOPARTY = 2;
constexpr std::uint8_t PARTY_MAXDISTANCIA = 18;
constexpr bool PARTY_EXPERIENCIAPORGOLPE = false;

constexpr std::uint8_t MINPARTYLEVEL = 15;
constexpr std::uint8_t MAXPARTYDELTALEVEL = 7;
constexpr bool CASTIGOS = false;

struct tPartyMember {
    std::int16_t UserIndex = 0;
    double Experiencia = 0.0;
};

} // namespace ModParty

class clsParty {
public:
    clsParty();
    ~clsParty() = default;

    void Class_Initialize();
    void Class_Terminate();

    void UpdateSumaNivelesElevados(std::int16_t lvl);
    std::int32_t MiExperiencia(std::int16_t user_index) const;
    void ObtenerExito(std::int32_t exp_ganada, std::int16_t mapa, std::int16_t x, std::int16_t y);
    void MandarMensajeAConsola(std::string_view texto, std::string_view sender);
    bool EsPartyLeader(std::int16_t user_index) const;
    bool NuevoMiembro(std::int16_t user_index);
    bool SaleMiembro(std::int16_t user_index);
    bool HacerLeader(std::int16_t user_index);
    void ObtenerMiembrosOnline(std::array<std::int16_t, ModParty::PARTY_MAXMEMBERS>& member_list) const;
    std::int32_t ObtenerExperienciaTotal() const;
    bool PuedeEntrar(std::int16_t user_index, std::string& razon) const;
    void FlushExperiencia();
    std::int16_t CantMiembros() const;

    // Direct accessors for internal state / testing
    std::int16_t Fundador() const { return p_Fundador; }
    float SumaNivelesElevados() const { return p_SumaNivelesElevados; }
    const std::array<ModParty::tPartyMember, ModParty::PARTY_MAXMEMBERS>& GetMembers() const { return p_members; }

private:
    void CompactMemberList();

    std::array<ModParty::tPartyMember, ModParty::PARTY_MAXMEMBERS> p_members{};
    std::int32_t p_expTotal = 0;
    std::int16_t p_Fundador = 0;
    std::int16_t p_CantMiembros = 0;
    float p_SumaNivelesElevados = 0.0f; // MUST be float for exact VB6 Single parity!
};

#endif // CLSPARTY_HPP
