#include <asio.hpp>
#include <iostream>
#include <string>
#include <array>

using asio::ip::tcp;

int main() {
    try {
        asio::io_context io_context;

        // Escuchar en el puerto 5000 (IPv4)
        unsigned short port = 5000;
        tcp::acceptor acceptor(io_context, tcp::endpoint(tcp::v4(), port));
        std::cout << "[Server] Esperando conexion entrante en el puerto " << port << "..." << std::endl;

        tcp::socket socket(io_context);
        acceptor.accept(socket);
        std::cout << "[Server] Cliente conectado desde: " << socket.remote_endpoint() << std::endl;

        // Leer mensaje del cliente
        std::array<char, 1024> buffer;
        asio::error_code error;
        size_t bytes_transferred = socket.read_some(asio::buffer(buffer), error);

        if (error && error != asio::error::eof) {
            std::cerr << "[Server] Error al leer del socket: " << error.message() << std::endl;
            return 1;
        }

        std::string received_message(buffer.data(), bytes_transferred);
        std::cout << "[Server] Mensaje recibido del cliente: \"" << received_message << "\"" << std::endl;

        // Reenviar (echo) el mismo texto al cliente
        asio::write(socket, asio::buffer(received_message), error);
        if (error) {
            std::cerr << "[Server] Error al enviar eco: " << error.message() << std::endl;
            return 1;
        }
        std::cout << "[Server] Eco enviado correctamente al cliente." << std::endl;

        // Cerrar la conexion y salir
        asio::error_code ec_shutdown;
        socket.shutdown(tcp::socket::shutdown_both, ec_shutdown);
        socket.close(ec_shutdown);
        std::cout << "[Server] Conexion cerrada. Saliendo..." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Server] Excepcion no controlada: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

