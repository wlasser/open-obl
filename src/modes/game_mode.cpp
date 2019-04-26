#include "game_settings.hpp"
#include "globals.hpp"
#include "gui/menu.hpp"
#include "meta.hpp"
#include "modes/console_mode.hpp"
#include "modes/debug_draw_impl.hpp"
#include "modes/game_mode.hpp"
#include "modes/loading_menu_mode.hpp"
#include "modes/menu_mode.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include "scene_manager.hpp"
#include "settings.hpp"
#include "time_manager.hpp"
#include "sdl/sdl.hpp"
#include <absl/container/flat_hash_set.h>
#include <imgui/imgui.h>
#include <OgreBone.h>
#include <OgreSkeletonInstance.h>
#include <spdlog/fmt/ostr.h>

namespace oo {

GameMode::GameMode(ApplicationContext &/*ctx*/, oo::CellPacket cellPacket)
    : mExteriorMgr(cellPacket),
      mDebugDrawImpl(std::make_unique<oo::DebugDrawImpl>(this)) {
  mCell = std::move(cellPacket.mInteriorCell);

  mPlayerStartPos = std::move(cellPacket.mPlayerPosition);
  mPlayerStartOrientation = std::move(cellPacket.mPlayerOrientation);

  mInInterior = mExteriorMgr.getNearCells().empty();
}

GameMode::~GameMode() = default;

GameMode::GameMode(GameMode &&other) noexcept
    : mExteriorMgr(std::move(other.mExteriorMgr)),
      mCell(std::move(other.mCell)),
      mCenterCell(other.mCenterCell),
      mInInterior(other.mInInterior),
      mPlayerStartPos(other.mPlayerStartPos),
      mPlayerStartOrientation(other.mPlayerStartOrientation),
      mPlayerController(std::move(other.mPlayerController)),
      mCollisionCaller(std::move(other.mCollisionCaller)),
      mDebugDrawImpl(std::make_unique<oo::DebugDrawImpl>(this)) {}

GameMode &GameMode::operator=(GameMode &&other) noexcept {
  mExteriorMgr = std::move(other.mExteriorMgr);
  mCell = std::move(other.mCell);
  mCenterCell = other.mCenterCell;
  mInInterior = other.mInInterior;
  mPlayerStartPos = other.mPlayerStartPos;
  mPlayerStartOrientation = other.mPlayerStartOrientation;
  mPlayerController = std::move(other.mPlayerController);
  mCollisionCaller = std::move(other.mCollisionCaller);
  mDebugDrawImpl = std::make_unique<oo::DebugDrawImpl>(this);

  return *this;
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

RefId GameMode::getCrosshairRef() const {
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
  mPlayerController = std::make_unique<oo::CharacterController>(
      getSceneManager(), getPhysicsWorld());
  oo::CharacterController *controller{mPlayerController.get()};
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

void GameMode::logRefUnderCursor(ApplicationContext &ctx) const {
  static oo::RefId refUnderCrosshair{0};
  oo::RefId newRefUnderCrosshair = getCrosshairRef();
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
  mPlayerController->setOrientation(mPlayerStartOrientation);

  registerSceneListeners(ctx);

  refocus(ctx);
}

void GameMode::refocus(ApplicationContext &) {
  sdl::setRelativeMouseMode(false);
}

bool GameMode::updateCenterCell(ApplicationContext &) {
  const auto pos{oo::toBSCoordinates(mPlayerController->getPosition())};
  const auto cellIndex{oo::getCellIndex(qvm::X(pos), qvm::Y(pos))};

  if (cellIndex != mCenterCell) {
    mCenterCell = cellIndex;
    return true;
  }

  return false;
}

void GameMode::advanceGameClock(float delta) {
  chrono::GameClock::advance(delta * 60.0f);
  chrono::GameClock::updateGlobals();

  const chrono::game_days today(chrono::GameClock::getDate());
  const auto now{chrono::GameClock::now()};

  if (mInInterior) return;

  oo::RenderJobManager::runJob([&]() {
    mExteriorMgr.getWorld().updateAtmosphere(
        chrono::duration_cast<chrono::minutes>(now - today));
  });
}

GameMode::transition_t GameMode::handleActivate(ApplicationContext &ctx) {
  const oo::RefId refId{getCrosshairRef()};
  auto refrRes{ctx.getRefrResolvers()};

  if (refId == oo::RefId{0}) return {};

  if (auto door{oo::getComponent<record::raw::REFRDoor>(refId, refrRes)}) {
    return handleActivate(ctx, *door);
  }

  return {};
}

GameMode::transition_t
GameMode::handleActivate(ApplicationContext &ctx,
                         const record::raw::REFRDoor &door) {
  // TODO: Handle animable interior doors
  if (!door.teleport) return {};

  const auto &data{door.teleport->data};
  auto dstCell{getDoorDestinationCell(ctx, *door.teleport)};
  if (!dstCell) return {};

  oo::CellRequest request{
      *dstCell,
      oo::fromBSCoordinates(Ogre::Vector3{data.x, data.y, data.z + 128}),
      oo::fromBSTaitBryan(Ogre::Radian(data.aX),
                          Ogre::Radian(data.aY),
                          Ogre::Radian(data.aZ))
  };

  if (mInInterior) {
    mCell->setVisible(false);
  } else {
    mExteriorMgr.setVisible(false);
  }

  unregisterSceneListeners(ctx);
  return {true, oo::LoadingMenuMode(ctx, std::move(request))};
}

tl::optional<oo::BaseId>
GameMode::getDoorDestinationCell(ApplicationContext &ctx,
                                 const record::XTEL &teleport) const {
  const auto &refMap{ctx.getPersistentReferenceLocator()};
  auto baseRes{ctx.getBaseResolvers()};
  const auto &data{teleport.data};

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
}

void GameMode::update(ApplicationContext &ctx, float delta) {
  updateAnimation(delta);
  mPlayerController->update(delta);
  getPhysicsWorld()->stepSimulation(delta, 4);
  dispatchCollisions();
  advanceGameClock(delta);

  if (!mInInterior && updateCenterCell(ctx)) {
    oo::JobManager::runJob([&]() {
      mExteriorMgr.reifyNeighborhood(mCenterCell, ctx);
    });
  }

  mDebugDrawImpl->drawDebug();
  mDebugDrawImpl->drawFpsDisplay(delta);

  logRefUnderCursor(ctx);
}

void GameMode::toggleCollisionGeometry() {
  const bool isEnabled{mDebugDrawImpl->getDrawCollisionGeometryEnabled()};
  mDebugDrawImpl->setDrawCollisionGeometryEnabled(!isEnabled);
  mDebugDrawImpl->refreshDebugDrawer();
}

void GameMode::toggleOcclusionGeometry() {
  const bool isEnabled{mDebugDrawImpl->getDrawOcclusionGeometryEnabled()};
  mDebugDrawImpl->setDrawOcclusionGeometryEnabled(!isEnabled);
  mDebugDrawImpl->refreshDebugDrawer();
}

void GameMode::toggleFps() {
  mDebugDrawImpl->setDisplayFpsEnabled(!mDebugDrawImpl->getDisplayFpsEnabled());
}

} // namespace oo
