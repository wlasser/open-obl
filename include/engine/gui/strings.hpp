#ifndef OPENOBLIVION_ENGINE_GUI_STRINGS_HPP
#define OPENOBLIVION_ENGINE_GUI_STRINGS_HPP

#include "engine/settings.hpp"
#include "engine/gui/trait.hpp"
#include "ogre/text_resource_manager.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <pugixml.hpp>
#include <spdlog/spdlog.h>

namespace engine::gui {

class StringsElement {
 private:
  std::unordered_map<std::string, std::string> mStrings{};
  Ogre::TextResourceManager &txtMgr{Ogre::TextResourceManager::getSingleton()};

  std::stringstream openXMLStream(const std::string &filename) const {
    auto logger{spdlog::get(settings::log)};
    auto &txtMgr{Ogre::TextResourceManager::getSingleton()};
    auto stringsPtr{txtMgr.getByName(filename, settings::resourceGroup)};
    if (!stringsPtr) {
      logger->error("Resource {} does not exist", filename);
      throw std::runtime_error("Failed to open strings file");
    }
    stringsPtr->load(false);
    return std::stringstream{stringsPtr->getString()};
  }

  pugi::xml_document readXMLDocument(std::stringstream &is) const {
    auto logger{spdlog::get(settings::log)};
    pugi::xml_document doc{};
    pugi::xml_parse_result result{doc.load(is)};
    if (!result) {
      logger->error("Failed to parse strings XML [{}]: {}",
                    result.offset, result.description());
      throw std::runtime_error("Failed to parse strings XML");
    }
    return doc;
  }

 public:
  explicit StringsElement(const std::string &filename) {
    auto logger{spdlog::get(settings::log)};
    auto menuStream{openXMLStream(filename)};
    auto doc{readXMLDocument(menuStream)};

    const auto stringsNode{doc.find_child_by_attribute("name", "Strings")};
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

  Trait<std::string> makeTrait(const std::string &name) const {
    auto it{mStrings.find(name)};
    if (it == mStrings.end()) {
      spdlog::get(settings::log)->warn("{} is not a strings() trait", name);
      return Trait<std::string>(name, "");
    }
    const std::string value{it->second};
    return Trait<std::string>(name, value);
  };
};

} // namespace engine::gui

#endif // OPENOBLIVION_ENGINE_GUI_STRINGS_HPP
