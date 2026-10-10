#ifndef __CIRCULARDOUBLYLINKEDLIST_H__
#define __CIRCULARDOUBLYLINKEDLIST_H__
#include <mutex>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "traits.h"
#include "../foreach.h"
#include "doublylinkedlist.h"

template <typename Traits>
class CircularDoublyLinkedList : public DoublyLinkedList<Traits>
{
public:
    using value_type = typename Traits::value_type;
    using Node = typename Traits::Node;
    using NodePtr = Node*;
    using ForwardIterator = typename Traits::ForwardIterator;
    using BackwardIterator = typename Traits::BackwardIterator;
    using Compare = typename Traits::Compare;
    using Delim = typename Node::Delim;
protected:
    NodePtr m_pSentinel = nullptr;
    virtual NodePtr GetRoot() const { return m_pSentinel; }
    void internalInsert(const value_type& value, Ref ref, NodePtr&& rParent) override;
public:
    CircularDoublyLinkedList& operator=(const CircularDoublyLinkedList& another);
    void clear() override;
    void push_back(const value_type& value, Ref ref) override;
    bool empty() const override { return m_pSentinel == nullptr; }

    ForwardIterator begin() const override { return ForwardIterator(m_pSentinel ? m_pSentinel->m_pNext : nullptr); }
    ForwardIterator end() const override { return ForwardIterator(m_pSentinel); }
    BackwardIterator rbegin() const override { return BackwardIterator(m_pSentinel ? m_pSentinel->m_pPrev : nullptr); }
    BackwardIterator rend() const override { return BackwardIterator(m_pSentinel); }
};

template <typename Traits>
CircularDoublyLinkedList<Traits>& CircularDoublyLinkedList<Traits>::operator=(const CircularDoublyLinkedList<Traits>& other)
{
    if (this == &other)
        return *this;

    clear();
    std::lock_guard<std::mutex> lock(other.m_mutex);

    if (other.m_pSentinel == nullptr || other.m_pSentinel->m_pNext == other.m_pSentinel)
        return *this;

    auto* curr = other.m_pSentinel->m_pNext;
    while (curr != other.m_pSentinel)
    {
        this->push_back(curr->m_value);
        curr = curr->m_pNext;
    }

    return *this;
}

template <typename Traits>
void CircularDoublyLinkedList<Traits>::clear()
{
    scoped_lock lock(this->m_mutex);
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

template <typename Traits>
void CircularDoublyLinkedList<Traits>::push_back(const value_type& value, Ref ref)
{
    scoped_lock lock(this->m_mutex);
    if (!this->m_pSentinel)
        internalInsert(value, ref, std::move(nullptr));
    else
        internalInsert(value, ref, std::move(m_pSentinel->m_pPrev));
}

template <typename Traits>
void CircularDoublyLinkedList<Traits>::internalInsert(const value_type& value, Ref ref, NodePtr&& rParent)
{
    if (!m_pSentinel)
    {
        m_pSentinel = new Node({}, {}, nullptr, nullptr);
        m_pSentinel->m_pNext = m_pSentinel;
        m_pSentinel->m_pPrev = m_pSentinel;
        auto node = new Node(value, ref, m_pSentinel->m_pNext, m_pSentinel);
        m_pSentinel->m_pNext->m_pPrev = node;
        m_pSentinel->m_pNext = node;
        return;
    }

    auto prevNode = !rParent ? m_pSentinel : rParent;
    auto node = new Node(value, ref, prevNode->m_pNext, prevNode);
    prevNode->m_pNext->m_pPrev = node;
    prevNode->m_pNext = node;
    return;
}
#endif // __CIRCULARDOUBLYLINKEDLIST_H__