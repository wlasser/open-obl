#include "application.hpp"
#include "job/job.hpp"

int main(int argc, char *argv[]) {
  oo::JobManager::start();

  std::unique_ptr<oo::Application> application{};

  oo::RenderJobManager::start([&application]() {
    application = std::make_unique<oo::Application>("EspEdit");
    application->getRoot()->startRendering();
  });

  oo::JobManager::stop();
}