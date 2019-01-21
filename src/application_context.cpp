#include "application_context.hpp"
#include "controls.hpp"
#include "esp_coordinator.hpp"
#include "game_settings.hpp"
#include "nifloader/mesh_loader.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include "nifloader/collision_object_loader.hpp"
#include "nifloader/skeleton_loader.hpp"
#include "resolvers/acti_resolver.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/static_resolver.hpp"
#include "scripting/console_engine.hpp"

namespace oo {

ApplicationContext::ApplicationContext()
    : nifLoader{std::make_unique<oo::MeshLoader>()},
      nifCollisionLoader{std::make_unique<oo::CollisionObjectLoader>()},
      skeletonLoader{std::make_unique<oo::SkeletonLoader>()} {}

ApplicationContext::~ApplicationContext() = default;

void ApplicationContext::setCameraAspectRatio(gsl::not_null<Ogre::Camera *> camera) const {
  const auto &settings{GameSettings::getSingleton()};
  const auto screenWidth
      {gsl::narrow_cast<float>(settings.iGet("Display.iSize W"))};
  const auto screenHeight
      {gsl::narrow_cast<float>(settings.iGet("Display.iSize H"))};

  const float aspectRatio{screenWidth / screenHeight};
  camera->setAspectRatio(aspectRatio);
  camera->setNearClipDistance(0.1f);

  // We are given the horizontal fov, but can only set the vertical fov.
  // Internally Ogre probably undoes this operation so this is inefficient and
  // possibly inaccurate.
  {
    Ogre::Degree xFov{settings.get<float>("Display.fDefaultFOV", 75.0f)};
    xFov = Ogre::Math::Clamp(xFov.valueDegrees(), 1.0f, 179.0f);
    const auto tanXFov2{Ogre::Math::Tan(xFov / 2.0f)};
    Ogre::Degree yFov{2.0f * Ogre::Math::ATan(1.0f / aspectRatio * tanXFov2)};
    camera->setFOVy(yFov);
  }
}

std::shared_ptr<spdlog::logger> ApplicationContext::getLogger() {
  return logger;
}

Ogre::Root &ApplicationContext::getRoot() {
  return *ogreRoot;
}

oo::event::KeyMap &ApplicationContext::getKeyMap() {
  return *keyMap;
}

Ogre::ImGuiManager *ApplicationContext::getImGuiManager() const {
  return imguiMgr.get();
}

oo::DoorResolver &ApplicationContext::getDoorResolver() const {
  return *doorRes;
}

oo::LighResolver &ApplicationContext::getLighResolver() const {
  return *lighRes;
}

oo::StatResolver &ApplicationContext::getStatResolver() const {
  return *statRes;
}

oo::ActiResolver &ApplicationContext::getActiResolver() const {
  return *actiRes;
}

oo::RefrDoorResolver &ApplicationContext::getRefrDoorResolver() const {
  return *refrDoorRes;
}

oo::RefrLighResolver &ApplicationContext::getRefrLighResolver() const {
  return *refrLighRes;
}

oo::RefrStatResolver &ApplicationContext::getRefrStatResolver() const {
  return *refrStatRes;
}

oo::RefrActiResolver &ApplicationContext::getRefrActiResolver() const {
  return *refrActiRes;
}

oo::CellResolver &ApplicationContext::getCellResolver() const {
  return *cellRes;
}

oo::ConsoleEngine &ApplicationContext::getConsoleEngine() {
  return *consoleEngine;
}

Ogre::OverlaySystem *ApplicationContext::getOverlaySystem() {
  return overlaySys.get();
}

void ApplicationContext::setCamera(gsl::not_null<Ogre::Camera *> camera) {
  setCameraAspectRatio(camera);
  auto &windowPtr{std::get<Ogre::RenderWindowPtr>(windows)};
  if (windowPtr->hasViewportWithZOrder(0)) {
    windowPtr->removeViewport(0);
  }
  std::get<Ogre::RenderWindowPtr>(windows)->addViewport(camera);
  ogreRoot->getRenderSystem()->_setViewport(camera->getViewport());
}

} // namespace oo