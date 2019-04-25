#include "math/conversions.hpp"
#include "mesh/entity.hpp"
#include "modes/debug_draw_impl.hpp"
#include "modes/game_mode.hpp"
#include "scene_manager.hpp"
#include <imgui/imgui.h>
#include <OgreBone.h>
#include <OgreSceneNode.h>
#include <OgreSkeletonInstance.h>

namespace oo {

DebugDrawImpl::DebugDrawImpl(GameMode *gameMode) noexcept
    : mGameMode(gameMode), mFrameTimes(NUM_FPS_SAMPLES) {}

void DebugDrawImpl::drawNodeChildren(gsl::not_null<Ogre::Node *> node,
                                     const Ogre::Affine3 &t) {
  const auto p{qvm::convert_to<btVector3>(t * node->_getDerivedPosition())};
  for (auto *child : node->getChildren()) {
    const auto q{qvm::convert_to<btVector3>(t * child->_getDerivedPosition())};
    mDebugDrawer->drawLine(p, q, {1.0f, 0.0f, 0.0f});
    drawNodeChildren(gsl::make_not_null(child), t);
  }
}

void DebugDrawImpl::drawSkeleton(gsl::not_null<oo::Entity *> entity) {
  if (!entity->hasSkeleton()) return;
  auto *node{entity->getParentSceneNode()};
  auto *skel{entity->getSkeleton()};
  for (auto *root : skel->getRootBones()) {
    drawNodeChildren(gsl::make_not_null(root), node->_getFullTransform());
  }
}

void DebugDrawImpl::drawBoundingBox(gsl::not_null<oo::Entity *> entity) {
  const auto &bbox{entity->getWorldBoundingBox()};
  const auto min{qvm::convert_to<btVector3>(bbox.getMinimum())};
  const auto max{qvm::convert_to<btVector3>(bbox.getMaximum())};
  mDebugDrawer->drawBox(min, max, {0.0f, 1.0f, 0.0f});
}

void DebugDrawImpl::drawBoundingBox(gsl::not_null<Ogre::SceneNode *> node) {
  const auto bbox{node->_getWorldAABB()};
  const auto min{qvm::convert_to<btVector3>(bbox.getMinimum())};
  const auto max{qvm::convert_to<btVector3>(bbox.getMaximum())};
  mDebugDrawer->drawBox(min, max, {1.0f, 0.0f, 0.0f});
}

void DebugDrawImpl::drawBoundingBox(gsl::not_null<oo::OctreeNode *> node) {
  const auto bbox{node->getBoundingBox()};
  const btVector3 min(bbox.min[0], bbox.min[1], bbox.min[2]);
  const btVector3 max(bbox.max[0], bbox.max[1], bbox.max[2]);
  mDebugDrawer->drawBox(min * oo::OctreeNode::UNIT_SIZE,
                        max * oo::OctreeNode::UNIT_SIZE,
                        {1.0f, 1.0f, 0.0f});
}

void DebugDrawImpl::drawFpsDisplay(float delta) {
  mFrameTimes.push_back(delta);

  if (!mFpsDisplayEnabled) return;

  ImGui::Begin("Debug Display", nullptr, ImGuiWindowFlags_None);
  ImGui::Text("FPS: %f", 1.0f / delta);
  std::array<float, NUM_FPS_SAMPLES> frameTimes;
  std::copy(mFrameTimes.begin(), mFrameTimes.end(), frameTimes.begin());
  ImGui::PlotLines("Frame times", frameTimes.data(), mFrameTimes.size());
  ImGui::End();
}

void DebugDrawImpl::drawDebug() {
  if (!mDebugDrawer) return;
  mDebugDrawer->clearLines();

  auto scnMgr{mGameMode->getSceneManager()};
  auto physicsWorld{mGameMode->getPhysicsWorld()};

  if (getDrawCollisionGeometryEnabled()) {
    physicsWorld->debugDrawWorld();

    auto entities{scnMgr->getMovableObjectIterator("oo::Entity")};
    for (const auto&[_, entity] : entities) {
      drawSkeleton(gsl::make_not_null(static_cast<oo::Entity *>(entity)));
    }
  }

  if (getDrawOcclusionGeometryEnabled()) {
    if (auto
        *intScnMgr{dynamic_cast<oo::InteriorSceneManager *>(scnMgr.get())}) {
      oo::preOrderDFS(intScnMgr->_getOctree(), [&](oo::OctreeNode *node) {
        if (!node) return false;
        drawBoundingBox(gsl::make_not_null(node));
        return true;
      });
    }

    auto entities{scnMgr->getMovableObjectIterator("oo::Entity")};
    for (const auto &[_, entity] : entities) {
      drawBoundingBox(gsl::make_not_null(entity->getParentSceneNode()));
    }
  }

  mDebugDrawer->build();
}

void DebugDrawImpl::setDebugDrawerEnabled(bool enable) {
  auto scnMgr{mGameMode->getSceneManager()};
  constexpr static const char *DEBUG_NODE_NAME{"__DebugDrawerNode"};
  if (enable && !mDebugDrawer) {
    mDebugDrawer = std::make_unique<Ogre::DebugDrawer>(scnMgr,
                                                       oo::SHADER_GROUP);
    auto *root{scnMgr->getRootSceneNode()};
    auto *node{root->createChildSceneNode(DEBUG_NODE_NAME)};
    node->attachObject(mDebugDrawer->getObject());
    setDrawCollisionGeometryEnabled(getDrawCollisionGeometryEnabled());
    setDrawOcclusionGeometryEnabled(getDrawOcclusionGeometryEnabled());
  } else if (!enable && mDebugDrawer) {
    scnMgr->destroySceneNode(DEBUG_NODE_NAME);
    setDrawOcclusionGeometryEnabled(false);
    setDrawCollisionGeometryEnabled(false);
    mDebugDrawer.reset();
  }
}

void DebugDrawImpl::refreshDebugDrawer() {
  setDebugDrawerEnabled(mDebugDrawFlags != DebugDrawFlags::None);
}

void DebugDrawImpl::setDrawCollisionGeometryEnabled(bool enabled) {
  if (enabled) mDebugDrawFlags |= DebugDrawFlags::Collision;
  else mDebugDrawFlags &= ~DebugDrawFlags::Collision;

  if (enabled) mGameMode->getPhysicsWorld()->setDebugDrawer(mDebugDrawer.get());
  else mGameMode->getPhysicsWorld()->setDebugDrawer(nullptr);
}

void DebugDrawImpl::setDrawOcclusionGeometryEnabled(bool enabled) {
  if (enabled) mDebugDrawFlags |= DebugDrawFlags::Occlusion;
  else mDebugDrawFlags &= ~DebugDrawFlags::Occlusion;
}

void DebugDrawImpl::setDisplayFpsEnabled(bool enabled) {
  mFpsDisplayEnabled = enabled;
}

bool DebugDrawImpl::getDrawCollisionGeometryEnabled() const noexcept {
  return static_cast<bool>(mDebugDrawFlags & DebugDrawFlags::Collision);
}

bool DebugDrawImpl::getDrawOcclusionGeometryEnabled() const noexcept {
  return static_cast<bool>(mDebugDrawFlags & DebugDrawFlags::Occlusion);
}

bool DebugDrawImpl::getDisplayFpsEnabled() const noexcept {
  return mFpsDisplayEnabled;
}

} // namespace oo