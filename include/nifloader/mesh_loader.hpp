#ifndef OPENOBL_NIFLOADER_MESH_LOADER_HPP
#define OPENOBL_NIFLOADER_MESH_LOADER_HPP

#include <OgreResource.h>

namespace oo {

/// Loader for `oo::Mesh`es defined in NIF files.
/// \ingroup OpenOBLNifloader
class MeshLoader : public Ogre::ManualResourceLoader {
 private:
  friend class MeshLoaderState;

 public:
  void loadResource(Ogre::Resource *resource) override;
};

} // namespace oo

#endif // OPENOBL_NIFLOADER_MESH_LOADER_HPP
