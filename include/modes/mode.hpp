#ifndef OPENOBLIVION_MODE_HPP
#define OPENOBLIVION_MODE_HPP

#include <optional>
#include <tuple>
#include <variant>

template<class ...States>
using ModeTransition = std::tuple<bool, std::optional<std::variant<States...>>>;

#endif // OPENOBLIVION_MODE_HPP
