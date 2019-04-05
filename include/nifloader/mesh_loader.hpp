#ifndef OPENOBLIVION_NIFLOADER_MESH_LOADER_HPP
#define OPENOBLIVION_NIFLOADER_MESH_LOADER_HPP

#include <OgreResource.h>

namespace oo {

/// Loader for `oo::Mesh`es defined in NIF files.
/// \ingroup OpenOblivionNifloader
class MeshLoader : public Ogre::ManualResourceLoader {
 private:
  friend class MeshLoaderState;

 public:
  void loadResource(Ogre::Resource *resource) override;
};

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_MESH_LOADER_HPP
