#include <istream>
#include <string>
#include <array>
#include <optional>
#include "io_util.hpp"

std::string readBzString(std::istream &s) {
  // Read length
  uint8_t len = 0;
  s.read(reinterpret_cast<char *>(&len), 1);
  // Easier to read into a char array then convert than into a string
  auto arr = new char[len];
  s.read(arr, len);
  std::string str(arr);
  delete[] arr;
  return str;
}

std::string readBString(std::istream &s) {
  // Much the same as readBzString but without the null-terminator so
  // we add it ourselves
  uint8_t len = 0;
  s.read(reinterpret_cast<char *>(&len), 1);
  auto arr = new char[len + 1];
  s.read(arr, len);
  arr[len] = 0;
  std::string str(arr);
  delete[] arr;
  return str;
}

template<>
void readBytes(std::istream &is, std::string &data) {
  std::getline(is, data, '\0');
  if (is.rdstate() != std::ios::goodbit) {
    throw IOReadError("std::string", is.rdstate());
  }
}

std::string peekRecordType(std::istream &is) {
  // Peek at the next 4 bytes
  char type[5]{};
  is.read(type, 4);
  is.seekg(-4, std::istream::cur);
  return std::string(type);
  //std::string s(type);
  //if (s == "EDID" || s == "FULL" || s == "MODL" || s == "MODB"
  //|| s == "ICON" || s == "SCRI" || s == "EFID" || s == "DATA"
  //|| s == "CNAM" || s == "SNAM" || s == "MAST" || s == "DELE"
  //|| s == "MODT" || s == "ENIT" || s == "EFIT" || s == "SCIT"
  //|| s == "HEDR" || s == "OFST" || s == "ALCH" || s == "TES4"
  //|| s == "GMST" || s == "GRUP")
  //return s;
  //return "";
}
