#include "enum_template.hpp"
#include "engine/gui/gui.hpp"
#include "engine/settings.hpp"
#include <boost/algorithm/string/trim.hpp>
#include <cstdlib>
#include <pugixml.hpp>
#include <spdlog/spdlog.h>
#include <unordered_map>

namespace engine::gui::xml {

template<>
bool parseEntity(const std::string &entity) {
  return entity == "&true;";
}

template<>
MenuType parseEntity(const std::string &entity) {
  const static std::unordered_map<std::string, MenuType> map{
      {"&AlchemyMenu;", MenuType::AlchemyMenu},
      {"&AudioMenu;", MenuType::AudioMenu},
      {"&ClassMenu;", MenuType::ClassMenu},
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
int getValue(const pugi::xml_node &node) {
  // stoi discards whitespace so we don't need to trim.
  // 0 means the base is autodetected.
  // There is a string construction here but otherwise we need strtol which
  // complains when sizeof(long) > sizeof(int).
  return std::stoi(node.value(), nullptr, 0);
}

template<>
int getChildValue(const pugi::xml_node &node, const char *name) {
  return std::stoi(node.child_value(name), nullptr, 0);
}

template<>
int getChildValue(const pugi::xml_node &node) {
  return std::stoi(node.child_value(), nullptr, 0);
}

template<>
float getValue(const pugi::xml_node &node) {
  // No string construction necessary, unlike with getValue<int>
  return std::strtof(node.value(), nullptr);
}

template<>
float getChildValue(const pugi::xml_node &node, const char *name) {
  return std::strtof(node.child_value(name), nullptr);
}

template<>
float getChildValue(const pugi::xml_node &node) {
  return std::strtof(node.child_value(), nullptr);
}

template<>
bool getValue(const pugi::xml_node &node) {
  return parseEntity<bool>(getValue<std::string>(node));
}

template<>
bool getChildValue(const pugi::xml_node &node, const char *name) {
  return parseEntity<bool>(getChildValue<std::string>(node, name));
}

template<>
bool getChildValue(const pugi::xml_node &node) {
  return parseEntity<bool>(getChildValue<std::string>(node));
}

template<>
std::string getValue(const pugi::xml_node &node) {
  std::string value{node.value()};
  boost::algorithm::trim(value);
  return value;
}

template<>
std::string getChildValue(const pugi::xml_node &node, const char *name) {
  std::string value{node.child_value(name)};
  boost::algorithm::trim(value);
  return value;
}

template<>
std::string getChildValue(const pugi::xml_node &node) {
  std::string value{node.child_value()};
  boost::algorithm::trim(value);
  return value;
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
  auto menuType = getChildValue<MenuType>(classNode);
  MenuVariant menu{};
  enumvar::defaultConstruct(menuType, menu);

  // TODO: Use std::visit to delegate to a concrete representative creator

  // Now we construct the dependency graph of the dynamic representation
  for (const auto &node : menuNode.children()) {
    if (node.name() == "x"s) {

    }
  }
}

} // namespace engine::gui::xml