#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include <cstdint>
#include <cstddef>
#include <vector>
#include <string>
#include <string_view>
#include <span>

#include "Declares.hpp"

namespace ao::net::protocol {

/**
 * @brief Identificadores de paquetes salientes (Servidor -> Cliente).
 * Mapeo byte-exacto de ServerPacketID de VB6 (Protocol.bas:46-154).
 * En VB6 los enumeradores sin inicializador explícito inician en 0.
 */
enum class ServerPacketID : std::uint8_t {
    Logged = 0, // LOGGED
    RemoveDialogs, // QTDL
    RemoveCharDialog, // QDL
    NavigateToggle, // NAVEG
    Disconnect, // FINOK
    CommerceEnd, // FINCOMOK
    BankEnd, // FINBANOK
    CommerceInit, // INITCOM
    BankInit, // INITBANCO
    UserCommerceInit, // INITCOMUSU
    UserCommerceEnd, // FINCOMUSUOK
    UserOfferConfirm,
    CommerceChat,
    ShowBlacksmithForm, // SFH
    ShowCarpenterForm, // SFC
    UpdateSta, // ASS
    UpdateMana, // ASM
    UpdateHP, // ASH
    UpdateGold, // ASG
    UpdateBankGold,
    UpdateExp, // ASE
    ChangeMap, // CM
    PosUpdate, // PU
    ChatOverHead, // ||
    ConsoleMsg, // || - Beware!! its the same as above, but it was properly splitted
    GuildChat, // |+
    ShowMessageBox, // !!
    UserIndexInServer, // IU
    UserCharIndexInServer, // IP
    CharacterCreate, // CC
    CharacterRemove, // BP
    CharacterChangeNick,
    CharacterMove, // MP, +, * and _ '
    ForceCharMove,
    CharacterChange, // CP
    ObjectCreate, // HO
    ObjectDelete, // BO
    BlockPosition, // BQ
    PlayMidi, // TM
    PlayWave, // TW
    guildList, // GL
    AreaChanged, // CA
    PauseToggle, // BKW
    RainToggle, // LLU
    CreateFX, // CFX
    UpdateUserStats, // EST
    WorkRequestTarget, // T01
    ChangeInventorySlot, // CSI
    ChangeBankSlot, // SBO
    ChangeSpellSlot, // SHS
    Atributes, // ATR
    BlacksmithWeapons, // LAH
    BlacksmithArmors, // LAR
    CarpenterObjects, // OBR
    RestOK, // DOK
    ErrorMsg, // ERR
    Blind, // CEGU
    Dumb, // DUMB
    ShowSignal, // MCAR
    ChangeNPCInventorySlot, // NPCI
    UpdateHungerAndThirst, // EHYS
    Fame, // FAMA
    MiniStats, // MEST
    LevelUp, // SUNI
    AddForumMsg, // FMSG
    ShowForumForm, // MFOR
    SetInvisible, // NOVER
    DiceRoll, // DADOS
    MeditateToggle, // MEDOK
    BlindNoMore, // NSEGUE
    DumbNoMore, // NESTUP
    SendSkills, // SKILLS
    TrainerCreatureList, // LSTCRI
    guildNews, // GUILDNE
    OfferDetails, // PEACEDE & ALLIEDE
    AlianceProposalsList, // ALLIEPR
    PeaceProposalsList, // PEACEPR
    CharacterInfo, // CHRINFO
    GuildLeaderInfo, // LEADERI
    GuildMemberInfo,
    GuildDetails, // CLANDET
    ShowGuildFundationForm, // SHOWFUN
    ParalizeOK, // PARADOK
    ShowUserRequest, // PETICIO
    TradeOK, // TRANSOK
    BankOK, // BANCOOK
    ChangeUserTradeSlot, // COMUSUINV
    SendNight, // NOC
    Pong,
    UpdateTagAndStatus,
    SpawnList, // SPL
    ShowSOSForm, // MSOS
    ShowMOTDEditionForm, // ZMOTD
    ShowGMPanelForm, // ABPANEL
    UserNameList, // LISTUSU
    ShowGuildAlign,
    ShowPartyForm,
    UpdateStrenghtAndDexterity,
    UpdateStrenght,
    UpdateDexterity,
    AddSlots,
    MultiMessage,
    StopWorking,
    CancelOfferItem
};

/**
 * @brief Identificadores de paquetes entrantes (Cliente -> Servidor).
 * Mapeo byte-exacto de ClientPacketID de VB6 (Protocol.bas:156-286).
 * En VB6 los enumeradores sin inicializador explícito inician en 0.
 */
enum class ClientPacketID : std::uint8_t {
    LoginExistingChar = 0, // OLOGIN
    ThrowDices, // TIRDAD
    LoginNewChar, // NLOGIN
    Talk, // ;
    Yell, // -
    Whisper, /* \ */
    Walk, // M
    RequestPositionUpdate, // RPU
    Attack, // AT
    PickUp, // AG
    SafeToggle, // /SEG & SEG  (SEG's behaviour has to be coded in the client)
    ResuscitationSafeToggle,
    RequestGuildLeaderInfo, // GLINFO
    RequestAtributes, // ATR
    RequestFame, // FAMA
    RequestSkills, // ESKI
    RequestMiniStats, // FEST
    CommerceEnd, // FINCOM
    UserCommerceEnd, // FINCOMUSU
    UserCommerceConfirm,
    CommerceChat,
    BankEnd, // FINBAN
    UserCommerceOk, // COMUSUOK
    UserCommerceReject, // COMUSUNO
    Drop, // TI
    CastSpell, // LH
    LeftClick, // LC
    DoubleClick, // RC
    Work, // UK
    UseSpellMacro, // UMH
    UseItem, // USA
    CraftBlacksmith, // CNS
    CraftCarpenter, // CNC
    WorkLeftClick, // WLC
    CreateNewGuild, // CIG
    SpellInfo, // INFS
    EquipItem, // EQUI
    ChangeHeading, // CHEA
    ModifySkills, // SKSE
    Train, // ENTR
    CommerceBuy, // COMP
    BankExtractItem, // RETI
    CommerceSell, // VEND
    BankDeposit, // DEPO
    ForumPost, // DEMSG
    MoveSpell, // DESPHE
    MoveBank,
    ClanCodexUpdate, // DESCOD
    UserCommerceOffer, // OFRECER
    GuildAcceptPeace, // ACEPPEAT
    GuildRejectAlliance, // RECPALIA
    GuildRejectPeace, // RECPPEAT
    GuildAcceptAlliance, // ACEPALIA
    GuildOfferPeace, // PEACEOFF
    GuildOfferAlliance, // ALLIEOFF
    GuildAllianceDetails, // ALLIEDET
    GuildPeaceDetails, // PEACEDET
    GuildRequestJoinerInfo, // ENVCOMEN
    GuildAlliancePropList, // ENVALPRO
    GuildPeacePropList, // ENVPROPP
    GuildDeclareWar, // DECGUERR
    GuildNewWebsite, // NEWWEBSI
    GuildAcceptNewMember, // ACEPTARI
    GuildRejectNewMember, // RECHAZAR
    GuildKickMember, // ECHARCLA
    GuildUpdateNews, // ACTGNEWS
    GuildMemberInfo, // 1HRINFO<
    GuildOpenElections, // ABREELEC
    GuildRequestMembership, // SOLICITUD
    GuildRequestDetails, // CLANDETAILS
    Online, // /ONLINE
    Quit, // /SALIR
    GuildLeave, // /SALIRCLAN
    RequestAccountState, // /BALANCE
    PetStand, // /QUIETO
    PetFollow, // /ACOMPAÑAR
    ReleasePet, // /LIBERAR
    TrainList, // /ENTRENAR
    Rest, // /DESCANSAR
    Meditate, // /MEDITAR
    Resucitate, // /RESUCITAR
    Heal, // /CURAR
    Help, // /AYUDA
    RequestStats, // /EST
    CommerceStart, // /COMERCIAR
    BankStart, // /BOVEDA
    Enlist, // /ENLISTAR
    Information, // /INFORMACION
    Reward, // /RECOMPENSA
    RequestMOTD, // /MOTD
    UpTime, // /UPTIME
    PartyLeave, // /SALIRPARTY
    PartyCreate, // /CREARPARTY
    PartyJoin, // /PARTY
    Inquiry, // /ENCUESTA ( with no params )
    GuildMessage, // /CMSG
    PartyMessage, // /PMSG
    CentinelReport, // /CENTINELA
    GuildOnline, // /ONLINECLAN
    PartyOnline, // /ONLINEPARTY
    CouncilMessage, // /BMSG
    RoleMasterRequest, // /ROL
    GMRequest, // /GM
    bugReport, // /_BUG
    ChangeDescription, // /DESC
    GuildVote, // /VOTO
    Punishments, // /PENAS
    ChangePassword, // /CONTRASEÑA
    Gamble, // /APOSTAR
    InquiryVote, // /ENCUESTA ( with parameters )
    LeaveFaction, // /RETIRAR ( with no arguments )
    BankExtractGold, // /RETIRAR ( with arguments )
    BankDepositGold, // /DEPOSITAR
    Denounce, // /DENUNCIAR
    GuildFundate, // /FUNDARCLAN
    GuildFundation,
    PartyKick, // /ECHARPARTY
    PartySetLeader, // /PARTYLIDER
    PartyAcceptMember, // /ACCEPTPARTY
    Ping, // /PING
    RequestPartyForm,
    ItemUpgrade,
    GMCommands,
    InitCrafting,
    Home,
    ShowGuildNews,
    ShareNpc, // /COMPARTIR
    StopSharingNpc,
    Consultation
};

/**
 * @brief Último ID existente de paquete de cliente (Protocol.bas:289).
 */
inline constexpr std::uint8_t LAST_CLIENT_PACKET_ID = 128;

/**
 * @brief Estilos de fuente de consola de chat (Protocol.bas:291-313).
 */
enum class FontTypeNames : std::uint8_t {
    FONTTYPE_TALK = 0,
    FONTTYPE_FIGHT,
    FONTTYPE_WARNING,
    FONTTYPE_INFO,
    FONTTYPE_INFOBOLD,
    FONTTYPE_EJECUCION,
    FONTTYPE_PARTY,
    FONTTYPE_VENENO,
    FONTTYPE_GUILD,
    FONTTYPE_SERVER,
    FONTTYPE_GUILDMSG,
    FONTTYPE_CONSEJO,
    FONTTYPE_CONSEJOCAOS,
    FONTTYPE_CONSEJOVesA,
    FONTTYPE_CONSEJOCAOSVesA,
    FONTTYPE_CENTINELA,
    FONTTYPE_GMMSG,
    FONTTYPE_GM,
    FONTTYPE_CITIZEN,
    FONTTYPE_CONSE,
    FONTTYPE_DIOS
};

/**
 * @brief Opciones de edición administrativa de personajes (Protocol.bas:315-331).
 * En VB6 eo_Gold inicia explícitamente en 1.
 */
enum class eEditOptions : std::uint8_t {
    eo_Gold = 1,
    eo_Experience,
    eo_Body,
    eo_Head,
    eo_CiticensKilled,
    eo_CriminalsKilled,
    eo_Level,
    eo_Class,
    eo_Skills,
    eo_SkillPointsLeft,
    eo_Nobleza,
    eo_Asesino,
    eo_Sex,
    eo_Raza,
    eo_addGold
};

/**
 * @brief Códigos de resultado del despachador binario de paquetes.
 */
enum class PacketParseResult : std::uint8_t {
    Ok,                 // Paquete decodificado y procesado con éxito
    NeedMoreData,       // Faltan bytes en el búfer (fragmentación TCP)
    InvalidSession,     // Paquete ilegal para el estado de login actual
    MalformedData,      // Carga corrupta o que viola límites del protocolo
    UnknownPacket       // Opcode no registrado o fuera de rango
};

// ============================================================================
// Paso 2: Generadores Multicast (PrepareMessage...)
// ============================================================================

std::vector<std::uint8_t> PrepareMessageSetInvisible(std::int16_t CharIndex, bool invisible);
std::vector<std::uint8_t> PrepareMessageCharacterChangeNick(std::int16_t CharIndex, std::string_view newNick);
std::vector<std::uint8_t> PrepareMessageChatOverHead(std::string_view Chat, std::int16_t CharIndex, std::int32_t color);
std::vector<std::uint8_t> PrepareMessageConsoleMsg(std::string_view Chat, FontTypeNames FontIndex);
std::vector<std::uint8_t> PrepareMessageCreateFX(std::int16_t CharIndex, std::int16_t FX, std::int16_t FXLoops);
std::vector<std::uint8_t> PrepareMessagePlayWave(std::uint8_t wave, std::uint8_t X, std::uint8_t Y);
std::vector<std::uint8_t> PrepareMessageGuildChat(std::string_view Chat);
std::vector<std::uint8_t> PrepareMessageShowMessageBox(std::string_view Chat);
std::vector<std::uint8_t> PrepareMessagePlayMidi(std::uint8_t midi, std::int16_t loops = -1);
std::vector<std::uint8_t> PrepareMessagePauseToggle();
std::vector<std::uint8_t> PrepareMessageRainToggle();
std::vector<std::uint8_t> PrepareMessageObjectDelete(std::uint8_t X, std::uint8_t Y);
std::vector<std::uint8_t> PrepareMessageBlockPosition(std::uint8_t X, std::uint8_t Y, bool Blocked);
std::vector<std::uint8_t> PrepareMessageObjectCreate(std::int16_t GrhIndex, std::uint8_t X, std::uint8_t Y);
std::vector<std::uint8_t> PrepareMessageCharacterRemove(std::int16_t CharIndex);
std::vector<std::uint8_t> PrepareMessageRemoveCharDialog(std::int16_t CharIndex);
std::vector<std::uint8_t> PrepareMessageCharacterCreate(std::int16_t body, std::int16_t Head, eHeading heading,
                                                       std::int16_t CharIndex, std::uint8_t X, std::uint8_t Y,
                                                       std::int16_t weapon, std::int16_t shield, std::int16_t FX,
                                                       std::int16_t FXLoops, std::int16_t helmet, std::string_view name,
                                                       std::uint8_t NickColor, std::uint8_t Privileges);
std::vector<std::uint8_t> PrepareMessageCharacterChange(std::int16_t body, std::int16_t Head, eHeading heading,
                                                       std::int16_t CharIndex, std::int16_t weapon, std::int16_t shield,
                                                       std::int16_t FX, std::int16_t FXLoops, std::int16_t helmet);
std::vector<std::uint8_t> PrepareMessageCharacterMove(std::int16_t CharIndex, std::uint8_t X, std::uint8_t Y);
std::vector<std::uint8_t> PrepareMessageForceCharMove(eHeading Direccion);
std::vector<std::uint8_t> PrepareMessageUpdateTagAndStatus(std::int16_t UserIndex, std::uint8_t NickColor, std::string_view Tag);
std::vector<std::uint8_t> PrepareMessageErrorMsg(std::string_view message);


// ============================================================================
// Paso 3: Primitivas de Salida Unicast (Write...) y FlushBuffer
// ============================================================================

void FlushBuffer(std::int16_t UserIndex);

void WriteMultiMessage(std::int16_t UserIndex, std::int16_t MessageIndex, std::int32_t Arg1 = 0, std::int32_t Arg2 = 0, std::int32_t Arg3 = 0, std::string_view StringArg1 = {});
void WriteLoggedMessage(std::int16_t UserIndex);
void WriteRemoveAllDialogs(std::int16_t UserIndex);
void WriteRemoveCharDialog(std::int16_t UserIndex, std::int16_t CharIndex);
void WriteNavigateToggle(std::int16_t UserIndex);
void WriteDisconnect(std::int16_t UserIndex);
void WriteUserOfferConfirm(std::int16_t UserIndex);
void WriteCommerceEnd(std::int16_t UserIndex);
void WriteBankEnd(std::int16_t UserIndex);
void WriteCommerceInit(std::int16_t UserIndex);
void WriteBankInit(std::int16_t UserIndex);
void WriteUserCommerceInit(std::int16_t UserIndex);
void WriteUserCommerceEnd(std::int16_t UserIndex);
void WriteShowBlacksmithForm(std::int16_t UserIndex);
void WriteShowCarpenterForm(std::int16_t UserIndex);
void WriteUpdateSta(std::int16_t UserIndex);
void WriteUpdateMana(std::int16_t UserIndex);
void WriteUpdateHP(std::int16_t UserIndex);
void WriteUpdateGold(std::int16_t UserIndex);
void WriteUpdateBankGold(std::int16_t UserIndex);
void WriteUpdateExp(std::int16_t UserIndex);
void WriteUpdateStrenghtAndDexterity(std::int16_t UserIndex);
void WriteUpdateDexterity(std::int16_t UserIndex);
void WriteUpdateStrenght(std::int16_t UserIndex);
void WriteChangeMap(std::int16_t UserIndex, std::int16_t Map, std::int16_t version);
void WritePosUpdate(std::int16_t UserIndex);
void WriteChatOverHead(std::int16_t UserIndex, std::string_view Chat, std::int16_t CharIndex, std::int32_t color);
void WriteConsoleMsg(std::int16_t UserIndex, std::string_view Chat, FontTypeNames FontIndex);
void WriteCommerceChat(std::int16_t UserIndex, std::string_view Chat, FontTypeNames FontIndex);
void WriteGuildChat(std::int16_t UserIndex, std::string_view Chat);
void WriteShowMessageBox(std::int16_t UserIndex, std::string_view message);
void WriteUserIndexInServer(std::int16_t UserIndex);
void WriteUserCharIndexInServer(std::int16_t UserIndex);
void WriteCharacterCreate(std::int16_t UserIndex, std::int16_t body, std::int16_t Head, eHeading heading, std::int16_t CharIndex, std::uint8_t X, std::uint8_t Y, std::int16_t weapon, std::int16_t shield, std::int16_t FX, std::int16_t FXLoops, std::int16_t helmet, std::string_view name, std::uint8_t NickColor, std::uint8_t Privileges);
void WriteCharacterRemove(std::int16_t UserIndex, std::int16_t CharIndex);
void WriteCharacterMove(std::int16_t UserIndex, std::int16_t CharIndex, std::uint8_t X, std::uint8_t Y);
void WriteForceCharMove(std::int16_t UserIndex, eHeading Direccion);
void WriteCharacterChange(std::int16_t UserIndex, std::int16_t body, std::int16_t Head, eHeading heading, std::int16_t CharIndex, std::int16_t weapon, std::int16_t shield, std::int16_t FX, std::int16_t FXLoops, std::int16_t helmet);
void WriteObjectCreate(std::int16_t UserIndex, std::int16_t GrhIndex, std::uint8_t X, std::uint8_t Y);
void WriteObjectDelete(std::int16_t UserIndex, std::uint8_t X, std::uint8_t Y);
void WriteBlockPosition(std::int16_t UserIndex, std::uint8_t X, std::uint8_t Y, bool Blocked);
void WritePlayMidi(std::int16_t UserIndex, std::uint8_t midi, std::int16_t loops = -1);
void WritePlayWave(std::int16_t UserIndex, std::uint8_t wave, std::uint8_t X, std::uint8_t Y);
void WriteGuildList(std::int16_t UserIndex, std::span<const std::string> guildList);
void WriteAreaChanged(std::int16_t UserIndex);
void WritePauseToggle(std::int16_t UserIndex);
void WriteRainToggle(std::int16_t UserIndex);
void WriteCreateFX(std::int16_t UserIndex, std::int16_t CharIndex, std::int16_t FX, std::int16_t FXLoops);
void WriteUpdateUserStats(std::int16_t UserIndex);
void WriteWorkRequestTarget(std::int16_t UserIndex, eSkill Skill);
void WriteChangeInventorySlot(std::int16_t UserIndex, std::uint8_t Slot);
void WriteAddSlots(std::int16_t UserIndex, eMochilas Mochila);
void WriteChangeBankSlot(std::int16_t UserIndex, std::uint8_t Slot);
void WriteChangeSpellSlot(std::int16_t UserIndex, std::uint8_t Slot);
void WriteAttributes(std::int16_t UserIndex);
void WriteBlacksmithWeapons(std::int16_t UserIndex);
void WriteBlacksmithArmors(std::int16_t UserIndex);
void WriteCarpenterObjects(std::int16_t UserIndex);
void WriteRestOK(std::int16_t UserIndex);
void WriteErrorMsg(std::int16_t UserIndex, std::string_view message);
void WriteBlind(std::int16_t UserIndex);
void WriteDumb(std::int16_t UserIndex);
void WriteShowSignal(std::int16_t UserIndex, std::int16_t ObjIndex);
void WriteChangeNPCInventorySlot(std::int16_t UserIndex, std::uint8_t Slot, const Obj& Obj, float price);
void WriteUpdateHungerAndThirst(std::int16_t UserIndex);
void WriteFame(std::int16_t UserIndex);
void WriteMiniStats(std::int16_t UserIndex);
void WriteLevelUp(std::int16_t UserIndex, std::int16_t skillPoints);
void WriteAddForumMsg(std::int16_t UserIndex, eForumType ForumType, std::string_view Title, std::string_view Author, std::string_view message);
void WriteShowForumForm(std::int16_t UserIndex);
void WriteSetInvisible(std::int16_t UserIndex, std::int16_t CharIndex, bool invisible);
void WriteDiceRoll(std::int16_t UserIndex);
void WriteMeditateToggle(std::int16_t UserIndex);
void WriteBlindNoMore(std::int16_t UserIndex);
void WriteDumbNoMore(std::int16_t UserIndex);
void WriteSendSkills(std::int16_t UserIndex);
void WriteTrainerCreatureList(std::int16_t UserIndex, std::int16_t NpcIndex);
void WriteGuildNews(std::int16_t UserIndex, std::string_view guildNews, std::span<const std::string> enemies, std::span<const std::string> allies);
void WriteOfferDetails(std::int16_t UserIndex, std::string_view details);
void WriteAlianceProposalsList(std::int16_t UserIndex, std::span<const std::string> guilds);
void WritePeaceProposalsList(std::int16_t UserIndex, std::span<const std::string> guilds);
void WriteCharacterInfo(std::int16_t UserIndex, std::string_view charName, eRaza race, eClass Class, eGenero gender, std::uint8_t level, std::int32_t gold, std::int32_t bank, std::int32_t reputation, std::string_view previousPetitions, std::string_view currentGuild, std::string_view previousGuilds, bool RoyalArmy, bool CaosLegion, std::int32_t citicensKilled, std::int32_t criminalsKilled);
void WriteGuildLeaderInfo(std::int16_t UserIndex, std::span<const std::string> guildList, std::span<const std::string> MemberList, std::string_view guildNews, std::span<const std::string> joinRequests);
void WriteGuildMemberInfo(std::int16_t UserIndex, std::span<const std::string> guildList, std::span<const std::string> MemberList);
void WriteGuildDetails(std::int16_t UserIndex, std::string_view GuildName, std::string_view founder, std::string_view foundationDate, std::string_view leader, std::string_view URL, std::int16_t memberCount, bool electionsOpen, std::string_view alignment, std::int16_t enemiesCount, std::int16_t AlliesCount, std::string_view antifactionPoints, std::span<const std::string> codex, std::string_view guildDesc);
void WriteShowGuildAlign(std::int16_t UserIndex);
void WriteShowGuildFundationForm(std::int16_t UserIndex);
void WriteParalizeOK(std::int16_t UserIndex);
void WriteShowUserRequest(std::int16_t UserIndex, std::string_view details);
void WriteTradeOK(std::int16_t UserIndex);
void WriteBankOK(std::int16_t UserIndex);
void WriteChangeUserTradeSlot(std::int16_t UserIndex, std::uint8_t OfferSlot, std::int16_t ObjIndex, std::int32_t Amount);
void WriteSendNight(std::int16_t UserIndex, bool night);
void WriteSpawnList(std::int16_t UserIndex, std::span<const std::string> npcNames);
void WriteShowSOSForm(std::int16_t UserIndex);
void WriteShowPartyForm(std::int16_t UserIndex);
void WriteShowMOTDEditionForm(std::int16_t UserIndex, std::string_view currentMOTD);
void WriteShowGMPanelForm(std::int16_t UserIndex);
void WriteUserNameList(std::int16_t UserIndex, std::span<const std::string> userNamesList, std::int16_t cant);
void WritePong(std::int16_t UserIndex);
void WriteStopWorking(std::int16_t UserIndex);
void WriteCancelOfferItem(std::int16_t UserIndex, std::uint8_t Slot);

// ============================================================================
// Paso 4: Despachador monolítico y bucle de recepción
// ============================================================================

// Despacha un único paquete inspeccionado desde la cola incomingData del usuario
PacketParseResult DispatchPacket(std::int16_t UserIndex, ClientPacketID opcode);

// Bucle iterativo central de recepción, desfragmentación y despacho
void HandleIncomingData(std::int16_t UserIndex);

} // namespace ao::net::protocol

#endif // PROTOCOL_HPP
