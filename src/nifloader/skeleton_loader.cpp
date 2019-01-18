#include "nifloader/skeleton_loader.hpp"
#include "nifloader/skeleton_loader_state.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include "settings.hpp"
#include <OgreException.h>
#include <OgreSkeleton.h>

void oo::SkeletonLoader::loadResource(Ogre::Resource *resource) {
  if (!logger) logger = spdlog::get(oo::OGRE_LOG);
  auto skeleton = dynamic_cast<Ogre::Skeleton *>(resource);
  // TODO: Handle this properly
  assert(skeleton != nullptr);

  auto nifPtr{Ogre::NifResourceManager::getSingleton()
                  .getByName(skeleton->getName(), skeleton->getGroup())};
  if (!nifPtr) {
    OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND,
                "Could not load nif resource backing this mesh",
                "SkeletonLoader::loadResource()");
  }
  nifPtr->load();

  logger->info("Skeleton: {}", resource->getName());
  SkeletonLoaderState instance(skeleton, nifPtr->getBlockGraph());
}