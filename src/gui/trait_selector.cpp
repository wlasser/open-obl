#include "gui/logging.hpp"
#include "gui/strings.hpp"
#include "gui/trait_selector.hpp"
#include "gui/xml.hpp"
#include <boost/range/adaptor/reversed.hpp>
#include <boost/algorithm/string/join.hpp>
#include <regex>

namespace gui {

// TODO: Use std::string_view and a faster regex
std::optional<TraitSelector> tokenizeTraitSelector(const std::string &src) {
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
  } else {
    return std::nullopt;
  }

  // Third match is argument, if any
  if (selectorMatch.size() == 3 && !selectorMatch[2].str().empty()) {
    parsedSelector.argument = selectorMatch[2].str();
  }

  return parsedSelector;
}

std::string fullyQualifyName(pugi::xml_node node) {
  // Can't just use node.path as that uses tag names, not name attributes.
  std::vector<std::string> ancestors;
  for (; node.attribute("name"); node = node.parent()) {
    ancestors.emplace_back(node.attribute("name").value());
  }
  return boost::algorithm::join(ancestors | boost::adaptors::reversed, ".");
}

std::string
invokeChildSelector(pugi::xml_node node, std::optional<std::string> arg) {
  if (arg) {
    // Search through this node's descendants for a node whose name matches the
    // argument, but search from the last sibling to the first
    for (auto child : node.children() | boost::adaptors::reversed) {
      if (child.attribute("name") && *arg == child.attribute("name").value()) {
        return gui::fullyQualifyName(child);
      } else {
        const auto childName{gui::invokeChildSelector(child, arg)};
        if (!childName.empty()) return childName;
      }
    }
    return "";
  } else {
    // Just return this node's last child that has a name attribute; this
    // ensures that we are not returning a trait.
    for (auto child : node.children() | boost::adaptors::reversed) {
      if (child.attribute("name")) {
        return gui::fullyQualifyName(child);
      }
    }
    // No valid children, so return nothing
    return "";
  }
}

std::string invokeLastSelector(pugi::xml_node node) {
  gui::guiLogger()->warn("last() is unimplemented (offset {})",
                         node.offset_debug());
  return "";
}

std::string invokeMeSelector(pugi::xml_node node) {
  return gui::fullyQualifyName(node);
}

std::string invokeParentSelector(pugi::xml_node node) {
  return gui::fullyQualifyName(node.parent());
}

std::string invokeScreenSelector() {
  return "__screen";
}

std::string
invokeSiblingSelector(pugi::xml_node node, std::optional<std::string> arg) {
  const auto parent{node.parent()};
  if (arg) {
    // The unique sibling with the given name is the first child of the parent
    // with that name, since names are unique.
    if (auto child = parent.find_child_by_attribute("name", arg->c_str())) {
      // sibling(me()) == "" contractually
      if (*arg == node.attribute("name").value()) return "";
      else return gui::fullyQualifyName(child);
    }
    return "";
  } else {
    // previous_sibling might return a trait, so we have to walk backwards and
    // find the first non-trait.
    for (auto sibling = node.previous_sibling(); sibling;
         sibling = sibling.previous_sibling()) {
      if (sibling.attribute("name")) return gui::fullyQualifyName(sibling);
    }
  }
  return "";
}

std::string invokeStringsSelector() {
  return std::string{gui::StringsElement::getName()};
}

std::string invokeSelector(pugi::xml_node node, const TraitSelector &selector) {
  switch (selector.type) {
    case TraitSelector::Type::child: {
      return gui::invokeChildSelector(node, selector.argument);
    }
    case TraitSelector::Type::last: {
      return gui::invokeLastSelector(node);
    }
    case TraitSelector::Type::me: {
      return gui::invokeMeSelector(node);
    }
    case TraitSelector::Type::parent: {
      return gui::invokeParentSelector(node);
    }
    case TraitSelector::Type::screen: {
      return gui::invokeScreenSelector();
    }
    case TraitSelector::Type::sibling: {
      return gui::invokeSiblingSelector(node, selector.argument);
    }
    case TraitSelector::Type::strings: {
      return gui::invokeStringsSelector();
    }
    default: return "";
  }
}

std::optional<std::string> resolveTrait(pugi::xml_node node) {
  const auto srcAttr{node.attribute("src")};
  const auto traitAttr{node.attribute("trait")};

  if (!(srcAttr && traitAttr)) return std::nullopt;

  const std::string trait{traitAttr.value()};
  if (const auto
        selector{gui::tokenizeTraitSelector(srcAttr.value())}; selector) {
    // node is (most likely) an operator, but gui::invokeSelector expects to be
    // given the containing element, so we need to go up the tree and find it.
    for (; node && !node.attribute("name"); node = node.parent());
    if (!node) return std::nullopt;
    return gui::invokeSelector(node, *selector) + "." + trait;
  } else {
    // src is unlikely to be fully-qualified, but by itself does not necessarily
    // uniquely identify a uiElement. This happens in prefabs, which refer to
    // uiElements within themselves and which could be included multiple times.
    // If src is ambiguous then, the 'closest' matching uiElement to node should
    // be chosen.
    const std::string src{srcAttr.value()};
    node = gui::findClosestNode(node, [&](pugi::xml_node n) {
      return src == n.attribute("name").value();
    });
    return gui::fullyQualifyName(node) + "." + trait;
  }
}

} // namespace gui
