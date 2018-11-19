#ifndef OPENOBLIVION_SCRIPTING_GRAMMAR_HPP
#define OPENOBLIVION_SCRIPTING_GRAMMAR_HPP

#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <string>
#include <string_view>
#include <variant>

/// \addtogroup OpenOblivionScripting Scripting
/// Parser for the built-in default scripting language.
/// @{
namespace scripting {

namespace pegtl = tao::TAO_PEGTL_NAMESPACE;

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

/// `StrShort <- "short"`
struct StrShort : pegtl::string<'s', 'h', 'o', 'r', 't'> {};

/// `StrLong <- "long"`
struct StrLong : pegtl::string<'l', 'o', 'n', 'g'> {};

/// `StrFloat <- "float"`
struct StrFloat : pegtl::string<'f', 'l', 'o', 'a', 't'> {};

/// `StrRef <- "ref"`
struct StrRef : pegtl::string<'r', 'e', 'f'> {};

/// `IdChar <- [a-zA-Z0-9]`
struct IdChar : pegtl::alnum {};

/// `InitialIdChar <- [a-zA-Z]`
struct InitialIdChar : pegtl::alpha {};

/// `Comment <- ";" (!EndOfLine .)* EndOfLine`
struct Comment : pegtl::seq<StrSemicolon,
                            pegtl::star<pegtl::seq<pegtl::not_at<pegtl::eolf>,
                                                   pegtl::any>>,
                            pegtl::eolf> {
};

/// `Spacing <- (Space / Comment)*`
struct Spacing : pegtl::star<pegtl::sor<pegtl::space, Comment>> {};

namespace impl {

/// Convenience class template for allowing trailing space.
/// Keywords, types, and punctuation may all have trailing space, but we do not
/// want the space to appear in the AST.
template<class Raw>
struct Spaced : pegtl::seq<Raw, Spacing> {};

}

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

/// `RawShort <- StrShort`
struct RawShort : StrShort {};

/// `RawLong <- StrLong`
struct RawLong : StrLong {};

/// `RawFloat <- StrFloat`
struct RawFloat : StrFloat {};

/// `RawRef <- StrRef`
struct RawRef : StrRef {};

/// `Scriptname <- RawScriptname Spacing`
struct Scriptname : impl::Spaced<RawScriptname> {};

/// `Begin <- RawBegin Spacing`
struct Begin : impl::Spaced<RawBegin> {};

/// `End <- RawEnd Spacing`
struct End : impl::Spaced<RawEnd> {};

/// `StringLiteralBannedChar <- ["] / eolf`
struct StringLiteralBannedChar : pegtl::sor<pegtl::one<'"'>, pegtl::eolf> {};

/// `StringLiteralContents <- (!StringLiteralBannedChar .)*`
struct StringLiteralContents : pegtl::star<
    pegtl::seq<pegtl::not_at<StringLiteralBannedChar>, pegtl::any>> {
};

/// `StringLiteral <- ["] / StringLiteralContents / ["]`
/// \remark Unlike in most languages, there are no escape sequences in strings.
///         For example, `\\t` is a literal backslash followed by a `t`, not a
///         tab. In particular, string literals cannot contain double quotes
///         directly.
struct StringLiteral : pegtl::seq<pegtl::one<'"'>,
                                  StringLiteralContents,
                                  pegtl::one<'"'>> {
};

/// `IntegerLiteral <- "0" / ([1-9] [0-9]*)`
struct IntegerLiteral : pegtl::sor<pegtl::one<'0'>,
                                   pegtl::seq<pegtl::range<'1', '9'>,
                                              pegtl::star<pegtl::digit>>> {

};

/// `RefLiteralPrefix <- "#"`
struct RefLiteralPrefix : pegtl::one<'#'> {};

/// `RefLiteralContents [0-9a-fA-F]+`
struct RefLiteralContents : pegtl::plus<pegtl::xdigit> {};

/// `RefLiteral <- RefLiteralPrefix RefLiteralContents`
struct RefLiteral : pegtl::seq<RefLiteralPrefix, RefLiteralContents> {};

/// `FloatLiteralSupUnity <- [1-9] [0-9]* "." [0-9]*`
struct FloatLiteralSupUnity : pegtl::seq<pegtl::range<'1', '9'>,
                                         pegtl::star<pegtl::digit>,
                                         pegtl::one<'.'>,
                                         pegtl::star<pegtl::digit>> {
};

/// `FloatLiteralSubUnity <- "0." [0-9]*`
struct FloatLiteralSubUnity : pegtl::seq<pegtl::string<'0', '.'>,
                                         pegtl::star<pegtl::digit>> {
};

/// `FloatLiteralLeadingPoint <- "." [0-9]+`
struct FloatLiteralLeadingPoint : pegtl::seq<pegtl::one<'.'>,
                                             pegtl::plus<pegtl::digit>> {
};

/// `FloatLiteral <- FloatLiteralSupUnity / FloatLiteralSubUnity
///                  / FloatLiteralLeadingPoint`
struct FloatLiteral : pegtl::sor<FloatLiteralSupUnity,
                                 FloatLiteralSubUnity,
                                 FloatLiteralLeadingPoint> {
};

/// `Literal <- StringLiteral / IntegerLiteral / FloatLiteral / RefLiteral`
struct Literal : pegtl::sor<StringLiteral,
                            IntegerLiteral,
                            FloatLiteral,
                            RefLiteral> {
};

/// `RawIdentifier <- InitialIdChar IdChar*
struct RawIdentifier : pegtl::seq<InitialIdChar, pegtl::star<IdChar>> {};
/// `Identifier <- RawIdentifier Spacing`
struct Identifier : impl::Spaced<RawIdentifier> {};

/// `ShortType <- RawShort`
struct ShortType : impl::Spaced<RawShort> {};

/// `LongType <- RawLong`
struct LongType : impl::Spaced<RawLong> {};

/// `FloatType <- RawFloat`
struct FloatType : impl::Spaced<RawFloat> {};

/// `RefType <- RawRef`
struct RefType : impl::Spaced<RawRef> {};

/// `Type <- ShortType / LongType / FloatType / RefType`
struct Type : pegtl::sor<ShortType, LongType, FloatType, RefType> {};

/// `RawScriptnameStatement <- Scriptname Identifier`
struct RawScriptnameStatement : pegtl::seq<Scriptname, Identifier> {};

/// `ScriptnameStatement <- RawScriptnameStatement (Spacing / eolf)`
struct ScriptnameStatement : pegtl::seq<RawScriptnameStatement,
                                        pegtl::sor<Spacing, pegtl::eolf>> {
};

/// `DeclarationStatement <- Type Identifier`
struct DeclarationStatement : pegtl::seq<Type, Identifier> {};

struct BlockBeginStatement : pegtl::seq<Begin,
                                        Identifier,
                                        pegtl::opt<IntegerLiteral>,
                                        Spacing> {
};

struct BlockEndStatement : End {};

struct BlockStatement : pegtl::seq<BlockBeginStatement,
                                   pegtl::star<DeclarationStatement>,
                                   BlockEndStatement> {
};

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
template<> struct AstSelector<IntegerLiteral> : std::true_type {};
template<> struct AstSelector<RefLiteralContents> : std::true_type {};
template<> struct AstSelector<FloatLiteral> : std::true_type {};
template<> struct AstSelector<DeclarationStatement> : std::true_type {};
template<> struct AstSelector<RawShort> : std::true_type {};
template<> struct AstSelector<RawLong> : std::true_type {};
template<> struct AstSelector<RawFloat> : std::true_type {};
template<> struct AstSelector<RawRef> : std::true_type {};

template<class F, class State>
void visitAst(const pegtl::parse_tree::node &node, State state, F &&visitor) {
  state = visitor(node, state);
  for (const auto &child : node.children) visitAst(*child, state, visitor);
}

} // namespace scripting

/// @}

#endif // OPENOBLIVION_SCRIPTING_GRAMMAR_HPP
