#pragma once
#include <type_traits>

template<typename E>
struct EnableBitmaskOperators : std::false_type {};

#define ENABLE_BITMASK(E) \
template<> struct EnableBitmaskOperators<E> : std::true_type {}

template<class E> constexpr auto operator| (E a, E b) -> E requires EnableBitmaskOperators<E>::value { using U = std::underlying_type_t<E>; return static_cast<E>(static_cast<U>(a) | static_cast<U>(b)); }
template<class E> constexpr auto operator& (E a, E b) -> E requires EnableBitmaskOperators<E>::value { using U = std::underlying_type_t<E>; return static_cast<E>(static_cast<U>(a) & static_cast<U>(b)); }
template<class E> constexpr auto operator^ (E a, E b) -> E requires EnableBitmaskOperators<E>::value { using U = std::underlying_type_t<E>; return static_cast<E>(static_cast<U>(a) ^ static_cast<U>(b)); }
template<class E> constexpr auto operator~ (E a)      -> E requires EnableBitmaskOperators<E>::value { using U = std::underlying_type_t<E>; return static_cast<E>(~static_cast<U>(a)); }
template<class E> constexpr auto operator|=(E& a, E b) -> E& requires EnableBitmaskOperators<E>::value { return a = (a | b); }
template<class E> constexpr auto operator&=(E& a, E b) -> E& requires EnableBitmaskOperators<E>::value { return a = (a & b); }
template<class E> constexpr auto operator^=(E& a, E b) -> E& requires EnableBitmaskOperators<E>::value { return a = (a ^ b); }

template<class E> constexpr auto HasAny(E value, E mask) -> bool requires EnableBitmaskOperators<E>::value { using U = std::underlying_type_t<E>; return (static_cast<U>(value) & static_cast<U>(mask)) != 0; }
template<class E> constexpr auto HasAll(E value, E mask) -> bool requires EnableBitmaskOperators<E>::value { using U = std::underlying_type_t<E>; return (static_cast<U>(value) & static_cast<U>(mask)) == static_cast<U>(mask); }
