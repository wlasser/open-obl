#include "nif/basic.hpp"

std::istream &nif::basic::operator>>(std::istream &is, HeaderString &t) {
  std::getline(is, t.str, '\n');
  return is;
}

std::istream &nif::basic::operator>>(std::istream &is, LineString &t) {
  std::getline(is, t.str, '\n');
  return is;
}

template<>
void io::readBytes(std::istream &is, std::vector<nif::basic::Byte> &data,
                   std::size_t length) {
  data.assign(length, {});
  is.read(reinterpret_cast<char *>(data.data()), length);
}
