#include "application.hpp"
#include "settings.hpp"

int main() {
  oo::Application app(oo::RENDER_TARGET);
  oo::getApplication(&app);
  app.getRoot()->startRendering();
  return 0;
}
