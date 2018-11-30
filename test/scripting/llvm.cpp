#include "helpers.hpp"
#include "scripting/console_engine.hpp"
#include "scripting/script_engine.hpp"
#include <catch2/catch.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string_view>

TEST_CASE("can compile empty blocks", "[scripting]") {
  auto &se{oo::getScriptEngine()};
  auto logger{spdlog::stderr_color_mt("scripting_test")};
  oo::scriptingLogger("scripting_test");

  {
    std::string_view script = R"script(
scn MyScript
begin GameMode
end
)script";
    REQUIRE_NOTHROW(se.compile(script));
    REQUIRE_NOTHROW(se.call<void>("MyScript", "GameMode"));
  }
}

TEST_CASE("can use llvm", "[scripting]") {
  std::string_view script = R"script(
scn MyScript

begin TestLong
  long foo
  set foo to 1
  long bar
  set bar to 0

  if foo < 1
    return 1
  elseif foo == 1
    if bar == 0
      set bar to Func 1
      return bar
    else
      return 10
    endif
  else
    return 2
  endif
end
  )script";

  std::string_view script2 = R"script(
scn MyOtherScript

begin TestLong
  long foo
  set foo to Func 7
  return foo
end
  )script";

  auto logger{spdlog::stderr_color_mt("test")};
  oo::scriptingLogger("test");

  auto &se{oo::getScriptEngine()};

  se.compile(script);
  {
    const auto result{se.call<int>("MyScript", "TestLong")};
    REQUIRE(result);
    REQUIRE(*result == 9);
  }

  se.compile(script2);
  {
    const auto result{se.call<int>("MyOtherScript", "TestLong")};
    REQUIRE(result);
    REQUIRE(*result == 63);
  }
  {
    const auto result{se.call<int>("MyScript", "TestLong")};
    REQUIRE(result);
    REQUIRE(*result == 9);
  }
}