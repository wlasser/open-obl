#ifndef OPENOBLIVION_GUI_STRINGS_HPP
#define OPENOBLIVION_GUI_STRINGS_HPP

#include "gui/trait.hpp"
#include "ogre/text_resource_manager.hpp"
#include <absl/container/flat_hash_map.h>
#include <pugixml.hpp>

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
/// <rect name="Strings>
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
/// trait.
class StringsElement {
 private:
  absl::flat_hash_map<std::string, std::string> mStrings{};
  Ogre::TextResourceManager &txtMgr{Ogre::TextResourceManager::getSingleton()};

  /// Open the `Ogre::TextResource` with the given `filename` and return a
  /// stream to it.
  /// \throws std::runtime_error if the resource does not exist.
  std::stringstream openXMLStream(const std::string &filename) const;

  /// Parse a text stream `is` representing an XML document into an actual
  /// document.
  /// \throws std::runtime_error if the document fails to parse.
  pugi::xml_document readXMLDocument(std::stringstream &is) const;

 public:
  /// Load an XML document of localized strings from the `Ogre::TextResource`
  /// called `filename`.
  explicit StringsElement(const std::string &filename);

  /// Construct a user trait whose value is the localized string with the given
  /// identifier `name`.
  /// \remark If there is no localized string with identifier equal to `name`,
  ///         then this returns a `Trait` with the requested `name` whose value
  ///         is the empty string.
  Trait<std::string> makeTrait(const std::string &name) const;
};

} // namespace gui

#endif // OPENOBLIVION_GUI_STRINGS_HPP
