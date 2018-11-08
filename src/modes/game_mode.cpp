#include "game_settings.hpp"
#include "meta.hpp"
#include "modes/console_mode.hpp"
#include "modes/game_mode.hpp"
#include "sdl/sdl.hpp"
#include <spdlog/fmt/ostr.h>

GameMode::transition_t
GameMode::handleEvent(ApplicationContext &ctx, const sdl::Event &event) {
  auto keyEvent{ctx.getKeyMap().translateKey(event)};
  if (keyEvent) {
    return std::visit(overloaded{
        [this](event::Forward e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [this](event::Backward e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [this](event::SlideLeft e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [this](event::SlideRight e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [](event::Use) -> transition_t { return {}; },
        [](event::Activate) -> transition_t { return {}; },
        [](event::Block) -> transition_t { return {}; },
        [](event::Cast) -> transition_t { return {}; },
        [](event::ReadyItem) -> transition_t { return {}; },
        [this](event::Sneak e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [this](event::Run e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [this](event::AlwaysRun e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [](event::AutoMove) -> transition_t { return {}; },
        [this](event::Jump e) -> transition_t {
          mPlayerController->handleEvent(e);
          return {};
        },
        [](event::TogglePov) -> transition_t { return {}; },
        [](event::MenuMode) -> transition_t { return {}; },
        [](event::Rest) -> transition_t { return {}; },
        [](event::QuickMenu) -> transition_t { return {}; },
        [](event::Quick) -> transition_t { return {}; },
        [](event::QuickSave) -> transition_t { return {}; },
        [](event::QuickLoad) -> transition_t { return {}; },
        [](event::Grab) -> transition_t { return {}; },
        [&ctx](event::Console e) -> transition_t {
          if (e.down) return {false, ConsoleMode(ctx)};
          return {};
        },
        [&ctx](event::SystemMenu e) -> transition_t {
          ctx.getRoot().queueEndRendering();
          return {};
        }
    }, *keyEvent);
  }

  if (sdl::typeOf(event) == sdl::EventType::MouseMotion) {
    const auto &settings{GameSettings::getSingleton()};
    const float sensitivity{settings.fGet("Controls.fMouseSensitivity")};
    const event::Pitch pitch{{event.motion.yrel * sensitivity}};
    const event::Yaw yaw{{event.motion.xrel * sensitivity}};
    mPlayerController->handleEvent(pitch);
    mPlayerController->handleEvent(yaw);
  }
  return {false, std::nullopt};
}

void GameMode::dispatchCollisions() {
  auto *const btDispatcher{mCell->physicsWorld->getDispatcher()};
  gsl::not_null dispatcher{dynamic_cast<btCollisionDispatcher *>(btDispatcher)};
  mCollisionCaller.runCallbacks(dispatcher);
}

RefId GameMode::getCrosshairRef() {
  using Ogre::conversions::toBullet;
  GameSetting<int> iActivatePickLength{"iActivatePickLength", 150};

  auto *const camera{mPlayerController->getCamera()};
  const auto cameraPos{toBullet(camera->getDerivedPosition())};
  const auto cameraDir{toBullet(camera->getDerivedDirection())};
  const auto rayStart{cameraPos + 0.5f * cameraDir};
  const float rayLength
      {*iActivatePickLength * conversions::metersPerUnit<float>};
  const auto rayEnd{cameraPos + rayLength * cameraDir};

  btCollisionWorld::ClosestRayResultCallback callback(rayStart, rayEnd);
  mCell->physicsWorld->rayTest(rayStart, rayEnd, callback);

  if (callback.hasHit()) {
    return RefId{decodeFormId(callback.m_collisionObject->getUserPointer())};
  } else {
    return RefId{};
  }
}

void GameMode::loadCell(ApplicationContext &ctx, BaseId cellId) {
  mCell = ctx.getInteriorCellResolver().make(cellId);
  ctx.getLogger()->info("Loaded cell {}", cellId);

  mPlayerController =
      std::make_unique<character::PlayerController>(mCell->scnMgr);
  mCell->physicsWorld->addRigidBody(mPlayerController->getRigidBody());
  character::PlayerController *controller{mPlayerController.get()};
  mCollisionCaller.addCallback(
      mPlayerController->getRigidBody(),
      [controller](const auto *other, const auto &contact) {
        controller->handleCollision(other, contact);
      });

  ctx.setCamera(mPlayerController->getCamera());

  const auto startPos = []() {
    auto pos{conversions::fromBSCoordinates({0, 0, 0})};
    pos.y += 4.0f;
    return pos;
  }();
  mPlayerController->moveTo(startPos);

  mCell->scnMgr->addRenderQueueListener(ctx.getImGuiManager());
}

void GameMode::update(ApplicationContext &ctx, float delta) {
  mPlayerController->update(delta);
  mCell->physicsWorld->stepSimulation(delta);
  dispatchCollisions();

  static RefId refUnderCrosshair{0};
  RefId newRefUnderCrosshair = getCrosshairRef();
  if (newRefUnderCrosshair != refUnderCrosshair) {
    refUnderCrosshair = newRefUnderCrosshair;
    ctx.getLogger()->info("Looking at {}", refUnderCrosshair);
  }
}