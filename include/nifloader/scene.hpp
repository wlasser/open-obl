#ifndef OPENOBLIVION_NIFLOADER_SCENE_HPP
#define OPENOBLIVION_NIFLOADER_SCENE_HPP

#include <gsl/gsl>
#include <Ogre.h>
#include <btBulletDynamicsCommon.h>

namespace oo {

/// \addtogroup OpenOblivionNifloader
/// @{

Ogre::SceneNode *insertNif(const std::string &name, const std::string &group,
                           gsl::not_null<Ogre::SceneManager *> scnMgr,
                           gsl::not_null<btDiscreteDynamicsWorld *> world,
                           gsl::not_null<Ogre::SceneNode *> parent);

/// Call `oo::insertNif()` with the root scene node passed as the `parent` node.
Ogre::SceneNode *insertNif(const std::string &name, const std::string &group,
                           gsl::not_null<Ogre::SceneManager *> scnMgr,
                           gsl::not_null<btDiscreteDynamicsWorld *> world);

void attachRagdoll(const std::string &name, const std::string &group,
                   gsl::not_null<Ogre::SceneManager *> scnMgr,
                   gsl::not_null<btDiscreteDynamicsWorld *> world,
                   gsl::not_null<Ogre::Entity *> entity);

/// @}

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_SCENE_HPP
