#include "gui/logging.hpp"
#include "gui/xml.hpp"
#include "ogre/text_resource_manager.hpp"
#include "settings.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <pugixml.hpp>
#include <cstdlib>
#include <sstream>
#include <string>

namespace gui {

std::stringstream openXmlStream(const std::string &filename) {
  auto &txtMgr{Ogre::TextResourceManager::getSingleton()};
  auto xmlPtr{txtMgr.getByName(filename, oo::RESOURCE_GROUP)};
  if (!xmlPtr) {
    gui::guiLogger()->error("XML file '{}' does not exist", filename);
    throw std::runtime_error("Could not open stream, XML file does not exist");
  }

  xmlPtr->load();
  return std::stringstream{xmlPtr->getString()};
}

pugi::xml_document readXmlDocument(std::istream &is) {
  pugi::xml_document doc{};
  if (const auto result{doc.load(is)}; !result) {
    gui::guiLogger()->error("Failed to parse XML [offset {}]: {}",
                            result.offset, result.description());
    throw std::runtime_error("Failed to parse XML");
  }
  return doc;
}

pugi::xml_document readXmlDocument(const std::string &filename) {
  auto is{gui::openXmlStream(filename)};
  return gui::readXmlDocument(is);
}

template<> bool parseXmlEntity(const std::string &entity) {
  return entity == "&true;";
}

template<> int getXmlValue(pugi::xml_node node) {
  // stoi discards whitespace so we don't need to trim.
  // 0 means the base is autodetected.
  // There is a string construction here but otherwise we need strtol which
  // complains when sizeof(long) > sizeof(int).
  return std::stoi(node.value(), nullptr, 0);
}

template<> int getXmlChildValue(pugi::xml_node node, const char *name) {
  return std::stoi(node.child_value(name), nullptr, 0);
}

template<> int getXmlChildValue(pugi::xml_node node) {
  return std::stoi(node.child_value(), nullptr, 0);
}

template<> float getXmlValue(pugi::xml_node node) {
  // No string construction necessary, unlike with getXmlValue<int>
  return std::strtof(node.value(), nullptr);
}

template<> float getXmlChildValue(pugi::xml_node node, const char *name) {
  return std::strtof(node.child_value(name), nullptr);
}

template<> float getXmlChildValue(pugi::xml_node node) {
  return std::strtof(node.child_value(), nullptr);
}

template<> bool getXmlValue(pugi::xml_node node) {
  return parseXmlEntity<bool>(getXmlValue<std::string>(node));
}

template<> bool getXmlChildValue(pugi::xml_node node, const char *name) {
  return parseXmlEntity<bool>(getXmlChildValue<std::string>(node, name));
}

template<> bool getXmlChildValue(pugi::xml_node node) {
  return parseXmlEntity<bool>(getXmlChildValue<std::string>(node));
}

template<> std::string getXmlValue(pugi::xml_node node) {
  std::string value{node.value()};
  boost::algorithm::trim(value);
  return value;
}

template<> std::string getXmlChildValue(pugi::xml_node node, const char *name) {
  std::string value{node.child_value(name)};
  boost::algorithm::trim(value);
  return value;
}

template<> std::string getXmlChildValue(pugi::xml_node node) {
  std::string value{node.child_value()};
  boost::algorithm::trim(value);
  return value;
}

} // namespace gui