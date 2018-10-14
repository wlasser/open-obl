#include "enum_template.hpp"
#include "engine/gui/gui.hpp"
#include "engine/gui/xml.hpp"
#include "engine/settings.hpp"
#include <boost/algorithm/string/predicate.hpp>
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

MenuInterfaceVariant makeInterfaceBuffer(const MenuVariant &menuVar) {
  return makeInterfaceBufferImpl(menuVar);
}

pugi::xml_document loadDocument(std::istream &is) {
  auto logger = spdlog::get(settings::log);

  pugi::xml_document doc{};
  pugi::xml_parse_result result = doc.load(is);
  if (!result) {
    logger->error("Failed to parse menu XML [offset {}]: {}",
                  result.offset, result.description());
    throw std::runtime_error("Failed to parse menu XML");
  }
  return doc;
}

std::pair<pugi::xml_node, MenuType> getMenuNode(const pugi::xml_document &doc) {
  const auto menuNode{doc.child("menu")};
  if (!menuNode) {
    throw std::runtime_error("Menu does not have a <menu> tag");
  }

  const auto classNode{menuNode.child("class")};
  if (!classNode) {
    throw std::runtime_error("<menu> must have a <class> child tag");
  }

  const auto menuType = xml::getChildValue<MenuType>(classNode);
  return {menuNode, menuType};
}

std::string getMenuName(pugi::xml_node menuNode) {
  const auto attrib{menuNode.attribute("name")};
  if (!attrib) {
    throw std::runtime_error("<menu> tag does not have a 'name' attribute");
  }
  return attrib.value();
}

void addChildren(Traits &traits, pugi::xml_node parentNode,
                 UiElement *parentElement) {
  for (const auto &node : parentNode.children()) {
    traits.addAndBindImplementationTrait(node, parentElement);
    traits.addAndBindUserTrait(node, parentElement);
    // TODO: Look for an add other UiElements, e.g. rect, image
  }
}

// Parse an entire menu from an XML stream
void parseMenu(std::istream &is) {
  auto doc{loadDocument(is)};
  const auto[menuNode, menuType]{getMenuNode(doc)};

  // Construct a Menu<menuType> (menuType, not MenuType!) then extract a pointer
  // to base so we can do virtual dispatch.
  MenuVariant menu{};
  enumvar::defaultConstruct(menuType, menu);
  auto *menuElement{extractUiElement(menu)};

  menuElement->set_name(getMenuName(menuNode));

  // Now construct the dependency graph of the dynamic representation
  Traits menuTraits{};
  addChildren(menuTraits, menuNode, menuElement);
  menuTraits.addImplementationElementTraits();
  menuTraits.addTraitDependencies();
  menuTraits.update();

  // Construct a suitable user interface buffer and link it to the user traits
  MenuInterfaceVariant interfaceBuffer{makeInterfaceBuffer(menu)};

  std::visit([&menuTraits](auto &&t) {
    menuTraits.setUserTraitSources(t.value);
  }, interfaceBuffer);
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