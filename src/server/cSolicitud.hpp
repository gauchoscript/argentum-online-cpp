#pragma once

#include <string>

/**
 * @brief Transliteración 1:1 de legacy/server/Codigo/cSolicitud.cls.
 * 
 * Contenedor de datos plano (DTO) para solicitudes de ingreso a clanes.
 * No posee métodos ni lógica interna en la base de código legacy VB6, por lo que
 * se implementa como un struct plano (idéntico al criterio aplicado en cGarbage).
 * 
 * Nota sobre identificadores: Aunque en los archivos INI .sol de clanes las claves
 * guardadas en disco se denominan "Nombre" y "Detalle", la clase legacy VB6 cSolicitud.cls
 * declara explícitamente los miembros públicos 'UserName' y 'desc'. Siguiendo la Naming Policy,
 * se conservan estos identificadores exactos.
 */
struct cSolicitud {
    std::string UserName;
    std::string desc;
};
