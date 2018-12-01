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

// Every element has a set of named values called traits, given as children of
// its root XML node. Each trait has a particular type T (defined by the
// implementation, not specified syntactically) and is described by
// an XML element whose type is the name of the trait. The trait element may
// contain a single value of type T or an entire function returning a T that is
// allowed to depend on the traits of any other ui element in the menu, along
// with some implementation defined traits.
// There is a standard list of traits defined by the implementation that affect
// the state of the ui directly. These have the same meaning across every ui
// element that uses them. There is also a finite set of numbered user traits
// whose meaning depends on the ui element containing them, and which are used
// to exchange data with the implementation. Finally, the user can define any
// number of custom traits whose name begins with an '_'. These traits are
// ignored by the implementation and can be used as configurable data or as
// functions.
// Since it is not possible to know ahead of time whether a trait will be a
// constant value, a compile-time (not C++ compile-time, but at the time of
// parsing the XML file on load) computable function, or a runtime-dependent
// function, we choose to represent everything dynamically. At compile-time we
// generate two parallel representations of the menu.
// The concrete representation consists of actual ui elements as recognised by
// the rendering engine, backed by classes deriving from UiElement. These
// override virtual setter functions for each of the implementation and user
// traits that they care about, which should update the state of the ui
// appropriately.
// The dynamic representation is a graph (hopefully a tree) whose nodes are
// the trait names of all ui elements in the menu, prefixed by their parent
// name, e.g. "AudioMenu.locus". This includes custom traits, unlike the
// concrete representation. Since element names must be unique within menus,
// the parent name is sufficient for unique trait lookup in functions. An edge
// is made from trait A to trait B if trait B is defined by a function that uses
// the value of trait A, thus giving a dependency graph of traits. When a value
// of one trait is modified (by the implementation or by another trait), the
// values of all child traits are updated using the new value. Functions may
// refer to other traits by naming the parent directly or using a selector.
// Since the structure of the menu cannot change at runtime, it is possible to
// resolve all selectors to absolute names at compile-time.
// To link the two representations, every node in the dynamic representation
// that corresponds to an implementation or user trait is given a pointer to
// its concrete representative, and calls its corresponding virtual setter on
// update. Custom traits still have nodes in the dynamic representation, but
// since they do not correspond to ui state, they do not call any methods on
// a concrete node (indeed they do not even have a concrete representative).

namespace gui {

// All the Menu<MenuType> inherit from UiElement, so why not just do everything
// with a UiElement* ? Well we want to select the value of MenuType at runtime
// without doing an if-else over every value of MenuType; a variant encapsulates
// a map from MenuType to Menu<MenuType> which lets us do this via
// enumvar::defaultConstruct.
using MenuVariant = enumvar::sequential_variant<MenuType, Menu, MenuType::N>;

// Once the correct Menu has been constructed, we can drop back to runtime
// polymorphism by casting the stored variant value to a pointer to its base.
UiElement *extractUiElement(MenuVariant &menu);
const UiElement *extractUiElement(const MenuVariant &menu);

template<class ...Ts>
auto makeInterfaceBufferImpl(const std::variant<Ts...> &var) {
  return std::visit([](auto &&t) -> std::variant<typename Ts::UserInterface...> {
    return (typename std::decay_t<decltype(t)>::UserInterface) {};
  }, var);
}

// A variant of user trait interfaces coinciding with MenuVariant.
// Handwaving, MenuVariant::UserInterface = MenuInterfaceVariant.
using MenuInterfaceVariant = decltype(makeInterfaceBufferImpl(
    std::declval<MenuVariant>()));

// Construct a user interface buffer from the menu
MenuInterfaceVariant makeInterfaceBuffer(const MenuVariant &menuVar);

pugi::xml_document loadDocument(std::istream &is);

std::pair<pugi::xml_node, MenuType> getMenuNode(const pugi::xml_document &doc);

std::string getMenuElementName(pugi::xml_node menuNode);

std::vector<std::unique_ptr<UiElement>>
addChildren(Traits &traits,
            pugi::xml_node parentNode,
            UiElement *parentElement);

// Parse an entire menu from an XML stream
void parseMenu(std::istream &is);

// MenuType specializations
template<>
MenuType getXmlValue(const pugi::xml_node &node);

template<>
MenuType getXmlChildValue(const pugi::xml_node &node, const char *name);

template<>
MenuType getXmlChildValue(const pugi::xml_node &node);

template<>
MenuType parseXmlEntity(const std::string &entity);

} // namespace gui

#endif // OPENOBLIVION_GUI_GUI_HPP
