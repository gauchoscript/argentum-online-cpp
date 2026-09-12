#include "Declares.hpp"

// Instanciaciones de variables globales
bool SERVERONLINE = false;
std::string ULTIMAVERSION;
bool BackUp = false;

std::array<std::string, NUMRAZAS + 1> ListaRazas{};
std::array<std::string, NUMSKILLS + 1> SkillsNames{};
std::array<std::string, NUMCLASES + 1> ListaClases{};
std::array<std::string, NUMATRIBUTOS + 1> ListaAtributos{};

std::int32_t recordusuarios = 0;

std::string IniPath;
std::string CharPath;
std::string MapPath;
std::string DatPath;

std::uint8_t MinXBorder = 0;
std::uint8_t MaxXBorder = 0;
std::uint8_t MinYBorder = 0;
std::uint8_t MaxYBorder = 0;

std::int16_t NumUsers = 0;
std::int16_t LastUser = 0;
std::int16_t LastChar = 0;
std::int16_t NumChars = 0;
std::int16_t LastNPC = 0;
std::int16_t NumNPCs = 0;
std::int16_t NumFX = 0;
std::int16_t NumMaps = 0;
std::int16_t NumObjDatas = 0;
std::int16_t NumeroHechizos = 0;
std::uint8_t AllowMultiLogins = 0;
std::int16_t IdleLimit = 0;
std::int16_t MaxUsers = 0;
std::uint8_t HideMe = 0;
std::string LastBackup;
std::string Minutos;
bool haciendoBK = false;
std::int16_t PuedeCrearPersonajes = 0;
std::int16_t ServerSoloGMs = 0;

std::uint8_t MD5ClientesActivado = 0;

bool EnPausa = false;
bool EnTesting = false;

std::vector<User> UserList;
std::vector<ConnGroup> ConnGroups;
std::array<npc, MAXNPCS + 1> Npclist{};
std::vector<MapBlock> MapData;
std::vector<MapInfo> MapInfoList;
std::vector<tHechizo> Hechizos;
std::array<std::int16_t, MAXCHARS + 1> CharList{};
std::vector<ObjData> ObjDataList;
std::vector<FXdata> FX;
std::vector<tCriaturasEntrenador> SpawnList;
std::array<LevelSkill, 51> LevelSkillList{};
std::vector<std::string> ForbidenNames;
std::vector<std::int16_t> ArmasHerrero;
std::vector<std::int16_t> ArmadurasHerrero;
std::vector<std::int16_t> ObjCarpintero;
std::vector<std::string> MD5s;

std::unique_ptr<clsAntiDoS> aDos;
// aClon (clsAntiMassClon) excluido por tratarse de código muerto (ver docs/audit/02b-antimassclon-detalle.md)

std::array<std::unique_ptr<clsParty>, MAX_PARTIES + 1> Parties;
std::array<ModClase, NUMCLASES + 1> ModClaseList{};
std::array<ModRaza, NUMRAZAS + 1> ModRazaList{};
std::array<double, NUMCLASES + 1> ModVida{};
std::array<std::int16_t, 6> DistribucionEnteraVida{};
std::array<std::int16_t, 5> DistribucionSemienteraVida{};
std::array<WorldPos, NUMCIUDADES + 1> Ciudades{};
std::vector<HomeDistance> distanceToCities;

WorldPos Nix{};
WorldPos Ullathorpe{};
WorldPos Banderbill{};
WorldPos Lindos{};
WorldPos Arghal{};

WorldPos Prision{};
WorldPos Libertad{};

std::unique_ptr<cCola> Ayuda;
std::unique_ptr<ConsultasPopulares> ConsultaPopular;
std::unique_ptr<SoundMapInfo> SonidosMapas;
std::vector<cGarbage> TrashCollector;
std::int16_t IntervaloParalizado = 500;

// Variables globales de MOTD, Server.ini e Intervalos
std::vector<tMotd> MOTD;
std::int16_t MaxLines = 0;
std::uint8_t BootDelBackUp = 0;
std::int16_t Puerto = 7666;
std::int16_t MAPA_PRETORIANO = 0;

std::int16_t PorcentajeRecuperoMana = 0;
float ExponenteNivelParty = 0.0f;
std::array<std::int32_t, NUM_RANGOS_FACCION + 1> RecompensaFacciones{};
tAPuestas Apuestas{};
std::array<std::array<tFaccionArmaduras, NUMRAZAS + 1>, NUMCLASES + 1> ArmadurasFaccion{};

std::int16_t ArmaduraImperial1 = 0, ArmaduraImperial2 = 0, ArmaduraImperial3 = 0, TunicaMagoImperial = 0, TunicaMagoImperialEnanos = 0;
std::int16_t ArmaduraCaos1 = 0, ArmaduraCaos2 = 0, ArmaduraCaos3 = 0, TunicaMagoCaos = 0, TunicaMagoCaosEnanos = 0;
std::int16_t VestimentaImperialHumano = 0, VestimentaImperialEnano = 0, TunicaConspicuaHumano = 0, TunicaConspicuaEnano = 0, ArmaduraNobilisimaHumano = 0, ArmaduraNobilisimaEnano = 0, ArmaduraGranSacerdote = 0;
std::int16_t VestimentaLegionHumano = 0, VestimentaLegionEnano = 0, TunicaLobregaHumano = 0, TunicaLobregaEnano = 0, TunicaEgregiaHumano = 0, TunicaEgregiaEnano = 0, SacerdoteDemoniaco = 0;

std::int16_t SanaIntervaloSinDescansar = 0, StaminaIntervaloSinDescansar = 0, SanaIntervaloDescansar = 0, StaminaIntervaloDescansar = 0;
std::int16_t IntervaloSed = 0, IntervaloHambre = 0, IntervaloVeneno = 0;
std::int16_t IntervaloInvisible = 0, IntervaloFrio = 0, IntervaloWavFx = 0, IntervaloInvocacion = 0, IntervaloParaConexion = 0;
std::int16_t IntervaloPuedeSerAtacado = 0, IntervaloAtacable = 0, IntervaloOwnedNpc = 0;
std::int16_t IntervaloUserPuedeCastear = 0, IntervaloUserPuedeTrabajar = 0, IntervaloUserPuedeAtacar = 0;
std::int16_t IntervaloMagiaGolpe = 0, IntervaloGolpeMagia = 0, IntervaloGolpeUsar = 0;
std::int16_t MinutosWs = 0, IntervaloCerrarConexion = 0, IntervaloUserPuedeUsar = 0, IntervaloFlechasCazadores = 0, IntervaloOculto = 0;
