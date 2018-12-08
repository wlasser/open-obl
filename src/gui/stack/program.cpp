#include "gui/stack/program.hpp"
#include "gui/trait_selector.hpp"
#include <mutex>
#include <string_view>

namespace gui::stack {

Program::Program(const Program &other) {
  std::unique_lock lock{other.lastReturnMutex};
  lastReturn = other.lastReturn;
  instructions = other.instructions;
  dependencies = other.dependencies;
}

Program &Program::operator=(const Program &other) {
  if (*this != other) {
    std::unique_lock writeLock(lastReturnMutex, std::defer_lock);
    std::unique_lock readLock(other.lastReturnMutex, std::defer_lock);
    std::lock(writeLock, readLock);
    lastReturn = other.lastReturn;
    instructions = other.instructions;
    dependencies = other.dependencies;
  }
  return *this;
}

Program::Program(Program &&other) noexcept {
  std::unique_lock lock{other.lastReturnMutex};
  std::swap(lastReturn, other.lastReturn);
  std::swap(instructions, other.instructions);
  std::swap(dependencies, other.dependencies);
}

Program &Program::operator=(Program &&other) noexcept {
  if (*this != other) {
    std::unique_lock writeLock(lastReturnMutex, std::defer_lock);
    std::unique_lock readLock(other.lastReturnMutex, std::defer_lock);
    std::lock(writeLock, readLock);
    std::swap(lastReturn, other.lastReturn);
    std::swap(instructions, other.instructions);
    std::swap(dependencies, other.dependencies);
  }
  return *this;
}

Program compile(pugi::xml_node node) {
  Program program{};

  auto addInstr = [&program](auto &&instr) {
    program.instructions.emplace_back(std::forward<decltype(instr)>(instr));
  };
  auto addDep = [&program](std::string dep) {
    program.dependencies.emplace_back(std::move(dep));
  };

  stack::postOrderDFS(node, [&addInstr, &addDep](pugi::xml_node child) {
    // If 'src' and 'trait' attributes are given, then a push_t is performed
    // with the selected trait before the operation.
    if (const auto srcOpt{gui::resolveTrait(child)}; srcOpt) {
      push_t instr{stack::TraitName{*srcOpt}};
      addInstr(std::move(instr));
      addDep(*srcOpt);
    }

    // If the child has a PCDATA child, then the value is pushed before the
    // operation.
    if (auto pcdata{child.first_child()}; pcdata.type() == pugi::node_pcdata) {
      const std::string_view valueStr{pcdata.value()};
      ValueType value{stack::parseValueType(valueStr)};
      push_t instr{std::visit([](auto &v) { return ArgumentType{v}; }, value)};
      addInstr(std::move(instr));
    }

    const std::string_view name{child.name()};
    if (name == "copy") {} // Pushing has already been taken care of.
    else if (name == "add") addInstr(add_t{});
    else if (name == "sub") addInstr(sub_t{});
    else if (name == "mul" || name == "mult") addInstr(mul_t{});
    else if (name == "div") addInstr(div_t{});
    else if (name == "mod") addInstr(mod_t{});
    else if (name == "floor") addInstr(floor_t{});
    else if (name == "ceil") addInstr(ceil_t{});
    else if (name == "min") addInstr(min_t{});
    else if (name == "max") addInstr(max_t{});
    else if (name == "abs") addInstr(abs_t{});
    else if (name == "gcd") addInstr(gcd_t{});
    else if (name == "lcm") addInstr(lcm_t{});
    else if (name == "gt") addInstr(gt_t{});
    else if (name == "gte") addInstr(gte_t{});
    else if (name == "lt") addInstr(lt_t{});
    else if (name == "lte") addInstr(lte_t{});
    else if (name == "eq") addInstr(eq_t{});
    else if (name == "neq") addInstr(neq_t{});
    else if (name == "and") addInstr(and_t{});
    else if (name == "or") addInstr(or_t{});
    else if (name == "not") addInstr(not_t{});
    else if (name == "onlyif") addInstr(onlyif_t{});
    else if (name == "onlyifnot") addInstr(onlyifnot_t{});
    else if (name == "rand") addInstr(rand_t{});
    else if (name == "ref") addInstr(ref_t{});
  });

  return program;
}

ValueType Program::operator()() {
  std::unique_lock lock{lastReturnMutex};
  Stack stack{};
  if (lastReturn) stack.push_back(*lastReturn);
  for (const auto &instr : instructions) {
    std::visit([&stack](auto &&op) { op(stack); }, instr);
  }
  if (stack.empty()) {
    throw std::runtime_error("Program stack is empty on termination");
  }
  lastReturn = stack.back();
  return *lastReturn;
}

} // namespace gui::stack