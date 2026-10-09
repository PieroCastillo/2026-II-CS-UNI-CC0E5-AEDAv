#ifndef __CIRCULARLINKEDLIST_H__
#define __CIRCULARLINKEDLIST_H__
#include <mutex>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "traits.h"
#include "../foreach.h"

template <typename T>
class CircularListNode : public GeneralNode<T> {
    using Node = CircularListNode<T>;
    using NodePtr = Node*;
public:
    Node* m_pNext = nullptr; // puntero al siguiente nodo, npara acceso directo necesita ser publico
    CircularListNode() : GeneralNode<T>(T{}, Ref{}), m_pNext(nullptr) {}
    // añadir friend class añade otro typename al template
    CircularListNode(const T& value, Ref ref, Node* pNext) : GeneralNode<T>(value, ref), m_pNext(pNext) {}
};

template <typename T>
class CircularListForwardIterator : public GeneralIterator<CircularListForwardIterator<T>, CircularListNode<T>> {
public:
    using value_type = CircularListNode<T>;
    using MySelf = CircularListForwardIterator<T>;
    using Parent = GeneralIterator<MySelf, value_type>;
    using Parent::Parent; // Inherit constructor
    CircularListForwardIterator& operator++() { Parent::m_ptr = Parent::m_ptr->m_pNext; return *this; }
};

template <typename T>
struct CircularListAscTraits : public AscendingTraits<T> {
    using Node = CircularListNode<T>;
    using ForwardIterator = CircularListForwardIterator<T>;  // itera sobre Node, no sobre T
};

template <typename T>
struct CircularListDescTraits : public DescendingTraits<T> {
    using Node = CircularListNode<T>;
    using ForwardIterator = CircularListForwardIterator<T>;  // itera sobre Node, no sobre T
};
template <typename Traits>
class CircularList {
public:
    using value_type = typename Traits::value_type;
    using Node = typename Traits::Node;
    using NodePtr = Node*;
    using ForwardIterator = typename Traits::ForwardIterator;
    using Compare = typename Traits::Compare;
    using Delim = typename Node::Delim;
private:
    NodePtr m_pSentinel = nullptr;

    Compare m_comp; // comparador para ordenar los nodos de la lista
    mutable std::mutex m_mutex; // mutex para sincronización

    NodePtr GetRoot() const { return m_pRoot; }
    void internalInsert(const value_type& value, Ref ref, NodePtr& rParent);

public:
    CircularList() {}
    CircularList(const CircularList& another) { *this = another; } // copia profunda de la lista enlazada
    CircularList& operator=(const CircularList& another); // no se permite asignacion
    CircularList(initializer_list<pair<value_type, Ref>> values) {
        for (const auto& v : values)
            push_back(v.first, v.second);
    }

    void clear();
    virtual ~CircularList() { clear(); };

    void push_back(const value_type& value, Ref ref);

    bool empty() const { return m_pRoot == nullptr; }

    void insert(const value_type& value, Ref ref) {
        scoped_lock lock(m_mutex);
        internalInsert(value, ref, m_pRoot);
    }

    std::ostream& write(std::ostream& os) { return os << *this; }
    std::istream& read(std::istream& is) { return is >> *this; }

    friend std::ostream& operator <<(std::ostream& os, const CircularList<Traits>& list) {
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

    friend std::istream& operator >>(std::istream& is, CircularList<Traits>& list) {
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
CircularList<Traits>& CircularList<Traits>::operator=(const CircularList<Traits>& other) {
    clear();
    // if (!other.m_pRoot)
    //     return;
    // std::lock_guard<std::mutex> lock(other.m_mutex);

    // m_pRoot = new Node(other.GetRoot()->getValue(), other.GetRoot()->getRef(), nullptr);

    // NodePtr next = other.m_pRoot->m_pNext;
    // NodePtr curr = m_pRoot;

    // while (next) {
    //     curr->m_pNext = new Node(next->getValue(), next->getRef(), nullptr);
    //     curr = curr->m_pNext;
    //     next = next->m_pNext;
    // }
    // m_pTail = curr;
    return *this;
}

template <typename Traits>
void CircularList<Traits>::clear() {
    scoped_lock lock(m_mutex);
    if (!m_pSentinel)
        return;

    auto curr = m_pSentinel->m_pNext;
    while (curr != m_pSentinel && curr)
    {
        auto next = curr->m_pNext;
        delete curr;
        curr = next;
    }

    delete m_pSentinel;
    m_pSentinel = nullptr;
}

template<typename Traits>
void CircularList<Traits>::push_back(const value_type& value, Ref ref) {
    scoped_lock lock(m_mutex);
    if (!m_pSentinel)
    {
        m_pSentinel = new Node();
        m_pSentinel->m_pNext = m_pSentinel;
    }

    auto node = new Node(value, ref, m_pSentinel);
    auto tail = m_pSentinel;

    while (tail->m_pNext != m_pSentinel)
        tail = tail->m_pNext;

    tail->m_pNext = node;
}

template <typename Traits>
void CircularList<Traits>::internalInsert(const value_type& value, Ref ref, NodePtr& rParent) {
    if (!m_pSentinel)
    {
        m_pSentinel = new Node({}, {}, nullptr, nullptr);
        m_pSentinel->m_pNext = m_pSentinel;
        auto node = new Node(value, ref, m_pSentinel->m_pNext);
        m_pSentinel->m_pNext = node;
        return;
    }

    auto prevNode = !rParent ? m_pSentinel : rParent;
    auto node = new Node(value, ref, prevNode->m_pNext);
    prevNode->m_pNext = node;
    return;

}
#endif // __CIRCULARLINKEDLIST_H__