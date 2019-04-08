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

/// Return pointers to the child `UiElement`s of the given `node`.
std::vector<UiElementNode> getChildElements(pugi::xml_node node);

/// Bind all of `node`'s traits to `uiElement`, then recurse through its child
/// `UiElement`s and do the same.
/// \returns All the descendant `UiElementNode`s of `uiElement`, **not**
///          including `uiElement` itself.
/// \remark The nodes are arranged in depth-first order.
std::vector<UiElementNode>
addDescendants(Traits &traits, UiElement *uiElement, pugi::xml_node node);

/// \name MenuType specializations
///@{
template<> MenuType getXmlValue(pugi::xml_node node);
template<> MenuType getXmlChildValue(pugi::xml_node node, const char *name);
template<> MenuType getXmlChildValue(pugi::xml_node node);
template<>
void XmlEntityConverter::operator()(std::string_view entity,
                                    boost::optional<MenuType> &out) const;
///@}

class MenuContext::Impl {
 private:
  std::unique_ptr<Traits> mTraits;
  std::unique_ptr<MenuVariant> mMenu;
  std::vector<UiElementNode> mUiElements;
  pugi::xml_document mDocument;

 public:
  Impl(std::unique_ptr<Traits> traits,
       std::unique_ptr<MenuVariant> menu,
       std::vector<UiElementNode> uiElements,
       pugi::xml_document document);

  static std::optional<MenuContext>
  loadMenu(pugi::xml_document doc,
           std::optional<pugi::xml_document> stringsDoc);

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
};

} // namespace gui

#endif //OPENOBLIVION_GUI_IMPL_HPP
