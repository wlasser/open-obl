#ifndef OPENOBLIVION_TEST_SCRIPTING_HELPERS_HPP
#define OPENOBLIVION_TEST_SCRIPTING_HELPERS_HPP

#include "record/formid.hpp"
#include "scripting/ast.hpp"
#include "scripting/script_engine.hpp"
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <string>
#include <string_view>

extern "C" __attribute__((visibility("default"), used)) int Func(int x);
extern "C" __attribute__((visibility("default"), used)) int ConsoleFunc(int x);
extern "C" __attribute__((visibility("default"), used)) int MemberFunc(uint32_t mem,
                                                                       int x);
extern "C" __attribute__((visibility("default"), used)) int NoArgFunc();

namespace oo {

[[nodiscard]] bool isStatement(const AstNode &node);

void printNode(const AstNode &node);

void printAstImpl(const AstNode &node, const std::string &indent);

void printAst(const AstNode &node);

[[nodiscard]] bool isString(const AstNode &node, std::string_view expected);

[[nodiscard]] bool isInteger(const AstNode &node, int expected);

[[nodiscard]] bool isReference(const AstNode &node, FormId expected);

[[nodiscard]] bool isFloat(const AstNode &node, float expected);

template<class T>
[[nodiscard]] bool isVariable(const AstNode &node, std::string_view name) {
  return node.children.size() == 2
      && node.children[0]->is<T>()
      && node.children[1]->has_content()
      && node.children[1]->content() == name;
}

[[nodiscard]] oo::ScriptEngine &getScriptEngine();

} // namespace oo

#endif // OPENOBLIVION_TEST_SCRIPTING_HELPERS_HPP
