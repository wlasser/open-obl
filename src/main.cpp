#include "application.hpp"
#include "job/job.hpp"
#include "settings.hpp"

int main() {
  oo::JobManager::start();

  oo::RenderJobManager::start([]() {
    oo::Application app(oo::RENDER_TARGET);
    oo::getApplication(&app);
    app.getRoot()->startRendering();
  });

  oo::JobManager::stop();

  return 0;
}
