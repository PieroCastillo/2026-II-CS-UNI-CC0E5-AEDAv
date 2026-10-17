#ifndef __TRAITS_H__
#define __TRAITS_H__

#include <functional>
#include <istream>
#include "GeneralIterator.h"

template <typename T, typename _Compare>
struct DefaultTraits {
    using value_type = T;
    using Compare = _Compare;
    using Delim = std::istream::char_type;
};
template <typename T, typename _Compare = std::less<T>>
struct AscendingTraits : public DefaultTraits<T, _Compare> {};

template <typename T, typename _Compare = std::greater<T>>
struct DescendingTraits : public DefaultTraits<T, _Compare> {};

template <typename T>
class VectorForwardIterator : public GeneralIterator<VectorForwardIterator<T>, T> {
public:
    using value_type = T;
    using MySelf = VectorForwardIterator<T>;
    using Parent = GeneralIterator<MySelf, T>;
    using Parent::Parent; // Inherit constructor

    // Prefix increment
    VectorForwardIterator& operator++() { ++Parent::m_ptr; return *this; }
};

template <typename T>
class VectorBackwardIterator : public GeneralIterator<VectorBackwardIterator<T>, T> {
public:
    using value_type = T;
    using MySelf = VectorBackwardIterator<T>;
    using Parent = GeneralIterator<MySelf, T>;
    using Parent::Parent; // Inherit constructor

    // Prefix increment
    VectorBackwardIterator& operator++() { --Parent::m_ptr; return *this; }
};

template <typename T>
struct VectorAscTraits : AscendingTraits<T> {
    using value_type = T;
    using Node = GeneralNode<T>;
    using ForwardIterator = VectorForwardIterator<Node>;  // itera sobre Node, no sobre T
    using BackwardIterator = VectorBackwardIterator<Node>; // itera sobre Node, no sobre T
};

template <typename T>
class HeapForwardIterator : public GeneralIterator<HeapForwardIterator<T>, T> {
public:
    using value_type = T;
    using MySelf = HeapForwardIterator<T>;
    using Parent = GeneralIterator<MySelf, T>;
    using Parent::Parent; // Inherit constructor

    // Prefix increment
    HeapForwardIterator& operator++() { ++Parent::m_ptr; return *this; }
};

template <typename T, typename _Compare = std::less<T>>
struct MinHeapTraits : DefaultTraits<T, _Compare> {
    using value_type = T;
    using Node = GeneralNode<T>;
    using ForwardIterator = HeapForwardIterator<Node>;
};

template <typename T, typename _Compare = std::greater<T>>
struct MaxHeapTraits : DefaultTraits<T, _Compare> {
    using value_type = T;
    using Node = GeneralNode<T>;
    using ForwardIterator = HeapForwardIterator<Node>;
};
#endif // __TRAITS_H__