#ifndef OPENOBLIVION_ENGINE_GUI_GUI_HPP
#define OPENOBLIVION_ENGINE_GUI_GUI_HPP

#include "enum_template.hpp"
#include "engine/gui/xml.hpp"
#include <boost/graph/adjacency_list.hpp>
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
  // Transparency. 0 is completely transparent, 255 is completely opaque
  virtual void set_alpha(int alpha) {}
  // If true, this element is used to anchor the position of its children
  virtual void set_locus(bool locus) {}
  // If false, this element and all its descendants are hidden and un-clickable
  virtual void set_visible(bool visible) {}
  // Time in seconds for fade-in or fade-out
  virtual void set_menufade(float menufade) {}
  // This is probably distinct from menuFade, but we treat it as an alias
  virtual void set_explorefade(float explorefade) {
    set_menufade(explorefade);
  }

  // Every UiElement is required to have a name which identifies it uniquely in
  // the scope of the surrounding menu, or if the UiElement is a menu, then in
  // the scope of the application.
  virtual std::string get_name() const = 0;
  virtual void set_name(std::string name) = 0;
};

// Each menu must be one of the following types, given in the XML by its <class>
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
class Menu : public UiElement {
 protected:
  std::string mName{};

 public:
  std::string get_name() const override {
    return mName;
  }

  void set_name(std::string name) override {
    mName = name;
  }
};

// Shorthands for each concrete menu
// TODO: Shorthands for the rest of the menu types
using LoadingMenu = Menu<MenuType::LoadingMenu>;

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

// TraitFun represents a function used to set/compute the value of the dynamic
// representative of a trait.
template<class T>
using TraitFun = std::function<T(void)>;

// TraitSetterFun represents a function used to set the value of the concrete
// representative of a trait.
template<class T>
using TraitSetterFun = std::function<void(UiElement *, T)>;

// The Trait class encapsulates a dynamic representative of a trait, and should
// be bound to a concrete representative via an appropriate setter.
template<class T>
class Trait {
 private:
  TraitFun<T> mValue{};
  std::string mName{};
  TraitSetterFun<T> mSetter{};
  UiElement *mConcrete{};

 public:
  explicit Trait(std::string name, T &&t) : mName(std::move(name)),
                                            mValue([t]() { return t; }) {}
  explicit Trait(std::string name, const T &t) : mName(std::move(name)),
                                                 mValue([t]() { return t; }) {}

  explicit Trait(std::string name, TraitFun<T> t) : mName(std::move(name)),
                                                    mValue(t) {}

  Trait(const Trait<T> &) = default;
  Trait<T> &operator=(const Trait<T> &) = default;

  Trait(Trait &&) noexcept(std::is_nothrow_move_constructible_v<TraitFun<T>>
      && std::is_nothrow_move_constructible_v<TraitSetterFun<T>>) = default;

  Trait<T> &operator=(Trait &&) noexcept(
  std::is_nothrow_move_assignable_v<TraitFun<T>>
      && std::is_nothrow_move_assignable_v<TraitSetterFun<T>>) = default;

  // Bind this Trait as the concrete representative of a trait in the
  // concreteElement, whose value is modifable using the setter.
  void bind(UiElement *concreteElement, TraitSetterFun<T> setter) {
    mConcrete = concreteElement;
    mSetter = setter;
  }

  // Calculate the actual value of this trait. This does not update the concrete
  // representative.
  T invoke() const {
    return std::invoke(mValue);
  }

  // Calculate the actual value of this trait and update the concrete
  // representative, if any.
  void update() const {
    if (mConcrete) {
      auto v = invoke();
      std::invoke(mSetter, mConcrete, v);
    }
  }
};

// Encapsulate the dynamic representation of all traits associated with a menu
// and its children.
class Traits {
 private:
  using TraitVertex = std::variant<Trait<int>,
                                   Trait<float>,
                                   Trait<std::string>,
                                   Trait<bool>>;
  using TraitGraph = boost::adjacency_list<boost::vecS, boost::vecS,
                                           boost::directedS, TraitVertex>;

  // Dependency graph of traits. There is an edge from u to v if the trait v
  // requires the value of trait u to compute its value. This should be a DAG,
  // and will usually have multiple connected components.
  TraitGraph mGraph{};
  // Map for looking up traits by name in the dependency graph.
  std::unordered_map<std::string, TraitGraph::vertex_descriptor> mIndices{};
  // Dependency graph vertex descriptors in (a) topological order. This is not
  // updated every time a trait is added, and is only valid if mSorted == true.
  std::vector<TraitGraph::vertex_descriptor> mOrdering{};
  bool mSorted{false};

  // Topologically sort the vertices in the dependency graph, store the result
  // in mOrdering, and set mSorted. If the graph is already sorted, i.e. if
  // mSorted == true, then this does nothing.
  void sort();

 public:
  // Return a reference to the dynamic trait with the given fully-qualified name
  template<class T>
  Trait<T> &getTrait(const std::string &name) {
    auto index = mIndices.find(name);
    if (index == mIndices.end()) return Trait<T>(name, {});
    const auto &vertex = mGraph[index->second];
    if (std::holds_alternative<T>(vertex)) {
      return std::get<T>(vertex);
    } else {
      throw std::runtime_error("Incorrect trait type");
    }
  }

  // Construct a new trait with the given name by forwarding the args to the
  // Trait constructor, add it to the dependency graph, and return a reference
  // to the added trait. No edges are created.
  template<class T, class ...Args>
  Trait<T> &addTrait(std::string name, Args &&... args) {
    mSorted = false;
    auto index = boost::add_vertex(
        Trait<T>(std::move(name), std::forward<Args>(args)...),
        mGraph);
    return std::get<Trait<T>>(mGraph[index]);
  }

  // Given an XML node describing a trait, such as <x>100</x>, construct a
  // corresponding Trait and bind it to the uiElement with the setterFun as
  // in Trait::bind.
  template<class T>
  void addTraitAndBind(UiElement *uiElement, TraitSetterFun<T> setterFun,
                       const pugi::xml_node &node);

  // If the given XML node corresponds to an implementation trait, then bind it
  // to the given uiElement and return true, otherwise return false.
  bool addAndBindImplementationTrait(const pugi::xml_node &node,
                                     UiElement *uiElement);

  // Update every trait, notifying the concrete representation of the new
  // values. Throws if the underlying dependency graph is not a DAG.
  void update();
};

// Parse an entire menu from an XML stream
void parseMenu(std::istream &is);

// Given an XML node representing a trait, produce a TraitFun which performs the
// same operations. If the node does not represent a valid trait, then the
// returned TraitFun<T> returns a value-initialized T.
template<class T>
TraitFun<T> getTraitFun(const pugi::xml_node &node) {
  if (node.text()) {
    auto value = xml::getChildValue<T>(node);
    return [value]() { return value; };
  }
  return []() -> T { return {}; };
}

template<class T>
void Traits::addTraitAndBind(UiElement *uiElement,
                             TraitSetterFun<T> setterFun,
                             const pugi::xml_node &node) {
  auto fun = getTraitFun<T>(node);
  auto &trait = addTrait<T>(uiElement->get_name() + "." + node.name(), fun);
  trait.bind(uiElement, setterFun);
}

namespace xml {

// MenuType specializations
template<>
MenuType getValue(const pugi::xml_node &node);

template<>
MenuType getChildValue(const pugi::xml_node &node, const char *name);

template<>
MenuType getChildValue(const pugi::xml_node &node);

template<>
MenuType parseEntity(const std::string &entity);

}

} // namespace engine::gui

#endif // OPENOBLIVION_ENGINE_GUI_GUI_HPP
