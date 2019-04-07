#include "gui/logging.hpp"
#include "gui/strings.hpp"
#include "gui/trait.hpp"
#include "gui/xml.hpp"
#include "settings.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <pugixml.hpp>
#include <string>
#include <unordered_map>

namespace gui {

void StringsElement::parseXmlDocument(pugi::xml_node doc) {
  const auto stringsNode{doc.find_child([](pugi::xml_node node) {
    using namespace std::literals;
    const auto attr{node.attribute("name")};
    return node.name() == "rect"s && attr && attr.value() == "Strings"s;
  })};

  if (stringsNode) {
    for (auto node : stringsNode.children()) {
      const auto name{std::string{getPrefix()} + node.name()};
      std::string text{node.child_value()};
      boost::algorithm::trim(text);
      mStrings[name] = text;
    }
  } else {
    gui::guiLogger()->error("Strings XML does not have a node with name "
                            "'Strings'");
  }
}

StringsElement::StringsElement(pugi::xml_node doc) {
  parseXmlDocument(doc);
}

Trait<std::string> StringsElement::makeTrait(const std::string &name) const {
  auto it{mStrings.find(name)};
  if (it == mStrings.end()) {
    gui::guiLogger()->warn("{} is not a strings() trait", name);
    return Trait<std::string>(name, "");
  }
  const std::string value{it->second};
  return Trait<std::string>(name, value);
}

} // namespace gui