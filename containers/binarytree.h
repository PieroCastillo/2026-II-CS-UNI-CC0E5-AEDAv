#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__

#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "GeneralContainer.h"
#include "mixins.h"
#include "treeUtils.h"

#include <print>
#include <string>
#include <utility>
#include <mutex>

template <typename T, typename Derived>
class BaseBinaryTreeNode : public GeneralNode<T> {
public:
    using value_type = T;
    using MySelf = Derived;
    using Node = MySelf;
    using NodePtr = Node*;
    using Parent = GeneralNode<T>;

    using Parent::Parent;

    NodePtr m_pChild[2] = { nullptr, nullptr };
    NodePtr m_parent = nullptr;
};

template <typename T>
class BinaryTreeNode : public BaseBinaryTreeNode<T, BinaryTreeNode<T>> {
public:
    using Parent = BaseBinaryTreeNode<T, BinaryTreeNode<T>>;
    using Parent::Parent;
};

template <typename T, TX Dir, typename Node>
class BaseBinaryTreeIterator : public GeneralIterator<BaseBinaryTreeIterator<T, Dir, Node>, Node>
{
public:
    using MySelf = BaseBinaryTreeIterator<T, Dir, Node>;
    using Parent = GeneralIterator<MySelf, Node>;
    using NodePtr = Node*;

    using Parent::Parent;

    MySelf& operator++() {
        auto node = Parent::m_ptr;

        if (!node)
            return *this;

        if (node->m_pChild[1 - Dir]) {
            node = node->m_pChild[1 - Dir];

            while (node->m_pChild[Dir])
                node = node->m_pChild[Dir];

            Parent::m_ptr = node;
        }
        else {
            auto parent = node->m_parent;

            while (parent && node == parent->m_pChild[1 - Dir]) {
                node = parent;
                parent = parent->m_parent;
            }

            Parent::m_ptr = parent;
        }

        return *this;
    }
};

template <typename T>
using BinaryTreeForwardIterator =
BaseBinaryTreeIterator<T, 0, BinaryTreeNode<T>>;

template <typename T>
using BinaryTreeBackwardIterator =
BaseBinaryTreeIterator<T, 1, BinaryTreeNode<T>>;

template <typename T>
struct BinaryTreeTraits {
    using Node = BinaryTreeNode<T>;
    using ForwardIterator = BinaryTreeForwardIterator<T>;
    using BackwardIterator = BinaryTreeBackwardIterator<T>;
};

template <typename T>
struct BinaryTreeAscTraits : public AscendingTraits<T>, public BinaryTreeTraits<T> {};

template <typename T>
struct BinaryTreeDescTraits : public DescendingTraits<T>, public BinaryTreeTraits<T> {};

template <typename Traits, template <typename> typename... Mixins>
class BaseBinaryTree;

template <typename Traits>
using BinaryTree = BaseBinaryTree<Traits, Callable, Applicable, Searchable, ReverseCallable, Printable, Readable>;

template <typename Traits, template <typename> typename... Mixins>
class BaseBinaryTree : public GeneralContainer<Traits, Mixins...> {
public:
    using MySelf = BaseBinaryTree;
    using Parent = GeneralContainer<Traits, Mixins...>;
    using typename Parent::value_type;
    using typename Parent::Node;
    using typename Parent::NodePtr;
    using typename Parent::ForwardIterator;
    using BackwardIterator = typename Traits::BackwardIterator;
    using Compare = typename Traits::Compare;

protected:
    using Parent::m_mutex;

    NodePtr m_root = nullptr; // root of the binary tree
    virtual NodePtr internal_insert(NodePtr& node, const value_type& value, Ref ref, NodePtr parent = nullptr);
    virtual void printNode(const Node* node) const { std::print("{}", node->getValue()); }

    void transplant_unlocked(NodePtr u, NodePtr v) noexcept;

    virtual void remove_unlocked(NodePtr node);

public:
    BaseBinaryTree() = default;

    BaseBinaryTree(const BaseBinaryTree& other) {
        this->copy_from(other);
    }

    BaseBinaryTree(BaseBinaryTree&& other) noexcept {
        this->move_from(other, [&] {m_root = std::exchange(other.m_root, nullptr);});
    }

    virtual ~BaseBinaryTree() { clear(); }

    void insert(const value_type& value, Ref ref) {
        std::lock_guard lock(m_mutex);
        internal_insert(m_root, value, ref);
    }

    NodePtr root() noexcept {
        return m_root;
    }

    const Node* root() const noexcept {
        return m_root;
    }

    void clear() override;

    void clear_recursive(NodePtr node) noexcept {
        treeUtils::destroy(node);
    }

    void push_back(const value_type& value, Ref ref) override {
        insert(value, ref);
    }

    virtual void remove(NodePtr node);

    ForwardIterator begin() const override;
    ForwardIterator end() const override {
        return ForwardIterator(nullptr);
    }

    BackwardIterator rbegin() const;
    BackwardIterator rend() const {
        return BackwardIterator(nullptr);
    }

    BaseBinaryTree& operator=(const BaseBinaryTree& other);
    BaseBinaryTree& operator=(BaseBinaryTree&& other) noexcept;

    virtual void visualize() const;
};

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::internal_insert(NodePtr& node, const value_type& value, Ref ref, NodePtr parent) -> NodePtr
{
    if (!node) {
        node = new Node(value, ref);
        node->m_parent = parent;
        return node;
    }

    const auto branch = Compare{}(node->getValue(), value);

    internal_insert(node->m_pChild[branch], value, ref, node);
    return node;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::transplant_unlocked(NodePtr u, NodePtr v) noexcept
{
    if (!u->m_parent)
        m_root = v;
    else if (u == u->m_parent->m_pChild[0])
        u->m_parent->m_pChild[0] = v;
    else
        u->m_parent->m_pChild[1] = v;

    if (v)
        v->m_parent = u->m_parent;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::remove_unlocked(NodePtr node) {
    if (!node)
        return;

    if (!node->m_pChild[0]) {
        transplant_unlocked(node, node->m_pChild[1]);
    }
    else if (!node->m_pChild[1]) {
        transplant_unlocked(node, node->m_pChild[0]);
    }
    else {
        NodePtr successor = node->m_pChild[1];

        while (successor->m_pChild[0])
            successor = successor->m_pChild[0];

        if (successor->m_parent != node) {
            transplant_unlocked(successor, successor->m_pChild[1]);

            successor->m_pChild[1] = node->m_pChild[1];
            successor->m_pChild[1]->m_parent = successor;
        }

        transplant_unlocked(node, successor);

        successor->m_pChild[0] = node->m_pChild[0];
        successor->m_pChild[0]->m_parent = successor;
    }

    delete node;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::remove(NodePtr node) {
    if (!node)
        return;

    std::lock_guard lock(m_mutex);
    remove_unlocked(node);
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::clear() {
    std::lock_guard lock(m_mutex);

    clear_recursive(m_root);
    m_root = nullptr;
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::begin() const -> ForwardIterator {
    NodePtr curr = m_root;

    while (curr && curr->m_pChild[0])
        curr = curr->m_pChild[0];

    return ForwardIterator(curr);
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::rbegin() const -> BackwardIterator {
    NodePtr curr = m_root;

    while (curr && curr->m_pChild[1])
        curr = curr->m_pChild[1];

    return BackwardIterator(curr);
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::operator=(const BaseBinaryTree& other) -> BaseBinaryTree&
{
    if (this != &other)
        this->copy_from(other);

    return *this;
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::operator=(BaseBinaryTree&& other) noexcept -> BaseBinaryTree&
{
    if (this != &other)
        this->move_from(other, [&] {m_root = std::exchange(other.m_root, nullptr);});

    return *this;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::visualize() const {
    std::lock_guard lock(m_mutex);

    if (!m_root) {
        std::println("(empty tree)");
        return;
    }

    treeUtils::visualize(
        m_root,
        [](const Node* node) {
            return std::format("{}", node->getValue());
        }
    );
}

#endif // __MERKLE_TREE_H__