#ifndef __DOUBLE_LINKEDLIST_H__
#define __DOUBLE_LINKEDLIST_H__

#include "linkedlist.h"

template <typename T>
class LinkedListBackwardIterator : public GeneralIterator<LinkedListBackwardIterator<T>, LinkedListNode<T>> {
public:
    using value_type = LinkedListNode<T>;
    using MySelf = LinkedListBackwardIterator<T>;
    using Parent = GeneralIterator<MySelf, value_type>;
    using Parent::Parent; // Inherit constructor
    LinkedListBackwardIterator& operator++() {
        Parent::m_ptr = Parent::m_ptr->m_pPrev == Parent::m_pStart ? nullptr : Parent::m_ptr->m_pPrev;
        return *this;
    }
};

template <typename T>
struct DoubleLinkedListAscTraits : public LinkedListAscTraits<T> {
    using BackwardIterator = LinkedListBackwardIterator<T>;
};

template <typename T>
struct DoubleLinkedListDescTraits : public LinkedListDescTraits<T> {
    using BackwardIterator = LinkedListBackwardIterator<T>;
};

template <typename Traits>
class DoubleLinkedList : public virtual LinkedList<Traits>, public ReversibleContainerBase<Traits> {
public:
    using Base = LinkedList<Traits>;
    using value_type = typename Traits::value_type;
    using Node = typename Traits::Node;
    using NodePtr = Node*;
    using ForwardIterator = typename Traits::ForwardIterator;
    using BackwardIterator = typename Traits::BackwardIterator;

protected:
    void linkNode(NodePtr pNew, NodePtr pPrev, NodePtr pNext = nullptr) override {
        pNew->m_pPrev = pPrev;
        if (pNext) pNext->m_pPrev = pNew;
    }

public:
    DoubleLinkedList() = default;
    DoubleLinkedList(const DoubleLinkedList& other) {
        Base::operator=(static_cast<const Base&>(other));
    }

    DoubleLinkedList& operator=(const DoubleLinkedList& other) {
        if (this != &other) {
            Base::operator=(static_cast<const Base&>(other));
        }
        return *this;
    }

    BackwardIterator rbegin() const override { return BackwardIterator(this->m_pTail); }
    BackwardIterator rend()   const override { return BackwardIterator(nullptr); }
};
#endif