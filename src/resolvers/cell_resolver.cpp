#include "esp.hpp"
#include "conversions.hpp"
#include "resolvers/cell_resolver.hpp"
#include <Ogre.h>

std::pair<Resolver<record::CELL>::RecordIterator, bool>
Resolver<record::CELL>::insertOrAppend(BaseId baseId,
                                       const record::CELL &rec,
                                       esp::EspAccessor accessor) {
  RecordEntry entry{std::make_pair(rec, tl::nullopt)};
  Metadata meta{0, {accessor}, {}};
  auto[it, inserted]{mRecords.try_emplace(baseId, entry, meta)};
  if (inserted) return {it, inserted};

  auto &wrappedEntry{it->second};
  wrappedEntry.second.mAccessors.push_back(accessor);
  wrappedEntry.first = std::make_pair(rec, tl::nullopt);

  return {it, inserted};
}

const bullet::Configuration &
Resolver<record::CELL>::getBulletConfiguration() const {
  return mBulletConf;
}

tl::optional<const record::CELL &>
Resolver<record::CELL>::get(BaseId baseId) const {
  const auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  const auto &entry{it->second.first};

  return entry.second ? *entry.second : entry.first;
}

tl::optional<record::CELL &>
Resolver<record::CELL>::get(BaseId baseId) {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  auto &entry{it->second.first};

  if (!entry.second) entry.second.emplace(entry.first);
  return *entry.second;
}

void Resolver<record::CELL>::setDetachTime(BaseId baseId, int detachTime) {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return;
  it->second.second.mDetachTime = detachTime;
}

int Resolver<record::CELL>::getDetachTime(BaseId baseId) const {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return 0;
  return it->second.second.mDetachTime;
}

bool Resolver<record::CELL>::contains(BaseId baseId) const {
  return mRecords.find(baseId) != mRecords.end();
}

void Resolver<record::CELL>::load(BaseId baseId,
                                  RefrResolverContext refrCtx,
                                  BaseResolverContext baseCtx) {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return;
  auto &meta{it->second.second};
  meta.mReferences.clear();

  CellVisitor visitor(meta, refrCtx, baseCtx);
  // Taking accessors by value so subsequent reads will work
  for (auto accessor : meta.mAccessors) {
    esp::readCellChildren(accessor, visitor, visitor, visitor);
  }
}

tl::optional<const std::vector<RefId> &>
Resolver<record::CELL>::getReferences(BaseId baseId) const {
  auto it{mRecords.find(baseId)};
  if (it == mRecords.end()) return tl::nullopt;
  return it->second.second.mReferences;
}

template<> void
Resolver<record::CELL>::CellVisitor::readRecord<record::REFR>(esp::EspAccessor &accessor) {
  const BaseId baseId{accessor.peekBaseId()};

  const auto &statRes{std::get<const Resolver<record::STAT> &>(mBaseCtx)};
  const auto &doorRes{std::get<const Resolver<record::DOOR> &>(mBaseCtx)};
  const auto &lighRes{std::get<const Resolver<record::LIGH> &>(mBaseCtx)};

  auto &refrStatRes{std::get<Resolver<record::REFR_STAT, RefId> &>(mRefrCtx)};
  auto &refrDoorRes{std::get<Resolver<record::REFR_DOOR, RefId> &>(mRefrCtx)};
  auto &refrLighRes{std::get<Resolver<record::REFR_LIGH, RefId> &>(mRefrCtx)};

  if (statRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_STAT>().value};
    refrStatRes.insertOrAssignEspRecord(RefId{ref.mFormId}, ref);
    mMeta.mReferences.emplace_back(ref.mFormId);
  } else if (doorRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_DOOR>().value};
    refrDoorRes.insertOrAssignEspRecord(RefId{ref.mFormId}, ref);
    mMeta.mReferences.emplace_back(ref.mFormId);
  } else if (lighRes.contains(baseId)) {
    const auto ref{accessor.readRecord<record::REFR_LIGH>().value};
    refrLighRes.insertOrAssignEspRecord(RefId{ref.mFormId}, ref);
    mMeta.mReferences.emplace_back(ref.mFormId);
  } else {
    accessor.skipRecord();
  }
}

Cell::~Cell() {
  auto root{Ogre::Root::getSingletonPtr()};
  if (root) root->destroySceneManager(scnMgr);
  // TODO: No raw loops
  for (int i = physicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
    btCollisionObject *obj{physicsWorld->getCollisionObjectArray()[i]};
    physicsWorld->removeCollisionObject(obj);
  }
}

ReifyRecordTrait<record::CELL>::type
reifyRecord(const record::CELL &refRec,
            ReifyRecordTrait<record::CELL>::resolvers resolvers) {
  const auto &cellRes{std::get<const Resolver<record::CELL> &>(resolvers)};
  const auto &bulletConf{cellRes.getBulletConfiguration()};

  auto cell{std::make_shared<Cell>(BaseId{refRec.mFormId},
                                   bulletConf.makeDynamicsWorld())};

  cell->name = (refRec.name ? refRec.name->data : "");

  if (auto lighting{refRec.lighting}; lighting) {
    Ogre::ColourValue ambient{};
    ambient.setAsABGR(lighting->data.ambient.v);
    cell->scnMgr->setAmbientLight(ambient);

    // TODO: Directional lighting, fog, water, etc.
  }

  cell->physicsWorld->setGravity({0.0f, -9.81f, 0.0f});

  const auto refs{cellRes.getReferences(BaseId{refRec.mFormId})};
  if (!refs) return cell;

  const auto &statRes{std::get<const Resolver<record::STAT> &>(resolvers)};
  const auto &doorRes{std::get<const Resolver<record::DOOR> &>(resolvers)};
  const auto &lighRes{std::get<const Resolver<record::LIGH> &>(resolvers)};
  const auto
      &refrStatRes{std::get<Resolver<record::REFR_STAT, RefId> &>(resolvers)};
  const auto
      &refrDoorRes{std::get<Resolver<record::REFR_DOOR, RefId> &>(resolvers)};
  const auto
      &refrLighRes{std::get<Resolver<record::REFR_LIGH, RefId> &>(resolvers)};

  Ogre::SceneNode *rootNode{cell->scnMgr->getRootSceneNode()};

  for (auto refId : *refs) {
    gsl::not_null<Ogre::SceneNode *> node{rootNode->createChildSceneNode()};
    if (auto stat{refrStatRes.get(refId)}; stat) {
      cell->attach(*stat, node, std::forward_as_tuple(statRes));
    } else if (auto door{refrDoorRes.get(refId)}; door) {
      cell->attach(*door, node, std::forward_as_tuple(doorRes));
    } else if (auto ligh{refrLighRes.get(refId)}; ligh) {
      cell->attach(*ligh, node, std::forward_as_tuple(lighRes));
    }
  }

  return cell;
}

void Cell::setNodeTransform(Ogre::SceneNode *node,
                            const record::raw::REFRTransformation &transform) {
  const auto &data{transform.positionRotation.data};

  node->setPosition(conversions::fromBSCoordinates({data.x, data.y, data.z}));

  if (transform.scale) {
    const float scale{transform.scale->data};
    node->setScale(scale, scale, scale);
  }

  // Rotations are extrinsic rotations about the z, y, then x axes.
  // Positive rotations refer to clockwise rotations, not anticlockwise.
  // This can no doubt be optimized by constructing a quaternion directly from
  // the angle data, building in the coordinate change, but building a rotation
  // matrix and changing coordinates was conceptually simpler.
  Ogre::Matrix3 rotX, rotY, rotZ;
  rotX.FromAngleAxis(Ogre::Vector3::UNIT_X, Ogre::Radian(-data.aX));
  rotY.FromAngleAxis(Ogre::Vector3::UNIT_Y, Ogre::Radian(-data.aY));
  rotZ.FromAngleAxis(Ogre::Vector3::UNIT_Z, Ogre::Radian(-data.aZ));
  const auto rotMat{conversions::fromBSCoordinates(rotX * rotY * rotZ)};
  const Ogre::Quaternion rotation{rotMat};
  node->rotate(rotation, Ogre::SceneNode::TS_WORLD);
}
