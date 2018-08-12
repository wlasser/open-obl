#ifndef OPENOBLIVION_MEMSTREAM_HPP
#define OPENOBLIVION_MEMSTREAM_HPP

#include <istream>

namespace io {

// Stream wrapping binary data. Does not take ownership.
class memstream : public std::istream {
 private:
  class membuf : public std::basic_streambuf<char> {
   public:
    membuf(const uint8_t *p, std::size_t l) {
      setg((char *) (p), (char *) (p), (char *) (p) + l);
    }
  };

  membuf buffer;

 public:
  memstream(const uint8_t *p, std::size_t l) :
      std::istream(&buffer),
      buffer(p, l) {
    rdbuf(&buffer);
  }
};

} // namespace io

#endif //OPENOBLIVION_MEMSTREAM_HPP
