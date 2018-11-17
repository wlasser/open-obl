#include <catch.hpp>
#include "fs/path.hpp"

TEST_CASE("Can match paths", "[fs]") {
  // Internal wildcards
  fs::Path pat1{"h*o"};

  fs::Path src11{"hello"};
  REQUIRE(src11.match(pat1));

  fs::Path src12{"hell"};
  REQUIRE_FALSE(src12.match(pat1));

  fs::Path src13{"HelLo"};
  REQUIRE(src13.match(pat1));

  fs::Path src14{"hello world"};
  REQUIRE_FALSE(src14.match(pat1));

  // Trailing wildcards
  fs::Path pat2{"hel*"};

  fs::Path src21{"hello"};
  REQUIRE(src21.match(pat2));

  fs::Path src22{"hell"};
  REQUIRE(src22.match(pat2));

  fs::Path src23{"he"};
  REQUIRE_FALSE(src23.match(pat2));

  fs::Path src24{"hello world"};
  REQUIRE(src24.match(pat2));

  fs::Path src25{"HeLlO"};
  REQUIRE(src25.match(pat2));
}
