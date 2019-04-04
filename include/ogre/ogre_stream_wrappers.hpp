#ifndef OPENOBLIVION_OGRE_OGRE_DATA_STREAMBUF_HPP
#define OPENOBLIVION_OGRE_OGRE_DATA_STREAMBUF_HPP

#include <OgreDataStream.h>
#include <cstddef>
#include <fstream>
#include <memory>
#include <type_traits>

// Ogre uses its own Ogre::DataStream class as an alternative to standard
// streams, arguing that its simpler interface can be implemented for libraries
// which use io, but are incompatible with standard streams. This is good, but
// unfortunately means that any library that *does* use standard streams still
// has to write wrappers into Ogre::DataStream.
// In the future, it may be a good idea to patch Ogre so that Ogre::DataStream
// and its subclasses derive from std::istream; then we can avoid these wrappers
// entirely. A barrier to this is that for standard streams the decision for a
// stream to be read-only is made at compile-time, whereas it can be made at
// compile-time in Ogre. Then every stream will have to be an std::iostream, and
// we will still need wrapper code when passing an std::istream into Ogre.
// TODO: Convert Ogre to use standard streams?
namespace Ogre {

// OgreDataStreambuf is used to convert an existing Ogre::DataStream into a
// standard stream. Currently only input is supported.
// TODO: Support output in OgreDataStreambuf
class OgreDataStreambuf : public std::streambuf {
 private:
  std::shared_ptr<Ogre::DataStream> ogreDataStream{};

 protected:
  int_type underflow() override;
  int_type uflow() override;
  int_type pbackfail(int_type) override;
  pos_type seekpos(pos_type, std::ios_base::openmode) override;
  pos_type seekoff(off_type, std::ios_base::seekdir,
                   std::ios_base::openmode) override;

 public:
  explicit OgreDataStreambuf(std::shared_ptr<Ogre::DataStream> ogreDataStream);
};

// OgreStandardStream is used to convert an existing std::istream into an
// Ogre::DataStream. Currently only input is supported, and the resulting
// stream is read-only.
template<class T>
class OgreStandardStream : public Ogre::DataStream {
  static_assert(std::is_base_of_v<std::istream, T>,
                "T must derive from std::istream");
  static_assert(std::is_move_constructible_v<T>,
                "T must be move constructible");

 private:
  // std::istream::tellg() is not const as it modifies the state bits, but
  // tell() is required to be const, so this is mutable.
  mutable T stream;

 public:
  OgreStandardStream(const Ogre::String &name, T &&stream);
  OgreStandardStream(const OgreStandardStream &other) = delete;
  // std::istream move constructor is not noexcept
  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  OgreStandardStream(OgreStandardStream &&other) = default;
  ~OgreStandardStream() override = default;
  OgreStandardStream &operator=(const OgreStandardStream &other) = delete;
  // std::istream move constructor is not noexcept
  // NOLINTNEXTLINE(performance-noexcept-move-constructor)
  OgreStandardStream &operator=(OgreStandardStream &&other) = default;

  // The underlying standard stream may not be closeable, so this does nothing.
  // It should be specialized for streams that support closing.
  void close() override {}
  bool eof() const override;
  std::size_t read(void *buf, std::size_t count) override;
  void seek(std::size_t pos) override;
  void skip(long count) override;
  std::size_t tell() const override;
};

template<class T>
OgreStandardStream<T>::OgreStandardStream(const Ogre::String &name, T &&stream)
    : Ogre::DataStream(name), stream(std::move(stream)) {}

template<class T>
bool OgreStandardStream<T>::eof() const {
  return stream.eof();
}

template<class T>
std::size_t OgreStandardStream<T>::read(void *buf, std::size_t count) {
  stream.read(static_cast<char *>(buf), count);
  // The standard library guarantees that std::streamsize is non-negative here
  return static_cast<std::size_t>(stream.gcount());
}

template<class T>
void OgreStandardStream<T>::seek(std::size_t pos) {
  stream.seekg(static_cast<typename T::pos_type>(pos));
}

template<class T>
void OgreStandardStream<T>::skip(long count) {
  stream.seekg(static_cast<typename T::off_type>(count), std::ios_base::cur);
}

template<class T>
std::size_t OgreStandardStream<T>::tell() const {
  return static_cast<std::size_t>(stream.tellg());
}

template<>
void OgreStandardStream<std::ifstream>::close();

} // namespace Ogre

#endif // OPENOBLIVION_OGRE_OGRE_DATA_STREAMBUF_HPP

