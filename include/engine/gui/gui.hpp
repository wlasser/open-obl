#ifndef OPENOBLIVION_ENGINE_GUI_GUI_HPP
#define OPENOBLIVION_ENGINE_GUI_GUI_HPP

#include "enum_template.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <pugixml.hpp>
#include <functional>
#include <type_traits>
#include <unordered_map>
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

namespace engine::gui {

class UiElement {
 public:
  // Position of left edge, relative to position of locus ancestor
  virtual void set_x(int x) {}
  // Position of top edge, relative to position of locus ancestor
  virtual void set_y(int y) {}
  // Width in pixels
  virtual void set_width(int width) {}
  // Height in pixels
  virtual void set_height(int height) {}
  // If true, this element is used to anchor the position of its children
  virtual void set_locus(bool locus) {}
  // If false, this element and all its descendants are hidden and un-clickable
  virtual void set_visible(bool visible) {}
  // Time in seconds for fade-in or fade-out
  virtual void set_menuFade(float menuFade) {}
  // This is probably distinct from menuFade, but we treat it as an alias
  virtual void set_exploreFade(float exploreFade) {
    set_menuFade(exploreFade);
  }
};

enum class MenuType {
  AlchemyMenu,
  AudioMenu,
  ClassMenu,
  ControlsMenu,
  CreditsMenu,
  DialogMenu,
  EffectSettingMenu,
  EnchantmentMenu,
  GameplayMenu,
  GenericMenu,
  HUDInfoMenu,
  HUDMainMenu,
  HUDSubtitleMenu,
  InventoryMenu,
  LevelUpMenu,
  LoadingMenu,
  LoadMenu,
  LockPickMenu,
  MagicMenu,
  MagicPopupMenu,
  MainMenu,
  MapMenu,
  MessageMenu,
  NegotiateMenu,
  OptionsMenu,
  PauseMenu,
  PersuasionMenu,
  QuantityMenu,
  QuickKeysMenu,
  RaceSexMenu,
  RechargeMenu,
  RepairMenu,
  SaveMenu,
  SigilStoneMenu,
  SkillsMenu,
  SleepWaitMenu,
  SpellMakingMenu,
  SpellPurchaseMenu,
  StatsMenu,
  TextEditMenu,
  TrainingMenu,
  VideoMenu,
  N
};

// The idea here is that we have a shallow inheritance hierarchy parameterised
// by the MenuType enum. Specialising this class template and overriding its
// corresponding methods allows us to do virtual dispatch based on a runtime
// enum value, without manually checking for each value.
template<MenuType Type>
class Menu : public UiElement {};

using LoadingMenu = Menu<MenuType::LoadingMenu>;

using MenuVariant = enumvar::sequential_variant<MenuType, Menu, MenuType::N>;

template<class T>
using TraitFunc = std::function<T(void)>;

template<class T>
class Trait {
 private:
  TraitFunc<T> mValue{};
  std::string mName{};

 public:
  explicit Trait(std::string name, T &&t) : mName(std::move(name)),
                                            mValue([t]() { return t; }) {}
  explicit Trait(std::string name, const T &t) : mName(std::move(name)),
                                                 mValue([t]() { return t; }) {}

  Trait(const Trait<T> &) = default;
  Trait<T> &operator=(const Trait<T> &) = default;
  Trait(Trait &&)
  noexcept(std::is_nothrow_move_constructible_v<TraitFunc<T>>) = default;
  Trait<T> &operator=(Trait &&)
  noexcept(std::is_nothrow_move_assignable_v<TraitFunc<T>>) = default;

  T invoke() const {
    return std::invoke(mValue);
  }
};

using TraitVertex = std::variant<Trait<int>,
                                 Trait<float>,
                                 Trait<std::string>,
                                 Trait<bool>>;
using TraitGraph = boost::adjacency_list<boost::vecS, boost::vecS,
                                         boost::directedS, TraitVertex>;

class Traits {
 private:
  TraitGraph mGraph{};
  std::unordered_map<std::string, TraitGraph::vertex_descriptor> mIndices{};

 public:
  template<class T>
  Trait<T> getTrait(const std::string &name) {
    auto index = mIndices.find(name);
    if (index == mIndices.end()) return Trait<T>(name, {});
    const auto &vertex = mGraph[index->second];
    if (std::holds_alternative<T>(vertex)) {
      return std::get<T>(vertex);
    } else {
      // TODO: Warn about incorrect type
      return Trait<T>(name, {});
    }
  }
};

namespace xml {

// We don't have a DTD so can't specify custom entities directly. Instead they
// should be treated as strings by the parser and decoded using the following
// functions.
template<class T>
T parseEntity(const std::string &entity) = delete;

template<>
bool parseEntity(const std::string &entity);

template<>
MenuType parseEntity(const std::string &entity);

// xml_node::value() and xml_node::child_value() return const char *, which
// frequently have untrimmed whitespace due to the xml formatting, e.g.
// <x> 0 </x> or <locus> &true; </locus>. These functions trim the whitespace
// and convert to the requested type.

// Base templates
template<class T>
T getValue(const pugi::xml_node &node) = delete;

template<class T>
T getChildValue(const pugi::xml_node &node, const char *name) = delete;

template<class T>
T getChildValue(const pugi::xml_node &node) = delete;

// int specializations
template<>
int getValue(const pugi::xml_node &node);

template<>
int getChildValue(const pugi::xml_node &node, const char *name);

template<>
int getChildValue(const pugi::xml_node &node);

// float specializations
template<>
float getValue(const pugi::xml_node &node);

template<>
float getChildValue(const pugi::xml_node &node, const char *name);

template<>
float getChildValue(const pugi::xml_node &node);

// bool specialization
template<>
bool getValue(const pugi::xml_node &node);

template<>
bool getChildValue(const pugi::xml_node &node, const char *name);

template<>
bool getChildValue(const pugi::xml_node &node);

// std::string specialization
template<>
std::string getValue(const pugi::xml_node &node);

template<>
std::string getChildValue(const pugi::xml_node &node, const char *name);

template<>
std::string getChildValue(const pugi::xml_node &node);

// MenuType specialization
template<>
MenuType getValue(const pugi::xml_node &node);

template<>
MenuType getChildValue(const pugi::xml_node &node, const char *name);

template<>
MenuType getChildValue(const pugi::xml_node &node);

// Parse an entire menu from an XML stream
void parseMenu(std::istream &is);

} // namespace xml

} // namespace engine::gui

#endif // OPENOBLIVION_ENGINE_GUI_GUI_HPP
