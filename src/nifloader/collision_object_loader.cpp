#include "nifloader/collision_object_loader.hpp"
#include "nifloader/collision_object_loader_state.hpp"
#include "nifloader/logging.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include "ogrebullet/collision_shape.hpp"
#include <OgreException.h>

namespace oo {

void CollisionObjectLoader::loadResource(Ogre::Resource *resource) {
  auto collisionObject = dynamic_cast<Ogre::CollisionShape *>(resource);
  // TODO: Handle this properly
  assert (collisionObject != nullptr);

  auto nifPtr{Ogre::NifResourceManager::getSingleton()
                  .getByName(collisionObject->getName(),
                             collisionObject->getGroup())};
  if (!nifPtr) {
    OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND,
                "Could not load nif resource backing this collision object",
                "CollisionObjectLoader::loadResource()");
  }

  oo::nifloaderLogger()->info("CollisionShape: {}", resource->getName());
  CollisionObjectLoaderState instance(collisionObject, nifPtr->getBlockGraph());
}

} // namespace oo
