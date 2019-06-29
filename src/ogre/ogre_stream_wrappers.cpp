#include "ogre/ogre_stream_wrappers.hpp"
#include <gsl/gsl_util>
#include <OgreDataStream.h>
#include <fstream>
#include <istream>
#include <streambuf>

namespace Ogre {

OgreDataStreambuf::OgreDataStreambuf(std::shared_ptr<Ogre::DataStream> ogreDataStream)
    : std::streambuf(), mOgreStream(std::move(ogreDataStream)) {}

auto OgreDataStreambuf::underflow() -> int_type {
  if (mOgreStream->eof()) return traits_type::eof();
  char_type ch{};
  mOgreStream->read(&ch, 1);
  mOgreStream->skip(-1);
  return traits_type::to_int_type(ch);
}

auto OgreDataStreambuf::uflow() -> int_type {
  if (mOgreStream->eof()) return traits_type::eof();
  char_type ch{};
  mOgreStream->read(&ch, 1);
  return traits_type::to_int_type(ch);
}

auto OgreDataStreambuf::pbackfail(int_type c) -> int_type {
  if (mOgreStream->tell() == 0) return traits_type::eof();
  mOgreStream->skip(-1);
  int_type lastCh{};
  mOgreStream->read(&lastCh, 1);
  if (c != lastCh) return traits_type::eof();
  mOgreStream->skip(-1);
  return c == traits_type::eof() ? traits_type::not_eof(c) : c;
}

auto OgreDataStreambuf::seekpos(pos_type pos,
                                std::ios_base::openmode which) -> pos_type {
  return seekoff(off_type(pos), std::ios_base::beg, which);
}

auto OgreDataStreambuf::seekoff(off_type off, std::ios_base::seekdir dir,
                                std::ios_base::openmode /*which*/) -> pos_type {
  switch (dir) {
    case std::ios_base::beg:
      if (off < 0) mOgreStream->seek(0u);
      else mOgreStream->seek(static_cast<std::size_t>(pos_type(off)));
      break;
    case std::ios_base::cur: mOgreStream->skip(gsl::narrow_cast<long>(off));
      break;
    case std::ios_base::end: mOgreStream->seek(mOgreStream->size() + off);
      break;
    default: return pos_type(off_type(-1));
  }
  return mOgreStream->tell();
}

template<>
void OgreStandardStream<std::ifstream>::close() {
  mStream.close();
}

} // namespace Ogre
