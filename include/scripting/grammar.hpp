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

/// `StrScriptname <- "scriptname"`
struct StrScriptname : pegtl::string<'s', 'c', 'r', 'i', 'p', 't',
                                     'n', 'a', 'm', 'e'> {
};

/// `StrScn <- "scn"`
struct StrScn : pegtl::string<'s', 'c', 'n'> {};

/// `StrSemicolon <- ";"`
struct StrSemicolon : pegtl::one<';'> {};

/// `StrBegin <- "begin"`
struct StrBegin : pegtl::string<'b', 'e', 'g', 'i', 'n'> {};

/// `StrEnd <- "end"`
struct StrEnd : pegtl::string<'e', 'n', 'd'> {};

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

namespace impl {

/// Convenience class template for allowing trailing space.
/// Keywords, types, and punctuation may all have trailing space, but we do not
/// want the space to appear in the AST.
template<class Raw>
struct Spaced : pegtl::seq<Raw, Spacing> {};

}

/// \name Raw keywords
///@{

/// `ScriptnameLong <- StrScriptname`
struct ScriptnameLong : StrScriptname {};

/// `ScriptnameShort <- StrScn`
struct ScriptnameShort : StrScn {};

/// `RawScriptname <- ScriptnameLong / ScriptnameShort`
struct RawScriptname : pegtl::sor<ScriptnameLong, ScriptnameShort> {};

/// `RawBegin <- StrBegin`
struct RawBegin : StrBegin {};

/// `RawEnd <- StrEnd`
struct RawEnd : StrEnd {};

///@}

/// \name Spaced keywords
///@{

/// `Scriptname <- RawScriptname Spacing`
struct Scriptname : impl::Spaced<RawScriptname> {};

/// `Begin <- RawBegin Spacing`
struct Begin : impl::Spaced<RawBegin> {};

/// `End <- RawEnd Spacing`
struct End : impl::Spaced<RawEnd> {};

///@}

/// \name Literals
///@{

/// `StringLiteralBannedChar <- ["] / eolf`
struct StringLiteralBannedChar : pegtl::sor<pegtl::one<'"'>, pegtl::eolf> {};

/// `StringLiteralContents <- (!StringLiteralBannedChar .)*`
struct StringLiteralContents : pegtl::star<
    pegtl::seq<pegtl::not_at<StringLiteralBannedChar>, pegtl::any>> {
};

/// `StringLiteral <- ["] / StringLiteralContents / ["]
/// \remark Unlike in most languages, there are no escape sequences in strings.
///         For example, `\\t` is a literal backslash followed by a `t`, not a
///         tab. In particular, string literals cannot contain double quotes
///         directly.
struct StringLiteral : pegtl::seq<pegtl::one<'"'>,
                                  StringLiteralContents,
                                  pegtl::one<'"'>> {
};

///@}

/// `RawIdentifier <- InitialIdChar IdChar*
struct RawIdentifier : pegtl::seq<InitialIdChar, pegtl::star<IdChar>> {};
/// `Identifier <- RawIdentifier Spacing`
struct Identifier : impl::Spaced<RawIdentifier> {};

/// \name Statements
///@{

/// `RawScriptnameStatement <- Scriptname Identifier`
struct RawScriptnameStatement : pegtl::seq<Scriptname, Identifier> {};

/// `ScriptnameStatement <- RawScriptnameStatement (Spacing / eolf)`
struct ScriptnameStatement : pegtl::seq<RawScriptnameStatement,
                                        pegtl::sor<Spacing, pegtl::eolf>> {
};

struct BlockBeginStatement : pegtl::seq<Begin,
                                        Identifier
    /* [IntegerLiteral]? */> {
};

struct BlockEndStatement : End {};

struct BlockStatement : pegtl::seq<BlockBeginStatement,
    /* (Statement)* */
                                   BlockEndStatement> {
};

///@}

struct Grammar : pegtl::must<Spacing,
                             ScriptnameStatement,
                             pegtl::star<BlockStatement>,
                             pegtl::eof> {
};

template<class Rule> struct AstSelector : std::false_type {};

template<> struct AstSelector<RawScriptnameStatement> : std::true_type {};
template<> struct AstSelector<RawScriptname> : std::true_type {};
template<> struct AstSelector<RawIdentifier> : std::true_type {};
template<> struct AstSelector<BlockBeginStatement> : std::true_type {};
template<> struct AstSelector<BlockEndStatement> : std::true_type {};
template<> struct AstSelector<StringLiteralContents> : std::true_type {};

template<class F, class State>
void visitAst(const pegtl::parse_tree::node &node, State state, F &&visitor) {
  state = visitor(node, state);
  for (const auto &child : node.children) visitAst(*child, state, visitor);
}

} // namespace scripting

#endif // OPENOBLIVION_SCRIPTING_GRAMMAR_HPP
