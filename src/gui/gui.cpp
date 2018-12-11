#include "enum_template.hpp"
#include "gui/elements/image.hpp"
#include "gui/elements/rect.hpp"
#include "gui/elements/text.hpp"
#include "gui/gui.hpp"
#include "gui/xml.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/graph/topological_sort.hpp>
#include <pugixml.hpp>
#include <unordered_map>

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

std::pair<pugi::xml_node, MenuType> getMenuNode(pugi::xml_node doc) {
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

void addTraits(Traits &traits, UiElement *uiElement, pugi::xml_node node) {
  for (auto n : node.children()) {
    traits.addAndBindImplementationTrait(n, uiElement);
    traits.addAndBindUserTrait(n, uiElement);
    // TODO: addAndBindCustomTrait
  }
}

std::vector<UiElementNode> getChildElements(pugi::xml_node node) {
  std::vector<UiElementNode> uiElements;

  for (auto n : node.children()) {
    using namespace std::literals;
    std::unique_ptr<UiElement> element = [&]() -> std::unique_ptr<UiElement> {
      if (n.name() == "image"s) {
        return std::make_unique<Image>(gui::fullyQualifyName(n));
      } else if (n.name() == "rect"s) {
        return std::make_unique<Rect>(gui::fullyQualifyName(n));
      } else if (n.name() == "text"s) {
        return std::make_unique<Text>(gui::fullyQualifyName(n));
      }
      else return nullptr;
    }();

    if (element) {
      uiElements.emplace_back(std::move(element), n);
    }
  }

  return uiElements;
}

std::vector<std::unique_ptr<UiElement>>
addDescendants(Traits &traits, UiElement *uiElement, pugi::xml_node node) {
  gui::addTraits(traits, uiElement, node);
  std::vector<std::unique_ptr<UiElement>> accum{};

  for (auto &child : gui::getChildElements(node)) {
    auto descendants{addDescendants(traits, child.first.get(), child.second)};
    accum.reserve(accum.size() + 1u + descendants.size());
    accum.emplace_back(std::move(child.first));
    accum.insert(accum.end(),
                 std::make_move_iterator(descendants.begin()),
                 std::make_move_iterator(descendants.end()));
  }

  return accum;
}

void loadMenu(pugi::xml_node doc) {
  const auto[menuNode, menuType]{gui::getMenuNode(doc)};

  MenuVariant menu{};
  enumvar::defaultConstruct(menuType, menu);
  auto *menuElement{gui::extractUiElement(menu)};

  const std::string menuName{menuNode.attribute("name").value()};
  if (menuName.empty()) {
    // TODO: Return an empty optional
  }
  menuElement->set_name(menuName);

  // Construct the dependency graph of the dynamic representation
  Traits menuTraits{};
  menuTraits.loadStrings("menus/strings.xml");
  auto uiElements = gui::addDescendants(menuTraits, menuElement, menuNode);
  menuTraits.addImplementationElementTraits();
  for (const auto &uiElement : uiElements) {
    menuTraits.addProvidedTraits(uiElement.get());
  }
  menuTraits.addTraitDependencies();
  menuTraits.update();

  // Construct a suitable user interface buffer and link it to the user traits
  MenuInterfaceVariant interfaceBuffer{gui::makeInterfaceBuffer(menu)};
  std::visit([&menuTraits](auto &&t) {
    menuTraits.setUserTraitSources(t.value);
  }, interfaceBuffer);
}

void loadMenu(const std::string &filename) {
  auto doc{gui::loadDocument(filename)};
  loadMenu(doc.document_element());
}

template<>
MenuType parseXmlEntity(const std::string &entity) {
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

template<> MenuType getXmlValue(pugi::xml_node node) {
  return parseXmlEntity<MenuType>(getXmlValue<std::string>(node));
}

template<> MenuType getXmlChildValue(pugi::xml_node node, const char *name) {
  return parseXmlEntity<MenuType>(getXmlChildValue<std::string>(node, name));
}

template<> MenuType getXmlChildValue(pugi::xml_node node) {
  return parseXmlEntity<MenuType>(getXmlChildValue<std::string>(node));
}

} // namespace gui