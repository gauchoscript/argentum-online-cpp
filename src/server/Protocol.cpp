
#include "Protocol.hpp"
#include "clsByteQueue.hpp"
#include "Declares.hpp"
#include "clsParty.hpp"
#include "TCP.hpp"
#include <cmath>
#include <algorithm>
#include <array>
#include <span>

namespace ao::net::protocol {

namespace {

constexpr char SEPARATOR = '\0';
constexpr std::uint8_t REDUCTOR_PRECIOVENTA = 3;

inline clsByteQueue* get_user_outgoing(std::int16_t userIndex) {
    if (userIndex < 1 || static_cast<std::size_t>(userIndex) >= UserList.size()) {
        return nullptr;
    }
    return UserList[userIndex].outgoingData.get();
}

inline bool esArmada(std::int16_t userIndex) {
    if (userIndex < 1 || static_cast<std::size_t>(userIndex) >= UserList.size()) return false;
    return UserList[userIndex].Faccion.ArmadaReal == 1;
}

inline bool esCaos(std::int16_t userIndex) {
    if (userIndex < 1 || static_cast<std::size_t>(userIndex) >= UserList.size()) return false;
    return UserList[userIndex].Faccion.FuerzasCaos == 1;
}

inline bool EsGM(std::int16_t userIndex) {
    if (userIndex < 1 || static_cast<std::size_t>(userIndex) >= UserList.size()) return false;
    return (UserList[userIndex].flags.Privilegios & (PlayerType::Admin | PlayerType::Dios | PlayerType::SemiDios | PlayerType::Consejero)) != 0;
}

inline std::int32_t SalePrice(std::int16_t objIndex) {
    if (objIndex < 1 || static_cast<std::size_t>(objIndex) >= ObjDataList.size()) return 0;
    return static_cast<std::int32_t>(ObjDataList[objIndex].Valor / REDUCTOR_PRECIOVENTA);
}

inline float ModHerreriA(eClass clase) {
    if (clase == eClass::Worker) return 1.0f;
    return 4.0f;
}

inline std::int32_t ModCarpinteria(eClass clase) {
    if (clase == eClass::Worker) return 1;
    return 3;
}

inline std::string join_strings_with_separator(std::span<const std::string> items) {
    std::string result;
    for (std::size_t i = 0; i < items.size(); ++i) {
        result += items[i];
        if (i + 1 < items.size()) {
            result += SEPARATOR;
        }
    }
    return result;
}

inline std::vector<std::uint8_t> extract_payload(clsByteQueue& queue) {
    const std::int32_t len = queue.length();
    if (len <= 0) {
        return {};
    }
    std::vector<std::uint8_t> payload(static_cast<std::size_t>(len));
    queue.ReadBlock(payload.data(), len);
    return payload;
}


/**
 * @brief Transacción RAII para reversión atómica de buffers ante fragmentación TCP.
 *
 * Correspondencia con Mecánica Legacy en VB6:
 * En el servidor original (Protocol.bas), la tolerancia a paquetes fragmentados combinaba dos
 * mecanismos manuales: para tramas fijas se verificaba `incomingData.length < N` antes de leer;
 * para tramas con strings dinámicos se instanciaba una cola auxiliar (`Dim buffer As New clsByteQueue`),
 * se copiaba la cola completa con `buffer.CopyBuffer(.incomingData)` y se leía de la copia, o bien
 * se capturaba el error NOT_ENOUGH_DATA y se reinyectaba manualmente el opcode con `.WriteByte(packetID)`.
 *
 * Justificación en C++20:
 * Al carecer VB6 de destructores deterministas (RAII) y manejo moderno de excepciones, en C++ se
 * encapsuló esa misma semántica en esta clase interna dentro de Protocol.cpp. Esto garantiza rollback
 * atómico si salta NotEnoughDataException sin contaminar los 129 case del switch con lógica de
 * restauración manual repetitiva ni exponer abstracciones fuera de la unidad de traducción.
 *
 * Nota de Rendimiento y Deuda Técnica Futura:
 * La implementación actual clona la cola (m_snapshot(queue)), emulando el costo del CopyBuffer de VB6.
 * Para fases posteriores con perfiles de carga masiva de producción, este componente debe optimizarse
 * hacia un esquema de checkpoint de índices/punteros de lectura (read_offset), evitando copias
 * redundantes de memoria en cada tick.
 */
class ByteQueueTransaction {
public:
    explicit ByteQueueTransaction(clsByteQueue& queue)
        : m_queue(queue), m_snapshot(queue), m_committed(false) {}

    void commit() noexcept {
        m_committed = true;
    }

    ~ByteQueueTransaction() {
        if (!m_committed) {
            m_queue = m_snapshot;
        }
    }

    ByteQueueTransaction(const ByteQueueTransaction&) = delete;
    ByteQueueTransaction& operator=(const ByteQueueTransaction&) = delete;
    ByteQueueTransaction(ByteQueueTransaction&&) = delete;
    ByteQueueTransaction& operator=(ByteQueueTransaction&&) = delete;

private:
    clsByteQueue& m_queue;
    clsByteQueue m_snapshot;
    bool m_committed;
};

// ============================================================================
// Stubs Desacoplados de Despacho de Red (Paso 4)
//
// Desviación Intencional respecto a VB6:
// En el servidor legacy, cada Sub Handle... invocaba directamente funciones de juego
// repartidas en múltiples módulos (inventarios, combate, hechizos, NPCs, comercio, etc.).
// En C++20, para mantener la Capa 4 de Red estrictamente desacoplada de las Capas 6 a 10
// que aún no han sido migradas, cada rama de DispatchPacket consume completamente los
// argumentos binarios del stream y delega en un stub vacío (Handle...Stub).
//
// Contrato para Capas Posteriores:
// Cuando se porten las Capas 6 (Usuarios, Inventario, Hechizos), 7 (Combate), 8 (Comercio,
// Clanes, NPCs) y 10 (Admin), cada stub será conectado a su respectivo subsistema.
// ============================================================================
void HandleLoginExistingCharStub(std::int16_t UserIndex, const std::string& UserName, const std::string& Password, std::uint8_t appMajor, std::uint8_t appMinor, std::uint8_t appRevision) {
    (void)UserIndex; (void)UserName; (void)Password; (void)appMajor; (void)appMinor; (void)appRevision;
}

void HandleThrowDicesStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleLoginNewCharStub(std::int16_t UserIndex, const std::string& UserName, const std::string& Password, std::uint8_t appMajor, std::uint8_t appMinor, std::uint8_t appRevision, std::uint8_t race, std::uint8_t gender, std::uint8_t classIndex, std::int16_t head, const std::string& mail, std::uint8_t homeland) {
    (void)UserIndex; (void)UserName; (void)Password; (void)appMajor; (void)appMinor; (void)appRevision; (void)race; (void)gender; (void)classIndex; (void)head; (void)mail; (void)homeland;
}

void HandleTalkStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleYellStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleWhisperStub(std::int16_t UserIndex, std::int16_t arg1, const std::string& arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleWalkStub(std::int16_t UserIndex, std::uint8_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleRequestPositionUpdateStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleAttackStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandlePickUpStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleSafeToggleStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleResuscitationToggleStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleRequestGuildLeaderInfoStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleRequestAtributesStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleRequestFameStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleRequestSkillsStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleRequestMiniStatsStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleCommerceEndStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleCommerceChatStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleUserCommerceEndStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleUserCommerceConfirmStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleBankEndStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleUserCommerceOkStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleUserCommerceRejectStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleDropStub(std::int16_t UserIndex, std::uint8_t arg1, std::int16_t arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleCastSpellStub(std::int16_t UserIndex, std::uint8_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleLeftClickStub(std::int16_t UserIndex, std::uint8_t arg1, std::uint8_t arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleDoubleClickStub(std::int16_t UserIndex, std::uint8_t arg1, std::uint8_t arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleWorkStub(std::int16_t UserIndex, std::uint8_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleUseSpellMacroStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleUseItemStub(std::int16_t UserIndex, std::uint8_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleCraftBlacksmithStub(std::int16_t UserIndex, std::int16_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleCraftCarpenterStub(std::int16_t UserIndex, std::int16_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleWorkLeftClickStub(std::int16_t UserIndex, std::uint8_t arg1, std::uint8_t arg2, std::uint8_t arg3) {
    (void)UserIndex; (void)arg1; (void)arg2; (void)arg3;
}

void HandleCreateNewGuildStub(std::int16_t UserIndex, const std::string& arg1, const std::string& arg2, const std::string& arg3, const std::string& arg4) {
    (void)UserIndex; (void)arg1; (void)arg2; (void)arg3; (void)arg4;
}

void HandleSpellInfoStub(std::int16_t UserIndex, std::uint8_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleEquipItemStub(std::int16_t UserIndex, std::uint8_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleChangeHeadingStub(std::int16_t UserIndex, std::uint8_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleModifySkillsStub(std::int16_t UserIndex, const std::array<std::uint8_t, NUMSKILLS>& points) {
    (void)UserIndex; (void)points;
}

void HandleTrainStub(std::int16_t UserIndex, std::uint8_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleCommerceBuyStub(std::int16_t UserIndex, std::uint8_t arg1, std::int16_t arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleBankExtractItemStub(std::int16_t UserIndex, std::uint8_t arg1, std::int16_t arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleCommerceSellStub(std::int16_t UserIndex, std::uint8_t arg1, std::int16_t arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleBankDepositStub(std::int16_t UserIndex, std::uint8_t arg1, std::int16_t arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleForumPostStub(std::int16_t UserIndex, std::uint8_t arg1, const std::string& arg2, const std::string& arg3) {
    (void)UserIndex; (void)arg1; (void)arg2; (void)arg3;
}

void HandleMoveSpellStub(std::int16_t UserIndex, bool arg1, std::uint8_t arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleMoveBankStub(std::int16_t UserIndex, bool arg1, std::uint8_t arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleClanCodexUpdateStub(std::int16_t UserIndex, const std::string& arg1, const std::string& arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleUserCommerceOfferStub(std::int16_t UserIndex, std::uint8_t arg1, std::int32_t arg2, std::uint8_t arg3) {
    (void)UserIndex; (void)arg1; (void)arg2; (void)arg3;
}

void HandleGuildAcceptPeaceStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildRejectAllianceStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildRejectPeaceStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildAcceptAllianceStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildOfferPeaceStub(std::int16_t UserIndex, const std::string& arg1, const std::string& arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleGuildOfferAllianceStub(std::int16_t UserIndex, const std::string& arg1, const std::string& arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleGuildAllianceDetailsStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildPeaceDetailsStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildRequestJoinerInfoStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildAlliancePropListStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleGuildPeacePropListStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleGuildDeclareWarStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildNewWebsiteStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildAcceptNewMemberStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildRejectNewMemberStub(std::int16_t UserIndex, const std::string& arg1, const std::string& arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleGuildKickMemberStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildUpdateNewsStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildMemberInfoStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildOpenElectionsStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleGuildRequestMembershipStub(std::int16_t UserIndex, const std::string& arg1, const std::string& arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleGuildRequestDetailsStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleOnlineStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleQuitStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleGuildLeaveStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleRequestAccountStateStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandlePetStandStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandlePetFollowStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleReleasePetStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleTrainListStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleRestStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleMeditateStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleResucitateStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleHealStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleHelpStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleRequestStatsStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleCommerceStartStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleBankStartStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleEnlistStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleInformationStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleRewardStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleRequestMOTDStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleUpTimeStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandlePartyLeaveStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandlePartyCreateStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandlePartyJoinStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleInquiryStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleGuildMessageStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandlePartyMessageStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleCentinelReportStub(std::int16_t UserIndex, std::int16_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildOnlineStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandlePartyOnlineStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleCouncilMessageStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleRoleMasterRequestStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGMRequestStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleBugReportStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleChangeDescriptionStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildVoteStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandlePunishmentsStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleChangePasswordStub(std::int16_t UserIndex, const std::string& arg1, const std::string& arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleGambleStub(std::int16_t UserIndex, std::int16_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleInquiryVoteStub(std::int16_t UserIndex, std::uint8_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleLeaveFactionStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleBankExtractGoldStub(std::int16_t UserIndex, std::int32_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleBankDepositGoldStub(std::int16_t UserIndex, std::int32_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleDenounceStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGuildFundateStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleGuildFundationStub(std::int16_t UserIndex, std::uint8_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandlePartyKickStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandlePartySetLeaderStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandlePartyAcceptMemberStub(std::int16_t UserIndex, const std::string& arg1) {
    (void)UserIndex; (void)arg1;
}

void HandlePingStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandlePartyFormStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleItemUpgradeStub(std::int16_t UserIndex, std::int16_t arg1) {
    (void)UserIndex; (void)arg1;
}

void HandleGMCommandsStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleInitCraftingStub(std::int16_t UserIndex, std::int32_t arg1, std::int16_t arg2) {
    (void)UserIndex; (void)arg1; (void)arg2;
}

void HandleHomeStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleShowGuildNewsStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleShareNpcStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleStopSharingNpcStub(std::int16_t UserIndex) {
    (void)UserIndex;
}

void HandleConsultationStub(std::int16_t UserIndex) {
    (void)UserIndex;
}


} // namespace

// ============================================================================
// Generadores Multicast (PrepareMessage...) - Protocol.bas:17259-17805
// ============================================================================

std::vector<std::uint8_t> PrepareMessageSetInvisible(std::int16_t CharIndex, bool invisible) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::SetInvisible));
    buffer.WriteInteger(CharIndex);
    buffer.WriteBoolean(invisible);
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageCharacterChangeNick(std::int16_t CharIndex, std::string_view newNick) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::CharacterChangeNick));
    buffer.WriteInteger(CharIndex);
    buffer.WriteASCIIString(std::string(newNick));
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageChatOverHead(std::string_view Chat, std::int16_t CharIndex, std::int32_t color) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::ChatOverHead));
    buffer.WriteASCIIString(std::string(Chat));
    buffer.WriteInteger(CharIndex);

    // En VB6: se extraen y serializan Ãºnicamente 3 bytes (canales RGB) para ahorrar 1 byte respecto a un Long completo.
    buffer.WriteByte(static_cast<std::uint8_t>(color & 0xFF));
    buffer.WriteByte(static_cast<std::uint8_t>((color & 0xFF00) >> 8));
    buffer.WriteByte(static_cast<std::uint8_t>((color & 0xFF0000) >> 16));

    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageConsoleMsg(std::string_view Chat, FontTypeNames FontIndex) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::ConsoleMsg));
    buffer.WriteASCIIString(std::string(Chat));
    buffer.WriteByte(static_cast<std::uint8_t>(FontIndex));
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageCreateFX(std::int16_t CharIndex, std::int16_t FX, std::int16_t FXLoops) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::CreateFX));
    buffer.WriteInteger(CharIndex);
    buffer.WriteInteger(FX);
    buffer.WriteInteger(FXLoops);
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessagePlayWave(std::uint8_t wave, std::uint8_t X, std::uint8_t Y) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::PlayWave));
    buffer.WriteByte(wave);
    buffer.WriteByte(X);
    buffer.WriteByte(Y);
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageGuildChat(std::string_view Chat) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::GuildChat));
    buffer.WriteASCIIString(std::string(Chat));
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageShowMessageBox(std::string_view Chat) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowMessageBox));
    buffer.WriteASCIIString(std::string(Chat));
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessagePlayMidi(std::uint8_t midi, std::int16_t loops) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::PlayMidi));
    buffer.WriteByte(midi);
    buffer.WriteInteger(loops);
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessagePauseToggle() {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::PauseToggle));
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageRainToggle() {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::RainToggle));
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageObjectDelete(std::uint8_t X, std::uint8_t Y) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::ObjectDelete));
    buffer.WriteByte(X);
    buffer.WriteByte(Y);
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageBlockPosition(std::uint8_t X, std::uint8_t Y, bool Blocked) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::BlockPosition));
    buffer.WriteByte(X);
    buffer.WriteByte(Y);
    buffer.WriteBoolean(Blocked);
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageObjectCreate(std::int16_t GrhIndex, std::uint8_t X, std::uint8_t Y) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::ObjectCreate));
    buffer.WriteByte(X);
    buffer.WriteByte(Y);
    buffer.WriteInteger(GrhIndex);
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageCharacterRemove(std::int16_t CharIndex) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::CharacterRemove));
    buffer.WriteInteger(CharIndex);
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageRemoveCharDialog(std::int16_t CharIndex) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::RemoveCharDialog));
    buffer.WriteInteger(CharIndex);
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageCharacterCreate(std::int16_t body, std::int16_t Head, eHeading heading,
                                                       std::int16_t CharIndex, std::uint8_t X, std::uint8_t Y,
                                                       std::int16_t weapon, std::int16_t shield, std::int16_t FX,
                                                       std::int16_t FXLoops, std::int16_t helmet, std::string_view name,
                                                       std::uint8_t NickColor, std::uint8_t Privileges) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::CharacterCreate));
    buffer.WriteInteger(CharIndex);
    buffer.WriteInteger(body);
    buffer.WriteInteger(Head);
    buffer.WriteByte(static_cast<std::uint8_t>(heading));
    buffer.WriteByte(X);
    buffer.WriteByte(Y);
    buffer.WriteInteger(weapon);
    buffer.WriteInteger(shield);
    buffer.WriteInteger(helmet);
    buffer.WriteInteger(FX);
    buffer.WriteInteger(FXLoops);
    buffer.WriteASCIIString(std::string(name));
    buffer.WriteByte(NickColor);
    buffer.WriteByte(Privileges);
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageCharacterChange(std::int16_t body, std::int16_t Head, eHeading heading,
                                                       std::int16_t CharIndex, std::int16_t weapon, std::int16_t shield,
                                                       std::int16_t FX, std::int16_t FXLoops, std::int16_t helmet) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::CharacterChange));
    buffer.WriteInteger(CharIndex);
    buffer.WriteInteger(body);
    buffer.WriteInteger(Head);
    buffer.WriteByte(static_cast<std::uint8_t>(heading));
    buffer.WriteInteger(weapon);
    buffer.WriteInteger(shield);
    buffer.WriteInteger(helmet);
    buffer.WriteInteger(FX);
    buffer.WriteInteger(FXLoops);
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageCharacterMove(std::int16_t CharIndex, std::uint8_t X, std::uint8_t Y) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::CharacterMove));
    buffer.WriteInteger(CharIndex);
    buffer.WriteByte(X);
    buffer.WriteByte(Y);
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageForceCharMove(eHeading Direccion) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::ForceCharMove));
    buffer.WriteByte(static_cast<std::uint8_t>(Direccion));
    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageUpdateTagAndStatus(std::int16_t UserIndex, std::uint8_t NickColor, std::string_view Tag) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::UpdateTagAndStatus));

    std::int16_t charIndex = 0;
    if (UserIndex > 0 && static_cast<std::size_t>(UserIndex) < UserList.size()) {
        charIndex = UserList[UserIndex].char_appearance.CharIndex;
    }
    buffer.WriteInteger(charIndex);
    buffer.WriteByte(NickColor);
    buffer.WriteASCIIString(std::string(Tag));

    return extract_payload(buffer);
}

std::vector<std::uint8_t> PrepareMessageErrorMsg(std::string_view message) {
    clsByteQueue buffer;
    buffer.WriteByte(static_cast<std::uint8_t>(ServerPacketID::ErrorMsg));
    buffer.WriteASCIIString(std::string(message));
    return extract_payload(buffer);
}

// ============================================================================
// Infraestructura y Vaciado: FlushBuffer
// ============================================================================

void FlushBuffer(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    const std::int32_t len = queue->length();
    if (len <= 0) return;
    std::string sndData = queue->ReadASCIIStringFixed(len);
    TCP::EnviarDatosASlot(UserIndex, sndData);
}

// ============================================================================
// Primitivas de Salida Unicast (102 métodos Write...)
//
// Desviación Intencional respecto a VB6 y Erradicación del Bug #19:
// En VB6 (Protocol.bas), cada rutina de escritura envolvía su lógica en:
//   On Error GoTo Errhandler
//   ...
//   Errhandler:
//     If Err.Number = NOT_ENOUGH_SPACE Then
//       Call FlushBuffer(UserIndex)
//       Resume
//     End If
// Cuando el socket TCP del sistema operativo entraba en WSAEWOULDBLOCK (buffer del kernel
// saturado), FlushBuffer rebotaba y Resume reintentaba la escritura indefinidamente,
// generando un bucle infinito ocupado (busy-loop) al 100% de CPU que congelaba el servidor.
//
// En C++20, este patrón fue formalmente erradicado: las funciones Write... simplemente
// encolan los datos en UserList[UserIndex].outgoingData. La gestión de buffers de envío,
// backpressure y desconexión segura por saturación se delegan a la capa de transporte TCP
// (TCP::EnviarDatosASlot).
// ============================================================================

void WriteRemoveCharDialog(std::int16_t UserIndex, std::int16_t CharIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageRemoveCharDialog(CharIndex);
    queue->WriteBlock(payload);
}

void WriteChatOverHead(std::int16_t UserIndex, std::string_view Chat, std::int16_t CharIndex, std::int32_t color) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageChatOverHead(Chat, CharIndex, color);
    queue->WriteBlock(payload);
}

void WriteConsoleMsg(std::int16_t UserIndex, std::string_view Chat, FontTypeNames FontIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageConsoleMsg(Chat, FontIndex);
    queue->WriteBlock(payload);
}

void WriteGuildChat(std::int16_t UserIndex, std::string_view Chat) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageGuildChat(Chat);
    queue->WriteBlock(payload);
}

void WriteCharacterCreate(std::int16_t UserIndex, std::int16_t body, std::int16_t Head, eHeading heading, std::int16_t CharIndex, std::uint8_t X, std::uint8_t Y, std::int16_t weapon, std::int16_t shield, std::int16_t FX, std::int16_t FXLoops, std::int16_t helmet, std::string_view name, std::uint8_t NickColor, std::uint8_t Privileges) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageCharacterCreate(body, Head, heading, CharIndex, X, Y, weapon, shield, FX, FXLoops, helmet, name, NickColor, Privileges);
    queue->WriteBlock(payload);
}

void WriteCharacterRemove(std::int16_t UserIndex, std::int16_t CharIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageCharacterRemove(CharIndex);
    queue->WriteBlock(payload);
}

void WriteCharacterMove(std::int16_t UserIndex, std::int16_t CharIndex, std::uint8_t X, std::uint8_t Y) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageCharacterMove(CharIndex, X, Y);
    queue->WriteBlock(payload);
}

void WriteForceCharMove(std::int16_t UserIndex, eHeading Direccion) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageForceCharMove(Direccion);
    queue->WriteBlock(payload);
}

void WriteCharacterChange(std::int16_t UserIndex, std::int16_t body, std::int16_t Head, eHeading heading, std::int16_t CharIndex, std::int16_t weapon, std::int16_t shield, std::int16_t FX, std::int16_t FXLoops, std::int16_t helmet) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageCharacterChange(body, Head, heading, CharIndex, weapon, shield, FX, FXLoops, helmet);
    queue->WriteBlock(payload);
}

void WriteObjectCreate(std::int16_t UserIndex, std::int16_t GrhIndex, std::uint8_t X, std::uint8_t Y) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageObjectCreate(GrhIndex, X, Y);
    queue->WriteBlock(payload);
}

void WriteObjectDelete(std::int16_t UserIndex, std::uint8_t X, std::uint8_t Y) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageObjectDelete(X, Y);
    queue->WriteBlock(payload);
}

void WritePlayMidi(std::int16_t UserIndex, std::uint8_t midi, std::int16_t loops) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessagePlayMidi(midi, loops);
    queue->WriteBlock(payload);
}

void WritePlayWave(std::int16_t UserIndex, std::uint8_t wave, std::uint8_t X, std::uint8_t Y) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessagePlayWave(wave, X, Y);
    queue->WriteBlock(payload);
}

void WritePauseToggle(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessagePauseToggle();
    queue->WriteBlock(payload);
}

void WriteRainToggle(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageRainToggle();
    queue->WriteBlock(payload);
}

void WriteCreateFX(std::int16_t UserIndex, std::int16_t CharIndex, std::int16_t FX, std::int16_t FXLoops) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageCreateFX(CharIndex, FX, FXLoops);
    queue->WriteBlock(payload);
}

void WriteErrorMsg(std::int16_t UserIndex, std::string_view message) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageErrorMsg(message);
    queue->WriteBlock(payload);
}

void WriteSetInvisible(std::int16_t UserIndex, std::int16_t CharIndex, bool invisible) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    auto payload = PrepareMessageSetInvisible(CharIndex, invisible);
    queue->WriteBlock(payload);
}

void WriteLoggedMessage(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::Logged));
}

void WriteRemoveAllDialogs(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::RemoveDialogs));
}

void WriteNavigateToggle(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::NavigateToggle));
}

void WriteDisconnect(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::Disconnect));
}

void WriteUserOfferConfirm(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UserOfferConfirm));
}

void WriteCommerceEnd(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::CommerceEnd));
}

void WriteBankEnd(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::BankEnd));
}

void WriteCommerceInit(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::CommerceInit));
}

void WriteUserCommerceEnd(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UserCommerceEnd));
}

void WriteShowBlacksmithForm(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowBlacksmithForm));
}

void WriteShowCarpenterForm(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowCarpenterForm));
}

void WriteRestOK(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::RestOK));
}

void WriteBlind(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::Blind));
}

void WriteDumb(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::Dumb));
}

void WriteMeditateToggle(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::MeditateToggle));
}

void WriteBlindNoMore(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::BlindNoMore));
}

void WriteDumbNoMore(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::DumbNoMore));
}

void WriteShowGuildAlign(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowGuildAlign));
}

void WriteShowGuildFundationForm(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowGuildFundationForm));
}

void WriteParalizeOK(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ParalizeOK));
}

void WriteTradeOK(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::TradeOK));
}

void WriteBankOK(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::BankOK));
}

void WriteShowGMPanelForm(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowGMPanelForm));
}

void WritePong(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::Pong));
}

void WriteStopWorking(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::StopWorking));
}

void WriteBankInit(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::BankInit));
    queue->WriteLong(UserList[UserIndex].Stats.Banco);
}


void WriteUpdateSta(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UpdateSta));
    queue->WriteInteger(UserList[UserIndex].Stats.MinSta);
}

void WriteUpdateMana(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UpdateMana));
    queue->WriteInteger(UserList[UserIndex].Stats.MinMAN);
}

void WriteUpdateHP(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UpdateHP));
    queue->WriteInteger(UserList[UserIndex].Stats.MinHp);
}

void WriteUpdateGold(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UpdateGold));
    queue->WriteLong(UserList[UserIndex].Stats.GLD);
}

void WriteUpdateBankGold(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UpdateBankGold));
    queue->WriteLong(UserList[UserIndex].Stats.Banco);
}

void WriteUpdateExp(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UpdateExp));
    queue->WriteLong(static_cast<std::int32_t>(UserList[UserIndex].Stats.Exp));
}

void WriteUpdateStrenghtAndDexterity(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UpdateStrenghtAndDexterity));
    queue->WriteByte(UserList[UserIndex].Stats.UserAtributos[static_cast<std::size_t>(eAtributos::Fuerza)]);
    queue->WriteByte(UserList[UserIndex].Stats.UserAtributos[static_cast<std::size_t>(eAtributos::Agilidad)]);
}

void WriteUpdateDexterity(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UpdateDexterity));
    queue->WriteByte(UserList[UserIndex].Stats.UserAtributos[static_cast<std::size_t>(eAtributos::Agilidad)]);
}

void WriteUpdateStrenght(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UpdateStrenght));
    queue->WriteByte(UserList[UserIndex].Stats.UserAtributos[static_cast<std::size_t>(eAtributos::Fuerza)]);
}

void WritePosUpdate(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::PosUpdate));
    queue->WriteByte(UserList[UserIndex].Pos.X);
    queue->WriteByte(UserList[UserIndex].Pos.Y);
}

void WriteUserIndexInServer(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UserIndexInServer));
    queue->WriteInteger(UserIndex);
}

void WriteUserCharIndexInServer(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UserCharIndexInServer));
    queue->WriteInteger(UserList[UserIndex].char_appearance.CharIndex);
}

void WriteAreaChanged(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    if (UserIndex <= 0 || static_cast<std::size_t>(UserIndex) >= UserList.size()) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::AreaChanged));
    queue->WriteByte(static_cast<std::uint8_t>(UserList[UserIndex].Pos.X));
    queue->WriteByte(static_cast<std::uint8_t>(UserList[UserIndex].Pos.Y));
}

void WriteUpdateUserStats(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UpdateUserStats));
    const auto& stats = UserList[UserIndex].Stats;
    queue->WriteInteger(stats.MaxHp);
    queue->WriteInteger(stats.MinHp);
    queue->WriteInteger(stats.MaxMAN);
    queue->WriteInteger(stats.MinMAN);
    queue->WriteInteger(stats.MaxSta);
    queue->WriteInteger(stats.MinSta);
    queue->WriteLong(stats.GLD);
    queue->WriteByte(stats.ELV);
    queue->WriteLong(stats.ELU);
    queue->WriteLong(static_cast<std::int32_t>(stats.Exp));
}

void WriteUpdateHungerAndThirst(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UpdateHungerAndThirst));
    const auto& stats = UserList[UserIndex].Stats;
    queue->WriteByte(static_cast<std::uint8_t>(stats.MaxAGU));
    queue->WriteByte(static_cast<std::uint8_t>(stats.MinAGU));
    queue->WriteByte(static_cast<std::uint8_t>(stats.MaxHam));
    queue->WriteByte(static_cast<std::uint8_t>(stats.MinHam));
}

void WriteFame(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::Fame));
    const auto& rep = UserList[UserIndex].Reputacion;
    queue->WriteLong(rep.AsesinoRep);
    queue->WriteLong(rep.BandidoRep);
    queue->WriteLong(rep.BurguesRep);
    queue->WriteLong(rep.LadronesRep);
    queue->WriteLong(rep.NobleRep);
    queue->WriteLong(rep.PlebeRep);
    queue->WriteLong(rep.Promedio);
}

void WriteMiniStats(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::MiniStats));
    queue->WriteLong(UserList[UserIndex].Faccion.CiudadanosMatados);
    queue->WriteLong(UserList[UserIndex].Faccion.CriminalesMatados);
    queue->WriteLong(UserList[UserIndex].Stats.UsuariosMatados);
    queue->WriteInteger(UserList[UserIndex].Stats.NPCsMuertos);
    queue->WriteByte(static_cast<std::uint8_t>(UserList[UserIndex].clase));
    queue->WriteLong(UserList[UserIndex].Counters.Pena);
}

void WriteDiceRoll(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::DiceRoll));
    const auto& attrs = UserList[UserIndex].Stats.UserAtributos;
    queue->WriteByte(attrs[static_cast<std::size_t>(eAtributos::Fuerza)]);
    queue->WriteByte(attrs[static_cast<std::size_t>(eAtributos::Agilidad)]);
    queue->WriteByte(attrs[static_cast<std::size_t>(eAtributos::Inteligencia)]);
    queue->WriteByte(attrs[static_cast<std::size_t>(eAtributos::Carisma)]);
    queue->WriteByte(attrs[static_cast<std::size_t>(eAtributos::Constitucion)]);
}

void WriteAttributes(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::Atributes));
    const auto& attrs = UserList[UserIndex].Stats.UserAtributos;
    queue->WriteByte(attrs[static_cast<std::size_t>(eAtributos::Fuerza)]);
    queue->WriteByte(attrs[static_cast<std::size_t>(eAtributos::Agilidad)]);
    queue->WriteByte(attrs[static_cast<std::size_t>(eAtributos::Inteligencia)]);
    queue->WriteByte(attrs[static_cast<std::size_t>(eAtributos::Carisma)]);
    queue->WriteByte(attrs[static_cast<std::size_t>(eAtributos::Constitucion)]);
}

void WriteSendSkills(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::SendSkills));
    const auto& stats = UserList[UserIndex].Stats;
    for (std::size_t i = 1; i <= NUMSKILLS; ++i) {
        queue->WriteByte(static_cast<std::uint8_t>(stats.UserSkills[i]));
        if (stats.UserSkills[i] < MAXSKILLPOINTS) {
            const auto elu = stats.EluSkills[i];
            const std::uint8_t pct = (elu > 0) ? static_cast<std::uint8_t>((stats.ExpSkills[i] * 100) / elu) : 0;
            queue->WriteByte(pct);
        } else {
            queue->WriteByte(0);
        }
    }
}
void WriteMultiMessage(std::int16_t UserIndex, std::int16_t MessageIndex, std::int32_t Arg1, std::int32_t Arg2, std::int32_t Arg3, std::string_view StringArg1) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;

    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::MultiMessage));
    queue->WriteByte(static_cast<std::uint8_t>(MessageIndex));

    switch (MessageIndex) {
        case static_cast<std::int16_t>(eMessages::NPCHitUser):
            queue->WriteByte(static_cast<std::uint8_t>(Arg1));
            queue->WriteInteger(static_cast<std::int16_t>(Arg2));
            break;
        case static_cast<std::int16_t>(eMessages::UserHitNPC):
            queue->WriteLong(Arg1);
            break;
        case static_cast<std::int16_t>(eMessages::UserAttackedSwing): {
            std::int16_t charIndex = 0;
            if (Arg1 > 0 && static_cast<std::size_t>(Arg1) < UserList.size()) {
                charIndex = UserList[Arg1].char_appearance.CharIndex;
            }
            queue->WriteInteger(charIndex);
            break;
        }
        case static_cast<std::int16_t>(eMessages::UserHittedByUser):
        case static_cast<std::int16_t>(eMessages::UserHittedUser):
            queue->WriteInteger(static_cast<std::int16_t>(Arg1));
            queue->WriteByte(static_cast<std::uint8_t>(Arg2));
            queue->WriteInteger(static_cast<std::int16_t>(Arg3));
            break;
        case static_cast<std::int16_t>(eMessages::WorkRequestTarget):
            queue->WriteByte(static_cast<std::uint8_t>(Arg1));
            break;
        case static_cast<std::int16_t>(eMessages::HaveKilledUser): {
            std::int16_t charIndex = 0;
            if (Arg1 > 0 && static_cast<std::size_t>(Arg1) < UserList.size()) {
                charIndex = UserList[Arg1].char_appearance.CharIndex;
            }
            queue->WriteInteger(charIndex);
            queue->WriteLong(Arg2);
            break;
        }
        case static_cast<std::int16_t>(eMessages::UserKill): {
            std::int16_t charIndex = 0;
            if (Arg1 > 0 && static_cast<std::size_t>(Arg1) < UserList.size()) {
                charIndex = UserList[Arg1].char_appearance.CharIndex;
            }
            queue->WriteInteger(charIndex);
            break;
        }
        case static_cast<std::int16_t>(eMessages::Home):
            queue->WriteByte(static_cast<std::uint8_t>(Arg1));
            queue->WriteInteger(static_cast<std::int16_t>(Arg2));
            queue->WriteASCIIString(std::string(StringArg1));
            break;
        default:
            break;
    }
}

void WriteUserCommerceInit(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UserCommerceInit));
    queue->WriteASCIIString(UserList[UserIndex].ComUsu.DestNick);
}

void WriteChangeMap(std::int16_t UserIndex, std::int16_t Map, std::int16_t version) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ChangeMap));
    queue->WriteInteger(Map);
    queue->WriteInteger(version);
}

void WriteCommerceChat(std::int16_t UserIndex, std::string_view Chat, FontTypeNames FontIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::CommerceChat));
    queue->WriteASCIIString(std::string(Chat));
    queue->WriteByte(static_cast<std::uint8_t>(FontIndex));
}

void WriteShowMessageBox(std::int16_t UserIndex, std::string_view Chat) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowMessageBox));
    queue->WriteASCIIString(std::string(Chat));
}

void WriteBlockPosition(std::int16_t UserIndex, std::uint8_t X, std::uint8_t Y, bool Blocked) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::BlockPosition));
    queue->WriteByte(X);
    queue->WriteByte(Y);
    queue->WriteBoolean(Blocked);
}

void WriteGuildList(std::int16_t UserIndex, std::span<const std::string> guildList) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::guildList));
    queue->WriteASCIIString(join_strings_with_separator(guildList));
}

void WriteWorkRequestTarget(std::int16_t UserIndex, eSkill Skill) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::WorkRequestTarget));
    queue->WriteByte(static_cast<std::uint8_t>(Skill));
}

void WriteChangeInventorySlot(std::int16_t UserIndex, std::uint8_t Slot) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ChangeInventorySlot));
    queue->WriteByte(Slot);

    std::int16_t ObjIndex = 0;
    std::int16_t Amount = 0;
    bool Equipped = false;
    if (Slot <= MAX_INVENTORY_SLOTS) {
        ObjIndex = UserList[UserIndex].Invent.Object[Slot].ObjIndex;
        Amount = UserList[UserIndex].Invent.Object[Slot].Amount;
        Equipped = (UserList[UserIndex].Invent.Object[Slot].Equipped != 0);
    }

    queue->WriteInteger(ObjIndex);
    if (ObjIndex > 0 && static_cast<std::size_t>(ObjIndex) < ObjDataList.size()) {
        const auto& obj = ObjDataList[ObjIndex];
        queue->WriteASCIIString(obj.name);
        queue->WriteInteger(Amount);
        queue->WriteBoolean(Equipped);
        queue->WriteInteger(obj.GrhIndex);
        queue->WriteByte(static_cast<std::uint8_t>(obj.OBJType));
        queue->WriteInteger(obj.MaxHIT);
        queue->WriteInteger(obj.MinHIT);
        queue->WriteInteger(obj.MaxDef);
        queue->WriteInteger(obj.MinDef);
        queue->WriteSingle(static_cast<float>(SalePrice(ObjIndex)));
    } else {
        queue->WriteASCIIString("");
        queue->WriteInteger(0);
        queue->WriteBoolean(false);
        queue->WriteInteger(0);
        queue->WriteByte(0);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteSingle(0.0f);
    }
}

void WriteAddSlots(std::int16_t UserIndex, eMochilas Mochila) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::AddSlots));
    queue->WriteByte(static_cast<std::uint8_t>(Mochila));
}

void WriteChangeBankSlot(std::int16_t UserIndex, std::uint8_t Slot) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ChangeBankSlot));
    queue->WriteByte(Slot);

    std::int16_t ObjIndex = 0;
    std::int16_t Amount = 0;
    if (Slot <= MAX_BANCOINVENTORY_SLOTS) {
        ObjIndex = UserList[UserIndex].BancoInvent.Object[Slot].ObjIndex;
        Amount = UserList[UserIndex].BancoInvent.Object[Slot].Amount;
    }

    queue->WriteInteger(ObjIndex);
    if (ObjIndex > 0 && static_cast<std::size_t>(ObjIndex) < ObjDataList.size()) {
        const auto& obj = ObjDataList[ObjIndex];
        queue->WriteASCIIString(obj.name);
        queue->WriteInteger(Amount);
        queue->WriteInteger(obj.GrhIndex);
        queue->WriteByte(static_cast<std::uint8_t>(obj.OBJType));
        queue->WriteInteger(obj.MaxHIT);
        queue->WriteInteger(obj.MinHIT);
        queue->WriteInteger(obj.MaxDef);
        queue->WriteInteger(obj.MinDef);
        queue->WriteLong(obj.Valor);
    } else {
        queue->WriteASCIIString("");
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteByte(0);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteLong(0);
    }
}

void WriteChangeSpellSlot(std::int16_t UserIndex, std::uint8_t Slot) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ChangeSpellSlot));
    queue->WriteByte(Slot);
    std::int16_t hIndex = 0;
    if (Slot <= MAXUSERHECHIZOS) {
        hIndex = UserList[UserIndex].Stats.UserHechizos[Slot];
    }
    if (hIndex > 0 && static_cast<std::size_t>(hIndex) < Hechizos.size()) {
        queue->WriteInteger(hIndex);
        queue->WriteASCIIString(Hechizos[hIndex].Nombre);
    } else {
        queue->WriteInteger(0);
        queue->WriteASCIIString("(None)");
    }
}

void WriteBlacksmithWeapons(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::BlacksmithWeapons));

    std::string str;
    const auto skill = static_cast<float>(UserList[UserIndex].Stats.UserSkills[static_cast<std::size_t>(eSkill::Herreria)]);
    const float mod = ModHerreriA(UserList[UserIndex].clase);
    const auto maxSkill = static_cast<std::int16_t>(std::round(skill / mod));

    for (std::size_t i = 1; i < ArmasHerrero.size(); ++i) {
        const auto objIdx = ArmasHerrero[i];
        if (objIdx > 0 && static_cast<std::size_t>(objIdx) < ObjDataList.size()) {
            const auto& obj = ObjDataList[objIdx];
            if (obj.SkHerreria <= maxSkill) {
                if (!str.empty()) str += SEPARATOR;
                str += obj.name + " (" + std::to_string(obj.LingH) + " Lingotes de Hierro";
                if (obj.LingP > 0) {
                    str += ", " + std::to_string(obj.LingP) + " Lingotes de Plata";
                }
                if (obj.LingO > 0) {
                    str += ", " + std::to_string(obj.LingO) + " Lingotes de Oro";
                }
                str += ")";
            }
        }
    }
    queue->WriteASCIIString(str);
}

void WriteBlacksmithArmors(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::BlacksmithArmors));

    std::string str;
    const auto skill = static_cast<float>(UserList[UserIndex].Stats.UserSkills[static_cast<std::size_t>(eSkill::Herreria)]);
    const float mod = ModHerreriA(UserList[UserIndex].clase);
    const auto maxSkill = static_cast<std::int16_t>(std::round(skill / mod));

    for (std::size_t i = 1; i < ArmadurasHerrero.size(); ++i) {
        const auto objIdx = ArmadurasHerrero[i];
        if (objIdx > 0 && static_cast<std::size_t>(objIdx) < ObjDataList.size()) {
            const auto& obj = ObjDataList[objIdx];
            if (obj.SkHerreria <= maxSkill) {
                if (!str.empty()) str += SEPARATOR;
                str += obj.name + " (" + std::to_string(obj.LingH) + " Lingotes de Hierro";
                if (obj.LingP > 0) {
                    str += ", " + std::to_string(obj.LingP) + " Lingotes de Plata";
                }
                if (obj.LingO > 0) {
                    str += ", " + std::to_string(obj.LingO) + " Lingotes de Oro";
                }
                str += ")";
            }
        }
    }
    queue->WriteASCIIString(str);
}

void WriteCarpenterObjects(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::CarpenterObjects));

    std::string str;
    const auto skill = UserList[UserIndex].Stats.UserSkills[static_cast<std::size_t>(eSkill::Carpinteria)];
    const auto mod = ModCarpinteria(UserList[UserIndex].clase);
    const auto maxSkill = (mod != 0) ? (skill / mod) : skill;

    for (std::size_t i = 1; i < ObjCarpintero.size(); ++i) {
        const auto objIdx = ObjCarpintero[i];
        if (objIdx > 0 && static_cast<std::size_t>(objIdx) < ObjDataList.size()) {
            const auto& obj = ObjDataList[objIdx];
            if (obj.SkCarpinteria <= maxSkill) {
                if (!str.empty()) str += SEPARATOR;
                str += obj.name + " (" + std::to_string(obj.Madera) + " Madera";
                if (obj.MaderaElfica > 0) {
                    str += ", " + std::to_string(obj.MaderaElfica) + " Madera Elfica";
                }
                str += ")";
            }
        }
    }
    queue->WriteASCIIString(str);
}

void WriteShowSignal(std::int16_t UserIndex, std::string_view Texto, std::int16_t Grh) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowSignal));
    queue->WriteASCIIString(std::string(Texto));
    queue->WriteInteger(Grh);
}

void WriteChangeNPCInventorySlot(std::int16_t UserIndex, std::uint8_t Slot, const Obj& obj, float price) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ChangeNPCInventorySlot));
    queue->WriteByte(Slot);
    if (obj.ObjIndex > 0 && static_cast<std::size_t>(obj.ObjIndex) < ObjDataList.size()) {
        const auto& obj_info = ObjDataList[obj.ObjIndex];
        queue->WriteASCIIString(obj_info.name);
        queue->WriteInteger(obj.Amount);
        queue->WriteSingle(price);
        queue->WriteInteger(obj_info.GrhIndex);
        queue->WriteInteger(obj.ObjIndex);
        queue->WriteByte(static_cast<std::uint8_t>(obj_info.OBJType));
        queue->WriteInteger(obj_info.MaxHIT);
        queue->WriteInteger(obj_info.MinHIT);
        queue->WriteInteger(obj_info.MaxDef);
        queue->WriteInteger(obj_info.MinDef);
    } else {
        queue->WriteASCIIString("");
        queue->WriteInteger(0);
        queue->WriteSingle(0.0f);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteByte(0);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
    }
}

void WriteLevelUp(std::int16_t UserIndex, std::int16_t SkillPoints) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::LevelUp));
    queue->WriteInteger(SkillPoints);
}

void WriteAddForumMsg(std::int16_t UserIndex, eForumType ForumType, std::string_view Title, std::string_view Author, std::string_view message) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::AddForumMsg));
    queue->WriteByte(static_cast<std::uint8_t>(ForumType));
    queue->WriteASCIIString(std::string(Title));
    queue->WriteASCIIString(std::string(Author));
    queue->WriteASCIIString(std::string(message));
}

void WriteShowForumForm(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;

    std::uint8_t forumVisibilities = 0;
    if (esArmada(UserIndex)) {
        forumVisibilities |= 1;
    } else if (esCaos(UserIndex)) {
        forumVisibilities |= 2;
    }
    if (EsGM(UserIndex)) {
        forumVisibilities |= (1 | 2);
    }

    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowForumForm));
    queue->WriteByte(forumVisibilities);
    const bool canSticky = EsGM(UserIndex) || 
        ((UserList[UserIndex].flags.Privilegios & (PlayerType::ChaosCouncil | PlayerType::RoyalCouncil)) != 0);
    queue->WriteBoolean(canSticky);
}

void WriteTrainerCreatureList(std::int16_t UserIndex, std::int16_t NpcIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::TrainerCreatureList));
    std::string str;
    if (NpcIndex > 0 && static_cast<std::size_t>(NpcIndex) < Npclist.size()) {
        const auto& npc = Npclist[NpcIndex];
        for (std::int16_t i = 0; i < npc.NroCriaturas && static_cast<std::size_t>(i) < npc.Criaturas.size(); ++i) {
            if (!str.empty()) str += SEPARATOR;
            str += npc.Criaturas[i].NpcName;
        }
    }
    queue->WriteASCIIString(str);
}

void WriteGuildNews(std::int16_t UserIndex, std::string_view news, std::span<const std::string> enemiesList, std::span<const std::string> alliesList) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::guildNews));
    queue->WriteASCIIString(std::string(news));
    queue->WriteASCIIString(join_strings_with_separator(enemiesList));
    queue->WriteASCIIString(join_strings_with_separator(alliesList));
}

void WriteOfferDetails(std::int16_t UserIndex, std::string_view details) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::OfferDetails));
    queue->WriteASCIIString(std::string(details));
}

void WriteAlianceProposalsList(std::int16_t UserIndex, std::span<const std::string> proposalsList) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::AlianceProposalsList));
    queue->WriteASCIIString(join_strings_with_separator(proposalsList));
}

void WritePeaceProposalsList(std::int16_t UserIndex, std::span<const std::string> proposalsList) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::PeaceProposalsList));
    queue->WriteASCIIString(join_strings_with_separator(proposalsList));
}

void WriteCharacterInfo(std::int16_t UserIndex, std::string_view charName, eRaza race, eClass Class, eGenero gender, std::uint8_t level, std::int32_t gold, std::int32_t bank, std::int32_t reputation, std::string_view previousPetitions, std::string_view currentGuild, std::string_view previousGuilds, bool RoyalArmy, bool CaosLegion, std::int32_t citicensKilled, std::int32_t criminalsKilled) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::CharacterInfo));
    queue->WriteASCIIString(std::string(charName));
    queue->WriteByte(static_cast<std::uint8_t>(race));
    queue->WriteByte(static_cast<std::uint8_t>(Class));
    queue->WriteByte(static_cast<std::uint8_t>(gender));
    queue->WriteByte(level);
    queue->WriteLong(gold);
    queue->WriteLong(bank);
    queue->WriteLong(reputation);
    queue->WriteASCIIString(std::string(previousPetitions));
    queue->WriteASCIIString(std::string(currentGuild));
    queue->WriteASCIIString(std::string(previousGuilds));
    queue->WriteBoolean(RoyalArmy);
    queue->WriteBoolean(CaosLegion);
    queue->WriteLong(citicensKilled);
    queue->WriteLong(criminalsKilled);
}

void WriteGuildLeaderInfo(std::int16_t UserIndex, std::span<const std::string> guildList, std::span<const std::string> MemberList, std::string_view guildNews, std::span<const std::string> joinRequests) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::GuildLeaderInfo));
    queue->WriteASCIIString(join_strings_with_separator(guildList));
    queue->WriteASCIIString(join_strings_with_separator(MemberList));
    queue->WriteASCIIString(std::string(guildNews));
    queue->WriteASCIIString(join_strings_with_separator(joinRequests));
}

void WriteGuildMemberInfo(std::int16_t UserIndex, std::span<const std::string> guildList, std::span<const std::string> MemberList) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::GuildMemberInfo));
    queue->WriteASCIIString(join_strings_with_separator(guildList));
    queue->WriteASCIIString(join_strings_with_separator(MemberList));
}

void WriteGuildDetails(std::int16_t UserIndex, std::string_view GuildName, std::string_view founder, std::string_view foundationDate, std::string_view leader, std::string_view URL, std::int16_t memberCount, bool electionsOpen, std::string_view alignment, std::int16_t enemiesCount, std::int16_t AlliesCount, std::string_view antifactionPoints, std::span<const std::string> codex, std::string_view guildDesc) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::GuildDetails));
    queue->WriteASCIIString(std::string(GuildName));
    queue->WriteASCIIString(std::string(founder));
    queue->WriteASCIIString(std::string(foundationDate));
    queue->WriteASCIIString(std::string(leader));
    queue->WriteASCIIString(std::string(URL));
    queue->WriteInteger(memberCount);
    queue->WriteBoolean(electionsOpen);
    queue->WriteASCIIString(std::string(alignment));
    queue->WriteInteger(enemiesCount);
    queue->WriteInteger(AlliesCount);
    queue->WriteASCIIString(std::string(antifactionPoints));
    queue->WriteASCIIString(join_strings_with_separator(codex));
    queue->WriteASCIIString(std::string(guildDesc));
}

void WriteShowUserRequest(std::int16_t UserIndex, std::string_view details) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowUserRequest));
    queue->WriteASCIIString(std::string(details));
}

void WriteChangeUserTradeSlot(std::int16_t UserIndex, std::uint8_t OfferSlot, std::int16_t ObjIndex, std::int32_t Amount) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ChangeUserTradeSlot));
    queue->WriteByte(OfferSlot);
    queue->WriteInteger(ObjIndex);
    queue->WriteLong(Amount);
    if (ObjIndex > 0 && static_cast<std::size_t>(ObjIndex) < ObjDataList.size()) {
        const auto& obj = ObjDataList[ObjIndex];
        queue->WriteInteger(obj.GrhIndex);
        queue->WriteByte(static_cast<std::uint8_t>(obj.OBJType));
        queue->WriteInteger(obj.MaxHIT);
        queue->WriteInteger(obj.MinHIT);
        queue->WriteInteger(obj.MaxDef);
        queue->WriteInteger(obj.MinDef);
        queue->WriteLong(SalePrice(ObjIndex));
        queue->WriteASCIIString(obj.name);
    } else {
        queue->WriteInteger(0);
        queue->WriteByte(0);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteInteger(0);
        queue->WriteLong(0);
        queue->WriteASCIIString("");
    }
}

void WriteSendNight(std::int16_t UserIndex, bool night) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::SendNight));
    queue->WriteBoolean(night);
}

void WriteSpawnList(std::int16_t UserIndex, std::span<const std::string> npcNames) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::SpawnList));
    queue->WriteASCIIString(join_strings_with_separator(npcNames));
}

void WriteShowSOSForm(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowSOSForm));
    queue->WriteASCIIString("");
}

void WriteShowPartyForm(std::int16_t UserIndex) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowPartyForm));
    const std::int16_t PI = UserList[UserIndex].PartyIndex;
    clsParty* party = (PI > 0 && static_cast<std::size_t>(PI) < Parties.size()) ? Parties[PI].get() : nullptr;
    const std::uint8_t isLeader = party ? (party->EsPartyLeader(UserIndex) ? 1 : 0) : 0;
    queue->WriteByte(isLeader);

    std::string tmp;
    if (party) {
        std::array<std::int16_t, ModParty::PARTY_MAXMEMBERS> members{};
        party->ObtenerMiembrosOnline(members);
        for (std::size_t i = 0; i < ModParty::PARTY_MAXMEMBERS; ++i) {
            const auto mIdx = members[i];
            if (mIdx > 0 && static_cast<std::size_t>(mIdx) < UserList.size()) {
                if (!tmp.empty()) tmp += SEPARATOR;
                tmp += UserList[mIdx].name + " (" + std::to_string(static_cast<long>(party->MiExperiencia(mIdx))) + ")";
            }
        }
    }
    queue->WriteASCIIString(tmp);
    queue->WriteLong(party ? party->ObtenerExperienciaTotal() : 0);
}

void WriteShowMOTDEditionForm(std::int16_t UserIndex, std::string_view currentMOTD) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::ShowMOTDEditionForm));
    queue->WriteASCIIString(std::string(currentMOTD));
}

void WriteUserNameList(std::int16_t UserIndex, std::span<const std::string> userNamesList, std::int16_t cant) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::UserNameList));
    const auto count = std::min<std::size_t>(userNamesList.size(), static_cast<std::size_t>(cant > 0 ? cant : 0));
    queue->WriteASCIIString(join_strings_with_separator(userNamesList.subspan(0, count)));
}

void WriteCancelOfferItem(std::int16_t UserIndex, std::uint8_t Slot) {
    auto* queue = get_user_outgoing(UserIndex);
    if (!queue) return;
    queue->WriteByte(static_cast<std::uint8_t>(ServerPacketID::CancelOfferItem));
    queue->WriteByte(Slot);
}

// ============================================================================
// Paso 4: Despachador Monolítico y Bucle de Recepción
// ============================================================================

PacketParseResult DispatchPacket(std::int16_t UserIndex, ClientPacketID opcode) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return PacketParseResult::MalformedData;
    }
    auto& user = UserList[UserIndex];
    if (!user.incomingData) {
        return PacketParseResult::MalformedData;
    }

    if (static_cast<std::uint8_t>(opcode) > LAST_CLIENT_PACKET_ID) {
        return PacketParseResult::UnknownPacket;
    }

    auto& queue = *user.incomingData;
    ByteQueueTransaction tx(queue);

    try {
        queue.ReadByte(); // Remueve el opcode de la cola

        switch (opcode) {
    case ClientPacketID::LoginExistingChar: {
        std::string UserName = queue.ReadASCIIString();
        std::string Password = queue.ReadASCIIString();
        std::uint8_t appMajor = queue.ReadByte();
        std::uint8_t appMinor = queue.ReadByte();
        std::uint8_t appRevision = queue.ReadByte();
        HandleLoginExistingCharStub(UserIndex, UserName, Password, appMajor, appMinor, appRevision);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::ThrowDices: {
        HandleThrowDicesStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::LoginNewChar: {
        std::string UserName = queue.ReadASCIIString();
        std::string Password = queue.ReadASCIIString();
        std::uint8_t appMajor = queue.ReadByte();
        std::uint8_t appMinor = queue.ReadByte();
        std::uint8_t appRevision = queue.ReadByte();
        std::uint8_t race = queue.ReadByte();
        std::uint8_t gender = queue.ReadByte();
        std::uint8_t classIndex = queue.ReadByte();
        std::int16_t head = queue.ReadInteger();
        std::string mail = queue.ReadASCIIString();
        std::uint8_t homeland = queue.ReadByte();
        HandleLoginNewCharStub(UserIndex, UserName, Password, appMajor, appMinor, appRevision, race, gender, classIndex, head, mail, homeland);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Talk: {
        std::string arg1 = queue.ReadASCIIString();
        HandleTalkStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Yell: {
        std::string arg1 = queue.ReadASCIIString();
        HandleYellStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Whisper: {
        std::int16_t arg1 = queue.ReadInteger();
        std::string arg2 = queue.ReadASCIIString();
        HandleWhisperStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Walk: {
        std::uint8_t arg1 = queue.ReadByte();
        HandleWalkStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::RequestPositionUpdate: {
        HandleRequestPositionUpdateStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Attack: {
        HandleAttackStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::PickUp: {
        HandlePickUpStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::SafeToggle: {
        HandleSafeToggleStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::ResuscitationSafeToggle: {
        HandleResuscitationToggleStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::RequestGuildLeaderInfo: {
        HandleRequestGuildLeaderInfoStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::RequestAtributes: {
        HandleRequestAtributesStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::RequestFame: {
        HandleRequestFameStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::RequestSkills: {
        HandleRequestSkillsStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::RequestMiniStats: {
        HandleRequestMiniStatsStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::CommerceEnd: {
        HandleCommerceEndStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::CommerceChat: {
        std::string arg1 = queue.ReadASCIIString();
        HandleCommerceChatStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::UserCommerceEnd: {
        HandleUserCommerceEndStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::UserCommerceConfirm: {
        HandleUserCommerceConfirmStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::BankEnd: {
        HandleBankEndStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::UserCommerceOk: {
        HandleUserCommerceOkStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::UserCommerceReject: {
        HandleUserCommerceRejectStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Drop: {
        std::uint8_t arg1 = queue.ReadByte();
        std::int16_t arg2 = queue.ReadInteger();
        HandleDropStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::CastSpell: {
        std::uint8_t arg1 = queue.ReadByte();
        HandleCastSpellStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::LeftClick: {
        std::uint8_t arg1 = queue.ReadByte();
        std::uint8_t arg2 = queue.ReadByte();
        HandleLeftClickStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::DoubleClick: {
        std::uint8_t arg1 = queue.ReadByte();
        std::uint8_t arg2 = queue.ReadByte();
        HandleDoubleClickStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Work: {
        std::uint8_t arg1 = queue.ReadByte();
        HandleWorkStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::UseSpellMacro: {
        HandleUseSpellMacroStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::UseItem: {
        std::uint8_t arg1 = queue.ReadByte();
        HandleUseItemStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::CraftBlacksmith: {
        std::int16_t arg1 = queue.ReadInteger();
        HandleCraftBlacksmithStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::CraftCarpenter: {
        std::int16_t arg1 = queue.ReadInteger();
        HandleCraftCarpenterStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::WorkLeftClick: {
        std::uint8_t arg1 = queue.ReadByte();
        std::uint8_t arg2 = queue.ReadByte();
        std::uint8_t arg3 = queue.ReadByte();
        HandleWorkLeftClickStub(UserIndex, arg1, arg2, arg3);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::CreateNewGuild: {
        std::string arg1 = queue.ReadASCIIString();
        std::string arg2 = queue.ReadASCIIString();
        std::string arg3 = queue.ReadASCIIString();
        std::string arg4 = queue.ReadASCIIString();
        HandleCreateNewGuildStub(UserIndex, arg1, arg2, arg3, arg4);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::SpellInfo: {
        std::uint8_t arg1 = queue.ReadByte();
        HandleSpellInfoStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::EquipItem: {
        std::uint8_t arg1 = queue.ReadByte();
        HandleEquipItemStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::ChangeHeading: {
        std::uint8_t arg1 = queue.ReadByte();
        HandleChangeHeadingStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::ModifySkills: {
        std::array<std::uint8_t, NUMSKILLS> points{};
        for (std::size_t i = 0; i < NUMSKILLS; ++i) {
            points[i] = queue.ReadByte();
        }
        HandleModifySkillsStub(UserIndex, points);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Train: {
        std::uint8_t arg1 = queue.ReadByte();
        HandleTrainStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::CommerceBuy: {
        std::uint8_t arg1 = queue.ReadByte();
        std::int16_t arg2 = queue.ReadInteger();
        HandleCommerceBuyStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::BankExtractItem: {
        std::uint8_t arg1 = queue.ReadByte();
        std::int16_t arg2 = queue.ReadInteger();
        HandleBankExtractItemStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::CommerceSell: {
        std::uint8_t arg1 = queue.ReadByte();
        std::int16_t arg2 = queue.ReadInteger();
        HandleCommerceSellStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::BankDeposit: {
        std::uint8_t arg1 = queue.ReadByte();
        std::int16_t arg2 = queue.ReadInteger();
        HandleBankDepositStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::ForumPost: {
        std::uint8_t arg1 = queue.ReadByte();
        std::string arg2 = queue.ReadASCIIString();
        std::string arg3 = queue.ReadASCIIString();
        HandleForumPostStub(UserIndex, arg1, arg2, arg3);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::MoveSpell: {
        bool arg1 = queue.ReadBoolean();
        std::uint8_t arg2 = queue.ReadByte();
        HandleMoveSpellStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::MoveBank: {
        bool arg1 = queue.ReadBoolean();
        std::uint8_t arg2 = queue.ReadByte();
        HandleMoveBankStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::ClanCodexUpdate: {
        std::string arg1 = queue.ReadASCIIString();
        std::string arg2 = queue.ReadASCIIString();
        HandleClanCodexUpdateStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::UserCommerceOffer: {
        std::uint8_t arg1 = queue.ReadByte();
        std::int32_t arg2 = queue.ReadLong();
        std::uint8_t arg3 = queue.ReadByte();
        HandleUserCommerceOfferStub(UserIndex, arg1, arg2, arg3);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildAcceptPeace: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildAcceptPeaceStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildRejectAlliance: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildRejectAllianceStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildRejectPeace: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildRejectPeaceStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildAcceptAlliance: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildAcceptAllianceStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildOfferPeace: {
        std::string arg1 = queue.ReadASCIIString();
        std::string arg2 = queue.ReadASCIIString();
        HandleGuildOfferPeaceStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildOfferAlliance: {
        std::string arg1 = queue.ReadASCIIString();
        std::string arg2 = queue.ReadASCIIString();
        HandleGuildOfferAllianceStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildAllianceDetails: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildAllianceDetailsStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildPeaceDetails: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildPeaceDetailsStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildRequestJoinerInfo: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildRequestJoinerInfoStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildAlliancePropList: {
        HandleGuildAlliancePropListStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildPeacePropList: {
        HandleGuildPeacePropListStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildDeclareWar: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildDeclareWarStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildNewWebsite: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildNewWebsiteStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildAcceptNewMember: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildAcceptNewMemberStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildRejectNewMember: {
        std::string arg1 = queue.ReadASCIIString();
        std::string arg2 = queue.ReadASCIIString();
        HandleGuildRejectNewMemberStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildKickMember: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildKickMemberStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildUpdateNews: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildUpdateNewsStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildMemberInfo: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildMemberInfoStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildOpenElections: {
        HandleGuildOpenElectionsStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildRequestMembership: {
        std::string arg1 = queue.ReadASCIIString();
        std::string arg2 = queue.ReadASCIIString();
        HandleGuildRequestMembershipStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildRequestDetails: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildRequestDetailsStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Online: {
        HandleOnlineStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Quit: {
        HandleQuitStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildLeave: {
        HandleGuildLeaveStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::RequestAccountState: {
        HandleRequestAccountStateStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::PetStand: {
        HandlePetStandStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::PetFollow: {
        HandlePetFollowStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::ReleasePet: {
        HandleReleasePetStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::TrainList: {
        HandleTrainListStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Rest: {
        HandleRestStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Meditate: {
        HandleMeditateStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Resucitate: {
        HandleResucitateStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Heal: {
        HandleHealStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Help: {
        HandleHelpStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::RequestStats: {
        HandleRequestStatsStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::CommerceStart: {
        HandleCommerceStartStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::BankStart: {
        HandleBankStartStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Enlist: {
        HandleEnlistStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Information: {
        HandleInformationStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Reward: {
        HandleRewardStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::RequestMOTD: {
        HandleRequestMOTDStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::UpTime: {
        HandleUpTimeStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::PartyLeave: {
        HandlePartyLeaveStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::PartyCreate: {
        HandlePartyCreateStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::PartyJoin: {
        HandlePartyJoinStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Inquiry: {
        HandleInquiryStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildMessage: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildMessageStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::PartyMessage: {
        std::string arg1 = queue.ReadASCIIString();
        HandlePartyMessageStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::CentinelReport: {
        std::int16_t arg1 = queue.ReadInteger();
        HandleCentinelReportStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildOnline: {
        HandleGuildOnlineStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::PartyOnline: {
        HandlePartyOnlineStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::CouncilMessage: {
        std::string arg1 = queue.ReadASCIIString();
        HandleCouncilMessageStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::RoleMasterRequest: {
        std::string arg1 = queue.ReadASCIIString();
        HandleRoleMasterRequestStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GMRequest: {
        HandleGMRequestStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::bugReport: {
        std::string arg1 = queue.ReadASCIIString();
        HandleBugReportStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::ChangeDescription: {
        std::string arg1 = queue.ReadASCIIString();
        HandleChangeDescriptionStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildVote: {
        std::string arg1 = queue.ReadASCIIString();
        HandleGuildVoteStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Punishments: {
        std::string arg1 = queue.ReadASCIIString();
        HandlePunishmentsStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::ChangePassword: {
        std::string arg1 = queue.ReadASCIIString();
        std::string arg2 = queue.ReadASCIIString();
        HandleChangePasswordStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Gamble: {
        std::int16_t arg1 = queue.ReadInteger();
        HandleGambleStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::InquiryVote: {
        std::uint8_t arg1 = queue.ReadByte();
        HandleInquiryVoteStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::LeaveFaction: {
        HandleLeaveFactionStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::BankExtractGold: {
        std::int32_t arg1 = queue.ReadLong();
        HandleBankExtractGoldStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::BankDepositGold: {
        std::int32_t arg1 = queue.ReadLong();
        HandleBankDepositGoldStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Denounce: {
        std::string arg1 = queue.ReadASCIIString();
        HandleDenounceStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildFundate: {
        HandleGuildFundateStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GuildFundation: {
        std::uint8_t arg1 = queue.ReadByte();
        HandleGuildFundationStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::PartyKick: {
        std::string arg1 = queue.ReadASCIIString();
        HandlePartyKickStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::PartySetLeader: {
        std::string arg1 = queue.ReadASCIIString();
        HandlePartySetLeaderStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::PartyAcceptMember: {
        std::string arg1 = queue.ReadASCIIString();
        HandlePartyAcceptMemberStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Ping: {
        HandlePingStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::RequestPartyForm: {
        HandlePartyFormStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::ItemUpgrade: {
        std::int16_t arg1 = queue.ReadInteger();
        HandleItemUpgradeStub(UserIndex, arg1);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::GMCommands: {
        HandleGMCommandsStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::InitCrafting: {
        std::int32_t arg1 = queue.ReadLong();
        std::int16_t arg2 = queue.ReadInteger();
        HandleInitCraftingStub(UserIndex, arg1, arg2);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Home: {
        HandleHomeStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::ShowGuildNews: {
        HandleShowGuildNewsStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::ShareNpc: {
        HandleShareNpcStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::StopSharingNpc: {
        HandleStopSharingNpcStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }
    case ClientPacketID::Consultation: {
        HandleConsultationStub(UserIndex);
        tx.commit();
        return PacketParseResult::Ok;
    }

        default:
            return PacketParseResult::UnknownPacket;
        }
    } catch (const NotEnoughDataException&) {
        return PacketParseResult::NeedMoreData;
    } catch (...) {
        return PacketParseResult::MalformedData;
    }
}

void HandleIncomingData(std::int16_t UserIndex) {
    if (UserIndex < 1 || static_cast<std::size_t>(UserIndex) >= UserList.size()) {
        return;
    }
    auto& user = UserList[UserIndex];
    if (!user.incomingData) {
        return;
    }

    while (user.incomingData->length() > 0) {
        std::uint8_t raw_opcode = 0;
        try {
            raw_opcode = user.incomingData->PeekByte();
        } catch (const NotEnoughDataException&) {
            break;
        }

        auto opcode = static_cast<ClientPacketID>(raw_opcode);

        // Validación de sesión según especificación oficial
        if (!user.flags.UserLogged) {
            if (opcode != ClientPacketID::LoginExistingChar &&
                opcode != ClientPacketID::ThrowDices &&
                opcode != ClientPacketID::LoginNewChar) {
                TCP::CloseSocket(UserIndex);
                return;
            }
        } else {
            if (opcode == ClientPacketID::LoginExistingChar ||
                opcode == ClientPacketID::LoginNewChar) {
                TCP::CloseSocket(UserIndex);
                return;
            }
        }

        if (raw_opcode <= LAST_CLIENT_PACKET_ID) {
            user.Counters.IdleCount = 0;
            user.flags.NoPuedeSerAtacado = false;
        }

        PacketParseResult res = DispatchPacket(UserIndex, opcode);

        if (res == PacketParseResult::NeedMoreData) {
            break;
        }

        if (res == PacketParseResult::MalformedData || res == PacketParseResult::UnknownPacket) {
            TCP::CloseSocket(UserIndex);
            return;
        }
    }

    FlushBuffer(UserIndex);
}

} // namespace ao::net::protocol
