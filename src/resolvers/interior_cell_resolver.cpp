#include "conversions.hpp"
#include "esp.hpp"
#include "esp_coordinator.hpp"
#include "formid.hpp"
#include "records.hpp"
#include "resolvers/interior_cell_resolver.hpp"
#include "resolvers/resolvers.hpp"
#include <gsl/gsl>
#include <OgreRoot.h>
#include <OgreSceneNode.h>

InteriorCell::~InteriorCell() {
  auto root{Ogre::Root::getSingletonPtr()};
  if (root) root->destroySceneManager(scnMgr);
  // TODO: No raw loops
  for (int i = physicsWorld->getNumCollisionObjects() - 1; i >= 0; --i) {
    btCollisionObject *obj = physicsWorld->getCollisionObjectArray()[i];
    physicsWorld->removeCollisionObject(obj);
  }
}

auto Resolver<record::CELL>::peek(BaseId baseId) const -> peek_t {
  const auto entry{mMap.find(baseId)};
  if (entry == mMap.end()) return nullptr;
  else return entry->second.mRecord.get();
}

auto Resolver<record::CELL>::get(BaseId baseId) const -> get_t {
  return peek(baseId);
}

auto Resolver<record::CELL>::make(BaseId baseId) const -> make_t {
  const auto entry{mMap.find(baseId)};
  if (entry == mMap.end()) return nullptr;

  // If this cell has been loaded before and is still loaded, then we can return
  // a new shared_ptr to it. Also update the currently loaded cell.
  auto &cell{entry->second.mCell};
  if (!cell.expired()) {
    mStrategy->notify(cell.lock());
    return cell.lock();
  }

  // Otherwise we have to create a new shared_ptr and load the cell.
  const auto
      ptr{std::make_shared<InteriorCell>(mBulletConf.makeDynamicsWorld())};
  mStrategy->notify(ptr);
  cell = std::weak_ptr(ptr);

  // Fill in the scene data from the cell record, which we already have.
  const auto &rec{entry->second.mRecord};
  ptr->name = (rec->data.name ? rec->data.name->data : "");
  if (auto lighting{rec->data.lighting}; lighting) {
    const auto ambient{lighting->data.ambient};
    ptr->ambientLight.setAsABGR(ambient.v);
    ptr->scnMgr->setAmbientLight(ptr->ambientLight);

    // TODO: Directional lighting, fog, water, etc
  }
  ptr->physicsWorld->setGravity({0.0f, -9.81f, 0.0f});

  Processor processor(*ptr, mResolvers);

  // Read from a copy so subsequent reads work if the CELL is unloaded
  esp::EspAccessor accessor{entry->second.mAccessor};
  accessor.skipRecord();
  esp::readCellChildren(accessor, processor, processor, processor);

  return ptr;
}

bool Resolver<record::CELL>::add(BaseId, store_t entry) {
  const BaseId baseId{entry.mRecord->id};
  return mMap.insert_or_assign(baseId, std::move(entry)).second;
}

template<>
void Resolver<record::CELL>::Processor::readRecord<record::REFR>(esp::EspAccessor &accessor) {
  const auto ref{accessor.readRecord<record::REFR>().value};
  auto *const node{mCell.scnMgr->getRootSceneNode()->createChildSceneNode()};
  const BaseId baseId{ref.data.baseID.data};
  const RefId refId{ref.id};

  const auto &data{ref.data.positionRotation.data};

  // Set the position
  node->setPosition(conversions::fromBSCoordinates({data.x, data.y, data.z}));

  // Set the scale
  if (ref.data.scale) {
    const float scale{ref.data.scale->data};
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

  auto scnMgr{mCell.scnMgr};
  gsl::not_null<Ogre::SceneNode *> workingNode{node};
  gsl::not_null<btDiscreteDynamicsWorld *> world{mCell.physicsWorld.get()};

  // Construct the actual entities and attach them to the node
  if (mResolvers.statRes.contains(baseId)) {
    auto entity{mResolvers.statRes.make(baseId, scnMgr, {})};
    attachAll(workingNode, refId, world, entity);
  } else if (mResolvers.doorRes.contains(baseId)) {
    auto entity{mResolvers.doorRes.make(baseId, scnMgr, {})};
    attachAll(workingNode, refId, world, entity);
  } else if (mResolvers.lighRes.contains(baseId)) {
    auto entity{mResolvers.lighRes.make(baseId, scnMgr, {})};
    attachAll(workingNode, refId, world, entity);
  } else {
    mCell.scnMgr->destroySceneNode(node);
  }
}
