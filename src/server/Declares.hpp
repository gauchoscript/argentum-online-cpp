#pragma once

#include <cstdint>
#include <string>
#include <array>
#include <vector>
#include <memory>
#include <chrono>
#include <cstring>

#if defined(_WIN32)
#include <windows.h>
#endif

#include "Matematicas.hpp"
#include "cGarbage.hpp"

// Declaraciones de clases de módulos de clase aun no porteados
class clsByteQueue {};
class clsAntiDoS {};
// clsAntiMassClon excluido por tratarse de código muerto (ver docs/audit/02b-antimassclon-detalle.md)
class cCola {};
class clsParty {};
class ConsultasPopulares {};
class SoundMapInfo {};

// Tipos auxiliares de módulos aun no porteados (AI_NPC, ModAreas, modGuilds, Queue, mdlCOmercioConUsuario)
enum class TipoAI : std::int32_t {
    StaticNPC = 0,
    NpcPathFinding = 1,
    NPCMAmbula = 2
};

enum class ALINEACION_GUILD : std::int32_t {
    ALINEACION_NINGUNA = 0,
    ALINEACION_LEGION = 1,
    ALINEACION_CRIMINAL = 2,
    ALINEACION_NEUTRO = 3,
    ALINEACION_CIUDA = 4,
    ALINEACION_ARMADA = 5,
    ALINEACION_MASTER = 6
};

struct AreaInfo {
    std::uint8_t AreaRecibida[9]{};
    std::uint8_t AreaReenviada[9]{};
};

struct tCOmercioUsuario {
    std::int32_t DestUsu{0};
    std::int32_t DestPos{0};
    std::int32_t SenderPos{0};
    std::uint8_t Estado{0};
};

struct tVertice {
    std::int16_t X{0};
    std::int16_t Y{0};
};

struct SecurityData {
    std::int32_t Dummy{0};
};

// ==========================================
// CONSTANTES GLOBALES
// ==========================================

constexpr std::int32_t MAXSPAWNATTEMPS = 60;
constexpr std::int16_t INFINITE_LOOPS = -1;
constexpr std::int32_t FXSANGRE = 14;

constexpr std::int32_t CHAT_COLOR_DEAD_CHAR = 0xC0C0C0;
constexpr std::int32_t CHAT_COLOR_GM_YELL = 0xF82FF;

constexpr std::uint8_t NO_3D_SOUND = 0;

constexpr std::int32_t iFragataFantasmal = 87;
constexpr std::int32_t iFragataReal = 190;
constexpr std::int32_t iFragataCaos = 189;
constexpr std::int32_t iBarca = 84;
constexpr std::int32_t iGalera = 85;
constexpr std::int32_t iGaleon = 86;
constexpr std::int32_t iBarcaCiuda = 395;
constexpr std::int32_t iBarcaPk = 396;
constexpr std::int32_t iGaleraCiuda = 397;
constexpr std::int32_t iGaleraPk = 398;
constexpr std::int32_t iGaleonCiuda = 399;
constexpr std::int32_t iGaleonPk = 400;

constexpr std::uint8_t LimiteNewbie = 12;

constexpr std::int16_t TIEMPO_INICIOMEDITAR = 2000;

constexpr std::int16_t NingunEscudo = 2;
constexpr std::int16_t NingunCasco = 2;
constexpr std::int16_t NingunArma = 2;

constexpr std::int16_t EspadaMataDragonesIndex = 402;
constexpr std::int16_t LAUDMAGICO = 696;
constexpr std::int16_t FLAUTAMAGICA = 208;

constexpr std::int16_t LAUDELFICO = 1049;
constexpr std::int16_t FLAUTAELFICA = 1050;

constexpr std::int16_t APOCALIPSIS_SPELL_INDEX = 25;
constexpr std::int16_t DESCARGA_SPELL_INDEX = 23;

constexpr std::uint8_t SLOTS_POR_FILA = 5;

constexpr std::uint8_t PROB_ACUCHILLAR = 20;
constexpr float DAÑO_ACUCHILLAR = 0.2f;

constexpr std::uint8_t MAXMASCOTASENTRENADOR = 7;
constexpr std::int32_t TIEMPO_CARCEL_PIQUETE = 10;

const std::string Bosque = "BOSQUE";
const std::string Nieve = "NIEVE";
const std::string Desierto = "DESIERTO";
const std::string Ciudad = "CIUDAD";
const std::string Campo = "CAMPO";
const std::string Dungeon = "DUNGEON";

constexpr std::uint8_t MAXUSERHECHIZOS = 35;

constexpr std::uint8_t EsfuerzoTalarGeneral = 4;
constexpr std::uint8_t EsfuerzoTalarLeñador = 2;

constexpr std::uint8_t EsfuerzoPescarPescador = 1;
constexpr std::uint8_t EsfuerzoPescarGeneral = 3;

constexpr std::uint8_t EsfuerzoExcavarMinero = 2;
constexpr std::uint8_t EsfuerzoExcavarGeneral = 5;

constexpr std::int16_t FX_TELEPORT_INDEX = 1;

constexpr float PORCENTAJE_MATERIALES_UPGRADE = 0.85f;

constexpr std::int16_t Guardias = 6;

constexpr std::int32_t MAX_ORO_EDIT = 5000000;

const std::string STANDARD_BOUNTY_HUNTER_MESSAGE = "Se te ha otorgado un premio por ayudar al proyecto reportando bugs, el mismo está disponible en tu bóveda.";
const std::string TAG_USER_INVISIBLE = "[INVISIBLE]";
const std::string TAG_CONSULT_MODE = "[CONSULTA]";

constexpr std::int32_t MAXREP = 6000000;
constexpr std::int32_t MAXORO = 90000000;
constexpr std::int32_t MAXEXP = 99999999;

constexpr std::int32_t MAXUSERMATADOS = 65000;

constexpr std::uint8_t MAXATRIBUTOS = 40;
constexpr std::uint8_t MINATRIBUTOS = 6;

constexpr std::int16_t LingoteHierro = 386;
constexpr std::int16_t LingotePlata = 387;
constexpr std::int16_t LingoteOro = 388;
constexpr std::int16_t Leña = 58;
constexpr std::int16_t LeñaElfica = 1006;

constexpr std::int16_t MAXNPCS = 10000;
constexpr std::int16_t MAXCHARS = 10000;

constexpr std::int16_t HACHA_LEÑADOR = 127;
constexpr std::int16_t HACHA_LEÑA_ELFICA = 1005;
constexpr std::int16_t PIQUETE_MINERO = 187;

constexpr std::int16_t DAGA = 15;

constexpr std::int16_t FOGATA_APAG = 136;
constexpr std::int16_t FOGATA = 63;
constexpr std::int16_t ORO_MINA = 194;
constexpr std::int16_t PLATA_MINA = 193;
constexpr std::int16_t HIERRO_MINA = 192;
constexpr std::int16_t MARTILLO_HERRERO = 389;
constexpr std::int16_t SERRUCHO_CARPINTERO = 198;
constexpr std::int16_t ObjArboles = 4;
constexpr std::int16_t RED_PESCA = 543;
constexpr std::int16_t CAÑA_PESCA = 138;

constexpr std::uint8_t MIN_APUÑALAR = 10;

constexpr std::uint8_t NUMSKILLS = 20;
constexpr std::uint8_t NUMATRIBUTOS = 5;
constexpr std::uint8_t NUMCLASES = 12;
constexpr std::uint8_t NUMRAZAS = 5;

constexpr std::uint8_t MAXSKILLPOINTS = 100;
constexpr std::uint8_t NUMCIUDADES = 5;
constexpr std::uint8_t NUM_RANGOS_FACCION = 15;

enum class eTipoDefArmors : std::uint8_t {
    ieBaja = 1,
    ieMedia = 2,
    ieAlta = 3
};

struct tFaccionArmaduras {
    std::array<std::int16_t, 4> Armada{};
    std::array<std::int16_t, 4> Caos{};
};

struct tAPuestas {
    std::int32_t Ganancias{0};
    std::int32_t Perdidas{0};
    std::int32_t Jugadas{0};
};

constexpr std::uint8_t MAXMASCOTAS = 3;

constexpr std::int16_t vlASALTO = 100;
constexpr std::int16_t vlASESINO = 1000;
constexpr std::int16_t vlCAZADOR = 5;
constexpr std::int16_t vlNoble = 5;
constexpr std::int16_t vlLadron = 25;
constexpr std::int16_t vlProleta = 2;

constexpr std::int16_t iCuerpoMuerto = 8;
constexpr std::int16_t iCabezaMuerto = 500;

constexpr std::uint8_t iORO = 12;
constexpr std::uint8_t Pescado = 139;

constexpr std::uint8_t FundirMetal = 88;

constexpr std::uint8_t AdicionalHPGuerrero = 2;
constexpr std::uint8_t AdicionalHPCazador = 1;

constexpr std::uint8_t AumentoSTDef = 15;
constexpr std::uint8_t AumentoStBandido = AumentoSTDef + 23;
constexpr std::uint8_t AumentoSTLadron = AumentoSTDef + 3;
constexpr std::uint8_t AumentoSTMago = AumentoSTDef - 1;
constexpr std::uint8_t AumentoSTTrabajador = AumentoSTDef + 25;

constexpr std::uint8_t XMaxMapSize = 100;
constexpr std::uint8_t XMinMapSize = 1;
constexpr std::uint8_t YMaxMapSize = 100;
constexpr std::uint8_t YMinMapSize = 1;

constexpr std::uint8_t TileSizeX = 32;
constexpr std::uint8_t TileSizeY = 32;

constexpr std::uint8_t XWindow = 17;
constexpr std::uint8_t YWindow = 13;

constexpr std::uint8_t SND_SWING = 2;
constexpr std::uint8_t SND_TALAR = 13;
constexpr std::uint8_t SND_PESCAR = 14;
constexpr std::uint8_t SND_MINERO = 15;
constexpr std::uint8_t SND_WARP = 3;
constexpr std::uint8_t SND_PUERTA = 5;
constexpr std::uint8_t SND_NIVEL = 6;

constexpr std::uint8_t SND_USERMUERTE = 11;
constexpr std::uint8_t SND_IMPACTO = 10;
constexpr std::uint8_t SND_IMPACTO2 = 12;
constexpr std::uint8_t SND_LEÑADOR = 13;
constexpr std::uint8_t SND_FOGATA = 14;
constexpr std::uint8_t SND_AVE = 21;
constexpr std::uint8_t SND_AVE2 = 22;
constexpr std::uint8_t SND_AVE3 = 34;
constexpr std::uint8_t SND_GRILLO = 28;
constexpr std::uint8_t SND_GRILLO2 = 29;
constexpr std::uint8_t SND_SACARARMA = 25;
constexpr std::uint8_t SND_ESCUDO = 37;
constexpr std::uint8_t MARTILLOHERRERO = 41;
constexpr std::uint8_t LABUROCARPINTERO = 42;
constexpr std::uint8_t SND_BEBER = 46;

constexpr std::int16_t MAX_INVENTORY_OBJS = 10000;
constexpr std::uint8_t MAX_INVENTORY_SLOTS = 30;
constexpr std::uint8_t MAX_NORMAL_INVENTORY_SLOTS = 20;
constexpr std::int16_t FLAGORO = MAX_INVENTORY_SLOTS + 1;

const std::string FONTTYPE_TALK = "~255~255~255~0~0";
const std::string FONTTYPE_FIGHT = "~255~0~0~1~0";
const std::string FONTTYPE_WARNING = "~32~51~223~1~1";
const std::string FONTTYPE_INFO = "~65~190~156~0~0";
const std::string FONTTYPE_INFOBOLD = "~65~190~156~1~0";
const std::string FONTTYPE_EJECUCION = "~130~130~130~1~0";
const std::string FONTTYPE_PARTY = "~255~180~255~0~0";
const std::string FONTTYPE_VENENO = "~0~255~0~0~0";
const std::string FONTTYPE_GUILD = "~255~255~255~1~0";
const std::string FONTTYPE_SERVER = "~0~185~0~0~0";
const std::string FONTTYPE_GUILDMSG = "~228~199~27~0~0";
const std::string FONTTYPE_CONSEJO = "~130~130~255~1~0";
const std::string FONTTYPE_CONSEJOCAOS = "~255~60~00~1~0";
const std::string FONTTYPE_CONSEJOVesA = "~0~200~255~1~0";
const std::string FONTTYPE_CONSEJOCAOSVesA = "~255~50~0~1~0";
const std::string FONTTYPE_CENTINELA = "~0~255~0~1~0";

constexpr std::uint8_t STAT_MAXELV = 255;
constexpr std::int16_t STAT_MAXHP = 999;
constexpr std::int16_t STAT_MAXSTA = 999;
constexpr std::int16_t STAT_MAXMAN = 9999;
constexpr std::uint8_t STAT_MAXHIT_UNDER36 = 99;
constexpr std::int16_t STAT_MAXHIT_OVER36 = 999;
constexpr std::uint8_t STAT_MAXDEF = 99;

constexpr std::uint8_t ELU_SKILL_INICIAL = 200;
constexpr std::uint8_t EXP_ACIERTO_SKILL = 50;
constexpr std::uint8_t EXP_FALLO_SKILL = 20;

constexpr std::uint8_t MAX_BANCOINVENTORY_SLOTS = 40;
constexpr std::uint8_t MAX_STICKY_POST = 10;
constexpr std::uint8_t MAX_GENERAL_POST = 35;
constexpr std::uint8_t MAX_NPC_DROPS = 5;
constexpr std::int16_t MAX_PARTIES = 300;

// ==========================================
// ENUMERACIONES (C++ enum standard)
// ==========================================

enum iMinerales : std::int32_t {
    HierroCrudo = 192,
    PlataCruda = 193,
    OroCrudo = 194,
    LingoteDeHierro = 386,
    LingoteDePlata = 387,
    LingoteDeOro = 388
};

enum PlayerType : std::int32_t {
    UserPlayer = 0x1,
    Consejero = 0x2,
    SemiDios = 0x4,
    Dios = 0x8,
    Admin = 0x10,
    RoleMaster = 0x20,
    ChaosCouncil = 0x40,
    RoyalCouncil = 0x80
};

enum eClass : std::int32_t {
    Mage = 1,
    Cleric,
    Warrior,
    Assasin,
    Thief,
    Bard,
    Druid,
    Bandit,
    Paladin,
    Hunter,
    Worker,
    Pirat
};

enum eCiudad : std::int32_t {
    cUllathorpe = 1,
    cNix,
    cBanderbill,
    cLindos,
    cArghal
};

enum eRaza : std::int32_t {
    Humano = 1,
    Elfo,
    Drow,
    Gnomo,
    Enano
};

enum eGenero : std::int32_t {
    Hombre = 1,
    Mujer
};

enum eClanType : std::int32_t {
    ct_RoyalArmy,
    ct_Evil,
    ct_Neutral,
    ct_GM,
    ct_Legal,
    ct_Criminal
};

enum FXIDs : std::int32_t {
    FXWARP = 1,
    FXMEDITARCHICO = 4,
    FXMEDITARMEDIANO = 5,
    FXMEDITARGRANDE = 6,
    FXMEDITARXGRANDE = 16,
    FXMEDITARXXGRANDE = 34
};

enum eTrigger : std::int32_t {
    NADA = 0,
    BAJOTECHO = 1,
    trigger_2 = 2,
    POSINVALIDA = 3,
    ZONASEGURA = 4,
    ANTIPIQUETE = 5,
    ZONAPELEA = 6
};

enum eTrigger6 : std::int32_t {
    TRIGGER6_PERMITE = 1,
    TRIGGER6_PROHIBE = 2,
    TRIGGER6_AUSENTE = 3
};

enum TargetType : std::int32_t {
    uUsuarios = 1,
    uNPC = 2,
    uUsuariosYnpc = 3,
    uTerreno = 4
};

enum TipoHechizo : std::int32_t {
    uPropiedades = 1,
    uEstado = 2,
    uMaterializa = 3,
    uInvocacion = 4
};

enum PartesCuerpo : std::int32_t {
    bCabeza = 1,
    bPiernaIzquierda = 2,
    bPiernaDerecha = 3,
    bBrazoDerecho = 4,
    bBrazoIzquierdo = 5,
    bTorso = 6
};

enum eNPCType : std::int32_t {
    Comun = 0,
    Revividor = 1,
    GuardiaReal = 2,
    Entrenador = 3,
    Banquero = 4,
    Noble = 5,
    DRAGON = 6,
    Timbero = 7,
    Guardiascaos = 8,
    ResucitadorNewbie = 9,
    Pretoriano = 10,
    Gobernador = 11
};

enum eHeading : std::int32_t {
    NORTH = 1,
    EAST = 2,
    SOUTH = 3,
    WEST = 4
};

enum PECES_POSIBLES : std::int32_t {
    PESCADO1 = 139,
    PESCADO2 = 544,
    PESCADO3 = 545,
    PESCADO4 = 546
};

enum eSkill : std::int32_t {
    Magia = 1,
    Robar = 2,
    Tacticas = 3,
    Armas = 4,
    Meditar = 5,
    Apuñalar = 6,
    Ocultarse = 7,
    Supervivencia = 8,
    Talar = 9,
    Comerciar = 10,
    Defensa = 11,
    Pesca = 12,
    Mineria = 13,
    Carpinteria = 14,
    Herreria = 15,
    Liderazgo = 16,
    Domar = 17,
    Proyectiles = 18,
    Wrestling = 19,
    Navegacion = 20
};

enum eMochilas : std::int32_t {
    Mediana = 1,
    Grande = 2
};

enum eAtributos : std::int32_t {
    Fuerza = 1,
    Agilidad = 2,
    Inteligencia = 3,
    Carisma = 4,
    Constitucion = 5
};

enum eOBJType : std::int32_t {
    otUseOnce = 1,
    otWeapon = 2,
    otArmadura = 3,
    otArboles = 4,
    otGuita = 5,
    otPuertas = 6,
    otContenedores = 7,
    otCarteles = 8,
    otLlaves = 9,
    otForos = 10,
    otPociones = 11,
    otBebidas = 13,
    otLeña = 14,
    otFogata = 15,
    otESCUDO = 16,
    otCASCO = 17,
    otAnillo = 18,
    otTeleport = 19,
    otYacimiento = 22,
    otMinerales = 23,
    otPergaminos = 24,
    otInstrumentos = 26,
    otYunque = 27,
    otFragua = 28,
    otBarcos = 31,
    otFlechas = 32,
    otBotellaVacia = 33,
    otBotellaLlena = 34,
    otManchas = 35,
    otArbolElfico = 36,
    otMochilas = 37,
    otCualquiera = 1000
};

enum eNickColor : std::int32_t {
    ieCriminal = 0x1,
    ieCiudadano = 0x2,
    ieAtacable = 0x4,
    ieCAOS,
    ieCAOS_STICKY
};

enum eForumVisibility : std::int32_t {
    ieGENERAL_MEMBER = 0x1,
    ieREAL_MEMBER = 0x2,
    ieCAOS_MEMBER = 0x4
};

enum eForumType : std::int32_t {
    ieGeneral,
    ieREAL,
    ieForumCAOS
};

enum e_ObjetosCriticos : std::int32_t {
    Manzana = 1,
    Manzana2 = 2,
    ManzanaNewbie = 467
};

enum eMessages : std::int32_t {
    DontSeeAnything,
    NPCSwing,
    NPCKillUser,
    BlockedWithShieldUser,
    BlockedWithShieldother,
    UserSwing,
    SafeModeOn,
    SafeModeOff,
    ResuscitationSafeOff,
    ResuscitationSafeOn,
    NobilityLost
};

// ==========================================
// ESTRUCTURAS (UDTs)
// ==========================================

struct tCabecera {
    char desc[255]{};
    std::int32_t crc{0};
    std::int32_t MagicWord{0};
};

struct tHechizo {
    std::string Nombre;
    std::string desc;
    std::string PalabrasMagicas;
    
    std::string HechizeroMsg;
    std::string TargetMsg;
    std::string PropioMsg;
    
    TipoHechizo Tipo{TipoHechizo::uPropiedades};
    
    std::int16_t WAV{0};
    std::int16_t FXgrh{0};
    std::uint8_t loops{0};
    
    std::uint8_t SubeHP{0};
    std::int16_t MinHp{0};
    std::int16_t MaxHp{0};
    
    std::uint8_t SubeMana{0};
    std::int16_t MiMana{0};
    std::int16_t MaMana{0};
    
    std::uint8_t SubeSta{0};
    std::int16_t MinSta{0};
    std::int16_t MaxSta{0};
    
    std::uint8_t SubeHam{0};
    std::int16_t MinHam{0};
    std::int16_t MaxHam{0};
    
    std::uint8_t SubeSed{0};
    std::int16_t MinSed{0};
    std::int16_t MaxSed{0};
    
    std::uint8_t SubeAgilidad{0};
    std::int16_t MinAgilidad{0};
    std::int16_t MaxAgilidad{0};
    
    std::uint8_t SubeFuerza{0};
    std::int16_t MinFuerza{0};
    std::int16_t MaxFuerza{0};
    
    std::uint8_t SubeCarisma{0};
    std::int16_t MinCarisma{0};
    std::int16_t MaxCarisma{0};
    
    std::uint8_t Invisibilidad{0};
    std::uint8_t Paraliza{0};
    std::uint8_t Inmoviliza{0};
    std::uint8_t RemoverParalisis{0};
    std::uint8_t RemoverEstupidez{0};
    std::uint8_t CuraVeneno{0};
    std::uint8_t Envenena{0};
    std::uint8_t Maldicion{0};
    std::uint8_t RemoverMaldicion{0};
    std::uint8_t Bendicion{0};
    std::uint8_t Estupidez{0};
    std::uint8_t Ceguera{0};
    std::uint8_t Revivir{0};
    std::uint8_t Morph{0};
    std::uint8_t Mimetiza{0};
    std::uint8_t RemueveInvisibilidadParcial{0};
    
    std::uint8_t Warp{0};
    std::uint8_t Invoca{0};
    std::int16_t NumNpc{0};
    std::int16_t cant{0};
    
    std::int16_t MinSkill{0};
    std::int16_t ManaRequerido{0};
    std::int16_t StaRequerido{0};
    
    TargetType Target{TargetType::uUsuarios};
    
    std::int16_t NeedStaff{0};
    bool StaffAffected{false};
};

struct LevelSkill {
    std::int16_t LevelValue{0};
};

struct UserOBJ {
    std::int16_t ObjIndex{0};
    std::int16_t Amount{0};
    std::uint8_t Equipped{0};
};

struct Inventario {
    UserOBJ Object[MAX_INVENTORY_SLOTS + 1]{};
    std::int16_t WeaponEqpObjIndex{0};
    std::uint8_t WeaponEqpSlot{0};
    std::int16_t ArmourEqpObjIndex{0};
    std::uint8_t ArmourEqpSlot{0};
    std::int16_t EscudoEqpObjIndex{0};
    std::uint8_t EscudoEqpSlot{0};
    std::int16_t CascoEqpObjIndex{0};
    std::uint8_t CascoEqpSlot{0};
    std::int16_t MunicionEqpObjIndex{0};
    std::uint8_t MunicionEqpSlot{0};
    std::int16_t AnilloEqpObjIndex{0};
    std::uint8_t AnilloEqpSlot{0};
    std::int16_t BarcoObjIndex{0};
    std::uint8_t BarcoSlot{0};
    std::int16_t MochilaEqpObjIndex{0};
    std::uint8_t MochilaEqpSlot{0};
    std::int16_t NroItems{0};
};

struct tPartyData {
    std::int16_t PIndex{0};
    double RemXP{0.0};
    std::int16_t TargetUser{0};
};

struct Position {
    std::int16_t X{0};
    std::int16_t Y{0};
};

struct FXdata {
    std::string Nombre;
    std::int16_t GrhIndex{0};
    std::int16_t Delay{0};
};

struct Char {
    std::int16_t CharIndex{0};
    std::int16_t Head{0};
    std::int16_t body{0};
    
    std::int16_t WeaponAnim{0};
    std::int16_t ShieldAnim{0};
    std::int16_t CascoAnim{0};
    
    std::int16_t FX{0};
    std::int16_t loops{0};
    
    eHeading heading{eHeading::NORTH};
};

struct ObjData {
    std::string name;
    eOBJType OBJType{eOBJType::otUseOnce};
    
    std::int16_t GrhIndex{0};
    std::int16_t GrhSecundario{0};
    
    std::int16_t MaxItems{0};
    Inventario Conte{};
    std::uint8_t Apuñala{0};
    std::uint8_t Acuchilla{0};
    
    std::int16_t HechizoIndex{0};
    std::string ForoID;
    
    std::int16_t MinHp{0};
    std::int16_t MaxHp{0};
    
    std::int16_t MineralIndex{0};
    std::int16_t LingoteInex{0};
    
    std::int16_t proyectil{0};
    std::int16_t Municion{0};
    
    std::uint8_t Crucial{0};
    std::int16_t Newbie{0};
    
    std::int16_t MinSta{0};
    std::uint8_t TipoPocion{0};
    std::int16_t MaxModificador{0};
    std::int16_t MinModificador{0};
    std::int32_t DuracionEfecto{0};
    std::int16_t MinSkill{0};
    std::int16_t LingoteIndex{0};
    
    std::int16_t MinHIT{0};
    std::int16_t MaxHIT{0};
    std::int16_t MinHam{0};
    std::int16_t MinSed{0};
    
    std::int16_t def{0};
    std::int16_t MinDef{0};
    std::int16_t MaxDef{0};
    
    std::int16_t Ropaje{0};
    std::int16_t WeaponAnim{0};
    std::int16_t WeaponRazaEnanaAnim{0};
    std::int16_t ShieldAnim{0};
    std::int16_t CascoAnim{0};
    
    std::int32_t Valor{0};
    std::int16_t Cerrada{0};
    std::uint8_t Llave{0};
    std::int32_t clave{0};
    
    std::int16_t Radio{0};
    std::uint8_t MochilaType{0};
    std::uint8_t Guante{0};
    
    std::int16_t IndexAbierta{0};
    std::int16_t IndexCerrada{0};
    std::int16_t IndexCerradaLlave{0};
    
    std::uint8_t RazaEnana{0};
    std::uint8_t RazaDrow{0};
    std::uint8_t RazaElfa{0};
    std::uint8_t RazaGnoma{0};
    std::uint8_t RazaHumana{0};
    
    std::uint8_t Mujer{0};
    std::uint8_t Hombre{0};
    
    std::uint8_t Envenena{0};
    std::uint8_t Paraliza{0};
    std::uint8_t Agarrable{0};
    
    std::int16_t LingH{0};
    std::int16_t LingO{0};
    std::int16_t LingP{0};
    std::int16_t Madera{0};
    std::int16_t MaderaElfica{0};
    
    std::int16_t SkHerreria{0};
    std::int16_t SkCarpinteria{0};
    
    std::string texto;
    eClass ClaseProhibida[NUMCLASES + 1]{};
    
    std::int16_t Snd1{0};
    std::int16_t Snd2{0};
    std::int16_t Snd3{0};
    
    std::int16_t Real{0};
    std::int16_t Caos{0};
    
    std::int16_t NoSeCae{0};
    std::int16_t StaffPower{0};
    std::int16_t StaffDamageBonus{0};
    std::int16_t DefensaMagicaMax{0};
    std::int16_t DefensaMagicaMin{0};
    std::uint8_t Refuerzo{0};
    
    std::uint8_t Log{0};
    std::uint8_t NoLog{0};
    std::int16_t Upgrade{0};
};

struct Obj {
    std::int16_t ObjIndex{0};
    std::int16_t Amount{0};
};

struct ModClase {
    double Evasion{0.0};
    double AtaqueArmas{0.0};
    double AtaqueProyectiles{0.0};
    double AtaqueWrestling{0.0};
    double DañoArmas{0.0};
    double DañoProyectiles{0.0};
    double DañoWrestling{0.0};
    double Escudo{0.0};
};

struct ModRaza {
    float Fuerza{0.0f};
    float Agilidad{0.0f};
    float Inteligencia{0.0f};
    float Carisma{0.0f};
    float Constitucion{0.0f};
};

struct BancoInventario {
    UserOBJ Object[MAX_BANCOINVENTORY_SLOTS + 1]{};
    std::int16_t NroItems{0};
};

struct tForo {
    std::string StickyTitle[MAX_STICKY_POST + 1];
    std::string StickyPost[MAX_STICKY_POST + 1];
    std::string GeneralTitle[MAX_GENERAL_POST + 1];
    std::string GeneralPost[MAX_GENERAL_POST + 1];
};

struct tReputacion {
    std::int32_t NobleRep{0};
    std::int32_t BurguesRep{0};
    std::int32_t PlebeRep{0};
    std::int32_t LadronesRep{0};
    std::int32_t BandidoRep{0};
    std::int32_t AsesinoRep{0};
    std::int32_t Promedio{0};
};

struct UserStats {
    std::int32_t GLD{0};
    std::int32_t Banco{0};
    
    std::int16_t MaxHp{0};
    std::int16_t MinHp{0};
    
    std::int16_t MaxSta{0};
    std::int16_t MinSta{0};
    std::int16_t MaxMAN{0};
    std::int16_t MinMAN{0};
    std::int16_t MaxHIT{0};
    std::int16_t MinHIT{0};
    
    std::int16_t MaxHam{0};
    std::int16_t MinHam{0};
    
    std::int16_t MaxAGU{0};
    std::int16_t MinAGU{0};
    
    std::int16_t def{0};
    double Exp{0.0};
    std::uint8_t ELV{0};
    std::int32_t ELU{0};
    
    std::uint8_t UserSkills[NUMSKILLS + 1]{};
    std::uint8_t UserAtributos[NUMATRIBUTOS + 1]{};
    std::uint8_t UserAtributosBackUP[NUMATRIBUTOS + 1]{};
    std::int16_t UserHechizos[MAXUSERHECHIZOS + 1]{};
    
    std::int32_t UsuariosMatados{0};
    std::int32_t CriminalesMatados{0};
    std::int16_t NPCsMuertos{0};
    
    std::int16_t SkillPts{0};
    std::int32_t ExpSkills[NUMSKILLS + 1]{};
    std::int32_t EluSkills[NUMSKILLS + 1]{};
};

struct UserFlags {
    std::uint8_t Muerto{0};
    std::uint8_t Escondido{0};
    bool Comerciando{false};
    bool UserLogged{false};
    bool Meditando{false};
    std::string Descuento;
    std::uint8_t Hambre{0};
    std::uint8_t Sed{0};
    std::uint8_t PuedeMoverse{0};
    std::int32_t TimerLanzarSpell{0};
    std::uint8_t PuedeTrabajar{0};
    std::uint8_t Envenenado{0};
    std::uint8_t Paralizado{0};
    std::uint8_t Inmovilizado{0};
    std::uint8_t Estupidez{0};
    std::uint8_t Ceguera{0};
    std::uint8_t invisible{0};
    std::uint8_t Maldicion{0};
    std::uint8_t Bendicion{0};
    std::uint8_t Oculto{0};
    std::uint8_t Desnudo{0};
    bool Descansar{false};
    std::int16_t Hechizo{0};
    bool TomoPocion{false};
    std::uint8_t TipoPocion{0};
    
    bool NoPuedeSerAtacado{false};
    std::int16_t AtacablePor{0};
    std::int16_t ShareNpcWith{0};
    
    std::uint8_t Vuela{0};
    std::uint8_t Navegando{0};
    bool Seguro{false};
    bool SeguroResu{false};
    
    std::int32_t DuracionEfecto{0};
    std::int16_t TargetNPC{0};
    eNPCType TargetNpcTipo{eNPCType::Comun};
    std::int16_t OwnedNpc{0};
    std::int16_t NpcInv{0};
    
    std::uint8_t Ban{0};
    std::uint8_t AdministrativeBan{0};
    
    std::int16_t TargetUser{0};
    
    std::int16_t TargetObj{0};
    std::int16_t TargetObjMap{0};
    std::int16_t TargetObjX{0};
    std::int16_t TargetObjY{0};
    
    std::int16_t TargetMap{0};
    std::int16_t TargetX{0};
    std::int16_t TargetY{0};
    
    std::int16_t TargetObjInvIndex{0};
    std::int16_t TargetObjInvSlot{0};
    
    std::int16_t AtacadoPorNpc{0};
    std::int16_t AtacadoPorUser{0};
    std::int16_t NPCAtacado{0};
    bool Ignorado{false};
    
    bool EnConsulta{false};
    std::uint8_t StatsChanged{0};
    PlayerType Privilegios{PlayerType::UserPlayer};
    
    std::int16_t ValCoDe{0};
    std::string LastCrimMatado;
    std::string LastCiudMatado;
    
    std::int16_t OldBody{0};
    std::int16_t OldHead{0};
    std::uint8_t AdminInvisible{0};
    bool AdminPerseguible{false};
    
    std::int32_t ChatColor{0};
    std::string MD5Reportado;
    
    std::int32_t TimesWalk{0};
    std::int32_t StartWalk{0};
    std::int32_t CountSH{0};
    
    std::uint8_t UltimoMensaje{0};
    std::uint8_t Silenciado{0};
    std::uint8_t Mimetizado{0};
    bool CentinelaOK{false};
    
    std::int16_t lastMap{0};
    std::uint8_t Traveling{0};
};

struct UserCounters {
    std::int32_t IdleCount{0};
    std::int16_t AttackCounter{0};
    std::int16_t HPCounter{0};
    std::int16_t STACounter{0};
    std::int16_t Frio{0};
    std::int16_t Lava{0};
    std::int16_t COMCounter{0};
    std::int16_t AGUACounter{0};
    std::int16_t Veneno{0};
    std::int16_t Paralisis{0};
    std::int16_t Ceguera{0};
    std::int16_t Estupidez{0};
    
    std::int16_t Invisibilidad{0};
    std::int16_t TiempoOculto{0};
    std::int16_t Mimetismo{0};
    std::int32_t PiqueteC{0};
    std::int32_t Pena{0};
    WorldPos SendMapCounter{};
    bool Saliendo{false};
    std::int16_t Salir{0};
    
    std::int32_t tInicioMeditar{0};
    bool bPuedeMeditar{false};
    
    std::int32_t TimerLanzarSpell{0};
    std::int32_t TimerPuedeAtacar{0};
    std::int32_t TimerPuedeUsarArco{0};
    std::int32_t TimerPuedeTrabajar{0};
    std::int32_t TimerUsar{0};
    std::int32_t TimerMagiaGolpe{0};
    std::int32_t TimerGolpeMagia{0};
    std::int32_t TimerGolpeUsar{0};
    std::int32_t TimerPuedeSerAtacado{0};
    std::int32_t TimerPerteneceNpc{0};
    std::int32_t TimerEstadoAtacable{0};
    
    std::int32_t Trabajando{0};
    std::int32_t Ocultando{0};
    std::int32_t failedUsageAttempts{0};
    std::int32_t goHome{0};
    std::uint8_t AsignedSkills{0};
};

struct tFacciones {
    std::uint8_t ArmadaReal{0};
    std::uint8_t FuerzasCaos{0};
    std::int32_t CriminalesMatados{0};
    std::int32_t CiudadanosMatados{0};
    std::int32_t RecompensasReal{0};
    std::int32_t RecompensasCaos{0};
    std::uint8_t RecibioExpInicialReal{0};
    std::uint8_t RecibioExpInicialCaos{0};
    std::uint8_t RecibioArmaduraReal{0};
    std::uint8_t RecibioArmaduraCaos{0};
    std::uint8_t Reenlistadas{0};
    std::int16_t NivelIngreso{0};
    std::string FechaIngreso;
    std::int16_t MatadosIngreso{0};
    std::int16_t NextRecompensa{0};
};

struct tCrafting {
    std::int32_t Cantidad{0};
    std::int16_t PorCiclo{0};
};

struct User {
    std::string name;
    std::int32_t ID{0};
    bool showName{false};
    
    Char char_appearance{};
    Char CharMimetizado{};
    Char OrigChar{};
    
    std::string desc;
    std::string DescRM;
    
    eClass clase{eClass::Mage};
    eRaza raza{eRaza::Humano};
    eGenero Genero{eGenero::Hombre};
    std::string email;
    eCiudad Hogar{eCiudad::cUllathorpe};
    
    Inventario Invent{};
    WorldPos Pos{};
    
    bool ConnIDValida{false};
    std::int32_t ConnID{0};
    
    BancoInventario BancoInvent{};
    UserCounters Counters{};
    tCrafting Construir{};
    
    std::int16_t MascotasIndex[MAXMASCOTAS + 1]{};
    std::int16_t MascotasType[MAXMASCOTAS + 1]{};
    std::int16_t NroMascotas{0};
    
    UserStats Stats{};
    UserFlags flags{};
    tReputacion Reputacion{};
    tFacciones Faccion{};
    
    SecurityData Security{};
    
    std::string ip;
    tCOmercioUsuario ComUsu{};
    
    std::int16_t GuildIndex{0};
    ALINEACION_GUILD FundandoGuildAlineacion{ALINEACION_GUILD::ALINEACION_NINGUNA};
    std::int16_t EscucheClan{0};
    
    std::int16_t PartyIndex{0};
    std::int16_t PartySolicitud{0};
    std::int16_t KeyCrypt{0};
    
    AreaInfo AreasInfo{};
    
    std::unique_ptr<clsByteQueue> outgoingData;
    std::unique_ptr<clsByteQueue> incomingData;
    
    std::uint8_t CurrentInventorySlots{0};
};

struct NPCStats {
    std::int16_t Alineacion{0};
    std::int16_t MaxHp{0};
    std::int16_t MinHp{0};
    std::int16_t MaxHIT{0};
    std::int16_t MinHIT{0};
    std::int16_t def{0};
    std::int16_t defM{0};
};

struct NpcCounters {
    std::int16_t Paralisis{0};
    std::int32_t TiempoExistencia{0};
};

struct NPCFlags {
    std::uint8_t AfectaParalisis{0};
    std::int16_t Domable{0};
    std::uint8_t Respawn{0};
    bool NPCActive{false};
    bool Follow{false};
    std::uint8_t Faccion{0};
    std::uint8_t AtacaDoble{0};
    std::uint8_t LanzaSpells{0};
    
    std::int32_t ExpCount{0};
    TipoAI OldMovement{TipoAI::StaticNPC};
    std::uint8_t OldHostil{0};
    
    std::uint8_t AguaValida{0};
    std::uint8_t TierraInvalida{0};
    
    std::int16_t Sound{0};
    std::string AttackedBy;
    std::string AttackedFirstBy;
    std::uint8_t BackUp{0};
    std::uint8_t RespawnOrigPos{0};
    
    std::uint8_t Envenenado{0};
    std::uint8_t Paralizado{0};
    std::uint8_t Inmovilizado{0};
    std::uint8_t invisible{0};
    std::uint8_t Maldicion{0};
    std::uint8_t Bendicion{0};
    
    std::int16_t Snd1{0};
    std::int16_t Snd2{0};
    std::int16_t Snd3{0};
};

struct tCriaturasEntrenador {
    std::int16_t NpcIndex{0};
    std::string NpcName;
    std::int16_t tmpIndex{0};
};

struct NpcPathFindingInfo {
    std::vector<tVertice> Path;
    Position Target{};
    std::int16_t PathLenght{0};
    std::int16_t CurPos{0};
    std::int16_t TargetUser{0};
    bool NoPath{false};
};

struct tDrops {
    std::int16_t ObjIndex{0};
    std::int32_t Amount{0};
};

struct npc {
    std::string name;
    Char char_appearance{};
    std::string desc;
    
    eNPCType NPCtype{eNPCType::Comun};
    std::int16_t Numero{0};
    
    std::uint8_t InvReSpawn{0};
    std::int16_t Comercia{0};
    std::int32_t Target{0};
    std::int32_t TargetNPC{0};
    std::int16_t TipoItems{0};
    
    std::uint8_t Veneno{0};
    WorldPos Pos{};
    WorldPos Orig{};
    std::int16_t SkillDomar{0};
    
    TipoAI Movement{TipoAI::StaticNPC};
    std::uint8_t Attackable{0};
    std::uint8_t Hostile{0};
    std::int32_t PoderAtaque{0};
    std::int32_t PoderEvasion{0};
    
    std::int16_t Owner{0};
    std::int32_t GiveEXP{0};
    std::int32_t GiveGLD{0};
    tDrops Drop[MAX_NPC_DROPS + 1]{};
    
    NPCStats Stats{};
    NPCFlags flags{};
    NpcCounters Contadores{};
    
    Inventario Invent{};
    std::uint8_t CanAttack{0};
    
    std::uint8_t NroExpresiones{0};
    std::vector<std::string> Expresiones;
    
    std::uint8_t NroSpells{0};
    std::vector<std::int16_t> Spells;
    
    std::int16_t NroCriaturas{0};
    std::vector<tCriaturasEntrenador> Criaturas;
    std::int16_t MaestroUser{0};
    std::int16_t MaestroNpc{0};
    std::int16_t Mascotas{0};
    
    NpcPathFindingInfo PFINFO{};
    AreaInfo AreasInfo{};
    std::uint8_t Ciudad{0};
};

struct MapBlock {
    std::uint8_t Blocked{0};
    std::int16_t Graphic[5]{};
    std::int16_t UserIndex{0};
    std::int16_t NpcIndex{0};
    Obj ObjInfo{};
    WorldPos TileExit{};
    eTrigger trigger{eTrigger::NADA};
};

struct MapInfo {
    std::int16_t NumUsers{0};
    std::string Music;
    std::string name;
    WorldPos StartPos{};
    std::int16_t MapVersion{0};
    bool Pk{false};
    std::uint8_t MagiaSinEfecto{0};
    std::uint8_t NoEncriptarMP{0};
    std::uint8_t InviSinEfecto{0};
    std::uint8_t ResuSinEfecto{0};
    
    std::uint8_t RoboNpcsPermitido{0};
    std::string Terreno;
    std::string Zona;
    std::string Restringir;
    std::uint8_t BackUp{0};
};

struct HomeDistance {
    std::int16_t distanceToCity[6]{};
};

// ==========================================
// VARIABLES GLOBALES (DECLARACIONES EXTERN)
// ==========================================

extern bool SERVERONLINE;
extern std::string ULTIMAVERSION;
extern bool BackUp;

extern std::array<std::string, NUMRAZAS + 1> ListaRazas;
extern std::array<std::string, NUMSKILLS + 1> SkillsNames;
extern std::array<std::string, NUMCLASES + 1> ListaClases;
extern std::array<std::string, NUMATRIBUTOS + 1> ListaAtributos;

extern std::int32_t recordusuarios;

extern std::string IniPath;
extern std::string CharPath;
extern std::string MapPath;
extern std::string DatPath;

extern std::uint8_t MinXBorder;
extern std::uint8_t MaxXBorder;
extern std::uint8_t MinYBorder;
extern std::uint8_t MaxYBorder;

extern std::int16_t NumUsers;
extern std::int16_t LastUser;
extern std::int16_t LastChar;
extern std::int16_t NumChars;
extern std::int16_t LastNPC;
extern std::int16_t NumNPCs;
extern std::int16_t NumFX;
extern std::int16_t NumMaps;
extern std::int16_t NumObjDatas;
extern std::int16_t NumeroHechizos;
extern std::uint8_t AllowMultiLogins;
extern std::int16_t IdleLimit;
extern std::int16_t MaxUsers;
extern std::uint8_t HideMe;
extern std::string LastBackup;
extern std::string Minutos;
extern bool haciendoBK;
extern std::int16_t PuedeCrearPersonajes;
extern std::int16_t ServerSoloGMs;
extern std::int16_t IntervaloParalizado;

extern std::uint8_t MD5ClientesActivado;

extern bool EnPausa;
extern bool EnTesting;

extern std::vector<User> UserList;
extern std::array<npc, MAXNPCS + 1> Npclist;
extern std::vector<MapBlock> MapData;
extern std::vector<MapInfo> MapInfoList;
extern std::vector<tHechizo> Hechizos;
extern std::array<std::int16_t, MAXCHARS + 1> CharList;
extern std::vector<ObjData> ObjDataList;
extern std::vector<FXdata> FX;
extern std::vector<tCriaturasEntrenador> SpawnList;
extern std::array<LevelSkill, 51> LevelSkillList;
extern std::vector<std::string> ForbidenNames;
extern std::vector<std::int16_t> ArmasHerrero;
extern std::vector<std::int16_t> ArmadurasHerrero;
extern std::vector<std::int16_t> ObjCarpintero;
extern std::vector<std::string> MD5s;

extern std::unique_ptr<clsAntiDoS> aDos;
// aClon (clsAntiMassClon) excluido por tratarse de código muerto (ver docs/audit/02b-antimassclon-detalle.md)

extern std::array<std::unique_ptr<clsParty>, MAX_PARTIES + 1> Parties;
extern std::array<ModClase, NUMCLASES + 1> ModClaseList;
extern std::array<ModRaza, NUMRAZAS + 1> ModRazaList;
extern std::array<double, NUMCLASES + 1> ModVida;
extern std::array<std::int16_t, 6> DistribucionEnteraVida;
extern std::array<std::int16_t, 5> DistribucionSemienteraVida;
extern std::array<WorldPos, NUMCIUDADES + 1> Ciudades;
extern std::vector<HomeDistance> distanceToCities;

extern WorldPos Nix;
extern WorldPos Ullathorpe;
extern WorldPos Banderbill;
extern WorldPos Lindos;
extern WorldPos Arghal;

extern WorldPos Prision;
extern WorldPos Libertad;

struct tMotd {
    std::string texto;
    std::string Formato;
};

extern std::vector<tMotd> MOTD;
extern std::int16_t MaxLines;
extern std::uint8_t BootDelBackUp;
extern std::int16_t Puerto;
extern std::int16_t MAPA_PRETORIANO;

// Variables globales de Tablas de Datos (Grupo 5 FileIO)
extern std::int16_t NumObjDatas;
extern std::int16_t NumeroHechizos;
extern std::int16_t PorcentajeRecuperoMana;
extern float ExponenteNivelParty;
extern std::array<std::int32_t, NUM_RANGOS_FACCION + 1> RecompensaFacciones;
extern tAPuestas Apuestas;
extern std::array<std::array<tFaccionArmaduras, NUMRAZAS + 1>, NUMCLASES + 1> ArmadurasFaccion;

// Armaduras Faccionarias (ModFacciones.bas)
extern std::int16_t ArmaduraImperial1, ArmaduraImperial2, ArmaduraImperial3, TunicaMagoImperial, TunicaMagoImperialEnanos;
extern std::int16_t ArmaduraCaos1, ArmaduraCaos2, ArmaduraCaos3, TunicaMagoCaos, TunicaMagoCaosEnanos;
extern std::int16_t VestimentaImperialHumano, VestimentaImperialEnano, TunicaConspicuaHumano, TunicaConspicuaEnano, ArmaduraNobilisimaHumano, ArmaduraNobilisimaEnano, ArmaduraGranSacerdote;
extern std::int16_t VestimentaLegionHumano, VestimentaLegionEnano, TunicaLobregaHumano, TunicaLobregaEnano, TunicaEgregiaHumano, TunicaEgregiaEnano, SacerdoteDemoniaco;

// Intervalos de Servidor (Server.ini / Admin.bas)
extern std::int16_t SanaIntervaloSinDescansar, StaminaIntervaloSinDescansar, SanaIntervaloDescansar, StaminaIntervaloDescansar;
extern std::int16_t IntervaloSed, IntervaloHambre, IntervaloVeneno;
extern std::int16_t IntervaloInvisible, IntervaloFrio, IntervaloWavFx, IntervaloInvocacion, IntervaloParaConexion;
extern std::int16_t IntervaloPuedeSerAtacado, IntervaloAtacable, IntervaloOwnedNpc;
extern std::int16_t IntervaloUserPuedeCastear, IntervaloUserPuedeTrabajar, IntervaloUserPuedeAtacar;
extern std::int16_t IntervaloMagiaGolpe, IntervaloGolpeMagia, IntervaloGolpeUsar;
extern std::int16_t MinutosWs, IntervaloCerrarConexion, IntervaloUserPuedeUsar, IntervaloFlechasCazadores, IntervaloOculto;

extern std::unique_ptr<cCola> Ayuda;
extern std::unique_ptr<ConsultasPopulares> ConsultaPopular;
extern std::unique_ptr<SoundMapInfo> SonidosMapas;
extern std::vector<cGarbage> TrashCollector;

// Win32 API functions declarations
#if defined(_WIN32)
// GetTickCount, WritePrivateProfileStringA, GetPrivateProfileStringA, ZeroMemory ya están provistas por <windows.h>
#else
inline std::uint32_t GetTickCount() {
    using namespace std::chrono;
    return static_cast<std::uint32_t>(duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count());
}

inline void ZeroMemory(void* destination, std::size_t length) {
    std::memset(destination, 0, length);
}
#endif
