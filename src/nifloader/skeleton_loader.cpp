#include "nifloader/logging.hpp"
#include "nifloader/skeleton_loader.hpp"
#include "nifloader/skeleton_loader_state.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include <OgreException.h>
#include <OgreSkeleton.h>

void oo::SkeletonLoader::loadResource(Ogre::Resource *resource) {
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

  oo::nifloaderLogger()->info("Skeleton: {}", resource->getName());
  SkeletonLoaderState instance(skeleton, nifPtr->getBlockGraph());
}