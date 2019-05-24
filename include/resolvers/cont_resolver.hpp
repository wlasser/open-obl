#ifndef OPENOBL_CONT_RESOLVER_HPP
#define OPENOBL_CONT_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using ContResolver = Resolver<record::CONT>;
using RefrContResolver = Resolver<record::REFR_CONT, RefId>;

template<> struct CiteRecordImpl<record::CONT> {
  using type = record::REFR_CONT;
  type operator()(const record::CONT &baseRec, tl::optional<RefId> refId);
};

template<> struct ReifyRecordImpl<record::REFR_CONT> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::CONT>;
  type operator()(const record::REFR_CONT &refRec,
                  Ogre::SceneManager *scnMgr,
                  btDiscreteDynamicsWorld *world,
                  resolvers res,
                  Ogre::SceneNode *rootNode);
};

} // namespace oo

#endif // OPENOBL_CONT_RESOLVER_HPP
