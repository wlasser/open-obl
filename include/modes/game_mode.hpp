#ifndef OPENOBLIVION_GAME_MODE_HPP
#define OPENOBLIVION_GAME_MODE_HPP

#include "application_context.hpp"
#include "bitflag.hpp"
#include "bullet/collision.hpp"
#include "cell_cache.hpp"
#include "character_controller/player_controller.hpp"
#include "exterior_manager.hpp"
#include "modes/mode.hpp"
#include "modes/menu_mode.hpp"
#include "ogrebullet/debug_drawer.hpp"
#include "record/formid.hpp"
#include "sdl/sdl.hpp"
#include <memory>

namespace oo {

class ConsoleMode;

/// Mode active while the player is exploring the game world.
/// \ingroup OpenOblivionModes
class GameMode {
 public:
  using transition_t = oo::ModeTransition<oo::ConsoleMode, oo::LoadingMenuMode>;
 private:
  ExteriorManager mExteriorMgr;
  std::shared_ptr<InteriorCell> mCell{};

  oo::CellIndex mCenterCell{};
  bool mInInterior{true};

  // TODO: These are only here because they need to be passed from the
  //       constructor to enter(), when they should be given to enter() in the
  //       first place.
  Ogre::Vector3 mPlayerStartPos{Ogre::Vector3::ZERO};
  Ogre::Quaternion mPlayerStartOrientation{Ogre::Quaternion::IDENTITY};

  std::unique_ptr<oo::PlayerController> mPlayerController{};

  bullet::CollisionCaller mCollisionCaller{};

  struct DebugDrawFlags : Bitflag<8u, DebugDrawFlags> {
    static constexpr enum_t None{0u};
    static constexpr enum_t Collision{1u << 0u};
    static constexpr enum_t Occlusion{1u << 1u};
  };

  DebugDrawFlags mDebugDrawFlags{DebugDrawFlags::make(DebugDrawFlags::None)};
  std::unique_ptr<Ogre::DebugDrawer> mDebugDrawer{};

  /// Run all registered collision callbacks with the collisions for this frame.
  void dispatchCollisions();

  /// Return a reference to the object under the crosshair.
  /// This works by raytesting against all collision objects in the current
  /// cell within `iActivatePickLength` units. If no object is found, return the
  /// null reference `0`.
  /// TODO: Move this to scripting
  oo::RefId getCrosshairRef() const;

  void addPlayerToScene(ApplicationContext &ctx);
  void registerSceneListeners(ApplicationContext &ctx);
  void unregisterSceneListeners(ApplicationContext &ctx);

  gsl::not_null<Ogre::SceneManager *> getSceneManager() const;
  gsl::not_null<btDiscreteDynamicsWorld *> getPhysicsWorld() const;

  /// Use the debug drawer to draw a line from the given `node` to each of its
  /// children, then from each each child to their children, and so on.
  void drawNodeChildren(gsl::not_null<Ogre::Node *> node,
                        const Ogre::Affine3 &t = Ogre::Affine3::IDENTITY)
  /*C++20: [[expects : mDebugDrawer != nullptr]]*/;

  /// Use the debug drawer to draw the skeleton of the given `entity`.
  void drawSkeleton(gsl::not_null<oo::Entity *> entity)
  /*C++20: [[expects : mDebugDrawer != nullptr]]*/;

  /// Use the debug drawer to draw the bounding box of the given `entity`.
  void drawBoundingBox(gsl::not_null<oo::Entity *> entity)
  /*C++20: [[expects : mDebugDrawer != nullptr]]*/;

  /// Draw all enabled debug information, if any.
  /// Does nothing if the debug drawer is inactive.
  void drawDebug();

  void setDebugDrawerEnabled(bool enable);
  void setDrawCollisionGeometryEnabled(bool enabled);
  void setDrawOcclusionGeometryEnabled(bool enabled);
  bool getDrawCollisionGeometryEnabled() const noexcept;
  bool getDrawOcclusionGeometryEnabled() const noexcept;

  /// Print information about the reference under the cursor, if it has changed.
  void logRefUnderCursor(ApplicationContext &ctx) const;

  /// Update the enabled animation states of all entities in the scene.
  void updateAnimation(float delta);

  /// Update the centred cell if it has changed, loading new cells and unloading
  /// old ones as appropriate.
  /// Specifically, if the player has moved to a different cell this frame then
  /// load all unloaded cells in the neighbourhood of the player and unload the
  /// loaded cells outside of the neighbourhood.
  bool updateCenterCell(ApplicationContext &ctx);

  /// Advance time forward by `delta` milliseconds, updating the globals and
  /// atmosphere appropriately.
  void advanceGameClock(float delta);

  /// Called when the player presses the activate button.
  transition_t handleActivate(ApplicationContext &ctx);

  transition_t handleActivate(ApplicationContext &ctx,
                              const record::raw::REFRDoor &door);

  tl::optional<oo::BaseId>
  getDoorDestinationCell(ApplicationContext &ctx,
                         const record::XTEL &teleport) const;

 public:
  /// \see Mode::Mode()
  explicit GameMode(ApplicationContext &/*ctx*/, oo::CellPacket cellPacket);

  /// \see Mode::enter()
  void enter(ApplicationContext &ctx);

  /// \see Mode::refocus()
  void refocus(ApplicationContext &);

  /// \see Mode::handleEvent()
  transition_t handleEvent(ApplicationContext &ctx, const sdl::Event &event);

  /// \see Mode::update()
  void update(ApplicationContext &ctx, float delta);

  /// Toggle a wireframe display of all collision objects in the scene.
  void toggleCollisionGeometry();

  /// Toggle a wireframe display of the bounding boxes of all objects in the
  /// scene.
  void toggleOcclusionGeometry();
};

} // namespace oo

#endif // OPENOBLIVION_GAME_MODE_HPP
