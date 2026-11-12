#ifndef __BINARYTREE_H__
#define __BINARYTREE_H__
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "GeneralContainer.h"
#include "mixins.h"
#include <print>
#include <string>

template <typename T, typename Derivated>
class BaseBinaryTreeNode : public GeneralNode<T> {
public:
    using value_type = T;
    using MySelf = Derivated;
    using Node = MySelf;
    using NodePtr = Node*;
    using Parent = GeneralNode<T>;
    using Parent::Parent; // Inherit constructor

    NodePtr m_pChild[2] = { nullptr, nullptr }; // left and right children
    NodePtr m_parent = nullptr;

};

template <typename T>
class BinaryTreeNode : public BaseBinaryTreeNode<T, BinaryTreeNode<T>>
{
public:
    using Parent = BaseBinaryTreeNode<T, BinaryTreeNode<T>>;
    using Parent::Parent;
};

template <typename T, TX Dir, typename Node>
class BaseBinaryTreeIterator : public GeneralIterator<BaseBinaryTreeIterator<T, Dir, Node>, Node> {
public:
    using MySelf = BaseBinaryTreeIterator<T, Dir, Node>;
    using Parent = GeneralIterator<MySelf, Node>;
    using NodePtr = Node*;
    using Parent::Parent;
    NodePtr m_parent = nullptr;

    MySelf& operator++();
};

template <typename T, TX Dir, typename Node>
auto BaseBinaryTreeIterator<T, Dir, Node>::operator++() -> MySelf& {
    auto node = Parent::m_ptr;
    if (node == nullptr)
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

template <typename T>
using BinaryTreeForwardIterator = BaseBinaryTreeIterator<T, 0, BinaryTreeNode<T>>;

template <typename T>
using BinaryTreeBackwardIterator = BaseBinaryTreeIterator<T, 1, BinaryTreeNode<T>>;

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
    void visualize_recursive(NodePtr node, const std::string& prefix, bool isLast, bool isRoot) const;
public:
    BaseBinaryTree() = default;
    BaseBinaryTree(const BaseBinaryTree& another) { *this = another; }
    BaseBinaryTree(BaseBinaryTree&& another) noexcept { *this = std::move(another); }
    virtual ~BaseBinaryTree() { clear(); }

    void insert(const value_type& value, Ref ref) {
        std::lock_guard lock(this->m_mutex);
        internal_insert(m_root, value, ref);
    }
    // Move constructor and move assignment operator
    // Constructor and destructor
    // Persistencia (operator << y operator >> globales para todas las estructuras pero si es para BT con indentacion entonces hacerlo por separado)
    // Concurrencia
    // Iteradores (forward y backward)
    // FirstThat, ForEach, call, rcall por herencia

    NodePtr root() const noexcept { return m_root; }

    void clear() override;
    void push_back(const value_type& value, Ref ref) override { insert(value, ref); }
    void clear_recursive(NodePtr node);
    virtual void remove(NodePtr node);

    ForwardIterator begin() const override;
    ForwardIterator end() const override { return ForwardIterator(nullptr); }

    BackwardIterator rbegin() const;
    BackwardIterator rend() const { return BackwardIterator(nullptr); }

    BaseBinaryTree& operator=(const BaseBinaryTree& another);
    BaseBinaryTree& operator=(BaseBinaryTree&& another) noexcept;

    void visualize() const;
};
template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::visualize() const {
    std::lock_guard lock(this->m_mutex);

    if (!m_root) {
        std::println("(empty tree)");
        return;
    }

    visualize_recursive(m_root, "", true, true);
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::visualize_recursive(NodePtr node, const std::string& prefix, bool isLast, bool isRoot) const {
    if (!node)
        return;

    if (isRoot) {
        printNode(node);
        std::println();
    }
    else {
        std::print("{}{}", prefix, isLast ? "└── " : "├── ");
        printNode(node);
        std::println();
    }

    const auto nextPrefix = prefix + (isRoot ? "" : (isLast ? "    " : "│   "));

    if (node->m_pChild[1]) // right child
        visualize_recursive(node->m_pChild[1], nextPrefix, !node->m_pChild[0], false);

    if (node->m_pChild[0]) // left child
        visualize_recursive(node->m_pChild[0], nextPrefix, true, false);
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::internal_insert(NodePtr& node, const value_type& value, Ref ref, NodePtr parent) -> NodePtr {
    if (!node) {
        node = new Node(value, ref);
        node->m_parent = parent;
        return node;
    }
    auto branch = Compare()(node->getValue(), value);
    internal_insert(node->m_pChild[branch], value, ref, node);
    return node;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::clear() {
    std::lock_guard lock(this->m_mutex);
    clear_recursive(m_root);
    m_root = nullptr;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::clear_recursive(NodePtr node) {
    if (!node) return;
    clear_recursive(node->m_pChild[0]);
    clear_recursive(node->m_pChild[1]);
    delete node;
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::begin() const -> ForwardIterator {
    NodePtr curr = m_root;
    while (curr && curr->m_pChild[0]) curr = curr->m_pChild[0];
    return ForwardIterator(curr);
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::rbegin() const -> BackwardIterator {
    NodePtr curr = m_root;
    while (curr && curr->m_pChild[1]) curr = curr->m_pChild[1];
    return BackwardIterator(curr);
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::operator=(const BaseBinaryTree& another) -> BaseBinaryTree& {
    this->copy_from(another);
    return *this;
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseBinaryTree<Traits, Mixins...>::operator=(BaseBinaryTree&& another) noexcept -> BaseBinaryTree& {
    this->move_from(another, [&] { m_root = std::exchange(another.m_root, nullptr); });
    return *this;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseBinaryTree<Traits, Mixins...>::remove(NodePtr node) {
    if (!node)
        return;

    std::lock_guard lock(this->m_mutex);

    auto transplant = [&](NodePtr u, NodePtr v) {
        if (!u->m_parent) {
            m_root = v;
        }
        else if (u == u->m_parent->m_pChild[0]) {
            u->m_parent->m_pChild[0] = v;
        }
        else {
            u->m_parent->m_pChild[1] = v;
        }

        if (v)
            v->m_parent = u->m_parent;
        };

    if (!node->m_pChild[0]) {
        transplant(node, node->m_pChild[1]);
    }
    else if (!node->m_pChild[1]) {
        transplant(node, node->m_pChild[0]);
    }
    else {
        NodePtr successor = node->m_pChild[1];

        while (successor->m_pChild[0])
            successor = successor->m_pChild[0];

        if (successor->m_parent != node) {
            transplant(successor, successor->m_pChild[1]);

            successor->m_pChild[1] = node->m_pChild[1];
            successor->m_pChild[1]->m_parent = successor;
        }

        transplant(node, successor);

        successor->m_pChild[0] = node->m_pChild[0];
        successor->m_pChild[0]->m_parent = successor;
    }

    delete node;
}
#endif // __BINARYTREE_H__