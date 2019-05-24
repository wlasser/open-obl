#ifndef OPENOBL_LIGH_RESOLVER_HPP
#define OPENOBL_LIGH_RESOLVER_HPP

#include "resolvers/resolvers.hpp"

namespace oo {

using LighResolver = Resolver<record::LIGH>;
using RefrLighResolver = Resolver<record::REFR_LIGH, RefId>;

template<> struct CiteRecordImpl<record::LIGH> {
  using type = record::REFR_LIGH;
  type operator()(const record::LIGH &baseRec, tl::optional<RefId> refId);
};

template<>
struct ReifyRecordImpl<record::REFR_LIGH> {
  using type = Ogre::SceneNode *;
  using resolvers = ResolverTuple<record::LIGH>;
  type operator()(const record::REFR_LIGH &refRec,
                  Ogre::SceneManager *scnMgr,
                  btDiscreteDynamicsWorld *world,
                  resolvers res,
                  Ogre::SceneNode *rootNode);
};

} // namespace oo

#endif //OPENOBL_LIGH_RESOLVER_HPP
