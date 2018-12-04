#include "gui/strings.hpp"
#include "gui/trait.hpp"
#include "ogre/text_resource_manager.hpp"
#include "settings.hpp"
#include <absl/container/flat_hash_map.h>
#include <boost/algorithm/string/trim.hpp>
#include <pugixml.hpp>
#include <spdlog/spdlog.h>
#include <string>

namespace gui {

std::stringstream
StringsElement::openXMLStream(const std::string &filename) const {
  auto logger{spdlog::get(oo::LOG)};
  auto &txtMgr{Ogre::TextResourceManager::getSingleton()};

  auto stringsPtr{txtMgr.getByName(filename, oo::RESOURCE_GROUP)};
  if (!stringsPtr) {
    logger->error("Resource {} does not exist", filename);
    throw std::runtime_error("Failed to open strings file");
  }

  stringsPtr->load(false);
  return std::stringstream{stringsPtr->getString()};
}

pugi::xml_document StringsElement::readXMLDocument(std::istream &is) const {
  auto logger{spdlog::get(oo::LOG)};
  pugi::xml_document doc{};

  pugi::xml_parse_result result{doc.load(is)};
  if (!result) {
    logger->error("Failed to parse strings XML [{}]: {}",
                  result.offset, result.description());
    throw std::runtime_error("Failed to parse strings XML");
  }

  return doc;
}

void StringsElement::parseXMLDocument(pugi::xml_document doc) {
  auto logger{spdlog::get(oo::LOG)};

  const auto stringsNode{doc.find_child([](pugi::xml_node node) {
    using namespace std::literals;
    const auto attr{node.attribute("name")};
    return node.name() == "rect"s && attr && attr.value() == "Strings"s;
  })};

  if (stringsNode) {
    for (auto node : stringsNode.children()) {
      const auto name{std::string{"__strings."} + node.name()};
      std::string text{node.child_value()};
      boost::algorithm::trim(text);
      mStrings[name] = text;
    }
  } else {
    logger->error("XML does not have a node with name 'Strings'");
  }
}

StringsElement::StringsElement(std::istream &is) {
  auto doc{readXMLDocument(is)};
  parseXMLDocument(std::move(doc));
}

StringsElement::StringsElement(const std::string &filename) {
  auto is{openXMLStream(filename)};
  auto doc{readXMLDocument(is)};
  parseXMLDocument(std::move(doc));
}

Trait<std::string> StringsElement::makeTrait(const std::string &name) const {
  auto it{mStrings.find(name)};
  if (it == mStrings.end()) {
    spdlog::get(oo::LOG)->warn("{} is not a strings() trait", name);
    return Trait<std::string>(name, "");
  }
  const std::string value{it->second};
  return Trait<std::string>(name, value);
};

} // namespace gui