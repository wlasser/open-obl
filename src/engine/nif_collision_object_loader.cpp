#include "engine/nif_collision_object_loader.hpp"
#include "engine/nif_collision_object_loader_state.hpp"
#include "engine/nifloader/loader.hpp"
#include "ogre/ogre_stream_wrappers.hpp"
#include "ogrebullet/collision_object.hpp"

namespace engine {

void NifCollisionObjectLoader::loadResource(Ogre::Resource *resource) {
  if (!logger) logger = spdlog::get(settings::ogreLog);
  auto collisionObject = dynamic_cast<Ogre::CollisionObject *>(resource);
  // TODO: Handle this properly
  assert (collisionObject != nullptr);

  logger->info("CollisionObject: {}", resource->getName());

  auto ogreDataStream = Ogre::ResourceGroupManager::getSingletonPtr()
      ->openResource(collisionObject->getName(), collisionObject->getGroup());

  auto ogreDataStreamBuffer = Ogre::OgreDataStreambuf{ogreDataStream};
  std::istream is{&ogreDataStreamBuffer};

  auto blocks = nifloader::createBlockGraph(is);

  NifCollisionObjectLoaderState instance(collisionObject, blocks);
}

} // namespace engine
