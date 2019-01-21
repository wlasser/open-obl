#ifndef OPENOBLIVION_RESOLVERS_HELPERS_HPP
#define OPENOBLIVION_RESOLVERS_HELPERS_HPP

#include "fs/path.hpp"
#include "meta.hpp"
#include "record/formid.hpp"
#include "record/records.hpp"
#include "resolvers/ecs.hpp"
#include <gsl/gsl>
#include <tuple>
#include <type_traits>

namespace oo {

template<class T>
Ogre::Entity *loadMesh(const T &rec, gsl::not_null<Ogre::SceneManager *> mgr) {
  oo::Path rawPath;
  using modl_t = decltype(rec.modelFilename);
  if constexpr (std::is_same_v<modl_t, std::optional<record::MODL>>) {
    if (!rec.modelFilename) return nullptr;
    rawPath = oo::Path{rec.modelFilename->data};
  } else if constexpr (std::is_same_v<modl_t, record::MODL>) {
    rawPath = oo::Path{rec.modelFilename.data};
  } else {
    static_assert(false_v<T>, "Missing MODL record");
  }
  std::string meshName{(oo::Path{"meshes"} / rawPath).c_str()};
  return mgr->createEntity(meshName);
}

Ogre::RigidBody *
loadRigidBody(Ogre::Entity *entity, gsl::not_null<Ogre::SceneManager *> mgr);

// If `mesh` is non-null, attach it to the `node` and return a new child node,
// otherwise return `node`. If `final` is true, never create a child node.
gsl::not_null<Ogre::SceneNode *>
attachMesh(gsl::not_null<Ogre::SceneNode *> node, Ogre::Entity *mesh,
           bool final = false);

// If `rigidBody` is non-null, attach it to the `node`, link it to the `world`,
// and return a new child node. Otherwise return `node`. If `final` is true,
// never create a child node.
gsl::not_null<Ogre::SceneNode *>
attachRigidBody(gsl::not_null<Ogre::SceneNode *> node,
                Ogre::RigidBody *rigidBody,
                gsl::not_null<btDiscreteDynamicsWorld *> world,
                bool final = false);

// If `light` is non-null, attach it to the `node` and return a new child node,
// otherwise return `node.` If `final` is true, never create a child node.
gsl::not_null<Ogre::SceneNode *>
attachLight(gsl::not_null<Ogre::SceneNode *> node,
            Ogre::Light *light,
            bool final = false);

// Set the bullet user data in the RigidBody to the given RefId.
void setRefId(gsl::not_null<Ogre::RigidBody *> rigidBody, RefId refId);

// Attach all the components to the node in the correct order, and link to the
// given RefId.
template<class ...Components>
gsl::not_null<Ogre::SceneNode *>
attachAll(gsl::not_null<Ogre::SceneNode *> node,
          RefId refId,
          gsl::not_null<btDiscreteDynamicsWorld *> world,
          ecs::Entity<Components...> &entity) {
  std::size_t count{0};

  if constexpr (ecs::contains<ecs::RigidBody, Components...>()) {
    auto rigidBody{std::get<ecs::RigidBody>(entity).value};
    if (rigidBody) {
      oo::setRefId(gsl::make_not_null(rigidBody), refId);
    }
    const bool last{++count == sizeof...(Components)};
    node = oo::attachRigidBody(node, rigidBody, world, last);
  }

  if constexpr (ecs::contains<ecs::Mesh, Components...>()) {
    auto mesh{std::get<ecs::Mesh>(entity).value};
    const bool last{++count == sizeof...(Components)};
    node = oo::attachMesh(node, mesh, last);
  }

  if constexpr (ecs::contains<ecs::Light, Components...>()) {
    auto light{std::get<ecs::Light>(entity).value};
    const bool last{++count == sizeof...(Components)};
    node = oo::attachLight(node, light, last);
  }

  return node;
}

} // namespace oo

#endif // OPENOBLIVION_RESOLVERS_HELPERS_HPP
