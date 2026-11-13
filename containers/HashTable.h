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
#include <algorithm>
#include <cmath>
#include <limits>
#include <istream>
#include <ostream>

#include "GeneralContainer.h"
#include "avltree.h"
#include "treeUtils.h"

template <typename K, typename V>
struct HashEntry {
    using key_type = K;
    using mapped_type = V;

    K key{};
    V value{};

    HashEntry() = default;
    HashEntry(const K& k, const V& v = V{}) : key(k), value(v) {}
};

template <typename K, typename V>
struct std::formatter<HashEntry<K, V>> : std::formatter<std::string> {
    template <typename FormatContext>
    auto format(const HashEntry<K, V>& entry, FormatContext& ctx) const {
        return std::format_to(ctx.out(), "[{} : {}]", entry.key, entry.value);
    }
};

template <typename K, typename V, typename Compare>
struct HashEntryCompare {
    bool operator()(const HashEntry<K, V>& a, const HashEntry<K, V>& b) const
    {
        return Compare{}(a.key, b.key);
    }
};

template <typename K, typename V, typename KeyCompare = std::less<K>>
struct HashAvlTraits : AvlTreeTraits<HashEntry<K, V>> {
    using value_type = HashEntry<K, V>;
    using Compare = HashEntryCompare<K, V, KeyCompare>;
};

template <typename K, typename V, typename HashFn, typename KeyCompare>
struct HashTableTraits {
    using key_type = K;
    using mapped_type = V;
    using value_type = HashEntry<K, V>;
    using Hash = HashFn;
    using KeyCompareType = KeyCompare;
    using Compare = HashEntryCompare<K, V, KeyCompare>;
};

template <typename Traits>
class BaseHashTable;

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
    using ConstNodePtr = const Node*;

private:
    mutable std::recursive_mutex m_mutex;

    std::vector<Bucket> m_buckets;
    size_t m_size = 0;
    float m_maxLoadFactor = 0.75f;

private:
    size_t bucket_index(const key_type& key) const;

    NodePtr find_node(const key_type& key);
    ConstNodePtr find_node(const key_type& key) const;

    void initialize(size_t count);
    void insert_unlocked(const key_type& key, const mapped_type& value, Ref ref);
    void rehash(size_t count);
    std::string printNode(const Node* node) const;

public:
    explicit BaseHashTable(size_t capacity = 16);

    BaseHashTable(const BaseHashTable& other);
    BaseHashTable(BaseHashTable&& other);

    BaseHashTable& operator=(const BaseHashTable& other);
    BaseHashTable& operator=(BaseHashTable&& other);

    void swap(BaseHashTable& other);

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

    friend std::ostream& operator<<(std::ostream& os, const BaseHashTable& table) { return write(os, table); }
    friend std::istream& operator>>(std::istream& is, BaseHashTable& table) { return read(is, table); }
};

template <typename Traits>
size_t BaseHashTable<Traits>::bucket_index(const key_type& key) const {
    return Hash{}(key) % m_buckets.size();
}

template <typename Traits>
auto BaseHashTable<Traits>::find_node(const key_type& key) -> NodePtr {
    value_type probe{ key };

    return m_buckets[bucket_index(key)][probe];
}

template <typename Traits>
auto BaseHashTable<Traits>::find_node(const key_type& key) const -> ConstNodePtr {
    value_type probe{ key };

    return m_buckets[bucket_index(key)][probe];
}

template <typename Traits>
void BaseHashTable<Traits>::initialize(size_t count) {
    count = std::max<size_t>(count, 1);

    std::vector<Bucket> buckets;
    buckets.reserve(count);

    for (size_t i = 0; i < count; ++i)
        buckets.emplace_back();

    m_buckets.swap(buckets);
}

template <typename Traits>
void BaseHashTable<Traits>::insert_unlocked(const key_type& key,const mapped_type& value,Ref ref) {
    value_type probe{ key };

    auto& bucket = m_buckets[bucket_index(key)];

    if (auto node = bucket[probe]) {
        node->value().value = value;
        return;
    }

    bucket.insert(value_type{ key, value }, ref);
    ++m_size;
}


template <typename Traits>
void BaseHashTable<Traits>::rehash(size_t count) {
    count = std::max<size_t>(count, 1);

    std::vector<Bucket> newBuckets;
    newBuckets.reserve(count);

    for (size_t i = 0; i < count; ++i)
        newBuckets.emplace_back();

    for (const auto& bucket : m_buckets) {
        for (const auto& node : bucket) {
            const auto& entry = node.getValue();

            const auto index = Hash{}(entry.key) % count;

            newBuckets[index].insert(
                entry,
                node.getRef()
            );
        }
    }
    m_buckets.swap(newBuckets);
}


template <typename Traits>
std::string BaseHashTable<Traits>::printNode(const Node* node) const {
    const auto& entry = node->getValue();
    std::ostringstream os;
    os << std::format("{}: {} [Ref: {}]", entry.key, entry.value, node->getRef());

    return os.str();
}

template <typename Traits>
BaseHashTable<Traits>::BaseHashTable(size_t capacity) {
    initialize(capacity);
}

template <typename Traits>
BaseHashTable<Traits>::BaseHashTable(const BaseHashTable& other) {
    std::lock_guard lock(other.m_mutex);

    initialize(other.m_buckets.size());
    m_maxLoadFactor = other.m_maxLoadFactor;

    for (const auto& bucket : other.m_buckets) {
        for (const auto& node : bucket) {
            const auto& entry = node.getValue();

            insert_unlocked(entry.key, entry.value, node.getRef());
        }
    }
}

template <typename Traits>
BaseHashTable<Traits>::BaseHashTable(BaseHashTable&& other) {
    std::lock_guard lock(other.m_mutex);

    m_buckets = std::move(other.m_buckets);
    m_size = std::exchange(other.m_size, 0);
    m_maxLoadFactor = other.m_maxLoadFactor;

    other.initialize(1);
}

template <typename Traits>
BaseHashTable<Traits>& BaseHashTable<Traits>::operator=(const BaseHashTable& other) {
    if (this == &other)
        return *this;

    BaseHashTable temp(other);

    swap(temp);

    return *this;
}

template <typename Traits>
BaseHashTable<Traits>& BaseHashTable<Traits>::operator=(BaseHashTable&& other) {
    if (this == &other)
        return *this;

    std::scoped_lock lock(m_mutex, other.m_mutex);

    m_buckets = std::move(other.m_buckets);
    m_size = std::exchange(other.m_size, 0);
    m_maxLoadFactor = other.m_maxLoadFactor;

    other.initialize(1);
    return *this;
}

template <typename Traits>
void BaseHashTable<Traits>::swap(BaseHashTable& other) {
    if (this == &other)
        return;

    std::scoped_lock lock(m_mutex, other.m_mutex);

    m_buckets.swap(other.m_buckets);
    std::swap(m_size, other.m_size);
    std::swap(m_maxLoadFactor, other.m_maxLoadFactor);
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
    if (!std::isfinite(factor) || factor <= 0.0f)
        throw std::invalid_argument("Invalid load factor");

    std::lock_guard lock(m_mutex);

    const auto required = static_cast<size_t>(std::ceil(static_cast<double>(m_size) / factor));

    if (required > m_buckets.size())
        rehash(required);

    m_maxLoadFactor = factor;
}

template <typename Traits>
auto BaseHashTable<Traits>::operator[](const key_type& key) -> mapped_type& {
    std::lock_guard lock(m_mutex);

    if (auto node = find_node(key))
        return node->value().value;

    if (static_cast<float>(m_size + 1) /
        static_cast<float>(m_buckets.size()) > m_maxLoadFactor) {
        rehash(m_buckets.size() * 2);
    }

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
auto BaseHashTable<Traits>::at(const key_type& key) const -> const BaseHashTable<Traits>::mapped_type& {
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
bool BaseHashTable<Traits>::insert(const key_type& key, const mapped_type& value, Ref ref) {
    std::lock_guard lock(m_mutex);

    if (find_node(key))
        return false;

    if (static_cast<float>(m_size + 1) / static_cast<float>(m_buckets.size()) > m_maxLoadFactor)
        rehash(m_buckets.size() * 2);

    insert_unlocked(key, value, ref);
    return true;
}


template <typename Traits>
bool BaseHashTable<Traits>::erase(const key_type& key) {
    std::lock_guard lock(m_mutex);

    auto& bucket = m_buckets[bucket_index(key)];
    value_type probe{ key };
    auto node = bucket[probe];

    if (!node)
        return false;

    bucket.remove(node);
    --m_size;

    return true;
}


template <typename Traits>
void BaseHashTable<Traits>::clear() {
    std::lock_guard lock(m_mutex);

    for (auto& bucket : m_buckets)
        bucket.clear();

    m_size = 0;
}


template <typename Traits>
void BaseHashTable<Traits>::reserve(size_t count) {
    std::lock_guard lock(m_mutex);

    if (count > m_buckets.size())
        rehash(count);
}

template <typename Traits>
void BaseHashTable<Traits>::visualize() const {
    std::lock_guard lock(m_mutex);

    std::println("HashTable");

    for (size_t i = 0; i < m_buckets.size(); ++i) {
        const auto root = m_buckets[i].root();

        std::print("Bucket [{}]", i);

        if (!root) {
            std::println(" (empty)");
            continue;
        }

        std::println();

        treeUtils::visualize(root,
            [this](const Node* node) -> std::string {
                return printNode(node);
            });
    }
}

template <typename Traits>
std::ostream& BaseHashTable<Traits>::write(std::ostream& os, const BaseHashTable& table) {
    std::lock_guard lock(table.m_mutex);

    os << table.m_size << '\n';

    for (const auto& bucket : table.m_buckets) {
        for (const auto& node : bucket) {
            const auto& entry = node.getValue();
            os << entry.key << ' ' << entry.value << ' ' << node.getRef() << '\n';

            if (!os)
                return os;
        }
    }

    return os;
}

template <typename Traits>
std::istream& BaseHashTable<Traits>::read(std::istream& is, BaseHashTable& table) {
    size_t count{};

    if (!(is >> count))
        return is;

    BaseHashTable temp;
    {
        std::lock_guard lock(table.m_mutex);
        temp.m_maxLoadFactor = table.m_maxLoadFactor;
        temp.initialize(table.m_buckets.size());
    }

    for (size_t i = 0; i < count; ++i) {
        key_type key{};
        mapped_type value{};
        Ref ref{};

        if (!(is >> key >> value >> ref))
            return is;

        if (!temp.insert(key, value, ref)) {
            is.setstate(std::ios::failbit);
            return is;
        }
    }

    table.swap(temp);

    return is;
}
#endif // __HASH_TABLE_H__