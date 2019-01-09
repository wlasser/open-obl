#include "application.hpp"
#include "settings.hpp"

int main() {
  oo::Application app(oo::RENDER_TARGET);
  app.getRoot()->startRendering();
  return 0;
}
