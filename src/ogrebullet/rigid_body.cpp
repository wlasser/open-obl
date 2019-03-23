#include "ogrebullet/conversions.hpp"
#include "ogrebullet/collision_shape_manager.hpp"
#include "ogrebullet/rigid_body.hpp"
#include <gsl/gsl>

namespace Ogre {

void
RigidBody::setObjectType(CollisionShape::CollisionObjectType type) noexcept {
  switch (type) {
    default: [[fallthrough]];
    case CollisionShape::COT_DYNAMIC:
      setCollisionFlag(btCollisionObject::CF_STATIC_OBJECT,
                       false);
      setCollisionFlag(btCollisionObject::CF_KINEMATIC_OBJECT, false);
      break;
    case CollisionShape::COT_STATIC:
      setCollisionFlag(btCollisionObject::CF_STATIC_OBJECT,
                       true);
      setCollisionFlag(btCollisionObject::CF_KINEMATIC_OBJECT, false);
      break;
    case CollisionShape::COT_KINEMATIC:
      setCollisionFlag(btCollisionObject::CF_STATIC_OBJECT,
                       false);
      setCollisionFlag(btCollisionObject::CF_KINEMATIC_OBJECT, true);
      break;
  }
}

void RigidBody::setAllowDeactivationEnabled(bool enabled) noexcept {
  mRigidBody->setActivationState(enabled ? ACTIVE_TAG : DISABLE_DEACTIVATION);
}

bool RigidBody::getAllowDeactivationEnabled() const noexcept {
  return mRigidBody->getActivationState() != DISABLE_DEACTIVATION;
}

CollisionShape::CollisionObjectType RigidBody::getObjectType() const noexcept {
  if (getCollisionFlag(btCollisionObject::CF_STATIC_OBJECT)) {
    return CollisionShape::COT_STATIC;
  } else if (getCollisionFlag(btCollisionObject::CF_KINEMATIC_OBJECT)) {
    return CollisionShape::COT_KINEMATIC;
  } else {
    return CollisionShape::COT_DYNAMIC;
  }
}

void RigidBody::_notifyAttached(Node *parent, bool isTagPoint) {
  MovableObject::_notifyAttached(parent, isTagPoint);

  if (isTagPoint) {
    setObjectType(CollisionShape::COT_KINEMATIC);
  } else {
    setObjectType(mCollisionShape->getCollisionObjectType());
    bind(parent);
  }
}

void RigidBody::_notifyMoved() {
  // Unsure when this is called; only when the position is changed because of a
  // parent, or even when this->setPosition is called? If the latter, using it
  // to notify the motion state will cause a cycle
  // TODO: When is _notifyMoved called, will it create a cycle?
  if (!mMotionState) {
    Ogre::Affine3 ogreTrans{_getParentNodeFullTransform()};
    btTransform bulletTrans(Ogre::toBullet(ogreTrans.linear()),
                            Ogre::toBullet(ogreTrans.getTrans()));
    mRigidBody->setWorldTransform(bulletTrans);
  }
}

void RigidBody::_updateRenderQueue(RenderQueue */*queue*/) {
  // This is not renderable
}

const AxisAlignedBox &RigidBody::getBoundingBox() const {
  btVector3 min{};
  btVector3 max{};
  mRigidBody->getAabb(min, max);
  mBox.setExtents(Ogre::fromBullet(min),
                  Ogre::fromBullet(max));
  return mBox;
}

Real RigidBody::getBoundingRadius() const {
  // If we take a sphere centered at the bounding box center, then by symmetry
  // the distance from the center to any corner point is constant, and moreover
  // is the largest distance from the center to any point.
  const auto bbox{getBoundingBox()};
  const auto center{bbox.getCenter()};
  return (bbox.getCorner(AxisAlignedBox::FAR_LEFT_BOTTOM) - center).length();
}

const String &RigidBody::getMovableType() const {
  return mType;
}

void RigidBody::visitRenderables(Renderable::Visitor */*visitor*/,
                                 bool /*debugRenderables*/) {
  // This is not renderable
}

btRigidBody *RigidBody::getRigidBody() const {
  return mRigidBody.get();
}

void RigidBody::setCollisionFlag(btCollisionObject::CollisionFlags flag,
                                 bool enabled) noexcept {
  const auto flags{static_cast<flag_t>(mRigidBody->getCollisionFlags())};
  const auto f{static_cast<flag_t>(flag)};
  mRigidBody->setCollisionFlags(enabled ? (flags | f) : (flags & ~f));
}

bool RigidBody::getCollisionFlag(btCollisionObject::CollisionFlags flag) const noexcept {
  const auto f{static_cast<flag_t>(flag)};
  return 0 != (static_cast<flag_t>(mRigidBody->getCollisionFlags()) & f);
}

RigidBody::RigidBody(const String &name, CollisionShapePtr collisionShape)
    : MovableObject(name), mCollisionShape(std::move(collisionShape)) {
  if (const auto *info{mCollisionShape->getRigidBodyInfo()}) {
    mRigidBody = std::make_unique<btRigidBody>(*info);
    setObjectType(mCollisionShape->getCollisionObjectType());
    setAllowDeactivationEnabled(mCollisionShape->getAllowDeactivationEnabled());
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
      if (node->getScale() != Vector3::UNIT_SCALE) {
        setScale(node->getScale());
      }
    }
  }
}

void RigidBody::notify() {
  if (mMotionState) {
    mMotionState->notify();
    // Notifying the motionState is insufficient. We cannot force the
    // btRigidBody to update its transform, and must do it manually.
    btTransform trans{};
    mMotionState->getWorldTransform(trans);
    mRigidBody->setWorldTransform(trans);
  }
}

void RigidBody::setScale(const Vector3 &scale) {
  const auto localScale{Ogre::toBullet(scale)};

  if (mCollisionShapeOverride) {
    mCollisionShapeOverride->setLocalScaling(localScale);
    return;
  }

  const btCollisionShape *base{mCollisionShape->getCollisionShape()};

  // We can't copy the base in general
  if (auto *triMesh{dynamic_cast<const btBvhTriangleMeshShape *>(base)}) {
    mCollisionShapeOverride =
        std::make_unique<btScaledBvhTriangleMeshShape>(triMesh, localScale);
  } else if (auto *convexHull{dynamic_cast<const btConvexHullShape *>(base)}) {
    auto override{std::make_unique<btConvexHullShape>()};
    auto points{gsl::make_span(convexHull->getUnscaledPoints(),
                               convexHull->getNumPoints())};
    const btMatrix3x3 transform{localScale.x(), 0.0f, 0.0f,
                                0.0f, localScale.y(), 0.0f,
                                0.0f, 0.0f, localScale.z()};
    for (const auto &point : points) override->addPoint(transform * point);

    mCollisionShapeOverride = std::move(override);
  } else {
    // TODO: Scale other collision shapes
  }

  if (mCollisionShapeOverride) {
    mRigidBody->setCollisionShape(mCollisionShapeOverride.get());
  }
}

void RigidBodyFactory::destroyInstance(gsl::owner<MovableObject *> obj) {
  OGRE_DELETE obj;
}

const String &RigidBodyFactory::getType() const {
  return mType;
}

gsl::owner<MovableObject *>
RigidBodyFactory::createInstanceImpl(const String &name,
                                     const NameValuePairList *params) {
  CollisionShapePtr ptr{};
  String group{ResourceGroupManager::AUTODETECT_RESOURCE_GROUP_NAME};

  if (params) {
    if (auto it{params->find("resourceGroup")}; it != params->end()) {
      group = it->second;
    }

    if (auto it{params->find("collisionShape")}; it != params->end()) {
      auto &collisionMgr{CollisionShapeManager::getSingleton()};
      auto retrieveResult{collisionMgr.createOrRetrieve(it->second, group)};
      ptr = std::dynamic_pointer_cast<CollisionShape>(retrieveResult.first);
      if (ptr) ptr->load();
    }
  }
  if (!ptr) {
    OGRE_EXCEPT(Exception::ERR_INVALIDPARAMS,
                "'collisionShape' parameter required when constructing a "
                "RigidBody",
                "RigidBodyFactory::createInstance");
  }
  // It is possible that resource creation succeeded, but resource loading
  // failed because the CollisionShape could not find any physics data. This is
  // not exceptional behaviour (markers have no physics data, for instance), but
  // it clearly makes no sense to proceed with the construction of a RigidBody.
  // Ogre implicitly assumes that this method never returns nullptr, so we have
  // no choice but to throw an exception.
  if (!ptr->getRigidBodyInfo() || !ptr->getCollisionShape()) {
    throw PartialCollisionObjectException("CollisionShape has no RigidBodyInfo");
  }
  return OGRE_NEW RigidBody(name, std::move(ptr));
}

} // namespace Ogre
