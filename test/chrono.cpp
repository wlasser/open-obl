#include "time_manager.hpp"
#include <catch2/catch.hpp>

TEST_CASE("Can get the epoch date") {
  // Note: Days are zero-based, so day 25 is the 26th.
  const oo::chrono::year_month_day epoch(oo::chrono::year(433),
                                         oo::chrono::LastSeed,
                                         oo::chrono::day(25));
  REQUIRE(oo::chrono::GameClock::getEpochDate() == epoch);
}

TEST_CASE("Clock begins at the epoch") {
  oo::chrono::GameClock::reset();

  const auto epoch{oo::chrono::GameClock::now()};
  const auto clockDays{oo::chrono::time_point_cast<oo::chrono::days>(epoch)};
  const oo::chrono::year_month_day clockYmd{clockDays};
  // Note: Days are zero-based, so day 25 is the 26th.
  const oo::chrono::year_month_day literalYmd(oo::chrono::year(433),
                                              oo::chrono::LastSeed,
                                              oo::chrono::day(25));
  const oo::chrono::game_days literalDays{literalYmd};

  REQUIRE(clockYmd == literalYmd);
  REQUIRE(clockDays == literalDays);
  REQUIRE(oo::chrono::weekday(clockDays) == oo::chrono::Sundas);
}

TEST_CASE("Can advance time") {
  oo::chrono::GameClock::reset();

  const auto t0{oo::chrono::GameClock::now()};
  const auto dur{oo::chrono::seconds(73) + oo::chrono::days(16)};
  oo::chrono::GameClock::advance(dur);
  const auto t1{oo::chrono::GameClock::now()};

  REQUIRE (t1 - t0 == dur);
}

TEST_CASE("Can set the date") {
  const oo::chrono::year_month_day date(oo::chrono::year(435),
                                        oo::chrono::SunsDusk,
                                        oo::chrono::day(3));
  oo::chrono::GameClock::setDate(date);
  const auto now{oo::chrono::time_point_cast<oo::chrono::days>(
      oo::chrono::GameClock::now())};
  const oo::chrono::year_month_day ymd(now);
  REQUIRE(ymd == date);
}

TEST_CASE("Interface with Globals is invertible") {
  oo::chrono::GameClock::reset();
  oo::chrono::GameClock::advance(oo::chrono::seconds(123456789));
  const auto t0{oo::chrono::GameClock::now()};
  oo::chrono::GameClock::updateGlobals();
  oo::chrono::GameClock::updateFromGlobals();
  const auto t1{oo::chrono::GameClock::now()};

  // We can't actually expect 100% accuracy here because of the precision of
  // float. Being accuracte to seconds is good enough.
  // TODO: Test the largest possible time.
  REQUIRE(oo::chrono::time_point_cast<oo::chrono::seconds>(t0) ==
      oo::chrono::time_point_cast<oo::chrono::seconds>(t1));
}