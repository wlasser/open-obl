#ifndef OPENOBLIVION_GUI_MENU_HPP
#define OPENOBLIVION_GUI_MENU_HPP

#include "gui/ui_element.hpp"

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
  struct UserInterface {
    using type = std::tuple<>;
    type value{};
  };
};

} // namespace gui

#endif // OPENOBLIVION_GUI_MENU_HPP
