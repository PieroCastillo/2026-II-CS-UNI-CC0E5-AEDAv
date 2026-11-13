#ifndef __MERKLE_TREE_H__
#define __MERKLE_TREE_H__

#include <vector>
#include <mutex>
#include <print>
#include <type_traits>
#include <utility>
#include <stdexcept>
#include <memory>
#include <istream>
#include <ostream>

#include "GeneralContainer.h"
#include "binarytree.h"
#include "treeUtils.h"

template <typename T, typename Digest, typename Hasher>
struct MerkleTraits :
    DefaultTraits<Digest, std::less<Digest>>,
    BinaryTreeTraits<Digest>
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
class BaseMerkleTree : public GeneralContainer<Traits, Mixins...>
{
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

    digest_type hash_leaf(const payload_type& value) const {
        return Hash{}.hash_leaf(value);
    }

    digest_type hash_pair(const digest_type& left, const digest_type& right) const
    {
        return Hash{}.hash_pair(left, right);
    }

    NodePtr make_node(
        const digest_type& hash,
        NodePtr left = nullptr,
        NodePtr right = nullptr);

    void rebuild();
    void clear_unlocked() noexcept;

protected:
    virtual void printNode(const Node* node) const {
        std::print("{}", node->getValue());
    }

public:
    BaseMerkleTree() = default;

    BaseMerkleTree(const BaseMerkleTree& other) {
        std::lock_guard lock(other.m_mutex);

        m_leaves = other.m_leaves;
        rebuild();
    }

    BaseMerkleTree(BaseMerkleTree&& other) noexcept {
        std::lock_guard lock(other.m_mutex);

        m_leaves = std::move(other.m_leaves);
        m_root = std::exchange(other.m_root, nullptr);
    }

    virtual ~BaseMerkleTree() {
        clear();
    }

    BaseMerkleTree& operator=(const BaseMerkleTree& other) {
        if (this == &other)
            return *this;

        BaseMerkleTree temp(other);

        std::lock_guard lock(m_mutex);

        clear_unlocked();

        m_leaves = std::move(temp.m_leaves);
        m_root = std::exchange(temp.m_root, nullptr);

        return *this;
    }

    BaseMerkleTree& operator=(BaseMerkleTree&& other) noexcept {
        if (this == &other)
            return *this;

        std::scoped_lock lock(m_mutex, other.m_mutex);

        clear_unlocked();

        m_leaves = std::move(other.m_leaves);
        m_root = std::exchange(other.m_root, nullptr);

        return *this;
    }

    size_t size() const {
        std::lock_guard lock(m_mutex);
        return m_leaves.size();
    }

    bool empty() const {
        std::lock_guard lock(m_mutex);
        return m_leaves.empty();
    }

    void insert(const payload_type& value) {
        std::lock_guard lock(m_mutex);

        m_leaves.push_back(value);

        try {
            rebuild();
        }
        catch (...) {
            m_leaves.pop_back();
            throw;
        }
    }

    void push_back(const digest_type& value, Ref) override {
        if constexpr (std::is_same_v<payload_type, digest_type>)
            insert(value);
        else
            throw std::logic_error("MerkleTree::push_back(digest) is unsupported; ""use insert(payload)");
    }

    void clear() override {
        std::lock_guard lock(m_mutex);
        clear_unlocked();
        m_leaves.clear();
    }

    void update(size_t index, const payload_type& value) {
        std::lock_guard lock(m_mutex);

        auto previous = m_leaves.at(index);
        m_leaves[index] = value;

        try {
            rebuild();
        }
        catch (...) {
            m_leaves[index] = std::move(previous);
            throw;
        }
    }

    const payload_type& operator[](size_t index) const {
        std::lock_guard lock(m_mutex);
        return m_leaves.at(index);
    }

    digest_type root_hash() const {
        std::lock_guard lock(m_mutex);

        if (!m_root)
            throw std::out_of_range("Empty MerkleTree");

        return m_root->getValue();
    }

    bool verify_root() const {
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

    ForwardIterator begin() const override {
        NodePtr curr = m_root;

        while (curr && curr->m_pChild[0])
            curr = curr->m_pChild[0];

        return ForwardIterator(curr);
    }

    ForwardIterator end() const override {
        return ForwardIterator(nullptr);
    }

    void visualize() const {
        std::lock_guard lock(m_mutex);

        if (!m_root) {
            std::println("(empty MerkleTree)");
            return;
        }

        std::println("MerkleTree");

        treeUtils::visualize(m_root, "", true, true,
            [this](const Node* node) {
                printNode(node);
            });
    }

    static std::ostream& write(std::ostream& os, const BaseMerkleTree& tree)
    {
        std::lock_guard lock(tree.m_mutex);

        os << tree.m_leaves.size() << '\n';

        for (const auto& value : tree.m_leaves) {
            os << value << '\n';

            if (!os)
                break;
        }

        return os;
    }

    static std::istream& read(std::istream& is, BaseMerkleTree& tree)
    {
        size_t count{};

        if (!(is >> count))
            return is;

        BaseMerkleTree temp;

        for (size_t i = 0; i < count; ++i) {
            payload_type value{};

            if (!(is >> value))
                return is;

            temp.m_leaves.push_back(std::move(value));
        }

        try {
            temp.rebuild();
        }
        catch (...) {
            is.setstate(std::ios::failbit);
            return is;
        }

        std::lock_guard lock(tree.m_mutex);

        tree.clear_unlocked();

        tree.m_leaves = std::move(temp.m_leaves);
        tree.m_root = std::exchange(temp.m_root, nullptr);

        return is;
    }

    friend std::ostream& operator<<(std::ostream& os, const BaseMerkleTree& tree) { return write(os, tree); }
    friend std::istream& operator>>(std::istream& is, BaseMerkleTree& tree) { return read(is, tree); }
};

template <typename Traits, template <typename> typename... Mixins>
auto BaseMerkleTree<Traits, Mixins...>::make_node(const digest_type& hash, NodePtr left, NodePtr right) -> NodePtr
{
    std::unique_ptr<Node> node = std::make_unique<Node>(hash, Ref{});

    node->m_pChild[0] = left;
    node->m_pChild[1] = right;

    if (left)
        left->m_parent = node.get();

    if (right)
        right->m_parent = node.get();

    return node.release();
}

template <typename Traits, template <typename> typename... Mixins>
void BaseMerkleTree<Traits, Mixins...>::clear_unlocked() noexcept {
    treeUtils::destroy(m_root);
    m_root = nullptr;
}

template <typename Traits, template <typename> typename... Mixins>
void BaseMerkleTree<Traits, Mixins...>::rebuild() {
    std::vector<std::unique_ptr<Node>> owned;
    std::vector<NodePtr> level;

    owned.reserve(m_leaves.size() * 2);
    level.reserve(m_leaves.size());

    for (const auto& value : m_leaves) {
        auto node = std::make_unique<Node>(hash_leaf(value), Ref{});

        level.push_back(node.get());
        owned.push_back(std::move(node));
    }

    while (level.size() > 1) {
        std::vector<NodePtr> next;
        next.reserve((level.size() + 1) / 2);

        for (size_t i = 0; i < level.size(); i += 2) {
            if (i + 1 == level.size()) {
                next.push_back(level[i]);
                continue;
            }

            NodePtr left = level[i];
            NodePtr right = level[i + 1];

            const auto hash = hash_pair(left->getValue(), right->getValue());
            auto parent = std::make_unique<Node>(hash, Ref{});

            parent->m_pChild[0] = left;
            parent->m_pChild[1] = right;

            left->m_parent = parent.get();
            right->m_parent = parent.get();

            next.push_back(parent.get());
            owned.push_back(std::move(parent));
        }

        level = std::move(next);
    }

    NodePtr newRoot = level.empty() ? nullptr : level.front();

    if (newRoot)
        newRoot->m_parent = nullptr;

    clear_unlocked();

    m_root = newRoot;

    for (auto& node : owned)
        node.release();
}

#endif // __MERKLE_TREE_H__