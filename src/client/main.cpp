#include <asio.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <array>
#include <optional>

using asio::ip::tcp;

int main() {
    try {
        std::cout << "[Client] SFML Header OK. Version: "
                  << SFML_VERSION_MAJOR << "."
                  << SFML_VERSION_MINOR << "."
                  << SFML_VERSION_PATCH << std::endl;

        // Intento opcional de conexión con el servidor POC si estuviera activo
        try {
            asio::io_context io_context;
            tcp::resolver resolver(io_context);
            asio::error_code ec;

            auto endpoints = resolver.resolve("127.0.0.1", "5000", ec);
            if (!ec) {
                tcp::socket socket(io_context);
                asio::connect(socket, endpoints, ec);
                if (!ec) {
                    std::cout << "[Client] Conectado exitosamente al servidor!" << std::endl;
                    std::string message = "hello server";
                    asio::write(socket, asio::buffer(message), ec);
                    std::array<char, 1024> buffer;
                    size_t bytes_transferred = socket.read_some(asio::buffer(buffer), ec);
                    if (!ec || ec == asio::error::eof) {
                        std::cout << "[Client] Eco recibido del servidor: \"" 
                                  << std::string(buffer.data(), bytes_transferred) << "\"" << std::endl;
                    }
                    asio::error_code ec_shutdown;
                    socket.shutdown(tcp::socket::shutdown_both, ec_shutdown);
                    socket.close(ec_shutdown);
                } else {
                    std::cout << "[Client] No se detecto servidor ejecutandose (modo standalone)." << std::endl;
                }
            }
        } catch (const std::exception& e) {
            std::cout << "[Client] Excepcion en red (continuando en modo standalone): " << e.what() << std::endl;
        }

        // Crear ventana SFML 3 (800x600, titulo "AO Migration - Render POC")
        sf::RenderWindow window(sf::VideoMode({800u, 600u}), "AO Migration - Render POC");
        window.setFramerateLimit(60);

        if (!window.isOpen()) {
            std::cerr << "[Client] Error: sf::RenderWindow no pudo inicializarse correctamente." << std::endl;
            return 1;
        }

        // Crear un circulo en el centro de la ventana
        sf::CircleShape shape(60.0f);
        shape.setFillColor(sf::Color(0, 200, 255)); // Azul cian
        shape.setOrigin({60.0f, 60.0f});
        shape.setPosition({400.0f, 300.0f});

        std::cout << "[Client] Ventana iniciada. Esperando eventos..." << std::endl;

        // Bucle de eventos usando la sintaxis de SFML 3 (pollEvent devuelve std::optional<sf::Event>)
        while (window.isOpen()) {
            while (const std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                    if (keyPressed->code == sf::Keyboard::Key::Escape) {
                        window.close();
                    }
                }
            }

            window.clear(sf::Color(20, 24, 32));
            window.draw(shape);
            window.display();
        }

        std::cout << "Window closed cleanly" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[Client] Error fatal: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}



