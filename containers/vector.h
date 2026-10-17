#ifndef __VECTOR_H__
#define __VECTOR_H__
#include <mutex>
#include <initializer_list>
#include "GeneralIterator.h"
#include "GeneralNode.h"
#include "../foreach.h"
#include "../types.h" // Ref
#include "traits.h"
#include "containerbase.h"

using namespace std;

template <typename Traits>
class Vector : public ContainerBase<Traits, true> {
public:
    using value_type = Traits::value_type;
    using Node = Traits::Node;
    using ForwardIterator = Traits::ForwardIterator;
    using BackwardIterator = Traits::BackwardIterator;
    using Base = ContainerBase<Traits, true>;
    using Base::Base;
private:
    Node* m_data = nullptr;   // puntero al arreglo dinámico
    size_t       m_size = 0,         // cantidad actual
        m_capacity = 0;         // capacidad

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
    void push_back(const value_type& value, Ref ref) override;
    void clear() override;

    Vector() = default;
    ~Vector() override { clear(); };
    Vector(std::initializer_list<std::pair<value_type, Ref>> values) { this->initialize(values); }
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

    void pop_back() {
        lock_guard<mutex> lock(this->m_mutex);
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

    ForwardIterator begin() const override { return ForwardIterator(m_data); }
    ForwardIterator end() const override { return ForwardIterator(m_data + m_size); }
    BackwardIterator rbegin() { return BackwardIterator(m_data + m_size - 1); }
    BackwardIterator rend() { return BackwardIterator(m_data - 1); }

    template<typename Func, typename... Args>
    decltype(auto) rcall(Func func, Args&&... args)
    {
        lock_guard<mutex> lock(this->m_mutex);
        if constexpr (is_void_v<invoke_result_t<Func, Node&, Args...>>)
            ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
        else // return type is not void:
            return ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
    }
};

template <typename Traits>
void Vector<Traits>::clear() {
    lock_guard<mutex> lock(this->m_mutex);
    delete[] m_data;
    m_data = nullptr;
    m_size = 0;
    m_capacity = 0;
}

template <typename Traits>
void Vector<Traits>::push_back(const value_type& value, Ref ref) {
    lock_guard<mutex> lock(this->m_mutex);
    if (m_size == m_capacity) {
        size_t new_cap = (m_capacity == 0) ? 10 : m_capacity * 2;
        resize(new_cap);
    }
    m_data[m_size] = Node(value, ref);
    ++m_size;
}
#endif // __VECTOR_H__