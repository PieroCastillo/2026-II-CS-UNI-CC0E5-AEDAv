#ifndef __DOUBLYLINKEDLIST_H__
#define __DOUBLYLINKEDLIST_H__
#include <mutex>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "traits.h"
#include "../foreach.h"

template <typename T>
class DoublyLinkedListNode : public GeneralNode<T> {
    using Node = DoublyLinkedListNode<T>;
    using NodePtr = Node*;
public:
    Node* m_pNext = nullptr; // puntero al siguiente nodo, para acceso directo necesita ser publico
    Node* m_pPrev = nullptr; // puntero al nodo anterior, para acceso directo necesita ser publico
    DoublyLinkedListNode() : GeneralNode<T>(T{}, Ref{}), m_pNext(nullptr) {}
    // añadir friend class añade otro typename al template
    DoublyLinkedListNode(const T& value, Ref ref, Node* pNext, Node* pPrev) : GeneralNode<T>(value, ref), m_pNext(pNext), m_pPrev(pPrev) {}
};

template <typename T>
class DoublyLinkedListForwardIterator : public GeneralIterator<DoublyLinkedListForwardIterator<T>, DoublyLinkedListNode<T>> {
public:
    using value_type = DoublyLinkedListNode<T>;
    using MySelf = DoublyLinkedListForwardIterator<T>;
    using Parent = GeneralIterator<MySelf, value_type>;
    using Parent::Parent; // Inherit constructor
    DoublyLinkedListForwardIterator& operator++() { Parent::m_ptr = Parent::m_ptr->m_pNext; return *this; }
};

template <typename T>
class DoublyLinkedListBackwardIterator : public GeneralIterator<DoublyLinkedListBackwardIterator<T>, DoublyLinkedListNode<T>> {
public:
    using value_type = DoublyLinkedListNode<T>;
    using MySelf = DoublyLinkedListBackwardIterator<T>;
    using Parent = GeneralIterator<MySelf, value_type>;
    using Parent::Parent; // Inherit constructor
    DoublyLinkedListBackwardIterator& operator++() { Parent::m_ptr = Parent::m_ptr->m_pPrev; return *this; }
};

template <typename T>
struct DoublyLinkedListAscTraits : public AscendingTraits<T> {
    using Node = DoublyLinkedListNode<T>;
    using ForwardIterator = DoublyLinkedListForwardIterator<T>;  // itera sobre Node, no sobre T
    using BackwardIterator = DoublyLinkedListBackwardIterator<T>; // itera sobre Node, no sobre T
};

template <typename T>
struct DoublyLinkedListDescTraits : public DescendingTraits<T> {
    using Node = DoublyLinkedListNode<T>;
    using ForwardIterator = DoublyLinkedListForwardIterator<T>;  // itera sobre Node, no sobre T
    using BackwardIterator = DoublyLinkedListBackwardIterator<T>; // itera sobre Node, no sobre T
};

template <typename Traits>
class DoublyLinkedList {
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
    DoublyLinkedList() {}
    DoublyLinkedList(const DoublyLinkedList& another) { *this = another; } // copia profunda de la lista enlazada
    DoublyLinkedList& operator=(const DoublyLinkedList& another); // no se permite asignacion
    DoublyLinkedList(initializer_list<pair<value_type, Ref>> values) {
        for (const auto& v : values)
            push_back(v.first, v.second);
    }

    void clear();
    virtual ~DoublyLinkedList() { clear(); };

    void push_back(const value_type& value, Ref ref);

    bool empty() const { return m_pRoot == nullptr; }

    void insert(const value_type& value, Ref ref) {
        scoped_lock lock(m_mutex);
        internalInsert(value, ref, m_pRoot);
    }

    std::ostream& write(std::ostream& os) { return os << *this; }
    std::istream& read(std::istream& is) { return is >> *this; }

    friend std::ostream& operator <<(std::ostream& os, const DoublyLinkedList<Traits>& list) {
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

    friend std::istream& operator >>(std::istream& is, DoublyLinkedList<Traits>& list) {
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
DoublyLinkedList<Traits>& DoublyLinkedList<Traits>::operator=(const DoublyLinkedList<Traits>& other) {
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
void DoublyLinkedList<Traits>::clear() {
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
void DoublyLinkedList<Traits>::push_back(const value_type& value, Ref ref) {
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
void DoublyLinkedList<Traits>::internalInsert(const value_type& value, Ref ref, NodePtr& rParent) {
    if (rParent == nullptr) {
        auto node = new Node(value, ref, nullptr, nullptr);
        m_pRoot = node;
        m_pTail = node;
        return;
    }

    if(rParent == m_pTail)
    {
        auto node = new Node(value, ref, nullptr, m_pTail);
        m_pTail->m_pNext = node;
        m_pTail = node;
        return;
    }

    auto newNode = new Node(value, ref, rParent->m_pNext, rParent);
    rParent->m_pNext->m_pPrev = newNode;
    rParent->m_pNext = newNode;
}
#endif // __DOUBLYLINKEDLIST_H__