#include "modes/load_menu_mode.hpp"
#include "modes/game_mode.hpp"

namespace oo {

LoadMenuMode::transition_t
LoadMenuMode::handleEventImpl(ApplicationContext &/*ctx*/,
                              const sdl::Event &/*event*/) {
  return {false, std::nullopt};
}

void LoadMenuMode::updateImpl(ApplicationContext &/*ctx*/, float /*delta*/) {}

} // namespace oo
