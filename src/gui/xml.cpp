#include "fs/path.hpp"
#include "gui/logging.hpp"
#include "gui/xml.hpp"
#include "ogre/text_resource_manager.hpp"
#include "util/settings.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <boost/convert.hpp>
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
  auto doc{gui::readXmlDocument(is)};
  gui::processIncludes(doc);
  return doc;
}

void processIncludes(pugi::xml_document &doc) {
  gui::preOrderDFS(doc, [](pugi::xml_node &node) -> bool {
    if (node.name() != std::string{"include"}) return true;

    // Grab filename to include
    const auto srcAttr{node.attribute("src")};
    if (!srcAttr) {
      gui::guiLogger()->warn("Found <include> tag with no src, ignoring");
      return true;
    }
    const std::string filename{srcAttr.value()};

    // Load included document
    pugi::xml_document subDoc;
    try {
      auto path{oo::Path{"menus/prefabs"} / oo::Path{filename}};
      subDoc = gui::readXmlDocument(path.c_str());
    } catch (const std::runtime_error &e) {
      gui::guiLogger()->error("<include> tag with src {} failed to load",
                              filename);
      throw;
    }

    // Add children of node documents, since can't add xml_document directly
    pugi::xml_node parent{node.parent()};
    pugi::xml_node lastAdded{node};
    for (auto childNode : subDoc.children()) {
      lastAdded = parent.insert_copy_after(childNode, lastAdded);
    }

    // Removing the node invalidates iterators, but since it will be ignored
    // by the rest of the gui code we can just leave it.
    return false;
  });
}

template<>
void XmlEntityConverter::operator()(std::string_view entity,
                                    boost::optional<bool> &out) const {
  if (entity == "&true;") out = true;
  else if (entity == "&false;") out = false;
  else if (entity == "&xbox;") out = false;
  else if (entity == "&xenon;") out = false;
  else out = boost::none;
}

template<>
void XmlEntityConverter::operator()(std::string_view entity,
                                    boost::optional<float> &out) const {
  if (entity == "&xbuttona;") out = 1;
  else if (entity == "&xbuttonb;") out = 1;
  else if (entity == "&xbuttonlb;") out = 1;
  else if (entity == "&xbuttonls;") out = 1;
  else if (entity == "&xbuttonlt;") out = 1;
  else if (entity == "&xbuttonrb;") out = 1;
  else if (entity == "&xbuttonrs;") out = 1;
  else if (entity == "&xbuttonrt;") out = 1;
  else if (entity == "&xbuttonx;") out = 1;
  else if (entity == "&xbuttony;") out = 1;
  else if (entity == "&left;") out = 1;
  else if (entity == "&right;") out = 2;
  else if (entity == "&center;") out = 4;
  else out = boost::none;
}

template<> float getXmlValue(pugi::xml_node node) {
  if (auto opt{boost::convert<float>(getXmlValue<std::string>(node),
                                     XmlEntityConverter{})}) {
    return opt.value();
  }
  return std::strtof(node.value(), nullptr);
}

template<> float getXmlChildValue(pugi::xml_node node, const char *name) {
  if (auto opt{boost::convert<float>(getXmlChildValue<std::string>(node, name),
                                     XmlEntityConverter{})}) {
    return opt.value();
  }
  return std::strtof(node.child_value(name), nullptr);
}

template<> float getXmlChildValue(pugi::xml_node node) {
  if (auto opt{boost::convert<float>(getXmlChildValue<std::string>(node),
                                     XmlEntityConverter{})}) {
    return opt.value();
  }
  return std::strtof(node.child_value(), nullptr);
}

template<> bool getXmlValue(pugi::xml_node node) {
  return boost::convert<bool>(getXmlValue<std::string>(node),
                              XmlEntityConverter{}).value();
}

template<> bool getXmlChildValue(pugi::xml_node node, const char *name) {
  return boost::convert<bool>(getXmlChildValue<std::string>(node, name),
                              XmlEntityConverter{}).value();
}

template<> bool getXmlChildValue(pugi::xml_node node) {
  return boost::convert<bool>(getXmlChildValue<std::string>(node),
                              XmlEntityConverter{}).value();
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