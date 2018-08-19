#include "engine/nif_loader.hpp"
#include "io/memstream.hpp"
#include "nif/nif.hpp"

#include <OgreMesh.h>
#include <OgreResourceGroupManager.h>
#include <streambuf>

namespace engine {

namespace {
// Ogre::DataStream cannot be used with any of the io functions because it
// doesn't inherit from std::istream, though it does implement all the
// functionality we need it to. We can't construct an io::memstream because
// Ogre::Datastream doesn't (and cannot, for good reason) provide a pointer to
// any underlying data.
class OgreDataStreamWrapper : public std::streambuf {
 private:
  std::shared_ptr<Ogre::DataStream> ogreDataStream;
  int_type lastCh{};
 protected:
  int_type underflow() override {
    if (ogreDataStream->eof()) return traits_type::eof();
    int_type ch{};
    ogreDataStream->read(&ch, 1);
    ogreDataStream->skip(-1);
    return ch;
  }

  int_type uflow() override {
    if (ogreDataStream->eof()) return traits_type::eof();
    int_type ch{};
    ogreDataStream->read(&ch, 1);
    lastCh = ch;
    return ch;
  }

  int_type pbackfail(int_type c) override {
    if (ogreDataStream->tell() == 0
        || (c != traits_type::eof() && c != lastCh)) {
      return traits_type::eof();
    }
    ogreDataStream->skip(-1);
    return c == traits_type::eof() ? traits_type::not_eof(c) : c;
  }

 public:
  explicit OgreDataStreamWrapper(std::shared_ptr<Ogre::DataStream> ogreDataStream)
      : ogreDataStream(std::move(ogreDataStream)) {}
};
}

void NifLoader::loadResource(Ogre::Resource *resource) {
  auto mesh = dynamic_cast<Ogre::Mesh *>(resource);
  // TODO: Handle this properly
  assert(mesh != nullptr);

  // TODO: If the mesh doesn't exist, dynamically generate a placeholder
  auto ogreDataStream = Ogre::ResourceGroupManager::getSingletonPtr()
      ->openResource(mesh->getName(), mesh->getGroup());

  auto ogreDataStreamBuffer = OgreDataStreamWrapper{ogreDataStream};
  std::istream is{&ogreDataStreamBuffer};

  nif::NifModel nifModel(is);
}

} // namespace engine

