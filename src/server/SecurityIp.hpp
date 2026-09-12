#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <string_view>
#include <functional>
#include <stdexcept>

/**
 * @file SecurityIp.hpp
 * @brief Subconjunto de seguridad de IP y control anti-flood de SecurityIp.bas.
 *
 * NOTA DE ALCANCE:
 * Este módulo implementa las funciones públicas y ayudantes privados del subsistema
 * anti-flood (IpTables, intervalos mínimos entre conexiones y mantenimiento horario).
 * Todo lo relativo a MaxConTables, límites de concurrencia x IP (IPSecuritySuperaLimiteConexiones /
 * IpRestarConexion) y baneo permanente de IP está explícitamente postergado para etapas posteriores
 * (TCP.bas y Admin.bas). Ver docs/implementation/KNOWN-LEGACY-BUGS.md.
 */
namespace SecurityIp {

// Código de error de tiempo de ejecución de VB6 para desbordamiento aritmético (Error 6: "Overflow")
constexpr std::int32_t ERR_OVERFLOW = 6;

/**
 * @brief Excepción base para errores del subsistema de seguridad IP.
 *
 * Sigue el patrón estructural y de nomenclatura de ByteQueueException (clsByteQueue.hpp),
 * encapsulando el código de error legacy original de VB6.
 */
class SecurityIpException : public std::runtime_error {
public:
    SecurityIpException(const std::string& message, std::int32_t errorCode)
        : std::runtime_error(message), m_errorCode(errorCode) {}
    std::int32_t getErrorCode() const { return m_errorCode; }
private:
    std::int32_t m_errorCode;
};

/**
 * @brief Excepción lanzada ante desbordamiento aritmético al calcular intervalos temporales.
 *
 * Reproduce fielmente el Error 6 ("Overflow") que producía el ejecutable nativo de VB6
 * (compilado con OverflowCheck=0 en SERVER.VBP) al evaluar:
 * `IpTables(IpTableIndex + 1) + IntervaloEntreConexiones`
 * cuando el timestamp almacenado supera INT32_MAX - IntervaloEntreConexiones (~24.85 días de uptime
 * en GetTickCount interpretado como Long con signo).
 */
class TickCountOverflowException : public SecurityIpException {
public:
    TickCountOverflowException()
        : SecurityIpException("Tick count arithmetic overflow", ERR_OVERFLOW) {}
    explicit TickCountOverflowException(const std::string& details)
        : SecurityIpException("Tick count arithmetic overflow: " + details, ERR_OVERFLOW) {}
};

// Constante literal de intervalo mínimo entre conexiones en milisegundos (SecurityIp.bas:48)
constexpr std::int32_t IntervaloEntreConexiones = 1000;

// Variables de estado del módulo (en VB6: Private IpTables, EntrysCounter, MaxValue, Multiplicado)
extern std::vector<std::int32_t> IpTables;
extern std::int32_t EntrysCounter;
extern std::int32_t MaxValue;
extern std::int32_t Multiplicado;

// Tipo de puntero a función para la fuente de tiempo (GetTickCount)
using TimeSourceFn = std::uint32_t (*)();

/**
 * @brief Inicializa las tablas internas de intervalos (SecurityIp.bas líneas 63-78).
 *
 * NOTA DE POSTERGACIÓN:
 * En VB6 (SecurityIp.bas:75-76), esta rutina también dimensionaba MaxConTables con
 * `Declaraciones.MaxUsers * 2 - 1` y reseteaba `MaxConTablesEntry = 0`.
 * Dicha porción se encuentra deliberadamente omitida en este paso, postergada hasta el
 * porteo del módulo TCP.bas (donde se abordarán IPSecuritySuperaLimiteConexiones e IpRestarConexion).
 * NOTA ERGONÓMICA DE C++:
 * El valor por defecto (1000) es una conveniencia ergonómica exclusiva de C++; la signatura
 * original de VB6 requería obligatoriamente el argumento OptCountersValue en cada llamada.
 *
 * @param OptCountersValue Capacidad inicial base de entradas (por defecto 1000 en C++).
 */
void InitIpTables(std::int32_t OptCountersValue = 1000);

/**
 * @brief Limpieza y reescalado periódico horario de la tabla de intervalos (SecurityIp.bas líneas 86-97).
 *
 * QUIRK HISTÓRICO:
 * Restablece EntrysCounter dividiendo por el factor acumulado de Multiplicado y purga
 * únicamente IpTables, dejando intacta la tabla MaxConTables (ver docs/implementation/KNOWN-LEGACY-BUGS.md Entrada #15).
 */
void IpSecurityMantenimientoLista();

/**
 * @brief Evalúa si se autoriza un nuevo intento de conexión desde una IP dada (SecurityIp.bas líneas 99-130).
 *
 * Si la IP no existe en IpTables, la inserta con el timestamp actual y retorna true.
 * Si ya existe, valida que hayan transcurrido al menos IntervaloEntreConexiones (1000 ms)
 * mediante el operador '<=' exacto de VB6 (línea 111). Si transcurrieron, actualiza el timestamp
 * y retorna true; de lo contrario, retorna false.
 *
 * @param ip Dirección IP en formato numérico de 32 bits (Long en VB6).
 * @return true si la conexión fue aceptada; false si fue rechazada por flood.
 */
bool IpSecurityAceptarNuevaConexion(std::int32_t ip);

/**
 * @brief Permite inyectar una fuente de tiempo personalizada para pruebas unitarias deterministas.
 * @param fn Puntero a función que retorne milisegundos simulados (o nullptr para restaurar el reloj del sistema).
 */
void SetTimeSourceForTesting(TimeSourceFn fn);

/**
 * @brief Restablece la fuente de tiempo al reloj estándar del sistema (GetTickCount).
 */
void ResetTimeSource();

/**
 * @brief Emite un volcado de diagnóstico de las entradas activas en la tabla de IPs.
 *
 * Transliteración de DumpTables de SecurityIp.bas (líneas 314-327).
 * Itera las entradas de IpTables formateando la IP mediante TCP::GetAscIP.
 * Cuando se porte General.bas, la llamada legacy a General.LogCriticEvent se
 * canalizará mediante el log_sink provisto.
 *
 * @param log_sink Callback opcional receptor de cada línea de log. Si es nullptr, emite a std::clog.
 */
void DumpTables(std::function<void(std::string_view)> log_sink = nullptr);

// --- Funciones privadas / helpers internos ---

/**
 * @brief Búsqueda binaria literal sobre IpTables (SecurityIp.bas líneas 265-292).
 *
 * PRESERVA VERBATIM LOS SIGUIENTES DEFECTOS HISTÓRICOS DEL LEGACY:
 * 1. LEGACY BUG (SecurityIp.bas:291, see docs/implementation/KNOWN-LEGACY-BUGS.md Entrada #11):
 *    Retorno de ~(Middle * 2) en lugar de ~(First * 2), causando inserciones desordenadas.
 * 2. LEGACY QUIRK (SecurityIp.bas:278, see docs/implementation/KNOWN-LEGACY-BUGS.md Entrada #12):
 *    Cota superior Last = MaxValue (off-by-one en tablas vacías y con elementos).
 *
 * @param ip Dirección IP en formato entero con signo de 32 bits (Long en VB6).
 * @return Índice no negativo (Middle * 2) si la IP fue hallada, o valor negativo (~(Middle * 2))
 *         cuyo complemento bit a bit (~) indica dónde insertará AddNewIpIntervalo.
 */
std::int32_t FindTableIp(std::int32_t ip);

/**
 * @brief Inserta una nueva IP en IpTables corriendo los elementos hacia arriba (SecurityIp.bas líneas 133-154).
 *
 * @param ip Dirección IP a insertar.
 * @param index Índice par donde se coloca la IP (obtenido mediante ~FindTableIp).
 */
void AddNewIpIntervalo(std::int32_t ip, std::int32_t index);

} // namespace SecurityIp
