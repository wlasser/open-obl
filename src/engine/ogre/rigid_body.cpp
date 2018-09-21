#include "engine/nifloader/loader.hpp"
#include "engine/ogre/rigid_body.hpp"
#include "engine/ogre/ogre_stream_wrappers.hpp"
#include <OgreResourceGroupManager.h>

namespace Ogre {

RigidBody::RigidBody(Ogre::ResourceManager *creator,
                     const Ogre::String &name,
                     Ogre::ResourceHandle handle,
                     const Ogre::String &group,
                     bool isManual,
                     Ogre::ManualResourceLoader *loader)
    : Resource(creator, name, handle, group, isManual, loader) {}

btRigidBody *RigidBody::getRigidBody() {
  return mRigidBody.get();
}

void RigidBody::loadImpl() {
  auto ogreDataStream = ResourceGroupManager::getSingleton()
      .openResource(mName, mGroup);

  if (ogreDataStream == nullptr) {
    OGRE_EXCEPT(Exception::ERR_FILE_NOT_FOUND,
                "Unable to open resource",
                "RigidBody::load");
  }

  auto ogreDataStreambuf = engine::OgreDataStreambuf{ogreDataStream};
  std::istream is{&ogreDataStreambuf};

  auto blocks = engine::nifloader::createBlockGraph(is);

  // TODO: Actually load the thing
}

void RigidBody::unloadImpl() {
  // TODO: Actually unload the thing
}

} // namespace Ogre