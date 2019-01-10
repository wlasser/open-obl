#include "application.hpp"
#include "console_functions.hpp"

int console::QuitGame() {
  oo::getApplication()->quit();
  return 0;
}

int console::qqq() {
  return console::QuitGame();
}
