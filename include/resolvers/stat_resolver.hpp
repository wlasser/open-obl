#ifndef OPENOBL_STAT_RESOLVER_HPP
#define OPENOBL_STAT_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using StatResolver = Resolver<record::STAT>;
using RefrStatResolver = Resolver<record::REFR_STAT, RefId>;

template<> struct CiteRecordImpl<record::STAT> {
  using type = record::REFR_STAT;
  type operator()(const record::STAT &baseRec, tl::optional<RefId> refId);
};

template<>
struct ReifyRecordImpl<record::REFR_STAT> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::STAT>;
  type operator()(const record::REFR_STAT &refRec,
                  Ogre::SceneManager *scnMgr,
                  btDiscreteDynamicsWorld *world,
                  resolvers res,
                  Ogre::SceneNode *rootNode);
};

} // namespace oo

#endif //OPENOBL_STAT_RESOLVER_HPP
