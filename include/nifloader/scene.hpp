#ifndef OPENOBLIVION_NIFLOADER_SCENE_HPP
#define OPENOBLIVION_NIFLOADER_SCENE_HPP

#include "mesh/entity.hpp"
#include "mesh/subentity.hpp"
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

/// Load the given 'raw geometry' NIF file having a `nif::NiTriBasedGeom` as the
/// root block and insert it into the scene. The `nif::NiTriBasedGeom` is used
/// to create the single `oo::SubMesh` of an `oo::Mesh`, which is then given the
/// pointed-to material and attached to `nifRoot`.
/// This kind of NIF file is used for baked distant terrain.
/// \returns `nifRoot` for consistency with `insertNif`.
/// \pre The first `nif::NiObject` in the NIF file must be a
///      `nif::NiTriBasedGeom`.
Ogre::SceneNode *insertRawNif(const std::string &name, const std::string &group,
                              const Ogre::MaterialPtr &matPtr,
                              gsl::not_null<Ogre::SceneManager *> scnMgr,
                              gsl::not_null<Ogre::SceneNode *> nifRoot);

/// @}

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_SCENE_HPP
