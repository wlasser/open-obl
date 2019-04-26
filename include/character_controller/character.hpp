#ifndef OPENOBLIVION_CHARACTER_CONTROLLER_CHARACTER_HPP
#define OPENOBLIVION_CHARACTER_CONTROLLER_CHARACTER_HPP

#include "character_controller/character_controller.hpp"
#include "record/reference_records.hpp"
#include "resolvers/resolvers.hpp"

namespace oo {

class Character {
 private:
  oo::CharacterController mController;
 public:
  using resolvers = ResolverTuple<record::NPC_, record::RACE>;

  explicit Character(const record::REFR_NPC_ &refRec,
                     gsl::not_null<Ogre::SceneManager *> scnMgr,
                     gsl::not_null<btDiscreteDynamicsWorld *> world,
                     Character::resolvers resolvers);

  oo::CharacterController &getController() noexcept;
  const oo::CharacterController &getController() const noexcept;
};

} // namespace oo

#endif // OPENOBLIVION_CHARACTER_CONTROLLER_CHARACTER_HPP
