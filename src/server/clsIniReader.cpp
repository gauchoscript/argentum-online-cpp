#include "clsIniReader.hpp"
#include <fstream>
#include <cctype>
#include <algorithm>

/*
 * clsIniReader.cpp - Implementación de la lectura de INIs con búsqueda binaria.
 * Transliteración 1:1 de legacy/server/Codigo/clsIniReader.cls
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

static std::string UCase(const std::string& str) {
    std::string result = str;
    for (char& c : result) {
        c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }
    return result;
}

static std::string Trim(const std::string& str) {
    std::size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    std::size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

clsIniReader::clsIniReader() {
}

clsIniReader::~clsIniReader() {
    Class_Terminate();
}

void clsIniReader::Class_Terminate() {
    fileData.clear();
    MainNodes = 0;
}

void clsIniReader::Initialize(const std::string& file) {
    Class_Terminate();

    std::ifstream handle(file, std::ios::binary);
    if (!handle.is_open()) {
        return;
    }

    std::string Text;
    while (std::getline(handle, Text)) {
        if (!Text.empty() && Text.back() == '\r') {
            Text.pop_back();
        }

        if (!Text.empty()) {
            if (Text[0] == '[') {
                std::size_t pos = Text.find(']', 1);
                if (pos != std::string::npos) {
                    MainNode node;
                    node.name = UCase(Trim(Text.substr(1, pos - 1)));
                    fileData.push_back(node);
                    MainNodes++;
                }
            } else {
                std::size_t pos = Text.find('=', 1);
                if (pos != std::string::npos) {
                    if (MainNodes > 0) {
                        MainNode& currentMain = fileData[MainNodes - 1];
                        ChildNode child;
                        child.Value = Text.substr(pos + 1);
                        child.Key = UCase(Text.substr(0, pos));
                        currentMain.values.push_back(child);
                        currentMain.numValues++;
                    }
                }
            }
        }
    }
    handle.close();

    if (MainNodes > 0) {
        SortMainNodes(0, MainNodes - 1);
        for (std::int32_t i = 0; i < MainNodes; i++) {
            if (fileData[i].numValues > 0) {
                SortChildNodes(fileData[i], 0, fileData[i].numValues - 1);
            }
        }
    }
}

void clsIniReader::SortMainNodes(std::int32_t First, std::int32_t Last) {
    std::int32_t min = First;
    std::int32_t max = Last;
    MainNode temp;

    std::string comp = fileData[(min + max) / 2].name;

    do {
        while (fileData[min].name < comp && min < Last) {
            min++;
        }
        while (fileData[max].name > comp && max > First) {
            max--;
        }
        if (min <= max) {
            temp = fileData[min];
            fileData[min] = fileData[max];
            fileData[max] = temp;
            min++;
            max--;
        }
    } while (min <= max);

    if (First < max) SortMainNodes(First, max);
    if (min < Last) SortMainNodes(min, Last);
}

void clsIniReader::SortChildNodes(MainNode& Node, std::int32_t First, std::int32_t Last) {
    std::int32_t min = First;
    std::int32_t max = Last;
    ChildNode temp;

    std::string comp = Node.values[(min + max) / 2].Key;

    do {
        while (Node.values[min].Key < comp && min < Last) {
            min++;
        }
        while (Node.values[max].Key > comp && max > First) {
            max--;
        }
        if (min <= max) {
            temp = Node.values[min];
            Node.values[min] = Node.values[max];
            Node.values[max] = temp;
            min++;
            max--;
        }
    } while (min <= max);

    if (First < max) SortChildNodes(Node, First, max);
    if (min < Last) SortChildNodes(Node, min, Last);
}

std::int32_t clsIniReader::FindMain(const std::string& name) const {
    std::int32_t min = 0;
    std::int32_t max = MainNodes - 1;
    std::int32_t mid = 0;

    std::string searchName = UCase(name);

    while (min <= max) {
        mid = (min + max) / 2;
        if (fileData[mid].name < searchName) {
            min = mid + 1;
        } else if (fileData[mid].name > searchName) {
            max = mid - 1;
        } else {
            return mid;
        }
    }
    return ~mid;
}

std::int32_t clsIniReader::FindKey(const MainNode& Node, const std::string& Key) const {
    std::int32_t min = 0;
    std::int32_t max = Node.numValues - 1;
    std::int32_t mid = 0;

    std::string searchKey = UCase(Key);

    while (min <= max) {
        mid = (min + max) / 2;
        if (Node.values[mid].Key < searchKey) {
            min = mid + 1;
        } else if (Node.values[mid].Key > searchKey) {
            max = mid - 1;
        } else {
            return mid;
        }
    }
    return ~mid;
}

std::string clsIniReader::GetValue(const std::string& Main, const std::string& Key) {
    std::int32_t i = FindMain(UCase(Main));
    if (i >= 0) {
        std::int32_t j = FindKey(fileData[i], UCase(Key));
        if (j >= 0) {
            return fileData[i].values[j].Value;
        }
    }
    return "";
}

void clsIniReader::ChangeValue(const std::string& Main, const std::string& Key, std::int32_t Value) {
    ChangeValue(Main, Key, std::to_string(Value));
}

void clsIniReader::ChangeValue(const std::string& Main, const std::string& Key, const std::string& Value) {
    std::int32_t i = FindMain(UCase(Main));
    if (i >= 0) {
        std::int32_t j = FindKey(fileData[i], UCase(Key));
        if (j >= 0) {
            fileData[i].values[j].Value = Value;
        }
    }
}

bool clsIniReader::KeyExists(const std::string& name) {
    return FindMain(UCase(name)) >= 0;
}
