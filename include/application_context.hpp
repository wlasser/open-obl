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
#include "resolvers/cell_resolver.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/static_resolver.hpp"
#include "scripting/console_engine.hpp"
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

  std::unique_ptr<oo::event::KeyMap> keyMap{};

  std::unique_ptr<bullet::Configuration> bulletConf{};

  std::unique_ptr<Ogre::ImGuiManager> imguiMgr{};

  std::unique_ptr<oo::ConsoleEngine> consoleEngine{};

  oo::MeshLoader nifLoader{};
  oo::CollisionObjectLoader nifCollisionLoader{};

  std::unique_ptr<Ogre::NifResourceManager> nifResourceMgr{};
  std::unique_ptr<Ogre::CollisionObjectManager> collisionObjectMgr{};
  std::unique_ptr<Ogre::TextResourceManager> textResourceMgr{};

  std::unique_ptr<DoorResolver> doorRes{};
  std::unique_ptr<LightResolver> lightRes{};
  std::unique_ptr<StaticResolver> staticRes{};
  std::unique_ptr<RefrDoorResolver> refrDoorRes{};
  std::unique_ptr<RefrLightResolver> refrLightRes{};
  std::unique_ptr<RefrStaticResolver> refrStaticRes{};
  std::unique_ptr<CellResolver> cellRes{};

  std::unique_ptr<oo::EspCoordinator> espCoordinator{};

 public:
  std::shared_ptr<spdlog::logger> getLogger() {
    return logger;
  }

  Ogre::Root &getRoot() {
    return *ogreRoot;
  }

  oo::event::KeyMap &getKeyMap() {
    return *keyMap;
  }

  Ogre::ImGuiManager *getImGuiManager() const {
    return imguiMgr.get();
  }

  DoorResolver &getDoorResolver() const {
    return *doorRes;
  }

  LightResolver &getLightResolver() const {
    return *lightRes;
  }

  StaticResolver &getStaticResolver() const {
    return *staticRes;
  }

  RefrDoorResolver &getRefrDoorResolver() const {
    return *refrDoorRes;
  }

  RefrLightResolver &getRefrLightResolver() const {
    return *refrLightRes;
  }

  RefrStaticResolver &getRefrStaticResolver() const {
    return *refrStaticRes;
  }

  CellResolver &getCellResolver() const {
    return *cellRes;
  }

  oo::ConsoleEngine &getConsoleEngine() {
    return *consoleEngine;
  }

  void setCamera(Ogre::Camera *camera) {
    std::get<Ogre::RenderWindowPtr>(windows)->addViewport(camera);
    ogreRoot->getRenderSystem()->_setViewport(camera->getViewport());
  }
};

#endif // OPENOBLIVION_APPLICATION_CONTEXT_HPP
