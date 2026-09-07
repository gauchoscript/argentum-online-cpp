#include "server/ModCola.hpp"
#include <ctime>
#include <cctype>
#include <algorithm>

void cCola::Reset() {
    m_cola.clear();
}

int cCola::Longitud() const {
    return static_cast<int>(m_cola.size());
}

bool cCola::IndexValido(int index) const {
    return index >= 1 && static_cast<std::size_t>(index) <= m_cola.size();
}

std::string cCola::AMayusculas(const std::string& str) {
    std::string res = str;
    std::transform(res.begin(), res.end(), res.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return res;
}

std::string cCola::ObtenerHoraActual() {
    std::time_t t = std::time(nullptr);
    std::tm tm_buf{};
#if defined(_WIN32)
    localtime_s(&tm_buf, &t);
#else
    localtime_r(&t, &tm_buf);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%H:%M:%S", &tm_buf);
    return std::string(buf);
}

std::string cCola::VerElemento(int index) const {
    if (IndexValido(index)) {
        return AMayusculas(m_cola[static_cast<std::size_t>(index - 1)]);
    }
    return "0";
}

void cCola::Push(const std::string& Nombre) {
    PushConTiempo(Nombre, ObtenerHoraActual());
}

void cCola::PushConTiempo(const std::string& Nombre, const std::string& TimeStr) {
    std::string aux = TimeStr + " " + AMayusculas(Nombre);
    m_cola.push_back(aux);
}

std::string cCola::Pop() {
    if (!m_cola.empty()) {
        std::string elem = m_cola.front();
        m_cola.pop_front();
        return elem;
    }
    return "0";
}

std::string cCola::PopByVal() const {
    if (!m_cola.empty()) {
        return m_cola.front();
    }
    return "0";
}

bool cCola::Existe(const std::string& Nombre) const {
    std::string nombreEnMayus = AMayusculas(Nombre);
    for (int i = 1; i <= Longitud(); ++i) {
        std::string elem = VerElemento(i);
        if (elem.length() >= 9) {
            std::string v = elem.substr(9);
            if (v == nombreEnMayus) {
                return true;
            }
        }
    }
    return false;
}

void cCola::Quitar(const std::string& Nombre) {
    std::string nombreEnMayus = AMayusculas(Nombre);
    for (auto it = m_cola.begin(); it != m_cola.end(); ++it) {
        std::string elem = AMayusculas(*it);
        if (elem.length() >= 9) {
            std::string v = elem.substr(9);
            if (v == nombreEnMayus) {
                m_cola.erase(it);
                return;
            }
        }
    }
}

void cCola::QuitarIndex(int index) {
    if (IndexValido(index)) {
        m_cola.erase(m_cola.begin() + (index - 1));
    }
}
