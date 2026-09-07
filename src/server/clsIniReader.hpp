#ifndef CLSINIREADER_HPP
#define CLSINIREADER_HPP

#include <string>
#include <vector>
#include <cstdint>

/*
 * clsIniReader - Adaptación C++ del módulo legacy VB6 clsIniReader.cls
 *
 * AUDITORÍA DE COMPORTAMIENTO LEGACY (VB6):
 * 1. Codificación ANSI / Windows-1252: El código legacy asume que los archivos INI
 *    están guardados en codificación mono-byte ANSI (Windows-1252).
 * 2. Búsqueda insensible a mayúsculas/minúsculas (Case-Insensitive): Las secciones y las
 *    claves se transforman a mayúsculas (vía UCase$) al ser leídas e indexadas, por lo
 *    que las búsquedas con FindMain y FindKey son completamente insensibles a
 *    mayúsculas/minúsculas.
 * 3. Preservación del Valor: Aunque la sección y la clave se convierten a UCase$,
 *    el valor (Value) de la entrada preserva su formato y mayúsculas/minúsculas original.
 * 4. Quirk de KeyExists: La función KeyExists(name) del legacy VB6 verifica si existe
 *    la SECCIÓN principal ([SectionName]), no si existe una clave individual.
 */

class clsIniReader {
public:
    struct ChildNode {
        std::string Key;
        std::string Value;
    };

    struct MainNode {
        std::string name;
        std::vector<ChildNode> values;
        std::int32_t numValues{0};
    };

    clsIniReader();
    ~clsIniReader();

    void Initialize(const std::string& file);
    std::string GetValue(const std::string& Main, const std::string& Key);
    void ChangeValue(const std::string& Main, const std::string& Key, std::int32_t Value);
    void ChangeValue(const std::string& Main, const std::string& Key, const std::string& Value);
    bool KeyExists(const std::string& name);

private:
    void Class_Terminate();
    std::int32_t FindMain(const std::string& name) const;
    std::int32_t FindKey(const MainNode& Node, const std::string& Key) const;
    void SortMainNodes(std::int32_t First, std::int32_t Last);
    void SortChildNodes(MainNode& Node, std::int32_t First, std::int32_t Last);

    std::vector<MainNode> fileData;
    std::int32_t MainNodes{0};
};

#endif // CLSINIREADER_HPP
