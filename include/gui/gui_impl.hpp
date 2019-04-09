#ifndef OPENOBLIVION_GUI_IMPL_HPP
#define OPENOBLIVION_GUI_IMPL_HPP

#include "gui/gui.hpp"
#include "gui/menu.hpp"
#include "gui/trait.hpp"
#include "gui/traits.hpp"
#include "gui/trait_selector.hpp"
#include "gui/xml.hpp"

namespace gui {

/// Return a base pointer to the current menu.
/// `MenuVariant` is useful for construction, but once the menu has been
/// constructed it is convenient to drop back to runtime polymorphism.
UiElement *extractUiElement(MenuVariant &menu);

/// \overload extractUiElement(MenuVariant &)
const UiElement *extractUiElement(const MenuVariant &menu);

/// Return the first `<menu>` child of the `doc`, and the `MenuType` represented
/// by its `<class>` child.
/// \throws std::runtime_error if `doc` does not have a `<menu>` child.
/// \throws std::runtime_error if the first `<menu>` does not have a `<class>`
///                            child.
std::pair<pugi::xml_node, MenuType> getMenuNode(pugi::xml_node doc);

/// Given XML and concrete representations of a `uiElement`, add all its child
/// traits and bind them to the `uiElement`.
void addTraits(Traits &traits, UiElement *uiElement, pugi::xml_node node);

/// Owned `UiElement` and the XML node which represents it.
using UiElementNode = std::pair<std::unique_ptr<UiElement>, pugi::xml_node>;

/// A ordered list of `UiElementNode`s.
using UiElementNodeList = std::vector<UiElementNode>;

/// Return the fully-qualified name of `node`, ensuring that it is unique among
/// all the other `uiElements`.
/// \remark If the fully-qualified name of `node` is shared by an already
///         existing node, then an underscore is appended to the end of the name
///         until the name is unique. This scheme is subject to change.
/// \remark The explicit uniquing is only required when siblings have the same
///         name; this is deprecated, and should be avoided as supporting it
///         causes node insertion to be `O(n)` in time instead of `O(1)`.
std::string
getFullyQualifiedName(pugi::xml_node node, const UiElementNodeList &uiElements);

/// Use the name of the `node` to deduce the type of `UiElement` that it
/// represents, constructing one with given unique fully-qualified `name`.
std::unique_ptr<UiElement>
makeUiElement(pugi::xml_node node, std::string name);

/// Return pointers to the child `UiElement`s of the given `node`.
UiElementNodeList getChildElements(pugi::xml_node node);

/// Bind all of `node`'s traits to `uiElement`, then recurse through its child
/// `UiElement`s and do the same.
/// \returns All the descendant `UiElementNode`s of `uiElement`, **not**
///          including `uiElement` itself.
/// \remark The nodes are arranged in depth-first order.
UiElementNodeList
addDescendants(Traits &traits, UiElement *uiElement, pugi::xml_node node);

std::optional<MenuContext>
loadMenu(pugi::xml_document doc, std::optional<pugi::xml_document> stringsDoc);

/// \name MenuType specializations
///@{
template<> MenuType getXmlValue(pugi::xml_node node);
template<> MenuType getXmlChildValue(pugi::xml_node node, const char *name);
template<> MenuType getXmlChildValue(pugi::xml_node node);
template<>
void XmlEntityConverter::operator()(std::string_view entity,
                                    boost::optional<MenuType> &out) const;
///@}

/// Used to construct a `MenuContext` within the library without exposing
/// `pugi::xml_document` to the user of `MenuContext`.
///
/// The problem is that `MenuContext` needs to be constructed with a
/// `pugi::xml_document`, but we don't want to expose that type to a user of the
/// library. Since we're already using PImpl with `MenuContext`, we can make
/// `MenuContext` constructable from a pointer to its implementation
/// `MenuContext::Impl`, then give `MenuContext::Impl` the necessary
/// constructor. Nobody can construct the implementation though because it's
/// a private member of `MenuContext`, so we grant friend access to this proxy
/// class that has a static member function forwarding its arguments to the
/// constructor. The implementation's constructor is public so that we can use
/// `std::make_unique`.
struct MenuContextProxy {
  static MenuContext makeMenuContext(std::unique_ptr<Traits> traits,
                                     std::unique_ptr<MenuVariant> menu,
                                     UiElementNodeList uiElements,
                                     pugi::xml_document document);
};

class MenuContext::Impl {
 private:
  std::unique_ptr<Traits> mTraits;
  std::unique_ptr<MenuVariant> mMenu;
  UiElementNodeList mUiElements;
  pugi::xml_document mDocument;
  std::unordered_map<std::string, pugi::xml_node> mTemplates;
  uint32_t mNumInstantiations{};

 public:
  Impl(std::unique_ptr<Traits> traits,
       std::unique_ptr<MenuVariant> menu,
       UiElementNodeList uiElements,
       pugi::xml_document document);

  /// \copydoc MenuContext::update()
  void update();
  /// \copydoc MenuContext::clearEvents()
  void clearEvents();
  /// \copydoc MenuContext::getOverlay()
  Ogre::Overlay *getOverlay() const;
  /// \copydoc MenuContext::normalizeCoordinates()
  Ogre::Vector2 normalizeCoordinates(int32_t x, int32_t y) const;

  /// \copydoc MenuContext::set_user()
  void set_user(int index, gui::UiElement::UserValue value);
  /// \copydoc MenuContext::get_user()
  gui::UiElement::UserValue get_user(int index);

  /// \copydoc MenuContext::getElementWithId()
  const gui::UiElement *getElementWithId(int id) const;

  /// \copydoc MenuContext::getElementWithId()
  gui::UiElement *getElementWithId(int id);

  /// \copydoc MenuContext::registerTemplates()
  std::size_t registerTemplates();

  /// \copydoc MenuContext::appendTemplate()
  gui::UiElement *appendTemplate(gui::UiElement *parent,
                                 const std::string &templateName);

  /// Insert a template node if one with the given `name` doesn't already exist,
  /// returning `true` if an insertion took place.
  bool insertTemplate(std::string name, pugi::xml_node node);
};

} // namespace gui

#endif //OPENOBLIVION_GUI_IMPL_HPP
