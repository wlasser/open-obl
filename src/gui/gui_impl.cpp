#include "gui/elements/button.hpp"
#include "gui/elements/generic_background.hpp"
#include "gui/elements/image.hpp"
#include "gui/elements/rect.hpp"
#include "gui/elements/text.hpp"
#include "gui/elements/vertical_scroll.hpp"
#include "gui/gui_impl.hpp"
#include "gui/logging.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/convert.hpp>
#include <OgreOverlay.h>
#include <OgreOverlayManager.h>
#include <algorithm>
#include <map>

namespace gui {

MenuContext::Impl::Impl(std::unique_ptr<Traits> traits,
                        std::unique_ptr<MenuVariant> menu,
                        UiElementNodeList uiElements,
                        pugi::xml_document document)
    : mTraits(std::move(traits)),
      mMenu(std::move(menu)),
      mUiElements(std::move(uiElements)),
      mDocument(std::move(document)) {}

void MenuContext::Impl::update() {
  mTraits->update();
}

void MenuContext::Impl::clearEvents() {
  for (const auto &[uiElement, _] : mUiElements) {
    uiElement->clearEvents();
  }
}

Ogre::Overlay *MenuContext::Impl::getOverlay() const {
  return std::visit([](const auto &menu) { return menu.getOverlay(); }, *mMenu);
}

Ogre::Vector2
MenuContext::Impl::normalizeCoordinates(int32_t x, int32_t y) const {
  auto &overlayMgr{Ogre::OverlayManager::getSingleton()};
  const auto w{static_cast<float>(overlayMgr.getViewportWidth())};
  const auto h{static_cast<float>(overlayMgr.getViewportHeight())};
  return {static_cast<float>(x) / w, static_cast<float>(y) / h};
}

void MenuContext::Impl::set_user(int index, gui::UiElement::UserValue value) {
  gui::extractUiElement(*mMenu)->set_user(index, std::move(value));
}

gui::UiElement::UserValue MenuContext::Impl::get_user(int index) {
  return gui::extractUiElement(*mMenu)->get_user(index);
}

const gui::UiElement *MenuContext::Impl::getElementWithId(int id) const {
  if (id < 0) return nullptr;
  auto it{std::find_if(mUiElements.begin(), mUiElements.end(),
                       [id](const UiElementNode &pair) {
                         return pair.first->get_id() == id;
                       })};
  return it == mUiElements.end() ? nullptr : it->first.get();
}

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

std::string
getFullyQualifiedName(pugi::xml_node node,
                      const UiElementNodeList &uiElements) {
  // There are cases where two siblings have the same name, whereupon
  // fully-qualified names are insufficient for uniqueness. Non-uniqueness
  // could also occur on a more global scale, but unless this happens in an
  // original game file (not mods) we do not support it.
  auto name{gui::fullyQualifyName(node)};
  auto pred = [&name](const UiElementNode &elemNode) {
    return name == elemNode.first->get_name();
  };
  const auto begin{uiElements.begin()}, end{uiElements.end()};
  while (std::find_if(begin, end, pred) != end) {
    gui::guiLogger()->warn("Deprecated: Multiple siblings with the same "
                           "name (name: {}) (offset: {})",
                           name, node.offset_debug());
    name.push_back('_');
  }

  return name;
}

std::unique_ptr<UiElement>
makeUiElement(pugi::xml_node node, std::string name) {
  std::string shortName{node.attribute("name").value()};

  using std::literals::operator ""s;

  if (shortName == "load_background"s) {
    return std::make_unique<GenericBackground>(std::move(name));
  } else if (shortName == "rep_scroll_bar"s
      || shortName == "cont_scroll_bar"s
      || shortName == "class_list_scroll_bar"s
      || shortName == "item_listing_scroll_bar"s
      || shortName == "magic_scroll_bar"s
      || shortName == "inv_scroll_bar"s
      || shortName == "map_log_scroll_bar"s
      || shortName == "stat_p3_scroll_bar"s
      || shortName == "stat_p4_scroll_bar"s
      || shortName == "stat_p5_scroll_bar"s
      || shortName == "load_scroll_bar"s
      || shortName == "save_scroll_bar"s
      || shortName == "skills_list_scroll_bar"s
      || shortName == "race_scroll_bar"s
      || shortName == "sigil_known_effect_list_scroll_bar"s
      || shortName == "sigil_added_effect_list_scroll_bar"s
      || shortName == "effect_list_scroll_bar"s
      || shortName == "spell_buy_list_scroll_bar"s
      || shortName == "spell_known_effects_scroll_bar"s
      || shortName == "spell_added_effets_scroll_bar"s
      || shortName == "ench_known_effects_scroll_bar"s
      || shortName == "ench_added_effects_scroll_bar"s) {
    return std::make_unique<VerticalScroll>(std::move(name));
  } else if (shortName == "vertical_scroll_marker"s) {
    return std::make_unique<VerticalScrollMarker>(std::move(name));
  } else if (shortName == "load_return_button"s) {
    return std::make_unique<Button>(std::move(name));
  } else if (shortName == "save_FocusBox"s) {
    return nullptr;
  }

  if (node.name() == "image"s) {
    return std::make_unique<Image>(std::move(name));
  } else if (node.name() == "rect"s) {
    return std::make_unique<Rect>(std::move(name));
  } else if (node.name() == "text"s) {
    return std::make_unique<Text>(std::move(name));
  }

  return nullptr;
}

UiElementNodeList getChildElements(pugi::xml_node node) {
  UiElementNodeList uiElements;

  for (auto child : node.children()) {
    std::string name{gui::getFullyQualifiedName(child, uiElements)};
    if (auto element{gui::makeUiElement(child, std::move(name))}) {
      uiElements.emplace_back(std::move(element), child);
    }
  }

  return uiElements;
}

UiElementNodeList
addDescendants(Traits &traits, UiElement *uiElement, pugi::xml_node node) {
  gui::addTraits(traits, uiElement, node);

  auto *parentOverlay{uiElement->getOverlayElement()};
  auto *parentContainer{dynamic_cast<Ogre::OverlayContainer *>(parentOverlay)};

  auto children{gui::getChildElements(node)};
  uiElement->setChildCount(children.size());

  UiElementNodeList accum{};
  for (auto &[childElem, childNode] : children) {
    UiElement *childPtr{childElem.get()};
    if (parentContainer) {
      if (auto *childOverlayElem{childPtr->getOverlayElement()}) {
        parentContainer->addChild(childOverlayElem);
      }
    }

    auto descendants{gui::addDescendants(traits, childPtr, childNode)};
    accum.reserve(accum.size() + 1u + descendants.size());
    accum.emplace_back(std::move(childElem), childNode);
    accum.insert(accum.end(),
                 std::make_move_iterator(descendants.begin()),
                 std::make_move_iterator(descendants.end()));
  }

  return accum;
}

std::optional<MenuContext>
MenuContext::Impl::loadMenu(pugi::xml_document doc,
                            std::optional<pugi::xml_document> stringsDoc) {
  const auto[menuNode, menuType]{gui::getMenuNode(doc.root())};

  auto menu{std::make_unique<MenuVariant>()};
  enumvar::defaultConstruct(menuType, *menu);
  auto *menuElement{gui::extractUiElement(*menu)};

  if (std::string name{menuNode.attribute("name").value()}; !name.empty()) {
    menuElement->set_name(std::move(name));
  } else return std::nullopt;

  // Construct the dependency graph of the dynamic representation
  auto menuTraits{std::make_unique<Traits>()};
  if (stringsDoc) menuTraits->loadStrings(stringsDoc->root());
  auto uiElements{gui::addDescendants(*menuTraits, menuElement, menuNode)};
  menuTraits->addImplementationElementTraits();
  menuTraits->addProvidedTraits(menuElement);
  for (const auto &[uiElement, _] : uiElements) {
    menuTraits->addProvidedTraits(uiElement.get());
  }
  menuTraits->addQueuedCustomTraits();
  menuTraits->addTraitDependencies();
  menuTraits->update();

  // Link the output user traits (i.e. those set by the implementation) to the
  // parent element's user interface buffer.
  auto binnedTraits{menuTraits->binUserTraits()};
  menuElement->setOutputUserTraitSources(binnedTraits[menuElement->get_name()]);
  for (const auto &[uiElement, _] : uiElements) {
    uiElement->setOutputUserTraitSources(binnedTraits[uiElement->get_name()]);
  }

  return MenuContext(std::make_unique<Impl>(
      std::move(menuTraits),
      std::move(menu),
      std::move(uiElements),
      std::move(doc)
  ));
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