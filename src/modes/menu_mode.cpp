#include "modes/menu_mode.hpp"

MenuMode::transition_t
MenuMode::handleEvent(ApplicationContext &ctx, const sdl::Event &event) {
  // Pop if a event::MenuMode is pressed
  if (auto keyEvent{ctx.getKeyMap().translateKey(event)}) {
    if (auto *ev{std::get_if<oo::event::MenuMode>(&*keyEvent)}; ev) {
      return {ev->down, std::nullopt};
    }
  }
  return {false, std::nullopt};
}

void MenuMode::update(ApplicationContext &ctx, float delta) {
}
