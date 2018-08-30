#include "engine/ogre_data_stream_wrapper.hpp"

namespace engine {

auto OgreDataStreamWrapper::underflow() -> int_type {
  if (ogreDataStream->eof()) return traits_type::eof();
  int_type ch{};
  ogreDataStream->read(&ch, 1);
  ogreDataStream->skip(-1);
  return ch;
}

auto OgreDataStreamWrapper::uflow() -> int_type {
  if (ogreDataStream->eof()) return traits_type::eof();
  int_type ch{};
  ogreDataStream->read(&ch, 1);
  lastCh = ch;
  return ch;
}

auto OgreDataStreamWrapper::pbackfail(int_type c) -> int_type {
  if (ogreDataStream->tell() == 0
      || (c != traits_type::eof() && c != lastCh)) {
    return traits_type::eof();
  }
  ogreDataStream->skip(-1);
  return c == traits_type::eof() ? traits_type::not_eof(c) : c;
}

} // namespace engine