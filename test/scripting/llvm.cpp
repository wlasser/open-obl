#include "helpers.hpp"
#include "scripting/script_engine.hpp"
#include "scripting/llvm.hpp"
#include "scripting/pegtl.hpp"
#include <catch2/catch.hpp>
#include <string_view>

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

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

  oo::ScriptEngine se{};
  se.compile(script);
  llvm::errs() << se.call<int>("TestLong") << '\n';
}