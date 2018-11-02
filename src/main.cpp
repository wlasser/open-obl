#include "application.hpp"

int main() {
  engine::Application app("Open Oblivion");
  app.getRoot()->startRendering();
  return 0;
}
