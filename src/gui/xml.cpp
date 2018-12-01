#include "gui/xml.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <pugixml.hpp>
#include <cstdlib>
#include <string>

namespace gui {

template<>
bool parseXmlEntity(const std::string &entity) {
  return entity == "&true;";
}

template<>
int getXmlValue(const pugi::xml_node &node) {
  // stoi discards whitespace so we don't need to trim.
  // 0 means the base is autodetected.
  // There is a string construction here but otherwise we need strtol which
  // complains when sizeof(long) > sizeof(int).
  return std::stoi(node.value(), nullptr, 0);
}

template<>
int getXmlChildValue(const pugi::xml_node &node, const char *name) {
  return std::stoi(node.child_value(name), nullptr, 0);
}

template<>
int getXmlChildValue(const pugi::xml_node &node) {
  return std::stoi(node.child_value(), nullptr, 0);
}

template<>
float getXmlValue(const pugi::xml_node &node) {
  // No string construction necessary, unlike with getXmlValue<int>
  return std::strtof(node.value(), nullptr);
}

template<>
float getXmlChildValue(const pugi::xml_node &node, const char *name) {
  return std::strtof(node.child_value(name), nullptr);
}

template<>
float getXmlChildValue(const pugi::xml_node &node) {
  return std::strtof(node.child_value(), nullptr);
}

template<>
bool getXmlValue(const pugi::xml_node &node) {
  return parseXmlEntity<bool>(getXmlValue<std::string>(node));
}

template<>
bool getXmlChildValue(const pugi::xml_node &node, const char *name) {
  return parseXmlEntity<bool>(getXmlChildValue<std::string>(node, name));
}

template<>
bool getXmlChildValue(const pugi::xml_node &node) {
  return parseXmlEntity<bool>(getXmlChildValue<std::string>(node));
}

template<>
std::string getXmlValue(const pugi::xml_node &node) {
  std::string value{node.value()};
  boost::algorithm::trim(value);
  return value;
}

template<>
std::string getXmlChildValue(const pugi::xml_node &node, const char *name) {
  std::string value{node.child_value(name)};
  boost::algorithm::trim(value);
  return value;
}

template<>
std::string getXmlChildValue(const pugi::xml_node &node) {
  std::string value{node.child_value()};
  boost::algorithm::trim(value);
  return value;
}

} // namespace gui