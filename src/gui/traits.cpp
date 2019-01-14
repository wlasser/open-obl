#include "gui/gui.hpp"
#include "gui/logging.hpp"
#include "gui/strings.hpp"
#include "gui/trait.hpp"
#include "gui/traits.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/graph/graphviz.hpp>
#include <boost/graph/topological_sort.hpp>
#include <ostream>

namespace gui {

bool Traits::isSorted() const noexcept {
  return mSorted;
}

void Traits::sort() {
  if (mSorted) return;
  mOrdering.clear();
  // By our definitions, there is an edge uv iff v depends on u, and so in the
  // ordering u should come before v iff there is an edge uv.
  // boost::topological_sort says the reverse; that v comes before u iff there
  // is an edge uv.
  mOrdering.reserve(boost::num_vertices(mGraph));
  boost::topological_sort(mGraph, std::back_inserter(mOrdering));
  std::reverse(mOrdering.begin(), mOrdering.end());
  mSorted = true;
}

bool Traits::addAndBindImplementationTrait(pugi::xml_node node,
                                           UiElement *uiElement) {
  using namespace std::literals;
  if (node.name() == "x"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_x, node);
  } else if (node.name() == "y"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_y, node);
  } else if (node.name() == "width"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_width, node);
  } else if (node.name() == "height"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_height, node);
  } else if (node.name() == "depth"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_depth, node);
  } else if (node.name() == "alpha"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_alpha, node);
  } else if (node.name() == "red"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_red, node);
  } else if (node.name() == "green"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_green, node);
  } else if (node.name() == "blue"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_blue, node);
  } else if (node.name() == "locus"s) {
    addAndBindTrait<bool>(uiElement, &UiElement::set_locus, node);
  } else if (node.name() == "visible"s) {
    addAndBindTrait<bool>(uiElement, &UiElement::set_visible, node);
  } else if (node.name() == "menufade"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_menufade, node);
  } else if (node.name() == "explorefade"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_explorefade, node);
  } else if (node.name() == "filename"s) {
    addAndBindTrait<std::string>(uiElement, &UiElement::set_filename, node);
  } else if (node.name() == "zoom"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_zoom, node);
  } else if (node.name() == "target"s) {
    addAndBindTrait<bool>(uiElement, &UiElement::set_target, node);
  } else if (node.name() == "id"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_id, node);
  } else if (node.name() == "clicksound"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_clicksound, node);
  } else if (node.name() == "string"s) {
    addAndBindTrait<std::string>(uiElement, &UiElement::set_string, node);
  } else {
    return false;
  }
  return true;
}

bool Traits::addAndBindUserTrait(pugi::xml_node node, UiElement *uiElement) {
  const std::optional<int> indexOpt{getUserTraitIndex(node.name())};
  if (!indexOpt) return false;
  const int index{*indexOpt};

  auto setter = [index](UiElement *uiElement, auto &&value) {
    uiElement->set_user(index, value);
  };

  switch (uiElement->userTraitType(index)) {
    case TraitTypeId::Float: {
      addAndBindTrait<float>(uiElement, std::move(setter), node);
      break;
    }
    case TraitTypeId::Bool: {
      addAndBindTrait<bool>(uiElement, std::move(setter), node);
      break;
    }
    case TraitTypeId::String: {
      addAndBindTrait<std::string>(uiElement, std::move(setter), node);
      break;
    }
    case TraitTypeId::Unimplemented: {
      return false;
    }
  }

  return true;
}

bool Traits::queueCustomTrait(pugi::xml_node node, UiElement *uiElement) {
  const std::string name{node.name()};
  //C++20: if (!name.begins_with('_'))
  if (name.empty() || name[0] != '_') return false;

  // We don't know the type of this trait a priori so cannot add it to the trait
  // graph. Construct a stack program for it, which doesn't need to know the
  // return type, and defer addition until later.
  mDeferredTraits.emplace_back(uiElement->get_name() + "." + name,
                               gui::stack::compile(node, this),
                               gui::TraitTypeId::Unimplemented);
  return true;
}

std::vector<std::string>
Traits::getDependencies(const TraitVertex &vertex) const {
  if (vertex) {
    return std::visit([](const auto &v) {
      return v.getDependencies();
    }, vertex->var);
  } else {
    return {};
  }
}

const Traits::TraitVariant &
Traits::getTraitVariant(const std::string &name) const {
  const auto index{mIndices.find(name)};
  if (index == mIndices.end()) {
    gui::guiLogger()->error("Trait {} does not exist", name);
    throw std::runtime_error("No such trait");
  }
  const auto &vertex{mGraph[index->second]};
  if (!vertex) {
    gui::guiLogger()->error("Trait {} exists but is null", name);
    throw std::runtime_error("nullptr vertex");
  }
  return vertex->var;
}

void Traits::addImplementationElementTrait(const std::string &dep) {
  const std::string screenPrefix{ScreenElement::getPrefix()};

  if (dep == screenPrefix + "width") {
    addTrait(mScreen.makeWidthTrait());
  } else if (dep == screenPrefix + "height") {
    addTrait(mScreen.makeHeightTrait());
  } else if (dep == screenPrefix + "cropx") {
    addTrait(mScreen.makeCropXTrait());
  } else if (dep == screenPrefix + "cropy") {
    addTrait(mScreen.makeCropYTrait());
    //C++20: } else if (dep.starts_with(gui::StringsElement::getPrefix())) {
  } else if (boost::algorithm::starts_with(dep, StringsElement::getPrefix())) {
    if (mStrings) addTrait(mStrings->makeTrait(dep));
  }
}

void Traits::addImplementationElementTraits() {
  auto tryAddTrait = [&](const std::string &dep) {
    //C++20: if (dep.starts_with("__")) {
    if (boost::algorithm::starts_with(dep, "__")) {
      if (mIndices.find(dep) != mIndices.end()) return;
      addImplementationElementTrait(dep);
    }
  };

  for (TraitGraph::vertex_descriptor vIndex : mGraph.vertex_set()) {
    const TraitVertex &vPtr{mGraph[vIndex]};
    const auto deps{getDependencies(vPtr)};
    for (const auto &dep : deps) tryAddTrait(dep);
  }

  for (const auto &trait : mDeferredTraits) {
    for (const auto &dep : trait.program.dependencies) tryAddTrait(dep);
  }
}

void Traits::addProvidedTraits(const UiElement *uiElement) {
  addTrait(uiElement->make_x());
  addTrait(uiElement->make_y());
  addTrait(uiElement->make_width());
  addTrait(uiElement->make_height());
  addTrait(uiElement->make_filewidth());
  addTrait(uiElement->make_fileheight());
  addTrait(uiElement->make_alpha());
  addTrait(uiElement->make_locus());
  addTrait(uiElement->make_visible());
  addTrait(uiElement->make_menufade());
  addTrait(uiElement->make_explorefade());
  addTrait(uiElement->make_filename());
  addTrait(uiElement->make_zoom());
  addTrait(uiElement->make_clicked());
  addTrait(uiElement->make_shiftclicked());
  addTrait(uiElement->make_mouseover());
  addTrait(uiElement->make_childcount());
  addTrait(uiElement->make_child_count());
}

void Traits::deduceAndAddTrait(DeferredTrait trait) {
  gui::stack::ValueType val{trait.program()};
  if (std::holds_alternative<float>(val)) {
    addTrait<float>(std::move(trait));
  } else if (std::holds_alternative<bool>(val)) {
    addTrait<bool>(std::move(trait));
  } else if (std::holds_alternative<std::string>(val)) {
    addTrait<std::string>(std::move(trait));
  } else {
    gui::guiLogger()->error("The deduced return type of {} is not one of "
                            "float, bool, or std::string", trait.name);
    throw std::runtime_error("Unsupported custom trait type");
  }
}

void Traits::addQueuedCustomTraits() {
  auto tBegin{mDeferredTraits.begin()};
  auto tEnd{mDeferredTraits.end()};

  // If none of the trait's dependencies are custom traits then its return type
  // can be deduced immediately by evaluation.
  auto isCustomTrait = [&](const std::string &name) {
    return std::find_if(tBegin, tEnd, [&name](const DeferredTrait &trait) {
      return trait.name == name;
    }) != tEnd;
  };

  auto hasNoCustomDeps = [&](const DeferredTrait &trait) {
    const auto &deps{trait.program.dependencies};
    return std::none_of(deps.begin(), deps.end(), [&](const auto &dep) {
      return isCustomTrait(dep);
    });
  };

  for (auto &trait : mDeferredTraits) {
    if (hasNoCustomDeps(trait)) deduceAndAddTrait(std::move(trait));
  }

  mDeferredTraits.erase(std::remove_if(tBegin, tEnd, hasNoCustomDeps), tEnd);
  if (mDeferredTraits.empty()) return;

  // Any traits that are left depend on other custom traits. Topologically sort
  // them so they can be added in the correct order.
  auto g{makeDeferredTraitGraph(tBegin, tEnd)};
  std::vector<typename decltype(g)::vertex_descriptor>
      order(boost::num_vertices(g));
  boost::topological_sort(g, order.begin());
  std::reverse(order.begin(), order.end());

  for (auto desc : order) {
    deduceAndAddTrait(std::move(g[desc]));
  }
}

void Traits::addTraitDependencies() {
  for (TraitGraph::vertex_descriptor vIndex : mGraph.vertex_set()) {
    const TraitVertex &vPtr{mGraph[vIndex]};
    const auto deps{getDependencies(vPtr)};

    for (const auto &dep : deps) {
      // Switch statements (selected traits with trailing underscores) can
      // depend on all possible cases.
      if (dep.back() == '_') {
        for (auto uIndex : mGraph.vertex_set()) {
          const TraitVertex &uPtr{mGraph[uIndex]};
          if (!uPtr) continue;
          const bool match = std::visit([&dep](const auto &v) {
            //C++20: return v.getName().starts_with(dep);
            return boost::algorithm::starts_with(v.getName(), dep);
          }, uPtr->var);
          if (match) {
            boost::remove_edge(uIndex, vIndex, mGraph);
            boost::add_edge(uIndex, vIndex, mGraph);
          }
        }
      } else {
        const auto uIndexIt{mIndices.find(dep)};
        if (uIndexIt == mIndices.end()) {
          const std::string dependee{std::visit([](const auto &trait) {
            return trait.getName();
          }, vPtr->var)};
          gui::guiLogger()->error("Dependency {} of {} does not exist",
                                  dep, dependee);
          throw std::runtime_error("Nonexistent dependency");
        }
        const TraitGraph::vertex_descriptor uIndex{uIndexIt->second};
        boost::remove_edge(uIndex, vIndex, mGraph);
        boost::add_edge(uIndex, vIndex, mGraph);
      }
    }
  }
}

void Traits::update() {
  // Make sure we've got a topological order, then iterate over the graph in
  // that order and call update. The ordering guarantees that updates are
  // performed in the correct order wrt dependencies.
  sort();
  for (const auto &desc : mOrdering) {
    auto &vertex{mGraph[desc]};
    if (!vertex) {
      const std::string traitName{std::visit([](const auto &trait) {
        return trait.getName();
      }, vertex->var)};
      gui::guiLogger()->error("Trait {} exists but is null", traitName);
      throw std::runtime_error("nullptr vertex");
    }
    auto &cache{vertex->cache};
    if (cache) {
      std::visit([](auto &trait, auto &c) {
        if constexpr (std::is_same_v<decltype(trait.invoke()),
                                     std::decay_t<decltype(c)>>) {
          auto v{trait.invoke()};
          if (c != v) {
            c = v;
            trait.update();
          }
        }
      }, vertex->var, *cache);
    } else {
      std::visit([&cache](auto &&trait) {
        cache.emplace(trait.invoke());
        trait.update();
      }, vertex->var);
    }
  }
}

void Traits::loadStrings(pugi::xml_node doc) {
  mStrings = gui::StringsElement(doc);
}

namespace {

template<class Graph>
class VertexWriter {
 private:
  const Graph &mGraph;

 public:
  VertexWriter(const Graph &g) : mGraph(g) {}

  template<class VertexDesc>
  void operator()(std::ostream &os, VertexDesc desc) {
    auto vPtr{mGraph[desc]};
    if (!vPtr) return;

    std::visit([&os, desc](const auto &trait) {
      os << "[label=\"" << desc << ": " << trait.getName() << "\"]";
    }, vPtr->var);
  }
};

}

void Traits::printDot(std::ostream &os) {
  boost::write_graphviz(os, mGraph, VertexWriter(mGraph));
}

} // namespace gui
