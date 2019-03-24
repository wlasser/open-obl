#ifndef OPENOBLIVION_BULLET_CONFIGURATION_HPP
#define OPENOBLIVION_BULLET_CONFIGURATION_HPP

#include "bullet/collision.hpp"
#include <btBulletDynamicsCommon.h>
#include <memory>

namespace bullet {

class Configuration {
 public:
  std::unique_ptr<btDefaultCollisionConfiguration> collisionConfiguration{
      std::make_unique<btDefaultCollisionConfiguration>()
  };

  std::unique_ptr<btCollisionDispatcher> dispatcher{
      std::make_unique<btCollisionDispatcher>(collisionConfiguration.get())
  };

  std::unique_ptr<btBroadphaseInterface> broadphase{
      std::make_unique<btDbvtBroadphase>()
  };

  std::unique_ptr<btSequentialImpulseConstraintSolver> solver{
      std::make_unique<btSequentialImpulseConstraintSolver>()
  };

 public:
  std::unique_ptr<btDiscreteDynamicsWorld> makeDynamicsWorld() const {
    static LayeredOverlapFilter filter{};
    auto world = std::make_unique<btDiscreteDynamicsWorld>(
        dispatcher.get(),
        broadphase.get(),
        solver.get(),
        collisionConfiguration.get());
    world->getPairCache()->setOverlapFilterCallback(&filter);
    world->setGravity({0, -9.81f, 0});
    return world;
  }
};

} // namespace bullet

#endif // OPENOBLIVION_BULLET_CONFIGURATION_HPP
