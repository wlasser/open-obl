#ifndef OPENOBLIVION_GUI_TRAIT_SELECTOR_HPP
#define OPENOBLIVION_GUI_TRAIT_SELECTOR_HPP

#include <pugixml.hpp>
#include <optional>
#include <string>

namespace gui {

/// Trait selectors look up the name of an element based on a rule and an
/// optional argument, like `parent()` or `sibling(foo)`. They can be used
/// instead of trait names in operators.
struct TraitSelector {
  enum class Type {
    child, last, me, parent, screen, sibling, strings
  };
  Type type{};
  std::optional<std::string> argument{};
};

/// Attempt to read the selector string as a `TraitSelector`.
std::optional<TraitSelector> tokenizeTraitSelector(const std::string &selector);

/// Ascend back up the tree, building the fully-qualified name of the `node`.
/// Joins the `name` attributes of each of `node`'s ancestors separating them
/// by a dot `.`. The `name` attribute of `node` is also included. The `name`s
/// are concatenated from outside-in left-to-right. For example, in
/// ```xml
/// <menu name="Example">
///   <rect name="foo">
///     <x>0</x>
///   </rect>
/// </menu>
/// ```
/// the `<rect>` node has fully-qualified name `Example.foo`.
std::string fullyQualifyName(pugi::xml_node node);

/// Resolve the `child()` selector.
/// If an argument is given, return the fully-qualified name of a descendant of
/// `node`, whose name matches the argument, by performing a depth-first-search
/// but iterating over the children in reverse order. If no argument is given,
/// then the selector returns the fully-qualified name of the last non-trait
/// child of `node`.
std::string
invokeChildSelector(pugi::xml_node node, std::optional<std::string> arg);

/// Resolve the `last()` selector.
/// Not sure what this one is supposed to do, presumably it is not equivalent
/// to `child()`, which would return the last child of node. What it actually
/// does is return an empty string.
std::string invokeLastSelector(pugi::xml_node node);

/// Resolve the `me()` selector.
/// Returns the fully-qualified name of the containing element.
/// \see invokeSelector
std::string invokeMeSelector(pugi::xml_node node);

/// Resolve the `parent()` selector.
/// Returns the fully-qualified name of the containing element's parent.
/// \see invokeSelector
std::string invokeParentSelector(pugi::xml_node node);

/// Resolve the `screen()` selector.
/// Evaluates to the name of an implementation-defined element representing the
/// screen.
/// \see ScreenElement
std::string invokeScreenSelector();

/// Resolve the `sibling()` selector.
/// If an argument is given then return the fully-qualified name of the sibling
/// of the node whose name matches the argument. If no argument is given then
/// return the fully-qualified name of the sibling defined before the node.
/// This function is of the opinion that you are not your own sibling, calling
/// `sibling(foo)` inside `foo` will return an empty string, as will `sibling()`
/// when `foo` is an only child.
std::string
invokeSiblingSelector(pugi::xml_node node, std::optional<std::string> arg);

/// Resolve the `string()` selector.
/// Evaluates to the name of an implementation-defined element representing the
/// set of localized strings.
/// \see StringsElement
std::string invokeStringsSelector();

/// Return whatever the `selector` selects, starting from `node`. It is expected
/// that `node` point to the containing parent element of the operator invoking
/// the selector, so usually one has to go at least one level up before calling
/// this.
std::string invokeSelector(pugi::xml_node node, const TraitSelector &selector);

/// Return the fully-qualified path of the trait pointed to by the `src` and
/// `trait` attributes of `node`, if any. If `src` corresponds to a selector,
/// then it is invoked.
/// \returns The fully-qualified path of the pointed to trait, or an empty
///          optional if `node` is missing at least one of `src` and `trait`.
std::optional<std::string> resolveTrait(pugi::xml_node node);

} // namespace gui

#endif // OPENOBLIVION_GUI_TRAIT_SELECTOR_HPP
