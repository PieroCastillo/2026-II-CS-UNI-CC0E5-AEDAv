#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__

#include <initializer_list>
#include <utility>
#include "containerbase.h"
#include "traits.h"

template <typename T>
class BinaryTreeNode : public GeneralNode<T> {
public:
    using value_type = T;
    using MySelf = BinaryTreeNode<T>;
    using Node = MySelf;
    using NodePtr = Node*;
    using Parent = GeneralNode<T>;
    using Parent::Parent; // Inherit constructor

    NodePtr m_parent = nullptr;
    NodePtr m_pChild[2] = { nullptr, nullptr }; // left and right children, left: [0] , right [1]

    static NodePtr getMaximum(NodePtr node) {
        if (!node) return nullptr;
        while (node->m_pChild[1]) node = node->m_pChild[1];
        return node;
    }

    static NodePtr getPredecessor(NodePtr node) {
        if (!node) return nullptr;

        if (node->m_pChild[0])
            return getMaximum(node->m_pChild[0]);

        auto* p = node->m_parent;
        while (p && node == p->m_pChild[0]) {
            node = p;
            p = p->m_parent;
        }
        return p;
    }

    static NodePtr getMinimum(NodePtr node) {
        if (!node)
            return nullptr;
        while (node->m_pChild[0])
            node = node->m_pChild[0];
        return node;
    }

    static NodePtr getSuccessor(NodePtr node) {
        if (!node)
            return nullptr;

        if (node->m_pChild[1])
            return getMinimum(node->m_pChild[1]);

        auto* p = node->m_parent;
        while (p && node == p->m_pChild[1]) {
            node = p;
            p = p->m_parent;
        }
        return p;
    }
};

template <typename T>
class BinaryTreeForwardIterator : public GeneralIterator<BinaryTreeForwardIterator<T>, BinaryTreeNode<T>> {
public:
    using value_type = T;
    using Node = BinaryTreeNode<T>;
    using NodePtr = Node*;
    using MySelf = BinaryTreeForwardIterator<T>;
    using Parent = GeneralIterator<MySelf, Node>;
    using Parent::Parent; // Inherit constructor
public:
    // Prefix increment
    BinaryTreeForwardIterator& operator++() { Parent::m_ptr = Node::getSuccessor(Parent::m_ptr); return *this; }
};

template <typename T>
class BinaryTreeBackwardIterator : public GeneralIterator<BinaryTreeBackwardIterator<T>, BinaryTreeNode<T>> {
public:
    using value_type = T;
    using Node = BinaryTreeNode<T>;
    using NodePtr = Node*;
    using MySelf = BinaryTreeBackwardIterator<T>;
    using Parent = GeneralIterator<MySelf, Node>;
    using Parent::Parent; // Inherit constructor
public:
    // Prefix increment
    BinaryTreeBackwardIterator& operator++() { Parent::m_ptr = Node::getPredecessor(Parent::m_ptr); return *this; }
};

template <typename T>
struct BinaryTreeAscTraits : public AscendingTraits<T> {
    using Node = BinaryTreeNode<T>;
    using ForwardIterator = BinaryTreeForwardIterator<T>;
    using BackwardIterator = BinaryTreeBackwardIterator<T>;
};

template <typename T>
struct BinaryTreeDescTraits : public DescendingTraits<T> {
    using Node = BinaryTreeNode<T>;
    using ForwardIterator = BinaryTreeBackwardIterator<T>;
    using BackwardIterator = BinaryTreeForwardIterator<T>;
};

template <typename Traits>
class BinaryTree : public ReversibleContainerBase<Traits> {
public:
    using value_type = Traits::value_type;
    using Node = Traits::Node;
    using NodePtr = Node*;
    using ForwardIterator = Traits::ForwardIterator;
    using BackwardIterator = Traits::BackwardIterator;
private:
    NodePtr m_root = nullptr; // root of the binary tree
    void internal_insert(NodePtr& node, const value_type& value, Ref ref, NodePtr parent) {
        if (!node) {
            node = new Node(value, ref);
            node->m_parent = parent;
            return;
        }
        auto branch = value > node->value();
        internal_insert(node->m_pChild[branch], value, ref, node); // Tail recursion to insert in the correct branch
    }
public:
    BinaryTree() = default;
    ~BinaryTree() override { clear(); }
    BinaryTree(std::initializer_list<std::pair<value_type, Ref>> values) { this->initialize(values); }
    BinaryTree(BinaryTree&& other) noexcept : m_root(std::exchange(other.m_root, nullptr)) {};
    BinaryTree& operator=(BinaryTree&& other) noexcept
    {
        if (this != &other)
        {
            clear(); // deletes original tree to receives the new one
            m_root = std::exchange(other.m_root, nullptr);
        }
        return *this;
    }

    void clear() override;
    bool empty() const { return m_root == nullptr; }
    void push_back(const value_type& value, Ref ref) override;
    ForwardIterator begin() const { return ForwardIterator(Node::getMinimum(m_root)); };
    ForwardIterator end() const { return ForwardIterator(nullptr); };
    BackwardIterator rbegin() const { return BackwardIterator(Node::getMaximum(m_root)); };
    BackwardIterator rend() const { return BackwardIterator(nullptr); };

    void insert(const value_type& value, Ref ref) {
        lock_guard<mutex> lock(this->m_mutex);
        internal_insert(m_root, value, ref, nullptr);
    }
    // Persistencia (operator << y operator >> globales para todas las estructuras pero si es para BT con indentacion entonces hacerlo por separado)
};

template <typename Traits>
void BinaryTree<Traits>::clear()
{
    lock_guard<mutex> lock(this->m_mutex);

    while (m_root)
    {
        if (m_root->m_pChild[0])
        {
            auto* left = m_root->m_pChild[0];
            m_root->m_pChild[0] = left->m_pChild[1];
            left->m_pChild[1] = m_root;
            m_root = left;
        }
        else
        {
            auto* next = m_root->m_pChild[1];
            delete m_root;
            m_root = next;
        }
    }
}

template <typename Traits>
void BinaryTree<Traits>::push_back(const value_type& value, Ref ref)
{
    insert(value, ref);
}
#endif // __BINARYTREE_H__