#ifndef __CIRCULARDOUBLYLINKEDLIST_H__
#define __CIRCULARDOUBLYLINKEDLIST_H__
#include <mutex>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "traits.h"
#include "../foreach.h"

template <typename T>
class CircularDoublyLinkedListNode : public GeneralNode<T> {
    using Node = CircularDoublyLinkedListNode<T>;
    using NodePtr = Node*;
public:
    Node* m_pNext = nullptr; // puntero al siguiente nodo, para acceso directo necesita ser publico
    Node* m_pPrev = nullptr; // puntero al nodo anterior, para acceso directo necesita ser publico
    CircularDoublyLinkedListNode() : GeneralNode<T>(T{}, Ref{}), m_pNext(nullptr) {}
    // añadir friend class añade otro typename al template
    CircularDoublyLinkedListNode(const T& value, Ref ref, Node* pNext, Node* pPrev) : GeneralNode<T>(value, ref), m_pNext(pNext), m_pPrev(pPrev) {}
};

template <typename T>
class CircularDoublyLinkedListForwardIterator : public GeneralIterator<CircularDoublyLinkedListForwardIterator<T>, CircularDoublyLinkedListNode<T>> {
public:
    using value_type = CircularDoublyLinkedListNode<T>;
    using MySelf = CircularDoublyLinkedListForwardIterator<T>;
    using Parent = GeneralIterator<MySelf, value_type>;
    using Parent::Parent; // Inherit constructor
    CircularDoublyLinkedListForwardIterator& operator++() { Parent::m_ptr = Parent::m_ptr->m_pNext; return *this; }
};

template <typename T>
class CircularDoublyLinkedListBackwardIterator : public GeneralIterator<CircularDoublyLinkedListBackwardIterator<T>, CircularDoublyLinkedListNode<T>> {
public:
    using value_type = CircularDoublyLinkedListNode<T>;
    using MySelf = CircularDoublyLinkedListBackwardIterator<T>;
    using Parent = GeneralIterator<MySelf, value_type>;
    using Parent::Parent; // Inherit constructor
    CircularDoublyLinkedListBackwardIterator& operator++() { Parent::m_ptr = Parent::m_ptr->m_pPrev; return *this; }
};

template <typename T>
struct CircularDoublyLinkedListAscTraits : public AscendingTraits<T> {
    using Node = CircularDoublyLinkedListNode<T>;
    using ForwardIterator = CircularDoublyLinkedListForwardIterator<T>;  // itera sobre Node, no sobre T
    using BackwardIterator = CircularDoublyLinkedListBackwardIterator<T>; // itera sobre Node, no sobre T
};

template <typename T>
struct CircularDoublyLinkedListDescTraits : public DescendingTraits<T> {
    using Node = CircularDoublyLinkedListNode<T>;
    using ForwardIterator = CircularDoublyLinkedListForwardIterator<T>;  // itera sobre Node, no sobre T
    using BackwardIterator = CircularDoublyLinkedListBackwardIterator<T>; // itera sobre Node, no sobre T
};

template <typename Traits>
class CircularDoublyLinkedList {
public:
    using value_type = typename Traits::value_type;
    using Node = typename Traits::Node;
    using NodePtr = Node*;
    using ForwardIterator = typename Traits::ForwardIterator;
    using BackwardIterator = typename Traits::BackwardIterator;
    using Compare = typename Traits::Compare;
    using Delim = typename Node::Delim;
private:
    NodePtr m_pRoot = nullptr; // puntero al primer nodo de la lista enlazada
    NodePtr m_pTail = nullptr; // puntero al último nodo de la lista enlazada

    Compare m_comp; // comparador para ordenar los nodos de la lista
    mutable std::mutex m_mutex; // mutex para sincronización

    NodePtr GetRoot() const { return m_pRoot; }
    void internalInsert(const value_type& value, Ref ref, NodePtr& rParent);

public:
    CircularDoublyLinkedList() {}
    CircularDoublyLinkedList(const CircularDoublyLinkedList& another) { *this = another; } // copia profunda de la lista enlazada
    CircularDoublyLinkedList& operator=(const CircularDoublyLinkedList& another); // no se permite asignacion
    CircularDoublyLinkedList(initializer_list<pair<value_type, Ref>> values) {
        for (const auto& v : values)
            push_back(v.first, v.second);
    }

    void clear();
    virtual ~CircularDoublyLinkedList() { clear(); };

    void push_back(const value_type& value, Ref ref);

    bool empty() const { return m_pRoot == nullptr; }

    void insert(const value_type& value, Ref ref) {
        scoped_lock lock(m_mutex);
        internalInsert(value, ref, m_pRoot);
    }

    std::ostream& write(std::ostream& os) { return os << *this; }
    std::istream& read(std::istream& is) { return is >> *this; }

    friend std::ostream& operator <<(std::ostream& os, const CircularDoublyLinkedList<Traits>& list) {
        lock_guard lock(list.m_mutex);
        auto first = true;
        os << "[";
        for (auto it = list.begin(); it != list.end(); ++it) {
            if (!first)
                os << ",";
            os << *it;
            first = false;
        }
        return os << "]";
    }

    friend std::istream& operator >>(std::istream& is, CircularDoublyLinkedList<Traits>& list) {
        Delim d;
        Node node;
        list.clear();

        is >> d;
        if (is >> d && d != ']') {
            is.unget();
            while (is >> node >> d) {
                list.push_back(node.getValue(), node.getRef());
                if (d == ']') {
                    break;
                }
            }
        }

        return is;
    }

    // Iterators
    ForwardIterator begin() const { return ForwardIterator(m_pRoot); }
    ForwardIterator end() const { return ForwardIterator(nullptr); }
    BackwardIterator rbegin() const { return BackwardIterator(m_pTail); }
    BackwardIterator rend() const { return BackwardIterator(nullptr); }

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

    template<typename Func, typename... Args>
    decltype(auto) rcall(Func func, Args&&... args)
    {
        lock_guard<mutex> lock(m_mutex);
        if constexpr (is_void_v<invoke_result_t<Func, Node&, Args...>>)
            ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
        else // return type is not void:
            return ::call(rbegin(), rend(), std::forward<Func>(func), std::forward<Args>(args)...);
    }
};


template <typename Traits>
CircularDoublyLinkedList<Traits>& CircularDoublyLinkedList<Traits>::operator=(const CircularDoublyLinkedList<Traits>& other) {
    clear();
    if (!other.m_pRoot)
        return;
    std::lock_guard<std::mutex> lock(other.m_mutex);

    m_pRoot = new Node(other.GetRoot()->getValue(), other.GetRoot()->getRef(), nullptr);

    NodePtr next = other.m_pRoot->m_pNext;
    NodePtr curr = m_pRoot;

    while (next) {
        curr->m_pNext = new Node(next->getValue(), next->getRef(), nullptr);
        curr = curr->m_pNext;
        next = next->m_pNext;
    }
    m_pTail = curr;
    return *this;
}

template <typename Traits>
void CircularDoublyLinkedList<Traits>::clear() {
    scoped_lock lock(m_mutex);
    auto curr = m_pRoot;

    while(curr)
    {
        auto next = curr->m_pNext;
        delete curr;
        curr = next;
    }

    m_pRoot = nullptr;
    m_pTail = nullptr;
}

template<typename Traits>
void CircularDoublyLinkedList<Traits>::push_back(const value_type& value, Ref ref) {
    scoped_lock lock(m_mutex);
    NodePtr new_node = new Node(value, ref, nullptr, m_pTail);
    if (!this->m_pRoot) {
        this->m_pRoot = new_node;
        this->m_pTail = new_node;
    }
    else {
        this->m_pTail->m_pNext = new_node;
        this->m_pTail = new_node;
    }
}

template <typename Traits>
void CircularDoublyLinkedList<Traits>::internalInsert(const value_type& value, Ref ref, NodePtr& rParent) {
    if (rParent == nullptr) {
        auto node = new Node(value, ref, nullptr, nullptr);
        m_pRoot = node;
        m_pTail = node;
        node->m_pNext = node;
        node->m_pPrev = node;
        return;
    }

    if(rParent == m_pTail)
    {
        auto node = new Node(value, ref, m_pRoot, m_pTail);
        m_pTail->m_pNext = node;
        m_pRoot->m_pPrev = node;
        m_pTail = node;
        return;
    }

    auto newNode = new Node(value, ref, rParent->m_pNext, rParent);
    rParent->m_pNext->m_pPrev = newNode;
    rParent->m_pNext = newNode;
}
#endif // __CIRCULARDOUBLYLINKEDLIST_H__