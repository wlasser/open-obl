#include "io/string.hpp"

std::string io::readBzString(std::istream &s) {
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

std::string io::readBString(std::istream &s) {
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
