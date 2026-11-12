#ifndef __MERKLE_TREE_H__
#define __MERKLE_TREE_H__

#include <vector>
#include <mutex>
#include <print>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include "GeneralContainer.h"
#include "BinaryTree.h"

template <typename T, typename Digest, typename Hasher>
struct MerkleTraits : DefaultTraits<Digest, std::less<Digest>>, BinaryTreeTraits<Digest>
{
    using payload_type = T;
    using digest_type = Digest;
    using Hash = Hasher;
};

template <typename Traits, template <typename> typename... Mixins>
class BaseMerkleTree;

template <typename T, typename Digest, typename Hasher>
using MerkleTree = BaseMerkleTree<MerkleTraits<T, Digest, Hasher>, Callable, Applicable, Printable, Readable>;

template <typename Traits, template <typename> typename... Mixins>
class BaseMerkleTree : public GeneralContainer<Traits, Mixins...> {
public:
    using Parent = GeneralContainer<Traits, Mixins...>;
    using payload_type = typename Traits::payload_type;
    using digest_type = typename Traits::digest_type;
    using Hash = typename Traits::Hash;

    using typename Parent::Node;
    using typename Parent::NodePtr;
    using typename Parent::ForwardIterator;
protected:
    using Parent::m_mutex;
private:
    NodePtr m_root = nullptr;
    std::vector<payload_type> m_leaves;

    digest_type hash_leaf(const payload_type& value) const { return Hash{}.hash_leaf(value); }
    digest_type hash_pair(const digest_type& left, const digest_type& right) const { return Hash{}.hash_pair(left, right); }

    NodePtr make_node(const digest_type& hash, NodePtr left = nullptr, NodePtr right = nullptr);
    void clear_recursive(NodePtr node);
    void rebuild();

protected:
    virtual void printNode(const Node* node) const { std::print("{}", node->getValue()); }
    void visualize_recursive(NodePtr node, const std::string& prefix, bool isLast, bool isRoot) const;

public:
    BaseMerkleTree() = default;
    BaseMerkleTree(const BaseMerkleTree& other);
    BaseMerkleTree(BaseMerkleTree&& other) noexcept;

    virtual ~BaseMerkleTree() {
        clear();
    }

    BaseMerkleTree& operator=(const BaseMerkleTree& other);
    BaseMerkleTree& operator=(BaseMerkleTree&& other) noexcept;

    size_t size() const;
    bool empty() const;
    void insert(const payload_type& value);
    void push_back(const digest_type& value, Ref) override;
    void clear() override;
    void update(size_t index, const payload_type& value);

    const payload_type& operator[](size_t index) const;

    digest_type root_hash() const;
    bool verify_root() const;

    ForwardIterator begin() const override;
    ForwardIterator end() const override { return ForwardIterator(nullptr); }

    void visualize() const;

    static std::ostream& write(std::ostream& os, const BaseMerkleTree& tree);
    static std::istream& read(std::istream& is, BaseMerkleTree& tree);

    friend std::ostream& operator<<(std::ostream& os, const BaseMerkleTree& tree) { return write(os, tree); }
    friend std::istream& operator>>(std::istream& is, BaseMerkleTree& tree) { return read(is, tree); }
};

template <typename Traits, template <typename> typename... Mixins>
void BaseMerkleTree<Traits, Mixins...>::visualize() const {
    std::lock_guard lock(m_mutex);

    if (!m_root) {
        std::println("(empty MerkleTree)");
        return;
    }

    std::println("MerkleTree");
    visualize_recursive(m_root, "", true, true);
}

template <typename Traits, template <typename> typename... Mixins>
void BaseMerkleTree<Traits, Mixins...>::visualize_recursive(NodePtr node, const std::string& prefix, bool isLast, bool isRoot) const {
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

    if (node->m_pChild[1])
        visualize_recursive(node->m_pChild[1], nextPrefix, !node->m_pChild[0], false);

    if (node->m_pChild[0])
        visualize_recursive(node->m_pChild[0], nextPrefix, true, false);
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseMerkleTree<Traits, Mixins...>::make_node(const digest_type& hash, NodePtr left, NodePtr right) -> NodePtr
{
    NodePtr node = new Node(hash, Ref{});
    node->m_pChild[0] = left;
    node->m_pChild[1] = right;

    if (left) left->m_parent = node;
    if (right) right->m_parent = node;

    return node;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseMerkleTree<Traits, Mixins...>::clear_recursive(NodePtr node) {
    if (!node) return;

    clear_recursive(node->m_pChild[0]);
    clear_recursive(node->m_pChild[1]);
    delete node;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseMerkleTree<Traits, Mixins...>::rebuild() {
    clear_recursive(m_root);
    m_root = nullptr;

    if (m_leaves.empty()) return;

    std::vector<NodePtr> level;
    level.reserve(m_leaves.size());

    for (const auto& value : m_leaves)
        level.push_back(make_node(hash_leaf(value)));

    while (level.size() > 1) {
        std::vector<NodePtr> next;
        next.reserve((level.size() + 1) / 2);

        for (size_t i = 0; i < level.size(); i += 2) {
            if (i + 1 == level.size()) {
                next.push_back(level[i]);
                continue;
            }

            auto left = level[i];
            auto right = level[i + 1];

            auto hash = hash_pair(
                left->getValue(),
                right->getValue()
            );

            next.push_back(make_node(hash, left, right));
        }

        level = std::move(next);
    }

    m_root = level.front();
    m_root->m_parent = nullptr;
}

template <typename Traits, template <typename> typename... Mixins>
BaseMerkleTree<Traits, Mixins...>::BaseMerkleTree(const BaseMerkleTree& other) {
    std::lock_guard lock(other.m_mutex);
    m_leaves = other.m_leaves;
    rebuild();
}

template <typename Traits, template <typename> typename... Mixins>
BaseMerkleTree<Traits, Mixins...>::BaseMerkleTree(BaseMerkleTree&& other) noexcept {
    std::lock_guard lock(other.m_mutex);
    m_leaves = std::move(other.m_leaves);
    m_root = std::exchange(other.m_root, nullptr);
}

template <typename Traits, template <typename> typename... Mixins>
BaseMerkleTree<Traits, Mixins...>& BaseMerkleTree<Traits, Mixins...>::operator=(const BaseMerkleTree& other) {
    if (this == &other) return *this;

    std::scoped_lock lock(m_mutex, other.m_mutex);
    m_leaves = other.m_leaves;
    rebuild();
    return *this;
}

template <typename Traits, template <typename> typename... Mixins>
BaseMerkleTree<Traits, Mixins...>& BaseMerkleTree<Traits, Mixins...>::operator=(BaseMerkleTree&& other) noexcept {
    if (this == &other) return *this;

    std::scoped_lock lock(m_mutex, other.m_mutex);

    clear_recursive(m_root);
    m_root = nullptr;

    m_leaves = std::move(other.m_leaves);
    m_root = std::exchange(other.m_root, nullptr);
    return *this;
}

template <typename Traits, template <typename> typename... Mixins>
size_t BaseMerkleTree<Traits, Mixins...>::size() const {
    std::lock_guard lock(m_mutex);
    return m_leaves.size();
}

template <typename Traits, template <typename> typename... Mixins>
bool BaseMerkleTree<Traits, Mixins...>::empty() const {
    std::lock_guard lock(m_mutex);
    return m_leaves.empty();
}

template <typename Traits, template <typename> typename... Mixins>
void BaseMerkleTree<Traits, Mixins...>::insert(const payload_type& value) {
    std::lock_guard lock(m_mutex);
    m_leaves.push_back(value);
    rebuild();
}

template <typename Traits, template <typename> typename... Mixins>
void BaseMerkleTree<Traits, Mixins...>::push_back(const digest_type& value, Ref ref) {
    if constexpr (std::is_same_v<payload_type, digest_type>)
        insert(value);
    else
        throw std::logic_error("MerkleTree::push_back(digest) is unsupported; use insert(payload)");
}

template <typename Traits, template <typename> typename... Mixins>
void BaseMerkleTree<Traits, Mixins...>::clear() {
    std::lock_guard lock(m_mutex);
    clear_recursive(m_root);
    m_root = nullptr;
    m_leaves.clear();
}

template <typename Traits, template <typename> typename... Mixins>
void BaseMerkleTree<Traits, Mixins...>::update(size_t index, const payload_type& value) {
    std::lock_guard lock(m_mutex);

    m_leaves.at(index) = value;
    rebuild();
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseMerkleTree<Traits, Mixins...>::operator[](size_t index) const -> const typename BaseMerkleTree<Traits, Mixins...>::payload_type& {
    std::lock_guard lock(m_mutex);
    return m_leaves.at(index);
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseMerkleTree<Traits, Mixins...>::root_hash() const -> typename BaseMerkleTree<Traits, Mixins...>::digest_type {
    std::lock_guard lock(m_mutex);

    if (!m_root)
        throw std::out_of_range("Empty MerkleTree");

    return m_root->getValue();
}

template <typename Traits, template <typename> typename... Mixins>
bool BaseMerkleTree<Traits, Mixins...>::verify_root() const {
    std::lock_guard lock(m_mutex);

    if (m_leaves.empty())
        return m_root == nullptr;

    std::vector<digest_type> level;
    level.reserve(m_leaves.size());

    for (const auto& value : m_leaves)
        level.push_back(hash_leaf(value));

    while (level.size() > 1) {
        std::vector<digest_type> next;
        next.reserve((level.size() + 1) / 2);

        for (size_t i = 0; i < level.size(); i += 2) {
            if (i + 1 == level.size())
                next.push_back(level[i]);
            else
                next.push_back(hash_pair(level[i], level[i + 1]));
        }

        level = std::move(next);
    }

    return m_root && m_root->getValue() == level.front();
}

template <typename Traits, template <typename> typename... Mixins>
auto BaseMerkleTree<Traits, Mixins...>::begin() const -> typename BaseMerkleTree<Traits, Mixins...>::ForwardIterator {
    NodePtr curr = m_root;

    while (curr && curr->m_pChild[0])
        curr = curr->m_pChild[0];

    return ForwardIterator(curr);
}

template <typename Traits, template <typename> typename... Mixins>
std::ostream& BaseMerkleTree<Traits, Mixins...>::write(std::ostream& os, const BaseMerkleTree& tree)
{
    std::lock_guard lock(tree.m_mutex);
    os << tree.m_leaves.size() << '\n';

    for (const auto& value : tree.m_leaves)
        os << value << '\n';

    return os;
}

template <typename Traits, template <typename> typename... Mixins>
std::istream& BaseMerkleTree<Traits, Mixins...>::read(std::istream& is, BaseMerkleTree& tree)
{
    size_t count;
    if (!(is >> count)) return is;

    BaseMerkleTree temp;

    for (size_t i = 0; i < count; ++i) {
        payload_type value{};

        if (!(is >> value))
            return is;

        temp.m_leaves.push_back(std::move(value));
    }

    temp.rebuild();

    std::lock_guard lock(tree.m_mutex);
    tree.m_leaves = std::move(temp.m_leaves);
    tree.clear_recursive(tree.m_root);
    tree.m_root = std::exchange(temp.m_root, nullptr);

    return is;
}

#endif // __MERKLE_TREE_H__