#ifndef OPENOBLIVION_ENGINE_GUI_TRAITS_HPP
#define OPENOBLIVION_ENGINE_GUI_TRAITS_HPP

#include "engine/gui/screen.hpp"
#include "engine/gui/strings.hpp"
#include "engine/gui/trait.hpp"
#include "engine/gui/trait_selector.hpp"
#include "engine/gui/ui_element.hpp"
#include "engine/gui/xml.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <pugixml.hpp>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace engine::gui {

// Encapsulate the dynamic representation of all traits associated with a menu
// and its children.
class Traits {
 private:
  // Vertex properties are required to be default constructible and copy
  // constructible so we have to store a shared ownership pointer.
  using TraitVariant = std::variant<Trait<int>,
                                    Trait<float>,
                                    Trait<std::string>,
                                    Trait<bool>>;
  using TraitVertex = std::shared_ptr<TraitVariant>;
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

  // Implementation-defined element storing screen settings.
  const ScreenElement mScreen{};

  // Implementation-defined element storing localized strings.
  const StringsElement mStrings{"menus/strings.xml"};

  // Topologically sort the vertices in the dependency graph, store the result
  // in mOrdering, and set mSorted. If the graph is already sorted, i.e. if
  // mSorted == true, then this does nothing.
  void sort();

 public:
  // Return a reference to the dynamic trait with the given fully-qualified name
  template<class T>
  const Trait<T> &getTrait(const std::string &name) const {
    auto index = mIndices.find(name);
    if (index == mIndices.end()) {
      throw std::runtime_error("No such trait");
    }
    const auto &vertex = mGraph[index->second];
    if (!vertex) {
      throw std::runtime_error("nullptr vertex");
    }
    if (std::holds_alternative<Trait<T>>(*vertex)) {
      return std::get<Trait<T>>(*vertex);
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
    auto index = boost::add_vertex(std::make_shared<TraitVariant>(
        Trait<T>{std::move(name), std::forward<Args>(args)...}), mGraph);
    mIndices[name] = index;
    return std::get<Trait<T>>(*mGraph[index]);
  }

  // Add an already constructed trait to the dependency graph and return a
  // reference to it.
  template<class T>
  Trait<T> &addTrait(Trait<T> &&trait) {
    mSorted = false;
    auto index = boost::add_vertex(std::make_shared<TraitVariant>(trait),
                                   mGraph);
    mIndices[trait.getName()] = index;
    return std::get<Trait<T>>(*mGraph[index]);
  }

  // Given an XML node describing a trait, such as <x>100</x>, construct a
  // corresponding Trait and bind it to the uiElement with the setterFun as
  // in Trait::bind.
  template<class T>
  void addAndBindTrait(UiElement *uiElement, TraitSetterFun<T> setterFun,
                       const pugi::xml_node &node);

  // If the given XML node corresponds to an implementation trait, then bind it
  // to the given uiElement and return true, otherwise return false.
  bool addAndBindImplementationTrait(const pugi::xml_node &node,
                                     UiElement *uiElement);

  // If the given XML node corresponds to a user trait, then bind it to the
  // given uiElement and return true, otherwise return false.
  bool addAndBindUserTrait(const pugi::xml_node &node, UiElement *uiElement);

  // Return the names of the dependencies of a given vertex. Returns an empty
  // vector if the vertex is null.
  std::vector<std::string> getDependencies(const TraitVertex &vertex) const;

  // Add the traits of any implementation defined elements that are required as
  // dependencies of existing traits.
  void addImplementationElementTraits();

  // Set all the user traits to point to the given interface buffer.
  template<class ...Ts>
  void setUserTraitSources(const std::tuple<Ts...> &userInterface);

  // For each trait v, make an edge from u to v if u is a dependency of v.
  // This will throw if a trait has a nonexistent dependency.
  // Try to delay calling this until all traits have been added, as it
  // regenerates all dependency edges, even those that haven't changed. Since
  // traits are allowed to be defined out of order it doesn't really make sense
  // to call this after every addTrait anyway.
  void addTraitDependencies();

  // Update every trait, notifying the concrete representation of the new
  // values. Throws if the underlying dependency graph is not a DAG.
  // TODO: Provide a method to update only a trait and its dependents
  void update();
};

// When evaluating a trait that does not begin with a <copy> to initialize the
// working value, the working value is value-initialized on the first evaluation
// of the trait. On subsequent evaluations, the value of the previous evaluation
// is used as the initial working value. This is a lot like a function-local
// static variable, but cannot be achieved in the same way with lambdas; static
// variables in a lambda are defined in the surrounding function scope. Instead
// we use a stateful functor to remember the previous value, and use mutable
// lambdas to allow modifying its state.
template<class T>
struct PersistentFunctor {
  T state{};
  T operator()() const {
    return state;
  }
};

// Given a trait node whose body is given by a collection of operators, parse
// the sequence of operators into a TraitFun.
template<class T>
TraitFun<T> parseOperators(const Traits &traits, const pugi::xml_node &node) {
  using namespace std::literals;

  // Start with the <copy> if there is one, otherwise use a PersistentFunctor
  // to remember the previous value. The functor needs to be updated after the
  // entire evaluation is complete, which means it needs to act at both ends
  // of the evaluation; this is not possible with the nesting technique used.
  // Instead we have to pass a pointer to the functor all the way up the call
  // stack, then use it to make the update at the end. The working function
  // type is therefore not a TraitFun<T>.
  using FunctorPair = std::pair<PersistentFunctor<T> *, T>;
  std::function<FunctorPair(void)> fun{};
  // Need to keep track of the trait dependencies, which we can't give to the
  // TraitFun<T> until the end.
  std::vector<std::string> dependencies{};

  if (node.first_child().name() == "copy"s) {
    auto copyNode{node.first_child()};
    auto srcAttr = copyNode.attribute("src");
    auto traitAttr = copyNode.attribute("trait");
    if (srcAttr && traitAttr) {
      std::string src{srcAttr.value()};
      std::string trait{traitAttr.value()};
      const auto selector = tokenizeTraitSelector(src);
      if (selector) {
        // node points to a trait and therefore has no non-operator children; need
        // to go up another level to begin searching.
        src = invokeSelector(node.parent(), *selector);
      }

      // TODO: switch-case
      auto name = src + "." + trait;
      fun = [name, &tref = std::as_const(traits)]() -> FunctorPair {
        return {nullptr, tref.getTrait<T>(name).invoke()};
      };
      dependencies.push_back(std::move(name));
    }
  } else {
    fun = [functor = PersistentFunctor<T>{}]() mutable -> FunctorPair {
      return {&functor, functor()};
    };
  }

  // Now construct the actual function by embedding the functor update
  TraitFun<T> traitFun{[fun]() mutable -> T {
    auto[ptr, val] = fun();
    if (ptr) ptr->state = val;
    return val;
  }};
  // Notify the function of its dependencies
  for (auto &dep : dependencies) {
    traitFun.addDependency(std::move(dep));
  }

  return traitFun;
}

// Given an XML node representing a trait, produce a TraitFun which performs the
// same operations. If the node does not represent a valid trait, then the
// returned TraitFun<T> returns a value-initialized T.
template<class T>
TraitFun<T> getTraitFun(const Traits &traits, const pugi::xml_node &node) {
  if (node.text()) {
    auto value = xml::getChildValue<T>(node);
    return TraitFun<T>{[value]() { return value; }};
  } else {
    return parseOperators<T>(traits, node);
  }
}

template<class T>
void Traits::addAndBindTrait(UiElement *uiElement,
                             TraitSetterFun<T> setterFun,
                             const pugi::xml_node &node) {
  auto fun = getTraitFun<T>(*this, node);
  auto &trait = addTrait<T>(uiElement->get_name() + "." + node.name(), fun);
  trait.bind(uiElement, setterFun);
}

template<class ...Ts>
void Traits::setUserTraitSources(const std::tuple<Ts...> &userInterface) {
  for (TraitGraph::vertex_descriptor vIndex : mGraph.vertex_set()) {
    const TraitVertex &traitPtr = mGraph[vIndex];
    if (!traitPtr) {
      throw std::runtime_error("nullptr vertex");
    }
    auto &trait = *traitPtr;
    std::optional<int> userIndex{std::visit([](auto &&v) {
      return getUserTraitIndex(v.getName());
    }, trait)};

    if (!userIndex) continue;

    std::visit([&userInterface](auto &&v) {
      v.setSource(userInterface);
    }, trait);
  }
}

} // namespace engine::gui

#endif // OPENOBLIVION_ENGINE_GUI_TRAITS_HPP
