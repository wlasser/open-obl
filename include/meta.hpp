#ifndef OPENOBLIVION_META_HPP
#define OPENOBLIVION_META_HPP

#include <optional>
#include <string>
#include <type_traits>
#include <variant>

// Miscellaneous metaprogramming utilities.

// Helper type for std::visit. Let's you write (for example)
// ```cpp
// std::visit(overloaded{
//     [](int i) { doInt(i); },
//     [](float f) { doFloat(f); },
//     [](auto x) { doFallback(x); }
// }, var);
// ```
// instead of messing around with `if constexpr`.
// C++20: If p0051 has been merged then replace this with std::overload
template<class ...Ts>
struct overloaded : Ts ... {
  using Ts::operator()...;
};
template<class ...Ts> overloaded(Ts...) -> overloaded<Ts...>;

// Helper function for `variant_with`.
template<class U, class ...Ts>
auto variant_with_helper(std::variant<Ts...> v) -> std::variant<Ts..., U>;

// Append a type to the end of a std::variant.
template<class V, class T>
using variant_with = decltype(variant_with_helper<T>(std::declval<V>()));

static_assert(std::is_same_v<std::variant<int, float>,
                             variant_with<std::variant<int>, float>>);

// See Arthur O'Dwyer https://quuxplusone.github.io/blog/2018/04/02/false-v/
// Used for a 'static_assert(false)' that isn't UB.
template<class ...>
inline constexpr bool false_v = false;

// If f(*lhs, *rhs) makes sense then call it and return the result wrapped in an
// optional. Return an empty optional otherwise. The pointers are a hack to get
// a catch-all overload without using a variadic function, which cannot have
// non-trivial types (like std::string) passed to it.
// Note that implicit conversions are considered for the purpose of determining
// f(*lhs, *rhs). Any undesired overloads that may occur through implicit
// conversions should be explicitly deleted.
template<class A, class F>
constexpr auto try_functor(F &&f, const A *lhs, const A *rhs) ->
decltype((void) (f(*lhs, *rhs)), std::optional<A>{}) {
  return f(*lhs, *rhs);
}

template<class A, class F>
constexpr auto try_functor(F &&f, const void *lhs, const void *rhs) ->
std::optional<A> {
  return std::nullopt;
}

// Like try_functor, but for boolean return types.
template<class A, class F>
constexpr auto try_predicate(F &&f, const A *lhs, const A *rhs) ->
decltype((void) (f(*lhs, *rhs)), std::optional<bool>{}) {
  return f(*lhs, *rhs);
}

template<class A, class F>
constexpr auto try_predicate(F &&f, const void *lhs, const void *rhs) ->
std::optional<bool> {
  return std::nullopt;
}

#endif // OPENOBLIVION_META_HPP
