#include "resolvers/helpers.hpp"
#include <OgreMesh.h>

Ogre::RigidBody *
loadRigidBody(Ogre::Entity *entity, gsl::not_null<Ogre::SceneManager *> mgr) {
  if (!entity) return nullptr;
  const auto &mesh{entity->getMesh()};
  if (!mesh) return nullptr;

  const auto &name{mesh->getName()};
  const auto &group{mesh->getGroup()};
  const std::map<std::string, std::string> params{
      {"collisionObject", name},
      {"resourceGroup", group}
  };

  // Yes, we are using an exception for control flow. It is necessary, see
  // RigidBodyFactory::createInstanceImpl.
  // TODO: Replace with a mgr->createRigidBody on a derived SceneManager
  try {
    auto *const rigidBody{dynamic_cast<Ogre::RigidBody *>(
                              mgr->createMovableObject("RigidBody", &params))};
    return rigidBody;
  } catch (const Ogre::PartialCollisionObjectException &e) {
    return nullptr;
  }
}

gsl::not_null<Ogre::SceneNode *>
attachMesh(gsl::not_null<Ogre::SceneNode *> node,
           Ogre::Entity *mesh,
           bool final) {
  if (!mesh) return node;
  node->attachObject(mesh);
  return final ? node : gsl::make_not_null(node->createChildSceneNode());
}

gsl::not_null<Ogre::SceneNode *>
attachRigidBody(gsl::not_null<Ogre::SceneNode *> node,
                Ogre::RigidBody *rigidBody,
                gsl::not_null<btDiscreteDynamicsWorld *> world,
                bool final) {
  if (!rigidBody) return node;
  node->attachObject(rigidBody);
  // TODO: Replace with rigidBody->attach(world)
  world->addRigidBody(rigidBody->getRigidBody());
  return final ? node : gsl::make_not_null(node->createChildSceneNode());
}

gsl::not_null<Ogre::SceneNode *>
attachLight(gsl::not_null<Ogre::SceneNode *> node,
            Ogre::Light *light,
            bool final) {
  if (!light) return node;
  node->attachObject(light);
  return final ? node : gsl::make_not_null(node->createChildSceneNode());
}

void setRefId(gsl::not_null<Ogre::RigidBody *> rigidBody, RefId refId) {
  auto formId{static_cast<FormId>(refId)};
  rigidBody->getRigidBody()->setUserPointer(encodeFormId(formId));
}
