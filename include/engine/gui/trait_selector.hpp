#ifndef OPENOBLIVION_ENGINE_GUI_TRAIT_SELECTOR_HPP
#define OPENOBLIVION_ENGINE_GUI_TRAIT_SELECTOR_HPP

#include <pugixml.hpp>
#include <optional>
#include <string>

namespace engine::gui {

// Trait selectors look up the name of an element based on a rule and an
// optional argument, like parent() or sibling(foo). They can be used instead
// of trait names in operators.
struct TraitSelector {
  enum class Type {
    child, last, me, parent, screen, sibling, strings
  };
  Type type{};
  std::optional<std::string> argument{};
};

// Attempt to read the selector string as a TraitSelector
std::optional<TraitSelector> tokenizeTraitSelector(std::string selector);

// Ascend back up the tree, building the fully-qualified name of the node.
std::string fullyQualifyName(pugi::xml_node node);

// If an argument is given, then the 'child' selector returns the
// fully-qualified name of a descendant of node, whose name matches the
// argument, by performing a depth-first-search but iterating over the children
// in reverse order. If no argument is given, then the selector returns the
// fully-qualified name of the last non-trait child of node.
std::string invokeChildSelector(const pugi::xml_node &node,
                                std::optional<std::string> arg);

// Not sure what this one is supposed to do, presumably it is not equivalent
// to child(), which would return the last child of node. What it actually does
// is return an empty string.
std::string invokeLastSelector(const pugi::xml_node &node);

// Return the fully-qualified name of the containing element.
// See invokeSelector.
std::string invokeMeSelector(const pugi::xml_node &node);

// Return the fully-qualified name of the containing element's parent.
// See invokeSelector.
std::string invokeParentSelector(const pugi::xml_node &node);

// screen is an implementation defined element describing screen dimensions
// in normalized coordinates (NC). If screen width / screen height >= 1 then
// the height is normalized to 960px and the width computed according to the
// aspect ratio. Otherwise, the width is normalized to 1280px and the height
// is computed according to the aspect ratio. screen has the following traits:
//  - <width>: the screen width in NC
//  - <height>: the screen height in NC
//  - <cropX>: the horizontal safe zone margin width in NC
//  - <cropY>: the vertical safe zone margin height in NC
std::string invokeScreenSelector();

// If an argument is given then return the fully-qualified name of the sibling
// of the node whose name matches the argument. If no argument is given then
// return the fully-qualified name of the sibling defined before the node.
// This function is of the opinion that you are not your own sibling, calling
// sibling(foo) inside foo will return an empty string, as will sibling() when
// foo is an only child.
std::string invokeSiblingSelector(const pugi::xml_node &node,
                                  std::optional<std::string> arg);

// strings.xml is used for localization purposes, each trait takes the value
// of a localized string.
std::string invokeStringsSelector();

// Return whatever the selector selects, starting from node. It is expected that
// node point to the containing parent element of the operator invoking the
// selector, so usually one has to go at least one level up before calling this.
std::string invokeSelector(const pugi::xml_node &node,
                           const TraitSelector &selector);

} // namespace engine::gui

#endif // OPENOBLIVION_ENGINE_GUI_TRAIT_SELECTOR_HPP
