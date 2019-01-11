#include "modes/loading_menu_mode.hpp"
#include "modes/main_menu_mode.hpp"

namespace oo {

MainMenuMode::transition_t
MainMenuMode::handleEventImpl(ApplicationContext &ctx,
                              const sdl::Event &/*event*/) {
  if (const auto *btn{getMenuCtx()->getElementWithId(2)};
      btn && btn->is_clicked()) {
    return {true, oo::MenuMode<gui::MenuType::LoadingMenu>(ctx)};
  }

  if (const auto *btn{getMenuCtx()->getElementWithId(7)};
      btn && btn->is_clicked()) {
    return {true, std::nullopt};
  }

  return {false, std::nullopt};
}

void MainMenuMode::updateImpl(ApplicationContext &/*ctx*/, float /*delta*/) {
  const float transitionLength{getMenuCtx()->get_user<float>(4)};
  getMenuCtx()->set_user(0, true);
  getMenuCtx()->set_user(1, getClock() > transitionLength);
  getMenuCtx()->set_user(2, getClock() <= transitionLength);
  const float alpha{255.0f * (1.0f - getClock() / transitionLength)};
  getMenuCtx()->set_user(3, std::max(0.0f, alpha));
}

} // namespace oo
