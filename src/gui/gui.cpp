#include "enum_template.hpp"
#include "gui/elements/image.hpp"
#include "gui/elements/rect.hpp"
#include "gui/elements/text.hpp"
#include "gui/gui.hpp"
#include "gui/xml.hpp"
#include "settings.hpp"
#include <absl/container/flat_hash_map.h>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/graph/topological_sort.hpp>
#include <pugixml.hpp>
#include <spdlog/spdlog.h>

namespace gui {

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
  auto logger = spdlog::get(oo::LOG);

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

  const auto menuType = getXmlChildValue<MenuType>(classNode);
  return {menuNode, menuType};
}

std::string getMenuElementName(pugi::xml_node menuNode) {
  const auto attrib{menuNode.attribute("name")};
  if (!attrib) {
    throw std::runtime_error("Tag does not have a 'name' attribute");
  }
  return attrib.value();
}

std::vector<std::unique_ptr<UiElement>>
addChildren(Traits &traits,
            pugi::xml_node parentNode,
            UiElement *parentElement) {
  using namespace std::literals;
  std::vector<std::unique_ptr<UiElement>> uiElements;

  for (const auto &node : parentNode.children()) {
    traits.addAndBindImplementationTrait(node, parentElement);
    traits.addAndBindUserTrait(node, parentElement);

    std::unique_ptr<UiElement> element = [&]() -> std::unique_ptr<UiElement> {
      if (node.name() == "image"s) return std::make_unique<Image>();
      else if (node.name() == "rect"s) return std::make_unique<Rect>();
      else if (node.name() == "text"s) return std::make_unique<Text>();
      else return nullptr;
    }();

    if (element) {
      element->set_name(fullyQualifyName(node));
      auto children{addChildren(traits, node, element.get())};
      uiElements.reserve(uiElements.size() + 1u + children.size());
      uiElements.push_back(std::move(element));
      uiElements.insert(uiElements.end(),
                        std::move_iterator(children.begin()),
                        std::move_iterator(children.end()));
    }
  }

  return uiElements;
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

  const std::string menuName{getMenuElementName(menuNode)};
  menuElement->set_name(menuName);

  // Now construct the dependency graph of the dynamic representation
  Traits menuTraits{};
  auto uiElements = addChildren(menuTraits, menuNode, menuElement);
  menuTraits.addImplementationElementTraits();
  for (const auto &uiElement : uiElements) {
    menuTraits.addProvidedTraits(uiElement.get());
  }
  menuTraits.addTraitDependencies();
  menuTraits.update();

  // Construct a suitable user interface buffer and link it to the user traits
  MenuInterfaceVariant interfaceBuffer{makeInterfaceBuffer(menu)};

  std::visit([&menuTraits](auto &&t) {
    menuTraits.setUserTraitSources(t.value);
  }, interfaceBuffer);
}

template<>
MenuType parseXmlEntity(const std::string &entity) {
  const static absl::flat_hash_map<std::string, MenuType> map{
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
MenuType getXmlValue(const pugi::xml_node &node) {
  return parseXmlEntity<MenuType>(getXmlValue<std::string>(node));
}

template<>
MenuType getXmlChildValue(const pugi::xml_node &node, const char *name) {
  return parseXmlEntity<MenuType>(getXmlChildValue<std::string>(node, name));
}

template<>
MenuType getXmlChildValue(const pugi::xml_node &node) {
  return parseXmlEntity<MenuType>(getXmlChildValue<std::string>(node));
}

} // namespace gui