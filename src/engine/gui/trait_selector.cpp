#include "engine/gui/trait_selector.hpp"
#include <boost/range/adaptors.hpp>
#include <regex>

namespace engine::gui {

std::optional<TraitSelector> tokenizeTraitSelector(std::string src) {
  const std::regex selectorRegex{"(.+?)\\((.*?)\\)"};
  std::smatch selectorMatch{};
  std::regex_match(src, selectorMatch, selectorRegex);
  if (selectorMatch.empty()) {
    return std::nullopt;
  }

  TraitSelector parsedSelector{};

  // First match is the entire string, second match is the selector name
  const std::string selector{selectorMatch[1].str()};
  using namespace std::literals;
  if (selector == "child"s) {
    parsedSelector.type = TraitSelector::Type::child;
  } else if (selector == "last"s) {
    parsedSelector.type = TraitSelector::Type::last;
  } else if (selector == "me"s) {
    parsedSelector.type = TraitSelector::Type::me;
  } else if (selector == "parent"s) {
    parsedSelector.type = TraitSelector::Type::parent;
  } else if (selector == "screen"s) {
    parsedSelector.type = TraitSelector::Type::screen;
  } else if (selector == "sibling"s) {
    parsedSelector.type = TraitSelector::Type::sibling;
  } else if (selector == "strings"s) {
    parsedSelector.type = TraitSelector::Type::strings;
  }

  // Third match is argument, if any
  if (selectorMatch.size() == 3) {
    parsedSelector.argument = selectorMatch[2].str();
  }

  return parsedSelector;
}

std::string invokeChildSelector(const pugi::xml_node &node,
                                std::optional<std::string> arg) {
  if (arg) {
    // Search through this node's descendants for a node whose name matches the
    // argument, but search from the last sibling to the first
    for (const auto &child : node.children() | boost::adaptors::reversed) {
      if (arg == child.attribute("name").value()) {
        std::string parentName{node.attribute("name").value()};
        std::string childName{child.attribute("name").value()};
        return childName.append(".").append(parentName);
      } else {
        auto childName = invokeChildSelector(child, arg);
        if (!childName.empty()) return childName;
      }
    }
    return "";
  } else {
    // Just return this node's last child that has a name attribute; this
    // ensures that we are not returning a trait.
    for (const auto &child : node.children() | boost::adaptors::reversed) {
      if (child.attribute("name")) {
        std::string parentName{node.attribute("name").value()};
        std::string childName{child.attribute("name").value()};
        return childName.append(".").append(parentName);
      }
    }
    // No valid children, so return nothing
    return "";
  }
}

std::string invokeLastSelector(const pugi::xml_node &node) {
  // TODO: Warn that this is unimplemented
  return "";
}

std::string invokeMeSelector(const pugi::xml_node &node) {
  std::string parentName{node.parent().attribute("name").value()};
  std::string childName{node.attribute("name").value()};
  // <menu> elements have empty parents, in which case their fully-qualified
  // name is just their name.
  if (parentName.empty()) return childName;
  else return parentName.append(".").append(childName);
}

std::string invokeParentSelector(const pugi::xml_node &node) {
  std::string parentName{node.parent().attribute("name").value()};
  std::string grandparentName{node.parent().parent().attribute("name").value()};
  if (parentName.empty()) return "";
    // If the parent is a <menu> then the grandparent is empty, and the
    // fully-qualified parent name is just the parent name.
  else if (grandparentName.empty()) return parentName;
  else return grandparentName.append(".").append(parentName);
}

std::string invokeScreenSelector() {
  return "__screen";
}

std::string invokeSiblingSelector(const pugi::xml_node &node,
                                  std::optional<std::string> arg) {
  auto parent = node.parent();
  std::string parentName{parent.attribute("name").value()};
  if (arg) {
    // The unique sibling with the given name is the first child of the parent
    // with that name, since names are unique.
    if (parent.find_child_by_attribute("name", arg->c_str())) {
      // sibling(me()) == "" contractually
      if (*arg == node.attribute("name").value()) return "";
      else return parentName.append(".").append(*arg);
    }
    return "";
  } else {
    // previous_sibling might return a trait, so we have to walk backwards and
    // find the first non-trait.
    for (auto sibling = node.previous_sibling(); sibling;
         sibling = sibling.previous_sibling()) {
      if (sibling.attribute("name")) {
        return parentName.append(".").append(sibling.attribute("name").value());
      }
    }
  }
  return "";
}

std::string invokeStringsSelector() {
  return "__strings";
}

std::string invokeSelector(const pugi::xml_node &node,
                           const TraitSelector &selector) {
  switch (selector.type) {
    case TraitSelector::Type::child: {
      return invokeChildSelector(node, selector.argument);
    }
    case TraitSelector::Type::last: {
      return invokeLastSelector(node);
    }
    case TraitSelector::Type::me: {
      return invokeMeSelector(node);
    }
    case TraitSelector::Type::parent: {
      return invokeParentSelector(node);
    }
    case TraitSelector::Type::screen: {
      return invokeScreenSelector();
    }
    case TraitSelector::Type::sibling: {
      return invokeSiblingSelector(node, selector.argument);
    }
    case TraitSelector::Type::strings: {
      return invokeStringsSelector();
    }
  }
}

} // namespace engine::gui
