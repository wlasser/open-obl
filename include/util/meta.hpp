#ifndef OPENOBL_META_HPP
#define OPENOBL_META_HPP

#include <functional>
#include <optional>
#include <string>
#include <type_traits>
#include <variant>

/// \file
/// Miscellaneous metaprogramming utilities.

/// Helper type for std::visit. Let's you write (for example)
/// ```cpp
/// std::visit(overloaded{
///     [](int i) { doInt(i); },
///     [](float f) { doFloat(f); },
///     [](auto x) { doFallback(x); }
/// }, var);
/// ```
/// instead of messing around with `if constexpr`.
//C++20: If p0051 has been merged then replace this with std::overload
template<class ...Ts>
struct overloaded : Ts ... {
  using Ts::operator()...;
};
template<class ...Ts> overloaded(Ts...) -> overloaded<Ts...>;

/// Helper function for `variant_with`.
template<class U, class ...Ts>
auto variant_with_helper(std::variant<Ts...> v) -> std::variant<Ts..., U>;

/// Append a type to the end of a std::variant.
template<class V, class T>
using variant_with = decltype(variant_with_helper<T>(std::declval<V>()));

static_assert(std::is_same_v<std::variant<int, float>,
                             variant_with<std::variant<int>, float>>);

/// Simple dependent false.
/// See Arthur O'Dwyer https://quuxplusone.github.io/blog/2018/04/02/false-v/
/// Useful for a `static_assert(false)` that isn't UB.
template<class ...>
inline constexpr bool false_v = false;

/// Provides a member type `type` equal to `T`.
/// Used to store a type instead of an instance of that type.
/// \remark Equivalent to std::type_identity proposed in
///         [P0887](https://wg21.link/p0887), which has been merged into C++20.
///         A major motivation there was to inhibit template argument deduction.
//C++20: template<class T> using type_identity = std::type_identity<T>;
template<class T> struct type_identity {
  using type = T;
};

/// Helper variable template for type_identity.
template<class T> using type_identity_t = typename type_identity<T>::type;

/// Provide the member constant `value` equal to `true` if `T` has the member
/// type `type`, and `false` otherwise.
/// \tparam T The type to check
template<class T, class = void>
struct has_type : std::false_type {};

template<class T>
struct has_type<T, std::conditional_t<false, typename T::type, void>>
    : std::true_type {
};

template<class T>
static inline constexpr bool has_type_v = has_type<T>::value;

template<class T> struct function_traits {};

/// Trait for getting the return type and argument types of a function.
/// Provides the member types
/// - `result_t` equal to the return type of the function
/// - `args_t` equal to a std::tuple of the argument types of the function
///
/// Also provides the member type template `arg_t<I>` equal to the type of the
/// `I`-th argument, and the member constant `size` equal to the number of
/// arguments.
///
/// Lambdas are not supported directly, wrap them in a std::function.
template<class Ret, class ... Args>
struct function_traits<Ret(Args...)> {
  using result_t = Ret;
  using args_t = std::tuple<Args...>;

  static constexpr inline std::size_t size = sizeof...(Args);

  template<std::size_t I>
  using arg_t = std::tuple_element_t<I, args_t>;
};

/// \overload function_traits<Ret(Args...)>
template<class Ret, class ... Args>
struct function_traits<std::function<Ret(Args...)>> {
  using result_t = Ret;
  using args_t = std::tuple<Args...>;

  static constexpr inline std::size_t size = sizeof...(Args);

  template<std::size_t I>
  using arg_t = std::tuple_element_t<I, args_t>;
};

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

template<class A, class F> constexpr auto try_functor(F &&/*f*/,
                                                      const void */*lhs*/,
                                                      const void */*rhs*/) ->
std::optional<A> {
  return std::nullopt;
}

// Like try_functor, but for boolean return types.
template<class A, class F>
constexpr auto try_predicate(F &&f, const A *lhs, const A *rhs) ->
decltype((void) (f(*lhs, *rhs)), std::optional<bool>{}) {
  return f(*lhs, *rhs);
}

template<class A, class F> constexpr auto try_predicate(F &&/*f*/,
                                                        const void */*lhs*/,
                                                        const void */*rhs*/) ->
std::optional<bool> {
  return std::nullopt;
}

#endif // OPENOBL_META_HPP
