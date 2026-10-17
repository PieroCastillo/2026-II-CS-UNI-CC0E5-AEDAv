#ifndef __CONTAINERBASE_H__
#define __CONTAINERBASE_H__
#include <initializer_list>
#include <mutex>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "traits.h"
#include "../foreach.h"

template<typename Traits, bool AllowReverse>
class ContainerBase
{
public:
    using value_type = typename Traits::value_type;
    using Node = typename Traits::Node;
    using ForwardIterator = typename Traits::ForwardIterator;
    using Compare = typename Traits::Compare;
    using Delim = typename Traits::Delim;
protected:
    mutable std::mutex m_mutex; // mutex para sincronización

    template<typename Self, typename Range>
    void initialize(this Self& self, const Range& values) {
        for (const auto& v : values)
            self.push_back(v.first, v.second);
    }
public:
    ContainerBase() = default;
    virtual ~ContainerBase() = default;

    virtual void clear() = 0;
    virtual void push_back(const value_type& value, Ref ref) = 0;
    virtual ForwardIterator begin() const = 0;
    virtual ForwardIterator end() const = 0;

    std::ostream& write(std::ostream& os) { return os << *this; }
    std::istream& read(std::istream& is) { return is >> *this; }

    friend ostream& operator <<(ostream& os, const ContainerBase<Traits, AllowReverse>& container) {
        os << "[";
        for (auto it = container.begin(); it != container.end(); ++it) {
            os << *it;
            if (std::next(it) != container.end()) {
                os << ", ";
            }
        }
        return os << "]";
    }

    friend std::istream& operator>>(std::istream& is, ContainerBase<Traits, AllowReverse>& container)
    {
        Delim d{};

        if (!(is >> d) || d != '[') {
            is.setstate(std::ios::failbit);
            return is;
        }

        container.clear();
        Node node;

        while (is >> std::ws && is.peek() != ']') {
            if (!(is >> node))
                return is;

            container.push_back(node.getValue(), node.getRef());

            if (!(is >> d) || d == ']')
                return is;

            if (d != ',') {
                is.setstate(std::ios::failbit);
                return is;
            }
        }

        if (is.peek() == ']')
            is.get();
        else
            is.setstate(std::ios::failbit);

        return is;
    }

    template <typename Func, typename... Args>
    void ApplyFunction(Func func, Args... args) {
        call(func, std::forward<Args>(args)...);
    }
    template <typename Func, typename... Args>
    Node& FirstThat(Func func, Args... args) {
        return call(func, std::forward<Args>(args)...);
    }
    template<typename Func, typename... Args>
    decltype(auto) call(Func func, Args&&... args)
    {
        lock_guard<mutex> lock(m_mutex);
        if constexpr (is_void_v<invoke_result_t<Func, Node&, Args...>>)
            ::call(begin(), end(), std::forward<Func>(func), std::forward<Args>(args)...);
        else // return type is not void:
            return ::call(begin(), end(), std::forward<Func>(func), std::forward<Args>(args)...);
    }
};
#endif // __CONTAINERBASE_H__