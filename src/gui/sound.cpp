#include "gui/sound.hpp"

std::string gui::getClicksound(int index) {
  // Some sounds are chosen 'randomly' from a list of possibilities, but we
  // don't really care about randomness so instead just increment this counter
  // and mod it to choose a sound. Unsigned integer overflow is defined to wrap
  // so we don't need to worry there.
  static unsigned int counter{0};

  switch (index) {
    case 1: return "sound/fx/ui/menu/ui_menu_ok.wav";
    case 2: return "sound/fx/ui/menu/ui_menu_cancel.wav";
    case 3: return "sound/fx/ui/menu/ui_menu_prevnext.wav";
    case 4:
      switch (++counter % 3u) {
        default: [[fallthrough]];
        case 0: return "sound/fx/ui/menu/focus/ui_menu_focus_01.wav";
        case 1: return "sound/fx/ui/menu/focus/ui_menu_focus_02.wav";
        case 2: return "sound/fx/ui/menu/focus/ui_menu_focus_03.wav";
      }
    case 5:
      switch (++counter % 3u) {
        default: [[fallthrough]];
        case 0: return "sound/fx/ui/menu/tabs/ui_menu_tab_01.wav";
        case 1: return "sound/fx/ui/menu/tabs/ui_menu_tab_02.wav";
        case 2: return "sound/fx/ui/menu/tabs/ui_menu_tab_03.wav";
      }
    case 6:
      switch (++counter % 3u) {
        default: [[fallthrough]];
        case 0: return "sound/fx/itm/book/pageturn/itm_book_pageturn_01.wav";
        case 1: return "sound/fx/itm/book/pageturn/itm_book_pageturn_02.wav";
        case 2: return "sound/fx/itm/book/pageturn/itm_book_pageturn_03.wav";
      }
    case 7: return "sound/fx/ui/ui_speechrollover.wav";
    case 8: return "sound/fx/ui/ui_speechrotate.wav";
    case 9: return "sound/fx/ui/ui_questnew.wav";
    case 10: return "sound/fx/ui/ui_questupdate.wav";
    case 11: return "sound/fx/ui/ui_message.wav";
    case 12: return ""; // MenuEnd
    case 13: return ""; // MenuStart
    case 14: return "sound/fx/ui/menu/ui_menu_bracket.wav";
    case 15: return "sound/fx/ui/ui_messagefade.wav";
    case 16:
      switch (++counter % 3u) {
        default: [[fallthrough]];
        case 0: return "sound/fx/ui/inventory/open/ui_inventory_open_01.wav";
        case 1: return "sound/fx/ui/inventory/open/ui_inventory_open_02.wav";
        case 2: return "sound/fx/ui/inventory/open/ui_inventory_open_03.wav";
      }
    case 17:
      switch (++counter % 3u) {
        default: [[fallthrough]];
        case 0: return "sound/fx/ui/inventory/close/ui_inventory_close_01.wav";
        case 1: return "sound/fx/ui/inventory/close/ui_inventory_close_02.wav";
        case 2: return "sound/fx/ui/inventory/close/ui_inventory_close_03.wav";
      }
    case 18: return "sound/fx/ui/ui_potioncreate.wav";
    case 19: return "sound/fx/doors/drs_locked.wav";
    case 20: return "sound/fx/ui/ui_message.wav"; // Duplicate of 11
    case 21: return "sound/fx/ui/menu/ui_menu_cancel.wav"; // Duplicate of 2
    case 22: return "sound/fx/ui/ui_stats_skillup.wav";
    case 23: return "sound/fx/spl/spl_equip.wav";
    case 24: return "sound/fx/itm/itm_welkyndstoneuse.wav";
    case 25: return "sound/fx/itm/itm_scroll_open.wav";
    case 26: return "sound/fx/itm/itm_scroll_close.wav";
    case 27: return "sound/fx/itm/itm_book_open.wav";
    case 28: return "sound/fx/itm/itm_book_close.wav";
    case 29: return "sound/fx/itm/itm_takeall.wav";
    case 30: return "sound/fx/itm/itm_ingredient_nothing.wav";
    case 31: return "sound/fx/itm/itm_ingredient_down.wav";
    case 32: return "sound/fx/itm/itm_soultrap.wav";
    case 33: return "sound/fx/ui/ui_armorweapon_repairbreak.wav";
    case 34: return "sound/fx/itm/itm_bounddisappear.wav";
    case 35: return "sound/fx/itm/itm_gold_up.wav";
    case 36: return "sound/fx/ui/ui_itemenchant.wav";
    default: [[fallthrough]];
    case 0: return ""; // No sound
  }
}
