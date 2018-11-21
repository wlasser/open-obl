#ifndef OPENOBLIVION_TEST_SCRIPTING_HELPERS_HPP
#define OPENOBLIVION_TEST_SCRIPTING_HELPERS_HPP

#include "record/formid.hpp"
#include "scripting/grammar.hpp"
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

[[nodiscard]] bool
isString(const pegtl::parse_tree::node &node, std::string_view expected);

[[nodiscard]] bool
isInteger(const pegtl::parse_tree::node &node, int expected);

[[nodiscard]] bool
isReference(const pegtl::parse_tree::node &node, FormId expected);

[[nodiscard]] bool
isFloat(const pegtl::parse_tree::node &node, float expected);

template<class T>
[[nodiscard]] bool isVariable(const pegtl::parse_tree::node &node,
                              std::string_view name) {
  return node.children.size() == 2
      && node.children[0]->is<T>()
      && node.children[1]->has_content()
      && node.children[1]->content() == name;
}

} // namespace scripting

#endif // OPENOBLIVION_TEST_SCRIPTING_HELPERS_HPP
