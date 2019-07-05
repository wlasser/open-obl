#ifndef OPENOBL_OGRE_OGRE_DATA_STREAMBUF_HPP
#define OPENOBL_OGRE_OGRE_DATA_STREAMBUF_HPP

#include <OgreDataStream.h>
#include <cstddef>
#include <fstream>
#include <memory>
#include <type_traits>

/// \file ogre/ogre_stream_wrappers.hpp
/// \ingroup OpenOBLOgre
/// Adaptors for working with OGRE streams using a standard stream interface,
/// and vice-versa.
///
/// OGRE uses its own `Ogre::DataStream` class as an alternative to standard
/// streams, arguing that its simpler interface can be implemented for libraries
/// which use io, but are incompatible with standard streams. This is good, but
/// unfortunately means that any library that *does* use standard streams still
/// has to write wrappers into `Ogre::DataStream`. Unfortunately, these wrappers
/// come with *a lot* of overhead.
///
/// In the future, it may be a good idea to patch OGRE so that
/// `Ogre::DataStream` and its subclasses derive from `std::istream`; then we
/// can avoid these wrappers entirely. A barrier to this is that for standard
/// streams the decision for a stream to be read-only is made at compile-time,
/// whereas it can be made at runtime in Ogre. Then every stream will have to be
/// a `std::iostream`, and we will still need wrapper code when passing an
/// `std::istream` into Ogre.

namespace Ogre {

/// Convert an existing `Ogre::DataStream` into a standard stream.
/// Example usage is
/// ```cpp
/// Ogre::DataStreamPtr ogreStream = /* ... */;
/// Ogre::OgreDataStreambuf streambuf(dataStream);
/// std::istream is(&streambuf);
/// ```
/// Currently only input is supported.
/// \todo Support output in `OgreDataStreambuf`.
class OgreDataStreambuf : public std::streambuf {
 public:
  explicit OgreDataStreambuf(std::shared_ptr<Ogre::DataStream> ogreDataStream);

 protected:
  int_type underflow() override;
  int_type uflow() override;
  int_type pbackfail(int_type) override;
  pos_type seekpos(pos_type, std::ios_base::openmode) override;
  pos_type seekoff(off_type, std::ios_base::seekdir,
                   std::ios_base::openmode) override;

 private:
  std::shared_ptr<Ogre::DataStream> mOgreStream{};
};

/// Convert an existing `std::istream` into an `Ogre::DataStream`.
/// Currently only input is supported, and the resulting stream is read-only.
template<class T>
class OgreStandardStream : public Ogre::DataStream {
  static_assert(std::is_convertible_v<T *, std::istream *>,
                "T must derive from std::istream");
  static_assert(std::is_move_constructible_v<T>,
                "T must be move constructible");
 public:
  OgreStandardStream(const Ogre::String &name, T &&stream);
  OgreStandardStream(const OgreStandardStream &other) = delete;
  // std::istream move constructor is not noexcept
  // NOLINTNEXTLINE(performance-noexcept-move-constructor,hicpp-noexcept-move)
  OgreStandardStream(OgreStandardStream &&other) = default;
  ~OgreStandardStream() override = default;
  OgreStandardStream &operator=(const OgreStandardStream &other) = delete;
  // std::istream move assignment operator is not noexcept
  // NOLINTNEXTLINE(performance-noexcept-move-constructor,hicpp-noexcept-move)
  OgreStandardStream &operator=(OgreStandardStream &&other) = default;

  // The underlying standard stream may not be closeable, so this does nothing.
  // It should be specialized for streams that support closing.
  void close() override {}
  bool eof() const override;
  std::size_t read(void *buf, std::size_t count) override;
  void seek(std::size_t pos) override;
  void skip(long count) override;
  std::size_t tell() const override;

 private:
  // std::istream::tellg() is not const as it modifies the state bits, but
  // tell() is required to be const, so this is mutable.
  mutable T mStream;
};

template<class T>
OgreStandardStream<T>::OgreStandardStream(const Ogre::String &name, T &&stream)
    : Ogre::DataStream(name), mStream(std::move(stream)) {}

template<class T>
bool OgreStandardStream<T>::eof() const {
  return mStream.eof();
}

template<class T>
std::size_t OgreStandardStream<T>::read(void *buf, std::size_t count) {
  mStream.read(static_cast<char *>(buf), count);
  // The standard library guarantees that std::streamsize is non-negative here
  return static_cast<std::size_t>(mStream.gcount());
}

template<class T>
void OgreStandardStream<T>::seek(std::size_t pos) {
  mStream.seekg(static_cast<typename T::pos_type>(pos));
}

template<class T>
void OgreStandardStream<T>::skip(long count) {
  mStream.seekg(static_cast<typename T::off_type>(count), std::ios_base::cur);
}

template<class T>
std::size_t OgreStandardStream<T>::tell() const {
  return static_cast<std::size_t>(mStream.tellg());
}

template<>
void OgreStandardStream<std::ifstream>::close();

} // namespace Ogre

#endif // OPENOBL_OGRE_OGRE_DATA_STREAMBUF_HPP

