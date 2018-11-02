#ifndef OPENOBLIVION_GUI_XML_HPP
#define OPENOBLIVION_GUI_XML_HPP

#include <pugixml.hpp>
#include <string>

namespace gui::xml {

// We don't have a DTD so can't specify custom entities directly. Instead they
// should be treated as strings by the parser and decoded using the following
// functions.
template<class T>
T parseEntity(const std::string &entity) = delete;

template<>
bool parseEntity(const std::string &entity);

// xml_node::value() and xml_node::child_value() return const char *, which
// frequently have untrimmed whitespace due to the xml formatting, e.g.
// <x> 0 </x> or <locus> &true; </locus>. These functions trim the whitespace
// and convert to the requested type.

// Base templates
template<class T>
T getValue(const pugi::xml_node &node) = delete;

template<class T>
T getChildValue(const pugi::xml_node &node, const char *name) = delete;

template<class T>
T getChildValue(const pugi::xml_node &node) = delete;

// int specializations
template<>
int getValue(const pugi::xml_node &node);

template<>
int getChildValue(const pugi::xml_node &node, const char *name);

template<>
int getChildValue(const pugi::xml_node &node);

// float specializations
template<>
float getValue(const pugi::xml_node &node);

template<>
float getChildValue(const pugi::xml_node &node, const char *name);

template<>
float getChildValue(const pugi::xml_node &node);

// bool specialization
template<>
bool getValue(const pugi::xml_node &node);

template<>
bool getChildValue(const pugi::xml_node &node, const char *name);

template<>
bool getChildValue(const pugi::xml_node &node);

// std::string specialization
template<>
std::string getValue(const pugi::xml_node &node);

template<>
std::string getChildValue(const pugi::xml_node &node, const char *name);

template<>
std::string getChildValue(const pugi::xml_node &node);

} // namespace gui::xml

#endif // OPENOBLIVION_ENGINE_GUI_XML_HPP
