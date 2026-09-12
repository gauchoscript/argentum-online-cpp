cmake_minimum_required(VERSION 3.20)
project(ArgentumOnline VERSION 0.13.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(WIN32)
    add_compile_definitions(_WIN32_WINNT=0x0601)
endif()

# Find dependencies provided by vcpkg
find_package(asio CONFIG REQUIRED)
find_package(doctest CONFIG REQUIRED)
find_package(SFML CONFIG REQUIRED COMPONENTS Graphics Window System Network Audio)

# Server core static library
add_library(server_core STATIC
    src/server/Matematicas.cpp
    src/server/Declares.cpp
    src/server/clsIniReader.cpp
    src/server/clsdicc.cpp
    src/server/cGarbage.hpp
    src/server/ModCola.cpp
    src/server/modHexaStrings.cpp
    src/server/cSolicitud.hpp
    src/server/clsByteQueue.cpp
    src/server/FileIO.cpp
    src/server/clsClan.cpp
    src/server/modGuilds.cpp
    src/server/SecurityIp.cpp
    src/server/TCP.cpp
    src/server/modSendData.cpp
)
target_include_directories(server_core PUBLIC src)
target_link_libraries(server_core PUBLIC asio::asio)
if(WIN32)
    target_link_libraries(server_core PUBLIC ws2_32 mswsock)
endif()

# Server executable target
add_executable(server_poc src/server/main.cpp)
target_link_libraries(server_poc PRIVATE server_core)

# Client executable target
add_executable(client_poc src/client/main.cpp)
target_link_libraries(client_poc PRIVATE asio::asio SFML::Graphics SFML::Window SFML::System)
if(WIN32)
    target_link_libraries(client_poc PRIVATE ws2_32 mswsock)
endif()

# Unit tests target
enable_testing()
add_executable(unit_tests
    tests/test_matematicas.cpp
    tests/test_clsinireader.cpp
    tests/test_clsdicc.cpp
    tests/test_modcola.cpp
    tests/test_modhexastrings.cpp
    tests/test_clsbytequeue.cpp
    tests/test_fileio_charfile.cpp
    tests/test_fileio_serverconfig.cpp
    tests/test_fileio_gamedata.cpp
    tests/test_fileio_map.cpp
    tests/test_fileio_adminlogs.cpp
    tests/test_fileio_backup.cpp
    tests/test_clsclan.cpp
    tests/test_securityip.cpp
    tests/test_tcp.cpp
    tests/test_modsenddata.cpp
)
target_include_directories(unit_tests PRIVATE src)
target_link_libraries(unit_tests PRIVATE server_core doctest::doctest)

add_test(NAME unit_tests COMMAND unit_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
