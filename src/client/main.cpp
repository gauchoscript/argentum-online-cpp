#include <asio.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <string>
#include <array>
#include <sstream>
#include <optional>

using asio::ip::tcp;

int main() {
    try {
        std::cout << "[Client] Conectando al servidor 127.0.0.1:5000..." << std::endl;

        asio::io_context io_context;
        tcp::resolver resolver(io_context);
        asio::error_code ec;

        auto endpoints = resolver.resolve("127.0.0.1", "5000", ec);
        if (ec) {
            std::cerr << "[Client] Error al resolver servidor: " << ec.message() << std::endl;
            return 1;
        }

        tcp::socket socket(io_context);
        asio::connect(socket, endpoints, ec);
        if (ec) {
            std::cerr << "[Client] No se pudo conectar al servidor: " << ec.message() << std::endl;
            return 1;
        }

        std::cout << "[Client] Conectado exitosamente al servidor!" << std::endl;

        // Configurar socket en modo no bloqueante para no congelar la ventana SFML
        socket.non_blocking(true, ec);
        if (ec) {
            std::cerr << "[Client] Error al configurar socket no bloqueante: " << ec.message() << std::endl;
            return 1;
        }

        // Crear ventana SFML 3 (800x600, titulo "AO Migration - Render POC")
        sf::RenderWindow window(sf::VideoMode({800u, 600u}), "AO Migration - Render POC");
        window.setFramerateLimit(60);

        if (!window.isOpen()) {
            std::cerr << "[Client] Error: sf::RenderWindow no pudo inicializarse correctamente." << std::endl;
            return 1;
        }

        // Crear circulo en posicion inicial (400, 300)
        sf::CircleShape shape(40.0f);
        shape.setFillColor(sf::Color(0, 200, 255)); // Azul cian
        shape.setOrigin({40.0f, 40.0f});
        shape.setPosition({400.0f, 300.0f});

        std::cout << "[Client] Ventana iniciada. Renderizando según mensajes del servidor..." << std::endl;

        std::string incoming_buffer;
        std::array<char, 256> read_buffer;

        // Bucle de renderizado y eventos
        while (window.isOpen()) {
            // 1. Manejo de eventos SFML 3
            while (const std::optional event = window.pollEvent()) {
                if (event->is<sf::Event::Closed>()) {
                    window.close();
                } else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                    if (keyPressed->code == sf::Keyboard::Key::Escape) {
                        window.close();
                    }
                }
            }

            // 2. Lectura no bloqueante del socket
            size_t bytes_read = socket.read_some(asio::buffer(read_buffer), ec);
            if (!ec && bytes_read > 0) {
                incoming_buffer.append(read_buffer.data(), bytes_read);

                // Procesar comandos terminados en newline
                size_t pos;
                while ((pos = incoming_buffer.find('\n')) != std::string::npos) {
                    std::string line = incoming_buffer.substr(0, pos);
                    incoming_buffer.erase(0, pos + 1);

                    if (!line.empty() && line.back() == '\r') {
                        line.pop_back();
                    }

                    if (line.rfind("DRAW ", 0) == 0) {
                        std::istringstream iss(line.substr(5));
                        int x, y;
                        if (iss >> x >> y) {
                            shape.setPosition({static_cast<float>(x), static_cast<float>(y)});
                            std::cout << "[Client] Recibido comando: DRAW " << x << " " << y 
                                      << " -> Posicion actualizada." << std::endl;
                        }
                    }
                }
            } else if (ec && ec != asio::error::would_block && ec != asio::error::try_again) {
                std::cout << "[Client] Desconectado del servidor (" << ec.message() << ")." << std::endl;
            }

            // 3. Renderizado
            window.clear(sf::Color(20, 24, 32));
            window.draw(shape);
            window.display();
        }

        // Cerrar socket limpiamente al salir
        asio::error_code ec_shutdown;
        socket.shutdown(tcp::socket::shutdown_both, ec_shutdown);
        socket.close(ec_shutdown);
        std::cout << "[Client] Ventana cerrada limpiamente. Saliendo..." << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "[Client] Error fatal: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}




