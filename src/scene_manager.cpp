#include "scene_manager.hpp"
#include "settings.hpp"
#include <OgreCamera.h>
#include <OgreRoot.h>
#include <spdlog/spdlog.h>

namespace oo {

//===----------------------------------------------------------------------===//
// Deferred Lighting Base Scene Manager
//===----------------------------------------------------------------------===//
DeferredSceneManager::DeferredSceneManager(const Ogre::String &name)
    : Ogre::SceneManager(name) {}

const Ogre::String &DeferredSceneManager::getTypeName() const {
  static Ogre::String typeName{DeferredSceneManagerFactory::FACTORY_TYPE_NAME};
  return typeName;
}

Ogre::MovableObject *
DeferredSceneManager::createMovableObject(const Ogre::String &name,
                                          const Ogre::String &typeName,
                                          const Ogre::NameValuePairList *params) {
  auto *obj{Ogre::SceneManager::createMovableObject(name, typeName, params)};
  if (typeName == Ogre::LightFactory::FACTORY_TYPE_NAME) {
    auto *light{static_cast<Ogre::Light *>(obj)};
    mLights.emplace_back(light, std::make_unique<oo::DeferredLight>(light));
  }
  return obj;
}

void DeferredSceneManager::destroyMovableObject(const Ogre::String &name,
                                                const Ogre::String &typeName) {
  SceneManager::destroyMovableObject(name, typeName);
  if (typeName == Ogre::LightFactory::FACTORY_TYPE_NAME) {
    auto pred = [&name](const auto &info) {
      return info.light->getName() == name;
    };
    auto it{std::find_if(mLights.begin(), mLights.end(), pred)};
    if (it == mLights.end()) return;

    mLights.erase(it);
  }
}

void DeferredSceneManager::destroyAllMovableObjectsByType(const Ogre::String &typeName) {
  Ogre::SceneManager::destroyAllMovableObjectsByType(typeName);
  if (typeName == Ogre::LightFactory::FACTORY_TYPE_NAME) mLights.clear();
}

void DeferredSceneManager::destroyAllMovableObjects() {
  Ogre::SceneManager::destroyAllMovableObjects();
  mLights.clear();
}

std::vector<oo::DeferredLight *> DeferredSceneManager::getLights() const {
  std::vector<oo::DeferredLight *> lights(mLights.size());
  std::transform(mLights.begin(), mLights.end(), lights.begin(),
                 [](const auto &info) { return info.geometry.get(); });
  return lights;
}

DeferredSceneManager::LightInfo::LightInfo(Ogre::Light *light,
                                           std::unique_ptr<oo::DeferredLight> geometry)
    : light(light), geometry(std::move(geometry)) {}

//===----------------------------------------------------------------------===//
// Interior Scene Manager
//===----------------------------------------------------------------------===//
OctreeNode::OctreeNode(oo::OctreeNode *parent, oo::IntegralAAB bbox)
    : mBbox(bbox), mParent(parent) {}

oo::IntegralAAB OctreeNode::getBoundingBox() const noexcept {
  return mBbox;
}

std::array<oo::OctreeNodePtr, 8u> &OctreeNode::getChildren() noexcept {
  return mOctants;
}

void buildOctreeImpl(oo::OctreeNode *parent,
                     const std::set<oo::OctreeSceneNode *> &nodes) {
  if (nodes.empty()) return;

  const auto parentBox{parent->getBoundingBox()};
  const auto dims{parentBox.max - parentBox.min};

  // Do not subdivide if any edge would go below the minimum size.
  if (!(dims > Ogre::Vector3i{1, 1, 1})) return;

  const auto midpoint{parentBox.min + dims / 2};

  const auto x0{parentBox.min[0]}, y0{parentBox.min[1]}, z0{parentBox.min[2]};
  const auto x1{midpoint[0]}, y1{midpoint[1]}, z1{midpoint[2]};
  const auto x2{parentBox.max[0]}, y2{parentBox.max[1]}, z2{parentBox.max[2]};

  //  2  1    z    y
  // 3  4     |  /
  //  6  5    |/___x
  // 7  8
  // TODO: Use the same axes as the game world.
  std::array<oo::IntegralAAB, 8u> octantRegions{
      oo::IntegralAAB{{x1, y1, z1}, {x2, y2, z2}},
      oo::IntegralAAB{{x0, y1, z1}, {x1, y2, z2}},
      oo::IntegralAAB{{x0, y0, z1}, {x1, y1, z2}},
      oo::IntegralAAB{{x1, y0, z1}, {x2, y1, z2}},
      oo::IntegralAAB{{x1, y1, z0}, {x2, y2, z1}},
      oo::IntegralAAB{{x0, y1, z0}, {x1, y2, z1}},
      oo::IntegralAAB{{x0, y0, z0}, {x1, y1, z1}},
      oo::IntegralAAB{{x1, y0, z0}, {x2, y1, z1}}
  };

  // Find the octant that each node is wholly contained in, if any, adding it to
  // a list of nodes for that octant and a list of all added nodes.
  std::array<std::set<oo::OctreeSceneNode *>, 8u> containedNodes{};
  std::set<oo::OctreeSceneNode *> placed{};
  auto begin{octantRegions.begin()}, end{octantRegions.end()};
  for (auto *node : nodes) {
    const auto &bbox{node->_getWorldAABB()};

    auto it{std::find_if(begin, end, [&bbox](const oo::IntegralAAB &octant) {
      const auto &min{octant.min};
      const auto &max{octant.max};
      const Ogre::AxisAlignedBox region{
          oo::OctreeNode::UNIT_SIZE * Ogre::Vector3(min[0], min[1], min[2]),
          oo::OctreeNode::UNIT_SIZE * Ogre::Vector3(max[0], max[1], max[2])
      };
      return region.contains(bbox);
    })};

    if (it != end) {
      containedNodes[std::distance(begin, it)].insert(node);
      placed.insert(node);
    }
  }

  // Create each octant that containing least one node and recurse into it.
  for (std::size_t i = 0u; i < 8u; ++i) {
    if (containedNodes[i].empty()) continue;
    auto &octant{parent->mOctants[i]};
    octant = std::make_unique<oo::OctreeNode>(parent, octantRegions[i]);
    buildOctreeImpl(octant.get(), containedNodes[i]);
  }

  // Those nodes that were not added belong to the parent.
  std::set_difference(nodes.begin(), nodes.end(), placed.begin(), placed.end(),
                      std::back_inserter(parent->mSceneNodes));
}

OctreeSceneNode::OctreeSceneNode(Ogre::SceneManager *creator)
    : Ogre::SceneNode(creator) {}

OctreeSceneNode::OctreeSceneNode(Ogre::SceneManager *creator,
                                 const Ogre::String &name)
    : Ogre::SceneNode(creator, name) {}

InteriorSceneManager::InteriorSceneManager(const Ogre::String &name)
    : oo::DeferredSceneManager(name) {}

const Ogre::String &InteriorSceneManager::getTypeName() const {
  static Ogre::String typeName{InteriorSceneManagerFactory::FACTORY_TYPE_NAME};
  return typeName;
}

void InteriorSceneManager::_updateSceneGraph(Ogre::Camera *camera) {
  firePreUpdateSceneGraph(camera);

  Ogre::Node::processQueuedUpdates();

  getRootSceneNode()->_update(true, false);
  mOctree.reset();
  mOctree = oo::buildOctree(mSceneNodes.begin(), mSceneNodes.end());

  firePostUpdateSceneGraph(camera);
}

void InteriorSceneManager::_findVisibleObjects(Ogre::Camera *camera,
                                               Ogre::VisibleObjectsBoundsInfo *visibleBounds,
                                               bool onlyShadowCasters) {
  auto *queue{getRenderQueue()};
  queue->clear();

  oo::preOrderDFS(mOctree.get(), [&](oo::OctreeNode *node) -> bool {
    // Empty octants are null.
    if (!node) return false;

    // Abandon this branch if the region is not visible.
    const auto iBbox{node->getBoundingBox()};
    constexpr auto unitSize{oo::OctreeNode::UNIT_SIZE};
    Ogre::AxisAlignedBox bbox{
        unitSize * Ogre::Vector3(iBbox.min[0], iBbox.min[1], iBbox.min[2]),
        unitSize * Ogre::Vector3(iBbox.max[0], iBbox.max[1], iBbox.max[2])
    };
    if (!camera->isVisible(bbox)) return false;

    // Add direct children.
    for (oo::OctreeSceneNode *sceneNode : node->getSceneNodes()) {
      if (!camera->isVisible(sceneNode->_getWorldAABB())) continue;
      for (auto *obj : sceneNode->getAttachedObjects()) {
        queue->processVisibleObject(obj, camera, onlyShadowCasters,
                                    visibleBounds);
      }
    }

    return true;
  });
}

oo::OctreeNode *InteriorSceneManager::_getOctree() noexcept {
  return mOctree.get();
}

gsl::owner<oo::OctreeSceneNode *> InteriorSceneManager::createSceneNodeImpl() {
  return OGRE_NEW oo::OctreeSceneNode(this);
}

gsl::owner<oo::OctreeSceneNode *>
InteriorSceneManager::createSceneNodeImpl(const Ogre::String &name) {
  return OGRE_NEW oo::OctreeSceneNode(this, name);
}

//===----------------------------------------------------------------------===//
// Scene Manager Factories
//===----------------------------------------------------------------------===//
gsl::owner<Ogre::SceneManager *>
DeferredSceneManagerFactory::createInstance(const Ogre::String &instanceName) {
  return OGRE_NEW oo::DeferredSceneManager(instanceName);
}

void DeferredSceneManagerFactory::destroyInstance(gsl::owner<Ogre::SceneManager *> instance) {
  OGRE_DELETE instance;
}

void DeferredSceneManagerFactory::initMetaData() const {
  mMetaData.typeName = FACTORY_TYPE_NAME;
  mMetaData.worldGeometrySupported = false;
}

gsl::owner<Ogre::SceneManager *>
InteriorSceneManagerFactory::createInstance(const Ogre::String &instanceName) {
  return OGRE_NEW oo::InteriorSceneManager(instanceName);
}

void InteriorSceneManagerFactory::destroyInstance(gsl::owner<Ogre::SceneManager *> instance) {
  OGRE_DELETE instance;
}

void InteriorSceneManagerFactory::initMetaData() const {
  mMetaData.typeName = FACTORY_TYPE_NAME;
  mMetaData.worldGeometrySupported = false;
}

} // namespace oo