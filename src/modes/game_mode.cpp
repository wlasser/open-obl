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
  mExteriorCells = std::move(cellPacket.mExteriorCells);

  mPlayerStartPos = std::move(cellPacket.mPlayerPosition);
  mPlayerStartOrientation = std::move(cellPacket.mPlayerOrientation);

  mInInterior = mExteriorCells.empty();
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

void GameMode::reifyExteriorCell(oo::BaseId cellId, ApplicationContext &ctx) {
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
  mExteriorCells.emplace_back(std::static_pointer_cast<oo::ExteriorCell>(
      reifyRecord(*cellRec, mWrld->getSceneManager(), mWrld->getPhysicsWorld(),
                  getCellResolvers(ctx))));
  mWrld->loadTerrain(*mExteriorCells.back());

  ctx.getLogger()->info("Loaded cell {}", cellId);
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
  return mExteriorCells.front()->getSceneManager();
}

gsl::not_null<btDiscreteDynamicsWorld *> GameMode::getPhysicsWorld() const {
  if (mInInterior) return mCell->getPhysicsWorld();
  return mExteriorCells.front()->getPhysicsWorld();
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
  sdl::setRelativeMouseMode(true);
}

void GameMode::reifyNeighborhood(World::CellIndex centerCell,
                                 ApplicationContext &ctx) {
  const auto &gameSettings{oo::GameSettings::getSingleton()};
  const auto nearDiameter{gameSettings.get<unsigned int>(
      "General.uGridsToLoad", 3)};
  const auto farDiameter{gameSettings.get<unsigned int>(
      "General.uGridDistantCount", 5)};

  const auto &cache{*ctx.getCellCache()};

  // Find all cells in the far neighbourhood and make sure that they are loaded.
  // The set will be used later to provide fast lookup of cells that need to be
  // unloaded.
  auto farNeighbours{mWrld->getNeighbourhood(centerCell, farDiameter)};
  absl::flat_hash_set<oo::BaseId> farNeighbourSet;
  for (const auto &row : farNeighbours) {
    for (auto id : row) {
      farNeighbourSet.emplace(id);
      // C++20: if (mFarExteriorCells.contains(id)) {
      if (std::find(mFarExteriorCells.begin(), mFarExteriorCells.end(), id)
          != mFarExteriorCells.end()) {
        // Already loaded, skip
        continue;
      } else if (auto[cell, isInterior]{cache.get(id)}; cell && !isInterior) {
        // In cache, so its terrain has already been loaded and will stay loaded
        // even if it gets pushed out the cache later.
        // TODO: Show the terrain
        mFarExteriorCells.emplace_back(id);
      } else {
        mFarExteriorCells.emplace_back(id);
        mWrld->loadTerrainOnly(id, false);
      }
    }
  }

  // Find all cells in the near neighbourhood, loading any that aren't loaded.
  auto nearNeighbours{mWrld->getNeighbourhood(centerCell, nearDiameter)};
  absl::flat_hash_set<oo::BaseId> nearNeighbourSet;
  for (const auto &row : nearNeighbours) {
    for (auto id : row) {
      nearNeighbourSet.emplace(id);
      auto p = [id](const auto &cell) { return cell->getBaseId() == id; };
      if (std::any_of(mExteriorCells.begin(), mExteriorCells.end(), p)) {
        // Already loaded, skip
        continue;
      } else if (auto[cell, isInterior]{cache.get(id)}; cell && !isInterior) {
        // In cache so just copy the pointer
        mExteriorCells.emplace_back(
            std::static_pointer_cast<oo::ExteriorCell>(cell));
        static_cast<oo::ExteriorCell *>(cell.get())->setVisible(true);
        ctx.getLogger()->info("Loaded cell {} from cache", id);
      } else {
        reifyExteriorCell(id, ctx);
        ctx.getCellCache()->push_back(mExteriorCells.back());
      }
    }
  }

  // Remove any previously loaded far cells that are not in the new far
  // neighbourhood, but only remove their terrain if they are not cached (this
  // can happen with sufficiently large cache sizes or close near/far
  // neighbourhoods). Note that the near neighbour is a subset of the far
  // neighbourhood.
  // Use std::partition instead of std::remove_if because we need
  // to do some processing on the cells to be removed before getting rid of
  // them; std::remove_if leaves the to-be-removed elements in a valid but
  // unspecified state.
  {
    auto begin{mFarExteriorCells.begin()}, end{mFarExteriorCells.end()};
    auto it{std::partition(begin, end, [&farNeighbourSet](auto id) {
      return farNeighbourSet.contains(id);
    })};
    for (auto jt = it; jt != end; ++jt) {
      if (std::find_if(cache.exteriors().begin(), cache.exteriors().end(),
                       [&jt](const auto &cellPtr) {
                         return cellPtr->getBaseId() == *jt;
                       }) == cache.exteriors().end()) {
        mWrld->unloadTerrain(*jt);
      }
    }
    mFarExteriorCells.erase(it, end);
  }

  // Remove any previously loaded near cells that are not in the new near
  // neighbour, removing their collision object if they are not cached, but not
  // unloading their terrain.
  // not unload their terrain.
  {
    auto begin{mExteriorCells.begin()}, end{mExteriorCells.end()};
    auto it{std::partition(begin, end, [&nearNeighbourSet](const auto &ptr) {
      return nearNeighbourSet.contains(ptr->getBaseId());
    })};
    for (auto jt = it; jt != end; ++jt) {
      auto kt{std::find_if(cache.exteriors().begin(), cache.exteriors().end(),
                           [&jt](const auto &cellPtr) {
                             return cellPtr->getBaseId() == (*jt)->getBaseId();
                           })};
      if (kt != cache.exteriors().end()) {
        static_cast<oo::ExteriorCell *>(kt->get())->setVisible(false);
      } else {
        getPhysicsWorld()->removeCollisionObject((*jt)->getCollisionObject());
      }
    }
    mExteriorCells.erase(it, end);
  }
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
      reifyNeighborhood(mCenterCell, ctx);
    }
    mWrld->updateAtmosphere(
        chrono::duration_cast<chrono::minutes>(now - today));
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
