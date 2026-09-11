#ifndef TCP_HPP
#define TCP_HPP

#include <cstdint>
#include <string>
#include <string_view>
#include <functional>

/**
 * @file TCP.hpp
 * @brief Subsistema de red y utilidades de sockets (TCP.bas, wskapiAO.bas, wsksock.bas).
 *
 * Módulo #14 del servidor Argentum Online v0.13.0 (Capa 4 - Infraestructura de Red).
 * Paso 1 (G1): Utilidades de formato y conversión de direcciones IP.
 */
namespace TCP {

/**
 * @brief Convierte un entero IPv4 de 32 bits en network byte order a cadena de texto punteada.
 *
 * Transliteración y reemplazo portable de GetAscIP de wsksock.bas (líneas 527-545).
 * Si la dirección es 0xFFFFFFFF (INADDR_NONE) o inválida, devuelve "255.255.255.255".
 *
 * @param ip Dirección IPv4 en network byte order.
 * @return Representación en formato "a.b.c.d".
 */
std::string GetAscIP(std::uint32_t ip);

/**
 * @brief Convierte una cadena de texto IPv4 punteada a un entero de 32 bits en network byte order.
 *
 * Transliteración y reemplazo portable de GetLongIp de wsksock.bas (líneas 801-806).
 * Si la cadena no es una dirección IPv4 válida, devuelve 0.
 *
 * @param ip Cadena de texto en formato "a.b.c.d".
 * @return Dirección IPv4 de 32 bits en network byte order o 0 si es inválida.
 */
std::uint32_t GetLongIp(std::string_view ip);

/**
 * @brief Valida estrictamente si una cadena de texto representa una dirección IPv4 válida.
 *
 * Comprueba que la cadena posea exactamente 4 componentes numéricos en el rango [0, 255]
 * separados por puntos, sin caracteres espurios ni espacios en blanco.
 *
 * @param ip Cadena de texto a evaluar.
 * @return true si es una dirección IPv4 válida, false en caso contrario.
 */
bool IsValidIP(std::string_view ip);

// ============================================================================
// G2: Gestión de Slots de Usuario y Mapeo de Sockets
// ============================================================================

/**
 * @brief Busca el siguiente slot libre disponible en UserList.
 *
 * Transliteración de NextOpenUser de Modulo_UsUaRiOs.bas (líneas 868-883).
 * Evalúa secuencialmente los slots de 1 a MaxUsers buscando un usuario con
 * ConnID == -1 y flags.UserLogged == false.
 *
 * @return Índice del slot libre (1..MaxUsers) o MaxUsers + 1 si el servidor está lleno.
 */
std::int16_t NextOpenUser();

/**
 * @brief Blanquea y resetea el estado de un slot de usuario en memoria.
 *
 * Transliteración de ResetUserSlot de TCP.bas (líneas 1719-1756).
 *
 * @param user_index Índice del slot a blanquear (1..MaxUsers).
 */
void ResetUserSlot(std::int16_t user_index);

/**
 * @brief Busca el slot de UserList asociado a un identificador numérico de socket.
 *
 * Transliteración de BuscaSlotSock de wskapiAO.bas (líneas 130-142).
 *
 * @param sock_id Identificador numérico único del socket.
 * @return Índice del slot de usuario (1..MaxUsers), o -1 si no existe asociación.
 */
std::int16_t BuscaSlotSock(std::uint64_t sock_id);

/**
 * @brief Registra o actualiza la asociación entre un socket y un slot de usuario.
 *
 * Transliteración de AgregaSlotSock de wskapiAO.bas (líneas 144-186).
 *
 * @param sock_id Identificador numérico del socket.
 * @param slot Índice del slot asignado en UserList.
 */
void AgregaSlotSock(std::uint64_t sock_id, std::int16_t slot);

/**
 * @brief Elimina la asociación de un socket del mapeo asociativo.
 *
 * Transliteración de BorraSlotSock de wskapiAO.bas (líneas 188-199).
 *
 * @param sock_id Identificador numérico del socket a remover.
 */
void BorraSlotSock(std::uint64_t sock_id);

/**
 * @brief Limpia la totalidad del mapa asociativo de sockets.
 *
 * Utilidad auxiliar para pruebas unitarias y reinicio del subsistema de red.
 */
void ResetSockSlots();

// ============================================================================
// G3: Listener Asio y Aceptación de Conexiones
// ============================================================================

/**
 * @brief Inicia el acceptor TCP de Asio escuchando en el puerto y dirección indicados.
 *
 * Transliteración moderna de ListenForConnect de wsksock.bas (líneas 834-896).
 * Configura SO_REUSEADDR, enlaza al endpoint indicado y comienza la aceptación asíncrona.
 *
 * @param puerto Puerto local en el que escuchar (0 para puerto efímero asignado por el SO).
 * @param bind_ip Dirección IP local a la que enlazar ("" o vacía para INADDR_ANY / 0.0.0.0).
 * @return true si el listener se inició con éxito, false ante error de enlace o socket.
 */
bool IniciaServidor(std::uint16_t puerto, std::string_view bind_ip = "");

/**
 * @brief Detiene el servidor, cerrando el acceptor, cancelando operaciones y liberando sockets.
 */
void DetenerServidor();

/**
 * @brief Retorna el puerto local efectivo en el que está escuchando el acceptor.
 *
 * Útil para pruebas unitarias cuando se inicia en el puerto 0.
 *
 * @return Puerto local o 0 si el servidor no está escuchando.
 */
std::uint16_t PuertoEscucha();

/**
 * @brief Procesa de forma no bloqueante todos los eventos de red listos en el frame actual.
 *
 * Invoca síncronamente io_context::poll() en el hilo invocante (modelo monohilo).
 */
void PollRed();

// ============================================================================
// G4: Recepción Asíncrona de Datos
// ============================================================================

using PacketHandler = std::function<void(std::int16_t)>;

/**
 * @brief Inicia la recepción continua de datos para el socket del slot de usuario indicado.
 *
 * Transliteración de wskapiAO.bas (líneas 255-275).
 * Dispara socket.async_read_some() asociando el buffer de recepción SIZE_RCVBUF (8 KB).
 *
 * @param user_index Índice del usuario en UserList.
 */
void IniciarLectura(std::int16_t user_index);

/**
 * @brief Registra opcionalmente un callback que se invocará al recibir nuevos paquetes en incomingData.
 *
 * @param handler Callback invocado con el índice de slot del usuario que recibió datos.
 */
void SetPacketHandler(PacketHandler handler);

// ============================================================================
// G5: Envío de Datos, Buffers Salientes y Mitigación de Backpressure
// ============================================================================

constexpr std::size_t MAX_OUTGOING_BUFFER_SIZE = 65536; // 64 KB

/**
 * @brief Encola datos para su transmisión asíncrona hacia el socket del usuario.
 *
 * Transliteración moderna de EnviarDatosASlot (TCP.bas:822-843) y WsApiEnviar (wskapiAO.bas:314-352).
 * Implementa mitigación del Bug #19 mediante backpressure seguro: si los datos encolados
 * superan MAX_OUTGOING_BUFFER_SIZE (64 KB), la conexión se cancela y se cierra preventivamente.
 *
 * @param user_index Índice del usuario en UserList.
 * @param datos Vista o cadena de datos a transmitir.
 */
void EnviarDatosASlot(std::int16_t user_index, std::string_view datos);

/**
 * @brief Vacía el buffer de salida outgoingData de UserList(user_index) enviándolo al socket.
 *
 * Transliteración de FlushBuffer de Protocol.bas (líneas 17233-17249).
 *
 * @param user_index Índice del usuario en UserList.
 */
void FlushBuffer(std::int16_t user_index);

// ============================================================================
// G6: Ciclo de Vida de Desconexión y Mecánica Anti-CombatLog
// ============================================================================

using CloseUserHandler = std::function<void(std::int16_t)>;

/**
 * @brief Registra un handler opcional invocado al ejecutar CloseUser / deslogueo del personaje.
 *
 * @param handler Función que recibe el user_index para persistencia o limpieza de mapa.
 */
void SetCloseUserHandler(CloseUserHandler handler);

/**
 * @brief Cierra el socket físico TCP en Asio sin limpiar el slot ni desloguear el personaje.
 *
 * Transliteración de CloseSocketSL de TCP.bas (líneas 782-795).
 * El personaje permanece activo y visible en el mundo de juego (mecánica Anti-CombatLog).
 *
 * @param user_index Índice del usuario en UserList.
 */
void CloseSocketSL(std::int16_t user_index);

/**
 * @brief Inicia el proceso de salida de un usuario (cuenta regresiva o salida instantánea).
 *
 * Transliteración de Cerrar_Usuario de Modulo_UsUaRiOs.bas (líneas 1850-1897).
 * En zonas seguras asigna 0 segundos; en mapas PK asigna IntervaloCerrarConexion (10 segundos).
 * Desactiva inmediatamente la invisibilidad y el ocultamiento para evitar abusos tácticos.
 *
 * @param user_index Índice del usuario en UserList.
 */
void CerrarUsuario(std::int16_t user_index);

/**
 * @brief Realiza la desconexión y liberación definitiva del slot de usuario.
 *
 * Transliteración de CloseSocket de TCP.bas (líneas 610-672).
 * Cierra el socket físico si aún está activo, persiste el personaje, actualiza NumUsers
 * y blanquea el slot mediante ResetUserSlot.
 *
 * @param user_index Índice del usuario en UserList.
 */
void CloseSocket(std::int16_t user_index);

/**
 * @brief Decrementa el temporizador de salida de los usuarios cada segundo (Game Loop).
 *
 * Transliteración de PasarSegundoUsuarios de General.bas (líneas 1555-1569).
 * Al expirar la cuenta regresiva (<= 0), invoca CloseSocket.
 */
void PasarSegundoUsuarios();

// ============================================================================
// G7: Diagnóstico y Cierre de Módulo (WSApiReiniciarSockets e Integración Global)
// ============================================================================

/**
 * @brief Reinicia por completo el subsistema de sockets y red.
 *
 * Transliteración de WSApiReiniciarSockets de wskapiAO.bas (líneas 542-582).
 * Cierra todas las conexiones activas en UserList, detiene el acceptor de Asio
 * y vuelve a iniciar el listener en el mismo puerto configurado.
 */
void WSApiReiniciarSockets();

} // namespace TCP

#endif // TCP_HPP
