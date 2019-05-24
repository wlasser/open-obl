#ifndef OPENOBL_ACTI_RESOLVER_HPP
#define OPENOBL_ACTI_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using ActiResolver = Resolver<record::ACTI>;
using RefrActiResolver = Resolver<record::REFR_ACTI, RefId>;

template<> struct CiteRecordImpl<record::ACTI> {
  using type = record::REFR_ACTI;
  type operator()(const record::ACTI &baseRec, tl::optional<RefId> refId);
};

template<> struct ReifyRecordImpl<record::REFR_ACTI> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::ACTI>;
  type operator()(const record::REFR_ACTI &refRec,
                  Ogre::SceneManager *scnMgr,
                  btDiscreteDynamicsWorld *world,
                  resolvers res,
                  Ogre::SceneNode *rootNode);
};

} // namespace oo

#endif // OPENOBL_ACTI_RESOLVER_HPP
