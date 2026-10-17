#ifndef __CONTAINERBASE_H__
#define __CONTAINERBASE_H__
#include <initializer_list>
#include <mutex>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "../foreach.h"

template<typename Traits, bool AllowReverse>
class ContainerBase
{
private:
    using value_type = typename Traits::value_type;
    using Node = typename Traits::Node;
    using ForwardIterator = typename Traits::ForwardIterator;
    using Compare = typename Traits::Compare;
    using Delim = typename Traits::Delim;

    mutable std::mutex m_mutex; // mutex para sincronización
public:
    ContainerBase() {}
    ContainerBase(initializer_list<pair<value_type, Ref>> values) {
        for (const auto& v : values)
            push_back(v.first, v.second);
    }

    virtual ~ContainerBase() {
        clear();
    }

    virtual void clear();
    virtual void push_back(const value_type& value, Ref ref);

    virtual ForwardIterator begin() const;
    virtual ForwardIterator end() const;

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
        Delim d;
        value_type value;
        Ref ref;

        container.clear();

        if (!(is >> d) || d != '[')
            return is;

        while (is >> d && d != ']')
        {
            is.unget();

            if (!(is >> value >> d) || d != ',')
                return is;
            if (!(is >> ref >> d) || (d != ')' && d != ','))
                return is;

            container.push_back(value, ref);

            if (d == ']')
                break;
        }

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