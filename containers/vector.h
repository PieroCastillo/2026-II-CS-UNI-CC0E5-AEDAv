#ifndef __VECTOR_H__
#define __VECTOR_H__

#include <iostream>
#include <stdexcept>
#include <algorithm> // para std::swap
#include <functional>
using namespace std;

template <typename T>
class Vector {
private:
    T* m_data;        // puntero al arreglo dinámico
    size_t  m_size,        // cantidad actual
        m_capacity;    // capacidad

    void reserve(size_t new_cap) {
        if (new_cap <= m_capacity) return;
        T* new_data = new T[new_cap];
        for (size_t i = 0; i < m_size; ++i) {
            new_data[i] = m_data[i];
        }
        delete[] m_data;
        m_data = new_data;
        m_capacity = new_cap;
    }

public:
    Vector() : m_data(nullptr), m_size(0), m_capacity(0) {}

    ~Vector() {
        delete[] m_data;
    }

    Vector(const Vector& other) : m_data(nullptr), m_size(0), m_capacity(0) {
        reserve(other.m_size);
        for (size_t i = 0; i < other.m_size; ++i) {
            m_data[i] = other.m_data[i];
        }
        m_size = other.m_size;
    }

    Vector& operator=(Vector other) {
        swap(other);
        return *this;
    }

    // Move constructor and move assignment operator
    Vector(Vector&& other) noexcept : m_data(nullptr), m_size(0), m_capacity(0) {
        swap(other);
    }

    // Move assignment operator
    Vector& operator=(Vector&& other) noexcept {
        swap(other);
        return *this;
    }

    void swap(Vector& other) noexcept {
        std::swap(m_data, other.m_data);
        std::swap(m_size, other.m_size);
        std::swap(m_capacity, other.m_capacity);

        if (m_size == m_capacity) {
            size_t new_cap = (m_capacity == 0) ? 1 : m_capacity * 2;
            reserve(new_cap);
        }
        // m_data[m_size] = value;
        ++m_size;
    }

    void push_back(const T& value)
    {
        reserve(m_size + 1);
        m_data[m_size] = value;
        ++m_size;
    };

    void pop_back() {
        if (m_size > 0) {
            --m_size;
            // m_data[m_size].~T();
        }
    }

    T& operator[](size_t index) {
        if (index >= m_size) throw std::out_of_range("Indice fuera de rango");
        return m_data[index];
    }

    const T& operator[](size_t index) const {
        if (index >= m_size) throw std::out_of_range("Indice fuera de rango");
        return m_data[index];
    }

    T& at(size_t index) {
        if (index >= m_size) throw std::out_of_range("Indice fuera de rango");
        return m_data[index];
    }

    const T& at(size_t index) const {
        if (index >= m_size) throw std::out_of_range("Indice fuera de rango");
        return m_data[index];
    }

    T size() const { return m_size; }
    T capacity() const { return m_capacity; }
    bool empty() const { return m_size == 0; }

    void clear() {
        for (size_t i = 0; i < m_size; ++i) {
            // m_data[i].~T();
        }
        m_size = 0;
    }

    ostream& print(ostream& os) {
        os << "[";
        for (size_t i = 0; i < size() - 1; ++i)
            os << m_data[i] << " ";
        if (size() > 0)
            os << m_data[size() - 1];
        return os << "]" << endl;
    }

    void apply(std::function<void(const T&)> predicate)
    {
        for (size_t i = 0; i < m_size; ++i)
        {
            predicate(m_data[i]);
        }
    }
    // TODO: aplicarle una funcion a cada elemento.
    //       ej. sumarle un valor x
};

template <typename T>
ostream& operator<<(ostream& os, Vector<T>& vec) {
    return vec.print(os);
}

#endif // __VECTOR_H__