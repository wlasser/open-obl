#include "mesh.hpp"
#include "nifloader/logging.hpp"
#include "nifloader/mesh_loader.hpp"
#include "nifloader/mesh_loader_state.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include <OgreException.h>

namespace oo {

void MeshLoader::loadResource(Ogre::Resource *resource) {
  auto mesh = dynamic_cast<oo::Mesh *>(resource);
  // TODO: Handle this properly
  assert(mesh != nullptr);

  // TODO: If the mesh doesn't exist, dynamically generate a placeholder
  auto nifPtr{Ogre::NifResourceManager::getSingleton()
                  .getByName(mesh->getName(), mesh->getGroup())};
  if (!nifPtr) {
    OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND,
                "Could not load nif resource backing this mesh",
                "MeshLoader::loadResource()");
  }
  nifPtr->load();

  oo::nifloaderLogger()->info("Mesh: {}", resource->getName());
  MeshLoaderState instance(mesh, nifPtr->getBlockGraph());
}

} // namespace oo
