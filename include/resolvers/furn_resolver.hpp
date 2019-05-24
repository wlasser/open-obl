#ifndef OPENOBL_FURN_RESOLVER_HPP
#define OPENOBL_FURN_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using FurnResolver = Resolver<record::FURN>;
using RefrFurnResolver = Resolver<record::REFR_FURN, RefId>;

template<> struct CiteRecordImpl<record::FURN> {
  using type = record::REFR_FURN;
  type operator()(const record::FURN &baseRec, tl::optional<RefId> refId);
};

template<> struct ReifyRecordImpl<record::REFR_FURN> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::FURN>;
  type operator()(const record::REFR_FURN &refRec,
                  Ogre::SceneManager *scnMgr,
                  btDiscreteDynamicsWorld *world,
                  resolvers res,
                  Ogre::SceneNode *rootNode);
};

} // namespace oo

#endif // OPENOBL_FURN_RESOLVER_HPP
