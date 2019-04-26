#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_ANIMATION_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_ANIMATION_HPP

#include "fs/path.hpp"
#include "mesh/entity.hpp"
#include "record/records_fwd.hpp"
#include <OgreAnimation.h>
#include <OgreAnimationState.h>
#include <OgreSkeleton.h>

namespace oo {

/// Return a path to the animation file for the given animation group.
/// \todo Animation groups are supposed to contain many different animations for
///       different situations. There are 'Forward' animations for when the
///       character is holding a one-handed weapon, a two-handed weapon, a
///       staff, and so on, for example. This function should choose the correct
///       animation out of a set, instead of just returning one.
std::string getAnimFromGroup(const std::string &animGroup);

/// Given an animation filename and some context in which the animation is to
/// play, return the path of the animation file relative to `sMasterPath`.
oo::Path getAnimPath(const std::string &animName, bool firstPerson = false);

/// Return the animation with the given (full path) name owned by the
/// `skeleton`, creating the animation if the `skeleton` doesn't have an
/// animation with that name.
std::pair<Ogre::Animation *, bool>
createOrRetrieveAnimation(Ogre::Skeleton *skeleton,
                          const std::string &animPath);

/// Pick an idle animation for the `npc` and play it.
Ogre::AnimationState *pickIdle(oo::Entity *character);

/// Get the bone instance of the skeleton used by the `record::NPC_`.
Ogre::SkeletonPtr getSkeleton(const record::NPC_ &rec);

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_ANIMATION_HPP
