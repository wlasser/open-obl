#include "gui/stack/instructions.hpp"
#include "gui/stack/types.hpp"
#include "gui/traits.hpp"
#include <string>
#include <variant>

namespace gui::stack {

void push_t::operator()(Stack &stack) const {
  std::visit(overloaded{
      [&stack](const TraitName &trait) {
        // Trailing underscore implies a switch statement using the working
        // value, otherwise replace the working value.
        std::string fullName = [&]() {
          if (trait.str.back() == '_') {
            const auto val{stack.back()};
            return stack::appendSwitchCase(trait.str, val);
          }
          return trait.str;
        }();
        const auto &traitVar{trait.traits->getTraitVariant(fullName)};
        stack.emplace_back(std::visit([](const auto &t) -> ValueType {
          return t.invoke();
        }, traitVar));
      },
      [&stack](int i) { stack.emplace_back(i); },
      [&stack](float f) { stack.emplace_back(f); },
      [&stack](bool b) { stack.emplace_back(b); },
      [&stack](const std::string s) { stack.emplace_back(s); }
  }, arg_t);
}

} // namespace gui::stack