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
#include <spdlog/fmt/ostr.h>

namespace oo {

GameMode::GameMode(ApplicationContext &/*ctx*/,
                   oo::CellPacket cellPacket) {
  mWrld = std::move(cellPacket.mWrld);
  mCell = std::move(cellPacket.mInteriorCell);
  mNearCells = std::move(cellPacket.mExteriorCells);
  for (const auto &cell : mNearCells) {
    mNearLoaded.emplace(cell->getBaseId());
  }

  mPlayerStartPos = std::move(cellPacket.mPlayerPosition);
  mPlayerStartOrientation = std::move(cellPacket.mPlayerOrientation);

  mInInterior = mNearCells.empty();
}

GameMode::GameMode(GameMode &&other) noexcept {
  std::scoped_lock lock{other.mNearMutex, other.mFarMutex};
  mWrld = std::move(other.mWrld);
  mCell = std::move(other.mCell);
  mNearCells = std::move(other.mNearCells);
  mNearLoaded = std::move(other.mNearLoaded);
  mFarLoaded = std::move(other.mFarLoaded);
  mCenterCell = std::move(other.mCenterCell);
  mInInterior = other.mInInterior;
  mPlayerStartPos = std::move(other.mPlayerStartPos);
  mPlayerStartOrientation = std::move(other.mPlayerStartOrientation);
  mPlayerController = std::move(other.mPlayerController);
  mCollisionCaller = std::move(other.mCollisionCaller);
  mDebugDrawer = std::move(other.mDebugDrawer);

  // TODO: Some jobs launched by the GameMode capture `this`, despite how
  //       terrible an idea that is. Obviously they'll break if the GameMode
  //       is moved while they're active. As of writing that can't happen so
  //       long as no jobs are launched in the constructor, but I don't think
  //       that should really be relied on...
}

GameMode &GameMode::operator=(GameMode &&other) noexcept {
  if (this != &other) {
    std::scoped_lock lock{mNearMutex, mFarMutex,
                          other.mNearMutex, other.mFarMutex};
    mWrld = std::move(other.mWrld);
    mCell = std::move(other.mCell);
    mNearCells = std::move(other.mNearCells);
    mNearLoaded = std::move(other.mNearLoaded);
    mFarLoaded = std::move(other.mFarLoaded);
    mCenterCell = std::move(other.mCenterCell);
    mInInterior = other.mInInterior;
    mPlayerStartPos = std::move(other.mPlayerStartPos);
    mPlayerStartOrientation = std::move(other.mPlayerStartOrientation);
    mPlayerController = std::move(other.mPlayerController);
    mCollisionCaller = std::move(other.mCollisionCaller);
    mDebugDrawer = std::move(other.mDebugDrawer);
  }

  return *this;
}

void releasePlayerController(btDiscreteDynamicsWorld *physicsWorld,
                             oo::PlayerController *playerController) {
  const auto *rigidBody{playerController->getRigidBody()};
  if (rigidBody->isInWorld()) {
    physicsWorld->removeRigidBody(playerController->getRigidBody());
  }
  delete playerController;
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
        [](oo::event::Activate) -> transition_t { return {}; },
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
  mPlayerController = oo::makePlayerController(getPhysicsWorld(),
                                               getSceneManager(),
                                               getPhysicsWorld());
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

gsl::not_null<Ogre::SceneManager *> GameMode::getSceneManager() const {
  if (mInInterior) return mCell->getSceneManager();
  return mNearCells.front()->getSceneManager();
}

gsl::not_null<btDiscreteDynamicsWorld *> GameMode::getPhysicsWorld() const {
  if (mInInterior) return mCell->getPhysicsWorld();
  return mNearCells.front()->getPhysicsWorld();
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
  for (auto it{getSceneManager()->getMovableObjectIterator("Entity")};
       it.hasMoreElements();) {
    const auto *entity{static_cast<const Ogre::Entity *>(it.getNext())};

    const auto *anims{entity->getAllAnimationStates()};
    if (!anims) continue;

    const auto &animStates{anims->getEnabledAnimationStates()};
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

void GameMode::reifyFarExteriorCell(oo::BaseId cellId,
                                    ApplicationContext &ctx) {
  std::unique_lock lock{mFarMutex};

  ctx.getLogger()->info("[{}]: reifyFarExteriorCell({}) started",
                        boost::this_fiber::get_id(), cellId);

  auto _ = gsl::finally([&]() {
    mFarLoaded.emplace(cellId);
    ctx.getLogger()->info("[{}]: reifyFarExteriorCell({}) finished",
                          boost::this_fiber::get_id(), cellId);
  });

  // By construction, this cell is not already loaded.

  // If it's cached, promote it in the cache and show the terrain.
  if (auto[cellPtr, isInterior]{ctx.getCellCache()->get(cellId)};
      cellPtr && !isInterior) {
    // TODO: Show the terrain
    ctx.getCellCache()->promote(cellId);
    ctx.getLogger()->info("[{}]: Loaded cell {} terrain from cache",
                          boost::this_fiber::get_id(), cellId);
    return;
  }

  // Otherwise need to actually load
  oo::JobCounter terrainLoadDone{1};
  oo::RenderJobManager::runJob([this, cellId]() {
    mWrld->loadTerrainOnly(cellId, /*async=*/true);
  }, &terrainLoadDone);
  terrainLoadDone.wait();
}

void GameMode::unloadFarExteriorCell(oo::BaseId cellId,
                                     ApplicationContext &ctx) {
  std::unique_lock lock{mFarMutex};

  ctx.getLogger()->info("[{}]: unloadFarExteriorCell({}) started",
                        boost::this_fiber::get_id(), cellId);

  auto _ = gsl::finally([&]() {
    auto it{std::find(mFarLoaded.begin(), mFarLoaded.end(), cellId)};
    mFarLoaded.erase(it);
    ctx.getLogger()->info("[{}]: unloadFarExteriorCell({}) finished",
                          boost::this_fiber::get_id(), cellId);
  });

  // By construction, this cell is loaded.

  // If it's cached, then no unloading to do.
  if (auto[cellPtr, isInterior]{ctx.getCellCache()->get(cellId)};
      cellPtr && !isInterior) {
    // TODO: Hide the terrain
    ctx.getLogger()->info("[{}]: Cell is cached, hiding terrain [TODO]",
                          boost::this_fiber::get_id());
    return;
  }

  // Otherwise need to actually unload.
  mWrld->unloadTerrain(cellId);
}

void GameMode::reifyFarNeighborhood(World::CellIndex centerCell,
                                    ApplicationContext &ctx) {
  const auto &gameSettings{oo::GameSettings::getSingleton()};
  const auto farDiameter{gameSettings.get<unsigned int>(
      "General.uGridDistantCount", 5)};

  // Get the set of far neighbours that we want to end up being loaded.
  auto farNbrsArray{mWrld->getNeighbourhood(centerCell, farDiameter)};
  std::set<oo::BaseId> farNbrsNew{};
  for (const auto &row : farNbrsArray) {
    for (auto id : row) {
      farNbrsNew.emplace(id);
    }
  }

  // Don't need to take the mFarMutex because only one reifyFarNeighbourhood
  // can occur at a time and therefore reifyFarExteriorCell is not executing.

  // Get the set of far neighbours that are currently loaded that we don't want
  // to be.
  std::set<oo::BaseId> farToUnload{};
  std::set_difference(mFarLoaded.begin(), mFarLoaded.end(),
                      farNbrsNew.begin(), farNbrsNew.end(),
                      std::inserter(farToUnload, farToUnload.end()));

  // Get the set of all far neighbours that need loading, and start jobs to load
  // them all. Some of these might already be loaded, which is fine.
  std::set<oo::BaseId> farToLoad{};
  std::set_difference(farNbrsNew.begin(), farNbrsNew.end(),
                      mFarLoaded.begin(), mFarLoaded.end(),
                      std::inserter(farToLoad, farToLoad.end()));

  oo::JobCounter farLoadJc{static_cast<int>(farToLoad.size())};
  for (auto id : farToLoad) {
    oo::JobManager::runJob([this, id, &ctx]() {
      this->reifyFarExteriorCell(id, ctx);
    }, &farLoadJc);
  }

  oo::JobCounter farUnloadJc{static_cast<int>(farToUnload.size())};
  for (auto id : farToUnload) {
    oo::RenderJobManager::runJob([this, id, &ctx]() {
      this->unloadFarExteriorCell(id, ctx);
    }, &farUnloadJc);
  }

  farLoadJc.wait();
  farUnloadJc.wait();
}

void GameMode::reifyNearExteriorCell(oo::BaseId cellId,
                                     ApplicationContext &ctx) {
  std::unique_lock lock{mNearMutex};
  ctx.getLogger()->info("[{}]: reifyNearExteriorCell({}) started",
                        boost::this_fiber::get_id(), cellId);

  auto _ = gsl::finally([&]() {
    mNearLoaded.emplace(cellId);
    ctx.getLogger()->info("[{}]: reifyNearExteriorCell({}) finished",
                          boost::this_fiber::get_id(), cellId);
  });

  // By construction, this cell is not already loaded.

  // If it's cached, promote it in the cache and make it visible.
  if (auto[cellPtr, isInterior]{ctx.getCellCache()->get(cellId)};
      cellPtr && !isInterior) {
    ctx.getCellCache()->promote(cellId);
    auto extCellPtr{std::static_pointer_cast<oo::ExteriorCell>(cellPtr)};
    extCellPtr->setVisible(true);
    mNearCells.emplace_back(std::move(extCellPtr));
    ctx.getLogger()->info("[{}]: Loaded cell {} from cache",
                          boost::this_fiber::get_id(), cellId);
    return;
  }

  // Otherwise need to actually load.
  if (!mWrld) {
    throw std::runtime_error("Attempting to load an exterior cell "
                             "without a worldspace");
  }

  const auto wrldId{mWrld->getBaseId()};

  auto &cellRes{oo::getResolver<record::CELL>(ctx.getBaseResolvers())};
  auto &wrldRes{oo::getResolver<record::WRLD>(ctx.getBaseResolvers())};

  if (auto cells{wrldRes.getCells(wrldId)};
      !cells || !cells->contains(cellId)) {
    ctx.getLogger()->error("Cell {} does not exist in worldspace {}",
                           cellId, wrldId);
    throw std::runtime_error("Cell does not exist in worldspace");
  }

  const auto cellRec{cellRes.get(cellId)};

  cellRes.load(cellId, getCellRefrResolvers(ctx), getCellBaseResolvers(ctx));
  boost::this_fiber::yield();

  oo::JobCounter reifyDone{1};
  oo::RenderJobManager::runJob([this, cellRec, &ctx]() {
    mNearCells.emplace_back(std::static_pointer_cast<oo::ExteriorCell>(
        reifyRecord(*cellRec,
                    mWrld->getSceneManager(),
                    mWrld->getPhysicsWorld(),
                    getCellResolvers(ctx))));
  }, &reifyDone);
  reifyDone.wait();

  // This function could be called at the same time as reifyFarExteriorCell()
  // with the same cellId, because the far neighbourhood contains the near
  // neighbourhood. loadTerrain() calls loadTerrainOnly() to load the actual
  // terrain, which doesn't do anything if the terrain is already loaded, so
  // we don't have to worry about whether this function or
  // reifyFarExteriorCell() occurs first, so long as they don't occur
  // simultaneously. An easy way to prevent this is to just take the far lock.
  oo::JobCounter terrainLoadDone{1};
  oo::RenderJobManager::runJob([this]() {
    std::scoped_lock farLock{mFarMutex};
    mWrld->loadTerrain(*mNearCells.back());
  }, &terrainLoadDone);
  terrainLoadDone.wait();

  oo::JobCounter cacheAddDone{1};
  oo::RenderJobManager::runJob([this, &ctx]() {
    ctx.getCellCache()->push_back(mNearCells.back());
  }, &cacheAddDone);
  cacheAddDone.wait();
}

void GameMode::unloadNearExteriorCell(oo::BaseId cellId,
                                      ApplicationContext &ctx) {
  std::scoped_lock lock{mNearMutex};

  auto _ = gsl::finally([&]() {
    auto it{std::find(mNearLoaded.begin(), mNearLoaded.end(), cellId)};
    mNearLoaded.erase(it);
  });

  // By construction, this cell is loaded.

  // Find the loaded cell
  auto jt{std::find_if(mNearCells.begin(), mNearCells.end(),
                       [&](const auto &cellPtr) {
                         return cellPtr->getBaseId() == cellId;
                       })};

  // If it's cached, then need to hide the scene root and remove it from
  // nearCells; this won't unload the terrain since it's still in the cache.
  if (auto[cellPtr, isInterior]{ctx.getCellCache()->get(cellId)};
      cellPtr && !isInterior) {
    static_cast<oo::ExteriorCell *>(cellPtr.get())->setVisible(false);
    mNearCells.erase(jt);
    return;
  }

  // Otherwise, removing from nearCells will delete the only remaining pointer
  // and unload the cell.
  mNearCells.erase(jt);
}

void GameMode::reifyNearNeighborhood(World::CellIndex centerCell,
                                     ApplicationContext &ctx) {
  const auto &gameSettings{oo::GameSettings::getSingleton()};
  const auto nearDiameter{gameSettings.get<unsigned int>(
      "General.uGridsToLoad", 3)};

  // Get the set of near neighbours that we want to end up being loaded.
  auto nearNbrsArray{mWrld->getNeighbourhood(centerCell, nearDiameter)};
  std::set<oo::BaseId> nearNbrsNew{};
  for (const auto &row : nearNbrsArray) {
    for (auto id : row) {
      nearNbrsNew.emplace(id);
    }
  }

  // Don't need to take the mNearMutex because only one reifyNearNeighbourhood
  // can occur at a time and therefore reifyNearExteriorCell is not executing.

  // Get the set of near neighbours that are currently loaded but that we don't
  // want to be.
  std::set<oo::BaseId> nearToUnload{};
  std::set_difference(mNearLoaded.begin(), mNearLoaded.end(),
                      nearNbrsNew.begin(), nearNbrsNew.end(),
                      std::inserter(nearToUnload, nearToUnload.end()));

  // Get the set of all near neighbours that need loading and start jobs to
  // load them all. Some might already be loaded, which is fine.
  std::set<oo::BaseId> nearToLoad{};
  std::set_difference(nearNbrsNew.begin(), nearNbrsNew.end(),
                      mNearLoaded.begin(), mNearLoaded.end(),
                      std::inserter(nearToLoad, nearToLoad.end()));

  oo::JobCounter nearLoadJc{static_cast<int>(nearToLoad.size())};
  for (auto id : nearToLoad) {
    oo::JobManager::runJob([this, id, &ctx]() {
      this->reifyNearExteriorCell(id, ctx);
    }, &nearLoadJc);
  }

  oo::JobCounter nearUnloadJc{static_cast<int>(nearToUnload.size())};
  for (auto id : nearToUnload) {
    oo::RenderJobManager::runJob([this, id, &ctx]() {
      this->unloadNearExteriorCell(id, ctx);
    }, &nearUnloadJc);
  }

  nearLoadJc.wait();
  nearUnloadJc.wait();
}

void GameMode::reifyNeighborhood(World::CellIndex centerCell,
                                 ApplicationContext &ctx) {
  std::scoped_lock lock{mReifyMutex};
  ctx.getLogger()->info("[{}]: reifyNeighborhood()",
                        boost::this_fiber::get_id());
  reifyNearNeighborhood(centerCell, ctx);
  reifyFarNeighborhood(centerCell, ctx);
}

bool GameMode::updateCenterCell(ApplicationContext &ctx) {
  // TODO: Write a toBSCoordinates inverse of fromBSCoordinates
  auto pos{mPlayerController->getPosition()};
  auto cellIndex{mWrld->getCellIndex(pos.x * oo::unitsPerMeter<float>,
                                     -pos.z * oo::unitsPerMeter<float>)};

  if (cellIndex != mCenterCell) {
    mCenterCell = cellIndex;
    return true;
  }

  return false;
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
        reifyNeighborhood(mCenterCell, ctx);
      });
    }
    oo::RenderJobManager::runJob([&]() {
      mWrld->updateAtmosphere(
          chrono::duration_cast<chrono::minutes>(now - today));
    });
  }

  if (mDebugDrawer) {
    mDebugDrawer->clearLines();
    getPhysicsWorld()->debugDrawWorld();

    auto it{getSceneManager()->getMovableObjectIterator("Entity")};
    while (it.hasMoreElements()) {
      auto *entity{static_cast<Ogre::Entity *>(it.getNext())};
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
    ctx.getLogger()->info("Looking at {}", refUnderCrosshair);
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
