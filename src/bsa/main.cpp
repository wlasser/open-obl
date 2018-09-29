#include "bsa/application.hpp"

int main(int argc, char *argv[]) {

  auto application = bsa::Application::create();

  return application->run(argc, argv);
}
