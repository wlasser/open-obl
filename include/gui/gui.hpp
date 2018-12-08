#ifndef OPENOBLIVION_GUI_GUI_HPP
#define OPENOBLIVION_GUI_GUI_HPP

#include "enum_template.hpp"
#include "gui/menu.hpp"
#include "gui/menus/loading_menu.hpp"
#include "gui/trait.hpp"
#include "gui/traits.hpp"
#include "gui/trait_selector.hpp"
#include "gui/ui_element.hpp"
#include "gui/xml.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <functional>
#include <regex>
#include <stdexcept>
#include <tuple>
#include <type_traits>
#include <variant>

/// \file gui.hpp
/// \defgroup OpenOblivionGui Gui Library
/// XML-based user interface.
///
/// ## UI Elements
/// Every visible element of a GUI in this framework is an instance of some
/// class derived from `gui::UiElement`. These derived classes, which we shall
/// generically call *uiElements*, each represent some category of visual
/// elements, such as `gui::Text` or `gui::Image`. The `gui::Rect` uiElement
/// does not produce visible output directly, but rather serves as a container
/// for other uiElements, enabling more complicated positioning of uiElements.
///
/// The nesting of uiElements enabled by `gui::Rect` makes a generic GUI (i.e.
/// a collection of uiElements) into a forest. If the GUI itself is treated as
/// some kind of 'root' node whose children are its constituent uiElements, then
/// the GUI becomes a tree. This root node is itself a uiElement, called a
/// `gui::Menu`. These elements must always be top-level; no container uiElement
/// (such as a `gui::Rect` or another `gui::Menu`) may contain a `gui::Menu`.
///
/// Each menu (i.e. each specialization of `gui::Menu`) is associated to a
/// `gui::MenuType` which specifies the role of the menu. For instance, the menu
/// of type `gui::MenuType::InventoryMenu` is displayed whenever the player
/// opens their inventory, and the menu of type `gui::MenuType::LoadMenu` is
/// displayed during `record::CELL` loads. We say 'the' menu because it is
/// expected that only one menu of each type exist. If two different menus are
/// needed to represent the same type, then they must be used in two different
/// contexts so are not really the same type. Note that this does not mean that
/// `gui::Menu` is in any way a singleton; it is the *design* of the menu that
/// is expected to be unique to each `gui::MenuType`, not any in-process
/// instance of a particular menu.
///
/// The layout of the uiElements of a menu is specified using XML. Each XML file
/// describes a single menu, and begins appropriately with a `<menu>` element.
/// Other uiElements are written as children elements of the `<menu>` element.
/// For example, `gui::Image`, `gui::Text`, and `gui::Rect` uiElements are
/// described by `<image>`, `<text>`, and `<rect>` XML elements, respectively.
/// Every XML element representing a uiElement must have a `name` attribute
/// whose value uniquely identifies that uiElement within the menu. Since the
/// menu is a uiElement, its `<menu>` tag must also have a uniquely identifying
/// name within the set of all menus in the application. The dot `.` separated
/// concatenation of names of the ancestors of a uiElement describe its
/// *fully-qualified name* (see `gui::fullyQualifyName()`), which uniquely
/// identifies that uiElement.
///
/// It is usual that the name of a menu reflects its type, but there is no
/// requirement for it to do so. Instead, the menu type is set through a child
/// `<class>` element, whose value is an XML entity mirroring the name of the
/// corresponding `gui::MenuType` entry. For example, one would use the
/// `&InventoryMenu``;` entity to mark a menu as a
/// `gui::MenuType::InventoryMenu`.
///
/// ## Traits
/// Every uiElement has a set of named values called *traits*, given as children
/// of its XML element. Each trait has a particular type `T` (defined by the
/// implementation, not specified syntactically in the XML) and is described by
/// an XML element whose type is the name of the trait. The trait element may
/// contain a single value of type `T` or an entire function returning a `T`
/// that is allowed to depend on the traits of any other uiElement in the menu,
/// along with some implementation-defined traits which exist externally to the
/// menu. The allowed types are the typical integer, floating-point, boolean,
/// and string types.
///
/// There is a standard list of traits defined by the implementation that affect
/// the state of the ui directly. These have the same meaning across every
/// uiElement that uses them, and are called *implementation traits*. (Note the
/// distinction between implementation traits and implementation-defined traits,
/// apologies for the naming conflict there.) There is also a finite set of
/// numbered *user traits* whose meaning depends on the uiElement containing
/// them, and which are used to exchange data with the implementation. Finally,
/// the user can define any number of *custom traits* whose name begins with an
/// underscore `_`. These traits are ignored by the implementation and can be
/// used to implement variables and functions.
///
/// Since it is not possible to know ahead of time whether a trait will be a
/// constant value, a compile-time (not C++ compile-time, but at the time of
/// parsing the XML file) computable function, or a runtime-dependent function,
/// we choose to represent everything dynamically. At compile-time we generate
/// two parallel representations of the menu.
///
/// The *concrete representation* consists of actual uiElements as recognised by
/// the rendering engine, backed by classes deriving from `gui::UiElement`.
/// These override virtual setter functions for each of the implementation and
/// user traits that they care about, which should update the state of the ui
/// appropriately.
///
/// The *dynamic representation* is a graph (hopefully a tree) whose nodes are
/// the trait names of all uiElements in the menu, prefixed by the
/// fully-qualified name of their parent uiElement. This includes custom traits,
/// unlike the concrete representation. An edge in the graph is made from trait
/// `A` to trait `B` if trait `B` is defined by a function that uses the value
/// of trait `A`, thus giving a dependency graph of traits. When a value of one
/// trait is modified (by the implementation or by another trait), the values of
/// all child traits are updated using the new value.
///
/// Functions may refer to other traits by naming the parent directly or using a
/// *selector*. Selectors, such as `parent()`, look like C++ function calls and
/// resolve to a uiElement at runtime. The `parent()` selector is replaced by
/// the name of the parent of the uiElement containing the calling function,
/// for example. Selectors make it possible to refer to other uiElements
/// in the menu without precisely knowing the menu's structure. In particular,
/// one can create reusable XML snippets called *prefabs* that, while invalid on
/// their own, can be included into a menu XML file to add functionality to the
/// menu. The prefab uses selectors to refer to the uiElements in the XML file
/// that includes it, that it would otherwise have no knowledge of. Prefabs
/// are inserted verbatim into the containing XML file with the `<include>`
/// empty-element tag. The filename of the prefab is given in the `src`
/// attribute of the `<include>`.
///
/// While in principle thet selectors are resolved at runtime, because the
/// structure of the menu cannot change once it is created, it is possible to
/// resolve all selectors to fully-qualified names at (menu) compile-time.
///
/// To link the two representations, every node in the dynamic representation
/// that corresponds to an implementation trait or user trait is given a pointer
/// to its concrete representative, and calls its corresponding virtual setter
/// on update. Custom traits still have nodes in the dynamic representation, but
/// since they do not correspond to ui state, they do not call any methods on
/// a concrete node (indeed they do not even have a concrete representative).

/// \ingroup OpenOblivionGui
namespace gui {

/// Lift of MenuType from value space to type space.
/// All the Menu<MenuType> inherit from UiElement, so why not just do everything
/// with a UiElement* ? Well we want to select the value of MenuType at runtime
/// without doing an if-else over every value of MenuType; a variant
/// encapsulates a map from MenuType to Menu<MenuType> which lets us do this via
/// enumvar::defaultConstruct.
using MenuVariant = enumvar::sequential_variant<MenuType, Menu, MenuType::N>;

/// Once the correct Menu has been constructed, we can drop back to runtime
/// polymorphism by casting the stored variant value to a pointer to its base.
UiElement *extractUiElement(MenuVariant &menu);

/// \overload extractUiElement(MenuVariant &)
const UiElement *extractUiElement(const MenuVariant &menu);

template<class ...Ts>
auto makeInterfaceBufferImpl(const std::variant<Ts...> &var) {
  return std::visit([](auto &&t) -> std::variant<typename Ts::UserInterface...> {
    return (typename std::decay_t<decltype(t)>::UserInterface) {};
  }, var);
}

/// A variant of user trait interfaces coinciding with MenuVariant.
/// Handwaving, `MenuVariant::UserInterface = MenuInterfaceVariant`.
using MenuInterfaceVariant = decltype(makeInterfaceBufferImpl(
    std::declval<MenuVariant>()));

/// Construct a user interface buffer from the menu
MenuInterfaceVariant makeInterfaceBuffer(const MenuVariant &menuVar);

pugi::xml_document loadDocument(std::istream &is);

std::pair<pugi::xml_node, MenuType> getMenuNode(const pugi::xml_document &doc);

std::string getMenuElementName(pugi::xml_node menuNode);

std::vector<std::unique_ptr<UiElement>>
addChildren(Traits &traits,
            pugi::xml_node parentNode,
            UiElement *parentElement);

/// Parse an entire menu from an XML stream
void parseMenu(std::istream &is);

/// \name MenuType specializations
///@{
template<> MenuType getXmlValue(pugi::xml_node node);
template<> MenuType getXmlChildValue(pugi::xml_node node, const char *name);
template<> MenuType getXmlChildValue(pugi::xml_node node);
template<> MenuType parseXmlEntity(const std::string &entity);
///@}

} // namespace gui

#endif // OPENOBLIVION_GUI_GUI_HPP
