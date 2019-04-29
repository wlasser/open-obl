#ifndef OPENOBLIVION_RESOLVERS_HELPERS_HPP
#define OPENOBLIVION_RESOLVERS_HELPERS_HPP

#include "fs/path.hpp"
#include "meta.hpp"
#include "nifloader/scene.hpp"
#include "ogrebullet/rigid_body.hpp"
#include "record/formid.hpp"
#include "record/records.hpp"
#include "settings.hpp"
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <gsl/gsl>
#include <type_traits>

namespace oo {

/// Set the bullet user data in the `Ogre::RigidBody` to the given `refId`.
void setRefId(gsl::not_null<Ogre::RigidBody *> rigidBody, RefId refId);

/// Set the bullet user data in the attached `Ogre::RigidBody`s of `node` or any
/// of its children to the given `refId`.
void setRefId(gsl::not_null<Ogre::SceneNode *> node, RefId refId);

/// Given a base record with a `modelFilename` member of type `record::MODL`,
/// construct a child node of the given `parentNode` (or the scene root if none
/// is given) and use insert the record's model into the scene graph.
/// The child node will be named equal to the result of `refId.string()`.
template<class T> Ogre::SceneNode *
insertNif(const T &baseRec, RefId refId,
          gsl::not_null<Ogre::SceneManager *> scnMgr,
          gsl::not_null<btDiscreteDynamicsWorld *> world,
          Ogre::SceneNode *parentNode = nullptr) {
  oo::Path baseName;
  using modl_t = decltype(baseRec.modelFilename);
  if constexpr (std::is_same_v<modl_t, std::optional<record::MODL>>) {
    if (!baseRec.modelFilename) return nullptr;
    baseName = oo::Path{baseRec.modelFilename->data};
  } else if constexpr (std::is_same_v<modl_t, record::MODL>) {
    baseName = oo::Path{baseRec.modelFilename.data};
  } else {
    static_assert(false_v<T>, "Missing MODL record");
  }
  oo::Path name{oo::Path{"meshes"} / baseName};

  auto *parent{parentNode ? parentNode : scnMgr->getRootSceneNode()};
  auto *root{parent->createChildSceneNode(refId.string())};
  auto *node{oo::insertNif(name.c_str(), oo::RESOURCE_GROUP, scnMgr, world,
                           gsl::make_not_null(root))};
  if (node) oo::setRefId(gsl::make_not_null(root), refId);

  return node;
}

} // namespace oo

#endif // OPENOBLIVION_RESOLVERS_HELPERS_HPP
