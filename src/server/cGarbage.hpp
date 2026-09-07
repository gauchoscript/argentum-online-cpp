#pragma once

#include <cstdint>

/**
 * @file cGarbage.hpp
 * @brief Estructura DTO para registrar la ubicacion de objetos temporales del mapa.
 *
 * NOTA DE NOMENCLATURA (Misnomer de VB6):
 * Se preservan los nombres engañosos cGarbage / TrashCollector segun la politica de nombres de docs/CONVENTIONS.md.
 * A pesar de sus nombres, esto NO es un recolector de basura de memoria ni gestiona ciclos de referencias;
 * su funcion real es registrar la ubicacion en el mapa de objetos temporales (como fogatas encendidas por jugadores
 * mediante la habilidad Supervivencia en Acciones.bas) para su posterior eliminacion periodica (rutina LimpiarMundo
 * en General.bas).
 *
 * No unit tests yet — this struct's real behavior is exercised by Acciones.bas (creation) and General.bas (cleanup), neither ported yet. Integration testing for this struct is owed when those modules are ported (see docs/implementation/00-port-plan.md for their scheduled position).
 */
struct cGarbage {
    std::int16_t map{0};
    std::int16_t X{0};
    std::int16_t Y{0};
};
