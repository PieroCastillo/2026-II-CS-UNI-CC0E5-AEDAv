#ifndef __LINKEDLIST_H__
#define __LINKEDLIST_H__
#include <mutex>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "../foreach.h"
#include "containerbase.h"

template <typename T>
class LinkedListNode : public GeneralNode<T> {
    using Node = LinkedListNode<T>;
    using NodePtr = Node*;
public:
    Node* m_pNext = nullptr; // puntero al siguiente nodo, npara acceso directo necesita ser publico
    LinkedListNode() : GeneralNode<T>(T{}, Ref{}), m_pNext(nullptr) {}
    // añadir friend class añade otro typename al template
    LinkedListNode(const T& value, Ref ref, Node* pNext) : GeneralNode<T>(value, ref), m_pNext(pNext) {}
};

template <typename T>
class LinkedListForwardIterator : public GeneralIterator<LinkedListForwardIterator<T>, LinkedListNode<T>> {
public:
    using Node = LinkedListNode<T>;
    using MySelf = LinkedListForwardIterator<T>;
    using Parent = GeneralIterator<MySelf, Node>;
    using Parent::Parent; // Inherit constructor
    LinkedListForwardIterator& operator++() { Parent::m_ptr = Parent::m_ptr->m_pNext; return *this; }
};

template <typename T>
struct LinkedListAscTraits : public AscendingTraits<T> {
    using Node = LinkedListNode<T>;
    using ForwardIterator = LinkedListForwardIterator<T>;  // itera sobre Node, no sobre T
};

template <typename T>
struct LinkedListDescTraits : public DescendingTraits<T> {
    using Node = LinkedListNode<T>;
    using ForwardIterator = LinkedListForwardIterator<T>;  // itera sobre Node, no sobre T
};
template <typename Traits>
class LinkedList : public ContainerBase<Traits> {
public:
    using value_type = typename Traits::value_type;
    using Node = typename Traits::Node;
    using NodePtr = Node*;
    using ForwardIterator = typename Traits::ForwardIterator;
    using Compare = typename Traits::Compare;
    using Delim = typename Node::Delim;
    using Base = ContainerBase<Traits>;
    using Base::Base;
private:
    NodePtr m_pRoot = nullptr; // puntero al primer nodo de la lista enlazada
    NodePtr m_pTail = nullptr; // puntero al último nodo de la lista enlazada

    Compare m_comp; // comparador para ordenar los nodos de la lista

    NodePtr GetRoot() const { return m_pRoot; }
    void internalInsert(const value_type& value, Ref ref, NodePtr& rParent);
public:
    LinkedList() = default;
    ~LinkedList() override { clear(); };
    LinkedList(std::initializer_list<std::pair<value_type, Ref>> values) { this->initialize(values); }
    LinkedList(const LinkedList& another) { *this = another; } // copia profunda de la lista enlazada
    LinkedList& operator=(const LinkedList& another);

    ForwardIterator begin() const override { return ForwardIterator(m_pRoot); }
    ForwardIterator end() const override { return ForwardIterator(nullptr); }

    void push_back(const value_type& value, Ref ref) override;
    void clear() override;

    bool empty() const { return m_pRoot == nullptr; }

    void insert(const value_type& value, Ref ref) {
        scoped_lock lock(this->m_mutex);
        internalInsert(value, ref, m_pRoot);
    }
};


template <typename Traits>
LinkedList<Traits>& LinkedList<Traits>::operator=(const LinkedList<Traits>& other) {
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
void LinkedList<Traits>::clear() {
    scoped_lock lock(this->m_mutex);
    auto* curr = m_pRoot;
    while (curr != nullptr)
    {
        auto* next = curr->m_pNext;
        delete curr;
        curr = next;
    }
    m_pRoot = nullptr;
}

template<typename Traits>
void LinkedList<Traits>::push_back(const value_type& value, Ref ref) {
    scoped_lock lock(this->m_mutex);
    NodePtr new_node = new Node(value, ref, nullptr);
    if (!this->m_pRoot) {
        this->m_pRoot = new_node;
        this->m_pTail = new_node;
    }
    else {
        this->m_pTail->m_pNext = new_node;
        this->m_pTail = new_node;
    }
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
