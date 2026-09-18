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
    src/server/Protocol.cpp
    src/server/ModAreas.cpp
    src/server/Modulo_InventANDobj.cpp
    src/server/InvUsuario.cpp
    src/server/modBanco.cpp
    src/server/Comercio.cpp
    src/server/mdlCOmercioConUsuario.cpp
    src/server/SistemaCombate.cpp
    src/server/modHechizos.cpp
    src/server/ModFacciones.cpp
    src/server/Trabajo.cpp
    src/server/PathFinding.cpp
    src/server/MODULO_NPCs.cpp
    src/server/AI_NPC.cpp
    src/server/praetorians.cpp
    src/server/clsParty.cpp
    src/server/mdParty.cpp
    src/server/Acciones.cpp
    src/server/Characters.cpp
    src/server/Modulo_UsUaRiOs.cpp
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
    tests/test_protocol.cpp
    tests/test_modareas.cpp
    tests/test_modulo_inventandobj.cpp
    tests/test_invusuario.cpp
    tests/test_modbanco.cpp
    tests/test_comercio.cpp
    tests/test_comercio_usuario.cpp
    tests/test_sistemacombate.cpp
    tests/test_modhechizos.cpp
    tests/test_modfacciones.cpp
    tests/test_trabajo.cpp
    tests/test_pathfinding.cpp
    tests/test_modulo_npcs.cpp
    tests/test_ai_npc.cpp
    tests/test_praetorians.cpp
    tests/test_party.cpp
    tests/test_acciones.cpp
    tests/test_characters.cpp
    tests/test_modulo_usuarios.cpp
)

target_include_directories(unit_tests PRIVATE src)
target_link_libraries(unit_tests PRIVATE server_core doctest::doctest)

add_test(NAME unit_tests COMMAND unit_tests WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
