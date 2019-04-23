#include "esp.hpp"
#include "math/conversions.hpp"
#include "nifloader/animation.hpp"
#include "nifloader/scene.hpp"
#include "resolvers/cell_resolver.hpp"
#include <OgreRoot.h>
#include <OgreSceneNode.h>
#include <spdlog/fmt/ostr.h>
#include <mutex>

namespace oo {

Resolver<record::CELL>::Resolver(Resolver &&other) noexcept
    : mBulletConf(other.mBulletConf) {
  std::scoped_lock lock{other.mMtx};
  mRecords = std::move(other.mRecords);
}

Resolver<record::CELL> &Resolver<record::CELL>::operator=(Resolver &&other) noexcept {
  if (this != &other) {
    assert(&other.mBulletConf == &mBulletConf);
    std::scoped_lock lock{mMtx, other.mMtx};
    using std::swap;
    swap(mRecords, other.mRecords);
  }

  return *this;
}

std::pair<oo::Resolver<record::CELL>::RecordIterator, bool>
oo::Resolver<record::CELL>::insertOrAppend(oo::BaseId baseId,
                                           const record::CELL &rec,
                                           oo::EspAccessor accessor,
                                           bool isExterior) {
  std::scoped_lock lock{mMtx};
  RecordEntry entry{std::make_pair(rec, tl::nullopt)};
  Metadata meta{0, isExterior, {accessor}, {}, tl::nullopt};
  auto[it, inserted]{mRecords.try_emplace(baseId, entry, meta)};
  if (inserted) return {it, inserted};

  auto &wrappedEntry{it->second};
  wrappedEntry.second.mAccessors.push_back(accessor);
  wrappedEntry.first = std::make_pair(rec, tl::nullopt);

  return {it, inserted};
}

const bullet::Configuration &
oo::Resolver<record::CELL>::getBulletConfiguration() const {
  return mBulletConf;
}

tl::optional<const record::CELL &>
oo::Resolver<record::CELL>::get(oo::BaseId baseId) const {
  std::scoped_lock lock{mMtx};
  const auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  const auto &entry{it->second.first};

  return entry.second ? *entry.second : entry.first;
}

tl::optional<record::CELL &>
oo::Resolver<record::CELL>::get(oo::BaseId baseId) {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  auto &entry{it->second.first};

  if (!entry.second) entry.second.emplace(entry.first);
  return *entry.second;
}

void
oo::Resolver<record::CELL>::setDetachTime(oo::BaseId baseId, int detachTime) {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return;
  it->second.second.mDetachTime = detachTime;
}

int oo::Resolver<record::CELL>::getDetachTime(oo::BaseId baseId) const {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return 0;
  return it->second.second.mDetachTime;
}

bool oo::Resolver<record::CELL>::contains(oo::BaseId baseId) const {
  std::scoped_lock lock{mMtx};
  return mRecords.find(baseId) != mRecords.end();
}

void oo::Resolver<record::CELL>::load(oo::BaseId baseId,
                                      RefrResolverContext refrCtx,
                                      BaseResolverContext baseCtx) {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return;
  auto &meta{it->second.second};
  meta.mReferences.clear();

  CellVisitor visitor(meta, refrCtx, baseCtx);
  // Taking accessors by value so subsequent reads will work
  for (auto accessor : meta.mAccessors) {
    oo::readCellChildren(accessor, visitor, visitor, visitor);
  }
}

void oo::Resolver<record::CELL>::loadTerrain(oo::BaseId baseId,
                                             MoreResolverContext moreCtx) {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return;
  auto &meta{it->second.second};
  CellTerrainVisitor visitor(meta, moreCtx);
  // Taking accessors by reference since readCellTerrain takes by value
  for (const auto &accessor : meta.mAccessors) {
    oo::readCellTerrain(accessor, visitor);
  }
}

tl::optional<const absl::flat_hash_set<RefId> &>
oo::Resolver<record::CELL>::getReferences(oo::BaseId baseId) const {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  return it->second.second.mReferences;
}

tl::optional<oo::BaseId>
oo::Resolver<record::CELL>::getLandId(oo::BaseId baseId) const {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  return it->second.second.mLandId;
}

void oo::Resolver<record::CELL>::insertReferenceRecord(oo::BaseId cellId,
                                                       oo::RefId refId) {
  std::scoped_lock lock{mMtx};
  auto it{mRecords.find(cellId)};
  if (it == mRecords.end()) return;
  it->second.second.mReferences.insert(refId);
}

template<> void
oo::Resolver<record::CELL>::CellVisitor::readRecord<record::REFR>(oo::EspAccessor &accessor) {
  const BaseId baseId{accessor.peekBaseId()};

  const auto &actiRes{oo::getResolver<record::ACTI>(mBaseCtx)};
  const auto &doorRes{oo::getResolver<record::DOOR>(mBaseCtx)};
  const auto &lighRes{oo::getResolver<record::LIGH>(mBaseCtx)};
  const auto &miscRes{oo::getResolver<record::MISC>(mBaseCtx)};
  const auto &statRes{oo::getResolver<record::STAT>(mBaseCtx)};

  auto &refrActiRes{oo::getRefrResolver<record::REFR_ACTI>(mRefrCtx)};
  auto &refrDoorRes{oo::getRefrResolver<record::REFR_DOOR>(mRefrCtx)};
  auto &refrLighRes{oo::getRefrResolver<record::REFR_LIGH>(mRefrCtx)};
  auto &refrMiscRes{oo::getRefrResolver<record::REFR_MISC>(mRefrCtx)};
  auto &refrStatRes{oo::getRefrResolver<record::REFR_STAT>(mRefrCtx)};

  if (actiRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_ACTI>().value};
    refrActiRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mMeta.mReferences.emplace(ref.mFormId);
  } else if (doorRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_DOOR>().value};
    refrDoorRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mMeta.mReferences.emplace(ref.mFormId);
  } else if (lighRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_LIGH>().value};
    refrLighRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mMeta.mReferences.emplace(ref.mFormId);
  } else if (miscRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_MISC>().value};
    refrMiscRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mMeta.mReferences.emplace(ref.mFormId);
  } else if (statRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_STAT>().value};
    refrStatRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mMeta.mReferences.emplace(ref.mFormId);
  } else {
    accessor.skipRecord();
  }
}

template<> void
oo::Resolver<record::CELL>::CellVisitor::readRecord<record::ACHR>(oo::EspAccessor &accessor) {
  const BaseId baseId{accessor.peekBaseId()};

  const auto &npc_Res{oo::getResolver<record::NPC_>(mBaseCtx)};

  auto &refrNpc_Res{oo::getRefrResolver<record::REFR_NPC_>(mRefrCtx)};

  if (npc_Res.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_NPC_>().value};
    refrNpc_Res.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mMeta.mReferences.emplace(ref.mFormId);
  } else {
    accessor.skipRecord();
  }
}

template<> void
oo::Resolver<record::CELL>::CellTerrainVisitor::readRecord<record::LAND>(oo::EspAccessor &accessor) {
  const auto rec{accessor.readRecord<record::LAND>().value};
  const oo::BaseId baseId{rec.mFormId};
  oo::getResolver<record::LAND>(mMoreCtx).insertOrAssignEspRecord(baseId, rec);
  mMeta.mLandId.emplace(baseId);
}

oo::BaseId Cell::getBaseId() const {
  return mBaseId;
}

std::string Cell::getName() const {
  return mName;
}

void Cell::setName(std::string name) {
  mName = std::move(name);
}

bool Cell::isVisible() const noexcept {
  return mIsVisible;
}

void Cell::setVisibleImpl(bool /*visible*/) {}

void Cell::destroyMovableObjects(Ogre::SceneNode *root) {
  if (!root) return;

  // Detached objects are notified of their detachment so each object must be
  // detached before it is destroyed.

  // Repeatedly detach and destroy the last element.
  auto &objects{root->getAttachedObjects()};
  while (!objects.empty()) {
    // Ogre pops and swaps to remove the object and needs an O(n) lookup if
    // we detach by pointer instead of index.
    auto *obj{root->detachObject(objects.size() - 1)};
    if (auto *rigidBody{dynamic_cast<Ogre::RigidBody *>(obj)}) {
      getPhysicsWorld()->removeRigidBody(rigidBody->getRigidBody());
    }
    getSceneManager()->destroyMovableObject(obj);
  }

  for (auto *node : root->getChildren()) {
    destroyMovableObjects(dynamic_cast<Ogre::SceneNode *>(node));
  }
};

void Cell::showNode(gsl::not_null<Ogre::SceneNode *> root) {
  root->setVisible(true, /*cascade=*/false);
  auto &objects{root->getAttachedObjects()};
  for (Ogre::MovableObject *obj : objects) {
    if (auto *rigidBody{dynamic_cast<Ogre::RigidBody *>(obj)}) {
      bullet::addRigidBody(getPhysicsWorld(), gsl::make_not_null(rigidBody));
    }
  }
}

void Cell::hideNode(gsl::not_null<Ogre::SceneNode *> root) {
  root->setVisible(false, /*cascade=*/false);
  auto &objects{root->getAttachedObjects()};
  for (Ogre::MovableObject *obj : objects) {
    if (auto *rigidBody{dynamic_cast<Ogre::RigidBody *>(obj)}) {
      bullet::removeRigidBody(getPhysicsWorld(), gsl::make_not_null(rigidBody));
    }
  }
}

void Cell::setVisible(bool visible) {
  if (visible == isVisible()) return;

  std::function<void(Ogre::SceneNode *)> show = [&](Ogre::SceneNode *root) {
    showNode(gsl::make_not_null(root));
    for (Ogre::Node *node : root->getChildren()) {
      show(static_cast<Ogre::SceneNode *>(node));
    }
  };

  std::function<void(Ogre::SceneNode *)> hide = [&](Ogre::SceneNode *root) {
    hideNode(gsl::make_not_null(root));
    for (Ogre::Node *node : root->getChildren()) {
      hide(static_cast<Ogre::SceneNode *>(node));
    }
  };

  if (visible) {
    mIsVisible = true;
    show(getRootSceneNode());
    setVisibleImpl(true);
  } else {
    mIsVisible = false;
    hide(getRootSceneNode());
    setVisibleImpl(false);
  }
}

InteriorCell::InteriorCell(oo::BaseId baseId, std::string name,
                           std::unique_ptr<PhysicsWorld> physicsWorld)
    : Cell(baseId, std::move(name)),
      mScnMgr(Ogre::Root::getSingleton().createSceneManager(
          "oo::InteriorSceneManager")),
      mPhysicsWorld(std::move(physicsWorld)) {}

gsl::not_null<Ogre::SceneManager *> InteriorCell::getSceneManager() const {
  return mScnMgr;
}

gsl::not_null<Cell::PhysicsWorld *> InteriorCell::getPhysicsWorld() const {
  return gsl::make_not_null(mPhysicsWorld.get());
}

gsl::not_null<Ogre::SceneNode *> InteriorCell::getRootSceneNode() const {
  return gsl::make_not_null(mScnMgr->getRootSceneNode());
}

InteriorCell::~InteriorCell() {
  // Destruct physics world to unregister all existing rigid bodies and free
  // their broadphase proxies, while they are still alive.
  mPhysicsWorld.reset();
  // Now destruct the scene manager, which destructs all the (now worldless)
  // rigid bodies.
  auto root{Ogre::Root::getSingletonPtr()};
  if (root) root->destroySceneManager(mScnMgr);
}

ExteriorCell::ExteriorCell(oo::BaseId baseId, std::string name,
                           gsl::not_null<Ogre::SceneManager *> scnMgr,
                           gsl::not_null<PhysicsWorld *> physicsWorld)
    : Cell(baseId, std::move(name)),
      mScnMgr(scnMgr),
      mPhysicsWorld(physicsWorld),
      mRootSceneNode(gsl::make_not_null(
          mScnMgr->getRootSceneNode()->createChildSceneNode(
              getBaseId().string()))) {}

ExteriorCell::~ExteriorCell() {
  // TODO: If the cell is hidden then rigid bodies have already been removed so
  //       destroyMovableObjects cannot be used; we can improve performance by
  //       changing destroyMovableObjects instead of showing the cell before
  //       destruction then removing its terrain collision object (which is
  //       re-added by setVisible if previously removed).
  setVisible(true);
  if (mTerrainCollisionObject) {
    mPhysicsWorld->removeCollisionObject(mTerrainCollisionObject.get());
  }

  destroyMovableObjects(mRootSceneNode);
  mRootSceneNode->removeAndDestroyAllChildren();
  mScnMgr->destroySceneNode(mRootSceneNode);
  spdlog::get(oo::LOG)->info("Destroying cell {}", this->getBaseId());
}

void ExteriorCell::setVisibleImpl(bool visible) {
  if (visible) {
    mPhysicsWorld->addCollisionObject(mTerrainCollisionObject.get());
  } else {
    mPhysicsWorld->removeCollisionObject(mTerrainCollisionObject.get());
  }
}

gsl::not_null<Ogre::SceneManager *> ExteriorCell::getSceneManager() const {
  return mScnMgr;
}

gsl::not_null<Cell::PhysicsWorld *> ExteriorCell::getPhysicsWorld() const {
  return mPhysicsWorld;
}

gsl::not_null<Ogre::SceneNode *> ExteriorCell::getRootSceneNode() const {
  return mRootSceneNode;
}

btCollisionObject *ExteriorCell::getCollisionObject() const {
  return mTerrainCollisionObject.get();
}

void ExteriorCell::setTerrain(std::array<Ogre::Terrain *, 4> terrain) {
  mTerrain = terrain;

  // Join the four quadrants of terrain together, swapping the quadrants
  // vertically and reversing the rows in each quadrant.
  const float *h0{terrain[0]->getHeightData()};
  for (std::size_t j = 0; j < 17u; ++j) {
    std::memcpy(&mTerrainHeights[33u * (j + 16u)],
                &h0[17u * (16u - j)],
                17u * 4u);
  }

  const float *h1{terrain[1]->getHeightData()};
  for (std::size_t j = 0; j < 17u; ++j) {
    std::memcpy(&mTerrainHeights[33u * (j + 16u) + 16u],
                &h1[17u * (16u - j)],
                17u * 4u);
  }

  const float *h2{terrain[2]->getHeightData()};
  for (std::size_t j = 0; j < 17u; ++j) {
    std::memcpy(&mTerrainHeights[33u * j], &h2[17u * (16u - j)], 17u * 4u);
  }

  const float *h3{terrain[3]->getHeightData()};
  for (std::size_t j = 0; j < 17u; ++j) {
    std::memcpy(&mTerrainHeights[33u * j + 16u],
                &h3[17u * (16u - j)],
                17u * 4u);
  }

  const float width{4096.0f / oo::metersPerUnit<float>};
  mTerrainCollisionShape = std::make_unique<btHeightfieldTerrainShape>(
      33, 33, mTerrainHeights.data(), 1.0f,
      -width / 2.0f, width / 2.0f,
      1, PHY_ScalarType::PHY_FLOAT, false);

  // By default each grid square is 1m^2, so we need to scale up.
  mTerrainCollisionShape->setLocalScaling(qvm::convert_to<btVector3>(
      qvm::X1X(4096.0f * oo::metersPerUnit<float> / 32.0f)));

  mTerrainCollisionObject = std::make_unique<btCollisionObject>();
  mTerrainCollisionObject->setCollisionShape(mTerrainCollisionShape.get());
  const auto pos{mTerrain[0]->getPosition() / 4.0f
                     + mTerrain[1]->getPosition() / 4.0f
                     + mTerrain[2]->getPosition() / 4.0f
                     + mTerrain[3]->getPosition() / 4.0f};
  btTransform trans(btMatrix3x3::getIdentity(),
                    qvm::convert_to<btVector3>(pos));
  mTerrainCollisionObject->setWorldTransform(trans);
}

oo::ReifyRecordTrait<record::CELL>::type
reifyRecord(const record::CELL &refRec,
            Ogre::SceneManager *scnMgr,
            btDiscreteDynamicsWorld *physicsWorld,
            oo::ReifyRecordTrait<record::CELL>::resolvers resolvers) {
  const auto &cellRes{std::get<const oo::Resolver<record::CELL> &>(resolvers)};
  const auto &bulletConf{cellRes.getBulletConfiguration()};

  const oo::BaseId baseId{refRec.mFormId};
  std::string name{refRec.name ? refRec.name->data : ""};

  auto cell = [&]() -> std::shared_ptr<oo::Cell> {
    if (scnMgr && physicsWorld) {
      return std::make_shared<oo::ExteriorCell>(
          baseId, name,
          gsl::make_not_null(scnMgr),
          gsl::make_not_null(physicsWorld));
    } else {
      return std::make_shared<oo::InteriorCell>(
          baseId, name,
          bulletConf.makeDynamicsWorld());
    }
  }();

  return oo::populateCell(std::move(cell), refRec, std::move(resolvers));
}

ReifyRecordTrait<record::CELL>::type
populateCell(std::shared_ptr<oo::Cell> cell, const record::CELL &refRec,
             ReifyRecordTrait<record::CELL>::resolvers resolvers) {
  const auto &cellRes{std::get<const oo::Resolver<record::CELL> &>(resolvers)};

  if (auto lighting{refRec.lighting}; lighting) {
    Ogre::ColourValue ambient{};
    ambient.setAsABGR(lighting->data.ambient.v);
    cell->getSceneManager()->setAmbientLight(ambient);

    // Shaders expect fog, so if fog no fog then we just set it to be far away.
    const float fogNear{lighting->data.fogNear * oo::metersPerUnit<float>};
    const float fogFar = [&]() {
      const float f{lighting->data.fogFar * oo::metersPerUnit<float>};
      return Ogre::Math::RealEqual(f, 0.0f, 0.1f) ? 10'000.0f : f;
    }();
    Ogre::ColourValue fog{};
    fog.setAsABGR(lighting->data.fogColor.v);
    cell->getSceneManager()->setFog(Ogre::FogMode::FOG_LINEAR, fog, 0,
                                    fogNear, fogFar);

    // TODO: Directional lighting, water, etc.
  }

  cell->getPhysicsWorld()->setGravity({0.0f, -9.81f, 0.0f});

  const auto refs{cellRes.getReferences(BaseId{refRec.mFormId})};
  if (!refs) return cell;

  const auto &raceRes{oo::getResolver<record::RACE>(resolvers)};
  const auto &actiRes{oo::getResolver<record::ACTI>(resolvers)};
  const auto &doorRes{oo::getResolver<record::DOOR>(resolvers)};
  const auto &lighRes{oo::getResolver<record::LIGH>(resolvers)};
  const auto &miscRes{oo::getResolver<record::MISC>(resolvers)};
  const auto &statRes{oo::getResolver<record::STAT>(resolvers)};
  const auto &npc_Res{oo::getResolver<record::NPC_>(resolvers)};

  const auto &refrActiRes{oo::getRefrResolver<record::REFR_ACTI>(resolvers)};
  const auto &refrDoorRes{oo::getRefrResolver<record::REFR_DOOR>(resolvers)};
  const auto &refrLighRes{oo::getRefrResolver<record::REFR_LIGH>(resolvers)};
  const auto &refrMiscRes{oo::getRefrResolver<record::REFR_MISC>(resolvers)};
  const auto &refrStatRes{oo::getRefrResolver<record::REFR_STAT>(resolvers)};
  const auto &refrNpc_Res{oo::getRefrResolver<record::REFR_NPC_>(resolvers)};

  for (auto refId : *refs) {
    if (auto acti{refrActiRes.get(refId)}; acti) {
      cell->attach(*acti, std::forward_as_tuple(actiRes));
    } else if (auto door{refrDoorRes.get(refId)}; door) {
      cell->attach(*door, std::forward_as_tuple(doorRes));
    } else if (auto ligh{refrLighRes.get(refId)}; ligh) {
      cell->attach(*ligh, std::forward_as_tuple(lighRes));
    } else if (auto misc{refrMiscRes.get(refId)}; misc) {
      cell->attach(*misc, std::forward_as_tuple(miscRes));
    } else if (auto stat{refrStatRes.get(refId)}; stat) {
      cell->attach(*stat, std::forward_as_tuple(statRes));
    } else if (auto npc{refrNpc_Res.get(refId)}; npc) {
      cell->attach(*npc, std::forward_as_tuple(npc_Res, raceRes));
    }
  }

  return cell;
}

void Cell::setNodeTransform(gsl::not_null<Ogre::SceneNode *> node,
                            const record::raw::REFRTransformation &transform) {
  const auto &data{transform.positionRotation.data};

  node->resetToInitialState();

  node->setPosition(oo::fromBSCoordinates(
      Ogre::Vector3{data.x, data.y, data.z}));

  const auto rotation{oo::fromBSTaitBryan(Ogre::Radian(data.aX),
                                          Ogre::Radian(data.aY),
                                          Ogre::Radian(data.aZ))};
  node->rotate(rotation, Ogre::SceneNode::TS_WORLD);

  std::function<void(Ogre::SceneNode *)> notify = [&](Ogre::SceneNode *node) {
    for (Ogre::Node *child : node->getChildren()) {
      if (auto *sceneChild{dynamic_cast<Ogre::SceneNode *>(child)}) {
        notify(sceneChild);
      }
    }

    for (Ogre::MovableObject *obj : node->getAttachedObjects()) {
      if (auto *rigidBody{dynamic_cast<Ogre::RigidBody *>(obj)}) {
        rigidBody->notify();
      }
    }
  };

  notify(node);
}

void Cell::setNodeScale(gsl::not_null<Ogre::SceneNode *> node,
                        const record::raw::REFRScalable &scalable) {
  if (scalable.scale) {
    const float scale{scalable.scale->data};
    node->setScale(scale, scale, scale);

    std::function<void(Ogre::SceneNode *)> notify = [&](Ogre::SceneNode *node) {
      for (Ogre::Node *child : node->getChildren()) {
        if (auto *sceneChild{dynamic_cast<Ogre::SceneNode *>(child)}) {
          notify(sceneChild);
        }
      }

      for (Ogre::MovableObject *obj : node->getAttachedObjects()) {
        if (auto *rigidBody{dynamic_cast<Ogre::RigidBody *>(obj)}) {
          rigidBody->setScale({scale, scale, scale});
        }
      }
    };

    notify(node);
  }
}

} // namespace oo
