#ifndef OPENOBLIVION_GAME_MODE_HPP
#define OPENOBLIVION_GAME_MODE_HPP

#include "application_context.hpp"
#include "bullet/collision.hpp"
#include "cell_cache.hpp"
#include "character_controller/player_controller.hpp"
#include "exterior_manager.hpp"
#include "modes/mode.hpp"
#include "modes/menu_mode.hpp"
#include "ogrebullet/debug_drawer.hpp"
#include "record/formid.hpp"
#include "resolvers/cell_resolver.hpp"
#include "resolvers/wrld_resolver.hpp"
#include "sdl/sdl.hpp"
#include <memory>
#include <optional>
#include <set>
#include <tuple>
#include <variant>

namespace oo {

class ConsoleMode;

/// \name Custom deleter for the PlayerController.
/// Unlike the rest of the collision objects in the Cell, which are owned by
/// the Ogre::SceneManager and hence outlive the physics world, the
/// PlayerController must live for strictly less time than the physics world.
/// This is because the PlayerController requires the Ogre::SceneManager
/// during its construction, and it lives externally to the Cell. Consequently
/// the PlayerController must remove itself from the physics world during its
/// destruction so that the physics world doesn't attempt to dereference a
/// pointer to it during the broadphase cleanup.
/// \addtogroup OpenOblivionModes
///@{
using PlayerControllerDeleter = std::function<void(oo::PlayerController *)>;
using PlayerControllerPtr = std::unique_ptr<oo::PlayerController,
                                            oo::PlayerControllerDeleter>;

void releasePlayerController(btDiscreteDynamicsWorld *physicsWorld,
                             oo::PlayerController *playerController);

template<class ... Args> oo::PlayerControllerPtr
makePlayerController(btDiscreteDynamicsWorld *physicsWorld, Args &&...args) {
  return oo::PlayerControllerPtr(
      new oo::PlayerController(std::forward<Args>(args)...),
      [physicsWorld](oo::PlayerController *pc) -> void {
        oo::releasePlayerController(physicsWorld, pc);
      });
}
///@}

/// Mode active while the player is exploring the game world.
/// \ingroup OpenOblivionModes
class GameMode {
 private:
  ExteriorManager mExteriorMgr;
  std::shared_ptr<Cell> mCell{};

  World::CellIndex mCenterCell{};
  bool mInInterior{true};

  // TODO: These are only here because they need to be passed from the
  //       constructor to enter(), when they should be given to enter() in the
  //       first place.
  Ogre::Vector3 mPlayerStartPos{Ogre::Vector3::ZERO};
  Ogre::Quaternion mPlayerStartOrientation{Ogre::Quaternion::IDENTITY};

  oo::PlayerControllerPtr mPlayerController{};

  bullet::CollisionCaller mCollisionCaller{};

  std::unique_ptr<Ogre::DebugDrawer> mDebugDrawer{};

  /// Run all registered collision callbacks with the collisions for this frame.
  void dispatchCollisions();

  /// Return a reference to the object under the crosshair.
  /// This works by raytesting against all collision objects in the current
  /// cell within `iActivatePickLength` units. If no object is found, return the
  /// null reference `0`.
  /// TODO: Move this to scripting
  oo::RefId getCrosshairRef();

  void addPlayerToScene(ApplicationContext &ctx);
  void registerSceneListeners(ApplicationContext &ctx);

  gsl::not_null<Ogre::SceneManager *> getSceneManager() const;
  gsl::not_null<btDiscreteDynamicsWorld *> getPhysicsWorld() const;

  /// Use the debug drawer to draw a line from the given `node` to each of its
  /// children, then from each each child to their children, and so on.
  /// Does nothing if the debug drawer is inactive.
  void drawNodeChildren(Ogre::Node *node,
                        const Ogre::Affine3 &t = Ogre::Affine3::IDENTITY);

  /// Update the enabled animation states of all entities in the scene.
  void updateAnimation(float delta);

  /// Update the centred cell if it has changed, loading new cells and unloading
  /// old ones as appropriate.
  /// Specifically, if the player has moved to a different cell this frame then
  /// load all unloaded cells in the neighbourhood of the player and unload the
  /// loaded cells outside of the neighbourhood.
  bool updateCenterCell(ApplicationContext &ctx);


 public:
  using transition_t = oo::ModeTransition<oo::ConsoleMode, oo::LoadingMenuMode>;

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
};

} // namespace oo

#endif // OPENOBLIVION_GAME_MODE_HPP
