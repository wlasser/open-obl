#ifndef OPENOBLIVION_LOAD_MENU_MODE_HPP
#define OPENOBLIVION_LOAD_MENU_MODE_HPP

#include "modes/menu_mode.hpp"
#include "modes/menu_mode_base.hpp"
#include "save_state.hpp"

namespace oo {

/// \ingroup OpenOblivionModes
template<> struct MenuModeTransition<LoadMenuMode> {
  using type = ModeTransition<LoadMenuMode, LoadingMenuMode>;
};

/// Specialization of `oo::MenuMode` for the Load Game Menu.
/// \ingroup OpenOblivionModes
template<> class MenuMode<gui::MenuType::LoadMenu>
    : public MenuModeBase<LoadMenuMode> {
 private:
  /// `<id> 1 </id>`
  gui::UiElement *btnReturn{};

  /// `<id> 2 </id>`
  gui::UiElement *focusBoxSave{};

  /// `<id> 3 </id>`
  gui::UiElement *listScrollLoad{};

  /// `<id> 5 </id>`
  gui::UiElement *listLoad{};

  /// `<id> 6 </id>`
  // TODO: Two elements with the same id, what to do?
  gui::UiElement *imgLoadPictureBackground{};
  //gui::UiElement *imgLoadPicture{};

  /// `<id> 7 </id>`
  /// This displays metadata about the currently selected save.
  gui::UiElement *loadText{};

  /// `<id> 9 </id>`
  gui::UiElement *listPane{};

  using SaveEntry = std::filesystem::directory_entry;

  struct SaveGame {
    gui::UiElement *element;
    SaveState state;
    SaveEntry entry;
    explicit SaveGame(gui::UiElement *element,
                      SaveState state,
                      SaveEntry entry)
        : element(element), state(std::move(state)), entry(std::move(entry)) {}
  };

  std::vector<SaveGame> mSaveGames{};
  std::size_t mSaveIndex{};

  /// Find all the save games and sort them by access time, most recent first.
  std::vector<SaveEntry> getSaveEntries() const;

  std::string getSaveName(const SaveState &saveState) const;
  std::string getSaveDescription(const SaveState &saveState) const;

  void setCurrentSave(std::size_t index);

 public:
  explicit MenuMode<gui::MenuType::LoadMenu>(ApplicationContext &ctx);

  std::string getFilenameImpl() const {
    return "menus/options/load_menu.xml";
  }

  LoadMenuMode::transition_t
  handleEventImpl(ApplicationContext &ctx, const sdl::Event &event);

  void updateImpl(ApplicationContext &ctx, float delta);
};

} // namespace oo

#endif // OPENOBLIVION_LOAD_MENU_MODE_HPP
