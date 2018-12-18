#include "modes/menu_mode.hpp"

std::optional<gui::MenuContext> MenuMode::loadMenu(gui::MenuType type) {
  const auto loadWithStrings = [](const std::string &filename) {
    return gui::loadMenu(filename, "menus/strings.xml");
  };

  switch (type) {
    case gui::MenuType::AlchemyMenu: {
      return loadWithStrings("menus/dialog/alchemy.xml");
    }
    case gui::MenuType::AudioMenu: {
      return loadWithStrings("menus/options/audio_menu.xml");
    }
    case gui::MenuType::BookMenu: {
      return loadWithStrings("menus/book_menu.xml");
    }
    case gui::MenuType::BreathMenu: {
      return loadWithStrings("menus/breath_meter_menu.xml");
    }
    case gui::MenuType::ClassMenu: {
      return loadWithStrings("menus/chargen/class_menu.xml");
    }
    case gui::MenuType::ContainerMenu: {
      return loadWithStrings("menus/container_menu.xml");
    }
    case gui::MenuType::ControlsMenu: {
      return loadWithStrings("menus/options/controls_menu.xml");
    }
    case gui::MenuType::CreditsMenu: {
      return loadWithStrings("menus/options/credits_menu.xml");
    }
    case gui::MenuType::DialogMenu: {
      return loadWithStrings("menus/dialog/dialog_menu.xml");
    }
    case gui::MenuType::EffectSettingMenu: {
      return loadWithStrings("menus/dialog/enchantmentsetting_menu.xml");
    }
    case gui::MenuType::EnchantmentMenu: {
      return loadWithStrings("menus/dialog/enchantment.xml");
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
      return loadWithStrings("menus/main/hud_info_menu.xml");
    }
    case gui::MenuType::HUDMainMenu: {
      return loadWithStrings("menus/main/hud_main_menu.xml");
    }
    case gui::MenuType::HUDSubtitleMenu: {
      return loadWithStrings("menus/main/hud_subtitle_menu.xml");
    }
    case gui::MenuType::InventoryMenu: {
      return loadWithStrings("menus/main/inventory_menu.xml");
    }
    case gui::MenuType::LevelUpMenu: {
      return loadWithStrings("menus/levelup_menu.xml");
    }
    case gui::MenuType::LoadingMenu: {
      return loadWithStrings("menus/loading_menu.xml");
    }
    case gui::MenuType::LoadMenu: {
      return loadWithStrings("menus/options/load_menu.xml");
    }
    case gui::MenuType::LockPickMenu: {
      return loadWithStrings("menus/lockpick_menu.xml");
    }
    case gui::MenuType::MagicMenu: {
      return loadWithStrings("menus/main/magic_menu.xml");
    }
    case gui::MenuType::MagicPopupMenu: {
      return loadWithStrings("menus/main/magic_popup_menu.xml");
    }
    case gui::MenuType::MainMenu: {
      return loadWithStrings("menus/options/main_menu.xml");
    }
    case gui::MenuType::MapMenu: {
      return loadWithStrings("menus/main/map_menu.xml");
    }
    case gui::MenuType::MessageMenu: {
      return loadWithStrings("menus/message_menu.xml");
    }
    case gui::MenuType::NegotiateMenu: {
      return loadWithStrings("menus/negotiate_menu.xml");
    }
    case gui::MenuType::OptionsMenu: {
      return loadWithStrings("menus/options/options_menu.xml");
    }
    case gui::MenuType::PauseMenu: {
      return loadWithStrings("menus/options/pause_menu.xml");
    }
    case gui::MenuType::PersuasionMenu: {
      return loadWithStrings("menus/dialog/persuasion_menu.xml");
    }
    case gui::MenuType::QuantityMenu: {
      return loadWithStrings("menus/quantity_menu.xml");
    }
    case gui::MenuType::QuickKeysMenu: {
      return loadWithStrings("menus/main/quickkeys_menu.xml");
    }
    case gui::MenuType::RaceSexMenu: {
      return loadWithStrings("menus/chargen/race_sex_menu.xml");
    }
    case gui::MenuType::RechargeMenu: {
      return loadWithStrings("menus/recharge_menu.xml");
    }
    case gui::MenuType::RepairMenu: {
      return loadWithStrings("menus/repair_menu.xml");
    }
    case gui::MenuType::SaveMenu: {
      return loadWithStrings("menus/options/save_menu.xml");
    }
    case gui::MenuType::SigilStoneMenu: {
      return loadWithStrings("menus/dialog/sigilstone.xml");
    }
    case gui::MenuType::SkillsMenu: {
      return loadWithStrings("menus/chargen/skills_menu.xml");
    }
    case gui::MenuType::SleepWaitMenu: {
      return loadWithStrings("menus/sleep_wait_menu.xml");
    }
    case gui::MenuType::SpellMakingMenu: {
      return loadWithStrings("menus/dialog/spellmaking.xml");
    }
    case gui::MenuType::SpellPurchaseMenu: {
      return loadWithStrings("menus/dialog/spell_purchase.xml");
    }
    case gui::MenuType::StatsMenu: {
      return loadWithStrings("menus/main/stats_menu.xml");
    }
    case gui::MenuType::TextEditMenu: {
      return loadWithStrings("menus/dialog/texteditmenu.xml");
    }
    case gui::MenuType::TrainingMenu: {
      return loadWithStrings("menus/training_menu.xml");
    }
    case gui::MenuType::VideoMenu: {
      return loadWithStrings("menus/options/video_mode.xml");
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