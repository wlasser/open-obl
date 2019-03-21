#ifndef OPENOBLIVION_RESOLVERS_HELPERS_HPP
#define OPENOBLIVION_RESOLVERS_HELPERS_HPP

#include "fs/path.hpp"
#include "meta.hpp"
#include "nifloader/scene.hpp"
#include "record/formid.hpp"
#include "record/records.hpp"
#include "resolvers/ecs.hpp"
#include "settings.hpp"
#include <boost/mp11.hpp>
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

Ogre::RigidBody *
loadRigidBody(const std::string &name, const std::string &group,
              gsl::not_null<Ogre::SceneManager *> mgr);

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

// Set the bullet user data in the attached RigidBody to the given RefId.
// TODO: Support rigid bodies anywhere down the hierarchy
void setRefId(gsl::not_null<Ogre::SceneNode *> node, RefId refId);

template<int I, class ...Components>
gsl::not_null<Ogre::SceneNode *>
attachAllImpl(gsl::not_null<Ogre::SceneNode *> node,
              RefId refId,
              gsl::not_null<btDiscreteDynamicsWorld *> world,
              const ecs::Entity<Components...> &entity) {
  std::size_t count{0};

  if constexpr (ecs::contains<ecs::RigidBody<I>, Components...>()) {
    auto rigidBody{std::get<ecs::RigidBody<I>>(entity).value};
    if (rigidBody) {
      oo::setRefId(gsl::make_not_null(rigidBody), refId);
    }
    const bool last{++count == sizeof...(Components)};
    node = oo::attachRigidBody(node, rigidBody, world, last);
  }

  if constexpr (ecs::contains<ecs::Mesh<I>, Components...>()) {
    auto mesh{std::get<ecs::Mesh<I>>(entity).value};
    const bool last{++count == sizeof...(Components)};
    node = oo::attachMesh(node, mesh, last);
  }

  if constexpr (ecs::contains<ecs::Light<I>, Components...>()) {
    auto light{std::get<ecs::Light<I>>(entity).value};
    const bool last{++count == sizeof...(Components)};
    node = oo::attachLight(node, light, last);
  }

  return node;
}

// Attach all the components to the node in the correct order, and link to the
// given RefId.
template<class ...Components>
gsl::not_null<Ogre::SceneNode *>
attachAll(gsl::not_null<Ogre::SceneNode *> node,
          RefId refId,
          gsl::not_null<btDiscreteDynamicsWorld *> world,
          const ecs::Entity<Components...> &entity) {
  node = attachAllImpl<0>(node, refId, world, ecs::subsetOf<0>{}(entity));

  constexpr int maxIndex = ecs::maxIndexOf<Components...>();
  if constexpr (maxIndex != 0) {
    // Even though we know the size at compile time, using a std::array is
    // awkward because we can't value initialize.
    // TODO: Find a nice way to aggregate initialize by calling a lambda N times
    std::vector<gsl::not_null<Ogre::SceneNode *>> roots;
    roots.reserve(static_cast<std::size_t>(maxIndex));
    std::generate_n(std::back_inserter(roots), maxIndex, [node]() {
      return gsl::make_not_null(node->createChildSceneNode());
    });

    using Indices = boost::mp11::mp_iota_c<maxIndex>;
    boost::mp11::mp_for_each<Indices>([&](auto I) {
      roots[I] = attachAllImpl<I + 1>(roots[I], refId, world,
                                      ecs::subsetOf<I + 1>{}(entity));
    });
  }

  return node;
}

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
