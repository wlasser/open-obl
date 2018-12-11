#ifndef OPENOBLIVION_GUI_XML_HPP
#define OPENOBLIVION_GUI_XML_HPP

#include <pugixml.hpp>
#include <set>
#include <string>

namespace gui {

/// Load an XML document from the `Ogre::TextResource` with the given name in
/// the `oo::RESOURCE_GROUP` resource group.
/// \throws std::runtime_error if the file could not be loaded.
/// \throws std::runtime_error if the file could not be parsed as an XML file.
pugi::xml_document loadDocument(const std::string &filename);

/// Load an XML document from the stream, throwing if the loading fails.
/// \throws std::runtime_error if the stream could not be parsed as an XML file.
pugi::xml_document loadDocument(std::istream &is);

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

template<class T> T getXmlValue(pugi::xml_node node) = delete;
template<class T>
T getXmlChildValue(pugi::xml_node node, const char *name) = delete;
template<class T> T getXmlChildValue(pugi::xml_node node) = delete;

// int specializations
template<> int getXmlValue(pugi::xml_node node);
template<> int getXmlChildValue(pugi::xml_node node, const char *name);
template<> int getXmlChildValue(pugi::xml_node node);

// float specializations
template<> float getXmlValue(pugi::xml_node node);
template<> float getXmlChildValue(pugi::xml_node node, const char *name);
template<> float getXmlChildValue(pugi::xml_node node);

// bool specialization
template<> bool getXmlValue(pugi::xml_node node);
template<> bool getXmlChildValue(pugi::xml_node node, const char *name);
template<> bool getXmlChildValue(pugi::xml_node node);

// std::string specialization
template<> std::string getXmlValue(pugi::xml_node node);
template<> std::string getXmlChildValue(pugi::xml_node node, const char *name);
template<> std::string getXmlChildValue(pugi::xml_node node);
///@}

/// Find the node closest to `node` that matches the predicate `p`.
/// Specifically, find the node satisfying `p` that can be reached from `node`
/// in the minimum number of edge traversals out of all nodes satisying `p`.
/// `node` itself is included in the search space, so will be returned if it
/// matches the predicate. This is effectively a pivot so that `node` becomes
/// the root of the tree, followed by a breadth-first search.
template<class Predicate>
pugi::xml_node findClosestNode(pugi::xml_node node, Predicate &&p) {
  // TODO: Optimize this. Can the predicate be checked during insertion instead?
  std::set<pugi::xml_node> visited{};
  std::set<pugi::xml_node> frontier{node};
  for (unsigned int d{0};; ++d) {
    std::set<pugi::xml_node> newFrontier{};
    for (const auto &n : frontier) {
      if (p(n)) return n;
      if (auto parent{n.parent()}; parent) {
        //C++20: if (!visited.contains(parent)) newFrontier.insert(parent);
        if (visited.count(parent) == 0) newFrontier.insert(parent);
      }
      for (auto child : n.children()) {
        //C++20: if (!visited.contains(child)) newFrontier.insert(child);
        if (visited.count(child) == 0) newFrontier.insert(child);
      }
    }
    frontier = std::move(newFrontier);
    if (frontier.empty()) return pugi::xml_node{};
  }
}

} // namespace gui

#endif // OPENOBLIVION_ENGINE_GUI_XML_HPP
