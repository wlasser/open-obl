#include "application.hpp"

int main(int argc, char *argv[]) {
  std::unique_ptr<oo::Application>
      application{std::make_unique<oo::Application>("EspEdit")};
  application->getRoot()->startRendering();
}