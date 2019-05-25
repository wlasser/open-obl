#include "chrono.hpp"
#include "config/globals.hpp"
#include <memory>

namespace oo::chrono {

uint64_t GameClock::ticks = 0;

GameClock::time_point GameClock::now() noexcept {
  return GameClock::time_point(duration(ticks));
}

void GameClock::advance(float delta) noexcept {
  const auto &globs{Globals::getSingleton()};
  ticks += static_cast<uint64_t>(1000.0f * globs.sGet("TimeScale") * delta);
}

chrono::year_month_day GameClock::getEpochDate() noexcept {
  return chrono::year_month_day(chrono::game_days(chrono::days(0)));
}

void GameClock::setDate(const chrono::year_month_day &date) noexcept {
  ticks = chrono::duration_cast<GameClock::duration>(
      chrono::game_days(date).time_since_epoch()).count();
}

chrono::year_month_day GameClock::getDate() noexcept {
  return chrono::year_month_day{chrono::time_point_cast<chrono::days>(now())};
}

void GameClock::updateGlobals() {
  auto &globs{Globals::getSingleton()};
  const auto now{GameClock::getDate()};
  globs.sGet("GameEra") = 3;
  globs.sGet("GameYear") = static_cast<uint16_t>(int(now.year()));
  globs.sGet("GameMonth") = static_cast<uint16_t>(unsigned(now.month()));
  globs.sGet("GameDay") = static_cast<uint16_t>(unsigned(now.day()));

  const auto time{GameClock::now() - chrono::game_days(now)};
  auto milliseconds{chrono::duration_cast<chrono::milliseconds>(time).count()};
  globs.fGet("GameHour") = milliseconds / (1000.0f * 60.0f * 60.0f);
}

void GameClock::updateFromGlobals() {
  const auto &globs{Globals::getSingleton()};
  const chrono::year year{globs.sGet("GameYear")};
  const chrono::month month{static_cast<unsigned>(globs.sGet("GameMonth"))};
  const chrono::day day{static_cast<unsigned>(globs.sGet("GameDay"))};
  GameClock::setDate(year / month / day);

  float hours{globs.fGet("GameHour")};
  auto milliseconds{static_cast<uint64_t>(hours * (1000.0f * 60.0f * 60.0f))};
  GameClock::advance(chrono::milliseconds(milliseconds));
}

} // namespace oo::chrono