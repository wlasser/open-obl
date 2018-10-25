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
    record::skipRecord(is);
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
  record::skipRecord(is);
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

int main() {
  engine::Application app("Open Oblivion");
  app.getRoot()->startRendering();
  return 0;
}
