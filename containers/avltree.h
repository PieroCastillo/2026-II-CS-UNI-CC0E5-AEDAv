#ifndef __AVLTREE_H__
#define __AVLTREE_H__
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "GeneralContainer.h"
#include "mixins.h"
#include "binarytree.h"
#include <fstream>

template <typename T>
class AVLTreeNode : public BaseBinaryTreeNode<T, AVLTreeNode<T>> {
public:
    using value_type = T;
    using MySelf = AVLTreeNode<T>;
    using Node = MySelf;
    using NodePtr = Node*;
    using Parent = BaseBinaryTreeNode<T, AVLTreeNode<T>>;
    using Parent::Parent;

    size_t m_height = 1;
};

template <typename T>
using AvlTreeForwardIterator = BaseBinaryTreeIterator<T, 0, AVLTreeNode<T>>;

template <typename T>
using AvlTreeBackwardIterator = BaseBinaryTreeIterator<T, 1, AVLTreeNode<T>>;

template <typename T>
struct AvlTreeTraits : public BinaryTreeTraits<T> {
    using Node = AVLTreeNode<T>;
    using ForwardIterator = AvlTreeForwardIterator<T>;
    using BackwardIterator = AvlTreeBackwardIterator<T>;
};

template <typename T>
struct AvlTreeAscTraits : public AscendingTraits<T>, public AvlTreeTraits<T> {};

template <typename T>
struct AvlTreeDescTraits : public DescendingTraits<T>, public AvlTreeTraits<T> {};

// template <typename Traits>
// class AvlTree;

// template <typename Traits>
// std::ostream& operator<<(std::ostream& os, const AvlTree<Traits>& tree);

// template <typename Traits>
// std::istream& operator>>(std::istream& is, AvlTree<Traits>& tree);

template <typename Traits>
class AvlTree : public BinaryTree<Traits> {
public:
    using MySelf = AvlTree;
    using Parent = BinaryTree<Traits>;
    using typename Parent::value_type;
    using typename Parent::Node;
    using typename Parent::NodePtr;
    using typename Parent::ForwardIterator;
    using BackwardIterator = typename Traits::BackwardIterator;
    using Compare = typename Traits::Compare;
private:
    static int height(NodePtr node) noexcept {
        return node ? node->m_height : 0;
    }

    static int balance_factor(NodePtr node) noexcept {
        return height(node->m_pChild[0]) - height(node->m_pChild[1]);
    }

    static void update_height(NodePtr node) noexcept {
        node->m_height = 1 + std::max(height(node->m_pChild[0]), height(node->m_pChild[1]));
    }

    static void rotate_left(NodePtr& node) noexcept;
    static void rotate_right(NodePtr& node) noexcept;
    static void rebalance(NodePtr& node) noexcept;
protected:
    using Parent::m_mutex;
    using Parent::m_root;

    NodePtr internal_insert(NodePtr& node, const value_type& value, Ref ref, NodePtr parent = nullptr) override;
public:
    AvlTree() = default;
    AvlTree(const AvlTree& another) { *this = another; }
    AvlTree(AvlTree&& another) noexcept { *this = std::move(another); }

    void remove(NodePtr node) override;

    AvlTree& operator=(const AvlTree& another);
    AvlTree& operator=(AvlTree&& another) noexcept;

    auto operator[](const value_type& value) const->NodePtr;
};

template <typename Traits>
auto AvlTree<Traits>::operator[](const value_type& value) const -> NodePtr
{
    auto curr = m_root;
    Compare comp{};

    while (curr) {
        const auto& current = curr->getValue();

        if (comp(value, current))
            curr = curr->m_pChild[0];
        else if (comp(current, value))
            curr = curr->m_pChild[1];
        else
            return curr;
    }

    return nullptr;
}

template <typename Traits>
AvlTree<Traits>& AvlTree<Traits>::operator=(const AvlTree<Traits>& another)
{
    this->copy_from(another);
    return *this;
}

template <typename Traits>
AvlTree<Traits>& AvlTree<Traits>::operator=(AvlTree<Traits>&& another) noexcept
{
    this->move_from(another, [&] { m_root = std::exchange(another.m_root, nullptr); });
    return *this;
}

template <typename Traits>
auto AvlTree<Traits>::internal_insert(NodePtr& node, const value_type& value, Ref ref, NodePtr parent) -> NodePtr
{
    if (!node)
        return Parent::internal_insert(node, value, ref, parent);

    const auto branch = Compare()(node->getValue(), value);
    internal_insert(node->m_pChild[branch], value, ref, node);
    rebalance(node);
    return node;
}

template <typename Traits>
void AvlTree<Traits>::remove(NodePtr node)
{
    if (!node)
        return;

    NodePtr start = nullptr;

    if (!node->m_pChild[0] || !node->m_pChild[1]) {
        start = node->m_parent;
    }
    else {
        auto successor = node->m_pChild[1];

        while (successor->m_pChild[0])
            successor = successor->m_pChild[0];

        auto successor_parent = successor->m_parent;
        start = (successor_parent == node) ? successor : successor_parent;
    }

    Parent::remove(node);

    while (start) {
        NodePtr& subtree = !start->m_parent ? this->m_root : (start == start->m_parent->m_pChild[0] ? start->m_parent->m_pChild[0] : start->m_parent->m_pChild[1]);
        rebalance(subtree);
        start = subtree->m_parent;
    }
}

template <typename Traits>
void AvlTree<Traits>::rotate_left(NodePtr& node) noexcept {
    NodePtr x = node;
    NodePtr y = x->m_pChild[1];
    NodePtr beta = y->m_pChild[0];

    y->m_parent = x->m_parent;
    y->m_pChild[0] = x;

    x->m_parent = y;
    x->m_pChild[1] = beta;

    if (beta)
        beta->m_parent = x;

    node = y;

    update_height(x);
    update_height(y);
}

template <typename Traits>
void AvlTree<Traits>::rotate_right(NodePtr& node) noexcept {
    auto y = node;
    auto x = y->m_pChild[0];
    auto beta = x->m_pChild[1];

    x->m_parent = y->m_parent;
    x->m_pChild[1] = y;

    y->m_parent = x;
    y->m_pChild[0] = beta;

    if (beta)
        beta->m_parent = y;

    node = x;

    update_height(y);
    update_height(x);
}

template <typename Traits>
void AvlTree<Traits>::rebalance(NodePtr& node) noexcept {
    if (!node)
        return;

    update_height(node);

    const int balance = balance_factor(node);

    if (balance > 1) {
        if (balance_factor(node->m_pChild[0]) < 0)
            rotate_left(node->m_pChild[0]);

        rotate_right(node);
    }
    else if (balance < -1) {
        if (balance_factor(node->m_pChild[1]) > 0)
            rotate_right(node->m_pChild[1]);

        rotate_left(node);
    }
}
#endif // __AVLTREE_H__