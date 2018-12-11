#include "modes/menu_mode.hpp"

MenuMode::MenuMode(ApplicationContext &ctx, gui::MenuType type) {
  switch (type) {
    case gui::MenuType::AlchemyMenu: {
      gui::loadMenu("menus/dialog/alchemy.xml");
      break;
    }
    case gui::MenuType::AudioMenu: {
      gui::loadMenu("menus/options/audio_menu.xml");
      break;
    }
    case gui::MenuType::BookMenu: {
      gui::loadMenu("menus/book_menu.xml");
      break;
    }
    case gui::MenuType::BreathMenu: {
      gui::loadMenu("menus/breath_meter_menu.xml");
      break;
    }
    case gui::MenuType::ClassMenu: {
      gui::loadMenu("menus/chargen/class_menu.xml");
      break;
    }
    case gui::MenuType::ContainerMenu: {
      gui::loadMenu("menus/container_menu.xml");
      break;
    }
    case gui::MenuType::ControlsMenu: {
      gui::loadMenu("menus/options/controls_menu.xml");
      break;
    }
    case gui::MenuType::CreditsMenu: {
      gui::loadMenu("menus/options/credits_menu.xml");
      break;
    }
    case gui::MenuType::DialogMenu: {
      gui::loadMenu("menus/dialog/dialog_menu.xml");
      break;
    }
    case gui::MenuType::EffectSettingMenu: {
      gui::loadMenu("menus/dialog/enchantmentsetting_menu.xml");
      break;
    }
    case gui::MenuType::EnchantmentMenu: {
      gui::loadMenu("menus/dialog/enchantment.xml");
      break;
    }
    case gui::MenuType::GameplayMenu: {
// TODO: Is GameplayMenu a lack of menu, or something else?
      break;
    }
    case gui::MenuType::GenericMenu: {
// TODO: There are multiple Generic menus, need an additional parameter.
      break;
    }
    case gui::MenuType::HUDInfoMenu: {
      gui::loadMenu("menus/main/hud_info_menu.xml");
      break;
    }
    case gui::MenuType::HUDMainMenu: {
      gui::loadMenu("menus/main/hud_main_menu.xml");
      break;
    }
    case gui::MenuType::HUDSubtitleMenu: {
      gui::loadMenu("menus/main/hud_subtitle_menu.xml");
      break;
    }
    case gui::MenuType::InventoryMenu: {
      gui::loadMenu("menus/main/inventory_menu.xml");
      break;
    }
    case gui::MenuType::LevelUpMenu: {
      gui::loadMenu("menus/levelup_menu.xml");
      break;
    }
    case gui::MenuType::LoadingMenu: {
      gui::loadMenu("menus/loading_menu.xml");
      break;
    }
    case gui::MenuType::LoadMenu: {
      gui::loadMenu("menus/options/load_menu.xml");
      break;
    }
    case gui::MenuType::LockPickMenu: {
      gui::loadMenu("menus/lockpick_menu.xml");
      break;
    }
    case gui::MenuType::MagicMenu: {
      gui::loadMenu("menus/main/magic_menu.xml");
      break;
    }
    case gui::MenuType::MagicPopupMenu: {
      gui::loadMenu("menus/main/magic_popup_menu.xml");
      break;
    }
    case gui::MenuType::MainMenu: {
      gui::loadMenu("menus/options/main_menu.xml");
      break;
    }
    case gui::MenuType::MapMenu: {
      gui::loadMenu("menus/main/map_menu.xml");
      break;
    }
    case gui::MenuType::MessageMenu: {
      gui::loadMenu("menus/message_menu.xml");
      break;
    }
    case gui::MenuType::NegotiateMenu: {
      gui::loadMenu("menus/negotiate_menu.xml");
      break;
    }
    case gui::MenuType::OptionsMenu: {
      gui::loadMenu("menus/options/options_menu.xml");
      break;
    }
    case gui::MenuType::PauseMenu: {
      gui::loadMenu("menus/options/pause_menu.xml");
      break;
    }
    case gui::MenuType::PersuasionMenu: {
      gui::loadMenu("menus/dialog/persuasion_menu.xml");
      break;
    }
    case gui::MenuType::QuantityMenu: {
      gui::loadMenu("menus/quantity_menu.xml");
      break;
    }
    case gui::MenuType::QuickKeysMenu: {
      gui::loadMenu("menus/main/quickkeys_menu.xml");
      break;
    }
    case gui::MenuType::RaceSexMenu: {
      gui::loadMenu("menus/chargen/race_sex_menu.xml");
      break;
    }
    case gui::MenuType::RechargeMenu: {
      gui::loadMenu("menus/recharge_menu.xml");
      break;
    }
    case gui::MenuType::RepairMenu: {
      gui::loadMenu("menus/repair_menu.xml");
      break;
    }
    case gui::MenuType::SaveMenu: {
      gui::loadMenu("menus/options/save_menu.xml");
      break;
    }
    case gui::MenuType::SigilStoneMenu: {
      gui::loadMenu("menus/dialog/sigilstone.xml");
      break;
    }
    case gui::MenuType::SkillsMenu: {
      gui::loadMenu("menus/chargen/skills_menu.xml");
      break;
    }
    case gui::MenuType::SleepWaitMenu: {
      gui::loadMenu("menus/sleep_wait_menu.xml");
      break;
    }
    case gui::MenuType::SpellMakingMenu: {
      gui::loadMenu("menus/dialog/spellmaking.xml");
      break;
    }
    case gui::MenuType::SpellPurchaseMenu: {
      gui::loadMenu("menus/dialog/spell_purchase.xml");
      break;
    }
    case gui::MenuType::StatsMenu: {
      gui::loadMenu("menus/main/stats_menu.xml");
      break;
    }
    case gui::MenuType::TextEditMenu: {
      gui::loadMenu("menus/dialog/texteditmenu.xml");
      break;
    }
    case gui::MenuType::TrainingMenu: {
      gui::loadMenu("menus/training_menu.xml");
      break;
    }
    case gui::MenuType::VideoMenu: {
      gui::loadMenu("menus/options/video_mode.xml");
      break;
    }
    default:break;
  }
}

MenuMode::transition_t
MenuMode::handleEvent(ApplicationContext &ctx, const sdl::Event &event) {
  // Pop if a event::MenuMode is pressed
  if (auto keyEvent{ctx.getKeyMap().translateKey(event)}) {
    if (auto *ev{std::get_if<oo::event::MenuMode>(&*keyEvent)}; ev) {
      return {ev->down, std::nullopt};
    }
  }
  return {false, std::nullopt};
}

void MenuMode::update(ApplicationContext &ctx, float delta) {
}
