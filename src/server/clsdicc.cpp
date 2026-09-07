#include "clsdicc.hpp"
#include <algorithm>
#include <cctype>

static std::string ToUpper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return str;
}

std::int16_t clsdicc::CantElem() const {
    return static_cast<std::int16_t>(m_order.size());
}

bool clsdicc::AtPut(const std::string& clave, const std::string& elem) {
    if (clave.empty()) {
        return false;
    }

    std::string keyUpper = ToUpper(clave);

    auto it = m_elementos.find(keyUpper);
    if (it != m_elementos.end()) {
        it->second = elem;
    } else {
        m_elementos[keyUpper] = elem;
        m_order.push_back(keyUpper);
    }

    return true;
}

bool clsdicc::AtPut(const std::string& clave, int elem) {
    return AtPut(clave, std::to_string(elem));
}

std::string clsdicc::At(const std::string& clave) const {
    std::string keyUpper = ToUpper(clave);
    auto it = m_elementos.find(keyUpper);
    if (it != m_elementos.end()) {
        return it->second;
    }
    return "";
}

std::string clsdicc::AtIndex(int i) const {
    if (i >= 1 && i <= static_cast<int>(m_order.size())) {
        return m_order[static_cast<size_t>(i - 1)];
    }
    return "";
}

std::string clsdicc::MayorValor(int& cant) const {
    int maxVal = -1;
    cant = 0;
    std::string result;

    /*
     * Reproducción exacta de la lógica VB6 en clsdicc.cls (método MayorValor):
     *
     *   max = -1
     *   cant = 0
     *   clave = vbNullString
     *   For i = 1 To p_cant
     *       If max <= CInt(p_elementos(i).def) Then
     *           cant = IIf(max = CInt(p_elementos(i).def), cant + 1, 1)
     *           clave = IIf(max = CInt(p_elementos(i).def), clave & "," & p_elementos(i).clave, p_elementos(i).clave)
     *           max = CInt(p_elementos(i).def)
     *       End If
     *   Next i
     *
     * Nota: En caso de empate de valores máximos, concatena TODAS las claves empatadas
     * separadas por comas, manteniendo el ORDEN DE INSERCIÓN ORIGINAL.
     */
    for (const std::string& keyUpper : m_order) {
        auto it = m_elementos.find(keyUpper);
        if (it != m_elementos.end()) {
            int currentVal = 0;
            try {
                currentVal = std::stoi(it->second);
            } catch (...) {
                currentVal = 0;
            }

            if (maxVal <= currentVal) {
                if (maxVal == currentVal) {
                    cant += 1;
                    result = result + "," + keyUpper;
                } else {
                    cant = 1;
                    result = keyUpper;
                    maxVal = currentVal;
                }
            }
        }
    }

    return result;
}

void clsdicc::DumpAll() {
    m_elementos.clear();
    m_order.clear();
}
