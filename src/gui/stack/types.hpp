#ifndef OPENOBL_GUI_STACK_TYPES_HPP
#define OPENOBL_GUI_STACK_TYPES_HPP

#include "util/meta.hpp"
#include <string>
#include <variant>
#include <vector>

namespace gui {

class Traits;

namespace stack {

struct TraitName {
  /// Fully-qualified name of the trait
  std::string str{};
  /// gui::Traits context the trait is expected to be in upon execution.
  const gui::Traits *traits;

  TraitName(std::string name, const gui::Traits *traits);
};

inline bool operator==(const TraitName &a, const TraitName &b) noexcept {
  return a.str == b.str;
}

inline bool operator!=(const TraitName &a, const TraitName &b) noexcept {
  return !(a == b);
}

using ValueType = std::variant<int, float, bool, std::string>;
using ArgumentType = variant_with<ValueType, TraitName>;
using Stack = std::vector<ValueType>;

/// Deduce the type of the value in `str` and return a ValueType with that value.
/// The entities `&true``;` and `&false``;` are used for true and false.
/// Floating point numbers are written in standard format without the trailing
/// `f`.
ValueType parseValueType(std::string_view str);

/// Stringify `val` and append it to `name`. This is used to implement a switch
/// statement, hence the name, and is triggered when a copy operator has a
/// selector that selects a trait whose name ends in a trailing underscore `_`.
std::string appendSwitchCase(std::string name, const ValueType &val);

/// Pop the next element off the stack, returning a default value if the stack
/// is empty. Specifically, if the stack is empty and a `Hint` type equal to one
/// of the variant types of `ValueType` is provided, then return a default
/// constructed value of that type.
/// \throws std::runtime_error if the stack is empty and no valid `Hint` was
///                            provided.
template<class Hint = void>
[[nodiscard]] ValueType popFromStack(Stack &stack) {
  if constexpr (std::is_void_v<Hint>) {
    if (stack.empty()) throw std::runtime_error("Stack is empty");
  } else {
    if (stack.empty()) return Hint{};
  }
  auto v{stack.back()};
  stack.pop_back();
  return v;
}

} // namespace stack

} // namespace gui

#endif // OPENOBL_GUI_STACK_TYPES_HPP
