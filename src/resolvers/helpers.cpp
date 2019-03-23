#include "resolvers/helpers.hpp"

namespace oo {

void setRefId(gsl::not_null<Ogre::RigidBody *> rigidBody, oo::RefId refId) {
  auto formId{static_cast<oo::FormId>(refId)};
  rigidBody->getRigidBody()->setUserPointer(oo::encodeFormId(formId));
}

void setRefId(gsl::not_null<Ogre::SceneNode *> node, RefId refId) {
  std::function<void(Ogre::SceneNode *)> dfs = [&](Ogre::SceneNode *root) {
    for (auto obj : root->getAttachedObjects()) {
      if (auto *body{dynamic_cast<Ogre::RigidBody *>(obj)}) {
        oo::setRefId(gsl::make_not_null(body), refId);
      }
    }
    for (Ogre::Node *child : root->getChildren()) {
      if (auto *sceneChild{dynamic_cast<Ogre::SceneNode *>(child)}) {
        dfs(sceneChild);
      }
    }
  };

  dfs(node);
}

} // namespace oo
