#ifndef OPENOBLIVION_NIFLOADER_MESH_LOADER_HPP
#define OPENOBLIVION_NIFLOADER_MESH_LOADER_HPP

#include "nifloader/loader.hpp"
#include <Ogre.h>
#include <memory>

namespace oo {

/// Loader for `Ogre::Mesh`es defined in NIF files.
/// \ingroup OpenOblivionNifloader
class MeshLoader : public Ogre::ManualResourceLoader {
 private:
  friend class MeshLoaderState;

  std::shared_ptr<spdlog::logger> logger{};

 public:
  void loadResource(Ogre::Resource *resource) override;
};

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_MESH_LOADER_HPP
