#include "helpers.hpp"
#include "scripting/llvm.hpp"
#include "scripting/pegtl.hpp"
#include <catch2/catch.hpp>

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

TEST_CASE("can use llvm", "[scripting]") {
  const auto *script = R"script(
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
      return -1
    else
      return 10
    endif
  else
    return 2
  endif
end
  )script";

  pegtl::memory_input in(script, "");
  const auto root = oo::parseScript(in);
  REQUIRE(root != nullptr);
  oo::printAst(*root);
  oo::LLVMVisitor visitor("MyScript");
  visitor.visit(*root);
  visitor.print();
}