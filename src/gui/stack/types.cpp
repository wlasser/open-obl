#include "meta.hpp"
#include "gui/stack/types.hpp"
#include <boost/convert.hpp>
#include <boost/convert/stream.hpp>

namespace gui::stack {

ValueType parseValueType(std::string_view str) {
  boost::cnv::cstream converter{};
  if (str == "&true;") {
    return true;
  } else if (str == "&false;") {
    return false;
  } else if (auto intOpt{boost::convert<int>(str, converter)}) {
    return *intOpt;
  } else if (auto floatOpt{boost::convert<float>(str, converter)}) {
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
