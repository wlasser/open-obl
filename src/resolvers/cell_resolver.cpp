#include "esp.hpp"
#include "conversions.hpp"
#include "nifloader/animation.hpp"
#include "nifloader/scene.hpp"
#include "resolvers/cell_resolver.hpp"
#include <Ogre.h>

namespace oo {

std::pair<oo::Resolver<record::CELL>::RecordIterator, bool>
oo::Resolver<record::CELL>::insertOrAppend(oo::BaseId baseId,
                                           const record::CELL &rec,
                                           oo::EspAccessor accessor,
                                           bool isExterior) {
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
  const auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  const auto &entry{it->second.first};

  return entry.second ? *entry.second : entry.first;
}

tl::optional<record::CELL &>
oo::Resolver<record::CELL>::get(oo::BaseId baseId) {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  auto &entry{it->second.first};

  if (!entry.second) entry.second.emplace(entry.first);
  return *entry.second;
}

void
oo::Resolver<record::CELL>::setDetachTime(oo::BaseId baseId, int detachTime) {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return;
  it->second.second.mDetachTime = detachTime;
}

int oo::Resolver<record::CELL>::getDetachTime(oo::BaseId baseId) const {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return 0;
  return it->second.second.mDetachTime;
}

bool oo::Resolver<record::CELL>::contains(oo::BaseId baseId) const {
  return mRecords.find(baseId) != mRecords.end();
}

void oo::Resolver<record::CELL>::load(oo::BaseId baseId,
                                      RefrResolverContext refrCtx,
                                      BaseResolverContext baseCtx) {
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
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  return it->second.second.mReferences;
}

tl::optional<oo::BaseId>
oo::Resolver<record::CELL>::getLandId(oo::BaseId baseId) const {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  return it->second.second.mLandId;
}

template<> void
oo::Resolver<record::CELL>::CellVisitor::readRecord<record::REFR>(oo::EspAccessor &accessor) {
  const BaseId baseId{accessor.peekBaseId()};

  const auto &statRes{oo::getResolver<record::STAT>(mBaseCtx)};
  const auto &doorRes{oo::getResolver<record::DOOR>(mBaseCtx)};
  const auto &lighRes{oo::getResolver<record::LIGH>(mBaseCtx)};
  const auto &actiRes{oo::getResolver<record::ACTI>(mBaseCtx)};

  auto &refrStatRes{oo::getRefrResolver<record::REFR_STAT>(mRefrCtx)};
  auto &refrDoorRes{oo::getRefrResolver<record::REFR_DOOR>(mRefrCtx)};
  auto &refrLighRes{oo::getRefrResolver<record::REFR_LIGH>(mRefrCtx)};
  auto &refrActiRes{oo::getRefrResolver<record::REFR_ACTI>(mRefrCtx)};

  if (statRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_STAT>().value};
    refrStatRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mMeta.mReferences.emplace(ref.mFormId);
  } else if (doorRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_DOOR>().value};
    refrDoorRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mMeta.mReferences.emplace(ref.mFormId);
  } else if (lighRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_LIGH>().value};
    refrLighRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
    mMeta.mReferences.emplace(ref.mFormId);
  } else if (actiRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_ACTI>().value};
    refrActiRes.insertOrAssignEspRecord(oo::RefId{ref.mFormId}, ref);
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

InteriorCell::InteriorCell(oo::BaseId baseId, std::string name,
                           std::unique_ptr<PhysicsWorld> physicsWorld)
    : Cell(baseId, std::move(name)),
      mScnMgr(Ogre::Root::getSingleton().createSceneManager()),
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
  mScnMgr->destroySceneNode(mRootSceneNode);
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

    // TODO: Directional lighting, fog, water, etc.
  } else {
    cell->getSceneManager()
        ->setAmbientLight(Ogre::ColourValue{1.0f, 1.0f, 1.0f});
  }

  cell->getPhysicsWorld()->setGravity({0.0f, -9.81f, 0.0f});

  const auto refs{cellRes.getReferences(BaseId{refRec.mFormId})};
  if (!refs) return cell;

  const auto &statRes{oo::getResolver<record::STAT>(resolvers)};
  const auto &doorRes{oo::getResolver<record::DOOR>(resolvers)};
  const auto &lighRes{oo::getResolver<record::LIGH>(resolvers)};
  const auto &actiRes{oo::getResolver<record::ACTI>(resolvers)};
  const auto &npc_Res{oo::getResolver<record::NPC_>(resolvers)};
  const auto &raceRes{oo::getResolver<record::RACE>(resolvers)};

  const auto &refrStatRes{oo::getRefrResolver<record::REFR_STAT>(resolvers)};
  const auto &refrDoorRes{oo::getRefrResolver<record::REFR_DOOR>(resolvers)};
  const auto &refrLighRes{oo::getRefrResolver<record::REFR_LIGH>(resolvers)};
  const auto &refrActiRes{oo::getRefrResolver<record::REFR_ACTI>(resolvers)};
  const auto &refrNpc_Res{oo::getRefrResolver<record::REFR_NPC_>(resolvers)};

  gsl::not_null<Ogre::SceneNode *> rootNode{cell->getRootSceneNode()};

  for (auto refId : *refs) {
    gsl::not_null<Ogre::SceneNode *> node{rootNode->createChildSceneNode()};
    if (auto stat{refrStatRes.get(refId)}; stat) {
      cell->attach(*stat, node, std::forward_as_tuple(statRes));
    } else if (auto door{refrDoorRes.get(refId)}; door) {
      cell->attach(*door, node, std::forward_as_tuple(doorRes));
    } else if (auto ligh{refrLighRes.get(refId)}; ligh) {
      cell->attach(*ligh, node, std::forward_as_tuple(lighRes));
    } else if (auto acti{refrActiRes.get(refId)}; acti) {
      cell->attach(*acti, node, std::forward_as_tuple(actiRes));
    } else if (auto npc{refrNpc_Res.get(refId)}; npc) {
      cell->attach(*npc, node, std::forward_as_tuple(npc_Res, raceRes));
    }
  }

  // TODO: Testing only, remove this
  {
    auto &skelMgr{Ogre::SkeletonManager::getSingleton()};
    auto skelPtr{skelMgr.getByName("meshes/characters/_male/skeleton.nif",
                                   oo::RESOURCE_GROUP)};
    skelPtr->load();
    Ogre::Animation *anim = [&]() {
      if (skelPtr->hasAnimation("Idle")) return skelPtr->getAnimation("Idle");
      return oo::createAnimation(skelPtr.get(),
                                 "meshes/characters/_male/idle.kf",
                                 oo::RESOURCE_GROUP);
    }();

    auto makeNode = [&](const std::string &name) -> Ogre::Entity * {
      auto *node{oo::insertNif(name, oo::RESOURCE_GROUP,
                               cell->getSceneManager(),
                               cell->getPhysicsWorld(),
                               rootNode)};
      auto *entity{dynamic_cast<Ogre::Entity *>(node->getAttachedObject(0))};

      entity->getMesh()->setSkeletonName(skelPtr->getName());
      entity->_initialise(true);

      auto *animState{entity->getAnimationState(anim->getName())};
      animState->setEnabled(true);
      animState->setTimePosition(0.0f);

      node->setPosition({2.0f, -6.0f, 0.0f});

      return entity;
    };

    auto *upperbody{makeNode("meshes/characters/_male/femaleupperbody.nif")};
    for (const auto &name : {"meshes/characters/_male/femalelowerbody.nif",
                             "meshes/characters/_male/femalehand.nif",
                             "meshes/characters/_male/femalefoot.nif"}) {
      makeNode(name);
    }

    oo::attachRagdoll("meshes/characters/_male/skeleton.nif",
                      oo::RESOURCE_GROUP,
                      cell->getSceneManager(),
                      cell->getPhysicsWorld(),
                      gsl::make_not_null(upperbody));
  }

  return cell;
}

void Cell::setNodeTransform(Ogre::SceneNode *node,
                            const record::raw::REFRTransformation &transform) {
  const auto &data{transform.positionRotation.data};

  node->setPosition(oo::fromBSCoordinates(
      Ogre::Vector3{data.x, data.y, data.z}));

  // Rotations are extrinsic rotations about the z, y, then x axes.
  // Positive rotations refer to clockwise rotations, not anticlockwise.
  // This can no doubt be optimized by constructing a quaternion directly from
  // the angle data, building in the coordinate change, but building a rotation
  // matrix and changing coordinates was conceptually simpler.
  Ogre::Matrix3 rotX, rotY, rotZ;
  rotX.FromAngleAxis(Ogre::Vector3::UNIT_X, Ogre::Radian(-data.aX));
  rotY.FromAngleAxis(Ogre::Vector3::UNIT_Y, Ogre::Radian(-data.aY));
  rotZ.FromAngleAxis(Ogre::Vector3::UNIT_Z, Ogre::Radian(-data.aZ));
  const auto rotMat{oo::fromBSCoordinates(rotX * rotY * rotZ)};
  const Ogre::Quaternion rotation{rotMat};
  node->rotate(rotation, Ogre::SceneNode::TS_WORLD);
}

void Cell::setNodeScale(Ogre::SceneNode *node,
                        const record::raw::REFRScalable &scalable) {
  if (scalable.scale) {
    const float scale{scalable.scale->data};
    node->setScale(scale, scale, scale);
  }
}

} // namespace oo
