#ifndef OPENOBL_GUI_STRINGS_HPP
#define OPENOBL_GUI_STRINGS_HPP

#include "gui/trait.hpp"
#include <pugixml.hpp>
#include <string>
#include <unordered_map>

namespace gui {

/// Element containing all localized strings.
///
/// A localized string consists of an *identifier* that names the string,
/// say `_mystring`, and a *value* that contains the localization, say
/// "My String".
///
/// This element takes on construction an XML file describing the localized
/// strings, similar to the XML used to describe a general UI element.
/// The XML file must contain a single `<rect>` node with attribute `name` equal
/// to `Strings`. As children, the `<rect>` node must have a sequence of
/// `<NAME>` nodes, where each `NAME` is replaced by the identifier for
/// the localized string, and the node's content is the string's value.
///
/// Each localized string determines a custom trait with name given by its
/// `NAME` identifier and with value equal to the localized string. Since each
/// trait is a custom trait, the identifiers should begin with a single
/// underscore `_` character. For example,
/// ```xml
/// <!-- strings.xml -->
/// <rect name="Strings">
///     <_exit>Exit</_exit>
///     <_howmany>How Many?</_howmany>
/// </rect>
/// ```
/// defines a trait `_exit` with value `Exit` and a trait `_howmany` with value
/// `How Many?`.
///
/// To avoid cluttering the dependency graph, each localized string *does not*
/// generate a user trait automatically; `makeTrait()` must be called with the
/// identifier of each localized string that should have an associated user
/// trait. The name of this element (and hence the dotted prefix for each trait
/// name) is implementation-defined; call `getName()` and `getPrefix()` to get
/// the name of the element and dotted prefix respectively.
/// \remark `getName()` and `getPrefix()` return constants; they are the same
///         for every instantiation of `StringsElement`. One should be careful
///         using multiple StringsElement in the same dependency graph, and
///         prefer using just one. See `Traits` for example usage.
class StringsElement {
 private:
  std::unordered_map<std::string, std::string> mStrings{};

  /// Parse an XML document of localized strings and store them.
  void parseXmlDocument(pugi::xml_node doc);

  static constexpr inline std::string_view NAME{"__strings"};
  static constexpr inline std::string_view PREFIX{"__strings."};

 public:
  /// Load an XML document of localized strings.
  explicit StringsElement(pugi::xml_node doc);

  /// Construct a user trait whose value is the localized string with the given
  /// identifier `name`.
  /// \remark If there is no localized string with identifier equal to `name`,
  ///         then this returns a `Trait` with the requested `name` whose value
  ///         is the empty string.
  Trait<std::string> makeTrait(const std::string &name) const;

  /// Return the implementation-defined name of the `StringsElement`.
  static constexpr std::string_view getName() {
    return NAME;
  }

  /// Return `getName()`, followed by a dot.
  static constexpr std::string_view getPrefix() {
    return PREFIX;
  }
};

} // namespace gui

#endif // OPENOBL_GUI_STRINGS_HPP
