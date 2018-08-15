#ifndef OPENOBLIVION_IO_MEMSTREAM_HPP
#define OPENOBLIVION_IO_MEMSTREAM_HPP

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
    pos_type seekpos(pos_type pos, std::ios_base::openmode which) override {
      setg(eback(), eback() + pos, egptr());
      return gptr() - eback();
    }
  };

  membuf buffer;

 public:
  memstream(const uint8_t *p, std::size_t l)
      : std::istream(&buffer), buffer(p, l) {
    rdbuf(&buffer);
  }
};

} // namespace io

#endif //OPENOBLIVION_IO_MEMSTREAM_HPP
