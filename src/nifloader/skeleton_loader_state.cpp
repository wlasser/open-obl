#include "nifloader/skeleton_loader_state.hpp"

namespace oo {

SkeletonLoaderState::SkeletonLoaderState(Ogre::Skeleton *skeleton,
                                         oo::BlockGraph blocks)
    : mBlocks(blocks), mSkeleton(skeleton) {

}

} // namespace oo