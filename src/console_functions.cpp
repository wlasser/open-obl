#include "application.hpp"
#include "console_functions.hpp"

int console::QuitGame() {
  oo::getApplication()->quit();
  return 0;
}

int console::qqq() {
  return console::QuitGame();
}

int console::ToggleCollisionGeometry() {
  if (oo::getApplication()->isGameModeInStack()) {
    oo::getApplication()->getGameModeInStack().toggleCollisionGeometry();
  }

  return 0;
}

int console::tcg() {
  return console::ToggleCollisionGeometry();
}

int console::ShowMainMenu() {
  oo::getApplication()->openMenu(gui::MenuType::MainMenu);
  return 0;
}
