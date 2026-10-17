#ifndef __CONTAINERBASE_H__
#define __CONTAINERBASE_H__
#include <mutex>
#include "GeneralNode.h"
#include "GeneralIterator.h"
#include "../foreach.h"

template<typename Traits, bool forward>
class ContainerBase
{
private:
    using value_type = typename Traits::value_type;
    using Node = typename Traits::Node;
    using ForwardIterator = typename Traits::ForwardIterator;
    using Compare = typename Traits::Compare;
    
    mutable std::mutex m_mutex; // mutex para sincronización
public:
    ContainerBase() {}
    virtual ~ContainerBase() {
        clear();
    }

    virtual void clear();
    virtual void push_back(const value_type& value, Ref ref);

    virtual ForwardIterator begin() const;
    virtual ForwardIterator end() const;

    friend ostream& operator <<(ostream& os, const ContainerBase<Traits>& container) {
        os << "[";
        for (auto it = container.begin(); it != container.end(); ++it) {
            os << *it;
            if (std::next(it) != container.end()) {
                os << ", ";
            }
        }
        return os << "]";
    }

    friend std::istream& operator >>(std::istream& is, ContainerBase<Traits>& container) {
        Delim d;
        container.clear();
        Node node;

        is >> d;
        if (is >> d && d != ']') {
            is.unget();
            while (is >> node >> d) {
                container.push_back(node.getValue(), node.getRef());
                if (d == ']') {
                    break;
                }
            }
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
}

#endif // __CONTAINERBASE_H__