#include "modes/loading_menu_mode.hpp"

namespace oo {

LoadingMenuMode::transition_t
LoadingMenuMode::handleEventImpl(ApplicationContext &/*ctx*/,
                                 const sdl::Event &/*event*/) {
  return {false, std::nullopt};
}

void LoadingMenuMode::updateImpl(ApplicationContext &/*ctx*/, float /*delta*/) {
  const float maximumProgress{getMenuCtx()->get_user<float>(4)};
  const float currentProgress{(getClock() / 10.0f) * maximumProgress};
  getMenuCtx()->set_user(3, std::min(currentProgress, maximumProgress));
}

} // namespace oo
