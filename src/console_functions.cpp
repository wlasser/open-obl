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
  oo::getApplication()->openMenu<gui::MenuType::MainMenu>();
  return 0;
}

int console::ShowClassMenu() {
//  oo::getApplication()->openMenu<gui::MenuType::ClassMenu>();
  return 0;
}

int console::ShowEnchantmentMenu() {
//  oo::getApplication()->openMenu<gui::MenuType::EnchantmentMenu>();
  return 0;
}

int console::ShowMap() {
//  oo::getApplication()->openMenu<gui::MenuType::MapMenu>();
  return 0;
}

int console::ShowRaceMenu() {
//  oo::getApplication()->openMenu<gui::MenuType::RaceSexMenu>();
  return 0;
}

int console::ShowSpellmaking() {
//  oo::getApplication()->openMenu<gui::MenuType::SpellMakingMenu>();
  return 0;
}

int console::print(float value) {
  oo::ConsoleMode::print(std::to_string(value));
  return 0;
}