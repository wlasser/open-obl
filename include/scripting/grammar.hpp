#ifndef OPENOBLIVION_SCRIPTING_GRAMMAR_HPP
#define OPENOBLIVION_SCRIPTING_GRAMMAR_HPP

#include "meta.hpp"
#include <tao/pegtl.hpp>
#include <tao/pegtl/contrib/parse_tree.hpp>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

/// \addtogroup OpenOblivionScripting Scripting
/// Parser for the built-in default scripting language.
/// This document describes the parsing and execution method for user-defined
/// scripts. For information on the scripting language itself, see scripting.md.
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

/// `StrSet <- "set"`
struct StrSet : pegtl::string<'s', 'e', 't'> {};

/// `StrTo <- "to"`
struct StrTo : pegtl::string<'t', 'o'> {};

/// `StrReturn <- "return"`
struct StrReturn : pegtl::string<'r', 'e', 't', 'u', 'r', 'n'> {};

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

/// `Set <- StrSet Spacing`
struct Set : impl::Spaced<StrSet> {};

/// `To <- StrTo Spacing`
struct To : impl::Spaced<StrTo> {};

/// `Return <- StrReturn Spacing`
struct Return : impl::Spaced<StrReturn> {};

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

/// `MemberAccess <- (RefLiteral / Identifier) "." Identifier`
struct MemberAccess : pegtl::seq<pegtl::sor<RefLiteral, Identifier>,
                                 pegtl::one<'.'>,
                                 Identifier> {
};

/// `Variable <- Identifier / MemberAccess`
struct Variable : pegtl::sor<MemberAccess, Identifier> {
};

struct Expression;

/// `PrimaryExpression <- Variable / Literal / (LParen Expression RParen)`
struct PrimaryExpression : pegtl::sor<Variable,
                                      Literal,
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

struct Statement : pegtl::sor<DeclarationStatement,
                              SetStatement,
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

namespace impl {

/// Provides a member type `type` equal to `T`.
/// Used to store a type instead of an instance of that type.
/// \remark Equivalent to std::type_identity proposed in
///         [P0887](https://wg21.link/p0887), which has been merged into C++20.
///         A major motivation there was to inhibit template argument deduction.
//C++20: Remove and use std::type_identity
template<class T> struct type_identity {
  using type = T;
};

/// Helper variable template for type_identity.
template<class T> using type_identity_t = typename type_identity<T>::type;

/// Provide the member constant `value` equal to `true` if `T` has the member
/// type `type`, and `false` otherwise.
/// \tparam T The type to check
template<class T, class = void>
struct has_type : std::false_type {};

template<class T>
struct has_type<T, std::conditional_t<false, typename T::type, void>>
    : std::true_type {
};

template<class T>
static inline constexpr bool has_type_v = has_type<T>::value;

}

/// A node in the AST produced by PEGTL.
/// The content() refers to the string of characters in the input that was
/// matched by some rule in order to create this node; this is consistent with
/// the default pegtl::parse_tree::node.
/// The value() is a writable string used to store additional information about
/// the node. The exact content is dependent on the type represented by the
/// node.
class AstNode {
 public:
  /// The possible types this AstNode can represent.
  /// Should be all those types `T` for which AstSelector<T> defines the member
  /// value `value` equal to `true`.
  using type_variant_t = std::variant<
      impl::type_identity<std::tuple<>>,
      impl::type_identity<RawScriptnameStatement>,
      impl::type_identity<RawScriptname>,
      impl::type_identity<RawIdentifier>,
      impl::type_identity<BlockStatement>,
      impl::type_identity<StringLiteralContents>,
      impl::type_identity<IntegerLiteral>,
      impl::type_identity<RefLiteralContents>,
      impl::type_identity<FloatLiteral>,
      impl::type_identity<DeclarationStatement>,
      impl::type_identity<SetStatement>,
      impl::type_identity<ReturnStatement>,
      impl::type_identity<RawShort>,
      impl::type_identity<RawLong>,
      impl::type_identity<RawFloat>,
      impl::type_identity<RawRef>,
      impl::type_identity<MemberAccess>,
      impl::type_identity<StrPlus>,
      impl::type_identity<StrDash>,
      impl::type_identity<StrStar>,
      impl::type_identity<StrSlash>,
      impl::type_identity<StrLteq>,
      impl::type_identity<StrGteq>,
      impl::type_identity<StrLt>,
      impl::type_identity<StrGt>,
      impl::type_identity<StrEqeq>,
      impl::type_identity<StrNeq>,
      impl::type_identity<StrAnd>,
      impl::type_identity<StrOr>,
      impl::type_identity<BinaryOperator>,
      impl::type_identity<UnaryOperator>,
      impl::type_identity<MultiplicativeBinaryOperator>,
      impl::type_identity<AdditiveBinaryOperator>,
      impl::type_identity<ConditionalBinaryOperator>,
      impl::type_identity<EqualityBinaryOperator>,
      impl::type_identity<ConjunctionBinaryOperator>,
      impl::type_identity<DisjunctionBinaryOperator>,
      impl::type_identity<PrimaryExpression>,
      impl::type_identity<UnaryExpression>,
      impl::type_identity<MulExpression>,
      impl::type_identity<AddExpression>,
      impl::type_identity<CondExpression>,
      impl::type_identity<EqExpression>,
      impl::type_identity<AndExpression>,
      impl::type_identity<OrExpression>,
      impl::type_identity<Expression>>;

 private:
  /// The current type represented by the AstNode.
  type_variant_t mCurrentType{std::in_place_index<0>};

  std::string mSource{};
  pegtl::internal::iterator mBegin{};
  pegtl::internal::iterator mEnd{};

  std::string mValue{};

 public:
  std::vector<std::unique_ptr<AstNode>> children{};

  /// Return `true` if this node represents a `T`, and `false` otherwise.
  template<class T>
  [[nodiscard]] constexpr bool is() const noexcept {
    return std::holds_alternative<impl::type_identity<T>>(mCurrentType);
  }

  /// Return `true` if this node is the root of the AST, and `false` otherwise.
  [[nodiscard]] constexpr bool is_root() const noexcept {
    return mCurrentType.index() == 0;
  }

  /// The name of the type represented by this node.
  /// \todo This uses RTTI via `typeid` and requires the impl::has_type
  ///       machinery because of it. An alternative would be to std::visit the
  ///       current type with overloaded and hardcode the names. This would also
  ///       make it easy to customise the output for each type---like not
  ///       showing the content of statements---without asking the caller to do
  ///       that. Perhaps that is more suited to another member function.
  [[nodiscard]] std::string name() const noexcept {
    return std::visit([](auto t) -> std::string {
      using T = decltype(t);
      if constexpr (impl::has_type_v<T>) {
        return pegtl::internal::demangle(typeid(typename T::type).name());
      } else {
        return "";
      }
    }, mCurrentType);
  }

  /// Starting position of the content of this node in the input.
  [[nodiscard]] pegtl::position begin() const noexcept {
    return pegtl::position(mBegin, mSource);
  }

  /// Ending position of the content of this node in the input.
  [[nodiscard]] pegtl::position end() const noexcept {
    return pegtl::position(mEnd, mSource);
  }

  /// Returns `true` if this node has any content in the input, and `false`
  /// otherwise.
  [[nodiscard]] bool has_content() const noexcept {
    return mEnd.data != nullptr;
  }

  /// Return the content of the input that generated this node, if any.
  /// \pre `has_content() == true`.
  [[nodiscard]] std::string content() const noexcept
  /*C++20: [[expects: has_content()]]*/ {
    assert(has_content());
    return std::string(mBegin.data, mEnd.data);
  }

  /// Delete the content held by this node.
  /// \post `has_content() == false`.
  void remove_content() noexcept {
    mEnd.reset();
  }

  [[nodiscard]] constexpr std::string_view getValue() const noexcept {
    return mValue;
  }

  void setValue(const std::string &value) {
    mValue = value;
  }

  template<class Visitor>
  auto visit(Visitor &&visitor) {
    return std::visit(visitor, mCurrentType);
  }

  template<class T>
  void setType() {
    mCurrentType.emplace<impl::type_identity<T>>();
  }

  AstNode() = default;
  ~AstNode() = default;

  AstNode(const AstNode &) = delete;
  AstNode &operator=(const AstNode &) = delete;

  AstNode(AstNode &&) noexcept = default;
  AstNode &operator=(AstNode &&) noexcept = default;

  /// Called by PEGTL to initialize the (non-root) node.
  template<class Rule, class Input>
  void start(const Input &in) {
    mCurrentType.emplace<impl::type_identity<Rule>>();
    mSource = in.source();
    mBegin = in.iterator();
  }

  /// Called by PEGTL when parsing the Rule succeeded.
  template<class Rule, class Input>
  void success(const Input &in) noexcept {
    mEnd = in.iterator();
  }

  /// Called by PEGTL when parsing the Rule failed.
  template<class Rule, class Input>
  void failure(const Input &in) noexcept {}

  /// Called by PEGTL to append a child when parsing the node succeeded and the
  /// node was not removed by a transform.
  void emplace_back(std::unique_ptr<AstNode> child) {
    children.emplace_back(std::move(child));
  }
};

/// Rearrange expression nodes into operator nodes.
/// Explicitly, performs the transformations
/// - `(BinaryExpression (Op Arg1 Arg2)) -> (Op (Arg1 Arg2))`
/// - `(UnaryExpression (Op Arg1)) -> (Op (Arg1))`.
/// \remark Adapted from the parse_tree.cpp example in PEGTL.
struct ExprTransform : std::true_type {
  static void transform(std::unique_ptr<AstNode> &node) {
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

/// Transform specific BinaryOperators into the generic BinaryOperator with a
/// value equal to the parsed operator. A similar procedure should be done for
/// UnaryOperators too if more than one category of UnaryOperator is added.
/// For example, performs the transformation
/// `(AdditiveBinaryOperator (StrMul Arg1 Arg2)) -> (BinaryOperator:"+" (Arg1 Arg2))`
struct OpTransform : std::true_type {
  static void transform(std::unique_ptr<AstNode> &node) {
    if (node == nullptr || node->children.empty()) return;

    node->remove_content();
    auto &children{node->children};

    // Move the first child of the operator node (i.e. the operator itself) into
    // the value of the operator node, and transform the operator node into a
    // generic BinaryOperator or UnaryOperator.
    auto op{std::move(children.front())};
    children.erase(children.begin());
    node->setValue(op->content());
    node->visit([&node](auto t) {
      using T = decltype(t);
      if constexpr (impl::has_type_v<T>) {
        using Type = typename T::type;
        if constexpr (std::is_same_v<Type, MultiplicativeBinaryOperator> ||
            std::is_same_v<Type, AdditiveBinaryOperator> ||
            std::is_same_v<Type, ConditionalBinaryOperator> ||
            std::is_same_v<Type, EqualityBinaryOperator> ||
            std::is_same_v<Type, ConjunctionBinaryOperator> ||
            std::is_same_v<Type, DisjunctionBinaryOperator>) {
          node->setType<BinaryOperator>();
        }
      }
    });
  }
};

/// Specifies which node types should be kept in the AST and which
/// transformations should be performed to each node.
template<class Rule> struct AstSelector : std::false_type {};

template<> struct AstSelector<RawScriptnameStatement> : std::true_type {};
template<> struct AstSelector<RawScriptname> : std::true_type {};
template<> struct AstSelector<RawIdentifier> : std::true_type {};
template<> struct AstSelector<BlockStatement> : std::true_type {};
template<> struct AstSelector<StringLiteralContents> : std::true_type {};
template<> struct AstSelector<IntegerLiteral> : std::true_type {};
template<> struct AstSelector<RefLiteralContents> : std::true_type {};
template<> struct AstSelector<FloatLiteral> : std::true_type {};
template<> struct AstSelector<DeclarationStatement> : std::true_type {};
template<> struct AstSelector<SetStatement> : std::true_type {};
template<> struct AstSelector<ReturnStatement> : std::true_type {};
template<> struct AstSelector<RawShort> : std::true_type {};
template<> struct AstSelector<RawLong> : std::true_type {};
template<> struct AstSelector<RawFloat> : std::true_type {};
template<> struct AstSelector<RawRef> : std::true_type {};
template<> struct AstSelector<MemberAccess> : std::true_type {};
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

template<> struct AstSelector<BinaryOperator> : std::true_type {};
template<> struct AstSelector<UnaryOperator> : OpTransform {};
template<> struct AstSelector<MultiplicativeBinaryOperator> : OpTransform {};
template<> struct AstSelector<AdditiveBinaryOperator> : OpTransform {};
template<> struct AstSelector<ConditionalBinaryOperator> : OpTransform {};
template<> struct AstSelector<EqualityBinaryOperator> : OpTransform {};
template<> struct AstSelector<ConjunctionBinaryOperator> : OpTransform {};
template<> struct AstSelector<DisjunctionBinaryOperator> : OpTransform {};

template<> struct AstSelector<PrimaryExpression> : ExprTransform {};
template<> struct AstSelector<UnaryExpression> : ExprTransform {};
template<> struct AstSelector<MulExpression> : ExprTransform {};
template<> struct AstSelector<AddExpression> : ExprTransform {};
template<> struct AstSelector<CondExpression> : ExprTransform {};
template<> struct AstSelector<EqExpression> : ExprTransform {};
template<> struct AstSelector<AndExpression> : ExprTransform {};
template<> struct AstSelector<OrExpression> : ExprTransform {};
template<> struct AstSelector<Expression> : ExprTransform {};

template<class F, class State>
void visitAst(const AstNode &node, State state, F &&visitor) {
  state = visitor(node, state);
  for (const auto &child : node.children) visitAst(*child, state, visitor);
}

/// Parse a script from the given input source and produce an AST for it.
/// \tparam T a valid PEGTL input, e.g. pegtl::memory_input.
template<class T> [[nodiscard]] std::unique_ptr<AstNode>
parseScript(T &&in) {
  return pegtl::parse_tree::parse<Grammar, AstNode, AstSelector>(in);
}

} // namespace scripting

/// @}

#endif // OPENOBLIVION_SCRIPTING_GRAMMAR_HPP
