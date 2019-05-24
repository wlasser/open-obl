#ifndef OPENOBL_NIF_COLLISION_OBJECT_LOADER_HPP
#define OPENOBL_NIF_COLLISION_OBJECT_LOADER_HPP

#include <OgreResource.h>

namespace oo {

/// Loader for `Ogre::CollisionShape`s defined in NIF files.
/// \ingroup OpenOBLNifloader
class CollisionObjectLoader : public Ogre::ManualResourceLoader {
 private:
  friend class CollisionObjectLoaderState;

 public:
  void loadResource(Ogre::Resource *resource) override;
};

} // namespace oo

#endif // OPENOBL_NIF_COLLISION_OBJECT_LOADER_HPP
