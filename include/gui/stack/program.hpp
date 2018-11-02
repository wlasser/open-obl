#ifndef OPENOBLIVION_GUI_STACK_PROGRAM_HPP
#define OPENOBLIVION_GUI_STACK_PROGRAM_HPP

#include "gui/stack/instructions.hpp"
#include "gui/stack/meta.hpp"
#include "gui/stack/types.hpp"
#include <pugixml.hpp>
#include <cstdint>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <variant>
#include <vector>

// Traits in the xml may directly store values, in which case their TraitFun is
// constant and simple to create. Traits may instead store a tree of operators
// which describe a function used to evaluate the trait using the values of
// other traits in the menu. The computation model is that there is a
// working value that starts default initialized, and is then acted upon by a
// sequence of operators. Operators may get their second operand from a trait
// selector, from a value, or from a nested sequence of operators.  The working
// value after the last operator has been evaluated is the value of the trait,
// and will be the starting value the next time the TraitFun is invoked.
// For example, (DarN loading_menu.xml)
// <x>
// <copy src="parent()" trait="width"/>
//   <sub src="me()" trait="width"/>
//   <div> 2 </div>
// </x>
// This copies the parent element's width into the working value, subtracts the
// element's width, then divides the result by 2 and returns. There is no need
// to retain the working value as the next initial value because the sequence
// begins with an <copy> operator.
// In the above example, the type of the working value was invariant throughout
// the function. This is not always the case, for example (DarN magic_menu.xml)
// <zoom>
//   <copy> 75 </copy>
//   <add>
//     <copy> 15 </copy>
//     <onlyif>
//       <copy src="parent()" trait="mouseover" />
//       <eq> 1 </eq>
//       <or> <not src="parent()" trait="target" /> </or>
//     </onlyif>
//   </add>
// </zoom>
// In the innermost sequence the working value is initially an integer, but then
// becomes a boolean after the <eq> operator. This assumes that the type of the
// 'mouseover' trait is known to be an integer. For an implementation trait like
// 'mouseover', the type can be deduced immediately from just the trait name.
// For user traits, the <class> of the trait's parent needs to be checked and
// the corresponding interface consulted. For custom traits, the type must be
// deduced by reading the xml tree; there are no hints from the implementation.
// The particularly difficult case is when a custom trait is defined entirely
// in terms of other custom traits and no operators are used that constrain the
// types; user trait types are not locally deducible. In particular, this means
// that parsing operator sequences is not possible until all trait definitions
// are available.
// The solution taken here is as follows. Every trait is tagged with a
// TraitTypeId initialized as Unimplemented. During parse time, if a trait is an
// implementation trait, user trait, or a custom trait with a constant value,
// then its TraitTypeId is deduced and set. Also during parse time, the operator
// tree for each trait is translated into an instruction set for a stack machine
// by performing a post-order DFS. During this phase, selectors are resolved to
// trait names but types are ignored. Since the (C++) type T of each user and
// implementation trait is known, a Trait<T> and TraitFun<T> can be constructed.
// (The TraitFun<T> simply evaluates the stack program, expecting an output of
// type T.) The types of non-constant custom traits are not known, but can be
// deduced from the types of their dependencies during the first update, which
// are guaranteed to be known due to the topological order. They must therefore
// be added to the dependency graph as a Trait<std::variant<...> with a
// TraitFun<std::variant<...>>.

namespace gui {

namespace stack {

class Program {
 private:
  mutable std::optional<ValueType> lastReturn{};
  mutable std::mutex lastReturnMutex{};
 public:
  std::vector<Instruction> instructions{};
  std::vector<std::string> dependencies{};

  Program() = default;
  Program(const Program &other);
  Program &operator=(const Program &other);
  Program(Program &&other) noexcept;
  Program &operator=(Program &&other) noexcept;

  ValueType operator()();
};

inline bool operator==(const Program &lhs, const Program &rhs) {
  return lhs.instructions == rhs.instructions
      && lhs.dependencies == rhs.dependencies;
}

inline bool operator!=(const Program &lhs, const Program &rhs) {
  return !(lhs == rhs);
}

// Want to traverse using post-order DFS, pugixml gives pre-order DFS only.
template<class T>
void postOrderDFS(pugi::xml_node node, T &&visitor) {
  for (auto child : node.children()) {
    postOrderDFS(child, visitor);
  }
  visitor(node);
}

Program compile(pugi::xml_node node);

template<class T>
T run(const Program &program);

}; // namespace stack

} // namespace gui

#endif // OPENOBLIVION_GUI_STACK_PROGRAM_HPP
