#include "modes/load_menu_mode.hpp"
#include "modes/game_mode.hpp"

namespace oo {

MenuMode<gui::MenuType::LoadMenu>::MenuMode(ApplicationContext &ctx)
    : MenuModeBase<LoadMenuMode>(ctx),
      btnReturn{getElementWithId(1)},
      focusBoxSave{getElementWithId(2)},
      listScrollLoad{getElementWithId(3)},
      listLoad{getElementWithId(5)},
      imgLoadPictureBackground{getElementWithId(6)},
      loadText{getElementWithId(7)},
      listPane{getElementWithId(9)} {
  ctx.getLogger()->info("Registered {} templates",
                        getMenuCtx()->registerTemplates());

  auto *entry0{getMenuCtx()->appendTemplate(listPane, "load_game_template")};
  ctx.getLogger()->info("Instantiated {}", entry0->get_name());
  entry0->set_user(0, 0.0f);
  entry0->set_user(3, std::string{"First Save"});

  auto *entry1{getMenuCtx()->appendTemplate(listPane, "load_game_template")};
  ctx.getLogger()->info("Instantiated {}", entry1->get_name());
  entry1->set_user(0, 1.0f);
  entry1->set_user(3, std::string{"Other Save Game"});

  getMenuCtx()->update();
}

LoadMenuMode::transition_t
LoadMenuMode::handleEventImpl(ApplicationContext &/*ctx*/,
                              const sdl::Event &/*event*/) {
  return {false, std::nullopt};
}

void LoadMenuMode::updateImpl(ApplicationContext &/*ctx*/, float /*delta*/) {}

} // namespace oo
