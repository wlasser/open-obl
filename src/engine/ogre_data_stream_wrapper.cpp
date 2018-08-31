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

} // namespace engine