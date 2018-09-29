#ifndef OPENOBLIVION_ENGINE_NIFLOADER_MESH_LOADER_HPP
#define OPENOBLIVION_ENGINE_NIFLOADER_MESH_LOADER_HPP

#include "engine/nifloader/loader.hpp"
#include <Ogre.h>
#include <memory>

namespace engine::nifloader {

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

} // namespace engine::nifloader

#endif // OPENOBLIVION_ENGINE_NIFLOADER_MESH_LOADER_HPP
