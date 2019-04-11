#include "modes/load_menu_mode.hpp"
#include "modes/loading_menu_mode.hpp"
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

    auto *templ{getMenuCtx()->appendTemplate(listPane, "load_game_template")};
    templ->set_user(0, static_cast<float>(entry.index()));
    templ->set_user(3, std::move(name));

    mSaveGames.emplace_back(templ, std::move(saveState),
                            std::move(entry.value()));
  }

  setCurrentSave(0u);

  getMenuCtx()->update();
}

void LoadMenuMode::setCurrentSave(std::size_t index) {
  if (mSaveGames.empty()) return;
  mSaveIndex = index % mSaveGames.size();

  const auto &saveGame{mSaveGames[mSaveIndex]};
  auto desc{getSaveDescription(saveGame.state)};
  loadText->set_string(std::move(desc));
}

LoadMenuMode::transition_t
LoadMenuMode::handleEventImpl(ApplicationContext &ctx,
                              const sdl::Event &event) {
  auto keyEvent{ctx.getKeyMap().translateKey(event)};
  if (!keyEvent) return {false, std::nullopt};

  return std::visit(overloaded{
      [this](oo::event::Forward e) -> transition_t {
        if (e.down) setCurrentSave(mSaveIndex - 1u);
        return {};
      },
      [this](oo::event::Backward e) -> transition_t {
        if (e.down) setCurrentSave(mSaveIndex + 1u);
        return {};
      },
      [&](oo::event::SlideRight e) -> transition_t {
        if (e.down) {
          auto &saveGame{mSaveGames[mSaveIndex]};
          std::ifstream saveStream(saveGame.entry.path(),
                                   std::ios_base::binary);
          saveStream >> saveGame.state;

          std::vector<oo::Path> plugins(saveGame.state.mPlugins.begin(),
                                        saveGame.state.mPlugins.end());
          if (!ctx.getCoordinator().contains(plugins.begin(), plugins.end())) {
            ctx.getLogger()->warn("Plugins are not compatible");
            return {};
          }
          auto request{saveGame.state.makeCellRequest()};
          return {true, oo::LoadingMenuMode(ctx, std::move(request))};
        }

        return {};
      },
      [](auto) -> transition_t { return {}; }
  }, *keyEvent);
}

void LoadMenuMode::updateImpl(ApplicationContext &/*ctx*/, float /*delta*/) {}

} // namespace oo
