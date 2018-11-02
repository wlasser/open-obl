#ifndef OPENOBLIVION_GUI_STACK_INSTRUCTIONS_HPP
#define OPENOBLIVION_GUI_STACK_INSTRUCTIONS_HPP

#include "gui/stack/types.hpp"
#include <cmath>
#include <numeric>
#include <random>
#include <string>
#include <string_view>
#include <variant>

namespace gui::stack {

// Pop the next two values `a` and `b` off the stack and push the result of
// `f(a, b)` onto the stack if that expression is well-defined.
template<class F>
void invokeBinaryOperator(Stack &stack, F f) {
  // TODO: What if stack has < 2 elements?
  const auto b{stack.back()};
  stack.pop_back();
  const auto a{stack.back()};
  stack.pop_back();
  std::visit([&stack, &f](const auto &lhs, const auto &rhs) {
    if constexpr (std::is_same_v<decltype(lhs), decltype(rhs)>) {
      auto opt
          {meta::try_functor<std::decay_t<decltype(lhs) >>(f, &lhs, &rhs)};
      if (opt.has_value()) {
        stack.emplace_back(*opt);
      } else {
        throw std::runtime_error(
            "Type error: could not invoke binary functor with given arguments");
      }
    } else {
      throw std::runtime_error(
          "Type error: arguments to binary functor must have the same type");
    }
  }, a, b);
}

// Like invokeBinaryOperator, but for binary predicates (i.e. returning bool).
template<class F>
void invokeBinaryPredicate(Stack &stack, F f) {
  // TODO: What if stack has < 2 elements?
  const auto b{stack.back()};
  stack.pop_back();
  const auto a{stack.back()};
  stack.pop_back();
  std::visit([&stack, &f](const auto &lhs, const auto &rhs) {
    if constexpr (std::is_same_v<decltype(lhs), decltype(rhs)>) {
      auto opt
          {meta::try_predicate<std::decay_t<decltype(lhs) >>(f, &lhs, &rhs)};
      if (opt.has_value()) {
        stack.emplace_back(*opt);
      } else {
        throw std::runtime_error(
            "Type error: could not invoke binary predicate with given arguments");
      }
    } else {
      throw std::runtime_error(
          "Type error: arguments to binary predicate must have the same type");
    }
  }, a, b);
}

struct nop_t {
  void operator()(Stack &stack) const {}
};

struct push_t {
  ArgumentType arg_t;
  void operator()(Stack &stack) const {
    std::visit(meta::overloaded{
        [&stack](const TraitName &traitName) {
          const std::string_view name{traitName.str};
          // Trailing underscore implies a switch statement using the working
          // value, otherwise replace the working value.
          if (name.back() == '_') {
            const auto val{stack.back()};
            const auto nameCase{appendSwitchCase(std::string{name}, val)};
          }
          // TODO: Get T for this trait and push onto stack with getTrait<T>
        },
        [&stack](int i) { stack.emplace_back(i); },
        [&stack](float f) { stack.emplace_back(f); },
        [&stack](bool b) { stack.emplace_back(b); },
        [&stack](const std::string s) { stack.emplace_back(s); }
    }, arg_t);
  }
};

struct add_t {
  constexpr int operator()(int a, int b) const noexcept {
    return a + b;
  }
  constexpr float operator()(float a, float b) const noexcept {
    return a + b;
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  std::string operator()(const std::string &a, const std::string &b) const {
    return a + b;
  }
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct sub_t {
  constexpr int operator()(int a, int b) const noexcept {
    return a - b;
  }
  constexpr float operator()(float a, float b) const noexcept {
    return a - b;
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct mul_t {
  constexpr int operator()(int a, int b) const noexcept {
    return a * b;
  }
  constexpr float operator()(float a, float b) const noexcept {
    return a * b;
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct div_t {
  constexpr int operator()(int a, int b) const noexcept {
    return a / b;
  }
  constexpr float operator()(float a, float b) const noexcept {
    return a / b;
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct mod_t {
  constexpr int operator()(int a, int b) const noexcept {
    return a % b;
  }
  constexpr float operator()(float a, float b) const noexcept {
    return std::fmod(a, b);
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct floor_t {
  constexpr int operator()(int a, int b) const noexcept {
    return a + b;
  }
  constexpr float operator()(float a, float b) const noexcept {
    return std::floor(a + b);
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct ceil_t {
  constexpr int operator()(int a, int b) const noexcept {
    return a + b;
  }
  constexpr float operator()(float a, float b) const noexcept {
    return std::ceil(a + b);
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct min_t {
  constexpr int operator()(int a, int b) const noexcept {
    return std::min(a, b);
  }
  constexpr float operator()(float a, float b) const noexcept {
    return std::min(a, b);
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct max_t {
  constexpr int operator()(int a, int b) const noexcept {
    return std::max(a, b);
  }
  constexpr float operator()(float a, float b) const noexcept {
    return std::max(a, b);
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct abs_t {
  constexpr int operator()(int a, int b) const noexcept {
    return a < 0 ? std::abs(a + b) : a;
  }
  constexpr float operator()(float a, float b) const noexcept {
    return a < 0 ? std::abs(a + b) : a;
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct gcd_t {
  constexpr int operator()(int a, int b) const noexcept {
    return std::gcd(a, b);
  }
  constexpr float operator()(float, float) const noexcept = delete;
  constexpr bool operator()(bool, bool) const noexcept = delete;
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct lcm_t {
  constexpr int operator()(int a, int b) const noexcept {
    return std::lcm(a, b);
  }
  constexpr float operator()(float, float) const noexcept = delete;
  constexpr bool operator()(bool, bool) const noexcept = delete;
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct gt_t {
  constexpr bool operator()(int a, int b) const noexcept {
    return a > b;
  }
  constexpr bool operator()(float a, float b) const noexcept {
    return a > b;
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  bool operator()(const std::string &a, const std::string &b) const noexcept {
    return a > b;
  }
  void operator()(Stack &stack) const { invokeBinaryPredicate(stack, *this); }
};

struct gte_t {
  constexpr bool operator()(int a, int b) const noexcept {
    return a >= b;
  }
  constexpr bool operator()(float a, float b) const noexcept {
    return a >= b;
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  bool operator()(const std::string &a, const std::string &b) const noexcept {
    return a >= b;
  }
  void operator()(Stack &stack) const { invokeBinaryPredicate(stack, *this); }
};

struct lt_t {
  constexpr bool operator()(int a, int b) const noexcept {
    return a < b;
  }
  constexpr bool operator()(float a, float b) const noexcept {
    return a < b;
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  bool operator()(const std::string &a, const std::string &b) const noexcept {
    return a < b;
  }
  void operator()(Stack &stack) const { invokeBinaryPredicate(stack, *this); }
};

struct lte_t {
  constexpr bool operator()(int a, int b) const noexcept {
    return a <= b;
  }
  constexpr bool operator()(float a, float b) const noexcept {
    return a <= b;
  }
  constexpr bool operator()(bool, bool) const noexcept = delete;
  bool operator()(const std::string &a, const std::string &b) const noexcept {
    return a <= b;
  }
  void operator()(Stack &stack) const { invokeBinaryPredicate(stack, *this); }
};

struct eq_t {
  constexpr bool operator()(int a, int b) const noexcept {
    return a == b;
  }
  constexpr bool operator()(float a, float b) const noexcept {
    return a == b;
  }
  constexpr bool operator()(bool a, bool b) const noexcept {
    return a == b;
  }
  bool operator()(const std::string &a, const std::string &b) const noexcept {
    return a == b;
  }
  void operator()(Stack &stack) const { invokeBinaryPredicate(stack, *this); }
};

struct neq_t {
  constexpr bool operator()(int a, int b) const noexcept {
    return a != b;
  }
  constexpr bool operator()(float a, float b) const noexcept {
    return a != b;
  }
  constexpr bool operator()(bool a, bool b) const noexcept {
    return a != b;
  }
  bool operator()(const std::string &a, const std::string &b) const noexcept {
    return a != b;
  }
  void operator()(Stack &stack) const { invokeBinaryPredicate(stack, *this); }
};

struct and_t {
  constexpr int operator()(int, int) const noexcept = delete;
  constexpr float operator()(float, float) const noexcept = delete;
  constexpr bool operator()(bool a, bool b) const noexcept {
    return a && b;
  }
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct or_t {
  constexpr int operator()(int, int) const noexcept = delete;
  constexpr float operator()(float, float) const noexcept = delete;
  constexpr bool operator()(bool a, bool b) const noexcept {
    return a || b;
  }
  void operator()(Stack &stack) const { invokeBinaryOperator(stack, *this); }
};

struct not_t {
  void operator()(Stack &stack) const {
    const auto a{stack.back()};
    stack.pop_back();
    std::visit([&stack](auto &&pred) {
      if constexpr (std::is_same_v<std::decay_t<decltype(pred)>, bool>) {
        stack.emplace_back(!pred);
      } else {
        throw std::runtime_error("Type error: expected a bool");
      }
    }, a);
  }
};

struct onlyif_t {
  void operator()(Stack &stack) const {
    const auto b{stack.back()};
    stack.pop_back();
    const auto a{stack.back()};
    stack.pop_back();
    std::visit([&stack](auto &&working, auto &&pred) {
      if constexpr (std::is_same_v<std::decay_t<decltype(pred)>, bool>) {
        if (pred) {
          stack.emplace_back(working);
        } else {
          throw std::runtime_error("Type error: expected a bool");
        }
      }
    }, a, b);
  }
};

struct onlyifnot_t {
  void operator()(Stack &stack) const {
    const auto b{stack.back()};
    stack.pop_back();
    const auto a{stack.back()};
    stack.pop_back();
    std::visit([&stack](auto &&working, auto &&pred) {
      if constexpr (std::is_same_v<std::decay_t<decltype(pred)>, bool>) {
        if (!pred) stack.emplace_back(working);
      } else {
        throw std::runtime_error("Type error: expected a bool");
      }
    }, a, b);
  }
};

struct rand_t {
  void operator()(Stack &stack) const {
    thread_local static std::random_device rd{};
    thread_local static std::mt19937 gen{rd()};

    const auto a{stack.back()};
    stack.pop_back();
    std::visit([&stack](auto &&range) {
      if constexpr (std::is_same_v<std::decay_t<decltype(range)>, int>) {
        std::uniform_int_distribution<int> dist(0, range);
        stack.emplace_back(dist(gen));
      } else if constexpr (std::is_same_v<std::decay_t<decltype(range)>,
                                          float>) {
        std::uniform_real_distribution<float> dist(0.0, range);
        stack.emplace_back(dist(gen));
      } else {
        throw std::runtime_error("Type error: expected an int or float");
      }
    }, a);
  }
};

struct ref_t {
  // TODO: What does ref_t() do?
  void operator()(Stack &stack) const {}
};

// Implicitly declared copy constructor => implicitly declared equality operator
// would be great. TODO: != operators. My hands hurt enough already.

constexpr inline bool operator==(nop_t, nop_t) noexcept {
  return true;
}
constexpr inline bool operator==(const push_t &a, const push_t &b) noexcept {
  return a.arg_t == b.arg_t;
}
constexpr inline bool operator==(add_t, add_t) noexcept {
  return true;
}
constexpr inline bool operator==(sub_t, sub_t) noexcept {
  return true;
}
constexpr inline bool operator==(mul_t, mul_t) noexcept {
  return true;
}
constexpr inline bool operator==(div_t, div_t) noexcept {
  return true;
}
constexpr inline bool operator==(mod_t, mod_t) noexcept {
  return true;
}
constexpr inline bool operator==(floor_t, floor_t) noexcept {
  return true;
}
constexpr inline bool operator==(ceil_t, ceil_t) noexcept {
  return true;
}
constexpr inline bool operator==(min_t, min_t) noexcept {
  return true;
}
constexpr inline bool operator==(max_t, max_t) noexcept {
  return true;
}
constexpr inline bool operator==(abs_t, abs_t) noexcept {
  return true;
}
constexpr inline bool operator==(gcd_t, gcd_t) noexcept {
  return true;
}
constexpr inline bool operator==(lcm_t, lcm_t) noexcept {
  return true;
}
constexpr inline bool operator==(gt_t, gt_t) noexcept {
  return true;
}
constexpr inline bool operator==(gte_t, gte_t) noexcept {
  return true;
}
constexpr inline bool operator==(lt_t, lt_t) noexcept {
  return true;
}
constexpr inline bool operator==(lte_t, lte_t) noexcept {
  return true;
}
constexpr inline bool operator==(eq_t, eq_t) noexcept {
  return true;
}
constexpr inline bool operator==(neq_t, neq_t) noexcept {
  return true;
}
constexpr inline bool operator==(and_t, and_t) noexcept {
  return true;
}
constexpr inline bool operator==(or_t, or_t) noexcept {
  return true;
}
constexpr inline bool operator==(not_t, not_t) noexcept {
  return true;
}
constexpr inline bool operator==(onlyif_t, onlyif_t) noexcept {
  return true;
}
constexpr inline bool operator==(onlyifnot_t, onlyifnot_t) noexcept {
  return true;
}
constexpr inline bool operator==(rand_t, rand_t) noexcept {
  return true;
}
constexpr inline bool operator==(ref_t, ref_t) noexcept {
  return true;
}

using Instruction = std::variant<nop_t,
                                 push_t,
                                 add_t,
                                 sub_t,
                                 mul_t,
                                 div_t,
                                 mod_t,
                                 floor_t,
                                 ceil_t,
                                 min_t,
                                 max_t,
                                 abs_t,
                                 gcd_t,
                                 lcm_t,
                                 gt_t,
                                 gte_t,
                                 lt_t,
                                 lte_t,
                                 eq_t,
                                 neq_t,
                                 and_t,
                                 or_t,
                                 not_t,
                                 onlyif_t,
                                 onlyifnot_t,
                                 rand_t,
                                 ref_t>;

} // namespace gui::stack

#endif // OPENOBLIVION_GUI_STACK_INSTRUCTIONS_HPP
