#include "application_context.hpp"
#include "cell_cache.hpp"
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
#include "resolvers/npc_resolver.hpp"
#include "resolvers/static_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include "scripting/console_engine.hpp"
#include "scripting/script_engine.hpp"
#include "world_cache.hpp"

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

oo::BaseResolversRef ApplicationContext::getBaseResolvers() const {
  return std::make_from_tuple<oo::BaseResolversRef>(*baseResolvers);
}

oo::RefrResolversRef ApplicationContext::getRefrResolvers() const {
  return std::make_from_tuple<oo::RefrResolversRef>(*refrResolvers);
}

absl::flat_hash_map<oo::RefId, oo::BaseId> &
oo::ApplicationContext::getPersistentReferenceMap() noexcept {
  return persistentRefMap;
}

const absl::flat_hash_map<oo::RefId, oo::BaseId> &
oo::ApplicationContext::getPersistentReferenceMap() const noexcept {
  return persistentRefMap;
}

oo::ConsoleEngine &ApplicationContext::getConsoleEngine() {
  return *consoleEngine;
}

oo::ScriptEngine &ApplicationContext::getScriptEngine() {
  return *scriptEngine;
}

Ogre::OverlaySystem *ApplicationContext::getOverlaySystem() {
  return overlaySys.get();
}

oo::CellCache *ApplicationContext::getCellCache() {
  return cellCache.get();
}

oo::WorldCache *ApplicationContext::getWorldCache() {
  return worldCache.get();
}

void ApplicationContext::setCamera(gsl::not_null<Ogre::Camera *> camera) {
  setCameraAspectRatio(camera);
  auto &windowPtr{std::get<Ogre::RenderWindowPtr>(windows)};
  if (windowPtr->hasViewportWithZOrder(0)) {
    windowPtr->removeViewport(0);
  }
  std::get<Ogre::RenderWindowPtr>(windows)->addViewport(camera);
  ogreRoot->getRenderSystem()->_setViewport(camera->getViewport());
  auto &compMgr{Ogre::CompositorManager::getSingleton()};
  auto *compInstance{compMgr.addCompositor(camera->getViewport(), "Post")};
  compInstance->setEnabled(true);
}

} // namespace oo