#ifndef ESP_HPP
#define ESP_HPP

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
    if (!isOk) {
      std::clog << "read failed: ";
      switch (is.rdstate()) {
        case std::ios::goodbit: std::clog << "goodbit\n";
          break;
        case std::ios::badbit: std::clog << "badbit\n";
          break;
        case std::ios::failbit: std::clog << "failbit\n";
          break;
        case std::ios::eofbit: std::clog << "eofbit\n";
          break;
        default: std::clog << "unknown\n";
          break;
      }
    }
    std::cout << rec << "\n<CLEAR>\n";
    return isOk;
  }

  void parseCell(std::istream &is);
};

#endif // ESP_HPP
