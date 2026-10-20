#ifndef __HEAP_H__
#define __HEAP_H__

#include "containerbase.h"

template <typename Traits>
class Heap : public ContainerBase<Traits> {
public:
    using value_type = typename Traits::value_type;
    using ForwardIterator = Traits::ForwardIterator;
    using Node = typename Traits::Node;
    using NodePtr = Node*;
    using Compare = typename Traits::Compare;
private:
    mutable Vector<VectorAscTraits<value_type>> m_data;
    ForwardIterator begin() const override { return ForwardIterator(m_data.data()); }
    ForwardIterator end() const override { return ForwardIterator(m_data.data() + m_data.size()); }

public:
    void push_back(const value_type& value, Ref ref) override {
        m_data.push_back(value, ref);
        HeapifyUp(m_data.size() - 1);
    }

    void clear() override { m_data.clear(); }
    bool empty() const { return m_data.empty(); }

    void HeapifyUp(size_t index) {
        while (index > 0) {
            size_t parentIndex = (index - 1) / 2;
            if (Compare()(m_data[index].getValue(), m_data[parentIndex].getValue())) {
                std::swap(m_data[index], m_data[parentIndex]);
                index = parentIndex;
            }
            else {
                break;
            }
        }
    }

    void HeapifyDown(size_t index) {
        const size_t count = m_data.size();

        while (index < count) {
            size_t best = index;
            size_t left = 2 * index + 1;
            size_t right = 2 * index + 2;

            if (left < count && Compare()(m_data[left].getValue(), m_data[best].getValue()))
                best = left;

            if (right < count && Compare()(m_data[right].getValue(), m_data[best].getValue()))
                best = right;

            if (best == index)
                break;

            std::swap(m_data[index], m_data[best]);
            index = best;
        }
    }

    Node pop() {
        if (m_data.empty()) throw std::out_of_range("Heap is empty");

        std::swap(m_data[0], m_data[m_data.size() - 1]);
        Node result = std::move(m_data[m_data.size() - 1]);
        m_data.pop_back();

        if (!m_data.empty())
            HeapifyDown(0);

        return result;
    }
};

template <typename T>
class MinHeap : public Heap<MinHeapTraits<T>> {};

template <typename T>
class MaxHeap : public Heap<MaxHeapTraits<T>> {};
#endif // __HEAP_H__