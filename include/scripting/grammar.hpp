#ifndef OPENOBLIVION_SCRIPTING_GRAMMAR_HPP
#define OPENOBLIVION_SCRIPTING_GRAMMAR_HPP

#include "scripting/pegtl.hpp"

/// \addtogroup OpenOblivionScripting Scripting
/// Parser for the built-in default scripting language.
/// This document describes the parsing and execution method for user-defined
/// scripts. For information on the scripting language itself, see scripting.md.
/// @{
namespace oo::grammar {

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

/// `StrSet <- "set"`
struct StrSet : pegtl::string<'s', 'e', 't'> {};

/// `StrTo <- "to"`
struct StrTo : pegtl::string<'t', 'o'> {};

/// `StrReturn <- "return"`
struct StrReturn : pegtl::string<'r', 'e', 't', 'u', 'r', 'n'> {};

/// `StrIf <- "if"`
struct StrIf : pegtl::string<'i', 'f'> {};

/// `StrElseif <- "elseif"`
struct StrElseIf : pegtl::string<'e', 'l', 's', 'e', 'i', 'f'> {};

/// `StrElse <- "else"`
struct StrElse : pegtl::string<'e', 'l', 's', 'e'> {};

/// `StrEndif <- "endif"`
struct StrEndif : pegtl::string<'e', 'n', 'd', 'i', 'f'> {};

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

/// Convenience class template for allowing trailing space.
/// Keywords, types, and punctuation may all have trailing space, but we do not
/// want the space to appear in the AST.
template<class Raw>
struct Spaced : pegtl::seq<Raw, Spacing> {};

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
struct Scriptname : Spaced<RawScriptname> {};

/// `Begin <- RawBegin Spacing`
struct Begin : Spaced<RawBegin> {};

/// `End <- RawEnd Spacing`
struct End : Spaced<RawEnd> {};

/// `Eqeq <- StrEqeq Spacing`
struct Eqeq : Spaced<StrEqeq> {};

/// `Neq <- StrNeq Spacing`
struct Neq : Spaced<StrNeq> {};

/// `Lteq <- StrLteq Spacing`
struct Lteq : Spaced<StrLteq> {};

/// `Gteq <- StrGteq Spacing`
struct Gteq : Spaced<StrGteq> {};

/// `Lt <- StrLt Spacing`
struct Lt : Spaced<StrLt> {};

/// `Gt <- StrGt Spacing`
struct Gt : Spaced<StrGt> {};

/// `And <- StrAnd Spacing`
struct And : Spaced<StrAnd> {};

/// `Or <- StrOr Spacing`
struct Or : Spaced<StrOr> {};

/// `Lparen <- StrLparen Spacing`
struct Lparen : Spaced<StrLparen> {};

/// `Rparen <- StrRparen Spacing`
struct Rparen : Spaced<StrRparen> {};

/// `Lbrack <- StrLbrack Spacing`
struct Lbrack : Spaced<StrLbrack> {};

/// `Rbrack <- StrRbrack Spacing`
struct Rbrack : Spaced<StrRbrack> {};

/// `Plus <- StrPlus Spacing`
struct Plus : Spaced<StrPlus> {};

/// `Star <- StrStar Spacing`
struct Star : Spaced<StrStar> {};

/// `Dash <- StrDash Spacing`
struct Dash : Spaced<StrDash> {};

/// `Slash <- StrSlash Spacing`
struct Slash : Spaced<StrSlash> {};

/// `Dot <- StrDot Spacing`
struct Dot : Spaced<StrDot> {};

/// `Set <- StrSet Spacing`
struct Set : Spaced<StrSet> {};

/// `To <- StrTo Spacing`
struct To : Spaced<StrTo> {};

/// `Return <- StrReturn Spacing`
struct Return : Spaced<StrReturn> {};

/// `If <- StrIf Spacing`
struct If : Spaced<StrIf> {};

/// `Elseif <- StrElseif Spacing`
struct Elseif : Spaced<StrElseIf> {};

/// `Else <- StrElse Spacing`
struct Else : Spaced<StrElse> {};

/// `Endif <- StrEndif Spacing`
struct Endif : Spaced<StrEndif> {};

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

/// `RawIdentifier <- InitialIdChar IdChar*`
struct RawIdentifier : pegtl::seq<InitialIdChar, pegtl::star<IdChar>> {};

/// `Identifier <- RawIdentifier Spacing`
struct Identifier : Spaced<RawIdentifier> {};

/// `ShortType <- RawShort`
struct ShortType : Spaced<RawShort> {};

/// `LongType <- RawLong`
struct LongType : Spaced<RawLong> {};

/// `FloatType <- RawFloat`
struct FloatType : Spaced<RawFloat> {};

/// `RefType <- RawRef`
struct RefType : Spaced<RawRef> {};

/// `Type <- ShortType / LongType / FloatType / RefType`
struct Type : pegtl::sor<ShortType, LongType, FloatType, RefType> {};

/// `RawMemberAccess <- (RefLiteral / Identifier) "." RawIdentifier`
struct RawMemberAccess : pegtl::seq<pegtl::sor<RefLiteral, Identifier>,
                                    pegtl::one<'.'>,
                                    RawIdentifier> {
};

/// `MemberAccess <- RawMemberAccess Spacing`
struct MemberAccess : pegtl::seq<RawMemberAccess, Spacing> {};

/// `Variable <- MemberAccess / Identifier`
struct Variable : pegtl::sor<MemberAccess, Identifier> {};

/// `RawArgument <- RawMemberAccess / RawVariable / RawLiteral`
struct RawArgument : pegtl::sor<RawMemberAccess, RawIdentifier, RawLiteral> {};

/// `RawMemberCall <- RawMemberAccess (blank+ RawArgument)*`
struct RawMemberCall : pegtl::seq<RawMemberAccess,
                                  pegtl::plus<pegtl::plus<pegtl::blank>,
                                              RawArgument>> {
};

/// `RawFreeCall <- RawIdentifier (blank+ RawArgument)*`
struct RawFreeCall : pegtl::seq<RawIdentifier,
                                pegtl::plus<pegtl::plus<pegtl::blank>,
                                            RawArgument>> {
};

/// `RawCall <- RawMemberCall / RawFreeCall`
struct RawCall : pegtl::sor<RawMemberCall, RawFreeCall> {};

/// `Call <- RawCall Spacing`
struct Call : pegtl::seq<RawCall, Spacing> {};

struct Expression;

/// `PrimaryExpression <- Variable / Literal / (LParen Expression RParen)`
struct PrimaryExpression : pegtl::sor<Literal,
                                      Call,
                                      Variable,
                                      pegtl::seq<Lparen,
                                                 Expression,
                                                 Rparen>> {
};

/// `UnaryOperator <- StrPlus / StrDash`
struct UnaryOperator : pegtl::sor<StrPlus, StrDash> {};

/// `MultiplicativeBinaryOperator <- Star / Slash`
struct MultiplicativeBinaryOperator : pegtl::sor<Star, Slash> {};

/// `AdditiveBinaryOperator <- Plus / Dash`
struct AdditiveBinaryOperator : pegtl::sor<Plus, Dash> {};

/// `ConditionalBinaryOperator <- Lteq / Gteq / Lt / Gt`
struct ConditionalBinaryOperator : pegtl::sor<Lteq, Gteq, Lt, Gt> {};

/// `EqualityBinaryOperator <- Eqeq / Neq`
struct EqualityBinaryOperator : pegtl::sor<Eqeq, Neq> {};

/// `ConjunctionBinaryOperator <- And`
struct ConjunctionBinaryOperator : And {};

/// `DisjunctionBinaryOperator <- Or`
struct DisjunctionBinaryOperator : Or {};

/// Not used in the grammar, used as a more convenient representation of a
/// binary operator in the AST, to mirror UnaryOperator.
/// ```peg
/// BinaryOperator <- MultiplicativeBinaryOperator
///                    / AdditiveBinaryOperator
///                    / ConditionalBinaryOperator
///                    / EqualityBinaryOperator
///                    / ConjunctionBinaryOperator
///                    / DisjunctionBinaryOperator
/// ```
struct BinaryOperator : pegtl::sor<MultiplicativeBinaryOperator,
                                   AdditiveBinaryOperator,
                                   ConditionalBinaryOperator,
                                   EqualityBinaryOperator,
                                   ConjunctionBinaryOperator,
                                   DisjunctionBinaryOperator> {
};

/// `UnaryExpression <- UnaryOperator? PrimaryExpression`
struct UnaryExpression : pegtl::seq<pegtl::opt<UnaryOperator>,
                                    PrimaryExpression> {
};

/// `MulExpression <- UnaryExpression (MultiplicativeBinaryOperator UnaryExpression)*`
struct MulExpression : pegtl::seq<UnaryExpression,
                                  pegtl::star<
                                      pegtl::seq<MultiplicativeBinaryOperator,
                                                 UnaryExpression>>> {
};

/// `AddExpression <- MulExpression (AdditiveBinaryOperator MulExpression)*`
struct AddExpression : pegtl::seq<MulExpression,
                                  pegtl::star<
                                      pegtl::seq<AdditiveBinaryOperator,
                                                 MulExpression>>> {
};

/// `CondExpression <- AddExpression (ConditionalBinaryOperator AddExpression)*`
struct CondExpression : pegtl::seq<AddExpression,
                                   pegtl::star<
                                       pegtl::seq<ConditionalBinaryOperator,
                                                  AddExpression>>> {
};

/// `EqExpression <- CondExpression (EqualityBinaryOperator CondExpression)*`
struct EqExpression : pegtl::seq<CondExpression,
                                 pegtl::star<
                                     pegtl::seq<EqualityBinaryOperator,
                                                CondExpression>>> {
};

/// `AndExpression <- EqExpression (ConjunctionBinaryOperator EqExpression)*`
/// \remark DR3 has been applied.
struct AndExpression : pegtl::seq<EqExpression,
                                  pegtl::star<
                                      pegtl::seq<ConjunctionBinaryOperator,
                                                 EqExpression>>> {
};

/// `OrExpression <- AndExpression (DisjunctionBinaryOperator AndExpression)*`
/// \remark DR3 has been applied.
struct OrExpression : pegtl::seq<AndExpression,
                                 pegtl::star<
                                     pegtl::seq<DisjunctionBinaryOperator,
                                                AndExpression>>> {
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

/// `SetStatement <- Set Variable To Expression`
struct SetStatement : pegtl::seq<Set, Variable, To, Expression> {};

/// `ReturnStatement <- (StrReturn [ \t]+ Expression) / Return`
/// This awkward rule is required in order to match the core `return` with no
/// argugments, as well as a `return expr` for arbitrary expressions without
/// the core `return` stealing whatever is on the next line.
struct ReturnStatement : pegtl::sor<pegtl::seq<StrReturn,
                                               pegtl::plus<pegtl::blank>,
                                               Expression>, Return> {
};

struct Statement;

/// `ElseifStatement <- Elseif Expression Statement+`
struct ElseifStatement : pegtl::seq<Elseif,
                                    Expression,
                                    pegtl::plus<Statement>> {
};

/// `ElseStatement <- Else Statement+`
struct ElseStatement : pegtl::seq<Else, pegtl::plus<Statement>> {};

/// `IfStatement <- If Expression Statement+ ElseifStatement* ElseStatement? Endif`
struct IfStatement : pegtl::seq<If,
                                Expression,
                                pegtl::plus<Statement>,
                                pegtl::star<ElseifStatement>,
                                pegtl::opt<ElseStatement>,
                                Endif> {
};

/// `Statement <- DeclarationStatement / SetStatement / IfStatement / ReturnStatement`
struct Statement : pegtl::sor<DeclarationStatement,
                              SetStatement,
                              IfStatement,
                              ReturnStatement> {
};

struct BlockBeginStatement : pegtl::seq<Begin,
                                        Identifier,
                                        pegtl::opt<IntegerLiteral>,
                                        Spacing> {
};

struct BlockEndStatement : End {};

struct BlockStatement : pegtl::seq<BlockBeginStatement,
                                   pegtl::star<Statement>,
                                   BlockEndStatement> {
};

struct Grammar : pegtl::must<Spacing,
                             ScriptnameStatement,
                             pegtl::star<pegtl::sor<DeclarationStatement,
                                                    BlockStatement>>,
                             pegtl::eof> {
};

} // namespace oo::grammar

/// @}

#endif // OPENOBLIVION_SCRIPTING_GRAMMAR_HPP
