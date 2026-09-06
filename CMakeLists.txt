cmake_minimum_required(VERSION 3.20)
project(ArgentumOnline VERSION 0.13.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()


# Find dependencies provided by vcpkg
find_package(asio CONFIG REQUIRED)
find_package(SFML CONFIG REQUIRED COMPONENTS Graphics Window System Network Audio)

# Server executable target
add_executable(server_poc src/server/main.cpp)
target_link_libraries(server_poc PRIVATE asio::asio)
if(WIN32)
    target_link_libraries(server_poc PRIVATE ws2_32 mswsock)
endif()

# Client executable target
add_executable(client_poc src/client/main.cpp)
target_link_libraries(client_poc PRIVATE asio::asio SFML::Graphics SFML::Window SFML::System)
if(WIN32)
    target_link_libraries(client_poc PRIVATE ws2_32 mswsock)
endif()

