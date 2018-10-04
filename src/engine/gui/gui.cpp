#include "enum_template.hpp"
#include "engine/gui/gui.hpp"
#include "engine/gui/xml.hpp"
#include "engine/settings.hpp"
#include <boost/graph/topological_sort.hpp>
#include <pugixml.hpp>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace engine::gui {

UiElement *extractUiElement(MenuVariant &menu) {
  return std::visit([](auto &&arg) -> UiElement * {
    return static_cast<UiElement *>(&arg);
  }, menu);
}

const UiElement *extractUiElement(const MenuVariant &menu) {
  return std::visit([](auto &&arg) -> const UiElement * {
    return static_cast<const UiElement *>(&arg);
  }, menu);
}

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
    addTraitAndBind<int>(uiElement, &UiElement::set_x, node);
  } else if (node.name() == "y"s) {
    addTraitAndBind<int>(uiElement, &UiElement::set_y, node);
  } else if (node.name() == "width"s) {
    addTraitAndBind<int>(uiElement, &UiElement::set_width, node);
  } else if (node.name() == "height"s) {
    addTraitAndBind<int>(uiElement, &UiElement::set_height, node);
  } else if (node.name() == "alpha"s) {
    addTraitAndBind<int>(uiElement, &UiElement::set_alpha, node);
  } else if (node.name() == "locus"s) {
    addTraitAndBind<bool>(uiElement, &UiElement::set_locus, node);
  } else if (node.name() == "visible"s) {
    addTraitAndBind<bool>(uiElement, &UiElement::set_visible, node);
  } else if (node.name() == "menufade"s) {
    addTraitAndBind<float>(uiElement, &UiElement::set_menufade, node);
  } else if (node.name() == "explorefade"s) {
    addTraitAndBind<float>(uiElement, &UiElement::set_explorefade, node);
  } else {
    return false;
  }
  return true;
}

void Traits::addTraitDependencies() {
  for (TraitGraph::vertex_descriptor vIndex : mGraph.vertex_set()) {
    const TraitVertex &vPtr = mGraph[vIndex];
    if (!vPtr) {
      throw std::runtime_error("nullptr vertex");
    }
    const auto &deps = std::visit([](const auto &v) {
      return v.getDependencies();
    }, *vPtr);

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

// Parse an entire menu from an XML stream
void parseMenu(std::istream &is) {
  using namespace std::literals;

  auto logger = spdlog::get(settings::log);

  // Load the document. If this fails, we're done.
  pugi::xml_document doc{};
  pugi::xml_parse_result result = doc.load(is);
  if (!result) {
    logger->error("Failed to parse menu XML [{}]: {}",
                  result.offset,
                  result.description());
    return;
  }

  // All menus should start with a <menu> tag
  // TODO: Allow multiple menus in one XML file?
  auto menuNode = doc.first_child();
  if (!menuNode || menuNode.name() != "menu"s) {
    logger->error("XML does not start with a <menu> tag");
    return;
  }
  // Tag should have a name attribute uniquely identifying the menu
  if (!menuNode.attribute("name")) {
    logger->error("<menu> tag has no 'name' attribute");
    return;
  }
  std::string menuName = menuNode.attribute("name").value();

  // All menus must have a child <class> tag whose value determines which
  // MenuType it is.
  auto classNode = menuNode.child("class");
  if (!classNode) {
    logger->error("Menu must have a <class> tag");
    return;
  }
  auto menuType = xml::getChildValue<MenuType>(classNode);
  // Construct a Menu<menuType> (menuType, not MenuType!)
  MenuVariant menu{};
  enumvar::defaultConstruct(menuType, menu);
  // Extract a pointer to base of Menu<menuType> so we can do virtual dispatch.
  // One could do everything with std::visit instead if they wanted
  auto *uiElement = extractUiElement(menu);

  // Set the menu name
  uiElement->set_name(menuName);

  // TODO: Use std::visit to delegate to a concrete representative creator

  // Now we construct the dependency graph of the dynamic representation
  Traits menuTraits{};
  for (const auto &node : menuNode.children()) {
    menuTraits.addAndBindImplementationTrait(node, uiElement);
  }

  // Add the dependency graph edges
  menuTraits.addTraitDependencies();

  // Force an update to initialize everything
  menuTraits.update();
}

namespace xml {

template<>
MenuType parseEntity(const std::string &entity) {
  const static std::unordered_map<std::string, MenuType> map{
      {"&AlchemyMenu;", MenuType::AlchemyMenu},
      {"&AudioMenu;", MenuType::AudioMenu},
      {"&BookMenu;", MenuType::BookMenu},
      {"&BreathMenu;", MenuType::BreathMenu},
      {"&ClassMenu;", MenuType::ClassMenu},
      {"&ContainerMenu;", MenuType::ContainerMenu},
      {"&ControlsMenu;", MenuType::ControlsMenu},
      {"&CreditsMenu;", MenuType::CreditsMenu},
      {"&DialogMenu;", MenuType::DialogMenu},
      {"&EffectSettingMenu;", MenuType::EffectSettingMenu},
      {"&EnchantmentMenu;", MenuType::EnchantmentMenu},
      {"&GameplayMenu;", MenuType::GameplayMenu},
      {"&GenericMenu;", MenuType::GenericMenu},
      {"&HUDInfoMenu;", MenuType::HUDInfoMenu},
      {"&HUDMainMenu;", MenuType::HUDMainMenu},
      {"&HUDSubtitleMenu;", MenuType::HUDSubtitleMenu},
      {"&InventoryMenu;", MenuType::InventoryMenu},
      {"&LevelUpMenu;", MenuType::LevelUpMenu},
      {"&LoadingMenu;", MenuType::LoadingMenu},
      {"&LoadMenu;", MenuType::LoadMenu},
      {"&LockPickMenu;", MenuType::LockPickMenu},
      {"&MagicMenu;", MenuType::MagicMenu},
      {"&MagicPopupMenu;", MenuType::MagicPopupMenu},
      {"&MainMenu;", MenuType::MainMenu},
      {"&MapMenu;", MenuType::MapMenu},
      {"&MessageMenu;", MenuType::MessageMenu},
      {"&NegotiateMenu;", MenuType::NegotiateMenu},
      {"&OptionsMenu;", MenuType::OptionsMenu},
      {"&PauseMenu;", MenuType::PauseMenu},
      {"&PersuasionMenu;", MenuType::PersuasionMenu},
      {"&QuantityMenu;", MenuType::QuantityMenu},
      {"&QuickKeysMenu;", MenuType::QuickKeysMenu},
      {"&RaceSexMenu;", MenuType::RaceSexMenu},
      {"&RechargeMenu;", MenuType::RechargeMenu},
      {"&RepairMenu;", MenuType::RepairMenu},
      {"&SaveMenu;", MenuType::SaveMenu},
      {"&SigilStoneMenu;", MenuType::SigilStoneMenu},
      {"&SkillsMenu;", MenuType::SkillsMenu},
      {"&SleepWaitMenu;", MenuType::SleepWaitMenu},
      {"&SpellMakingMenu;", MenuType::SpellMakingMenu},
      {"&SpellPurchaseMenu;", MenuType::SpellPurchaseMenu},
      {"&StatsMenu;", MenuType::StatsMenu},
      {"&TextEditMenu;", MenuType::TextEditMenu},
      {"&TrainingMenu;", MenuType::TrainingMenu},
      {"&VideoMenu;", MenuType::VideoMenu},
  };
  if (auto it = map.find(entity); it != map.end()) return it->second;
  else throw std::runtime_error("Invalid entity");
}

template<>
MenuType getValue(const pugi::xml_node &node) {
  return parseEntity<MenuType>(getValue<std::string>(node));
}

template<>
MenuType getChildValue(const pugi::xml_node &node, const char *name) {
  return parseEntity<MenuType>(getChildValue<std::string>(node, name));
}

template<>
MenuType getChildValue(const pugi::xml_node &node) {
  return parseEntity<MenuType>(getChildValue<std::string>(node));
}

} // namespace xml

} // namespace engine::gui