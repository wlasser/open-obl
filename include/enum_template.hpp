#ifndef OPENOBLIVION_ENUM_TEMPLATE_HPP
#define OPENOBLIVION_ENUM_TEMPLATE_HPP

#include <cctype>
#include <functional>
#include <utility>
#include <variant>

namespace enumvar {

// Shorthand for a std::variant whose stored types are class templates depending
// on some non-type template parameter that takes only a finite number of
// values. For instance, one may have a collection
// Widget<0>, Widget<1>, Widget<2> of Widgets and know that they are
// storing one (but not which one) at runtime. Then one would use
// index_variant<int, Widget, 0, 1, 2>.
// The entries of the parameter pack I must be convertible to std::size_t, and
// when converted must be equivalent to the pack 0, 1, 2, ...
template<typename index_t, template<index_t> typename T, index_t ... I>
using index_variant = std::variant<T<I>...>;

// A sequential_variant is a shorthand for an index_variant where the indices do
// not need to be given explicitly; only the number of elements is required.
// Thus the widget example could be written
// sequential_variant<int, Widget, 3>.
// The casting is necessary because std::make_integer_sequence only works with
// integral types and we want to support enums.
template<typename index_t, template<index_t> typename T, std::size_t ...I>
auto make_sequential_variant_impl(std::index_sequence<I...>) {
  return index_variant<index_t, T, static_cast<index_t>(I)...>{};
}

template<typename index_t, template<index_t> typename T, index_t N>
auto make_sequential_variant() {
  return make_sequential_variant_impl<index_t, T>(
      std::make_index_sequence<static_cast<std::size_t>(N)>{});
}

template<typename index_t, template<index_t> typename T, index_t N>
using sequential_variant =
std::invoke_result_t<decltype(make_sequential_variant<index_t, T, N>)>;

// defaultConstruct is used to default construct the i-th value of an
// index_variant (or sequential_variant), where i is given at runtime. It is
// linear in sizeof(I).
template<typename index_t, typename T, index_t I1>
constexpr void defaultConstruct_impl(index_t i, T &x) {
  if (i == I1) {
    x.template emplace<static_cast<std::size_t>(I1)>();
  }
}

template<typename index_t, typename T, index_t I1, index_t I2, index_t ... I>
constexpr void defaultConstruct_impl(index_t i, T &x) {
  if (i == I1) {
    x.template emplace<static_cast<std::size_t>(I1)>();
  } else {
    defaultConstruct_impl<index_t, T, I2, I...>(i, x);
  }
}

template<typename index_t, template<index_t> typename U, index_t ... J>
constexpr auto
defaultConstruct(index_t i, index_variant<index_t, U, J ...> &x) {
  defaultConstruct_impl<index_t, index_variant<index_t, U, J...>, J...>(i, x);
}

// apply is like std::visit, and effectively calls f on std::get<i>(x).
// Note that the std::get will throw if x does not actually contain type i.
template<typename index_t, typename F, typename T, index_t I1>
constexpr void apply_impl(index_t i, F &&f, T x) {
  if (i == I1) {
    std::invoke(std::forward<F>(f), std::get<static_cast<std::size_t>(I1)>(x));
  }
}

template<typename index_t, typename F, typename T, index_t I1, index_t I2, index_t ... I>
constexpr void apply_impl(index_t i, F &&f, T x) {
  if (i == I1) {
    std::invoke(std::forward<F>(f), std::get<static_cast<std::size_t>(I1)>(x));
  } else {
    apply_impl<index_t, F, T, I2, I...>(i, std::forward<F>(f), x);
  }
}

template<typename index_t, typename F, template<index_t> typename U, index_t ... J>
constexpr void apply(index_t i, F &&f, index_variant<index_t, U, J ...> x) {
  apply_impl<index_t, F, index_variant<index_t, U, J...>, J...>(
      i, std::forward<F>(f), x);
}

} // namespace enumvar

#endif // OPENOBLIVION_ENUM_TEMPLATE_HPP
