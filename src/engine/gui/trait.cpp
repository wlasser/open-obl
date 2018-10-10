#include "engine/gui/trait.hpp"

namespace engine::gui {

std::optional<int> getUserTraitIndex(const std::string &name) {
  if (!boost::algorithm::starts_with(name, "user")) return std::nullopt;
  int index{0};
  try {
    index = std::stoi(name.substr(4));
  } catch (const std::exception &e) {
    return std::nullopt;
  }
  if (index < 0) return std::nullopt;
  return index;
}

} // namespace engine::gui