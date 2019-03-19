#ifndef OPENOBLIVION_APPLICATION_CONTEXT_HPP
#define OPENOBLIVION_APPLICATION_CONTEXT_HPP

#include "bullet/collision.hpp"
#include "bullet/configuration.hpp"
#include "bsa_archive_factory.hpp"
#include "fnt_loader.hpp"
#include "ogre/tex_image_codec.hpp"
#include "ogre/text_resource_manager.hpp"
#include "ogre/window.hpp"
#include "ogrebullet/collision_object_manager.hpp"
#include "ogrebullet/rigid_body.hpp"
#include "ogreimgui/imgui_manager.hpp"
#include "ogresoloud/sound_manager.hpp"
#include "ogresoloud/wav_resource_manager.hpp"
#include "resolvers/resolvers.hpp"
#include "sdl/sdl.hpp"
#include <absl/container/flat_hash_map.h>
#include <gsl/gsl>
#include <OgreOverlaySystem.h>
#include <OgreRoot.h>
#include <OGRE/Terrain/OgreTerrain.h>
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
class WorldCache;

namespace event {

class KeyMap;

} // namespace event

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
  std::unique_ptr<Ogre::TerrainGlobalOptions> terrainOptions{};

  std::unique_ptr<oo::ConsoleEngine> consoleEngine{};
  std::unique_ptr<oo::ScriptEngine> scriptEngine{};

  /// Map of persistent references to the cells that contain them.
  /// This is mostly useful for looking up where doors go, but also necessary
  /// for implementing the `getParentCell` OBSE function.
  absl::flat_hash_map<oo::RefId, oo::BaseId> persistentRefMap{};

  std::unique_ptr<oo::MeshLoader> nifLoader{};
  std::unique_ptr<oo::CollisionObjectLoader> nifCollisionLoader{};
  std::unique_ptr<oo::SkeletonLoader> skeletonLoader{};

  oo::FntLoader fntLoader{};

  std::unique_ptr<Ogre::NifResourceManager> nifResourceMgr{};
  std::unique_ptr<Ogre::CollisionObjectManager> collisionObjectMgr{};
  std::unique_ptr<Ogre::TextResourceManager> textResourceMgr{};
  std::unique_ptr<Ogre::WavResourceManager> wavResourceMgr{};

  std::unique_ptr<oo::BaseResolvers> baseResolvers{};
  std::unique_ptr<oo::RefrResolvers> refrResolvers{};

  std::unique_ptr<oo::EspCoordinator> espCoordinator{};
  std::unique_ptr<oo::WorldCache> worldCache{};
  std::unique_ptr<oo::CellCache> cellCache{};

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

  absl::flat_hash_map<oo::RefId, oo::BaseId> &
  getPersistentReferenceMap() noexcept;

  const absl::flat_hash_map<oo::RefId, oo::BaseId> &
  getPersistentReferenceMap() const noexcept;

  oo::ConsoleEngine &getConsoleEngine();
  oo::ScriptEngine &getScriptEngine();

  Ogre::OverlaySystem *getOverlaySystem();

  oo::WorldCache *getWorldCache();
  oo::CellCache *getCellCache();

  void setCamera(gsl::not_null<Ogre::Camera *> camera);
};

} // namespace oo

#endif // OPENOBLIVION_APPLICATION_CONTEXT_HPP
