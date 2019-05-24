#ifndef OPENOBL_MISC_RESOLVER_HPP
#define OPENOBL_MISC_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using MiscResolver = Resolver<record::MISC>;
using RefrMiscResolver = Resolver<record::REFR_MISC, RefId>;

template<> struct CiteRecordImpl<record::MISC> {
  using type = record::REFR_MISC;
  type operator()(const record::MISC &baseRec, tl::optional<RefId> refId);
};

template<>
struct ReifyRecordImpl<record::REFR_MISC> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::MISC>;
  type operator()(const record::REFR_MISC &refRec,
                  Ogre::SceneManager *scnMgr,
                  btDiscreteDynamicsWorld *world,
                  resolvers res,
                  Ogre::SceneNode *rootNode);
};

} // namespace oo

#endif // OPENOBL_MISC_RESOLVER_HPP
