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
  void readRecord(std::istream &is) {
    (void) record::skipRecord(is);
  }
};

NullProcessor nullProcessor;

struct LogProcessor {
  template<class R>
  void readRecord(std::istream &is) {
    auto rec = record::readRecord<R>(is);
    std::clog << "<rec>" << rec << "</rec>\n";
  }
};

LogProcessor logProcessor;

template<>
void NullProcessor::readRecord<record::CELL>(std::istream &is) {
  (void) record::skipRecord(is);
  esp::readCellChildren(is, nullProcessor, nullProcessor, nullProcessor);
}

void testEsmLoading() {
  std::ifstream esmStream("Data/Oblivion.esm", std::ios::binary);
  if (!esmStream.is_open()) {
    std::cerr << std::string("Failed to open file: ") << strerror(errno);
    return;
  }
  esp::readEsp(esmStream, nullProcessor);
}

void testApplication() {
  engine::Application app("Open Oblivion");
  app.getRoot()->addFrameListener(&app);
  app.getRoot()->startRendering();
}

void saveFromBSA(bsa::BSAReader &reader,
                 const std::filesystem::path &folder,
                 const std::filesystem::path &file,
                 const std::filesystem::path &destination) {
  auto is = reader[folder][file];
  auto size = is.size();
  std::vector<char> data(size);
  is.read(data.data(), size);
  std::ofstream of(destination, std::ios::binary);
  of.write(data.data(), size);
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
  //bsa::BSAReader reader("Data/Oblivion - Meshes.bsa");
  //saveFromBSA(reader, "meshes", "markerxheading.nif",
  //            "meshes/markerxheading.nif");
  testApplication();
  //testEsmLoading();
  //bsa::BSAReader reader("Data/Oblivion - Meshes.bsa");
  return 0;
}
