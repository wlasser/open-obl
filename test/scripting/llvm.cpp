#include "helpers.hpp"
#include "scripting/grammar.hpp"
#include "scripting/llvm.hpp"
#include <catch2/catch.hpp>

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

TEST_CASE("can use llvm", "[scripting]") {
  const auto *script = R"script(
scn MyScript

short smallInt

begin TestLong
  set smallInt to 360

  long myVar
  set myVar to 512 && 10

  float foo
  set foo to 3.15 + 10

  long aLong
  set aLong to foo + myVar + 10 * smallInt

  return aLong
end
  )script";

  pegtl::memory_input in(script, "");
  const auto root = scripting::parseScript(in);
  REQUIRE(root != nullptr);
  scripting::printAst(*root);
  scripting::LLVMVisitor visitor("MyScript");
  visitor.visit(*root);
  visitor.print();
}