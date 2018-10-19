#include "engine/conversions.hpp"
#include "engine/resolvers/resolvers.hpp"
#include "engine/resolvers/interior_cell_resolver.hpp"
#include "esp.hpp"
#include "formid.hpp"
#include "records.hpp"
#include <gsl/gsl>
#include <OgreRoot.h>
#include <OgreSceneNode.h>

namespace engine {

record::CELL *InteriorCellResolver::peek(BaseId baseId) const {
  const auto entry = cells.find(baseId);
  if (entry != cells.end()) return entry->second.record.get();
  else return nullptr;
}

std::shared_ptr<InteriorCell> InteriorCellResolver::get(BaseId baseId) const {
  // Try to find an InteriorCellEntry with the given baseId. Since every cell
  // should have an entry by now, if no entry exists then we can give up.
  const auto entry{cells.find(baseId)};
  if (entry == cells.end()) return nullptr;

  // If this cell has been loaded before and is still loaded, then we can return
  // a new shared_ptr to it. Also update the currently loaded cell.
  auto &cell{entry->second.cell};
  if (!cell.expired()) {
    strategy->notify(cell.lock());
    return cell.lock();
  }

  // Otherwise we have to create a new shared_ptr and load the cell.
  const auto ptr
      {std::make_shared<InteriorCell>(bulletConf->makeDynamicsWorld())};
  strategy->notify(ptr);
  cell = std::weak_ptr(ptr);

  // Fill in the scene data from the cell record, which we already have.
  const auto &rec{entry->second.record};
  ptr->name = (rec->data.name ? rec->data.name->data : "");
  if (auto lighting{rec->data.lighting}; lighting) {
    const auto ambient{lighting->data.ambient};
    ptr->ambientLight.setAsABGR(ambient.v);
    ptr->scnMgr->setAmbientLight(ptr->ambientLight);

    // TODO: Directional lighting, fog, water, etc
  }
  ptr->physicsWorld->setGravity({0.0f, -9.81f, 0.0f});

  Processor processor(ptr.get(), doorRes, lightRes, staticRes);

  // TODO: Lock a mutex here
  is.seekg(entry->second.tell);
  record::skipRecord(is);
  esp::readCellChildren(is, processor, processor, processor);

  return ptr;
}

bool InteriorCellResolver::add(BaseId baseId, InteriorCellEntry entry) {
  return cells.try_emplace(BaseId{entry.record->id}, std::move(entry)).second;
}

template<>
void InteriorCellResolver::Processor::readRecord<record::REFR>(std::istream &is) {
  const auto ref{record::readRecord<record::REFR>(is)};
  auto *const node{cell->scnMgr->getRootSceneNode()->createChildSceneNode()};
  const auto id{ref.data.baseID.data};

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

  // Construct the actual entities and attach them to the node
  if (auto[rigidBody, mesh]{staticRes->get(id, cell->scnMgr)}; rigidBody
      || mesh) {
    gsl::not_null<Ogre::SceneNode *> workingNode{node};

    if (rigidBody) setRefId(gsl::make_not_null(rigidBody), RefId{ref.id});

    workingNode = attachRigidBody(workingNode, rigidBody,
                                  gsl::make_not_null(cell->physicsWorld.get()));
    workingNode = attachMesh(workingNode, mesh, true);

    return;
  }

  if (auto[rigidBody, mesh]{doorRes->get(id, cell->scnMgr)}; rigidBody
      || mesh) {
    gsl::not_null<Ogre::SceneNode *> workingNode{node};

    if (rigidBody) setRefId(gsl::make_not_null(rigidBody), RefId{ref.id});

    workingNode = attachRigidBody(workingNode, rigidBody,
                                  gsl::make_not_null(cell->physicsWorld.get()));
    workingNode = attachMesh(workingNode, mesh, true);

    return;
  }

  if (auto[light, rigidBody, mesh]{lightRes->get(id, cell->scnMgr)}; light) {
    gsl::not_null<Ogre::SceneNode *> workingNode{node};

    if (rigidBody) setRefId(gsl::make_not_null(rigidBody), RefId{ref.id});

    workingNode = attachRigidBody(workingNode, rigidBody,
                                  gsl::make_not_null(cell->physicsWorld.get()));
    workingNode = attachMesh(workingNode, mesh);
    workingNode = attachLight(workingNode, light, true);

    return;
  }

  cell->scnMgr->destroySceneNode(node);
}

} // namespace engine