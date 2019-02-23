#include "globals.hpp"
#include "time_manager.hpp"
#include <memory>

namespace oo {

TimeManager &TimeManager::getSingleton() {
  // Cannot std::make_unique on a private constructor
  // TODO: Abseil TOTW #134
  static std::unique_ptr<TimeManager> instance{new TimeManager()};
  return *instance;
}

namespace chrono {

uint64_t GameClock::ticks = 0;

GameClock::time_point GameClock::now() noexcept {
  return GameClock::time_point(duration(ticks));
}

void GameClock::advance(float delta) noexcept {
  const auto &globs{Globals::getSingleton()};
  ticks += static_cast<uint64_t>(1000.0f * globs.fGet("TimeScale") * delta);
}

chrono::year_month_day GameClock::getEpochDate() noexcept {
  return chrono::year_month_day(chrono::game_days(chrono::days(0)));
}

void GameClock::setDate(const chrono::year_month_day &date) noexcept {
  ticks = chrono::duration_cast<GameClock::duration>(
      chrono::game_days(date).time_since_epoch()).count();
}

} // namespace chrono

} // namespace oo