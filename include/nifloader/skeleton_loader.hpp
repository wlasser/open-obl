#ifndef OPENOBLIVION_NIFLOADER_SKELETON_LOADER_HPP
#define OPENOBLIVION_NIFLOADER_SKELETON_LOADER_HPP

#include <OgreResource.h>

namespace oo {

/// Loader for `Ogre::Skeleton`s defined in NIF files.
/// \ingroup OpenOblivionNifloader
class SkeletonLoader : public Ogre::ManualResourceLoader {
 private:
  friend class SkeletonLoaderState;

 public:
  void loadResource(Ogre::Resource *resource) override;
};

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_SKELETON_LOADER_HPP
