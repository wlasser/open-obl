#ifndef OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
#define OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP

#include "engine/controls.hpp"
#include "engine/conversions.hpp"
#include "ogrebullet/motion_state.hpp"
#include "game_settings.hpp"
#include <btBulletDynamicsCommon.h>
#include <Ogre.h>
#include <array>
#include <variant>

namespace engine {

using KeyVariant = std::variant<
    event::Forward, event::Backward, event::SlideLeft, event::SlideRight,
    event::Jump>;
using MouseVariant = std::variant<event::Pitch, event::Yaw>;

class PlayerController;

class PlayerState {
 public:
  virtual ~PlayerState() = 0;

  virtual std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player, const KeyVariant &event) {
    return nullptr;
  }

  virtual void
  handleEvent(PlayerController *player, const MouseVariant &event) {}

  // Fallback for unhandled KeyEvents
  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *, const event::KeyEvent &) {
    return nullptr;
  }

  // Fallback for unhandled MouseEvents
  void handleEvent(PlayerController *, const event::MouseEvent &) {}

  virtual std::shared_ptr<PlayerState>
  update(PlayerController *player, float elapsed) = 0;

  virtual std::shared_ptr<PlayerState>
  handleCollision(PlayerController *player,
                  const btCollisionObject *other,
                  const btManifoldPoint &contact) {
    return nullptr;
  }

  virtual void enter(PlayerController *player) {}
};
inline PlayerState::~PlayerState() = default;

class PlayerController {
 public:
  explicit PlayerController(Ogre::SceneManager *scnMgr);

  Ogre::Camera *getCamera();
  btRigidBody *getRigidBody();

  void handleEvent(const KeyVariant &event);
  void handleEvent(const MouseVariant &event);

  void update(float elapsed);

  void moveTo(const Ogre::Vector3 &position);

  void handleCollision(const btCollisionObject *other,
                       const btManifoldPoint &contact);

 private:
  friend PlayerState;
  friend class PlayerStandState;
  friend class PlayerJumpState;
  friend struct MoveAbility;
  friend struct LookAbility;
  std::shared_ptr<PlayerState> state{};

  GameSetting<float> fMoveCharWalkMin{"fMoveCharWalkMin", 90.0f};
  GameSetting<float> fMoveCharWalkMax{"fMoveCharWalkMax", 130.0f};
  GameSetting<float> fMoveRunMult{"fMoveRunMult", 3.0f};
  GameSetting<float> fMoveRunAthleticsMult{"fMoveRunAthleticsMult", 1.0f};
  GameSetting<float> fJumpHeightMin{"fJumpHeightMin", 64.0f};
  GameSetting<float> fJumpHeightMax{"fJumpHeightMax", 164.0f};
  float speedAttribute{50.0f};
  float athleticsSkill{50.0f};
  float acrobaticsSkill{50.0f};

  float moveTypeModifier(float athleticsSkill) const {
    return *fMoveRunMult + *fMoveRunAthleticsMult * athleticsSkill * 0.01f;
  }

  float baseSpeed(float speedAttribute) const {
    const float walkRange = *fMoveCharWalkMax - *fMoveCharWalkMax;
    return *fMoveCharWalkMin + walkRange * speedAttribute * 0.01f;
  }

  float speed(float speedAttribute, float athleticsSkill) const {
    return moveTypeModifier(athleticsSkill) * baseSpeed(speedAttribute)
        * conversions::metersPerUnit<float>;
  }

  float jumpHeight(float acrobaticsSkill) const {
    const float heightRange = *fJumpHeightMax - *fJumpHeightMin;
    return (*fJumpHeightMin + heightRange * acrobaticsSkill * 0.01f)
        * conversions::metersPerUnit<float>;
  }

  void updatePhysics(float elapsed);

  float height{1.0f * 128 * conversions::metersPerUnit<float>};
  float mass{80.0f};

  Ogre::Radian pitch{0.0f};
  Ogre::Radian yaw{0.0f};
  Ogre::Vector3 localVelocity{Ogre::Vector3::ZERO};

  Ogre::SceneNode *cameraNode{};
  Ogre::SceneNode *pitchNode{};
  Ogre::Camera *camera{};

  Ogre::SceneNode *bodyNode{};
  std::unique_ptr<Ogre::MotionState> motionState{};
  std::unique_ptr<btCollisionShape> collisionShape{};
  std::unique_ptr<btRigidBody> rigidBody{};
};

struct MoveAbility {
  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player, const event::Forward &event) {
    player->localVelocity.z -= (event.down ? 1.0f : -1.0f);
    return nullptr;
  }

  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player, const event::Backward &event) {
    player->localVelocity.z += (event.down ? 1.0f : -1.0f);
    return nullptr;
  }

  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player,
              const event::SlideLeft &event) {
    player->localVelocity.x -= (event.down ? 1.0f : -1.0f);
    return nullptr;
  }

  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player,
              const event::SlideRight &event) {
    player->localVelocity.x += (event.down ? 1.0f : -1.0f);
    return nullptr;
  }
};

struct LookAbility {
  void handleEvent(PlayerController *player, const event::Pitch &event) {
    player->pitch += -Ogre::Radian(event.delta);
  }

  void handleEvent(PlayerController *player, const event::Yaw &event) {
    player->yaw += -Ogre::Radian(event.delta);
  }
};

class PlayerStandState : public PlayerState,
                         public MoveAbility,
                         public LookAbility {
 public:
  using PlayerState::handleEvent;
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;

  ~PlayerStandState() override = default;

  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player, const KeyVariant &event) override {
    const auto handler = [player, this](auto &&e) {
      return handleEvent(player, e);
    };
    return std::visit(handler, event);
  }

  void
  handleEvent(PlayerController *player, const MouseVariant &event) override {
    const auto handler = [player, this](auto &&e) {
      handleEvent(player, e);
    };
    std::visit(handler, event);
  }

  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player, const event::Jump &event);

  std::shared_ptr<PlayerState>
  update(PlayerController *player, float elapsed) override;
};

class PlayerJumpState : public PlayerState,
                        public MoveAbility,
                        public LookAbility {
 public:
  using PlayerState::handleEvent;
  using MoveAbility::handleEvent;
  using LookAbility::handleEvent;

  ~PlayerJumpState() override = default;

  std::shared_ptr<PlayerState>
  handleEvent(PlayerController *player, const KeyVariant &event) override {
    const auto handler = [player, this](auto &&e) {
      return handleEvent(player, e);
    };
    return std::visit(handler, event);
  }

  void
  handleEvent(PlayerController *player, const MouseVariant &event) override {
    const auto handler = [player, this](auto &&e) {
      handleEvent(player, e);
    };
    std::visit(handler, event);
  }

  std::shared_ptr<PlayerState>
  update(PlayerController *player, float elapsed) override;

  std::shared_ptr<PlayerState>
  handleCollision(PlayerController *player,
                  const btCollisionObject *other,
                  const btManifoldPoint &contact) override;

  void enter(PlayerController *player) override;
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_PLAYER_CONTROLLER_HPP
