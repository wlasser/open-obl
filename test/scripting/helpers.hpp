#ifndef OPENOBLIVION_TEST_SCRIPTING_HELPERS_HPP
#define OPENOBLIVION_TEST_SCRIPTING_HELPERS_HPP

#include "record/formid.hpp"
#include "scripting/grammar.hpp"
#include <catch2/catch.hpp>
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <string>

namespace scripting {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

[[nodiscard]] bool isStatement(const pegtl::parse_tree::node &node);

[[nodiscard]] std::string
printVisitor(const pegtl::parse_tree::node &node, const std::string &indent);

void printAst(const pegtl::parse_tree::node &node);

template<class T> [[nodiscard]] auto parseScript(T &&in) {
  return pegtl::parse_tree::parse<scripting::Grammar,
                                  scripting::AstSelector>(in);
}

void requireIsString(const pegtl::parse_tree::node &node,
                     const std::string &expected);

void requireIsInteger(const pegtl::parse_tree::node &node, int expected);

void requireIsReference(const pegtl::parse_tree::node &node, FormId expected);

void requireIsFloat(const pegtl::parse_tree::node &node, float expected);

template<class T>
void requireHasVariable(const pegtl::parse_tree::node &root,
                        const std::string &name) {
  REQUIRE(root.children.size() == 2);
  REQUIRE(root.children[0]->is<T>());
  REQUIRE(root.children[1]->has_content());
  REQUIRE(root.children[1]->content() == name);
}

} // namespace scripting

#endif // OPENOBLIVION_TEST_SCRIPTING_HELPERS_HPP
