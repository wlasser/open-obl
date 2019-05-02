#include "application_context.hpp"
#include "cell_cache.hpp"
#include "controls.hpp"
#include "deferred_light_pass.hpp"
#include "esp_coordinator.hpp"
#include "game_settings.hpp"
#include "mesh/entity.hpp"
#include "mesh/mesh_manager.hpp"
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
#include "scene_manager.hpp"
#include "scripting/console_engine.hpp"
#include "scripting/script_engine.hpp"
#include <OgreCamera.h>
#include <OgreCompositorInstance.h>
#include <OgreCompositorManager.h>

namespace oo {

ApplicationContext::ApplicationContext()
    : deferredLightPass{std::make_unique<oo::DeferredLightPass>()},
      nifLoader{std::make_unique<oo::MeshLoader>()},
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
  camera->setNearClipDistance(0.2f);
  camera->setFarClipDistance(30000.0f);

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

oo::PersistentReferenceLocator &
oo::ApplicationContext::getPersistentReferenceLocator() noexcept {
  return persistentRefLocator;
}

const oo::PersistentReferenceLocator &
oo::ApplicationContext::getPersistentReferenceLocator() const noexcept {
  return persistentRefLocator;
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

const oo::EspCoordinator &ApplicationContext::getCoordinator() const {
  return *espCoordinator.get();
}

oo::CellCache *ApplicationContext::getCellCache() {
  return cellCache.get();
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
  auto *gBufferInstance{compMgr.addCompositor(camera->getViewport(),
                                              "DeferredGBuffer")};
  gBufferInstance->setEnabled(true);

  auto *deferred{compMgr.addCompositor(camera->getViewport(),
                                       "DeferredShading")};
  auto *scnMgr{camera->getSceneManager()};
  if (auto *dScnMgr{dynamic_cast<oo::DeferredSceneManager *>(scnMgr)}) {
    deferred->addListener(dScnMgr->getFogListener());
  }
  deferred->setEnabled(true);
}

} // namespace oo