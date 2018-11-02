#include "gui/xml.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <pugixml.hpp>
#include <cstdlib>
#include <string>

namespace gui::xml {

template<>
bool parseEntity(const std::string &entity) {
  return entity == "&true;";
}

template<>
int getValue(const pugi::xml_node &node) {
  // stoi discards whitespace so we don't need to trim.
  // 0 means the base is autodetected.
  // There is a string construction here but otherwise we need strtol which
  // complains when sizeof(long) > sizeof(int).
  return std::stoi(node.value(), nullptr, 0);
}

template<>
int getChildValue(const pugi::xml_node &node, const char *name) {
  return std::stoi(node.child_value(name), nullptr, 0);
}

template<>
int getChildValue(const pugi::xml_node &node) {
  return std::stoi(node.child_value(), nullptr, 0);
}

template<>
float getValue(const pugi::xml_node &node) {
  // No string construction necessary, unlike with getValue<int>
  return std::strtof(node.value(), nullptr);
}

template<>
float getChildValue(const pugi::xml_node &node, const char *name) {
  return std::strtof(node.child_value(name), nullptr);
}

template<>
float getChildValue(const pugi::xml_node &node) {
  return std::strtof(node.child_value(), nullptr);
}

template<>
bool getValue(const pugi::xml_node &node) {
  return parseEntity<bool>(getValue<std::string>(node));
}

template<>
bool getChildValue(const pugi::xml_node &node, const char *name) {
  return parseEntity<bool>(getChildValue<std::string>(node, name));
}

template<>
bool getChildValue(const pugi::xml_node &node) {
  return parseEntity<bool>(getChildValue<std::string>(node));
}

template<>
std::string getValue(const pugi::xml_node &node) {
  std::string value{node.value()};
  boost::algorithm::trim(value);
  return value;
}

template<>
std::string getChildValue(const pugi::xml_node &node, const char *name) {
  std::string value{node.child_value(name)};
  boost::algorithm::trim(value);
  return value;
}

template<>
std::string getChildValue(const pugi::xml_node &node) {
  std::string value{node.child_value()};
  boost::algorithm::trim(value);
  return value;
}

} // namespace gui::xml