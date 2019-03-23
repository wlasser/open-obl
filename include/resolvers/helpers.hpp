#ifndef OPENOBLIVION_RESOLVERS_HELPERS_HPP
#define OPENOBLIVION_RESOLVERS_HELPERS_HPP

#include "fs/path.hpp"
#include "meta.hpp"
#include "nifloader/scene.hpp"
#include "ogrebullet/rigid_body.hpp"
#include "record/formid.hpp"
#include "record/records.hpp"
#include "settings.hpp"
#include <gsl/gsl>
#include <type_traits>

namespace oo {

// Set the bullet user data in the RigidBody to the given RefId.
void setRefId(gsl::not_null<Ogre::RigidBody *> rigidBody, RefId refId);

// Set the bullet user data in the attached RigidBody to the given RefId.
// TODO: Support rigid bodies anywhere down the hierarchy
void setRefId(gsl::not_null<Ogre::SceneNode *> node, RefId refId);

template<class T> Ogre::SceneNode *
insertNif(const T &baseRec, RefId refId,
          gsl::not_null<Ogre::SceneManager *> scnMgr,
          gsl::not_null<btDiscreteDynamicsWorld *> world,
          Ogre::SceneNode *rootNode = nullptr) {
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
  auto *node = [&]() -> Ogre::SceneNode * {
    if (rootNode) {
      return oo::insertNif(name.c_str(), oo::RESOURCE_GROUP, scnMgr, world,
                           gsl::make_not_null(rootNode));
    } else {
      return oo::insertNif(name.c_str(), oo::RESOURCE_GROUP, scnMgr, world);
    }
  }();

  if (!node) return nullptr;

  oo::setRefId(gsl::make_not_null(node), refId);

  return node;
}

} // namespace oo

#endif // OPENOBLIVION_RESOLVERS_HELPERS_HPP
