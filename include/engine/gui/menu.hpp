#ifndef OPENOBLIVION_ENGINE_GUI_MENU_HPP
#define OPENOBLIVION_ENGINE_GUI_MENU_HPP

#include "engine/gui/ui_element.hpp"

namespace engine::gui {

// Each menu must be one of the following types, given in the XML by its <class>
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

// The idea here is that we have a shallow inheritance hierarchy parameterised
// by the MenuType enum. Specialising this class template and overriding its
// corresponding methods allows us to do virtual dispatch based on a runtime
// enum value, without manually checking for each value.
template<MenuType Type>
class Menu : public UiElement {};

} // namespace engine::gui

#endif // OPENOBLIVION_ENGINE_GUI_MENU_HPP
