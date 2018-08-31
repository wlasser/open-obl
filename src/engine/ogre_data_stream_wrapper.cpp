#include "engine/ogre_data_stream_wrapper.hpp"

namespace engine {

auto OgreDataStreamWrapper::underflow() -> int_type {
  if (ogreDataStream->eof()) return traits_type::eof();
  char_type ch{};
  ogreDataStream->read(&ch, 1);
  ogreDataStream->skip(-1);
  return traits_type::to_int_type(ch);
}

auto OgreDataStreamWrapper::uflow() -> int_type {
  if (ogreDataStream->eof()) return traits_type::eof();
  char_type ch{};
  ogreDataStream->read(&ch, 1);
  return traits_type::to_int_type(ch);
}

auto OgreDataStreamWrapper::pbackfail(int_type c) -> int_type {
  if (ogreDataStream->tell() == 0) return traits_type::eof();
  ogreDataStream->skip(-1);
  int_type lastCh{};
  ogreDataStream->read(&lastCh, 1);
  if (c != lastCh) return traits_type::eof();
  ogreDataStream->skip(-1);
  return c == traits_type::eof() ? traits_type::not_eof(c) : c;
}

auto OgreDataStreamWrapper::seekpos(pos_type pos,
                                    std::ios_base::openmode which) -> pos_type {
  return seekoff(off_type(pos), std::ios_base::beg, which);
}

auto OgreDataStreamWrapper::seekoff(off_type off, std::ios_base::seekdir dir,
                                    std::ios_base::openmode which) -> pos_type {
  switch (dir) {
    case std::ios_base::beg:
      ogreDataStream
          ->seek(off < 0 ? 0u : static_cast<std::size_t>(pos_type(off)));
      break;
    case std::ios_base::cur:ogreDataStream->skip(off);
      break;
    case std::ios_base::end:ogreDataStream->seek(ogreDataStream->size() + off);
      break;
    default:return pos_type(off_type(-1));
  }
  return ogreDataStream->tell();
}

} // namespace engine