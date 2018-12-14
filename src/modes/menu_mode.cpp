#include "modes/menu_mode.hpp"

std::optional<gui::MenuContext> MenuMode::loadMenu(gui::MenuType type) {
  switch (type) {
    case gui::MenuType::AlchemyMenu: {
      return gui::loadMenu("menus/dialog/alchemy.xml");
    }
    case gui::MenuType::AudioMenu: {
      return gui::loadMenu("menus/options/audio_menu.xml");
    }
    case gui::MenuType::BookMenu: {
      return gui::loadMenu("menus/book_menu.xml");
    }
    case gui::MenuType::BreathMenu: {
      return gui::loadMenu("menus/breath_meter_menu.xml");
    }
    case gui::MenuType::ClassMenu: {
      return gui::loadMenu("menus/chargen/class_menu.xml");
    }
    case gui::MenuType::ContainerMenu: {
      return gui::loadMenu("menus/container_menu.xml");
    }
    case gui::MenuType::ControlsMenu: {
      return gui::loadMenu("menus/options/controls_menu.xml");
    }
    case gui::MenuType::CreditsMenu: {
      return gui::loadMenu("menus/options/credits_menu.xml");
    }
    case gui::MenuType::DialogMenu: {
      return gui::loadMenu("menus/dialog/dialog_menu.xml");
    }
    case gui::MenuType::EffectSettingMenu: {
      return gui::loadMenu("menus/dialog/enchantmentsetting_menu.xml");
    }
    case gui::MenuType::EnchantmentMenu: {
      return gui::loadMenu("menus/dialog/enchantment.xml");
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
      return gui::loadMenu("menus/main/hud_info_menu.xml");
    }
    case gui::MenuType::HUDMainMenu: {
      return gui::loadMenu("menus/main/hud_main_menu.xml");
    }
    case gui::MenuType::HUDSubtitleMenu: {
      return gui::loadMenu("menus/main/hud_subtitle_menu.xml");
    }
    case gui::MenuType::InventoryMenu: {
      return gui::loadMenu("menus/main/inventory_menu.xml");
    }
    case gui::MenuType::LevelUpMenu: {
      return gui::loadMenu("menus/levelup_menu.xml");
    }
    case gui::MenuType::LoadingMenu: {
      return gui::loadMenu("menus/loading_menu.xml");
    }
    case gui::MenuType::LoadMenu: {
      return gui::loadMenu("menus/options/load_menu.xml");
    }
    case gui::MenuType::LockPickMenu: {
      return gui::loadMenu("menus/lockpick_menu.xml");
    }
    case gui::MenuType::MagicMenu: {
      return gui::loadMenu("menus/main/magic_menu.xml");
    }
    case gui::MenuType::MagicPopupMenu: {
      return gui::loadMenu("menus/main/magic_popup_menu.xml");
    }
    case gui::MenuType::MainMenu: {
      return gui::loadMenu("menus/options/main_menu.xml");
    }
    case gui::MenuType::MapMenu: {
      return gui::loadMenu("menus/main/map_menu.xml");
    }
    case gui::MenuType::MessageMenu: {
      return gui::loadMenu("menus/message_menu.xml");
    }
    case gui::MenuType::NegotiateMenu: {
      return gui::loadMenu("menus/negotiate_menu.xml");
    }
    case gui::MenuType::OptionsMenu: {
      return gui::loadMenu("menus/options/options_menu.xml");
    }
    case gui::MenuType::PauseMenu: {
      return gui::loadMenu("menus/options/pause_menu.xml");
    }
    case gui::MenuType::PersuasionMenu: {
      return gui::loadMenu("menus/dialog/persuasion_menu.xml");
    }
    case gui::MenuType::QuantityMenu: {
      return gui::loadMenu("menus/quantity_menu.xml");
    }
    case gui::MenuType::QuickKeysMenu: {
      return gui::loadMenu("menus/main/quickkeys_menu.xml");
    }
    case gui::MenuType::RaceSexMenu: {
      return gui::loadMenu("menus/chargen/race_sex_menu.xml");
    }
    case gui::MenuType::RechargeMenu: {
      return gui::loadMenu("menus/recharge_menu.xml");
    }
    case gui::MenuType::RepairMenu: {
      return gui::loadMenu("menus/repair_menu.xml");
    }
    case gui::MenuType::SaveMenu: {
      return gui::loadMenu("menus/options/save_menu.xml");
    }
    case gui::MenuType::SigilStoneMenu: {
      return gui::loadMenu("menus/dialog/sigilstone.xml");
    }
    case gui::MenuType::SkillsMenu: {
      return gui::loadMenu("menus/chargen/skills_menu.xml");
    }
    case gui::MenuType::SleepWaitMenu: {
      return gui::loadMenu("menus/sleep_wait_menu.xml");
    }
    case gui::MenuType::SpellMakingMenu: {
      return gui::loadMenu("menus/dialog/spellmaking.xml");
    }
    case gui::MenuType::SpellPurchaseMenu: {
      return gui::loadMenu("menus/dialog/spell_purchase.xml");
    }
    case gui::MenuType::StatsMenu: {
      return gui::loadMenu("menus/main/stats_menu.xml");
    }
    case gui::MenuType::TextEditMenu: {
      return gui::loadMenu("menus/dialog/texteditmenu.xml");
    }
    case gui::MenuType::TrainingMenu: {
      return gui::loadMenu("menus/training_menu.xml");
    }
    case gui::MenuType::VideoMenu: {
      return gui::loadMenu("menus/options/video_mode.xml");
    }
    default: return std::nullopt;
  }
  return std::nullopt;
}

MenuMode::MenuMode(ApplicationContext &ctx, gui::MenuType type) {
  refocus(ctx);
  mMenuCtx = loadMenu(type);
  if (!mMenuCtx) {
    throw std::runtime_error("Failed to construct menu");
  }
  mMenuCtx->update();
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
