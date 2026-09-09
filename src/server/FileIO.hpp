#ifndef FILEIO_HPP
#define FILEIO_HPP

#include "Declares.hpp"
#include "clsIniReader.hpp"
#include <string>

// ============================================================================
// FileIO.hpp — Persistencia e E/S de Archivos (Servidor AO)
// Estructura organizada por Grupos Lógicos de Implementación (G1 a G7)
// ============================================================================

namespace FileIO {

// ============================================================================
// GRUPO 1: Utilidades Base de Archivos e INI (Low-Level INI & File Helpers)
// ============================================================================

/**
 * @brief Lee el valor de una clave en una sección de un archivo INI.
 * @param file Ruta del archivo INI.
 * @param main Nombre de la sección.
 * @param var Nombre de la clave.
 * @param emptySpaces Tamaño por defecto de búfer (compatibilidad con VB6).
 * @return Valor de la clave o cadena vacía si no existe.
 */
std::string GetVar(const std::string& file, const std::string& main, const std::string& var, long emptySpaces = 1024);

/**
 * @brief Escribe o actualiza una clave en una sección de un archivo INI (codificación ANSI/Windows-1252 y CRLF).
 * @param file Ruta del archivo INI.
 * @param main Nombre de la sección.
 * @param var Nombre de la clave.
 * @param value Valor a escribir.
 */
void WriteVar(const std::string& file, const std::string& main, const std::string& var, const std::string& value);

/**
 * @brief Retorna el tamaño en bytes de un archivo en disco.
 * @param name Ruta del archivo.
 * @return Tamaño en bytes o 0 si no existe.
 */
long TxtDimension(const std::string& name);

/**
 * @brief Obtiene una subcadena delimitada por un carácter (1-indexed, equivalente a ReadField de VB6).
 * @param pos Índice del campo (1-indexed).
 * @param text Cadena delimitada.
 * @param sepAscii Código ASCII del separador.
 * @return Subcadena obtenida.
 */
std::string ReadField(int pos, const std::string& text, int sepAscii);


// ============================================================================
// GRUPO 2: Persistencia de Personajes (.chr) — 🚨 CATEGORÍA CRÍTICA 1
// ============================================================================

/**
 * @brief Consulta si el personaje indicado por UserIndex es criminal en base a su reputación.
 * @param userIndex Índice del usuario en UserList.
 * @return true si es criminal, false en caso contrario.
 */
bool criminal(int userIndex);

/**
 * @brief Carga las estadísticas, habilidades, atributos y muertes del personaje desde un archivo INI (.chr).
 * @param userIndex Índice del usuario en UserList.
 * @param userFile Parser clsIniReader inicializado sobre el archivo .chr.
 */
void LoadUserStats(int userIndex, clsIniReader& userFile);

/**
 * @brief Carga los valores de reputación del personaje desde un archivo INI (.chr).
 * @param userIndex Índice del usuario en UserList.
 * @param userFile Parser clsIniReader inicializado sobre el archivo .chr.
 */
void LoadUserReputacion(int userIndex, clsIniReader& userFile);

/**
 * @brief Procedimiento principal de carga de datos iniciales, inventario, hechizos y estado de personaje (.chr).
 * @param userIndex Índice del usuario en UserList.
 * @param userFile Parser clsIniReader inicializado sobre el archivo .chr.
 */
void LoadUserInit(int userIndex, clsIniReader& userFile);

/**
 * @brief Serializa el estado completo del personaje en UserList(UserIndex) hacia su archivo INI (.chr).
 * @param userIndex Índice del usuario en UserList.
 * @param userFile Ruta del archivo .chr del usuario.
 */
void SaveUser(int userIndex, const std::string& userFile);


// ============================================================================
// GRUPO 3: Configuración del Servidor y Roles Administrativos (Server.ini & Roles)
// ============================================================================

/**
 * @brief Verifica si el nombre pertenece a la lista de Administradores [Admines] en Server.ini.
 * @note Consulta dinámica a disco en cada invocación, insensible a mayúsculas/minúsculas, no jerárquica.
 */
bool EsAdmin(const std::string& name);

/**
 * @brief Verifica si el nombre pertenece a la lista de Dioses [Dioses] en Server.ini.
 * @note Consulta dinámica a disco en cada invocación, insensible a mayúsculas/minúsculas, no jerárquica.
 */
bool EsDios(const std::string& name);

/**
 * @brief Verifica si el nombre pertenece a la lista de SemiDioses [SemiDioses] en Server.ini.
 * @note Consulta dinámica a disco en cada invocación, insensible a mayúsculas/minúsculas, no jerárquica.
 */
bool EsSemiDios(const std::string& name);

/**
 * @brief Verifica si el nombre pertenece a la lista de Consejeros [Consejeros] en Server.ini.
 * @note Consulta dinámica a disco en cada invocación, insensible a mayúsculas/minúsculas, no jerárquica.
 */
bool EsConsejero(const std::string& name);

/**
 * @brief Verifica si el nombre pertenece a la lista de RolesMasters [RolesMasters] en Server.ini.
 * @note Consulta dinámica a disco en cada invocación, insensible a mayúsculas/minúsculas, no jerárquica. Key: RM<N>.
 */
bool EsRolesMaster(const std::string& name);

/**
 * @brief Carga el mensaje del día desde Dat/Motd.ini en el arreglo global MOTD.
 */
void LoadMotd();

/**
 * @brief Carga los parámetros globales de configuración del servidor desde Server.ini y Ciudades.dat.
 */
void LoadSini();


// ============================================================================
// GRUPO 4: Carga y Guardado de Mapas Binarios e INI (.map, .inf, .dat)
// ============================================================================

/**
 * @brief Carga un mapa completo desde disco (.map binario, .inf binario y .dat INI).
 * @param map ID del mapa (1-indexed).
 * @param mapFile Ruta base del mapa sin extensión (ej. "Maps/Mapa1").
 */
void CargarMapa(long map, const std::string& mapFile);

/**
 * @brief Guarda un mapa completo a disco (.map binario, .inf binario y .dat INI).
 * @param map ID del mapa (1-indexed).
 * @param mapFile Ruta base de destino sin extensión.
 */
void GrabarMapa(long map, const std::string& mapFile);

/**
 * @brief Carga todos los mapas configurados en Dat/Map.dat.
 */
void LoadMapData();

/**
 * @brief Precomputa la matriz de distancias (distanceToCities) desde cada mapa hacia las ciudades.
 * @param mapa Parámetro heredado del VB6 original (MATRIX_INITIAL_MAP).
 */
void generateMatrix(int mapa);

/**
 * @brief Calcula recursivamente la distancia de un mapa a una ciudad explorando límites cardinales.
 * @param mapa ID del mapa actual.
 * @param city ID de la ciudad destino (1 a NUMCIUDADES).
 * @param side Dirección cardinal de entrada.
 * @param x Desplazamiento acumulado en X (por defecto 0).
 * @param y Desplazamiento acumulado en Y (por defecto 0).
 */
void setDistance(int mapa, uint8_t city, int side, int x = 0, int y = 0);

/**
 * @brief Obtiene el ID del mapa colindante en el lado indicado examinando los TileExit de borde.
 * @param mapa ID del mapa a inspeccionar.
 * @param side Dirección cardinal (eHeading::NORTH, EAST, SOUTH, WEST).
 * @return ID del mapa vecino o 0 si no hay transición.
 */
int getLimit(int mapa, uint8_t side);


// ============================================================================
// GRUPO 5: Carga de Tablas de Datos del Juego (Data-Table Loaders)
// ============================================================================

/**
 * @brief Carga la lista de criaturas invocables por entrenadores desde Dat/Invokar.dat hacia SpawnList.
 */
void CargarSpawnList();

/**
 * @brief Carga la lista de nombres prohibidos/inválidos desde Dat/NombresInvalidos.txt hacia ForbidenNames.
 */
void CargarForbidenWords();

/**
 * @brief Carga el catálogo de hechizos del juego desde Dat/Hechizos.dat hacia Hechizos.
 */
void CargarHechizos();

/**
 * @brief Carga el catálogo de armas de herrero desde Dat/ArmasHerrero.dat hacia ArmasHerrero.
 */
void LoadArmasHerreria();

/**
 * @brief Carga el catálogo de armaduras de herrero desde Dat/ArmadurasHerrero.dat hacia ArmadurasHerrero.
 */
void LoadArmadurasHerreria();

/**
 * @brief Carga las constantes y modificadores de balance desde Dat/Balance.dat hacia ModClaseList, ModRazaList, ModVida, etc.
 */
void LoadBalance();

/**
 * @brief Carga el catálogo de objetos de carpintero desde Dat/ObjCarpintero.dat hacia ObjCarpintero.
 */
void LoadObjCarpintero();

/**
 * @brief Carga la base de datos completa de objetos del juego desde Dat/Obj.dat hacia ObjDataList.
 */
void LoadOBJData();

/**
 * @brief Carga los contadores y ganancias de apuestas desde Dat/apuestas.dat hacia Apuestas.
 */
void CargaApuestas();

/**
 * @brief Carga los requerimientos e índices de armaduras faccionarias desde Dat/ArmadurasFaccionarias.dat hacia ArmadurasFaccion.
 */
void LoadArmadurasFaccion();

// ============================================================================
// GRUPO 7: Logging Administrativo y Sanciones (Admin Logging & Bans)
// ============================================================================

extern std::string LogsPath;

/**
 * @brief Registra el baneo de un usuario en línea (escribe en logs/BanDetail.log e inserta en logs/GenteBanned.log).
 * @param bannedIndex Índice del usuario sancionado en UserList.
 * @param userIndex Índice del administrador que aplica la sanción en UserList.
 * @param motivo Motivo o justificación del baneo.
 */
void LogBan(int16_t bannedIndex, int16_t userIndex, const std::string& motivo);

/**
 * @brief Registra el baneo de un usuario a partir de su nombre (escribe en logs/BanDetail.dat e inserta en logs/GenteBanned.log).
 * @param bannedName Nombre del usuario sancionado.
 * @param userIndex Índice del administrador que aplica la sanción en UserList.
 * @param motivo Motivo o justificación del baneo.
 */
void LogBanFromName(const std::string& bannedName, int16_t userIndex, const std::string& motivo);

/**
 * @brief Registra el baneo con nombre de sancionado y sancionador textual (escribe en logs/BanDetail.dat e inserta en logs/GenteBanned.log).
 * @param bannedName Nombre del usuario sancionado.
 * @param baneador Nombre del administrador sancionador.
 * @param motivo Motivo o justificación del baneo.
 */
void Ban(const std::string& bannedName, const std::string& baneador, const std::string& motivo);


// ============================================================================
// GRUPO 6: Sistema de Respaldos de Mundo (World Backup System)
// ============================================================================

extern std::string WorldBackupPath;

/**
 * @brief Persiste el estado completo de un NPC en disco (Dat/bkNPCs.dat) bajo formato INI.
 * @param npcIndex Índice del NPC en Npclist.
 */
void BackUPnPc(int16_t npcIndex);

/**
 * @brief Carga y restaura el estado de un NPC respaldado desde Dat/bkNPCs.dat hacia Npclist.
 * @param npcIndex Índice del NPC en Npclist.
 * @param npcNumber Número de plantilla del NPC.
 */
void CargarNpcBackUp(int16_t npcIndex, int16_t npcNumber);

/**
 * @brief Carga todos los mapas del mundo restaurando desde WorldBackUp/ aquellos que tengan BackUp=1.
 */
void CargarBackUp();

/**
 * @brief Ejecuta el respaldo periódico de mundo: mapas con BackUp=1, NPCs con BackUp=1 y log en logs/BackUps.log.
 */
void DoBackUp();

} // namespace FileIO

#endif // FILEIO_HPP

