#ifndef OPENOBLIVION_ESP_HPP
#define OPENOBLIVION_ESP_HPP

#include <io/io.hpp>
#include <istream>
#include <iostream>

class Esp {
 public:
  explicit Esp(std::istream &);

  template<class T>
  bool parseRecord(std::istream &is) {
    T rec;
    is >> rec;
    bool isOk = is.rdstate() == std::ios::goodbit;
    if (!is.good()) {
      std::clog << "read failed: " << io::decodeIosState(is.rdstate());
    }
    std::cout << rec << "\n<CLEAR>\n";
    return isOk;
  }

  void parseCell(std::istream &is);
};

#endif // OPENOBLIVION_ESP_HPP
