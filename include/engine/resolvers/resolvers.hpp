#ifndef OPENOBLIVION_ENGINE_RESOLVERS_HPP
#define OPENOBLIVION_ENGINE_RESOLVERS_HPP

// At runtime it is necessary to resolve `FormID`s into concrete instances of
// the types they represent, for example producing an `Ogre::Entity` and
// `Ogre::RigidBody` from the base id of a `STAT`. Sometimes it is feasible to
// load every instance of a type during esp parsing and keep them in memory for
// the duration of the application, other times loading should be deferred until
// necessary. In the latter case the esp should still only be parsed once, so a
// map of `FormID`s to file offsets should be stored to speed up loading from
// the disk when needed. Sometimes a small amount of information is needed from
// an object without loading it completely (e.g. the activation prompt for a
// `DOOR` should show the name of its linked `CELL`, but the entire `CELL` does
// not need to be loaded until the door is opened), so there must be more than
// one 'get' method.
// Since the caller should know before the call which type they except the
// `FormID` to resolve to, the resolution can be performed by a different class
// for each type, un-creatively called a `Resolver`. For the sake of consistency
// (and possible use of meta-programming later) a `Resolver` should look
// something like
//```
//class Resolver {
// public:
//  using get_t = /* ... */
//  using peek_t = /* ... */
//
//  get_t get(FormID, Ogre::SceneManager *) const;
//  peek_t peek(FormID) const;
//
// private:
//  using store_t = /* ... */
//  using map_t = std::unordered_map<FormID, store_t>;

//  map_t map;
//};
//```
// Obviously the private `map` is less of a requirement than the public
// interface. The idea is that a `Processor` will populate the `Resolver`'s
// internal map of `FormID`s with `store_t` instances containing sufficient
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

#endif // OPENOBLIVION_ENGINE_RESOLVERS_HPP
