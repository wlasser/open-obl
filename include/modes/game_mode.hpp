#ifndef OPENOBLIVION_GAME_MODE_HPP
#define OPENOBLIVION_GAME_MODE_HPP

#include "application_context.hpp"
#include "bullet/collision.hpp"
#include "cell_cache.hpp"
#include "character_controller/character_controller.hpp"
#include "exterior_manager.hpp"
#include "modes/mode.hpp"
#include "modes/menu_mode.hpp"
#include "record/formid.hpp"
#include "sdl/sdl.hpp"
#include <memory>

namespace oo {

class OctreeNode;
class DebugDrawImpl;

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

  std::unique_ptr<oo::CharacterController> mPlayerController{};

  bullet::CollisionCaller mCollisionCaller{};

  friend oo::DebugDrawImpl;
  std::unique_ptr<oo::DebugDrawImpl> mDebugDrawImpl;

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

  ~GameMode();
  GameMode(const GameMode &) = delete;
  GameMode &operator=(const GameMode &) = delete;
  GameMode(GameMode &&) noexcept;
  GameMode &operator=(GameMode &&) noexcept;

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

  /// Toggle a fps display window.
  void toggleFps();
};

} // namespace oo

#endif // OPENOBLIVION_GAME_MODE_HPP
