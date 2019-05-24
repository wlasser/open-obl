#ifndef OPENOBL_DOOR_RESOLVER_HPP
#define OPENOBL_DOOR_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using DoorResolver = Resolver<record::DOOR>;
using RefrDoorResolver = Resolver<record::REFR_DOOR, RefId>;

template<> struct CiteRecordImpl<record::DOOR> {
  using type = record::REFR_DOOR;
  type operator()(const record::DOOR &baseRec, tl::optional<RefId> refId);
};

template<> struct ReifyRecordImpl<record::REFR_DOOR> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::DOOR>;
  type operator()(const record::REFR_DOOR &refRec,
                  Ogre::SceneManager *scnMgr,
                  btDiscreteDynamicsWorld *world,
                  resolvers res,
                  Ogre::SceneNode *rootNode);
};

} // namespace oo

#endif // OPENOBL_DOOR_RESOLVER_HPP
