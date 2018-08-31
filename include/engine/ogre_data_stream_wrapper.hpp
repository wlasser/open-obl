#ifndef OPENOBLIVION_ENGINE_OGRE_DATA_STREAM_WRAPPER_HPP
#define OPENOBLIVION_ENGINE_OGRE_DATA_STREAM_WRAPPER_HPP

#include <OgreDataStream.h>
#include <memory>
#include <streambuf>

namespace engine {

// Ogre::DataStream cannot be used with any of the io functions because it
// doesn't inherit from std::istream, though it does implement all the
// functionality we need it to. We can't construct an io::memstream because
// Ogre::Datastream doesn't (and cannot, for good reason) provide a pointer to
// any underlying data.
class OgreDataStreamWrapper : public std::streambuf {
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
  explicit OgreDataStreamWrapper(std::shared_ptr<Ogre::DataStream> ogreDataStream)
      : std::streambuf(), ogreDataStream(std::move(ogreDataStream)) {}
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_OGRE_DATA_STREAM_WRAPPER_HPP

