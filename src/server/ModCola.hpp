#ifndef MODCOLA_HPP
#define MODCOLA_HPP

#include <string>
#include <deque>
#include <cstddef>

/**
 * @file ModCola.hpp
 * @brief Port C++ de ModCola.cls (nombre de clase VB6: cCola).
 *
 * QUIRK VB6 Y DECISIONES DE DISEÑO:
 * 1. A pesar de llamarse cCola, este módulo no es una cola FIFO pura. Se utiliza en el servidor
 *    legacy exclusivamente para gestionar la lista de peticiones de soporte (/AYUDA) mediante
 *    la variable global 'Ayuda As New cCola'.
 * 2. Cada elemento ingresado mediante Push() se almacena con un prefijo de hora de 8 caracteres
 *    en formato "HH:MM:SS" (equivalente a la función time$ de VB6) seguido de un espacio y el nombre
 *    normalizado en mayúsculas: "HH:MM:SS NOMBRE".
 * 3. Admite indexación 1-based (VerElemento, QuitarIndex), verificación de existencia ignorando
 *    el prefijo de timestamp (Existe) y eliminación por coincidencia de nombre (Quitar).
 * 4. Si la cola está vacía o un índice está fuera de rango, las funciones que retornan cadenas
 *    devuelven "0" (comportamiento idéntico al On Error Resume Next y conversión implícita de VB6).
 */
class cCola {
public:
    cCola() = default;
    ~cCola() = default;

    /// Vacía la cola por completo eliminando todos los elementos.
    void Reset();

    /// Devuelve la cantidad de elementos activos en la cola.
    int Longitud() const;

    /// Inspecciona el elemento en la posición 1-based 'index' en mayúsculas (retorna "0" si el índice es inválido).
    std::string VerElemento(int index) const;

    /// Agrega un nombre al final de la cola prefijándolo con la hora actual ("HH:MM:SS NOMBRE").
    void Push(const std::string& Nombre);

    /// Agrega un nombre al final de la cola con una marca de tiempo específica (útil para pruebas unitarias).
    void PushConTiempo(const std::string& Nombre, const std::string& TimeStr);

    /// Remueve y devuelve el primer elemento de la cola (retorna "0" si está vacía).
    std::string Pop();

    /// Devuelve el primer elemento de la cola sin removerlo (retorna "0" si está vacía).
    std::string PopByVal() const;

    /// Comprueba si un usuario existe en la cola ignorando el prefijo de timestamp ("HH:MM:SS ").
    bool Existe(const std::string& Nombre) const;

    /// Elimina la primera ocurrencia del usuario especificado (ignorando el prefijo de timestamp).
    void Quitar(const std::string& Nombre);

    /// Elimina el elemento en la posición 1-based 'index' (no hace nada si el índice es inválido).
    void QuitarIndex(int index);

private:
    std::deque<std::string> m_cola;

    bool IndexValido(int index) const;
    static std::string ObtenerHoraActual();
    static std::string AMayusculas(const std::string& str);
};

#endif // MODCOLA_HPP
