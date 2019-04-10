#include "modes/load_menu_mode.hpp"
#include "modes/game_mode.hpp"
#include "save_state.hpp"
#include <boost/range/adaptor/indexed.hpp>
#include <spdlog/fmt/bundled/chrono.h>

namespace oo {

MenuMode<gui::MenuType::LoadMenu>::MenuMode(ApplicationContext &ctx)
    : MenuModeBase<LoadMenuMode>(ctx),
      btnReturn{getElementWithId(1)},
      focusBoxSave{getElementWithId(2)},
      listScrollLoad{getElementWithId(3)},
      listLoad{getElementWithId(5)},
      imgLoadPictureBackground{getElementWithId(6)},
      loadText{getElementWithId(7)},
      listPane{getElementWithId(9)} {
  ctx.getLogger()->info("Registered {} templates",
                        getMenuCtx()->registerTemplates());
  auto saveDir{oo::getSaveDirectory()};
  ctx.getLogger()->info("Save game path is {}", saveDir.c_str());

  // TODO: Save games should be sorted in date order.
  float index{0.0f};
  for (const auto &saveFile : std::filesystem::directory_iterator(saveDir)) {
    const auto &savePath{saveFile.path()};
    if (savePath.extension() != ".ess") continue;
    // Get the required metadata from the save game header.
    oo::SaveState saveState(ctx.getBaseResolvers());
    std::ifstream saveStream(savePath, std::ios_base::binary);
    oo::readSaveHeader(saveStream, saveState);

    // Number of real-world milliseconds the player has played for.
    chrono::milliseconds playDuration{saveState.mGameTicksPassed};
    // Number of in-game days that have passed.
    auto gameDaysPassed{static_cast<long>(saveState.mGameDaysPassed)};

    // C++20: Use <format> instead of relying on spdlog's copy of {fmt}.
    // C++20: Convert the SystemTime date with <chrono> to get localised output.
    auto name{fmt::format("Save {} - Level {}\n"
                          "Play Time: {:%T}",
                          saveState.mSaveNumber,
                          saveState.mPlayerLevel,
                          playDuration)};
    const auto date{saveState.mSaveTime};
    auto description{fmt::format("{}\n"
                                 "Level {}\n"
                                 "{}\n"
                                 "Day {}\n"
                                 "{}/{}/{} {}:{}",
                                 saveState.mPlayerName,
                                 saveState.mPlayerLevel,
                                 saveState.mPlayerCellName,
                                 gameDaysPassed,
                                 date.month, date.day, date.year,
                                 date.hour, date.minute)};

    auto *entry{getMenuCtx()->appendTemplate(listPane, "load_game_template")};
    entry->set_user(0, index++);
    entry->set_user(3, name);

    // TODO: This should be set to the selected entry, not the last one.
    loadText->set_string(description);
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
