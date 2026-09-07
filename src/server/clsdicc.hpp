#ifndef CLSDICC_HPP
#define CLSDICC_HPP

#include <string>
#include <unordered_map>
#include <vector>
#include <cstdint>

/*
 * clsdicc - Adaptación C++ de la clase diccionario del servidor VB6 (legacy/server/Codigo/clsdicc.cls)
 *
 * NOTAS DE COMPORTAMIENTO Y COMPATIBILIDAD LEGACY:
 *
 * 1. Insensibilidad a mayúsculas/minúsculas (Case-Insensitive):
 *    Al igual que en VB6 (que utiliza UCase$), todas las claves se normalizan automáticamente a mayúsculas
 *    tanto al insertar (AtPut) como al consultar (At).
 *
 * 2. Límite de 100 elementos (Desviación Intencional Documentada):
 *    El código legacy VB6 tenía un arreglo estático con un cap rígido de 100 elementos (MAX_ELEM = 100).
 *    En C++ se utiliza std::unordered_map con std::vector para crecimiento dinámico sin tope artificial de 100.
 *    Esta omisión es una desviación intencional documentada (ver docs/implementation/01a-clsdicc-cgarbage.md
 *    y docs/implementation/04-clsdicc.md).
 *
 * 3. Preservación del orden de inserción y comportamiento de empates en MayorValor:
 *    El método MayorValor recorre los elementos en su ORDEN DE INSERCIÓN original.
 *    En caso de empate en la puntuación máxima:
 *      - Devuelve TODAS las claves empatadas concatenadas por comas (ej. "CLAVE1,CLAVE2,CLAVE3").
 *      - Conserva el orden de inserción (NO se ordenan alfabéticamente).
 *      - Asigna al parámetro de salida 'cant' la cantidad total de claves empatadas.
 *      - Las claves en el string retornado están normalizadas en MAYÚSCULAS.
 */

class clsdicc {
public:
    clsdicc() = default;
    ~clsdicc() = default;

    // Devuelve la cantidad de elementos almacenados en el diccionario
    std::int16_t CantElem() const;

    // Asigna o actualiza una clave con su valor. Retorna false si la clave está vacía.
    bool AtPut(const std::string& clave, const std::string& elem);
    bool AtPut(const std::string& clave, int elem);

    // Consulta el valor asociado a la clave (case-insensitive). Retorna "" si no existe.
    std::string At(const std::string& clave) const;

    // Consulta la clave almacenada en la posición de índice i (1-based, orden de inserción).
    std::string AtIndex(int i) const;

    // Determina la(s) clave(s) con el valor numérico más alto y la cantidad de claves empatadas.
    std::string MayorValor(int& cant) const;

    // Vacía todos los elementos del diccionario.
    void DumpAll();

private:
    std::unordered_map<std::string, std::string> m_elementos;
    std::vector<std::string> m_order;
};

#endif // CLSDICC_HPP
