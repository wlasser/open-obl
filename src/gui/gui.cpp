#include "enum_template.hpp"
#include "gui/elements/image.hpp"
#include "gui/elements/rect.hpp"
#include "gui/elements/text.hpp"
#include "gui/gui.hpp"
#include "gui/logging.hpp"
#include "gui/xml.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/convert.hpp>
#include <boost/graph/topological_sort.hpp>
#include <OgreOverlay.h>
#include <pugixml.hpp>
#include <algorithm>
#include <functional>
#include <map>

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

std::pair<pugi::xml_node, MenuType> getMenuNode(pugi::xml_node doc) {
  const auto menuNode{doc.child("menu")};
  if (!menuNode) {
    gui::guiLogger()->error("Menu does not have a <menu> tag");
    throw std::runtime_error("Menu does not have a <menu> tag");
  }

  const auto classNode{menuNode.child("class")};
  if (!classNode) {
    gui::guiLogger()->error("<menu> does not have a <class> child tag "
                            "(offset: {})", menuNode.offset_debug());
    throw std::runtime_error("<menu> does not have a <class> child tag");
  }

  const auto menuType = getXmlChildValue<MenuType>(classNode);
  return {menuNode, menuType};
}

void addTraits(Traits &traits, UiElement *uiElement, pugi::xml_node node) {
  for (auto n : node.children()) {
    if (traits.addAndBindImplementationTrait(n, uiElement)) continue;
    if (traits.addAndBindUserTrait(n, uiElement)) continue;
    traits.queueCustomTrait(n, uiElement);
  }
}

std::vector<UiElementNode> getChildElements(pugi::xml_node node) {
  std::vector<UiElementNode> uiElements;

  for (auto n : node.children()) {
    using namespace std::literals;
    std::unique_ptr<UiElement> element = [&]() -> std::unique_ptr<UiElement> {
      // There are cases where two siblings have the same name, whereupon
      // fully-qualifed names are insufficient for uniqueness. Non-uniqueness
      // could also occur on a more global scale, but unless this happens in an
      // original game file (not mods) we do not support it.
      auto name{gui::fullyQualifyName(n)};
      const auto pred = [&name](UiElementNode &elemNode) {
        return name == elemNode.first->get_name();
      };
      while (std::find_if(uiElements.begin(), uiElements.end(), pred)
          != uiElements.end()) {
        gui::guiLogger()->warn("Deprecated: Multiple siblings with the same "
                               "name (name: {}) (offset: {})",
                               name, n.offset_debug());
        name.push_back('_');
      }

      if (n.name() == "image"s) {
        return std::make_unique<Image>(name);
      } else if (n.name() == "rect"s) {
        return std::make_unique<Rect>(name);
      } else if (n.name() == "text"s) {
        return std::make_unique<Text>(name);
      } else return nullptr;
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

  auto *parentOverlay{uiElement->getOverlayElement()};
  auto *parentContainer{dynamic_cast<Ogre::OverlayContainer *>(parentOverlay)};

  for (auto &child : gui::getChildElements(node)) {
    UiElement *childPtr{child.first.get()};
    if (parentContainer) {
      if (auto *childOverlay{childPtr->getOverlayElement()}) {
        parentContainer->addChild(childOverlay);
      }
    }

    auto descendants{addDescendants(traits, childPtr, child.second)};
    accum.reserve(accum.size() + 1u + descendants.size());
    accum.emplace_back(std::move(child.first));
    accum.insert(accum.end(),
                 std::make_move_iterator(descendants.begin()),
                 std::make_move_iterator(descendants.end()));
  }

  return accum;
}

std::optional<MenuContext> loadMenu(pugi::xml_node doc,
                                    std::optional<pugi::xml_node> stringsDoc) {
  const auto[menuNode, menuType]{gui::getMenuNode(doc)};

  auto menu{std::make_unique<MenuVariant>()};
  enumvar::defaultConstruct(menuType, *menu);
  auto *menuElement{gui::extractUiElement(*menu)};

  const std::string menuName{menuNode.attribute("name").value()};
  if (menuName.empty()) {
    return std::nullopt;
  }
  menuElement->set_name(menuName);

  // Construct the dependency graph of the dynamic representation
  auto menuTraits{std::make_unique<Traits>()};
  if (stringsDoc) menuTraits->loadStrings(*stringsDoc);
  auto uiElements = gui::addDescendants(*menuTraits, menuElement, menuNode);
  menuTraits->addImplementationElementTraits();
  for (const auto &uiElement : uiElements) {
    menuTraits->addProvidedTraits(uiElement.get());
  }
  menuTraits->addQueuedCustomTraits();
  menuTraits->addTraitDependencies();
  menuTraits->update();

  // Link the output user traits (i.e. those set by the implementation) to the
  // menu's user interface buffer.
  std::visit([&menuTraits](auto &m) {
    menuTraits->setOutputUserTraitSources(m.getUserOutputTraitInterface());
  }, *menu);

  return std::optional<MenuContext>(std::in_place,
                                    std::move(menuTraits),
                                    std::move(menu),
                                    std::move(uiElements));
}

std::optional<MenuContext> loadMenu(const std::string &filename,
                                    const std::string &stringsFilename) {
  auto doc{gui::readXmlDocument(filename)};
  auto stringsDoc{gui::readXmlDocument(stringsFilename)};
  doc.save_file("out.xml", "  ");
  return loadMenu(doc, stringsDoc);
}

MenuContext::MenuContext(std::unique_ptr<Traits> traits,
                         std::unique_ptr<MenuVariant> menu,
                         std::vector<MenuContext::UiElementPtr> uiElements)
    : mTraits(std::move(traits)),
      mMenu(std::move(menu)),
      mUiElements(std::move(uiElements)) {}

void MenuContext::update() {
  mTraits->update();
}

void MenuContext::set_user(int index, gui::UiElement::UserValue value) {
  gui::extractUiElement(*mMenu)->set_user(index, std::move(value));
}

template<>
void XmlEntityConverter::operator()(std::string_view entity,
                                    boost::optional<MenuType> &out) const {
  const static std::map<std::string, MenuType, std::less<>> map{
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
  if (auto it = map.find(entity); it != map.end()) out = it->second;
  else out = boost::none;
}

template<> MenuType getXmlValue(pugi::xml_node node) {
  return boost::convert<MenuType>(getXmlValue<std::string>(node),
                                  XmlEntityConverter{}).value();
}

template<> MenuType getXmlChildValue(pugi::xml_node node, const char *name) {
  return boost::convert<MenuType>(getXmlChildValue<std::string>(node, name),
                                  XmlEntityConverter{}).value();
}

template<> MenuType getXmlChildValue(pugi::xml_node node) {
  return boost::convert<MenuType>(getXmlChildValue<std::string>(node),
                                  XmlEntityConverter{}).value();
}

} // namespace gui