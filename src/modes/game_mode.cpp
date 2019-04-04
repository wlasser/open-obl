#include "game_settings.hpp"
#include "globals.hpp"
#include "gui/menu.hpp"
#include "meta.hpp"
#include "modes/console_mode.hpp"
#include "modes/game_mode.hpp"
#include "modes/loading_menu_mode.hpp"
#include "modes/menu_mode.hpp"
#include "settings.hpp"
#include "time_manager.hpp"
#include "sdl/sdl.hpp"
#include <absl/container/flat_hash_set.h>
#include <OgreBone.h>
#include <OgreSkeletonInstance.h>
#include <spdlog/fmt/ostr.h>

namespace oo {

GameMode::GameMode(ApplicationContext &/*ctx*/,
                   oo::CellPacket cellPacket) : mExteriorMgr(cellPacket) {
  mCell = std::move(cellPacket.mInteriorCell);

  mPlayerStartPos = std::move(cellPacket.mPlayerPosition);
  mPlayerStartOrientation = std::move(cellPacket.mPlayerOrientation);

  mInInterior = mExteriorMgr.getNearCells().empty();
}

GameMode::transition_t
GameMode::handleEvent(ApplicationContext &ctx, const sdl::Event &event) {
  auto keyEvent{ctx.getKeyMap().translateKey(event)};
  if (keyEvent) {
    return std::visit(overloaded{
        [this](oo::event::Forward e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [this](oo::event::Backward e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [this](oo::event::SlideLeft e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [this](oo::event::SlideRight e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [](oo::event::Use) -> transition_t { return {}; },
        [&](oo::event::Activate) -> transition_t {
          return handleActivate(ctx);
        },
        [](oo::event::Block) -> transition_t { return {}; },
        [](oo::event::Cast) -> transition_t { return {}; },
        [](oo::event::ReadyItem) -> transition_t { return {}; },
        [this](oo::event::Sneak e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [this](oo::event::Run e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [this](oo::event::AlwaysRun e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [](oo::event::AutoMove) -> transition_t { return {}; },
        [this](oo::event::Jump e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [](oo::event::TogglePov) -> transition_t { return {}; },
        [](oo::event::MenuMode) -> transition_t { return {}; },
        [](oo::event::Rest) -> transition_t { return {}; },
        [](oo::event::QuickMenu) -> transition_t { return {}; },
        [](oo::event::Quick) -> transition_t { return {}; },
        [](oo::event::QuickSave) -> transition_t { return {}; },
        [](oo::event::QuickLoad) -> transition_t { return {}; },
        [](oo::event::Grab) -> transition_t { return {}; },
        [&ctx](oo::event::Console e) -> transition_t {
          if (e.down) return {false, ConsoleMode(ctx)};
          return {};
        },
        [&ctx](oo::event::SystemMenu) -> transition_t {
          ctx.getRoot().queueEndRendering();
          return {};
        }
    }, *keyEvent);
  }

  if (sdl::typeOf(event) == sdl::EventType::MouseMotion) {
    const auto &settings{GameSettings::getSingleton()};
    const float sensitivity{settings.fGet("Controls.fMouseSensitivity")};
    const oo::event::Pitch pitch{{event.motion.yrel * sensitivity}};
    const oo::event::Yaw yaw{{event.motion.xrel * sensitivity}};
    mPlayerController->handleEvent(pitch);
    mPlayerController->handleEvent(yaw);
  }
  return {false, std::nullopt};
}

void GameMode::dispatchCollisions() {
  auto *const btDispatcher{getPhysicsWorld()->getDispatcher()};
  gsl::not_null dispatcher{dynamic_cast<btCollisionDispatcher *>(btDispatcher)};
  mCollisionCaller.runCallbacks(dispatcher);
}

RefId GameMode::getCrosshairRef() {
  GameSetting<int> iActivatePickLength{"iActivatePickLength", 150};

  auto *const camera{mPlayerController->getCamera()};
  const auto camPos{qvm::convert_to<btVector3>(camera->getDerivedPosition())};
  const auto camDir{qvm::convert_to<btVector3>(camera->getDerivedDirection())};
  const auto rayStart{camPos + 0.5L * camDir};
  const auto rayLength{*iActivatePickLength * oo::metersPerUnit<btScalar>};
  const auto rayEnd{camPos + rayLength * camDir};

  btCollisionWorld::ClosestRayResultCallback callback(rayStart, rayEnd);
  getPhysicsWorld()->rayTest(rayStart, rayEnd, callback);

  if (callback.hasHit()) {
    return RefId{decodeFormId(callback.m_collisionObject->getUserPointer())};
  } else {
    return RefId{};
  }
}

void GameMode::addPlayerToScene(ApplicationContext &ctx) {
  mPlayerController = std::make_unique<oo::PlayerController>(
      getSceneManager(), getPhysicsWorld());
  oo::PlayerController *controller{mPlayerController.get()};
  mCollisionCaller.addCallback(
      mPlayerController->getRigidBody(),
      [controller](const auto *other, const auto &contact) {
        controller->handleCollision(other, contact);
      });

  ctx.setCamera(gsl::make_not_null(mPlayerController->getCamera()));
}

void GameMode::registerSceneListeners(ApplicationContext &ctx) {
  getSceneManager()->addRenderQueueListener(ctx.getImGuiManager());
  getSceneManager()->addRenderQueueListener(ctx.getOverlaySystem());
}

void GameMode::unregisterSceneListeners(ApplicationContext &ctx) {
  // OGRE's documentation makes no guarantees about calling this on a listener
  // that is not added to the scene manager. Currently the code nops if the
  // listener is not present, and OGRE does not appear to provide any way of
  // checking, so fingers crossed that behaviour doesn't change I guess?
  getSceneManager()->removeRenderQueueListener(ctx.getImGuiManager());
  getSceneManager()->removeRenderQueueListener(ctx.getOverlaySystem());
}

gsl::not_null<Ogre::SceneManager *> GameMode::getSceneManager() const {
  if (mInInterior) return mCell->getSceneManager();
  return mExteriorMgr.getWorld().getSceneManager();
}

gsl::not_null<btDiscreteDynamicsWorld *> GameMode::getPhysicsWorld() const {
  if (mInInterior) return mCell->getPhysicsWorld();
  return mExteriorMgr.getWorld().getPhysicsWorld();
}

void GameMode::drawNodeChildren(Ogre::Node *node, const Ogre::Affine3 &t) {
  if (!mDebugDrawer) return;

  const auto p{qvm::convert_to<btVector3>(t * node->_getDerivedPosition())};
  for (auto *child : node->getChildren()) {
    const auto q{qvm::convert_to<btVector3>(t * child->_getDerivedPosition())};
    mDebugDrawer->drawLine(p, q, {1.0f, 0.0f, 0.0f});
    drawNodeChildren(child, t);
  }
}

void GameMode::updateAnimation(float delta) {
  // Can't naively update the animation of every single entity since if two
  // entities share a skeleton then the update will be applied twice.
  std::set<const Ogre::AnimationStateSet *> animSets;
  for (auto it{getSceneManager()->getMovableObjectIterator("oo::Entity")};
       it.hasMoreElements();) {
    const auto *entity{static_cast<const oo::Entity *>(it.getNext())};

    const auto *anims{entity->getAllAnimationStates()};
    if (anims) animSets.insert(anims);
  }

  for (const auto *animSet : animSets) {
    const auto &animStates{animSet->getEnabledAnimationStates()};
    for (auto state : animStates) state->addTime(delta);
  }
}

void GameMode::enter(ApplicationContext &ctx) {
  addPlayerToScene(ctx);
  mPlayerController->moveTo(mPlayerStartPos);
  // TODO: Set the player's orientation, this needs a new method on the
  //       PlayerController.

  registerSceneListeners(ctx);

  refocus(ctx);
}

void GameMode::refocus(ApplicationContext &) {
  sdl::setRelativeMouseMode(false);
}

bool GameMode::updateCenterCell(ApplicationContext &) {
  // TODO: Write a toBSCoordinates inverse of fromBSCoordinates
  const auto pos{mPlayerController->getPosition()};
  const auto cellIndex{oo::getCellIndex(pos.x * oo::unitsPerMeter<float>,
                                        -pos.z * oo::unitsPerMeter<float>)};

  if (cellIndex != mCenterCell) {
    mCenterCell = cellIndex;
    return true;
  }

  return false;
}

GameMode::transition_t GameMode::handleActivate(ApplicationContext &ctx) {
  const oo::RefId refId{getCrosshairRef()};
  auto baseRes{ctx.getBaseResolvers()};
  auto refrRes{ctx.getRefrResolvers()};

  if (refId != oo::RefId{0}) {
    if (auto door{oo::getComponent<record::raw::REFRDoor>(refId, refrRes)}) {
      if (auto teleport{door->teleport}) {
        const auto &data{teleport->data};
        const auto &refMap{ctx.getPersistentReferenceLocator()};
        // TODO: Loading exterior cells this way is unnecessary work since the
        //       BaseId gets converted back to a CellIndex and worldspace.
        //       Allow CellRequests containing a CellIndex.
        tl::optional<oo::BaseId> dstCell = [&]() -> tl::optional<oo::BaseId> {
          if (auto cell{refMap.getCell(data.destinationId)}) {
            // Interior door, we're done.
            return cell;
          } else if (auto wrldOpt{refMap.getWorldspace(data.destinationId)}) {
            // Exterior door, need to find cell with the given index.
            auto indexOpt{refMap.getCellIndex(data.destinationId)};
            if (!indexOpt) return tl::nullopt;

            auto &wrldRes{oo::getResolver<record::WRLD>(baseRes)};

            // Target worldspace might not be loaded yet.
            // TODO: Add a builtin function for testing if a WRLD is loaded.
            if (!wrldRes.getCells(*wrldOpt)) {
              wrldRes.load(*wrldOpt, oo::getResolvers<record::CELL>(baseRes));
            }

            return wrldRes.getCell(*wrldOpt, *indexOpt);
          }
          return tl::nullopt;
        }();

        if (!dstCell) return {};

        oo::CellRequest request{
            *dstCell,
            oo::fromBSCoordinates(Ogre::Vector3{data.x, data.y, data.z + 128}),
            // TODO: Convert rotations into quaternion
        };

        if (mInInterior) {
          mCell->setVisible(false);
        } else {
          mExteriorMgr.setVisible(false);
        }

        unregisterSceneListeners(ctx);
        return {true, oo::LoadingMenuMode(ctx, std::move(request))};
      }
    }
  }

  return {};
}

void GameMode::update(ApplicationContext &ctx, float delta) {
  updateAnimation(delta);
  mPlayerController->update(delta);
  getPhysicsWorld()->stepSimulation(delta, 4);
  dispatchCollisions();
  chrono::GameClock::advance(delta * 60.0f);
  chrono::GameClock::updateGlobals();

  const auto date{chrono::GameClock::getDate()};
  const auto today{chrono::game_days(date).time_since_epoch()};
  const auto now{chrono::GameClock::now().time_since_epoch()};

  if (!mInInterior) {
    if (updateCenterCell(ctx)) {
      oo::JobManager::runJob([&]() {
        mExteriorMgr.reifyNeighborhood(mCenterCell, ctx);
      });
    }
    oo::RenderJobManager::runJob([&]() {
      mExteriorMgr.getWorld().updateAtmosphere(
          chrono::duration_cast<chrono::minutes>(now - today));
    });
  }

  if (mDebugDrawer) {
    mDebugDrawer->clearLines();
    getPhysicsWorld()->debugDrawWorld();

    auto it{getSceneManager()->getMovableObjectIterator("oo::Entity")};
    while (it.hasMoreElements()) {
      auto *entity{static_cast<oo::Entity *>(it.getNext())};
      if (!entity->hasSkeleton()) continue;
      auto *node{entity->getParentSceneNode()};
      auto *skel{entity->getSkeleton()};
      for (auto *root : skel->getRootBones()) {
        drawNodeChildren(root, node->_getFullTransform());
      }
    }

    mDebugDrawer->build();
  }

  static RefId refUnderCrosshair{0};
  RefId newRefUnderCrosshair = getCrosshairRef();
  if (newRefUnderCrosshair != refUnderCrosshair) {
    refUnderCrosshair = newRefUnderCrosshair;

    if (auto baseOpt{oo::getComponent<record::raw::REFRBase>(refUnderCrosshair,
                                                             ctx.getRefrResolvers())}) {
      ctx.getLogger()->info("Looking at RefId {} with BaseId {}",
                            refUnderCrosshair, baseOpt->baseId.data);
    } else {
      ctx.getLogger()->info("Looking at RefId {}", refUnderCrosshair);
    }
  }
}

void GameMode::toggleCollisionGeometry() {
  if (mDebugDrawer) {
    getSceneManager()->destroySceneNode("__DebugDrawerNode");
    getPhysicsWorld()->setDebugDrawer(nullptr);
    mDebugDrawer.reset();
  } else {
    mDebugDrawer = std::make_unique<Ogre::DebugDrawer>(getSceneManager(),
                                                       oo::SHADER_GROUP);
    auto *root{getSceneManager()->getRootSceneNode()};
    auto *node{root->createChildSceneNode("__DebugDrawerNode")};
    node->attachObject(mDebugDrawer->getObject());
    getPhysicsWorld()->setDebugDrawer(mDebugDrawer.get());
  }
}

} // namespace oo
