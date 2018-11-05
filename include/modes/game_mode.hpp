#ifndef OPENOBLIVION_GAME_MODE_HPP
#define OPENOBLIVION_GAME_MODE_HPP

#include "application_context.hpp"
#include "bullet/collision.hpp"
#include "character_controller/player_controller.hpp"
#include "formid.hpp"
#include "resolvers/interior_cell_resolver.hpp"
#include "sdl/sdl.hpp"
#include <memory>

class GameMode {
 private:
  std::shared_ptr<InteriorCell> mCell{};
  std::unique_ptr<character::PlayerController> mPlayerController{};
  bullet::CollisionCaller mCollisionCaller{};

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
  explicit GameMode(ApplicationContext &ctx) {
    loadCell(ctx, BaseId{0x00'048706});
  }

  /// Poll for SDL events and process all that have occurred.
  /// This is called at the start of Application::frameStarted.
  void handleEvent(ApplicationContext &ctx, const sdl::Event &event);

  /// Step the game world forward `delta` seconds.
  /// This is called during Application::frameStarted after pollEvents.
  void update(ApplicationContext &ctx, float delta);
};

#endif // OPENOBLIVION_GAME_MODE_HPP
