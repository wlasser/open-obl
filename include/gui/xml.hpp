#ifndef OPENOBLIVION_GUI_XML_HPP
#define OPENOBLIVION_GUI_XML_HPP

#include <pugixml.hpp>
#include <set>
#include <string>

namespace gui {

/// Open the `Ogre::TextResource` with the given `filename` in the
/// `oo::RESOURCE_GROUP` resource group and return a stream to it.
/// \throws std::runtime_error if the resource does not exist.
std::stringstream openXmlStream(const std::string &filename);

/// Load an XML document from the stream, throwing if the loading fails.
/// \throws std::runtime_error if the stream could not be parsed as an XML file.
pugi::xml_document readXmlDocument(std::istream &is);

/// Load an XML document from the `Ogre::TextResource` with the given name in
/// the `oo::RESOURCE_GROUP` resource group, processing any `<include>` tags.
/// \throws std::runtime_error if the file could not be loaded.
/// \throws std::runtime_error if the file could not be parsed as an XML file.
pugi::xml_document readXmlDocument(const std::string &filename);

/// Recursively process any `<include>` tags in the `doc`, modifying it in
/// place. The `src` of the `<include>` is interpreted relative to the
/// `menus/prefabs` directory, and is passed to
/// `gui::readXmlDocument(const std::string &)` after being qualified as such.
void processIncludes(pugi::xml_document &doc);

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
        if (visited.count(parent) == 0) {
          newFrontier.insert(parent);
          visited.insert(parent);
        }
      }
      for (auto child : n.children()) {
        //C++20: if (!visited.contains(child)) newFrontier.insert(child);
        if (visited.count(child) == 0) {
          newFrontier.insert(child);
          visited.insert(child);
        }
      }
    }
    frontier = std::move(newFrontier);
    if (frontier.empty()) return pugi::xml_node{};
  }
}

/// Traverse `node` and its children in depth-first pre-order, applying the
/// `visitor` to each node. The `visitor` should accept an lvalue reference
/// to a `pugi::xml_node` and return a boolean. If `false` is returned then the
/// subtree rooted at the passed node shall not be traversed, otherwise the
/// traversal continues as normal. In particular, the `visitor` is allows to
/// delete the passed node and its subtree, provided it returns `false`.
/// \warning Note the different meaning assigned to the `visitor`'s return value
///          compared to `pugi::xml_node::traverse`, where a `false` return
///          value means that the *entire* traversal should be stopped, not just
///          the current subtree.
/// \tparam F a function object with `bool operator()(pugi::xml_node &)`
template<class F>
void preOrderDFS(pugi::xml_node &node, F &&visitor) {
  if (!visitor(node)) return;
  for (auto &node : node.children()) {
    gui::preOrderDFS(node, std::forward<F>(visitor));
  }
}

} // namespace gui

#endif // OPENOBLIVION_ENGINE_GUI_XML_HPP
