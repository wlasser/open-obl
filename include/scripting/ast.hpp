#ifndef OPENOBLIVION_SCRIPTING_AST_HPP
#define OPENOBLIVION_SCRIPTING_AST_HPP

#include "meta.hpp"
#include "scripting/grammar.hpp"
#include "scripting/pegtl.hpp"
#include <memory>
#include <string>
#include <tuple>
#include <type_traits>
#include <variant>

namespace oo {

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
      type_identity<std::tuple<>>,
      type_identity<grammar::RawScriptnameStatement>,
      type_identity<grammar::RawScriptname>,
      type_identity<grammar::RawIdentifier>,
      type_identity<grammar::BlockStatement>,
      type_identity<grammar::StringLiteralContents>,
      type_identity<grammar::IntegerLiteral>,
      type_identity<grammar::RefLiteralContents>,
      type_identity<grammar::FloatLiteral>,
      type_identity<grammar::DeclarationStatement>,
      type_identity<grammar::SetStatement>,
      type_identity<grammar::ReturnStatement>,
      type_identity<grammar::IfStatement>,
      type_identity<grammar::ElseifStatement>,
      type_identity<grammar::ElseStatement>,
      type_identity<grammar::CallStatement>,
      type_identity<grammar::RawShort>,
      type_identity<grammar::RawLong>,
      type_identity<grammar::RawFloat>,
      type_identity<grammar::RawRef>,
      type_identity<grammar::RawMemberAccess>,
      type_identity<grammar::StrPlus>,
      type_identity<grammar::StrDash>,
      type_identity<grammar::StrStar>,
      type_identity<grammar::StrSlash>,
      type_identity<grammar::StrLteq>,
      type_identity<grammar::StrGteq>,
      type_identity<grammar::StrLt>,
      type_identity<grammar::StrGt>,
      type_identity<grammar::StrEqeq>,
      type_identity<grammar::StrNeq>,
      type_identity<grammar::StrAnd>,
      type_identity<grammar::StrOr>,
      type_identity<grammar::RawCall>,
      type_identity<grammar::BinaryOperator>,
      type_identity<grammar::UnaryOperator>,
      type_identity<grammar::MultiplicativeBinaryOperator>,
      type_identity<grammar::AdditiveBinaryOperator>,
      type_identity<grammar::ConditionalBinaryOperator>,
      type_identity<grammar::EqualityBinaryOperator>,
      type_identity<grammar::ConjunctionBinaryOperator>,
      type_identity<grammar::DisjunctionBinaryOperator>,
      type_identity<grammar::PrimaryExpression>,
      type_identity<grammar::UnaryExpression>,
      type_identity<grammar::MulExpression>,
      type_identity<grammar::AddExpression>,
      type_identity<grammar::CondExpression>,
      type_identity<grammar::EqExpression>,
      type_identity<grammar::AndExpression>,
      type_identity<grammar::OrExpression>,
      type_identity<grammar::Expression>>;

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
  template<class T> [[nodiscard]] constexpr bool is() const noexcept {
    return std::holds_alternative<type_identity<T>>(mCurrentType);
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
  [[nodiscard]] std::string name() const noexcept;

  /// Starting position of the content of this node in the input.
  [[nodiscard]] pegtl::position begin() const noexcept;

  /// Ending position of the content of this node in the input.
  [[nodiscard]] pegtl::position end() const noexcept;

  /// Returns `true` if this node has any content in the input, and `false`
  /// otherwise.
  [[nodiscard]] constexpr bool has_content() const noexcept {
    return mEnd.data != nullptr;
  }

  /// Return the content of the input that generated this node, if any.
  /// \pre `has_content() == true`.
  [[nodiscard]] std::string
  content() const noexcept /*C++20: [[expects: has_content()]]*/;

  /// Delete the content held by this node.
  /// \post `has_content() == false`.
  void remove_content() noexcept /*C++20: [[ensures: has_content()]]*/;

  /// Get the node's additional mutable information.
  [[nodiscard]] constexpr std::string_view getValue() const noexcept {
    return mValue;
  }

  /// Set the node's additional mutable information.
  void setValue(const std::string &value);

  /// \overload setValue(const std::string &)
  void setValue(std::string &&value) noexcept;

  /// Call the visitor with the current type.
  /// Intended to be used to dispatch to a different function depending on the
  /// current node type represented by this AstNode. Note that the visitor
  /// should not take an instance of a particular node type, but rather a
  /// type_identity wrapping the type.
  /// \tparam Visitor A function object taking a type_identity<T> for each node
  ///                 type T where `AstSelector<T>` has a member value `value`
  ///                 equal to true.
  template<class Visitor> constexpr auto visit(Visitor &&visitor) {
    return std::visit(visitor, mCurrentType);
  }

  /// \overload visit(Visitor &&)
  template<class Visitor> constexpr auto visit(Visitor &&visitor) const {
    return std::visit(visitor, mCurrentType);
  }

  template<class T> constexpr void setType() noexcept {
    mCurrentType.emplace<type_identity<T>>();
  }

  AstNode() = default;
  ~AstNode() = default;

  AstNode(const AstNode &) = delete;
  AstNode &operator=(const AstNode &) = delete;

  AstNode(AstNode &&) noexcept = default;
  AstNode &operator=(AstNode &&) noexcept = default;

  /// \name PEGTL interface
  ///@{

  /// Called by PEGTL to initialize the (non-root) node.
  template<class Rule, class Input>
  void start(const Input &in) {
    mCurrentType.emplace<type_identity<Rule>>();
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

  ///@}
};

template<class Type> constexpr inline bool
    isAstType_v = std::is_same_v<Type, grammar::RawShort>
    || std::is_same_v<Type, grammar::RawLong>
    || std::is_same_v<Type, grammar::RawRef>
    || std::is_same_v<Type, grammar::RawFloat>;

template<class Type> constexpr inline bool
    isBinaryOperator_v = std::is_same_v<Type, grammar::BinaryOperator>
    || std::is_same_v<Type, grammar::MultiplicativeBinaryOperator>
    || std::is_same_v<Type, grammar::AdditiveBinaryOperator>
    || std::is_same_v<Type, grammar::ConditionalBinaryOperator>
    || std::is_same_v<Type, grammar::EqualityBinaryOperator>
    || std::is_same_v<Type, grammar::ConjunctionBinaryOperator>
    || std::is_same_v<Type, grammar::DisjunctionBinaryOperator>;

/// Rearrange expression nodes into operator nodes.
/// Explicitly, performs the transformations
/// - `(BinaryExpression (Op Arg1 Arg2)) -> (Op (Arg1 Arg2))`
/// - `(UnaryExpression (Op Arg1)) -> (Op (Arg1))`.
/// \remark Adapted from the parse_tree.cpp example in PEGTL.
struct ExprTransform : std::true_type {
  static void transform(std::unique_ptr<AstNode> &node);
};

/// Transform specific BinaryOperators into the generic BinaryOperator with a
/// value equal to the parsed operator. A similar procedure should be done for
/// UnaryOperators too if more than one category of UnaryOperator is added.
/// For example, performs the transformation
/// `(AdditiveBinaryOperator (StrMul Arg1 Arg2)) -> (BinaryOperator:"+" (Arg1 Arg2))`
struct OpTransform : std::true_type {
  static void transform(std::unique_ptr<AstNode> &node);
};

/// Transforms member calls to free calls with `this` as the first argument.
struct CallTransform : std::true_type {
  static void transform(std::unique_ptr<AstNode> &node);
};

/// Specifies which node types should be kept in the AST and which
/// transformations should be performed to each node.
template<class Rule> struct AstSelector : std::false_type {};

// @formatter:off
template<> struct AstSelector<grammar::RawScriptnameStatement> : std::true_type {};
template<> struct AstSelector<grammar::RawScriptname> : std::true_type {};
template<> struct AstSelector<grammar::RawIdentifier> : std::true_type {};
template<> struct AstSelector<grammar::BlockStatement> : std::true_type {};
template<> struct AstSelector<grammar::StringLiteralContents> : std::true_type {};
template<> struct AstSelector<grammar::IntegerLiteral> : std::true_type {};
template<> struct AstSelector<grammar::RefLiteralContents> : std::true_type {};
template<> struct AstSelector<grammar::FloatLiteral> : std::true_type {};
template<> struct AstSelector<grammar::DeclarationStatement> : std::true_type {};
template<> struct AstSelector<grammar::SetStatement> : std::true_type {};
template<> struct AstSelector<grammar::ReturnStatement> : std::true_type {};
template<> struct AstSelector<grammar::IfStatement> : std::true_type {};
template<> struct AstSelector<grammar::ElseifStatement> : std::true_type {};
template<> struct AstSelector<grammar::ElseStatement> : std::true_type {};
template<> struct AstSelector<grammar::CallStatement> : std::true_type {};
template<> struct AstSelector<grammar::RawShort> : std::true_type {};
template<> struct AstSelector<grammar::RawLong> : std::true_type {};
template<> struct AstSelector<grammar::RawFloat> : std::true_type {};
template<> struct AstSelector<grammar::RawRef> : std::true_type {};
template<> struct AstSelector<grammar::RawMemberAccess> : std::true_type {};
template<> struct AstSelector<grammar::StrPlus> : std::true_type {};
template<> struct AstSelector<grammar::StrDash> : std::true_type {};
template<> struct AstSelector<grammar::StrStar> : std::true_type {};
template<> struct AstSelector<grammar::StrSlash> : std::true_type {};
template<> struct AstSelector<grammar::StrLteq> : std::true_type {};
template<> struct AstSelector<grammar::StrGteq> : std::true_type {};
template<> struct AstSelector<grammar::StrLt> : std::true_type {};
template<> struct AstSelector<grammar::StrGt> : std::true_type {};
template<> struct AstSelector<grammar::StrEqeq> : std::true_type {};
template<> struct AstSelector<grammar::StrNeq> : std::true_type {};
template<> struct AstSelector<grammar::StrAnd> : std::true_type {};
template<> struct AstSelector<grammar::StrOr> : std::true_type {};
template<> struct AstSelector<grammar::RawCall> : CallTransform {};

template<> struct AstSelector<grammar::BinaryOperator> : std::true_type {};
template<> struct AstSelector<grammar::UnaryOperator> : OpTransform {};
template<> struct AstSelector<grammar::MultiplicativeBinaryOperator> : OpTransform {};
template<> struct AstSelector<grammar::AdditiveBinaryOperator> : OpTransform {};
template<> struct AstSelector<grammar::ConditionalBinaryOperator> : OpTransform {};
template<> struct AstSelector<grammar::EqualityBinaryOperator> : OpTransform {};
template<> struct AstSelector<grammar::ConjunctionBinaryOperator> : OpTransform {};
template<> struct AstSelector<grammar::DisjunctionBinaryOperator> : OpTransform {};

template<> struct AstSelector<grammar::PrimaryExpression> : ExprTransform {};
template<> struct AstSelector<grammar::UnaryExpression> : ExprTransform {};
template<> struct AstSelector<grammar::MulExpression> : ExprTransform {};
template<> struct AstSelector<grammar::AddExpression> : ExprTransform {};
template<> struct AstSelector<grammar::CondExpression> : ExprTransform {};
template<> struct AstSelector<grammar::EqExpression> : ExprTransform {};
template<> struct AstSelector<grammar::AndExpression> : ExprTransform {};
template<> struct AstSelector<grammar::OrExpression> : ExprTransform {};
template<> struct AstSelector<grammar::Expression> : ExprTransform {};
// @formatter:on

/// Parse a script from the given input source and produce an AST for it.
/// \tparam T a valid PEGTL input, e.g. pegtl::memory_input.
template<class T> [[nodiscard]] std::unique_ptr<AstNode> parseScript(T &&in) {
  return pegtl::parse_tree::parse<grammar::Grammar, AstNode, AstSelector>(in);
}

/// Parse a single statement from the given input source and produce an AST for
/// it.
/// \tparam T a valid PEGTL input, e.g. pegtl::memory_input
template<class T> [[nodiscard]] std::unique_ptr<AstNode> parseStatement(T &&in) {
  return pegtl::parse_tree::parse<grammar::Statement, AstNode, AstSelector>(in);
}

} // namespace oo

#endif // OPENOBLIVION_SCRIPTING_AST_HPP
