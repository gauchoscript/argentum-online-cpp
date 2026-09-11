#include <doctest/doctest.h>
#include "server/SecurityIp.hpp"
#include <limits>

/**
 * =========================================================================================
 * TRAZA MANUAL (HAND-TRACE) DE INSERCIÓN LITERAL VB6 — SecurityIp.bas
 * =========================================================================================
 *
 * Secuencia de 5 IPs evaluada: [100, 300, 200, 400, 150]
 * Estado inicial:
 *   - EntrysCounter = 1000
 *   - Multiplicado = 1
 *   - MaxValue = 0
 *   - IpTables inicializado en ceros (2001 enteros).
 *
 * -----------------------------------------------------------------------------------------
 * PASO 1: Insertar IP = 100
 *   FindTableIp(100):
 *     - First = 0, Last = MaxValue = 0
 *     - Iteración 1: Middle = (0 + 0) \ 2 = 0
 *       IpTables(0) = 0 < 100 => First = Middle + 1 = 1
 *     - Fin bucle: First (1) > Last (0). Bucle termina.
 *     - Variables finales: First = 1, Middle = 0
 *     - Retorno: Not (Middle * 2) = Not 0 = -1
 *   En invocador:
 *     - index = ~(-1) = 0
 *     - AddNewIpIntervalo(100, 0): desplaza (0 - 0)*8 = 0 bytes.
 *     - IpTables(0) = 100; MaxValue pasa a 1.
 *   Estado tras Paso 1:
 *     - MaxValue = 1
 *     - IpTables: [100] (en índice 0)
 *
 * -----------------------------------------------------------------------------------------
 * PASO 2: Insertar IP = 300
 *   FindTableIp(300):
 *     - First = 0, Last = MaxValue = 1
 *     - Iteración 1: Middle = (0 + 1) \ 2 = 0
 *       IpTables(0) = 100 < 300 => First = 0 + 1 = 1
 *     - Iteración 2: First = 1, Last = 1 => Middle = (1 + 1) \ 2 = 1
 *       IpTables(2) = 0 < 300 => First = 1 + 1 = 2
 *     - Fin bucle: First (2) > Last (1). Bucle termina.
 *     - Variables finales: First = 2, Middle = 1
 *     - Retorno: Not (Middle * 2) = Not 2 = -3
 *   En invocador:
 *     - index = ~(-3) = 2
 *     - AddNewIpIntervalo(300, 2): desplaza (1 - 1)*8 = 0 bytes.
 *     - IpTables(2) = 300; MaxValue pasa a 2.
 *   Estado tras Paso 2:
 *     - MaxValue = 2
 *     - IpTables: [100, 300] (en índices 0 y 2)
 *
 * -----------------------------------------------------------------------------------------
 * PASO 3: Insertar IP = 200  <=== MANIFESTACIÓN DEL BUG DE LÍNEA 291 ===>
 *   FindTableIp(200):
 *     - First = 0, Last = MaxValue = 2
 *     - Iteración 1: Middle = (0 + 2) \ 2 = 1
 *       IpTables(2) = 300 > 200 => Last = Middle - 1 = 0
 *     - Iteración 2: First = 0, Last = 0 => Middle = (0 + 0) \ 2 = 0
 *       IpTables(0) = 100 < 200 => First = Middle + 1 = 1
 *     - Fin bucle: First (1) > Last (0). Bucle termina.
 *     - Variables finales:
 *         First = 1  <-- PUNTO DE INSERCIÓN CORRECTO (First * 2 = 2)
 *         Middle = 0 <-- VALOR RESIDUAL RETENIDO EN LA ÚLTIMA ITERACIÓN
 *     - Retorno literal: Not (Middle * 2) = Not 0 = -1
 *   En invocador:
 *     - index = ~(-1) = 0  (¡DEBIÓ SER 2, PERO CALCULA 0!)
 *     - AddNewIpIntervalo(200, 0):
 *       Desplaza (2 - 0)*8 = 16 bytes (desplaza 100 y 300 hacia índices 2 y 4).
 *       IpTables(0) = 200
 *       MaxValue pasa a 3.
 *   Estado tras Paso 3:
 *     - MaxValue = 3
 *     - IpTables: [200, 100, 300] (en índices 0, 2 y 4)
 *     - ¡EL ARRAY QUEDA COMPLETAMENTE DESORDENADO! 200 quedó antes que 100.
 *
 * -----------------------------------------------------------------------------------------
 * PASO 4: Insertar IP = 400
 *   FindTableIp(400):
 *     - First = 0, Last = MaxValue = 3
 *     - Iteración 1: Middle = (0 + 3) \ 2 = 1
 *       IpTables(2) = 100 < 400 => First = 1 + 1 = 2
 *     - Iteración 2: First = 2, Last = 3 => Middle = (2 + 3) \ 2 = 2
 *       IpTables(4) = 300 < 400 => First = 2 + 1 = 3
 *     - Iteración 3: First = 3, Last = 3 => Middle = (3 + 3) \ 2 = 3
 *       IpTables(6) = 0 < 400 => First = 3 + 1 = 4
 *     - Fin bucle: First (4) > Last (3). Bucle termina.
 *     - Variables finales: First = 4, Middle = 3
 *     - Retorno: Not (Middle * 2) = Not 6 = -7
 *   En invocador:
 *     - index = ~(-7) = 6
 *     - AddNewIpIntervalo(400, 6): desplaza (3 - 3)*8 = 0 bytes.
 *     - IpTables(6) = 400; MaxValue pasa a 4.
 *   Estado tras Paso 4:
 *     - MaxValue = 4
 *     - IpTables: [200, 100, 300, 400] (en índices 0, 2, 4 y 6)
 *
 * -----------------------------------------------------------------------------------------
 * PASO 5: Insertar IP = 150
 *   FindTableIp(150):
 *     - First = 0, Last = MaxValue = 4
 *     - Iteración 1: Middle = (0 + 4) \ 2 = 2
 *       IpTables(4) = 300 > 150 => Last = 2 - 1 = 1
 *     - Iteración 2: First = 0, Last = 1 => Middle = (0 + 1) \ 2 = 0
 *       IpTables(0) = 200 > 150 => Last = 0 - 1 = -1
 *     - Fin bucle: First (0) > Last (-1). Bucle termina.
 *     - Variables finales: First = 0, Middle = 0
 *     - Retorno: Not (Middle * 2) = Not 0 = -1
 *   En invocador:
 *     - index = ~(-1) = 0
 *     - AddNewIpIntervalo(150, 0): desplaza (4 - 0)*8 = 32 bytes (los 4 elementos existentes).
 *       IpTables(0) = 150
 *       MaxValue pasa a 5.
 *   Estado final tras Paso 5:
 *     - MaxValue = 5
 *     - IpTables: [150, 200, 100, 300, 400] (en índices 0, 2, 4, 6 y 8)
 * =========================================================================================
 */

TEST_SUITE("SecurityIp_AntiFlood_Helpers") {

    TEST_CASE("Reproducción fiel del bug de inserción desordenada en FindTableIp (SecurityIp.bas:291)") {
        /**
         * JUSTIFICACIÓN DE LA ASERCIÓN DEL COMPORTAMIENTO ERRÓNEO:
         *
         * En un algoritmo de búsqueda binaria canónico y correcto, al insertar la secuencia
         * [100, 300, 200, 400, 150], el array debería terminar ordenado monótonamente en:
         *   [100, 150, 200, 300, 400]
         *
         * Sin embargo, la directiva fundamental de este proyecto (docs/CONVENTIONS.md y
         * docs/audit/02a-securityip-detalle.md §6.1) es preservar con fidelidad estructural
         * y de comportamiento el código VB6 original sin corregir por cuenta propia los defectos
         * históricos del servidor legacy.
         *
         * En SecurityIp.bas línea 291:
         *   FindTableIp = Not (Middle * 2)
         * Se utiliza la variable `Middle` de la última iteración en lugar del punto de inserción real
         * `First`. Al insertar el tercer elemento (200), la última iteración evaluó Middle = 0 y redujo
         * First a 1. Al retornar Not (0 * 2), la subrutina AddNewIpIntervalo inserta el 200 en el índice 0,
         * empujando al 100 a la derecha y rompiendo la precondición de orden ascendente del array.
         *
         * Por lo tanto, el test C++ DEBE asertar el estado erróneo verbatim [150, 200, 100, 300, 400],
         * validando que la traslación reproduce el comportamiento real del binario VB6 histórico.
         */

        SecurityIp::InitIpTables(1000);

        const std::int32_t ipsToInsert[5] = {100, 300, 200, 400, 150};

        for (int i = 0; i < 5; ++i) {
            std::int32_t ip = ipsToInsert[i];
            std::int32_t ret = SecurityIp::FindTableIp(ip);

            // Cada IP es nueva, por lo que FindTableIp debe retornar un valor negativo
            CHECK(ret < 0);

            std::int32_t insertionIndex = ~ret;
            SecurityIp::AddNewIpIntervalo(ip, insertionIndex);
        }

        CHECK(SecurityIp::MaxValue == 5);

        // Aserciones sobre la disposición exacta predicha por la traza manual:
        CHECK(SecurityIp::IpTables[0] == 150);
        CHECK(SecurityIp::IpTables[2] == 200);
        CHECK(SecurityIp::IpTables[4] == 100);
        CHECK(SecurityIp::IpTables[6] == 300);
        CHECK(SecurityIp::IpTables[8] == 400);
    }

    TEST_CASE("Caso común sin manifestación del bug: inserción secuencial y búsqueda exitosa") {
        /**
         * Para una secuencia estrictamente creciente de 3 IPs [100, 200, 300], cada nueva IP
         * es mayor a todas las anteriores. En cada inserción, la última iteración del bucle
         * evalúa el extremo superior donde First = Middle + 1, y la discrepancia entre Middle y
         * First es absorbida por el hecho de que la posición final del array coincide con la cola,
         * manteniendo los elementos en orden ascendente y permitiendo búsquedas exitosas posteriores.
         */

        SecurityIp::InitIpTables(1000);

        const std::int32_t sequence[3] = {100, 200, 300};

        for (int i = 0; i < 3; ++i) {
            std::int32_t ip = sequence[i];
            std::int32_t ret = SecurityIp::FindTableIp(ip);
            CHECK(ret < 0);
            std::int32_t index = ~ret;
            SecurityIp::AddNewIpIntervalo(ip, index);
        }

        CHECK(SecurityIp::MaxValue == 3);

        // Las IPs quedan en orden ascendente:
        CHECK(SecurityIp::IpTables[0] == 100);
        CHECK(SecurityIp::IpTables[2] == 200);
        CHECK(SecurityIp::IpTables[4] == 300);

        // Verificamos que FindTableIp encuentra exitosamente cada una en su posición:
        CHECK(SecurityIp::FindTableIp(100) == 0);
        CHECK(SecurityIp::FindTableIp(200) == 2);
        CHECK(SecurityIp::FindTableIp(300) == 4);

        // Una IP inexistente retorna un valor negativo:
        CHECK(SecurityIp::FindTableIp(999) < 0);
    }

}

// =========================================================================================
// SUITE DE FUNCIONES PÚBLICAS: InitIpTables, IpSecurityMantenimientoLista, IpSecurityAceptarNuevaConexion
// =========================================================================================

static std::uint32_t s_simulatedTicks = 100000;

static std::uint32_t SimulatedGetTickCount() {
    return s_simulatedTicks;
}

TEST_SUITE("SecurityIp_PublicFunctions") {

    TEST_CASE("InitIpTables inicializa capacidad base y limpia contadores") {
        SecurityIp::InitIpTables(500);

        CHECK(SecurityIp::EntrysCounter == 500);
        CHECK(SecurityIp::Multiplicado == 1);
        CHECK(SecurityIp::MaxValue == 0);
        // Reserva EntrysCounter * 2 + 1 elementos:
        CHECK(SecurityIp::IpTables.size() == 1001);
        CHECK(SecurityIp::IpTables[0] == 0);
    }

    TEST_CASE("IpSecurityAceptarNuevaConexion valida intervalos de conexión y control anti-flood") {
        /**
         * JUSTIFICACIÓN DEL ENFOQUE DE INYECCIÓN DE TIEMPO:
         * Conforme a la Testing Philosophy de docs/CONVENTIONS.md ("Only mock genuine external boundaries:
         * system clock..."), inyectamos un puntero a función estático SimulatedGetTickCount que devuelve
         * ticks deterministas. Esto permite probar casos de borde exactos (delta de 999 ms, 1000 ms y 1001 ms)
         * instantáneamente y sin demorar la suite con llamadas reales a sleep.
         */
        SecurityIp::SetTimeSourceForTesting(SimulatedGetTickCount);

        // Inicializamos tabla para 1000 entradas
        SecurityIp::InitIpTables(1000);

        // Escenario con dos direcciones IP reales en formato numérico (Long de VB6):
        // ipA: 192.168.1.50 -> 0xC0A80132 = -1062731470 como entero con signo de 32 bits
        // ipB: 192.168.1.60 -> 0xC0A8013C = -1062731460 como entero con signo de 32 bits
        const std::int32_t ipA = -1062731470;
        const std::int32_t ipB = -1062731460;

        // T = 100.000 ms: Primera conexión desde ipA -> Debe ser ACEPTADA
        s_simulatedTicks = 100000;
        bool aceptada = SecurityIp::IpSecurityAceptarNuevaConexion(ipA);
        CHECK(aceptada == true);
        CHECK(SecurityIp::MaxValue == 1);

        // Verificamos que el timestamp guardado en IpTables(index + 1) es exactamente 100.000
        std::int32_t idxA = SecurityIp::FindTableIp(ipA);
        CHECK(idxA >= 0);
        CHECK(SecurityIp::IpTables[static_cast<std::size_t>(idxA + 1)] == 100000);

        // T = 100.500 ms (+500 ms): Reintento rápido desde ipA dentro de los 1000 ms -> RECHAZADA
        s_simulatedTicks = 100500;
        CHECK(SecurityIp::IpSecurityAceptarNuevaConexion(ipA) == false);
        // El timestamp en tabla NO debe haberse modificado tras el rechazo:
        CHECK(SecurityIp::IpTables[static_cast<std::size_t>(idxA + 1)] == 100000);

        // T = 100.999 ms (+999 ms): Reintento a un milisegundo antes de expirar -> RECHAZADA
        s_simulatedTicks = 100999;
        CHECK(SecurityIp::IpSecurityAceptarNuevaConexion(ipA) == false);
        CHECK(SecurityIp::IpTables[static_cast<std::size_t>(idxA + 1)] == 100000);

        // T = 101.000 ms (exactamente +1000 ms):
        // CASO DE BORDE DE LÍNEA 111: "IpTables(...) + 1000 <= GetTickCount"
        // 100.000 + 1.000 <= 101.000 es TRUE, por lo que a exactamente 1000 ms es ACEPTADA.
        s_simulatedTicks = 101000;
        CHECK(SecurityIp::IpSecurityAceptarNuevaConexion(ipA) == true);
        // El timestamp debe actualizarse a 101.000
        CHECK(SecurityIp::IpTables[static_cast<std::size_t>(idxA + 1)] == 101000);

        // T = 101.200 ms: ipB intenta conectarse por primera vez -> ACEPTADA independientemente de ipA
        s_simulatedTicks = 101200;
        CHECK(SecurityIp::IpSecurityAceptarNuevaConexion(ipB) == true);
        CHECK(SecurityIp::MaxValue == 2);

        std::int32_t idxB = SecurityIp::FindTableIp(ipB);
        CHECK(idxB >= 0);
        CHECK(SecurityIp::IpTables[static_cast<std::size_t>(idxB + 1)] == 101200);

        // T = 101.400 ms: ipB reintenta a los +200 ms -> RECHAZADA
        s_simulatedTicks = 101400;
        CHECK(SecurityIp::IpSecurityAceptarNuevaConexion(ipB) == false);

        // T = 103.500 ms (> 1000 ms desde 101.000): ipA intenta tras 2.500 ms -> ACEPTADA
        s_simulatedTicks = 103500;
        CHECK(SecurityIp::IpSecurityAceptarNuevaConexion(ipA) == true);
        idxA = SecurityIp::FindTableIp(ipA);
        CHECK(SecurityIp::IpTables[static_cast<std::size_t>(idxA + 1)] == 103500);

        // T = 103.700 ms: ipA reintenta a los +200 ms -> RECHAZADA
        s_simulatedTicks = 103700;
        CHECK(SecurityIp::IpSecurityAceptarNuevaConexion(ipA) == false);

        // ---------------------------------------------------------------------------------
        // MANTENIMIENTO A MITAD DE SECUENCIA:
        // Se ejecuta IpSecurityMantenimientoLista() mientras ipA está en período de rechazo.
        // Confirmamos que purga el estado y que ipA es inmediatamente aceptada nuevamente.
        // ---------------------------------------------------------------------------------
        SecurityIp::IpSecurityMantenimientoLista();
        CHECK(SecurityIp::MaxValue == 0);

        // Inmediatamente al mismo tiempo T = 103.700 ms, ipA vuelve a intentar conectarse.
        // Como la tabla se purgó, debe tratarse como una nueva entrada y ser ACEPTADA:
        CHECK(SecurityIp::IpSecurityAceptarNuevaConexion(ipA) == true);
        CHECK(SecurityIp::MaxValue == 1);

        // Y también ipB en el mismo tick es ACEPTADA como nueva entrada:
        CHECK(SecurityIp::IpSecurityAceptarNuevaConexion(ipB) == true);
        CHECK(SecurityIp::MaxValue == 2);

        // Restauramos la fuente de tiempo
        SecurityIp::ResetTimeSource();
    }

    TEST_CASE("IpSecurityMantenimientoLista restablece capacidad escalonada") {
        SecurityIp::InitIpTables(100);

        // Simulamos que la tabla creció dinámicamente:
        SecurityIp::Multiplicado = 3;
        SecurityIp::EntrysCounter = 300;

        SecurityIp::IpSecurityMantenimientoLista();

        // Debe haber dividido por Multiplicado y reseteado a 1:
        CHECK(SecurityIp::EntrysCounter == 100);
        CHECK(SecurityIp::Multiplicado == 1);
        CHECK(SecurityIp::MaxValue == 0);
        CHECK(SecurityIp::IpTables.size() == 201);
    }

    TEST_CASE("IpSecurityAceptarNuevaConexion reproduce Error 6 de VB6 ante desbordamiento de ticks") {
        /**
         * REPRODUCCIÓN DEL CRASH DE PRODUCCIÓN (Error 6 "Overflow" en VB6):
         *
         * En el binario nativo de VB6 (legacy/server/SERVER.VBP con OverflowCheck=0),
         * la expresión de SecurityIp.bas:111:
         *   IpTables(IpTableIndex + 1) + IntervaloEntreConexiones <= GetTickCount
         * realizaba una suma con signo de 32 bits (Long).
         *
         * Cuando el timestamp almacenado supera INT32_MAX - IntervaloEntreConexiones
         * (2.147.483.647 - 1.000 = 2.147.482.647, aprox. 24.85 días de uptime de GetTickCount),
         * la suma excede el rango de Long con signo y lanza el Error 6 en tiempo de ejecución.
         *
         * En C++, el desbordamiento de enteros con signo es Undefined Behavior (UB). Para emular
         * el crash con total fidelidad sin incurrir en UB, se implementó un pre-check que lanza
         * SecurityIp::TickCountOverflowException garantizando además la invariabilidad del estado interno.
         */
        SecurityIp::SetTimeSourceForTesting(SimulatedGetTickCount);
        SecurityIp::InitIpTables(100);

        const std::int32_t ip = 123456;
        constexpr std::int32_t maxSafeTick = std::numeric_limits<std::int32_t>::max() - SecurityIp::IntervaloEntreConexiones; // 2147482647

        // 1. LÍMITE EXACTO SEGURO: INT32_MAX - IntervaloEntreConexiones
        // Primera conexión con timestamp seguro en el límite exacto
        s_simulatedTicks = static_cast<std::uint32_t>(maxSafeTick);
        CHECK(SecurityIp::IpSecurityAceptarNuevaConexion(ip) == true);
        CHECK(SecurityIp::MaxValue == 1);

        std::int32_t idx = SecurityIp::FindTableIp(ip);
        CHECK(idx >= 0);
        CHECK(SecurityIp::IpTables[static_cast<std::size_t>(idx + 1)] == maxSafeTick);

        // Reintento de conexión cuando el reloj alcanza INT32_MAX exacto:
        // maxSafeTick + 1000 = INT32_MAX. La suma no desborda y debe evaluarse sin lanzar.
        s_simulatedTicks = static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max());
        CHECK_NOTHROW(SecurityIp::IpSecurityAceptarNuevaConexion(ip));
        CHECK(SecurityIp::IpTables[static_cast<std::size_t>(idx + 1)] == std::numeric_limits<std::int32_t>::max());

        // 2. DESBORDAMIENTO: INT32_MAX - IntervaloEntreConexiones + 1 (2147482648)
        // Forzamos el timestamp almacenado al primer valor que causa desbordamiento:
        SecurityIp::IpTables[static_cast<std::size_t>(idx + 1)] = maxSafeTick + 1;

        // Captura del estado interno para verificar invariabilidad (Strong Exception Guarantee)
        const auto ipTablesSnapshot = SecurityIp::IpTables;
        const auto maxValueSnapshot = SecurityIp::MaxValue;
        const auto entrysCounterSnapshot = SecurityIp::EntrysCounter;
        const auto multiplicadoSnapshot = SecurityIp::Multiplicado;

        // Debe lanzar TickCountOverflowException de forma obligatoria y tipada
        CHECK_THROWS_AS(SecurityIp::IpSecurityAceptarNuevaConexion(ip), SecurityIp::TickCountOverflowException);

        // Validación del código de error legacy original de VB6 (Error 6 "Overflow")
        try {
            SecurityIp::IpSecurityAceptarNuevaConexion(ip);
        } catch (const SecurityIp::TickCountOverflowException& ex) {
            CHECK(ex.getErrorCode() == SecurityIp::ERR_OVERFLOW);
        }

        // 3. INVARIANZA DE ESTADO: Ningún contenedor ni variable interna debe haber mutado tras la excepción
        CHECK(SecurityIp::IpTables == ipTablesSnapshot);
        CHECK(SecurityIp::MaxValue == maxValueSnapshot);
        CHECK(SecurityIp::EntrysCounter == entrysCounterSnapshot);
        CHECK(SecurityIp::Multiplicado == multiplicadoSnapshot);

        SecurityIp::ResetTimeSource();
    }

}
