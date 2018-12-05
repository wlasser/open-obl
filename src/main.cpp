#include "application.hpp"
#include "settings.hpp"

int main() {
  Application app(oo::RENDER_TARGET);
  app.getRoot()->startRendering();
  return 0;
}
