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

void requireHasString(const pegtl::parse_tree::node &root,
                      const std::string &expected);

void requireHasInteger(const pegtl::parse_tree::node &root, int expected);

void requireHasReference(const pegtl::parse_tree::node &root, FormId expected);

void requireHasFloat(const pegtl::parse_tree::node &root, float expected);

} // namespace scripting

#endif // OPENOBLIVION_TEST_SCRIPTING_HELPERS_HPP
