#include "meta.hpp"
#include "gui/stack/types.hpp"
#include <boost/convert.hpp>
#include <boost/convert/stream.hpp>

namespace gui::stack {

TraitName::TraitName(std::string name, const gui::Traits *traits)
    : str(name), traits(traits) {}

ValueType parseValueType(std::string_view str) {
  boost::cnv::cstream converter{};
  // Can use `converter(std::skipws)` to strip leading whitespace but not
  // trailing, and `boost::algorithm::trim` doesn't work with `string_view`.
  str.remove_prefix(std::min(str.find_first_not_of(" \t"), str.size()));
  if (const auto suffixPos{str.find_last_not_of(" \t")};
      suffixPos == std::string_view::npos) {
    str = "";
  } else {
    str.remove_suffix(str.size() - std::min(suffixPos + 1, str.size()));
  }

  if (str == "&true;") {
    return true;
  } else if (str == "&false;") {
    return false;
//  } else if (auto intOpt{boost::convert<int>(str, converter)};
//      intOpt.has_value()) {
//    return *intOpt;
  } else if (auto floatOpt{boost::convert<float>(str, converter)};
      floatOpt.has_value()) {
    return *floatOpt;
  } else return std::string{str};
}

std::string appendSwitchCase(std::string name, const ValueType &val) {
  return std::visit(overloaded{
      [&name](int i) {
        return name.append(std::to_string(i));
      },
      [&name](float f) {
        return name.append(std::to_string(f));
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
