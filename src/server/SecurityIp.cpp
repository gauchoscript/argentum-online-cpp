#include "SecurityIp.hpp"
#include <cstring>
#include <chrono>
#include <limits>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace SecurityIp {

// Definición de variables de estado de SecurityIp.bas (líneas 44-47)
std::vector<std::int32_t> IpTables;
std::int32_t EntrysCounter = 0;
std::int32_t MaxValue = 0;
std::int32_t Multiplicado = 1;

/**
 * @brief Fuente de tiempo estándar del sistema que consume la API Win32 GetTickCount()
 *        (o std::chrono::steady_clock en plataformas no-Windows).
 */
static std::uint32_t SystemGetTickCount() {
#if defined(_WIN32)
    return ::GetTickCount();
#else
    using namespace std::chrono;
    return static_cast<std::uint32_t>(
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch()).count()
    );
#endif
}

/**
 * @brief Puntero a la fuente de tiempo activa. Por defecto apunta a SystemGetTickCount.
 *
 * JUSTIFICACIÓN DE DISEÑO:
 * Conforme a la Testing Philosophy de docs/CONVENTIONS.md, el reloj del sistema es una
 * frontera externa legítima que requiere control determinista para no demorar la suite
 * con esperas activas (sleep). Usar un puntero a función liviano preserva exactamente
 * la firma pública de VB6 sin introducir frameworks de mocking ni sobrecosto de clases abstractas.
 */
static TimeSourceFn g_TimeSource = &SystemGetTickCount;

void SetTimeSourceForTesting(TimeSourceFn fn) {
    if (fn != nullptr) {
        g_TimeSource = fn;
    } else {
        g_TimeSource = &SystemGetTickCount;
    }
}

void ResetTimeSource() {
    g_TimeSource = &SystemGetTickCount;
}

void InitIpTables(std::int32_t OptCountersValue) {
    EntrysCounter = OptCountersValue;
    Multiplicado = 1;

    // En VB6 (SecurityIp.bas:72): ReDim IpTables(EntrysCounter * 2) As Long
    // Al ser 0-based inclusivo en VB6, reserva desde el índice 0 hasta EntrysCounter * 2
    // (es decir, EntrysCounter * 2 + 1 elementos).
    IpTables.assign(static_cast<std::size_t>(EntrysCounter * 2 + 1), 0);
    MaxValue = 0;

    // OMISIÓN DELIBERADA POR POSTERGACIÓN ARQUITECTÓNICA:
    // En el código legacy de VB6 (SecurityIp.bas:75-76):
    //   ReDim MaxConTables(Declaraciones.MaxUsers * 2 - 1) As Long
    //   MaxConTablesEntry = 0
    // La asignación y administración de MaxConTables y MaxConTablesEntry pertenecen a las
    // rutinas IPSecuritySuperaLimiteConexiones e IpRestarConexion, las cuales se encontraban
    // comentadas en el servidor de producción 0.13.0 y cuyo análisis está diferido al porteo
    // del módulo TCP.bas (ver docs/implementation/16a-securityip-known-bugs.md, entradas #3 y #4).
}

void IpSecurityMantenimientoLista() {
    // Port transliterado literal de SecurityIp.bas líneas 86-97.
    // 'Las borro todas cada 1 hora, asi se "renuevan"
    // Restablece la capacidad base original de EntrysCounter dividiendo por el factor acumulado de Multiplicado.
    if (Multiplicado > 0) {
        EntrysCounter = EntrysCounter / Multiplicado;
    }
    Multiplicado = 1;

    // ReDim IpTables(EntrysCounter * 2) As Long
    IpTables.assign(static_cast<std::size_t>(EntrysCounter * 2 + 1), 0);
    MaxValue = 0;

    // QUIRK HISTÓRICO (#5): Nótese que esta rutina no reinicia MaxConTables ni MaxConTablesEntry.
    // Dicha asimetría se preserva conforme a docs/implementation/16a-securityip-known-bugs.md #5.
}

bool IpSecurityAceptarNuevaConexion(std::int32_t ip) {
    // Port transliterado literal de SecurityIp.bas líneas 99-130
    std::int32_t IpTableIndex = FindTableIp(ip);
    const std::int32_t currentTicks = static_cast<std::int32_t>(g_TimeSource());

    if (IpTableIndex >= 0) {
        const std::size_t timestampIdx = static_cast<std::size_t>(IpTableIndex + 1);
        const std::int32_t lastTicks = IpTables[timestampIdx];

        // PRE-CHECK DE DESBORDAMIENTO ARITMÉTICO (LEGACY VB6 ERROR 6):
        // En el ejecutable nativo de VB6 (SERVER.VBP con OverflowCheck=0), la expresión
        // IpTables(IpTableIndex + 1) + IntervaloEntreConexiones lanzaba un error de runtime
        // Error 6 ("Overflow") cuando lastTicks > INT32_MAX - IntervaloEntreConexiones.
        // Para reproducir este crash histórico sin incurrir en Undefined Behavior (UB)
        // por desbordamiento de enteros con signo en C++, realizamos la comprobación previa
        // y lanzamos TickCountOverflowException de inmediato sin mutar ningún estado interno.
        if (lastTicks > std::numeric_limits<std::int32_t>::max() - IntervaloEntreConexiones) {
            throw TickCountOverflowException();
        }

        // En VB6 (SecurityIp.bas:111):
        // If IpTables(IpTableIndex + 1) + IntervaloEntreConexiones <= GetTickCount Then
        //
        // NOTA DE COMPORTAMIENTO EXACTO (docs/audit/02a-securityip-detalle.md §5):
        // El operador utilizado en VB6 es estrictamente '<=' (menor o igual). Por lo tanto,
        // una conexión que intente ingresar con un delta exactamente igual a IntervaloEntreConexiones
        // (1000 ms) es aceptada. Si la condición se cumple, se refresca el timestamp en
        // IpTables(IpTableIndex + 1) y se retorna true. Si no, se retorna false y el timestamp anterior
        // NO se altera, permitiendo que un intento subsiguiente a >= 1000 ms del primero sea aceptado.
        if (lastTicks + IntervaloEntreConexiones <= currentTicks) {
            IpTables[timestampIdx] = currentTicks;
            return true;
        } else {
            return false;
        }
    } else {
        // En VB6 (SecurityIp.bas:122):
        // IpTableIndex = Not IpTableIndex
        // AddNewIpIntervalo ip, IpTableIndex
        // IpTables(IpTableIndex + 1) = GetTickCount
        // IpSecurityAceptarNuevaConexion = True
        IpTableIndex = ~IpTableIndex;
        AddNewIpIntervalo(ip, IpTableIndex);
        IpTables[static_cast<std::size_t>(IpTableIndex + 1)] = currentTicks;
        return true;
    }
}

std::int32_t FindTableIp(std::int32_t ip) {
    // Port transliterado literal de SecurityIp.bas líneas 271-291 (rama IP_INTERVALOS)
    std::int32_t First = 0;

    // LEGACY QUIRK (SecurityIp.bas:278, see docs/implementation/16a-securityip-known-bugs.md #2):
    // La cota superior se inicializa en Last = MaxValue (off-by-one en tablas vacías y con elementos).
    std::int32_t Last = MaxValue;
    std::int32_t Middle = 0;

    while (First <= Last) {
        Middle = (First + Last) / 2;

        if (IpTables[static_cast<std::size_t>(Middle * 2)] < ip) {
            First = Middle + 1;
        } else if (IpTables[static_cast<std::size_t>(Middle * 2)] > ip) {
            Last = Middle - 1;
        } else {
            // Coincidencia exacta encontrada
            return Middle * 2;
        }
    }

    // LEGACY BUG (SecurityIp.bas:291, see docs/implementation/16a-securityip-known-bugs.md #1):
    // Retorna Not (Middle * 2) usando el Middle residual de la última iteración,
    // en lugar de Not (First * 2), provocando inserciones fuera de orden.
    return ~(Middle * 2);
}

void AddNewIpIntervalo(std::int32_t ip, std::int32_t index) {
    // Port transliterado literal de SecurityIp.bas líneas 133-154
    // 2) Pruebo si hay espacio, sino agrando la lista (líneas 140-146)
    if (MaxValue + 1 > EntrysCounter) {
        EntrysCounter = EntrysCounter / Multiplicado;
        Multiplicado = Multiplicado + 1;
        EntrysCounter = EntrysCounter * Multiplicado;

        // ReDim Preserve IpTables(EntrysCounter * 2) As Long
        IpTables.resize(static_cast<std::size_t>(EntrysCounter * 2 + 1), 0);
    }

    // 4) Corro todo el array para arriba (línea 149)
    // Call CopyMemory(IpTables(index + 2), IpTables(index), (MaxValue - index \ 2) * 8)
    // Cada entrada en la tabla de intervalos ocupa 2 Longs (8 bytes: IP y timestamp).
    std::int32_t count = (MaxValue - index / 2) * 2;
    if (count > 0) {
        std::memmove(&IpTables[static_cast<std::size_t>(index + 2)],
                     &IpTables[static_cast<std::size_t>(index)],
                     static_cast<std::size_t>(count) * sizeof(std::int32_t));
    }

    IpTables[static_cast<std::size_t>(index)] = ip;

    // 3) Subo el indicador de el maximo valor almacenado y listo :) (línea 153)
    MaxValue = MaxValue + 1;
}

} // namespace SecurityIp
