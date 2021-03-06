#ifndef OPENOBL_APPLICATION_CONTEXT_HPP
#define OPENOBL_APPLICATION_CONTEXT_HPP

#include "bullet/collision.hpp"
#include "bullet/configuration.hpp"
#include "ogre/bsa_archive_factory.hpp"
#include "ogre/fnt_loader.hpp"
#include "ogre/tex_image_codec.hpp"
#include "ogre/text_resource_manager.hpp"
#include "ogre/window.hpp"
#include "ogrebullet/collision_shape_manager.hpp"
#include "ogrebullet/rigid_body.hpp"
#include "ogreimgui/imgui_manager.hpp"
#include "ogresoloud/sound_manager.hpp"
#include "ogresoloud/wav_resource_manager.hpp"
#include "persistent_reference_locator.hpp"
#include "resolvers/resolvers.hpp"
#include "sdl/sdl.hpp"
#include <gsl/gsl>
#include <OgreOverlaySystem.h>
#include <RenderSystems/GL3Plus/OgreGL3PlusPlugin.h>
#include <OgreRoot.h>
#include <Terrain/OgreTerrain.h>
#include <spdlog/spdlog.h>
#include <memory>

namespace Ogre {

class NifResourceManager;

} // namespace Ogre

namespace oo {

class EspCoordinator;
class ConsoleEngine;
class MeshLoader;
class CollisionObjectLoader;
class ScriptEngine;
class SkeletonLoader;
class CellCache;
class MeshManager;
class MusicManager;
class EntityFactory;
class DeferredLightFactory;
class DeferredLightPass;
class DeferredSceneManager;
class DeferredSceneManagerFactory;
class InteriorSceneManager;

namespace event {

class KeyMap;

} // namespace event

class ApplicationContext {
 private:
  friend class Application;

  std::unique_ptr<Ogre::GL3PlusPlugin> gl3PlusPlugin{};
  std::unique_ptr<Ogre::BsaArchiveFactory> bsaArchiveFactory{};
  std::unique_ptr<Ogre::RigidBodyFactory> rigidBodyFactory{};
  std::unique_ptr<oo::EntityFactory> entityFactory;
  std::unique_ptr<oo::DeferredLightFactory> lightFactory;
  std::unique_ptr<oo::DeferredSceneManagerFactory> scnMgrFactory;

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

  std::unique_ptr<oo::DeferredLightPass> deferredLightPass;

  std::unique_ptr<Ogre::SoundManager> soundMgr{};
  std::unique_ptr<oo::MusicManager> musicMgr;

  std::unique_ptr<Ogre::OverlaySystem> overlaySys{};
  std::unique_ptr<Ogre::TerrainGlobalOptions> terrainOptions{};

  std::unique_ptr<oo::ConsoleEngine> consoleEngine;
  std::unique_ptr<oo::ScriptEngine> scriptEngine;

  oo::PersistentReferenceLocator persistentRefLocator{};

  std::unique_ptr<oo::MeshLoader> nifLoader;
  std::unique_ptr<oo::CollisionObjectLoader> nifCollisionLoader;
  std::unique_ptr<oo::SkeletonLoader> skeletonLoader;

  oo::FntLoader fntLoader{};

  std::unique_ptr<oo::MeshManager> meshMgr;
  std::unique_ptr<Ogre::NifResourceManager> nifResourceMgr;
  std::unique_ptr<Ogre::CollisionShapeManager> collisionObjectMgr{};
  std::unique_ptr<Ogre::TextResourceManager> textResourceMgr{};
  std::unique_ptr<Ogre::WavResourceManager> wavResourceMgr{};

  std::unique_ptr<oo::BaseResolvers> baseResolvers{};
  std::unique_ptr<oo::RefrResolvers> refrResolvers{};

  std::unique_ptr<oo::EspCoordinator> espCoordinator;
  std::unique_ptr<oo::CellCache> cellCache;

  void setCameraAspectRatio(gsl::not_null<Ogre::Camera *> camera) const;

 public:
  ApplicationContext();
  ~ApplicationContext();
  ApplicationContext(const ApplicationContext &) = delete;
  ApplicationContext &operator=(const ApplicationContext &) = delete;
  ApplicationContext(ApplicationContext &&) = delete;
  ApplicationContext &operator=(ApplicationContext &&) = delete;

  std::shared_ptr<spdlog::logger> getLogger();

  Ogre::Root &getRoot();

  oo::event::KeyMap &getKeyMap();

  Ogre::ImGuiManager *getImGuiManager() const;

  oo::BaseResolversRef getBaseResolvers() const;
  oo::RefrResolversRef getRefrResolvers() const;

  oo::PersistentReferenceLocator &
  getPersistentReferenceLocator() noexcept;

  const oo::PersistentReferenceLocator &
  getPersistentReferenceLocator() const noexcept;

  oo::ConsoleEngine &getConsoleEngine();
  oo::ScriptEngine &getScriptEngine();

  oo::MusicManager &getMusicManager();

  Ogre::OverlaySystem *getOverlaySystem();

  const oo::EspCoordinator &getCoordinator() const;
  oo::CellCache *getCellCache();

  void setCamera(gsl::not_null<Ogre::Camera *> camera);
};

} // namespace oo

#endif // OPENOBL_APPLICATION_CONTEXT_HPP
