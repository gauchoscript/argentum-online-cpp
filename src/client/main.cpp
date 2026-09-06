#include <asio.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <array>

using asio::ip::tcp;

int main() {
    // Verificación de coexistencia con SFML
    std::cout << "[Client] SFML Header OK. Version: "
              << SFML_VERSION_MAJOR << "."
              << SFML_VERSION_MINOR << "."
              << SFML_VERSION_PATCH << std::endl;

    sf::Color bg_color(100, 150, 200);
    std::cout << "[Client] SFML Graphics OK (sf::Color R="
              << static_cast<int>(bg_color.r) << ")" << std::endl;

    try {
        asio::io_context io_context;

        tcp::resolver resolver(io_context);
        asio::error_code ec;

        // Resolver direccion localhost:5000
        auto endpoints = resolver.resolve("127.0.0.1", "5000", ec);
        if (ec) {
            std::cerr << "[Client] Error al resolver direccion: " << ec.message() << std::endl;
            return 1;
        }

        tcp::socket socket(io_context);

        // Intentar conectar al servidor
        std::cout << "[Client] Conectando al servidor en 127.0.0.1:5000..." << std::endl;
        asio::connect(socket, endpoints, ec);

        if (ec) {
            std::cerr << "[Client] No se pudo conectar al servidor: " << ec.message() << std::endl;
            std::cerr << "[Client] Verificad que el servidor (server_poc) este ejecutandose primero." << std::endl;
            return 1;
        }

        std::cout << "[Client] Conectado exitosamente al servidor!" << std::endl;

        // Mensaje de prueba hardcodeado
        std::string message = "hello server";
        std::cout << "[Client] Enviando mensaje: \"" << message << "\"" << std::endl;

        asio::write(socket, asio::buffer(message), ec);
        if (ec) {
            std::cerr << "[Client] Error al enviar mensaje: " << ec.message() << std::endl;
            return 1;
        }

        // Leer la respuesta en eco del servidor
        std::array<char, 1024> buffer;
        size_t bytes_transferred = socket.read_some(asio::buffer(buffer), ec);

        if (ec && ec != asio::error::eof) {
            std::cerr << "[Client] Error al recibir eco: " << ec.message() << std::endl;
            return 1;
        }

        std::string echo_message(buffer.data(), bytes_transferred);
        std::cout << "[Client] Eco recibido del servidor: \"" << echo_message << "\"" << std::endl;

        // Cerrar la conexion y salir
        asio::error_code ec_shutdown;
        socket.shutdown(tcp::socket::shutdown_both, ec_shutdown);
        socket.close(ec_shutdown);
        std::cout << "[Client] Conexion cerrada. Saliendo..." << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Client] Excepcion no controlada: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}


