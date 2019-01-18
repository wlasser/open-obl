#ifndef OPENOBLIVION_APPLICATION_CONTEXT_HPP
#define OPENOBLIVION_APPLICATION_CONTEXT_HPP

#include "bullet/collision.hpp"
#include "bullet/configuration.hpp"
#include "bsa_archive_factory.hpp"
#include "controls.hpp"
#include "esp_coordinator.hpp"
#include "game_settings.hpp"
#include "nifloader/mesh_loader.hpp"
#include "nifloader/nif_resource_manager.hpp"
#include "nifloader/collision_object_loader.hpp"
#include "nifloader/skeleton_loader.hpp"
#include "fnt_loader.hpp"
#include "ogre/tex_image_codec.hpp"
#include "ogre/text_resource_manager.hpp"
#include "ogre/window.hpp"
#include "ogrebullet/collision_object_manager.hpp"
#include "ogrebullet/rigid_body.hpp"
#include "ogreimgui/imgui_manager.hpp"
#include "ogresoloud/sound_manager.hpp"
#include "ogresoloud/wav_resource_manager.hpp"
#include "resolvers/acti_resolver.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/door_resolver.hpp"
#include "resolvers/light_resolver.hpp"
#include "resolvers/static_resolver.hpp"
#include "scripting/console_engine.hpp"
#include "sdl/sdl.hpp"
#include <gsl/gsl>
#include <Ogre.h>
#include <OgreOverlaySystem.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <optional>

namespace oo {

class ApplicationContext {
 private:
  friend class Application;

  std::unique_ptr<Ogre::BsaArchiveFactory> bsaArchiveFactory{};
  std::unique_ptr<Ogre::RigidBodyFactory> rigidBodyFactory{};

  std::unique_ptr<Ogre::TexImageCodec> texImageCodec{};

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

  std::unique_ptr<Ogre::SoundManager> soundMgr{};

  std::unique_ptr<Ogre::OverlaySystem> overlaySys{};

  std::unique_ptr<oo::ConsoleEngine> consoleEngine{};

  oo::MeshLoader nifLoader{};
  oo::CollisionObjectLoader nifCollisionLoader{};
  oo::FntLoader fntLoader{};
  oo::SkeletonLoader skeletonLoader{};

  std::unique_ptr<Ogre::NifResourceManager> nifResourceMgr{};
  std::unique_ptr<Ogre::CollisionObjectManager> collisionObjectMgr{};
  std::unique_ptr<Ogre::TextResourceManager> textResourceMgr{};
  std::unique_ptr<Ogre::WavResourceManager> wavResourceMgr{};

  std::unique_ptr<oo::DoorResolver> doorRes{};
  std::unique_ptr<oo::LighResolver> lighRes{};
  std::unique_ptr<oo::StatResolver> statRes{};
  std::unique_ptr<oo::ActiResolver> actiRes{};
  std::unique_ptr<oo::RefrDoorResolver> refrDoorRes{};
  std::unique_ptr<oo::RefrLighResolver> refrLighRes{};
  std::unique_ptr<oo::RefrStatResolver> refrStatRes{};
  std::unique_ptr<oo::RefrActiResolver> refrActiRes{};
  std::unique_ptr<oo::CellResolver> cellRes{};

  std::unique_ptr<oo::EspCoordinator> espCoordinator{};

 private:
  void setCameraAspectRatio(gsl::not_null<Ogre::Camera *> camera) const {
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

  oo::DoorResolver &getDoorResolver() const {
    return *doorRes;
  }

  oo::LighResolver &getLighResolver() const {
    return *lighRes;
  }

  oo::StatResolver &getStatResolver() const {
    return *statRes;
  }

  oo::ActiResolver &getActiResolver() const {
    return *actiRes;
  }

  oo::RefrDoorResolver &getRefrDoorResolver() const {
    return *refrDoorRes;
  }

  oo::RefrLighResolver &getRefrLighResolver() const {
    return *refrLighRes;
  }

  oo::RefrStatResolver &getRefrStatResolver() const {
    return *refrStatRes;
  }

  oo::RefrActiResolver &getRefrActiResolver() const {
    return *refrActiRes;
  }

  oo::CellResolver &getCellResolver() const {
    return *cellRes;
  }

  oo::ConsoleEngine &getConsoleEngine() {
    return *consoleEngine;
  }

  Ogre::OverlaySystem *getOverlaySystem() {
    return overlaySys.get();
  }

  void setCamera(gsl::not_null<Ogre::Camera *> camera) {
    setCameraAspectRatio(camera);
    auto &windowPtr{std::get<Ogre::RenderWindowPtr>(windows)};
    if (windowPtr->hasViewportWithZOrder(0)) {
      windowPtr->removeViewport(0);
    }
    std::get<Ogre::RenderWindowPtr>(windows)->addViewport(camera);
    ogreRoot->getRenderSystem()->_setViewport(camera->getViewport());
  }
};

} // namespace oo

#endif // OPENOBLIVION_APPLICATION_CONTEXT_HPP
