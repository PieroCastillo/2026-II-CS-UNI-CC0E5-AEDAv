#ifndef __VECTOR_H__
#define __VECTOR_H__
#include <mutex>
#include <initializer_list>
#include "GeneralIterator.h"
#include "../types.h" // Ref
#include "../foreach.h" // ref
using namespace std;

template <typename T>
class VectorForwardIterator : public GeneralIterator<VectorForwardIterator<T>, T> {
public:
    using value_type = T;
    using MySelf = VectorForwardIterator<T>;
    using Parent = GeneralIterator<MySelf, T>;
    using Parent::Parent; // Inherit constructor

    // Prefix increment
    VectorForwardIterator& operator++() { ++Parent::m_ptr; return *this; }
};

template <typename T>
class VectorBackwardIterator : public GeneralIterator<VectorBackwardIterator<T>, T> {
public:
    using value_type = T;
    using MySelf = VectorBackwardIterator<T>;
    using Parent = GeneralIterator<MySelf, T>;
    using Parent::Parent; // Inherit constructor

    // Prefix increment
    VectorBackwardIterator& operator++() { --Parent::m_ptr; return *this; }
};

template <typename T>
struct GeneralNode {
private:
    T   m_value;
    Ref m_ref;      // Reference to the value

public:
    GeneralNode() = default; // requerido por resize(): new Node[new_cap]
    GeneralNode(const T& value, Ref ref) : m_value(value), m_ref(ref) {}
    T    getValue() const { return m_value; }
    Ref  getRef()   const { return m_ref; }
    T& value() { return m_value; } // acceso mutable para ApplyFunction

    friend ostream& operator <<(ostream& os, const GeneralNode<T>& node) {
        os << "(" << node.getValue() << "," << node.getRef() << ")";
        return os;
    }
};

template <typename T>
struct VectorAscTraits {
    using value_type = T;
    using Node = GeneralNode<T>;
    using ForwardIterator = VectorForwardIterator<Node>;  // itera sobre Node, no sobre T
    using BackwardIterator = VectorBackwardIterator<Node>; // itera sobre Node, no sobre T
};

template <typename Traits>
class Vector {
public:
    using value_type = Traits::value_type;
    using Node = Traits::Node;
    using ForwardIterator = Traits::ForwardIterator;
    using BackwardIterator = Traits::BackwardIterator;
private:
    Node* m_data = nullptr;   // puntero al arreglo dinámico
    size_t       m_size = 0,         // cantidad actual
        m_capacity = 0;         // capacidad
    mutex        m_mutex;                // mutex para sincronización

    void resize(size_t new_cap) {
        if (new_cap <= m_capacity) return;
        Node* new_data = new Node[new_cap];
        for (size_t i = 0; i < m_size; ++i)
            new_data[i] = m_data[i];
        delete[] m_data;
        m_data = new_data;
        m_capacity = new_cap;
    }

public:
    Vector() {}

    // Cada elemento de la lista es una pareja (valor, ref) para un Node
    Vector(initializer_list<pair<value_type, Ref>> values) {
        for (const auto& v : values)
            push_back(v.first, v.second);
    }

    virtual ~Vector() {
        clear();
    }

    Vector(const Vector& other) {
        resize(other.m_size);
        for (size_t i = 0; i < other.m_size; ++i)
            m_data[i] = other.m_data[i];
        m_size = other.m_size;
    }

    // Move constructor and move assignment operator
    Vector(Vector&& other) noexcept {
        m_data = std::exchange(other.m_data, nullptr);
        m_size = std::exchange(other.m_size, 0);
        m_capacity = std::exchange(other.m_capacity, 0);
    }

    // Move assignment operator
    Vector& operator=(Vector&& other) noexcept {
        if (this != &other) {
            clear();
            m_data = std::exchange(other.m_data, nullptr);
            m_size = std::exchange(other.m_size, 0);
            m_capacity = std::exchange(other.m_capacity, 0);
        }
        return *this;
    }

    void push_back(const value_type& value, Ref ref) {
        lock_guard<mutex> lock(m_mutex);
        if (m_size == m_capacity) {
            size_t new_cap = (m_capacity == 0) ? 10 : m_capacity * 2;
            resize(new_cap);
        }
        m_data[m_size] = Node(value, ref);
        ++m_size;
    }

    void pop_back() {
        lock_guard<mutex> lock(m_mutex);
        if (m_size > 0)
            --m_size;
    }

    Node& operator[](size_t index) {
        if (index >= m_size) throw std::out_of_range("Indice fuera de rango");
        return m_data[index];
    }

    Node& at(size_t index) {
        if (index >= m_size) throw std::out_of_range("Indice fuera de rango");
        return m_data[index];
    }

    size_t size()     const { return m_size; }
    size_t capacity() const { return m_capacity; }
    bool   empty()    const { return m_size == 0; }

    void clear() {
        lock_guard<mutex> lock(m_mutex);
        delete[] m_data;
        m_data = nullptr;
        m_size = 0;
        m_capacity = 0;
    }

    ForwardIterator   begin() { return ForwardIterator(m_data); }
    ForwardIterator   end() { return ForwardIterator(m_data + m_size); }
    BackwardIterator rbegin() { return BackwardIterator(m_data + m_size - 1); }
    BackwardIterator rend() { return BackwardIterator(m_data - 1); }

    // Persistencia
    ostream& write(ostream& os) {
        os << "[";

        size_t i = 0;
        ApplyFunction([&](Node& value, ostream& os) {
            os << value;
            if (i + 1 < size())
                os << ", ";
            ++i;
            }, os);

        return os << "]";
    }

    istream& read(istream& is) {
        char c;
        is >> c; // [

        while (is >> c && c != ']') {
            value_type value;
            Ref ref;
            // (
            is >> value;
            is >> c; // ,
            is >> ref;
            is >> c; // )

            push_back(value, ref);

            is >> c; // , or ]
            if (c == ']')
                break;
        }
        return is;
    }

    // Aplicarle una funcion a cada elemento.
    //       ej. sumarle un valor x
    // Variadic template to allow passing additional arguments to the function
    // Iterator Level #0
    template <typename Func, typename... Args>
    void ApplyFunction(Func func, Args&&... args) {
        lock_guard<mutex> lock(m_mutex);
        ::ApplyFunction(begin(), end(), func, std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    Node& FirstThat(Func func, Args... args) {
        lock_guard<mutex> lock(m_mutex);
        // TODO: retutilizar la funcion ApplyFunction generica de foreach.h
        for (size_t i = 0; i < size(); ++i)
            if (func(m_data[i], args...))
                return m_data[i];
    }

    // template<typename Func, typename... Args>
    // decltype(auto) call(Func func, Args&&... args)
    // {
    //     if constexpr(is_void_v<invoke_result_t<Func, Args...>>)
    //     {    //cout << "Function is returning: void!" << endl;
    //          invoke(forward<Func>(func), forward<Args>(args)...);
    //          //...  // do something before we return
    //          return;
    //     }
    //     else // return type is not void:
    //     { auto ret = invoke(forward<Func>(func), forward<Args>(args)...);
    //          //cout << "Function is returning: " << type_name<decltype(ret)>() << endl;
    //          //...  // do something (with ret) before we return
    //          return ret;
    //     }
    // }
};

template <typename T>
ostream& operator<<(ostream& os, Vector<T>& vec) {
    return vec.write(os);
}

template <typename T>
istream& operator>>(istream& is, Vector<T>& vec) {
    return vec.read(is);
}

#endif // __VECTOR_H__