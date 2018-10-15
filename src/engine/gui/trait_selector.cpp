#include "engine/gui/trait_selector.hpp"
#include <boost/range/adaptors.hpp>
#include <boost/algorithm/string/join.hpp>
#include <regex>

namespace engine::gui {

// TODO: Use std::string_view and a faster regex
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

std::string fullyQualifyName(pugi::xml_node node) {
  // Can't just use node.path as that uses tag names, not name attributes.
  std::vector<std::string> ancestors;
  for (; node.attribute("name"); node = node.parent()) {
    ancestors.push_back(node.attribute("name").value());
  }
  return boost::algorithm::join(ancestors | boost::adaptors::reversed, ".");
}

std::string invokeChildSelector(const pugi::xml_node &node,
                                std::optional<std::string> arg) {
  if (arg) {
    // Search through this node's descendants for a node whose name matches the
    // argument, but search from the last sibling to the first
    for (const auto &child : node.children() | boost::adaptors::reversed) {
      if (*arg == child.attribute("name").value()) {
        return fullyQualifyName(child);
      } else {
        const auto childName{invokeChildSelector(child, arg)};
        if (!childName.empty()) return childName;
      }
    }
    return "";
  } else {
    // Just return this node's last child that has a name attribute; this
    // ensures that we are not returning a trait.
    for (const auto &child : node.children() | boost::adaptors::reversed) {
      if (child.attribute("name")) {
        return fullyQualifyName(child);
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
  return fullyQualifyName(node);
}

std::string invokeParentSelector(const pugi::xml_node &node) {
  return fullyQualifyName(node.parent());
}

std::string invokeScreenSelector() {
  return "__screen";
}

std::string invokeSiblingSelector(const pugi::xml_node &node,
                                  std::optional<std::string> arg) {
  const auto parent{node.parent()};
  if (arg) {
    // The unique sibling with the given name is the first child of the parent
    // with that name, since names are unique.
    if (auto child = parent.find_child_by_attribute("name", arg->c_str())) {
      // sibling(me()) == "" contractually
      if (*arg == node.attribute("name").value()) return "";
      else return fullyQualifyName(child);
    }
    return "";
  } else {
    // previous_sibling might return a trait, so we have to walk backwards and
    // find the first non-trait.
    for (auto sibling = node.previous_sibling(); sibling;
         sibling = sibling.previous_sibling()) {
      if (sibling.attribute("name")) return fullyQualifyName(sibling);
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

std::optional<std::string> resolveTrait(pugi::xml_node node) {
  const auto srcAttr{node.attribute("src")};
  const auto traitAttr{node.attribute("trait")};

  if (!(srcAttr && traitAttr)) return std::nullopt;

  const std::string trait{traitAttr.value()};
  if (const auto selector{tokenizeTraitSelector(srcAttr.value())}; selector) {
    // node points to a trait and therefore has no non-operator children;
    // need to go up another level to begin searching.
    return invokeSelector(node.parent(), *selector) + "." + trait;
  } else {
    return std::string{srcAttr.value()} + "." + trait;
  }
}

} // namespace engine::gui
