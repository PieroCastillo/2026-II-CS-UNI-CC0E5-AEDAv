
#ifndef __HASH_TABLE_H__
#define __HASH_TABLE_H__

#include <vector>
#include <memory>
#include <format>
#include <functional>
#include <mutex>
#include <print>
#include <utility>
#include <stdexcept>
#include <string>
#include "GeneralContainer.h"
#include "AvlTree.h"

template <typename K, typename V>
struct HashEntry {
    using key_type = K;
    using mapped_type = V;

    K key{};
    V value{};

    HashEntry() = default;
    HashEntry(const K& k, const V& v = V{}) : key(k), value(v) {}
};

template<typename K, typename V>
struct std::formatter<HashEntry<K, V>> : std::formatter<std::string> {

    template<typename FormatContext>
    auto format(const HashEntry<K, V>& entry, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "[{} : {}]", entry.key, entry.value);
    }
};

template <typename K, typename V, typename Compare>
struct HashEntryCompare {
    bool operator()(const HashEntry<K, V>& a,
        const HashEntry<K, V>& b) const {
        return Compare{}(a.key, b.key);
    }
};

template <typename K, typename V, typename KeyCompare = std::less<K>>
struct HashAvlTraits :AvlTreeTraits<HashEntry<K, V>> {
    using value_type = HashEntry<K, V>;
    using CompareType = HashEntryCompare<K, V, KeyCompare>;
    using Compare = CompareType;
};

template <typename Traits>
class BaseHashTable;

template <typename K, typename V, typename HashFn, typename KeyCompare>
struct HashTableTraits {
    using key_type = K;
    using mapped_type = V;
    using value_type = HashEntry<K, V>;
    using Hash = HashFn;
    using Compare = KeyCompare;
};

template <typename K, typename V, typename HashFn = std::hash<K>, typename KeyCompare = std::less<K>>
using HashTable = BaseHashTable<HashTableTraits<K, V, HashFn, KeyCompare>>;

template <typename Traits>
class BaseHashTable {
public:
    using key_type = typename Traits::key_type;
    using mapped_type = typename Traits::mapped_type;
    using value_type = typename Traits::value_type;
    using Hash = typename Traits::Hash;
    using Compare = typename Traits::Compare;

    using BucketTraits = HashAvlTraits<key_type, mapped_type, Compare>;
    using Bucket = AvlTree<BucketTraits>;
    using Node = typename Bucket::Node;
    using NodePtr = typename Bucket::NodePtr;

private:
    mutable std::recursive_mutex m_mutex;
    std::vector<std::unique_ptr<Bucket>> m_buckets;
    size_t m_size = 0;
    float m_maxLoadFactor = 0.75f;

    size_t bucket_index(const key_type& key) const {
        return Hash{}(key) % m_buckets.size();
    }

    NodePtr find_node(const key_type& key) {
        value_type probe{ key };
        return (*m_buckets[bucket_index(key)])[probe];
    }

    const Node* find_node(const key_type& key) const {
        value_type probe{ key };
        return (*m_buckets[bucket_index(key)])[probe];
    }

    void initialize(size_t count);
    void rehash(size_t count);
    void insert_unlocked(const key_type& key, const mapped_type& value, Ref ref);

    virtual void printNode(const Node* node) const {
        const auto& entry = node->getValue();
        std::print("{}: {} [Ref: {}]", entry.key, entry.value, node->getRef());
    }

    void visualize_recursive(const Node* node, const std::string& prefix, bool isLast, bool isRoot) const;

public:
    BaseHashTable(size_t capacity = 16) {
        initialize(std::max<size_t>(capacity, 1));
    }

    BaseHashTable(const BaseHashTable& other);
    BaseHashTable& operator=(const BaseHashTable& other);

    size_t size() const;
    size_t bucket_count() const;
    bool empty() const;

    float load_factor() const;
    void max_load_factor(float factor);

    mapped_type& operator[](const key_type& key);
    mapped_type& at(const key_type& key);
    const mapped_type& at(const key_type& key) const;

    bool contains(const key_type& key) const;
    bool insert(const key_type& key, const mapped_type& value, Ref ref = {});
    bool erase(const key_type& key);
    void clear();
    void reserve(size_t count);

    void visualize() const;

    static std::ostream& write(std::ostream& os, const BaseHashTable& table);
    static std::istream& read(std::istream& is, BaseHashTable& table);

    friend std::ostream& operator<<(std::ostream& os, const BaseHashTable& table) {
        return write(os, table);
    }

    friend std::istream& operator>>(std::istream& is, BaseHashTable& table) {
        return read(is, table);
    }
};

template <typename Traits>
void BaseHashTable<Traits>::visualize() const {
    std::lock_guard lock(m_mutex);

    std::println("HashTable");

    for (size_t i = 0; i < m_buckets.size(); ++i) {
        const auto root = m_buckets[i]->root();

        std::print("Bucket [{}]", i);

        if (!root) {
            std::println(" (empty)");
            continue;
        }

        std::println();
        visualize_recursive(root, "", true, true);
    }
}

template <typename Traits>
void BaseHashTable<Traits>::visualize_recursive(const Node* node, const std::string& prefix, bool isLast, bool isRoot) const {
    if (!node)
        return;

    if (isRoot) {
        std::print("  ");
        printNode(node);
        std::println();
    }
    else {
        std::print("  {}{}", prefix, isLast ? "└── " : "├── ");

        printNode(node);
        std::println();
    }

    const auto nextPrefix =
        prefix + (isRoot ? "" : (isLast ? "    " : "│   "));

    if (node->m_pChild[1])
        visualize_recursive(node->m_pChild[1], nextPrefix, !node->m_pChild[0], false);

    if (node->m_pChild[0])
        visualize_recursive(node->m_pChild[0], nextPrefix, true, false);
}

template <typename Traits>
void BaseHashTable<Traits>::initialize(size_t count) {
    m_buckets.clear();
    m_buckets.reserve(count);
    for (size_t i = 0; i < count; ++i)
        m_buckets.emplace_back(std::make_unique<Bucket>());
}

template <typename Traits>
void BaseHashTable<Traits>::rehash(size_t count) {
    std::vector<std::unique_ptr<Bucket>> old;
    old.swap(m_buckets);
    initialize(count);
    m_size = 0;

    for (auto& bucket : old) {
        for (const auto& node : *bucket) {
            const auto& entry = node.getValue();
            insert_unlocked(entry.key, entry.value,
                node.getRef());
        }
    }
}

template <typename Traits>
void BaseHashTable<Traits>::insert_unlocked(const key_type& key,
    const mapped_type& value,
    Ref ref) {
    value_type probe{ key };
    auto& bucket = *m_buckets[bucket_index(key)];

    if (auto node = bucket[probe]) {
        node->value().value = value;
        return;
    }

    bucket.insert(value_type{ key,value }, ref);
    ++m_size;
}

template <typename Traits>
BaseHashTable<Traits>::BaseHashTable(const BaseHashTable& other) {
    std::scoped_lock lock(other.m_mutex);
    initialize(other.m_buckets.size());

    for (const auto& bucket : other.m_buckets)
        for (const auto& node : *bucket) {
            const auto& entry = node.getValue();
            insert_unlocked(entry.key, entry.value,
                node.getRef());
        }
}

template <typename Traits>
auto BaseHashTable<Traits>::operator=(const BaseHashTable& other) -> BaseHashTable&
{
    if (this == &other) return *this;

    std::scoped_lock lock(m_mutex, other.m_mutex);
    initialize(other.m_buckets.size());
    m_size = 0;

    for (const auto& bucket : other.m_buckets)
        for (const auto& node : *bucket) {
            const auto& entry = node.getValue();
            insert_unlocked(entry.key, entry.value,
                node.getRef());
        }

    return *this;
}

template <typename Traits>
size_t BaseHashTable<Traits>::size() const {
    std::lock_guard lock(m_mutex);
    return m_size;
}

template <typename Traits>
size_t BaseHashTable<Traits>::bucket_count() const {
    std::lock_guard lock(m_mutex);
    return m_buckets.size();
}

template <typename Traits>
bool BaseHashTable<Traits>::empty() const {
    std::lock_guard lock(m_mutex);
    return m_size == 0;
}

template <typename Traits>
float BaseHashTable<Traits>::load_factor() const {
    std::lock_guard lock(m_mutex);
    return static_cast<float>(m_size) / static_cast<float>(m_buckets.size());
}

template <typename Traits>
void BaseHashTable<Traits>::max_load_factor(float factor) {
    if (factor <= 0.0f)
        throw std::invalid_argument("Invalid load factor");

    std::lock_guard lock(m_mutex);
    m_maxLoadFactor = factor;
}

template <typename Traits>
auto BaseHashTable<Traits>::operator[](const key_type& key) -> mapped_type& {
    std::lock_guard lock(m_mutex);

    if (auto node = find_node(key))
        return node->value().value;

    if (load_factor() >= m_maxLoadFactor)
        rehash(m_buckets.size() * 2);

    insert_unlocked(key, mapped_type{}, Ref{});
    return find_node(key)->value().value;
}

template <typename Traits>
auto BaseHashTable<Traits>::at(const key_type& key) -> mapped_type& {
    std::lock_guard lock(m_mutex);

    if (auto node = find_node(key))
        return node->value().value;

    throw std::out_of_range("HashTable::at");
}

template <typename Traits>
auto BaseHashTable<Traits>::at(const key_type& key) const -> const mapped_type& {
    std::lock_guard lock(m_mutex);

    if (auto node = find_node(key))
        return node->getValue().value;

    throw std::out_of_range("HashTable::at");
}

template <typename Traits>
bool BaseHashTable<Traits>::contains(const key_type& key) const {
    std::lock_guard lock(m_mutex);
    return find_node(key) != nullptr;
}

template <typename Traits>
bool BaseHashTable<Traits>::insert(const key_type& key, const mapped_type& value, Ref ref)
{
    std::lock_guard lock(m_mutex);

    if (find_node(key))
        return false;

    if (load_factor() >= m_maxLoadFactor)
        rehash(m_buckets.size() * 2);

    insert_unlocked(key, value, ref);
    return true;
}

template <typename Traits>
bool BaseHashTable<Traits>::erase(const key_type& key) {
    std::lock_guard lock(m_mutex);

    auto& bucket = *m_buckets[bucket_index(key)];
    value_type probe{ key };

    auto node = bucket[probe];
    if (!node) return false;

    bucket.remove(node);
    --m_size;
    return true;
}

template <typename Traits>
void BaseHashTable<Traits>::clear() {
    std::lock_guard lock(m_mutex);
    for (auto& bucket : m_buckets)
        bucket->clear();
    m_size = 0;
}

template <typename Traits>
void BaseHashTable<Traits>::reserve(size_t count) {
    std::lock_guard lock(m_mutex);
    if (count > m_buckets.size())
        rehash(count);
}

template <typename Traits>
std::ostream& BaseHashTable<Traits>::write(std::ostream& os, const BaseHashTable& table) {
    std::lock_guard lock(table.m_mutex);

    os << table.m_size << '\n';

    for (const auto& bucket : table.m_buckets)
        for (const auto& node : *bucket)
            os << node.getValue().key << ' '
            << node.getValue().value << ' '
            << node.getRef() << '\n';

    return os;
}

template <typename Traits>
std::istream& BaseHashTable<Traits>::read(std::istream& is, BaseHashTable& table) {
    size_t count;
    if (!(is >> count)) return is;

    BaseHashTable temp(table.m_buckets.size());

    for (size_t i = 0; i < count; ++i) {
        key_type key{};
        mapped_type value{};
        Ref ref{};

        if (!(is >> key >> value >> ref))
            return is;

        temp.insert(key, value, ref);
    }

    std::scoped_lock lock(table.m_mutex, temp.m_mutex);
    table.m_buckets.swap(temp.m_buckets);
    table.m_size = temp.m_size;
    return is;
}

#endif // __HASH_TABLE_H__