#include "game_settings.hpp"
#include "meta.hpp"
#include "modes/game_mode.hpp"
#include "sdl/sdl.hpp"
#include <spdlog/fmt/ostr.h>

void GameMode::handleEvent(ApplicationContext &ctx, const sdl::Event &event) {
  auto keyEvent{ctx.getKeyMap().translateKey(event)};
  if (keyEvent) {
    std::visit(overloaded{
        [this](event::Forward e) { mPlayerController->handleEvent(e); },
        [this](event::Backward e) { mPlayerController->handleEvent(e); },
        [this](event::SlideLeft e) { mPlayerController->handleEvent(e); },
        [this](event::SlideRight e) { mPlayerController->handleEvent(e); },
        [](event::Use) {},
        [](event::Activate) {},
        [](event::Block) {},
        [](event::Cast) {},
        [](event::ReadyItem) {},
        [this](event::Sneak e) { mPlayerController->handleEvent(e); },
        [this](event::Run e) { mPlayerController->handleEvent(e); },
        [this](event::AlwaysRun e) { mPlayerController->handleEvent(e); },
        [](event::AutoMove) {},
        [this](event::Jump e) { mPlayerController->handleEvent(e); },
        [](event::TogglePov) {},
        [](event::MenuMode) {},
        [](event::Rest) {},
        [](event::QuickMenu) {},
        [](event::Quick) {},
        [](event::QuickSave) {},
        [](event::QuickLoad) {},
        [](event::Grab) {},
        [&ctx](event::Console e) { ctx.getLogger()->info("Console pressed"); },
        [&ctx](event::SystemMenu e) { ctx.getRoot().queueEndRendering(); }
    }, *keyEvent);
    return;
  }

  if (sdl::typeOf(event) == sdl::EventType::MouseMotion) {
    const auto &settings{GameSettings::getSingleton()};
    const float sensitivity{settings.fGet("Controls.fMouseSensitivity")};
    const event::Pitch pitch{{event.motion.yrel * sensitivity}};
    const event::Yaw yaw{{event.motion.xrel * sensitivity}};
    mPlayerController->handleEvent(pitch);
    mPlayerController->handleEvent(yaw);
  }
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