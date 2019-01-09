#ifndef OPENOBLIVION_MODE_HPP
#define OPENOBLIVION_MODE_HPP

#include <optional>
#include <tuple>
#include <variant>

namespace oo {

template<class ...States>
using ModeTransition = std::tuple<bool, std::optional<std::variant<States...>>>;

} // namespace oo

#endif // OPENOBLIVION_MODE_HPP
