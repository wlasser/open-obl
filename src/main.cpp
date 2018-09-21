#include "bsa.hpp"
#include "esp.hpp"
#include "engine/application.hpp"
#include "engine/nifloader/loader.hpp"
#include "records.hpp"
#include "record/record.hpp"
#include "save_state.hpp"
#include <boost/format.hpp>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

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

template<class Processor>
void readMaster(Processor &processor) {
  std::ifstream esmStream("Data/Oblivion.esm", std::ios::binary);
  if (!esmStream.is_open()) {
    throw io::IOReadError(boost::str(
        boost::format("Failed to open 'Data/Oblivion.esm': %s")
            % strerror(errno)));
  }
  esp::readEsp(esmStream, processor);
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

int main() {
  //bsa::BSAReader reader("Data/Oblivion - Meshes.bsa");
  //saveFromBSA(reader, "meshes", "markerxheading.nif",
  //            "meshes/markerxheading.nif");
  testApplication();
  return 0;
}
