#include "application.hpp"

int main() {
  Application app("Open Oblivion");
  app.getRoot()->startRendering();
  return 0;
}
