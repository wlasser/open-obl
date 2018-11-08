#include "nifloader/collision_object_loader.hpp"
#include "nifloader/collision_object_loader_state.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include "settings.hpp"
#include "ogrebullet/collision_object.hpp"
#include <OgreException.h>

namespace nifloader {

void CollisionObjectLoader::loadResource(Ogre::Resource *resource) {
  if (!logger) logger = spdlog::get(settings::ogreLog);
  auto collisionObject = dynamic_cast<Ogre::CollisionObject *>(resource);
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

  logger->info("CollisionObject: {}", resource->getName());
  CollisionObjectLoaderState instance(collisionObject, nifPtr->getBlockGraph());
}

} // namespace nifloader
