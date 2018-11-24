#ifndef OPENOBLIVION_NIFLOADER_MESH_LOADER_HPP
#define OPENOBLIVION_NIFLOADER_MESH_LOADER_HPP

#include "nifloader/loader.hpp"
#include <Ogre.h>
#include <memory>

namespace oo {

// Handles custom loading of Nif files for Ogre. Each instance of this class is
// expected to load more than one nif file, so it cannot really be stateful.
// This class therefore handles the IO portion of loading, then constructs a
// MeshLoaderState object to actually load the mesh.
class MeshLoader : public Ogre::ManualResourceLoader {
 private:
  friend class MeshLoaderState;

  std::shared_ptr<spdlog::logger> logger{};

 public:
  void loadResource(Ogre::Resource *resource) override;
  //void prepareResource(Ogre::Resource *resource) override;
};

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_MESH_LOADER_HPP
