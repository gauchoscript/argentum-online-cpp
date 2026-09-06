#include <asio.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <chrono>

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

        int x = 100;
        int y = 300;
        int dx = 50;

        while (true) {
            std::string message = "DRAW " + std::to_string(x) + " " + std::to_string(y) + "\n";
            asio::error_code error;
            asio::write(socket, asio::buffer(message), error);

            if (error) {
                std::cout << "[Server] Cliente desconectado o error de socket: " << error.message() << std::endl;
                break;
            }

            std::cout << "[Server] Enviado: " << message << std::flush;

            // Patron de rebote simple en el rango 100-700
            x += dx;
            if (x >= 700) {
                x = 700;
                dx = -50;
            } else if (x <= 100) {
                x = 100;
                dx = 50;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

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


