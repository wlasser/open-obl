#ifndef OPENOBL_NIFLOADER_SKELETON_LOADER_HPP
#define OPENOBL_NIFLOADER_SKELETON_LOADER_HPP

#include <OgreResource.h>

namespace oo {

/// Loader for `Ogre::Skeleton`s defined in NIF files.
/// \ingroup OpenOBLNifloader
class SkeletonLoader : public Ogre::ManualResourceLoader {
 private:
  friend class SkeletonLoaderState;

 public:
  void loadResource(Ogre::Resource *resource) override;
};

} // namespace oo

#endif // OPENOBL_NIFLOADER_SKELETON_LOADER_HPP
