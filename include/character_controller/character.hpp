#ifndef OPENOBL_CHARACTER_CONTROLLER_CHARACTER_HPP
#define OPENOBL_CHARACTER_CONTROLLER_CHARACTER_HPP

#include "character_controller/body.hpp"
#include "character_controller/character_mediator.hpp"
#include "character_controller/fallback_state.hpp"
#include "character_controller/jump_state.hpp"
#include "character_controller/character_mediator.hpp"
#include "character_controller/run_state.hpp"
#include "character_controller/sneak_jump_state.hpp"
#include "character_controller/sneak_stand_state.hpp"
#include "character_controller/stand_state.hpp"
#include "character_controller/walk_state.hpp"
#include "controls.hpp"
#include "record/actor_value.hpp"
#include "record/attribute.hpp"
#include "record/reference_records.hpp"
#include "resolvers/resolvers.hpp"
#include <variant>

namespace oo {

using StateVariant = std::variant<StandState,
                                  JumpState,
                                  SneakStandState,
                                  SneakJumpState>;
using MovementStateVariant = std::variant<WalkState, RunState>;

class Character {
 private:
  using StateOpt = std::optional<oo::StateVariant>;
  using MovementStateOpt = std::optional<oo::MovementStateVariant>;

  /// Used to provide states with access to the Character.
  oo::CharacterMediator mMediator;

  /// SceneManager the character belongs to, is used for proper destruction.
  Ogre::SceneManager *mScnMgr;
  /// PhysicsWorld the character belongs to.
  btDiscreteDynamicsWorld *mPhysicsWorld;
  /// General character state.
  oo::StateVariant mState{};
  /// Movement speed state.
  oo::MovementStateVariant mMovementState{};

  Ogre::SceneNode *mRoot{};
  static constexpr float CAPSULE_RADIUS{0.3f};
  std::unique_ptr<btCollisionShape> mCapsuleShape{};
  std::unique_ptr<btCollisionObject> mCapsule{};

  Ogre::SceneNode *mCameraNode{};
  Ogre::SceneNode *mPitchNode{};
  Ogre::Camera *mCamera{};

  std::array<oo::Entity *, 5u> mBodyParts{};

  // TODO: Don't hardcode this here, use the actual enum values.
  static constexpr uint32_t NUM_ACTOR_VALUES{72u};
  std::array<int, NUM_ACTOR_VALUES> mActorValues{};

  float mRaceHeight{1.0f};
  float mWornWeight{0.0f};

  float mHeight{mRaceHeight * 128 * oo::metersPerUnit<float>};
  float mMass{80.0f};

  // TODO: Aren't these implicit based on the current state?
  bool mHasWeaponOut{false};
  bool mIsRunning{false};
  // speedModifier(mHasWeaponOut, mIsRunning) gives runModifier,
  // swimWalkModifier, or swimRunModifier, multiplied by fMoveNoWeaponMult if
  // appropriate
  std::function<float(bool, bool)> mSpeedModifier{};

  Ogre::Radian mPitch{0.0f};
  Ogre::Radian mRootYaw{0.0f};
  Ogre::Radian mYaw{0.0f};
  Ogre::Vector3 mLocalVelocity{Ogre::Vector3::ZERO};
  Ogre::Vector3 mVelocity{Ogre::Vector3::ZERO};

  void setBodyPart(oo::BodyParts part, oo::Entity *entity) noexcept;

  friend CharacterMediator;

  void enter(oo::StateVariant &state);
  void enter(oo::MovementStateVariant &state);
  void exit(oo::StateVariant &state);
  void exit(oo::MovementStateVariant &state);
  void changeState(oo::StateVariant newState);
  void changeState(oo::MovementStateVariant newState);

  /// Twist the body towards the current camera direction.
  /// \todo Make the twist speed framerate-independent and consequently make
  ///       this take a `float elapsed` argument.
  void updateTwist() noexcept;
  /// Orient the camera, root, and pitch nodes according to the current
  /// pitch, root yaw, and yaw.
  void orientCamera() noexcept;
  /// Call `updateTwist` and `orientCamera`.
  void updateCamera() noexcept;
  /// Set the world transform of the capsule to match the world transform of
  /// the character.
  void updateCapsule() noexcept;

  constexpr static float MAX_RAYCAST_DISTANCE{1000.0f};
  using RaycastResult = bullet::ClosestNotMeRayResultCallback;
  RaycastResult raycast() const noexcept;

  Ogre::Vector4 getSurfaceNormal() const noexcept;
  Ogre::Matrix3 getSurfaceFrame() const noexcept;
  /// Return the distance from the root to the ground surface if a surface is
  /// below the character.
  std::optional<float> getSurfaceDist() const noexcept;

  Ogre::Matrix3 getDefaultFrame() const noexcept;

 public:
  using resolvers = ResolverTuple<record::NPC_, record::RACE>;

  explicit Character(const record::REFR_NPC_ &refRec,
                     gsl::not_null<Ogre::SceneManager *> scnMgr,
                     gsl::not_null<btDiscreteDynamicsWorld *> world,
                     Character::resolvers resolvers);
  ~Character();

  void update(float elapsed);
  void handleEvent(const oo::KeyVariant &event);
  void handleEvent(const oo::MouseVariant &event);

  float getMoveSpeed() const noexcept;

  void setPosition(const Ogre::Vector3 &position);
  Ogre::Vector3 getPosition() const;
  void setOrientation(const Ogre::Quaternion &orientation);

  Ogre::Camera *getCamera() noexcept;

  oo::Entity *getBodyPart(oo::BodyParts part) noexcept;
  Ogre::SkeletonInstance *getSkeleton() noexcept;

  int getActorValue(oo::ActorValue actorValue) const noexcept;
};

} // namespace oo

#endif // OPENOBL_CHARACTER_CONTROLLER_CHARACTER_HPP
