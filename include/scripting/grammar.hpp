#ifndef OPENOBLIVION_SCRIPTING_GRAMMAR_HPP
#define OPENOBLIVION_SCRIPTING_GRAMMAR_HPP

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <string>
#include <string_view>
#include <variant>

namespace scripting {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

/// \name Strings
/// Literal strings used for constructing keywords, types, etc.
///@{
struct StrScriptname : pegtl::string<'s', 'c', 'r', 'i', 'p', 't',
                                     'n', 'a', 'm', 'e'> {
};
struct StrScn : pegtl::string<'s', 'c', 'n'> {};
struct StrSemicolon : pegtl::one<';'> {};
///@}

/// \name Character classes
///@{

/// `IdChar <- [a-zA-Z0-9]`
struct IdChar : pegtl::alnum {};

/// `InitialIdChar <- [a-zA-Z]`
struct InitialIdChar : pegtl::alpha {};

///@}

/// \name Spacing
///@{
/// `Comment <- ";" (!EndOfLine .)* EndOfLine`
struct Comment : pegtl::seq<StrSemicolon,
                            pegtl::star<pegtl::seq<pegtl::not_at<pegtl::eolf>,
                                                   pegtl::any>>,
                            pegtl::eolf> {
};
/// `Spacing <- (Space / Comment)*`
struct Spacing : pegtl::star<pegtl::sor<pegtl::space, Comment>> {};
///@}

/// \name Keywords
/// @{

namespace impl {

/// Convenience class template for allowing trailing space.
/// Keywords, types, and punctuation may all have trailing space, but we do not
/// want the space to appear in the AST. Thus each of them should have
template<class Raw>
struct Spaced : pegtl::seq<Raw, Spacing> {};

}

/// SCRIPTNAME <- "scriptname"
struct ScriptnameLong : StrScriptname {};

/// SCN <- "scn"
struct ScriptnameShort : StrScn {};

/// @}

struct RawScriptname : pegtl::sor<ScriptnameLong, ScriptnameShort> {};
struct RawIdentifier : pegtl::seq<InitialIdChar, pegtl::star<IdChar>> {};

struct Scriptname : impl::Spaced<RawScriptname> {};
struct Identifier : impl::Spaced<RawIdentifier> {};

struct RawScriptnameStatement : pegtl::seq<Scriptname, Identifier> {};
struct ScriptnameStatement : pegtl::seq<RawScriptnameStatement,
                                        pegtl::sor<Spacing, pegtl::eolf>> {
};

struct Grammar : pegtl::must<Spacing,
                             ScriptnameStatement,
    //pegtl::star<Block>,
                             pegtl::eof> {
};

template<class Rule> struct AstSelector : std::false_type {};

template<> struct AstSelector<RawScriptnameStatement> : std::true_type {};
template<> struct AstSelector<RawScriptname> : std::true_type {};
template<> struct AstSelector<RawIdentifier> : std::true_type {};

template<class F, class State>
void visitAst(const pegtl::parse_tree::node &node, State state, F &&visitor) {
  state = visitor(node, state);
  for (const auto &child : node.children) visitAst(*child, state, visitor);
}

} // namespace scripting

#endif // OPENOBLIVION_SCRIPTING_GRAMMAR_HPP
