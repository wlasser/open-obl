#ifndef OPENOBLIVION_APPLICATION_CONTEXT_HPP
#define OPENOBLIVION_APPLICATION_CONTEXT_HPP

#include "bullet/collision.hpp"
#include "bullet/configuration.hpp"
#include "bsa_archive_factory.hpp"
#include "controls.hpp"
#include "esp_coordinator.hpp"
#include "nifloader/mesh_loader.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include "nifloader/collision_object_loader.hpp"
#include "ogre/text_resource_manager.hpp"
#include "ogre/window.hpp"
#include "ogrebullet/collision_object_manager.hpp"
#include "ogrebullet/rigid_body.hpp"
#include "ogreimgui/imgui_manager.hpp"
#include "resolvers/interior_cell_resolver.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/static_resolver.hpp"
#include "sdl/sdl.hpp"
#include <gsl/gsl>
#include <Ogre.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <optional>

class ApplicationContext {
 private:
  friend class Application;

  std::unique_ptr<Ogre::BsaArchiveFactory> bsaArchiveFactory{};
  std::unique_ptr<Ogre::RigidBodyFactory> rigidBodyFactory{};

  std::shared_ptr<spdlog::logger> logger{};

  std::unique_ptr<Ogre::Root> ogreRoot{};
  std::unique_ptr<sdl::Init> sdlInit{};

  std::tuple<sdl::WindowPtr, Ogre::RenderWindowPtr> windows{
      std::make_tuple<sdl::WindowPtr, Ogre::RenderWindowPtr>(
          {nullptr, nullptr}, nullptr
      )};

  std::unique_ptr<KeyMap> keyMap{};

  std::unique_ptr<bullet::Configuration> bulletConf{};

  std::unique_ptr<Ogre::ImGuiManager> imguiMgr{};

  nifloader::MeshLoader nifLoader{};
  nifloader::CollisionObjectLoader nifCollisionLoader{};

  std::unique_ptr<Ogre::NifResourceManager> nifResourceMgr{};
  std::unique_ptr<Ogre::CollisionObjectManager> collisionObjectMgr{};
  std::unique_ptr<Ogre::TextResourceManager> textResourceMgr{};

  std::unique_ptr<DoorResolver> doorRes{};
  std::unique_ptr<LightResolver> lightRes{};
  std::unique_ptr<StaticResolver> staticRes{};
  std::unique_ptr<InteriorCellResolver> interiorCellRes{};

  std::unique_ptr<esp::EspCoordinator> espCoordinator{};

 public:
  std::shared_ptr<spdlog::logger> getLogger() {
    return logger;
  }

  Ogre::Root &getRoot() {
    return *ogreRoot;
  }

  KeyMap &getKeyMap() {
    return *keyMap;
  }

  Ogre::ImGuiManager *getImGuiManager() const {
    return imguiMgr.get();
  }

  const DoorResolver &getDoorResolver() const {
    return *doorRes;
  }

  const LightResolver &getLightResolvers() const {
    return *lightRes;
  }

  const StaticResolver &getStaticResolver() const {
    return *staticRes;
  }

  const InteriorCellResolver &getInteriorCellResolver() const {
    return *interiorCellRes;
  }

  void setCamera(Ogre::Camera *camera) {
    std::get<Ogre::RenderWindowPtr>(windows)->addViewport(camera);
    ogreRoot->getRenderSystem()->_setViewport(camera->getViewport());
  }
};

#endif // OPENOBLIVION_APPLICATION_CONTEXT_HPP
