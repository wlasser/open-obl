#ifndef OPENOBLIVION_NIFLOADER_SCENE_HPP
#define OPENOBLIVION_NIFLOADER_SCENE_HPP

#include "mesh/entity.hpp"
#include <gsl/gsl>
#include <OgrePrerequisites.h>
#include <btBulletDynamicsCommon.h>

namespace oo {

/// \addtogroup OpenOblivionNifloader
/// @{

/// Load the given NIF file and insert it into the scene, using `nifRoot` as
/// the root `nif::NiNode` of the NIF file.
/// \pre The first `nif::NiObject` in the NIF file must be an `nif::NiNode` with
///      the name 'Scene Node'.
Ogre::SceneNode *insertNif(const std::string &name, const std::string &group,
                           gsl::not_null<Ogre::SceneManager *> scnMgr,
                           gsl::not_null<btDiscreteDynamicsWorld *> world,
                           gsl::not_null<Ogre::SceneNode *> nifRoot);

void attachRagdoll(const std::string &name, const std::string &group,
                   gsl::not_null<Ogre::SceneManager *> scnMgr,
                   gsl::not_null<btDiscreteDynamicsWorld *> world,
                   gsl::not_null<oo::Entity *> entity);

/// @}

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_SCENE_HPP
