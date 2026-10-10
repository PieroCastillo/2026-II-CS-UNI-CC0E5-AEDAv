#ifndef __CIRCULARLINKEDLIST_H__
#define __CIRCULARLINKEDLIST_H__
#include <mutex>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "traits.h"
#include "../foreach.h"
#include "linkedlist.h"

template <typename Traits>
class CircularList : public LinkedList<Traits>
{
public:
    using value_type = typename Traits::value_type;
    using Node = typename Traits::Node;
    using NodePtr = Node*;
    using ForwardIterator = typename Traits::ForwardIterator;
    using Compare = typename Traits::Compare;
    using Delim = typename Node::Delim;

protected:
    NodePtr m_pSentinel = nullptr;

    NodePtr GetRoot() const override { return m_pSentinel; }
    void internalInsert(const value_type& value, Ref ref, NodePtr&& rParent) override;
public:
    CircularList& operator=(const CircularList& another);
    void clear() override;
    void push_back(const value_type& value, Ref ref) override;
    bool empty() const override { return m_pSentinel == nullptr; }

    ForwardIterator begin() const override { return ForwardIterator(m_pSentinel ? m_pSentinel->m_pNext : nullptr); }
    ForwardIterator end() const override { return ForwardIterator(m_pSentinel); }
};

template <typename Traits>
CircularList<Traits>& CircularList<Traits>::operator=(const CircularList<Traits>& other)
{
    if (this == &other)
        return *this;

    clear();

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
void CircularList<Traits>::clear()
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
void CircularList<Traits>::push_back(const value_type& value, Ref ref)
{
    scoped_lock lock(this->m_mutex);
    if (!m_pSentinel)
    {
        NodePtr dummy = nullptr;
        internalInsert(value, ref, std::move(dummy));
        return;
    }

    auto curr = m_pSentinel;
    while (curr->m_pNext != m_pSentinel)
        curr = curr->m_pNext;

    internalInsert(value, ref, std::move(curr));
}

template <typename Traits>
void CircularList<Traits>::internalInsert(const value_type& value, Ref ref, NodePtr&& rParent)
{
    if (!m_pSentinel)
    {
        auto dummyValue = value_type{};
        m_pSentinel = new Node(dummyValue, Ref{}, nullptr);
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