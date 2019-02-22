#include "time_manager.hpp"
#include <catch2/catch.hpp>

TEST_CASE("Epoch is the correct time") {
  auto epoch{oo::chrono::GameClock::now()};
  auto clockDays{oo::chrono::time_point_cast<oo::chrono::days>(epoch)};
  oo::chrono::year_month_day clockYmd{clockDays};
  oo::chrono::year_month_day literalYmd(oo::chrono::year(433),
                                        oo::chrono::LastSeed,
                                        oo::chrono::day(26));
  oo::chrono::game_days literalDays{literalYmd};

  REQUIRE(clockYmd == literalYmd);
  REQUIRE(clockDays == literalDays);
  REQUIRE(oo::chrono::weekday(clockDays) == oo::chrono::Sundas);
}
