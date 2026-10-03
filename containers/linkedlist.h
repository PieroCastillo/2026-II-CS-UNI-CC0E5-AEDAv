#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__
#include <mutex>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "../foreach.h"

template <typename T>
class LinkedListNode : public GeneralNode<T> {
    using Node = LinkedListNode<T>;
    using NodePtr = Node*;
public:
    Node* m_pNext = nullptr; // puntero al siguiente nodo, npara acceso directo necesita ser publico
    // añadir friend class añade otro typename al template
    LinkedListNode(const T& value, Ref ref, Node* pNext) : GeneralNode<T>(value, ref), m_pNext(pNext) {}
};

template <typename T>
class LinkedListForwardIterator : public GeneralIterator<LinkedListForwardIterator<T>, LinkedListNode<T>> {
public:
    using value_type = LinkedListNode<T>;
    using MySelf = LinkedListForwardIterator<T>;
    using Parent = GeneralIterator<MySelf, value_type>;
    using Parent::Parent; // Inherit constructor
    LinkedListForwardIterator& operator++() { Parent::m_ptr = Parent::m_ptr->m_pNext; return *this; }
};

template <typename T>
struct LinkedListAscTraits {
    using value_type = T;
    using Node = LinkedListNode<T>;
    using ForwardIterator = LinkedListForwardIterator<T>;  // itera sobre Node, no sobre T
};

template <typename Traits>
class LinkedList {
public:
    using value_type = typename Traits::value_type;
    using Node = typename Traits::Node;
    using NodePtr = Node*;
    using ForwardIterator = typename Traits::ForwardIterator;
private:
    NodePtr m_pRoot = nullptr; // puntero al primer nodo de la lista enlazada
    NodePtr m_pTail = nullptr; // puntero al último nodo de la lista enlazada
    mutable std::mutex m_mutex; // mutex para sincronización

    NodePtrGetRoot() const { return m_pRoot; }
public:
    LinkedList() {}
    LinkedList(const LinkedList&) = delete; // no se permite copia
    LinkedList& operator=(const LinkedList&) = delete; // no se permite asignacion

    void clear();
    virtual ~LinkedList();

    void push_back(const value_type& value, Ref ref);

    bool empty() const { return m_pRoot == nullptr; }
private:
    void internalInsert(const value_type& value, Ref ref, NodePtr& rParent);
public:
    void insert(const value_type& value, Ref ref) {
        scoped_lock lock(m_mutex);
        internalInsert(value, ref, m_pRoot);
    }

    std::ostream& write(std::ostream& os) { return os << *this; }
    std::istream& read(std::istream& is) { return is >> *this; }
    friend std::ostream& operator <<(std::ostream& os, const LinkedList<Traits>& list) {
        lock_guard lock(list.m_mutex);
        auto first = true;
        os << "[";
        for (auto it = list.begin(); it != list.end(); ++it)
        {
            if (!first)
                os << ",";
            os << *it;
            first = false;
        }
        return os << "]";
    }
    
    friend std::istream& operator >>(std::istream& is, const LinkedList<Traits>& list) {
        lock_guard lock(list.m_mutex);
        uint32_t c;
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
        return is;
    }
    // Iterators
    ForwardIterator begin() { return ForwardIterator(m_pRoot); }
    ForwardIterator end() { return ForwardIterator(nullptr); }
    ForwardIterator begin() const { return ForwardIterator(m_pRoot); }
    ForwardIterator end() const { return ForwardIterator(nullptr); }

    template <typename Func, typename... Args>
    void ApplyFunction(Func func, Args... args) {
        call(func, std::forward<Args>(args)...);
    }

    template <typename Func, typename... Args>
    Node& FirstThat(Func func, Args... args) {
        return call(func, std::forward<Args>(args)...);
    }

    template<typename Func, typename... Args>
    decltype(auto) call(Func func, Args&&... args)
    {
        lock_guard<mutex> lock(m_mutex);
        if constexpr (is_void_v<invoke_result_t<Func, Node&, Args...>>)
            ::call(begin(), end(), std::forward<Func>(func), std::forward<Args>(args)...);
        else // return type is not void:
            return ::call(begin(), end(), std::forward<Func>(func), std::forward<Args>(args)...);
    }
};

template <typename Traits>
void LinkedList<Traits>::clear()
{
    scoped_lock lock(m_mutex);
    for (auto it = begin(); it != end(); ++it)
    {
        delete& (*it);
    }
    m_pRoot = nullptr;
    m_pTail = nullptr;
}

template <typename Traits>
LinkedList<Traits>::~LinkedList()
{
    clear();
}

template <typename Traits>
void LinkedList<Traits>::push_back(const value_type& value, Ref ref)
{
    scoped_lock lock(m_mutex);
    internalInsert(value, ref, m_pRoot);
}

/*
internalInsert(...) recorre todos los nodos hasta llegar al final, y en la condición de parada
si el nodo padre es nullptr, crea uno nuevo con el valor y retorna
insert(...) empieza desde el nodo raiz
*/
template <typename Traits>
void LinkedList<Traits>::internalInsert(const value_type& value, Ref ref, NodePtr& rParent) {
    if (rParent == nullptr || value < rParent->getValue()) {
        rParent = new Node(value, ref, rParent);
        m_pTail = rParent;
        return;
    }
    internalInsert(value, ref, rParent->m_pNext);
}
#endif // __LINKEDLIST_H__
