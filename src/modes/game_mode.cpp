#include "game_settings.hpp"
#include "gui/menu.hpp"
#include "meta.hpp"
#include "modes/console_mode.hpp"
#include "modes/game_mode.hpp"
#include "modes/loading_menu_mode.hpp"
#include "modes/menu_mode.hpp"
#include "settings.hpp"
#include "sdl/sdl.hpp"
#include <spdlog/fmt/ostr.h>

namespace oo {

void
releasePlayerController(Cell *cell, oo::PlayerController *playerController) {
  const auto *rigidBody{playerController->getRigidBody()};
  if (rigidBody->isInWorld()) {
    cell->getPhysicsWorld()->removeRigidBody(playerController->getRigidBody());
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
        [&ctx](oo::event::MenuMode e) -> transition_t {
          if (e.down) {
            return {false, oo::LoadingMenuMode(ctx)};
          }
          return {};
        },
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
  auto *const btDispatcher{mCell->getPhysicsWorld()->getDispatcher()};
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
  mCell->getPhysicsWorld()->rayTest(rayStart, rayEnd, callback);

  if (callback.hasHit()) {
    return RefId{decodeFormId(callback.m_collisionObject->getUserPointer())};
  } else {
    return RefId{};
  }
}

void GameMode::loadCell(ApplicationContext &ctx, BaseId cellId) {
  auto &cellRes{oo::getResolver<record::CELL>(ctx.getBaseResolvers())};
  if (!cellRes.contains(cellId)) {
    throw std::runtime_error("Cell does not exist");
  }
  const auto cellRec{cellRes.get(cellId)};

  auto baseResolvers{oo::getResolvers<
      record::STAT, record::DOOR, record::LIGH, record::ACTI,
      record::NPC_, record::RACE>(ctx.getBaseResolvers())};
  auto refrResolvers{oo::getRefrResolvers<
      record::REFR_STAT, record::REFR_DOOR, record::REFR_LIGH,
      record::REFR_ACTI, record::REFR_NPC_>(ctx.getRefrResolvers())};

  cellRes.load(cellId, refrResolvers, baseResolvers);
  mCell = reifyRecord(*cellRec, nullptr, nullptr, std::tuple_cat(
      baseResolvers, refrResolvers,
      oo::getResolvers<record::CELL>(ctx.getBaseResolvers())));

  ctx.getLogger()->info("Loaded cell {}", cellId);

  mPlayerController = makePlayerController(mCell, mCell->getSceneManager(),
                                           mCell->getPhysicsWorld());
  oo::PlayerController *controller{mPlayerController.get()};
  mCollisionCaller.addCallback(
      mPlayerController->getRigidBody(),
      [controller](const auto *other, const auto &contact) {
        controller->handleCollision(other, contact);
      });

  ctx.setCamera(gsl::make_not_null(mPlayerController->getCamera()));

  const auto startPos = []() {
    // Exterior
//    auto pos{oo::fromBSCoordinates(Ogre::Vector3{103799.0f, -152970.0f, 1575.0f})};
//    pos.y += 4.0f;

    auto pos{oo::fromBSCoordinates(Ogre::Vector3::ZERO)};
    pos.z += 3.0f;
    pos.y -= 4.0f;
    return pos;
  }();
  mPlayerController->moveTo(startPos);

  mCell->getSceneManager()->addRenderQueueListener(ctx.getImGuiManager());
  mCell->getSceneManager()->addRenderQueueListener(ctx.getOverlaySystem());
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
  for (auto it{mCell->getSceneManager()->getMovableObjectIterator("Entity")};
       it.hasMoreElements();) {
    const auto *entity{static_cast<const Ogre::Entity *>(it.getNext())};

    const auto *anims{entity->getAllAnimationStates()};
    if (!anims) continue;

    const auto &animStates{anims->getEnabledAnimationStates()};
    for (auto state : animStates) state->addTime(delta);
  }
}

void GameMode::enter(ApplicationContext &ctx) {
  loadCell(ctx, BaseId{0x00'031b59}); // Cheydinhal County Hall
//    loadCell(ctx, BaseId{0x00'048706}); // Horse Whisperer Stables
//  loadCell(ctx, BaseId{0x00'00619e}); // (25, -38)
  refocus(ctx);
}

void GameMode::refocus(ApplicationContext &) {
  sdl::setRelativeMouseMode(true);
}

void GameMode::update(ApplicationContext &ctx, float delta) {
  updateAnimation(delta);
  mPlayerController->update(delta);
  mCell->getPhysicsWorld()->stepSimulation(delta);
  dispatchCollisions();

  if (mDebugDrawer) {
    mDebugDrawer->clearLines();
    mCell->getPhysicsWorld()->debugDrawWorld();

    auto it{mCell->getSceneManager()->getMovableObjectIterator("Entity")};
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
  if (!mCell) return;
  if (mDebugDrawer) {
    mCell->getSceneManager()->destroySceneNode("__DebugDrawerNode");
    mCell->getPhysicsWorld()->setDebugDrawer(nullptr);
    mDebugDrawer.reset();
  } else {
    mDebugDrawer = std::make_unique<Ogre::DebugDrawer>(mCell->getSceneManager(),
                                                       oo::SHADER_GROUP);
    auto *root{mCell->getSceneManager()->getRootSceneNode()};
    auto *node{root->createChildSceneNode("__DebugDrawerNode")};
    node->attachObject(mDebugDrawer->getObject());
    mCell->getPhysicsWorld()->setDebugDrawer(mDebugDrawer.get());
  }
}

} // namespace oo