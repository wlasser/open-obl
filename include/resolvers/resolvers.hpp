#ifndef OPENOBLIVION_RESOLVERS_HPP
#define OPENOBLIVION_RESOLVERS_HPP

#include "formid.hpp"
#include "resolvers/helpers.hpp"
#include <gsl/gsl>
#include <OgreSceneManager.h>
#include <optional>

namespace engine {

// At runtime it is necessary to conver BaseIds into the base records that they
// identify, usually either to view information about the base record or to
// create a concrete realisation of it in the form of a reference record.
// The possibility of the latter operation depends on the type of base record;
// it makes sense to realise a `STAT` or `CONT`, but not a `CLAS`, for example.
// Further to this is the possibility of only loading a stub of a record and
// deferring the full loading until it is necessary. This can reduce memory
// usage and game startup time, and does not require parsing the esp file
// multiple times of a file offset is saved.
// This resolution of a BaseId into a base record or new reference record is
// handled by an uncreatively named `Resolver` for the record type. For the
// sake of consistency, a `Resolver` should look something like
template<class T>
class Resolver {
 public:
  // Replace with actual types
  using get_t = void;
  using peek_t = void;
  using make_t = void;

  // Return the base record, performing disk io if necessary.
  get_t get(BaseId) const;
  // Return a stub of the base record, do not perform disk io.
  peek_t peek(BaseId) const;
  // Return a new instance of the BaseId with the given RefId, or a new one.
  // May perform disk io.
  make_t make(BaseId, Ogre::SceneManager *, std::optional<RefId>) const;
};
// Note that in this context, the returned (base or reference) records do not
// have to have the same layout as the ones used for (de)serialization.
// Moreover, the reference records returned by `make` may contain rendering
// information for the rendering engine. Calling `Resolver<STAT>::make` for
// example should return a reference containing an `Ogre::RigidBody` and
// `Ogre::Entity`. In other words, reference records represent not just concrete
// realisations of base records within the scope of the game engine, but also
// within the scope of the rendering engine. By providing an
// `Ogre::SceneManager` argument, the `Resolver` can construct the reference in-
// place; it should not be the caller's responsibility to know how to link all
// the components together into a scene.

} // namespace engine

#endif // OPENOBLIVION_RESOLVERS_HPP
