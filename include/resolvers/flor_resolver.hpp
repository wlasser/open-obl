#ifndef OPENOBL_FLOR_RESOLVER_HPP
#define OPENOBL_FLOR_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using FlorResolver = Resolver<record::FLOR>;
using RefrFlorResolver = Resolver<record::REFR_FLOR, RefId>;

template<> struct CiteRecordImpl<record::FLOR> {
  using type = record::REFR_FLOR;
  type operator()(const record::FLOR &baseRec, tl::optional<RefId> refId);
};

template<> struct ReifyRecordImpl<record::REFR_FLOR> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::FLOR>;
  type operator()(const record::REFR_FLOR &refRec,
                  Ogre::SceneManager *scnMgr,
                  btDiscreteDynamicsWorld *world,
                  resolvers res,
                  Ogre::SceneNode *rootNode);
};

} // namespace oo

#endif // OPENOBL_FLOR_RESOLVER_HPP
