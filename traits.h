#ifndef __TRAITS_H__
#define __TRAITS_H__

#include <functional>

template <typename T, typename _Compare>
struct DefaultTraits {
    using value_type = T;
    using Compare = _Compare;
};
template <typename T, typename _Compare = std::less<T>>
struct AscendingTraits : public DefaultTraits<T, _Compare> {};

template <typename T, typename _Compare = std::greater<T>>
struct DescendingTraits : public DefaultTraits<T, _Compare> {};

#endif // __TRAITS_H__