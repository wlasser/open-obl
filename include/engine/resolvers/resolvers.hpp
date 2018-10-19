#ifndef OPENOBLIVION_ENGINE_RESOLVERS_HPP
#define OPENOBLIVION_ENGINE_RESOLVERS_HPP

#include "formid.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <gsl/gsl>
#include <OgreEntity.h>
#include <OgreSceneManager.h>

// At runtime it is necessary to resolve BaseIds into concrete instances of
// the types they represent, for example producing an `Ogre::Entity` and
// `Ogre::RigidBody` from the base id of a `STAT`. Sometimes it is feasible to
// load every instance of a type during esp parsing and keep them in memory for
// the duration of the application, other times loading should be deferred until
// necessary. In the latter case the esp should still only be parsed once, so a
// map of `FormId`s to file offsets should be stored to speed up loading from
// the disk when needed. Sometimes a small amount of information is needed from
// an object without loading it completely (e.g. the activation prompt for a
// `DOOR` should show the name of its linked `CELL`, but the entire `CELL` does
// not need to be loaded until the door is opened), so there must be more than
// one 'get' method.
// Since the caller should know before the call which type they except the
// `FormId` to resolve to, the resolution can be performed by a different class
// for each type, un-creatively called a `Resolver`. For the sake of consistency
// (and possible use of meta-programming later) a `Resolver` should look
// something like
//```
//class Resolver {
// public:
//  using get_t = /* ... */
//  using peek_t = /* ... */
//
//  get_t get(FormId, Ogre::SceneManager *) const;
//  peek_t peek(FormId) const;
//
// private:
//  using store_t = /* ... */
//  using map_t = std::unordered_map<FormId, store_t>;

//  map_t map;
//};
//```
// Obviously the private `map` is less of a requirement than the public
// interface. The idea is that a `Processor` will populate the `Resolver`'s
// internal map of `FormId`s with `store_t` instances containing sufficient
// information to build an engine representation of the object directly, or look
// up in the esp file how to build one. `get` should return this representation,
// loading it if necessary, and `peek` should return information about the
// object available without loading or doing any disk io.
// The `Ogre::SceneManager` is provided to `get` so that the `Resolver` can
// construct the representation directly in the scene, if this makes sense. For
// example, `get`ting a `STAT` could return a
// `std::pair<Ogre::Entity*, Ogre::RigidBody*>`, but it should not be the
// caller's responsibility to know how to link the two together in the scene.
// Moreover, this allows passing ownership to the `Ogre::SceneManager` instead
// of the `Resolver` managing that itself.

namespace engine {

// struct T {
//   std::string modelFilename;
// };
template<class T>
Ogre::Entity *loadMesh(const T &rec, Ogre::SceneManager *mgr) {
  if (rec.modelFilename.empty()) return nullptr;
  else return mgr->createEntity(rec.modelFilename);
}

Ogre::RigidBody *loadRigidBody(Ogre::Entity *entity, Ogre::SceneManager *mgr);

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

} // namespace engine

#endif // OPENOBLIVION_ENGINE_RESOLVERS_HPP
