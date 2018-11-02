#ifndef OPENOBLIVION_GUI_STACK_META_HPP
#define OPENOBLIVION_GUI_STACK_META_HPP

#include <optional>
#include <string>
#include <type_traits>
#include <variant>

namespace gui::stack::meta {

// Pointless metaprogramming. Appends a type to the end of a variant.
template<class U, class ...Ts>
auto variant_with_helper(std::variant<Ts...> v) -> std::variant<Ts..., U>;

template<class V, class T>
using variant_with = decltype(variant_with_helper<T>(std::declval<V>()));

// Not pointless metaprogramming. Used for std::visit.
template<class ...Ts>
struct overloaded : Ts ... {
  using Ts::operator()...;
};
template<class ...Ts> overloaded(Ts...) -> overloaded<Ts...>;

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

} // namespace gui::stack::meta

#endif // OPENOBLIVION_ENGINE_GUI_STACK_META_HPP
