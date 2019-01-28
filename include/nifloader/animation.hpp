#ifndef OPENOBLIVION_NIFLOADER_ANIMATION_HPP
#define OPENOBLIVION_NIFLOADER_ANIMATION_HPP

#include "nifloader/nif_resource.hpp"
#include <OgreAnimation.h>
#include <OgreSkeleton.h>
#include <string>

namespace oo {

/// Load an animation from the given `nif` resource and attach it to the
/// `skeleton`.
Ogre::Animation *createAnimation(Ogre::Skeleton *skeleton,
                                 Ogre::NifResource *nif);

/// Load an animation from the given nif file and attach it to the `skeleton`.
Ogre::Animation *createAnimation(Ogre::Skeleton *skeleton,
                                 const std::string &nifName,
                                 const std::string &nifGroup);

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_ANIMATION_HPP
