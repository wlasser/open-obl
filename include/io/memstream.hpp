#ifndef OPENOBL_IO_MEMSTREAM_HPP
#define OPENOBL_IO_MEMSTREAM_HPP

#include <istream>

namespace io {

/// Non-owning std::iostream wrapper for binary data.
/// Provides a standard std::iostream interface to an existing container of
/// bytes.
class memstream : public std::iostream {
 private:
  class membuf : public std::basic_streambuf<char> {
   public:
    membuf(const uint8_t *p, std::size_t l) {
      setg((char *) (p), (char *) (p), (char *) (p) + l);
    }

    pos_type seekpos(pos_type pos, std::ios_base::openmode which) override {
      return seekoff(pos, std::ios_base::beg, which);
    }

    pos_type seekoff(off_type off, std::ios_base::seekdir dir,
                     std::ios_base::openmode /*which*/) override {
      switch (dir) {
        case std::ios_base::beg:setg(eback(), eback() + off, egptr());
          break;
        case std::ios_base::cur:setg(eback(), gptr() + off, egptr());
          break;
        case std::ios_base::end:setg(eback(), egptr() + off, egptr());
          break;
        default: return pos_type(off_type(-1));
      }
      return pos_type(gptr() - eback());
    }
  };

  membuf buffer;

 public:
  memstream(const uint8_t *p, std::size_t l)
      : std::iostream(&buffer), buffer(p, l) {
    rdbuf(&buffer);
  }
};

} // namespace io

#endif //OPENOBL_IO_MEMSTREAM_HPP
