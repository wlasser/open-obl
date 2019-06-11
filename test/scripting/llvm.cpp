#include "helpers.hpp"
#include "scripting/console_engine.hpp"
#include "scripting/script_engine.hpp"
#include <catch2/catch.hpp>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <string_view>

TEST_CASE("can compile empty blocks", "[scripting]") {
  auto &se{oo::getScriptEngine()};

  {
    std::string_view script = "scn MyScript0";
    REQUIRE_NOTHROW(se.compile(script));
  }

  {
    std::string_view script = R"script(
scn MyScript1
begin GameMode
end
)script";
    REQUIRE_NOTHROW(se.compile(script));
    REQUIRE_NOTHROW(se.call<void>("MyScript1", "GameMode"));
  }

  {
    std::string_view script = R"script(
scn MyScript2
begin GameMode
end
begin TestLong
end
)script";
    REQUIRE_NOTHROW(se.compile(script));
    REQUIRE_NOTHROW(se.call<void>("MyScript2", "GameMode"));
    auto result{se.call<int>("MyScript2", "TestLong")};
    REQUIRE(result);
    REQUIRE(*result == 0);
  }
}

TEST_CASE("can compile single basic blocks", "[scripting]") {
  auto &se{oo::getScriptEngine()};

  {
    std::string_view script = R"script(
scn MyScript3
begin TestLong
  return 1
end
)script";
    REQUIRE_NOTHROW(se.compile(script));
    auto result{se.call<int>("MyScript3", "TestLong")};
    REQUIRE(result);
    REQUIRE(*result == 1);
  }

  {
    std::string_view script = R"script(
scn MyScript4
begin TestLong
  long foo
  set foo to 100
  return foo
end
)script";
    REQUIRE_NOTHROW(se.compile(script));
    auto result{se.call<int>("MyScript4", "TestLong")};
    REQUIRE(result);
    REQUIRE(*result == 100);
  }

  {
    std::string_view script = R"script(
scn MyScript5
begin TestFloat
  float foo
  set foo to 3.5
  return foo
end
)script";
    REQUIRE_NOTHROW(se.compile(script));
    auto result{se.call<float>("MyScript5", "TestFloat")};
    REQUIRE(result);
    REQUIRE_THAT(*result, Catch::WithinULP(3.5f, 1));
  }
}

TEST_CASE("can compile branched blocks", "[scripting]") {
  auto &se{oo::getScriptEngine()};

  {
    std::string_view script = R"script(
scn MyScript6
begin TestLong
  return 1
  return 2
end
)script";
    REQUIRE_NOTHROW(se.compile(script));
    auto result{se.call<int>("MyScript6", "TestLong")};
    REQUIRE(result);
    REQUIRE(*result == 1);
  }

  {
    std::string_view script = R"script(
scn MyScript7
begin TestLong
  long foo
  set foo to 3
  if foo < 3
    return 1
  elseif foo == 3
    return 2
  else
    return 3
  endif
end
)script";
    REQUIRE_NOTHROW(se.compile(script));
    auto result{se.call<int>("MyScript7", "TestLong")};
    REQUIRE(result);
    REQUIRE(*result == 2);
  }

  {
    std::string_view script = R"script(
scn MyScript8
begin TestLong
  long foo
  long bar
  set foo to 1
  set bar to 2
  if foo < 1
    return 1
  elseif foo == 1
    if bar < 2
      return 2
    else
      return 3
    endif
  endif
end
)script";
    REQUIRE_NOTHROW(se.compile(script));
    auto result{se.call<int>("MyScript8", "TestLong")};
    REQUIRE(result);
    REQUIRE(*result == 3);
  }
}

TEST_CASE("can compile scripts with implicit callee", "[scripting]") {
  std::string_view script = R"script(
scn MyScript
begin TestLong
  return MemberFunc 3
end
)script";

  auto &se{oo::getScriptEngine()};
  se.compile(script, 10u);
  const auto result{se.call<int>("MyScript", "TestLong")};
  REQUIRE(result);
  REQUIRE(*result == 30);
}

TEST_CASE("can call functions that do not have arguments", "[scripting]") {
  auto &se{oo::getScriptEngine()};

  {
    std::string_view script = R"script(
scn MyScript
begin TestLong
  return NoArgFunc
end
)script";

    se.compile(script);
    const auto result{se.call<int>("MyScript", "TestLong")};
    REQUIRE(result);
    REQUIRE(*result == 10);
  }

  {
    std::string_view script = R"script(
scn MyScript
begin TestLong
  MemoryFunc
  return 0
end
)script";

    se.compile(script);
    auto start = ::MemoryFunc();
    const auto result{se.call<int>("MyScript", "TestLong")};
    REQUIRE(result);
    // Called twice after start; once in the script, and once to check the value
    REQUIRE(::MemoryFunc() == start + 2);
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