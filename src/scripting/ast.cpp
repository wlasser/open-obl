#include "scripting/ast.hpp"

namespace oo {

std::string AstNode::name() const noexcept {
  return std::visit([](auto t) -> std::string {
    using T = decltype(t);
    if constexpr (has_type_v<T>) {
      return pegtl::internal::demangle(typeid(typename T::type).name());
    } else {
      return "";
    }
  }, mCurrentType);
}

pegtl::position AstNode::begin() const noexcept {
  return pegtl::position(mBegin, mSource);
}

pegtl::position AstNode::end() const noexcept {
  return pegtl::position(mEnd, mSource);
}

std::string AstNode::content() const noexcept {
  assert(has_content()); // C++20: Remove when contract is added
  return std::string(mBegin.data, mEnd.data);
}

void AstNode::remove_content() noexcept {
  mEnd.reset();
}

void AstNode::setValue(const std::string &value) {
  mValue = value;
}

void AstNode::setValue(std::string &&value) noexcept {
  mValue = std::move(value);
}

void ExprTransform::transform(std::unique_ptr<AstNode> &node) {
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

void OpTransform::transform(std::unique_ptr<AstNode> &node) {
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
    if constexpr (has_type_v<T> && isBinaryOperator_v<typename T::type>) {
      node->setType<grammar::BinaryOperator>();
    }
  });
}

void CallTransform::transform(std::unique_ptr<AstNode> &node) {
  if (node == nullptr || node->children.empty()) return;

  auto &children{node->children};

  if (children.front()->is<grammar::RawMemberAccess>()) {
    auto rawMemberAccess{std::move(children.front())};
    children.erase(children.begin());

    // First child of rawMemberAccess becomes the first parameter, the second
    // becomes the content of the call node.
    if (rawMemberAccess->children.size() != 2) {
      // TODO: Ill-formed member access, throw
      return;
    }

    auto funName{std::move(rawMemberAccess->children[1])};
    auto thisArg{std::move(rawMemberAccess->children[0])};

    node->remove_content();
    node->setValue(funName->content());
    children.insert(children.begin(), std::move(thisArg));
  } else if (children.front()->is<grammar::RawIdentifier>()) {
    auto funName{std::move(children.front())};
    children.erase(children.begin());

    node->remove_content();
    node->setValue(funName->content());
  }
}

} // namespace oo
