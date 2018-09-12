#include <cxxopts.hpp>
#include <OgreMesh.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>

#include "engine/application.hpp"
#include "engine/nif_loader.hpp"
#include "save_state.hpp"
#include "records.hpp"
#include "record/record.hpp"
#include "bsa.hpp"
#include "esp.hpp"

void testSaveLoading() {
  std::ifstream pcSaveStream("saves/marie.ess", std::ios::binary);
  SaveState pcSave(pcSaveStream);
}

struct NullProcessor {
  template<class R>
  void process(const R &record) {}
};

struct LogProcessor {
  template<class R>
  void process(const R &record) {
    std::clog << "<rec>" << record << "</rec>\n";
  }
};

NullProcessor nullProcessor;
LogProcessor logProcessor;

void testEsmLoading() {
  std::ifstream esmStream("Data/Oblivion.esm", std::ios::binary);
  if (!esmStream.is_open()) {
    std::cerr << std::string("Failed to open file: ") << strerror(errno);
    return;
  }
  Esp esm(esmStream, logProcessor);
}

void testBsaLoading() {
  bsa::BSAReader bsaReader("Data/Oblivion - Meshes.bsa");
  auto is = bsaReader["meshes/clothes/robeuc01/f"]["robeuc01f.nif"];
  auto data = std::unique_ptr<char[]>(new char[is.size()]);
  is.read(data.get(), is.size());
  std::ofstream of("test.nif", std::ios::binary);
  of.write(data.get(), is.size());
}

void testApplication() {
  engine::Application app("Open Oblivion");
  app.getRoot()->addFrameListener(&app);
  app.getRoot()->startRendering();
}

void extractNif(bsa::BSAReader &reader,
                const std::string &folder,
                const std::string &file,
                const std::string &destination) {
  auto is = reader[folder][file];
  auto data = std::unique_ptr<char[]>(new char[is.size()]);
  is.read(data.get(), is.size());
  std::ofstream of(destination, std::ios::binary);
  of.write(data.get(), is.size());
}

void checkNif(bsa::BSAReader &reader,
              const std::string &folder,
              const std::string &file) {
  std::filesystem::path fileName{file};
  if (fileName.extension() != ".nif") return;
  auto is = reader[folder][file];
  std::clog << "Loading " << folder << "/" << file << '\n';
  // TODO: Use NifLoaderState here
}

int main() {
  //testApplication();
  testEsmLoading();
  //bsa::BSAReader reader("Data/Oblivion - Meshes.bsa");
  return 0;
}
