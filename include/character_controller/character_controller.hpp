#ifndef OPENOBL_CHARACTER_CONTROLLER_CHARACTER_CONTROLLER_HPP
#define OPENOBL_CHARACTER_CONTROLLER_CHARACTER_CONTROLLER_HPP

#include "character_controller/character_controller_impl.hpp"
#include "config/game_settings.hpp"
#include "math/conversions.hpp"
#include "ogrebullet/motion_state.hpp"
#include <btBulletDynamicsCommon.h>
#include <gsl/gsl>
#include <variant>

namespace oo {

class CharacterController {
 public:
  explicit CharacterController(gsl::not_null<Ogre::SceneManager *> scnMgr,
                               gsl::not_null<btDiscreteDynamicsWorld *> world);

  Ogre::Camera *getCamera() const noexcept;
  btRigidBody *getRigidBody() const noexcept;
  const Ogre::SceneNode *getRootNode() const noexcept;
  Ogre::SceneNode *getRootNode() noexcept;

  void handleEvent(const KeyVariant &event);
  void handleEvent(const MouseVariant &event);

  void update(float elapsed);

  void moveTo(const Ogre::Vector3 &position);
  void setOrientation(const Ogre::Quaternion &orientation);

  void handleCollision(const btCollisionObject *other,
                       const btManifoldPoint &contact);

  Ogre::Vector3 getPosition() const noexcept;

 private:
  CharacterControllerImpl mImpl;
  StateVariant mState{};
  MovementStateVariant mMovementStateVariant{};

  void enter(StateVariant &state);
  void enter(MovementStateVariant &state);
  void exit(StateVariant &state);
  void exit(MovementStateVariant &state);
  void changeState(StateVariant newState);
  void changeState(MovementStateVariant newState);
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_CHARACTER_CONTROLLER_HPP
