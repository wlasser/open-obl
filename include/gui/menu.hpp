#ifndef OPENOBL_GUI_MENU_HPP
#define OPENOBL_GUI_MENU_HPP

#include "gui/ui_element.hpp"
#include <OgreOverlay.h>

namespace gui {

/// Each menu must be one of the following types, given in the XML by its
/// `<class>`
enum class MenuType {
  AlchemyMenu,
  AudioMenu,
  BookMenu,
  BreathMenu,
  ClassMenu,
  ContainerMenu,
  ControlsMenu,
  CreditsMenu,
  DialogMenu,
  EffectSettingMenu,
  EnchantmentMenu,
  GameplayMenu,
  GenericMenu,
  HUDInfoMenu,
  HUDMainMenu,
  HUDSubtitleMenu,
  InventoryMenu,
  LevelUpMenu,
  LoadingMenu,
  LoadMenu,
  LockPickMenu,
  MagicMenu,
  MagicPopupMenu,
  MainMenu,
  MapMenu,
  MessageMenu,
  NegotiateMenu,
  OptionsMenu,
  PauseMenu,
  PersuasionMenu,
  QuantityMenu,
  QuickKeysMenu,
  RaceSexMenu,
  RechargeMenu,
  RepairMenu,
  SaveMenu,
  SigilStoneMenu,
  SkillsMenu,
  SleepWaitMenu,
  SpellMakingMenu,
  SpellPurchaseMenu,
  StatsMenu,
  TextEditMenu,
  TrainingMenu,
  VideoMenu,
  N
};

template<class T>
struct UserInterfaceWrapper {
  using type = T;
  T value{};
};

/// The idea here is that we have a shallow inheritance hierarchy parameterised
/// by the MenuType enum. Specialising this class template and overriding its
/// corresponding methods allows us to do virtual dispatch based on a runtime
/// enum value, without manually checking for each value.
template<MenuType Type>
class Menu : public UiElement {
 public:
  /// Return a tuple of pointers to member variables that represent output user
  /// traits, namely those that are set by the implementation. The position in
  /// the tuple should correspond to the index of the user trait; user traits
  /// that are not output traits should have their entry set to `nullptr`.
  auto getUserOutputTraitInterface() {
    return std::tuple<>{};
  }

  Ogre::Overlay *getOverlay() const {
    return nullptr;
  }
};

} // namespace gui

#endif // OPENOBL_GUI_MENU_HPP
