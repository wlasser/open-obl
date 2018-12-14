#ifndef OPENOBLIVION_GUI_TRAITS_HPP
#define OPENOBLIVION_GUI_TRAITS_HPP

#include "gui/screen.hpp"
#include "gui/stack/program.hpp"
#include "gui/strings.hpp"
#include "gui/trait.hpp"
#include "gui/trait_selector.hpp"
#include "gui/ui_element.hpp"
#include "gui/xml.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <pugixml.hpp>
#include <iosfwd>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace gui {

/// Encapsulate the dynamic representation of all traits associated with a menu
/// and its children.
class Traits {
 public:
  /// Type of the nodes in the dependency graph, represents a Trait of any type.
  using TraitVariant = std::variant<Trait<int>,
                                    Trait<float>,
                                    Trait<std::string>,
                                    Trait<bool>>;
 private:
  /// Vertex properties are required to be default constructible and copy
  /// constructible so we have to store a shared ownership pointer.
  using TraitVertex = std::shared_ptr<TraitVariant>;

  /// Type of the dependency graph.
  using TraitGraph = boost::adjacency_list<boost::vecS, boost::vecS,
                                           boost::directedS, TraitVertex>;

  /// Dependency graph of traits. There is an edge from `u` to `v` if the trait
  /// `v` requires the value of trait `u` to compute its value. That is, if `v`
  /// *depends on* `u`. This graph should be a DAG, and will usually have
  /// multiple connected components.
  TraitGraph mGraph{};

  /// Map for looking up traits by name in the dependency graph.
  std::unordered_map<std::string, TraitGraph::vertex_descriptor> mIndices{};

  /// Dependency graph vertex descriptors in (a) topological order.
  /// \warning This is not updated every time a trait is added, and is only
  /// valid if `isSorted()`.
  std::vector<TraitGraph::vertex_descriptor> mOrdering{};

  /// Whether `mOrdering` is sorted. Prefer using `isSorted()`.
  bool mSorted{false};

  /// Implementation-defined element storing screen settings.
  ScreenElement mScreen{};

  /// Implementation-defined element storing localized strings.
  std::optional<StringsElement> mStrings{};

  /// Check whether the dependency graph is still topologically sorted, or
  /// needs resorting.
  bool isSorted() const noexcept;

  /// Topologically sort the vertices in the dependency graph, store the result
  /// in `mOrdering`, and set `mSorted`. If already `isSorted()`, do nothing.
  /// \throws boost::not_a_dag if the underlying depdency graph is not a DAG.
  void sort();

  /// If the optional is nonempty then add the contained trait, overwriting any
  /// existing trait in the dependency graph with the same name.
  template<class T> void addTrait(std::optional<Trait<T>> trait);

  /// Return the names of the dependencies of a given `vertex`. Returns an empty
  /// vector if the `vertex` is null.
  std::vector<std::string> getDependencies(const TraitVertex &vertex) const;

 public:
  /// Return a reference to the dynamic trait with the fully-qualified `name`.
  /// Use this method when the type of the trait is not known ahead of time,
  /// prefer getTrait(const std::string &) when it is.
  /// \throws std::runtime_error if no trait exists with the given `name`.
  const TraitVariant &getTraitVariant(const std::string &name) const;

  /// Return a reference to the dynamic trait with fully-qualified `name`.
  /// \throws std::runtime_error if no trait exists with the given `name`, or if
  ///                            a trait does exist with that `name` but its
  ///                            underlying type is not `T`.
  template<class T> const Trait<T> &getTrait(const std::string &name) const;

  /// Construct a new trait with the given name by forwarding the `args` to the
  /// `Trait` constructor and adding it to the dependency graph.
  /// \returns a reference to the added trait.
  /// \remark No edges are created.
  template<class T, class ...Args>
  Trait<T> &addTrait(const std::string &name, Args &&... args);

  /// Add an already constructed trait to the dependency graph.
  /// \returns a reference to the added trait.
  template<class T> Trait<T> &addTrait(Trait<T> &&trait);

  /// Construct a `Trait` from the `node` and bind it to the `uiElement` with
  /// the `setterFun`, as in `Trait::bind()`.
  /// The XML `node` should describe the trait directly, such as `<x>100</x>`.
  template<class T> void addAndBindTrait(UiElement *uiElement,
                                         TraitSetterFun<T> setterFun,
                                         pugi::xml_node node);

  /// If the given XML `node` corresponds to an implementation trait, then bind
  /// it to the given `uiElement` and return `true`, otherwise return `false`.
  bool addAndBindImplementationTrait(pugi::xml_node node, UiElement *uiElement);

  /// If the given XML `node` corresponds to a user trait, then bind it to the
  /// given `uiElement` and return `true`, otherwise return `false`.
  bool addAndBindUserTrait(pugi::xml_node node, UiElement *uiElement);

  /// Add the traits of any implementation-defined elements that are required as
  /// dependencies of existing traits.
  void addImplementationElementTraits();

  /// Add the `uiElement`'s provided traits, overriding any existing traits with
  /// the same name.
  void addProvidedTraits(const UiElement *uiElement);

  /// Set all the user traits to point to the given interface buffer.
  template<class ...Ts>
  void setUserTraitSources(const std::tuple<Ts...> &userInterface);

  /// For each trait `v`, make an edge from `u` to `v` iff `u` is a dependency
  /// of `v`.
  /// Try to delay calling this until all traits have been added, as it
  /// regenerates all dependency edges, even those that haven't changed. Since
  /// traits are allowed to be defined out of order it doesn't really make sense
  /// to call this after every `addTrait()` anyway.
  /// \throws std::runtime_error if a trait has a nonexistent dependency.
  void addTraitDependencies();

  /// Update every trait, notifying the concrete representation of the new
  /// values.
  /// \throws std::runtime_error if there are any null vertices.
  /// \throws boost::not_a_dag if the underlying depdency graph is not a DAG.
  // TODO: Provide a method to update only a trait and its dependents
  void update();

  /// Load an XML document of localized strings.
  /// This function should be called at most per instance of `Traits`.
  /// In practice, calling it multiple times should work as expected, but that
  /// may change in the future.
  /// \see gui::StringsElement(const std::string&)
  void loadStrings(const std::string &filename);

  /// Print the dependency graph as a DOT file.
  void printDot(std::ostream &os);
};

/// Given an XML node representing a trait, produce a TraitFun which performs
/// the same operations. If the node does not represent a valid trait, then the
/// returned TraitFun<T> returns a value-initialized T.
template<class T>
TraitFun<T> getTraitFun(const Traits &traits, pugi::xml_node node) {
  if (node.text()) {
    const auto value{gui::getXmlChildValue<T>(node)};
    return TraitFun<T>{[value]() { return value; }};
  } else if (!node.first_child()) {
    // This happens in particular when `node` contains an empty string, or only
    // whitespace, such as `<foo>  </foo>`. Because `xml_text` uses an empty
    // string for failure, such cases are not counted as strings.
    return TraitFun<T>{[]() -> T { return {}; }};
  } else {
    gui::stack::Program prog{gui::stack::compile(node, &traits)};
    auto fun{TraitFun<T>([prog]() -> T { return std::get<T>(prog()); })};
    for (const auto &dep : prog.dependencies) {
      fun.addDependency(dep);
    }
    return fun;
  }
}

//===----------------------------------------------------------------------===//
// Traits member function template implementations
//===----------------------------------------------------------------------===//

template<class T> const Trait<T> &
Traits::getTrait(const std::string &name) const {
  const auto &var{getTraitVariant(name)};
  if (std::holds_alternative<Trait<T>>(var)) {
    return std::get<Trait<T>>(var);
  } else {
    throw std::runtime_error("Incorrect trait type");
  }
}

template<class T> void Traits::addTrait(std::optional<Trait<T>> trait) {
  if (!trait) return;
  const auto it{mIndices.find(trait->getName())};
  if (it != mIndices.end()) {
    boost::remove_vertex(it->second, mGraph);
  }
  (void) addTrait(std::move(*trait));
}

template<class T, class ...Args>
Trait<T> &Traits::addTrait(const std::string &name, Args &&... args) {
  mSorted = false;
  const auto index{boost::add_vertex(std::make_shared<TraitVariant>(
      Trait<T>{name, std::forward<Args>(args)...}), mGraph)};
  mIndices[name] = index;
  return std::get<Trait<T>>(*mGraph[index]);
}

template<class T> Trait<T> &Traits::addTrait(Trait<T> &&trait) {
  mSorted = false;
  const auto index{boost::add_vertex(std::make_shared<TraitVariant>(trait),
                                     mGraph)};
  mIndices[trait.getName()] = index;
  return std::get<Trait<T>>(*mGraph[index]);
}

template<class T> void Traits::addAndBindTrait(UiElement *uiElement,
                                               TraitSetterFun<T> setterFun,
                                               pugi::xml_node node) {
  auto fun{getTraitFun<T>(*this, node)};
  auto &trait{addTrait<T>(uiElement->get_name() + "." + node.name(),
                          std::move(fun))};
  trait.bind(uiElement, setterFun);
}

template<class ...Ts>
void Traits::setUserTraitSources(const std::tuple<Ts...> &userInterface) {
  for (auto vIndex : mGraph.vertex_set()) {
    const TraitVertex &traitPtr{mGraph[vIndex]};
    if (!traitPtr) {
      throw std::runtime_error("nullptr vertex");
    }
    auto &trait = *traitPtr;

    const std::optional<int> userIndex{std::visit([](auto &&v) {
      return getUserTraitIndex(v.getName());
    }, trait)};
    if (!userIndex) continue;

    std::visit([&userInterface](auto &&v) {
      v.setSource(userInterface);
    }, trait);
  }
}

} // namespace gui

#endif // OPENOBLIVION_GUI_TRAITS_HPP
