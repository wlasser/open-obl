#include "application.hpp"
#include "job/job.hpp"
#include "util/settings.hpp"

#undef main

int main() {
  oo::JobManager::start();

  std::unique_ptr<oo::Application> application{};

  oo::RenderJobManager::start([&application]() {
    application = std::make_unique<oo::Application>(oo::RENDER_TARGET);
    oo::getApplication(application.get());
    application->getRoot()->startRendering();
  });

  oo::JobManager::stop();

  return 0;
}
