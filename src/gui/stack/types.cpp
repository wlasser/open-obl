#include "gui/stack/types.hpp"
#include "gui/xml.hpp"
#include <nostdx/functional.hpp>
#include <boost/convert.hpp>
#include <boost/convert/stream.hpp>

namespace gui::stack {

TraitName::TraitName(std::string name, const gui::Traits *traits)
    : str(std::move(name)), traits(traits) {}

ValueType parseValueType(std::string_view str) {
  gui::XmlEntityConverter conv{};
  boost::cnv::cstream fConv{};
  // Manually strip whitespace as `boost::algorithm::trim` doesn't work with
  // `std::string_view` and `boost::convert` can only skip leading whitespace
  // even if we did use the default converter.
  str.remove_prefix(std::min(str.find_first_not_of(" \t"), str.size()));
  if (const auto suffixPos{str.find_last_not_of(" \t")};
      suffixPos == std::string_view::npos) {
    str = "";
  } else {
    str.remove_suffix(str.size() - std::min(suffixPos + 1, str.size()));
  }

  if (auto bOpt{boost::convert<bool>(str, conv)}; bOpt.has_value()) {
    return *bOpt;
  } else if (auto fOpt{boost::convert<float>(str, conv)}; fOpt.has_value()) {
    return *fOpt;
  } else if (fOpt = boost::convert<float>(str, fConv); fOpt.has_value()) {
    return *fOpt;
  } else return std::string{str};
}

std::string appendSwitchCase(std::string name, const ValueType &val) {
  return std::visit(nostdx::overloaded{
      [&name](int i) {
        return name.append(std::to_string(i));
      },
      [&name](float f) {
        return name.append(std::to_string(static_cast<int>(f)));
      },
      [&name](bool b) {
        return name.append(b ? "true" : "false");
      },
      [&name](const std::string &s) {
        return name.append(s);
      }
  }, val);
}

} // namespace gui::stack
