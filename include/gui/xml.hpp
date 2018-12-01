#ifndef OPENOBLIVION_GUI_XML_HPP
#define OPENOBLIVION_GUI_XML_HPP

#include <pugixml.hpp>
#include <string>

namespace gui {

/// We don't have a DTD so can't specify custom entities directly. Instead they
/// should be treated as strings by the parser and decoded using the following
/// functions.
template<class T> T parseXmlEntity(const std::string &entity) = delete;

/// \overload parseXmlEntity(const std::string &)
template<> bool parseXmlEntity(const std::string &entity);

/// \name Xml node value getters
/// `xml_node::value` and `xml_node::child_value` return `const char *`, which
/// frequently has untrimmed whitespace due to the xml formatting, e.g.
/// `<x> 0 </x>` or `<locus> &true; </locus>`. These functions trim the
/// whitespace and convert to the requested type.
///@{

template<class T> T getXmlValue(const pugi::xml_node &node) = delete;

template<class T>
T getXmlChildValue(const pugi::xml_node &node, const char *name) = delete;

template<class T> T getXmlChildValue(const pugi::xml_node &node) = delete;

// int specializations
template<> int getXmlValue(const pugi::xml_node &node);

template<>
int getXmlChildValue(const pugi::xml_node &node, const char *name);

template<> int getXmlChildValue(const pugi::xml_node &node);

// float specializations
template<> float getXmlValue(const pugi::xml_node &node);

template<>
float getXmlChildValue(const pugi::xml_node &node, const char *name);

template<> float getXmlChildValue(const pugi::xml_node &node);

// bool specialization
template<> bool getXmlValue(const pugi::xml_node &node);

template<>
bool getXmlChildValue(const pugi::xml_node &node, const char *name);

template<> bool getXmlChildValue(const pugi::xml_node &node);

// std::string specialization
template<> std::string getXmlValue(const pugi::xml_node &node);

template<>
std::string getXmlChildValue(const pugi::xml_node &node, const char *name);

template<> std::string getXmlChildValue(const pugi::xml_node &node);
///@}

} // namespace gui

#endif // OPENOBLIVION_ENGINE_GUI_XML_HPP
