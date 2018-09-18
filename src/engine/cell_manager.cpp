#include "engine/cell_manager.hpp"
#include "engine/conversions.hpp"
#include "esp.hpp"
#include "formid.hpp"
#include "records.hpp"
#include <OgreSceneNode.h>

namespace engine {

record::CELL *InteriorCellManager::peek(FormID baseID) {
  auto entry = cells.find(baseID);
  if (entry != cells.end()) return entry->second.record.get();
  else return nullptr;
}

std::shared_ptr<InteriorCell> InteriorCellManager::get(FormID baseID,
                                                       Ogre::SceneManager *mgr) {
  // Try to find an InteriorCellEntry with the given baseID. Since every cell
  // should have an entry by now, if no entry exists then we can give up.
  auto entry = cells.find(baseID);
  if (entry == cells.end()) return nullptr;

  // If this cell has been loaded before and is still loaded, then we can return
  // a new shared_ptr to it.
  auto &cell = entry->second.cell;
  if (!cell.expired()) return cell.lock();

  // Otherwise we have to create a new shared_ptr and load the cell.
  auto ptr = std::make_shared<InteriorCell>();
  ptr->scnMgr = mgr;
  cell = std::weak_ptr(ptr);

  // Fill in the scene data from the cell record, which we already have.
  auto &rec = entry->second.record;
  ptr->name = (rec->data.name ? rec->data.name->data : "");
  if (auto lighting = rec->data.lighting) {
    auto ambient = lighting->data.ambient;
    ptr->ambientLight.setAsABGR(ambient.v);
    ptr->scnMgr->setAmbientLight(ptr->ambientLight);

    // TODO: Directional lighting, fog, water, etc
  }

  Processor processor(ptr.get(), lightMgr, staticMgr);

  // TODO: Lock a mutex here
  is.seekg(entry->second.tell);
  (void) record::skipRecord(is);
  esp::readCellChildren(is, processor, processor, processor);

  return ptr;
}

template<>
void InteriorCellManager::Processor::readRecord<record::REFR>(std::istream &is) {
  auto ref = record::readRecord<record::REFR>(is);
  auto *node = cell->scnMgr->getRootSceneNode()->createChildSceneNode();

  if (auto *statEntity = staticMgr->get(ref.data.baseID.data, cell->scnMgr)) {
    node->attachObject(statEntity);
  } else if (auto *light = lightMgr->get(ref.data.baseID.data, cell->scnMgr)) {
    node->attachObject(light);
  } else {
    cell->scnMgr->destroySceneNode(node);
    return;
  }

  auto &data = ref.data.positionRotation.data;

  node->setPosition(conversions::fromBSCoordinates({data.x, data.y, data.z}));

  if (ref.data.scale) {
    float scale = ref.data.scale->data;
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
  auto rotMat = conversions::fromBSCoordinates(rotX * rotY * rotZ);
  Ogre::Quaternion rotation{rotMat};
  node->rotate(rotation, Ogre::SceneNode::TS_WORLD);
}

} // namespace engine