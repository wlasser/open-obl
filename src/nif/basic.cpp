#include "nif/basic.hpp"

std::istream &nif::basic::operator>>(std::istream &is, HeaderString &t) {
  std::getline(is, t.str, '\n');
  return is;
}

std::istream &nif::basic::operator>>(std::istream &is, LineString &t) {
  std::getline(is, t.str, '\n');
  return is;
}
