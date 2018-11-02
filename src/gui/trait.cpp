#include "gui/trait.hpp"
#include <boost/algorithm/string/predicate.hpp>

namespace gui {

std::optional<int> getUserTraitIndex(std::string_view name) {
  // `name` might be fully-qualified so we remove the parent names, if any.
  const auto lastDot{name.rfind('.')};
  if (lastDot != std::string_view::npos) {
    name.remove_prefix(lastDot + 1);
  }
  // TODO: In C++20, use name.starts_with("user")
  if (!boost::algorithm::starts_with(name, "user")) return std::nullopt;
  try {
    const int index{std::stoi(std::string{name.substr(4)})};
    return index >= 0 ? std::optional{index} : std::nullopt;
  } catch (const std::exception &e) {
    return std::nullopt;
  }
}

} // namespace gui