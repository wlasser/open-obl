#ifndef OPENOBLIVION_NIFLOADER_SKELETON_LOADER_STATE_HPP
#define OPENOBLIVION_NIFLOADER_SKELETON_LOADER_STATE_HPP

#include "nifloader/loader.hpp"
#include "nifloader/loader_state.hpp"
#include <Ogre.h>

namespace oo {

class SkeletonLoaderState {
 private:
  oo::BlockGraph mBlocks{};
  Ogre::Skeleton *mSkeleton{};

 public:
  explicit SkeletonLoaderState(Ogre::Skeleton *skeleton, oo::BlockGraph blocks);
};

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_SKELETON_LOADER_STATE_HPP
