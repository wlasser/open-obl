#ifndef OPENOBLIVION_GAME_MODE_HPP
#define OPENOBLIVION_GAME_MODE_HPP

#include "application_context.hpp"
#include "bullet/collision.hpp"
#include "character_controller/player_controller.hpp"
#include "modes/mode.hpp"
#include "ogrebullet/debug_drawer.hpp"
#include "record/formid.hpp"
#include "resolvers/cell_resolver.hpp"
#include "sdl/sdl.hpp"
#include <memory>
#include <optional>
#include <tuple>
#include <variant>

namespace oo {

class ConsoleMode;
class MenuMode;

/// \name Custom deleter for the PlayerController.
/// Unlike the rest of the collision objects in the Cell, which are owned by
/// the Ogre::SceneManager and hence outlive the physics world, the
/// PlayerController must live for strictly less time than the physics world.
/// This is because the PlayerController requires the Ogre::SceneManager
/// during its construction, and it lives externally to the Cell. Consequently
/// the PlayerController must remove itself from the physics world during its
/// destruction so that the physics world doesn't attempt to dereference a
/// pointer to it during the broadphase cleanup.
///@{
using PlayerControllerPtr = std::unique_ptr<
    oo::PlayerController, std::function<void(oo::PlayerController *)>>;

void
releasePlayerController(Cell *cell, oo::PlayerController *playerController);

template<class ... Args>
PlayerControllerPtr makePlayerController(const std::shared_ptr<Cell> &cell,
                                         Args &&...args) {
  return PlayerControllerPtr(
      new oo::PlayerController(std::forward<Args>(args)...),
      [cell](oo::PlayerController *pc) -> void {
        releasePlayerController(cell.get(), pc);
      });
}
///@}

class GameMode {
 private:
  std::shared_ptr<Cell> mCell{};

  PlayerControllerPtr mPlayerController{};

  bullet::CollisionCaller mCollisionCaller{};

  std::unique_ptr<Ogre::DebugDrawer> mDebugDrawer{};

  /// Run all registered collision callbacks with the collisions for this frame.
  void dispatchCollisions();

  /// Return a reference to the object under the crosshair.
  /// This works by raytesting against all collision objects in the current
  /// cell within `iActivatePickLength` units. If no object is found, return the
  /// null reference `0`.
  /// TODO: Move this to scripting
  RefId getCrosshairRef();

  void loadCell(ApplicationContext &ctx, BaseId cellId);

 public:
  using transition_t = ModeTransition<ConsoleMode, MenuMode>;

  explicit GameMode(ApplicationContext &/*ctx*/) {}

  void enter(ApplicationContext &ctx) {
    loadCell(ctx, BaseId{0x00'048706});
    refocus(ctx);
  }

  void refocus(ApplicationContext &) {
    sdl::setRelativeMouseMode(true);
  }

  /// Poll for SDL events and process all that have occurred.
  /// This is called at the start of Application::frameStarted.
  transition_t handleEvent(ApplicationContext &ctx, const sdl::Event &event);

  /// Step the game world forward `delta` seconds.
  /// This is called during Application::frameStarted after pollEvents.
  void update(ApplicationContext &ctx, float delta);

  /// Toggle a wireframe display of all collision objects in the scene.
  void toggleCollisionGeometry();
};

} // namespace oo

#endif // OPENOBLIVION_GAME_MODE_HPP
