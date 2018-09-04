#include <cxxopts.hpp>
#include <OgreMesh.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <string>

#include "engine/nif_loader.hpp"
#include "save_state.hpp"
#include "records.hpp"
#include "record/record.hpp"
#include "bsa.hpp"
#include "esp.hpp"
#include "app.hpp"

void testSaveLoading() {
  std::ifstream pcSaveStream("saves/marie.ess", std::ios::binary);
  SaveState pcSave(pcSaveStream);
}

void testEsmLoading() {
  std::ifstream esmStream("Data/Oblivion.esm", std::ios::binary);
  if (!esmStream.is_open()) {
    std::cerr << std::string("Failed to open file: ") << strerror(errno);
    return;
  }
  Esp esm(esmStream);
}

void testBsaLoading() {
  bsa::BSAReader bsaReader("Data/Oblivion - Meshes.bsa");
  auto is = bsaReader["meshes/clothes/robeuc01/f"]["robeuc01f.nif"];
  auto data = std::unique_ptr<char[]>(new char[is.size()]);
  is.read(data.get(), is.size());
  std::ofstream of("test.nif", std::ios::binary);
  of.write(data.get(), is.size());
}

void testApp() {
  App app;
  app.initApp();
  app.getRoot()->startRendering();
  app.closeApp();
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
  // TODO: Use NifLoader here
}

int main() {
  testApp();
  //bsa::BSAReader reader("Data/Oblivion - Meshes.bsa");
  return 0;
}
