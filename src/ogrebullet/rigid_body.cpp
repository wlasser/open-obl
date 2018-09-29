#include "ogrebullet/conversions.hpp"
#include "ogrebullet/collision_object_manager.hpp"
#include "ogrebullet/rigid_body.hpp"

namespace Ogre {

void RigidBody::_notifyAttached(Node *parent, bool isTagPoint) {
  MovableObject::_notifyAttached(parent, isTagPoint);
  bind(parent);
}

void RigidBody::_notifyMoved() {
  // Unsure when this is called; only when the position is changed because of a
  // parent, or even when this->setPosition is called? If the latter, using it
  // to notify the motion state will cause a cycle
  // TODO: When is _notifyMoved called, will it create a cycle?
}

void RigidBody::_updateRenderQueue(RenderQueue *queue) {
  // This is not renderable
}

const AxisAlignedBox &RigidBody::getBoundingBox() const {
  btVector3 min{};
  btVector3 max{};
  mRigidBody->getAabb(min, max);
  mBox.setExtents(conversions::fromBullet(min),
                  conversions::fromBullet(max));
  return mBox;
}

Real RigidBody::getBoundingRadius() const {
  // If we take a sphere centered at the bounding box center, then by symmetry
  // the distance from the center to any corner point is constant, and moreover
  // is the largest distance from the center to any point.
  auto bbox = getBoundingBox();
  auto center = bbox.getCenter();
  return (bbox.getCorner(AxisAlignedBox::FAR_LEFT_BOTTOM) - bbox.getCenter())
      .length();
}

const String &RigidBody::getMovableType() const {
  return mType;
}

void RigidBody::visitRenderables(Renderable::Visitor *visitor,
                                 bool debugRenderables) {
  // This is not renderable
}

btRigidBody *RigidBody::getRigidBody() const {
  return mRigidBody.get();
}

RigidBody::RigidBody(const String &name, CollisionObjectPtr collisionObject)
    : mCollisionObject(std::move(collisionObject)) {
  mName = name;
  if (auto *info = mCollisionObject->getRigidBodyInfo()) {
    mRigidBody = std::make_unique<btRigidBody>(*info);
  } else {
    OGRE_EXCEPT(Exception::ERR_INTERNAL_ERROR,
                "getRigidBodyInfo() == nullptr, cannot create RigidBody",
                "RigidBody::RigidBody");
  }
}

void RigidBody::bind(Node *node) {
  // Reset and delete existing state
  if (mRigidBody) mRigidBody->setMotionState(nullptr);
  mMotionState.reset(nullptr);

  // Allocate and set new state
  if (node) {
    mMotionState = std::make_unique<MotionState>(node);
    if (mRigidBody) {
      mRigidBody->setMotionState(mMotionState.get());
      if (node->getScale() != Vector3{1.0f, 1.0f, 1.0f}) {
        setScale(node->getScale());
      }
    }
  }
}

void RigidBody::notify() {
  if (mMotionState) mMotionState->notify();
}

void RigidBody::setScale(const Vector3 &scale) {
  auto localScale = conversions::toBullet(scale);

  if (mCollisionShapeOverride) {
    mCollisionShapeOverride->setLocalScaling(localScale);
  } else {
    btCollisionShape *base = mCollisionObject->getCollisionShape();

    // We can't copy the base in general
    if (auto *triMesh = dynamic_cast<btBvhTriangleMeshShape *>(base)) {
      mCollisionShapeOverride =
          std::make_unique<btScaledBvhTriangleMeshShape>(triMesh, localScale);
    } else if (auto *convexHull = dynamic_cast<btConvexHullShape *>(base)) {
      auto override = std::make_unique<btConvexHullShape>();
      btVector3 *points = convexHull->getUnscaledPoints();
      for (int i = 0; i < convexHull->getNumPoints(); ++i) {
        override->addPoint(*(points + i));
      }
      mCollisionShapeOverride = std::move(override);
    } else {
      // TODO: Scale other collision shapes
    }

    if (mCollisionShapeOverride) {
      mRigidBody->setCollisionShape(mCollisionShapeOverride.get());
    }
  }
}

void RigidBodyFactory::destroyInstance(MovableObject *obj) {
  OGRE_DELETE obj;
}

const String &RigidBodyFactory::getType() const {
  return mType;
}

MovableObject *RigidBodyFactory::createInstanceImpl(const String &name,
                                                    const NameValuePairList *params) {
  CollisionObjectPtr ptr{};
  String group = ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME;

  if (params) {
    auto it = params->find("resourceGroup");
    if (it != params->end()) group = it->second;

    it = params->find("collisionObject");
    if (it != params->end()) {
      auto retrieveResult = CollisionObjectManager::getSingleton()
          .createOrRetrieve(it->second, group);
      ptr = std::dynamic_pointer_cast<CollisionObject>(retrieveResult.first);
      if (ptr) ptr->load();
    }
  }
  if (!ptr) {
    OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                "'collisionObject' parameter required when constructing a RigidBody",
                "RigidBodyFactory::createInstance");
  }
  // It is possible that resource creation succeeded, but resource loading
  // failed because the CollisionObject could not find any physics data. This is
  // not exceptional behaviour (markers have no physics data, for instance), but
  // it clearly makes no sense to proceed with the construction of a RigidBody.
  // Ogre implicitly assumes that this method never returns nullptr, so we have
  // no choice but to throw an exception.
  if (!ptr->getRigidBodyInfo() || !ptr->getCollisionShape()) {
    throw PartialCollisionObjectException("CollisionObject has no RigidBodyInfo");
  }
  return OGRE_NEW RigidBody(name, std::move(ptr));
}

} // namespace Ogre
