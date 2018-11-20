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

/// `StrEqeq <- "=="`
struct StrEqeq : pegtl::string<'=', '='> {};

/// `StrNeq <- "!="`
struct StrNeq : pegtl::string<'!', '='> {};

/// `StrLteq <- "<="`
struct StrLteq : pegtl::string<'<', '='> {};

/// `StrGteq <- ">="`
struct StrGteq : pegtl::string<'>', '='> {};

/// `StrLt <- "<"`
struct StrLt : pegtl::one<'<'> {};

/// `StrGt <- ">"`
struct StrGt : pegtl::one<'>'> {};

/// `StrAnd <- "&&"`
struct StrAnd : pegtl::string<'&', '&'> {};

/// `StrOr <- "||"`
struct StrOr : pegtl::string<'|', '|'> {};

/// `StrLparen <- "("`
struct StrLparen : pegtl::one<'('> {};

/// `StrRparen <- ")"`
struct StrRparen : pegtl::one<')'> {};

/// `StrLbrack <- "["`
struct StrLbrack : pegtl::one<'['> {};

/// `StrRbrack <- "]"`
struct StrRbrack : pegtl::one<']'> {};

/// `StrPlus <- "+"`
struct StrPlus : pegtl::one<'+'> {};

/// `StrStar <- "*"`
struct StrStar : pegtl::one<'*'> {};

/// `StrDash <- "-"`
struct StrDash : pegtl::one<'-'> {};

/// `StrSlash <- "/"`
struct StrSlash : pegtl::one<'/'> {};

/// `StrDot <- "."`
struct StrDot : pegtl::one<'.'> {};

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

/// `Eqeq <- StrEqeq Spacing`
struct Eqeq : impl::Spaced<StrEqeq> {};

/// `Neq <- StrNeq Spacing`
struct Neq : impl::Spaced<StrNeq> {};

/// `Lteq <- StrLteq Spacing`
struct Lteq : impl::Spaced<StrLteq> {};

/// `Gteq <- StrGteq Spacing`
struct Gteq : impl::Spaced<StrGteq> {};

/// `Lt <- StrLt Spacing`
struct Lt : impl::Spaced<StrLt> {};

/// `Gt <- StrGt Spacing`
struct Gt : impl::Spaced<StrGt> {};

/// `And <- StrAnd Spacing`
struct And : impl::Spaced<StrAnd> {};

/// `Or <- StrOr Spacing`
struct Or : impl::Spaced<StrOr> {};

/// `Lparen <- StrLparen Spacing`
struct Lparen : impl::Spaced<StrLparen> {};

/// `Rparen <- StrRparen Spacing`
struct Rparen : impl::Spaced<StrRparen> {};

/// `Lbrack <- StrLbrack Spacing`
struct Lbrack : impl::Spaced<StrLbrack> {};

/// `Rbrack <- StrRbrack Spacing`
struct Rbrack : impl::Spaced<StrRbrack> {};

/// `Plus <- StrPlus Spacing`
struct Plus : impl::Spaced<StrPlus> {};

/// `Star <- StrStar Spacing`
struct Star : impl::Spaced<StrStar> {};

/// `Dash <- StrDash Spacing`
struct Dash : impl::Spaced<StrDash> {};

/// `Slash <- StrSlash Spacing`
struct Slash : impl::Spaced<StrSlash> {};

/// `Dot <- StrDot Spacing`
struct Dot : impl::Spaced<StrDot> {};

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

/// `RawLiteral <- StringLiteral / IntegerLiteral / FloatLiteral / RefLiteral`
struct RawLiteral : pegtl::sor<StringLiteral,
                               FloatLiteral,
                               IntegerLiteral,
                               RefLiteral> {
};

/// `Literal <- RawLiteral Spacing`
struct Literal : pegtl::seq<RawLiteral, Spacing> {};

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

/// `MemberAccess <- (RefLiteral / Identifier) "."`
struct MemberAccess : pegtl::seq<pegtl::sor<RefLiteral, Identifier>,
                                 pegtl::one<'.'>> {
};

/// `Variable <- Identifier / (MemberAccess Identifier)`
struct Variable : pegtl::sor<Identifier,
                             pegtl::seq<MemberAccess, Identifier>> {
};

struct Expression;

/// `PrimaryExpression <- Variable / Literal / (LParen Expression RParen)`
struct PrimaryExpression : pegtl::sor<Variable,
                                      Literal,
                                      pegtl::seq<Lparen,
                                                 Expression,
                                                 Rparen>> {
};

/// `UnaryExpression <- [+-]? PrimaryExpression`
struct UnaryExpression : pegtl::seq<pegtl::opt<pegtl::sor<StrPlus, StrDash>>,
                                    PrimaryExpression> {
};

/// `MulExpression <- UnaryExpression ((Star / Slash) UnaryExpression)*`
struct MulExpression : pegtl::seq<UnaryExpression,
                                  pegtl::star<
                                      pegtl::seq<pegtl::sor<Star, Slash>,
                                                 UnaryExpression>>> {
};

/// `AddExpression <- MulExpression ((Plus / Dash) MulExpression)*`
struct AddExpression : pegtl::seq<MulExpression,
                                  pegtl::star<
                                      pegtl::seq<pegtl::sor<Plus, Dash>,
                                                 UnaryExpression>>> {
};

/// `CondExpression <- AddExpression ((Lteq / Gteq / Lt / Gt) AddExpression)*`
struct CondExpression : pegtl::seq<AddExpression,
                                   pegtl::star<
                                       pegtl::seq<pegtl::sor<Lteq,
                                                             Gteq,
                                                             Lt,
                                                             Gt>,
                                                  AddExpression>>> {
};

/// `EqExpression <- CondExpression ((Eqeq / Neq) CondExpression)*`
struct EqExpression : pegtl::seq<CondExpression,
                                 pegtl::star<
                                     pegtl::seq<pegtl::sor<Eqeq, Neq>,
                                                CondExpression>>> {
};

/// `AndExpression <- EqExpression (And EqExpression)*`
/// \remark DR3 has been applied.
struct AndExpression : pegtl::seq<EqExpression,
                                  pegtl::star<pegtl::seq<And, EqExpression>>> {
};

/// `OrExpression <- AndExpression (Or AndExpression)*`
/// \remark DR3 has been applied.
struct OrExpression : pegtl::seq<AndExpression,
                                 pegtl::star<pegtl::seq<Or, AndExpression>>> {
};

struct Expression : OrExpression {};

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

/// Rearrange expression nodes into operator nodes.
/// \remark Adapted from the parse_tree.cpp example in PEGTL.
struct ExpressionRearranger : std::true_type {
  static void transform(std::unique_ptr<pegtl::parse_tree::node> &node) {
    if (node == nullptr || node->children.empty()) return;

    // Only one child, so replace the parent with the child.
    if (node->children.size() == 1) {
      node = std::move(node->children.back());
      return;
    }

    node->remove_content();
    auto &children{node->children};

    auto rhs{std::move(children.back())};
    children.pop_back();

    auto op{std::move(children.back())};
    children.pop_back();

    // Node had more than two children so is not a unary expression. The lhs
    // will therefore be parsed and should be kept.
    if (!node->children.empty()) {
      op->children.emplace_back(std::move(node));
    }

    op->children.emplace_back(std::move(rhs));
    node = std::move(op);
    transform(node->children.front());
  }
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
template<> struct AstSelector<MemberAccess> : std::true_type {};
template<> struct AstSelector<Variable> : std::true_type {};
template<> struct AstSelector<StrPlus> : std::true_type {};
template<> struct AstSelector<StrDash> : std::true_type {};
template<> struct AstSelector<StrStar> : std::true_type {};
template<> struct AstSelector<StrSlash> : std::true_type {};
template<> struct AstSelector<StrLteq> : std::true_type {};
template<> struct AstSelector<StrGteq> : std::true_type {};
template<> struct AstSelector<StrLt> : std::true_type {};
template<> struct AstSelector<StrGt> : std::true_type {};
template<> struct AstSelector<StrEqeq> : std::true_type {};
template<> struct AstSelector<StrNeq> : std::true_type {};
template<> struct AstSelector<StrAnd> : std::true_type {};
template<> struct AstSelector<StrOr> : std::true_type {};
template<> struct AstSelector<PrimaryExpression> : ExpressionRearranger {};
template<> struct AstSelector<UnaryExpression> : ExpressionRearranger {};
template<> struct AstSelector<MulExpression> : ExpressionRearranger {};
template<> struct AstSelector<AddExpression> : ExpressionRearranger {};
template<> struct AstSelector<CondExpression> : ExpressionRearranger {};
template<> struct AstSelector<EqExpression> : ExpressionRearranger {};
template<> struct AstSelector<AndExpression> : ExpressionRearranger {};
template<> struct AstSelector<OrExpression> : ExpressionRearranger {};
template<> struct AstSelector<Expression> : ExpressionRearranger {};

template<class F, class State>
void visitAst(const pegtl::parse_tree::node &node, State state, F &&visitor) {
  state = visitor(node, state);
  for (const auto &child : node.children) visitAst(*child, state, visitor);
}

} // namespace scripting

/// @}

#endif // OPENOBLIVION_SCRIPTING_GRAMMAR_HPP
