#ifndef OPENOBLIVION_NIFLOADER_SCENE_HPP
#define OPENOBLIVION_NIFLOADER_SCENE_HPP

#include <gsl/gsl>
#include <Ogre.h>
#include <btBulletDynamicsCommon.h>

namespace oo {

Ogre::SceneNode *insertNif(const std::string &name, const std::string &group,
                           gsl::not_null<Ogre::SceneManager *> scnMgr,
                           gsl::not_null<btDiscreteDynamicsWorld *> world,
                           gsl::not_null<Ogre::SceneNode *> parent);

/// Call `insertNif(const std::string &, const std::string &,
///                 gsl::not_null<Ogre::SceneManager *>,
///                 gsl::not_null<btDiscreteDynamicsWorld *>,
///                 gsl::not_null<Ogre::SceneNode *>)` with the root scene node
/// passed as the `parent` node.
Ogre::SceneNode *insertNif(const std::string &name, const std::string &group,
                           gsl::not_null<Ogre::SceneManager *> scnMgr,
                           gsl::not_null<btDiscreteDynamicsWorld *> world);

void attachRagdoll(const std::string &name, const std::string &group,
                   gsl::not_null<Ogre::SceneManager *> scnMgr,
                   gsl::not_null<btDiscreteDynamicsWorld *> world,
                   gsl::not_null<Ogre::Entity *> entity);

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_SCENE_HPP
