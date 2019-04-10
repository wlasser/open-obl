#include "modes/load_menu_mode.hpp"
#include "modes/game_mode.hpp"
#include "save_state.hpp"
#include <boost/range/adaptor/indexed.hpp>
#include <spdlog/fmt/bundled/chrono.h>

namespace oo {

std::vector<LoadMenuMode::SaveEntry>
LoadMenuMode::getSaveEntries() const {
  namespace fs = std::filesystem;
  auto saveDir{oo::getSaveDirectory()};
  std::vector<fs::directory_entry> saves{};

  fs::directory_iterator begin(saveDir), end{};
  std::copy_if(begin, end, std::back_inserter(saves), [](const auto &entry) {
    return entry.path().extension() == ".ess";
  });

  std::sort(saves.begin(), saves.end(), [](const auto &a, const auto &b) {
    return a.last_write_time() > b.last_write_time();
  });

  return saves;
}

std::string LoadMenuMode::getSaveName(const SaveState &saveState) const {
  // Number of real-world milliseconds the player has played for.
  chrono::milliseconds playDuration{saveState.mGameTicksPassed};

  // C++20: Use <format> instead of {fmt}.
  return fmt::format("Save {} - Level {}\n"
                     "Play Time: {:%T}",
                     saveState.mSaveNumber, saveState.mPlayerLevel,
                     playDuration);
}

std::string LoadMenuMode::getSaveDescription(const SaveState &saveState) const {
  // Number of in-game days that have passed.
  auto gameDaysPassed{static_cast<long>(saveState.mGameDaysPassed)};

  // C++20: Convert the SystemTime date with <chrono> to get localised output.
  // C++20: Use <format> instead of {fmt}.
  const auto date{saveState.mSaveTime};
  return fmt::format("{}\n"
                     "Level {}\n"
                     "{}\n"
                     "Day {}\n"
                     "{}/{}/{} {}:{}",
                     saveState.mPlayerName,
                     saveState.mPlayerLevel,
                     saveState.mPlayerCellName,
                     gameDaysPassed,
                     date.month, date.day, date.year, date.hour, date.minute);
}

MenuMode<gui::MenuType::LoadMenu>::MenuMode(ApplicationContext &ctx)
    : MenuModeBase<LoadMenuMode>(ctx),
      btnReturn{getElementWithId(1)},
      focusBoxSave{getElementWithId(2)},
      listScrollLoad{getElementWithId(3)},
      listLoad{getElementWithId(5)},
      imgLoadPictureBackground{getElementWithId(6)},
      loadText{getElementWithId(7)},
      listPane{getElementWithId(9)} {
  getMenuCtx()->registerTemplates();
  auto entries{getSaveEntries()};
  for (const auto &entry : entries | boost::adaptors::indexed()) {
    // Get the required metadata from the save game header.
    oo::SaveState saveState(ctx.getBaseResolvers());
    std::ifstream saveStream(entry.value().path(), std::ios_base::binary);
    oo::readSaveHeader(saveStream, saveState);

    auto name{getSaveName(saveState)};
    auto desc{getSaveDescription(saveState)};

    auto *templ{getMenuCtx()->appendTemplate(listPane, "load_game_template")};
    templ->set_user(0, static_cast<float>(entry.index()));
    templ->set_user(3, std::move(name));

    // TODO: This should be set to the selected entry, not the last one.
    loadText->set_string(std::move(desc));
  }

  getMenuCtx()->update();
}

LoadMenuMode::transition_t
LoadMenuMode::handleEventImpl(ApplicationContext &/*ctx*/,
                              const sdl::Event &/*event*/) {
  return {false, std::nullopt};
}

void LoadMenuMode::updateImpl(ApplicationContext &/*ctx*/, float /*delta*/) {}

} // namespace oo
