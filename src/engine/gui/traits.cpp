#include "engine/gui/gui.hpp"
#include "engine/gui/trait.hpp"
#include "engine/gui/traits.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/graph/topological_sort.hpp>

namespace engine::gui {

void Traits::sort() {
  if (mSorted) return;
  mOrdering.clear();
  // By our definitions, there is an edge uv iff v depends on u, and so in the
  // ordering u should come before v iff there is an edge uv.
  // boost::topological_sort says the reverse; that v comes before u iff there
  // is an edge uv. Luckily for us, boost also gives the result in the reverse
  // order (to them), which is the right order for us.
  mOrdering.reserve(boost::num_vertices(mGraph));
  boost::topological_sort(mGraph, std::back_inserter(mOrdering));
  mSorted = true;
}

bool Traits::addAndBindImplementationTrait(const pugi::xml_node &node,
                                           engine::gui::UiElement *uiElement) {
  using namespace std::literals;
  if (node.name() == "x"s) {
    addAndBindTrait<int>(uiElement, &UiElement::set_x, node);
  } else if (node.name() == "y"s) {
    addAndBindTrait<int>(uiElement, &UiElement::set_y, node);
  } else if (node.name() == "width"s) {
    addAndBindTrait<int>(uiElement, &UiElement::set_width, node);
  } else if (node.name() == "height"s) {
    addAndBindTrait<int>(uiElement, &UiElement::set_height, node);
  } else if (node.name() == "alpha"s) {
    addAndBindTrait<int>(uiElement, &UiElement::set_alpha, node);
  } else if (node.name() == "locus"s) {
    addAndBindTrait<bool>(uiElement, &UiElement::set_locus, node);
  } else if (node.name() == "visible"s) {
    addAndBindTrait<bool>(uiElement, &UiElement::set_visible, node);
  } else if (node.name() == "menufade"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_menufade, node);
  } else if (node.name() == "explorefade"s) {
    addAndBindTrait<float>(uiElement, &UiElement::set_explorefade, node);
  } else {
    return false;
  }
  return true;
}

bool Traits::addAndBindUserTrait(const pugi::xml_node &node,
                                 UiElement *uiElement) {
  const std::optional<int> indexOpt{getUserTraitIndex(node.name())};
  if (!indexOpt) return false;
  const int index{*indexOpt};

  auto setter = [index](UiElement *uiElement, auto &&value) {
    uiElement->set_user(index, value);
  };

  switch (uiElement->userTraitType(index)) {
    case TraitTypeId::Int: {
      addAndBindTrait<int>(uiElement, setter, node);
      break;
    }
    case TraitTypeId::Float: {
      addAndBindTrait<float>(uiElement, setter, node);
      break;
    }
    case TraitTypeId::Bool: {
      addAndBindTrait<bool>(uiElement, setter, node);
      break;
    }
    case TraitTypeId::String: {
      addAndBindTrait<std::string>(uiElement, setter, node);
      break;
    }
    case TraitTypeId::Unimplemented: {
      return false;
    }
  }

  return true;
}

std::vector<std::string>
Traits::getDependencies(const TraitVertex &vertex) const {
  if (vertex) {
    return std::visit([](const auto &v) {
      return v.getDependencies();
    }, *vertex);
  } else {
    return {};
  }
}

void Traits::addImplementationElementTraits() {
  for (TraitGraph::vertex_descriptor vIndex : mGraph.vertex_set()) {
    const TraitVertex &vPtr = mGraph[vIndex];
    const auto deps = getDependencies(vPtr);

    for (const auto &dep : deps) {
      if (boost::algorithm::starts_with(dep, "__")) {
        if (mIndices.find(dep) != mIndices.end()) continue;

        if (dep == "__screen.width") {
          addTrait(mScreen.makeWidthTrait());
        } else if (dep == "__screen.height") {
          addTrait(mScreen.makeHeightTrait());
        } else if (dep == "__screen.cropX") {
          addTrait(mScreen.makeCropXTrait());
        } else if (dep == "__screen.cropY") {
          addTrait(mScreen.makeCropYTrait());
        } else if (boost::algorithm::starts_with(dep, "__strings.")) {
          addTrait(mStrings.makeTrait(dep));
        }
      }
    }
  }
}

void Traits::addTraitDependencies() {
  for (TraitGraph::vertex_descriptor vIndex : mGraph.vertex_set()) {
    const TraitVertex &vPtr = mGraph[vIndex];
    const auto deps = getDependencies(vPtr);

    for (const auto &dep : deps) {
      auto uIndexIt = mIndices.find(dep);
      if (uIndexIt == mIndices.end()) {
        throw std::runtime_error("Nonexistent dependency");
      }
      TraitGraph::vertex_descriptor uIndex = uIndexIt->second;
      boost::remove_edge(uIndex, vIndex, mGraph);
      boost::add_edge(uIndex, vIndex, mGraph);
    }
  }
}

void Traits::update() {
  // Make sure we've got a topological order, then iterate over the graph in
  // that order and call update. The ordering guarantees that updates are
  // performed in the correct order wrt dependencies.
  sort();
  for (const auto &desc : mOrdering) {
    auto &vertex = mGraph[desc];
    if (!vertex) {
      throw std::runtime_error("nullptr vertex");
    }
    std::visit([](auto &&trait) { trait.update(); }, *vertex);
  }
}

} // namespace engine::gui
