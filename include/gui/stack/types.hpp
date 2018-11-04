#ifndef OPENOBLIVION_GUI_STACK_TYPES_HPP
#define OPENOBLIVION_GUI_STACK_TYPES_HPP

#include "meta.hpp"
#include <string>
#include <variant>
#include <vector>

namespace gui::stack {

struct TraitName {
  std::string str{};
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

// Deduce the type of the value in `str` and return a ValueType with that value.
// The entities '&true;' and '&false;' are used for true and false. Floating
// point numbers are written in standard format without the trailing 'f'.
ValueType parseValueType(std::string_view str);

std::string appendSwitchCase(std::string name, const ValueType &val);

} // namespace gui::stack

#endif // OPENOBLIVION_GUI_STACK_TYPES_HPP
