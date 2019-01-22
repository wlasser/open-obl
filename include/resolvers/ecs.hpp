#ifndef OPENOBLIVION_RESOLVERS_ECS_HPP
#define OPENOBLIVION_RESOLVERS_ECS_HPP

#include "ogrebullet/rigid_body.hpp"
#include <OgreEntity.h>
#include <OgreLight.h>
#include <OgreSkeleton.h>
#include <tuple>
#include <type_traits>

// The types returned by Resolver<T>::make are composed of pointers to Ogre
// objects, so lend themselves to an entity component system. Passing tuples
// around is good enough here though.
namespace ecs {

template<class T>
struct Component {
  T value{};
  using type = T;
  operator T &() {
    return value;
  }
  operator T &() const {
    return value;
  }
};

template<class ...Ts>
using Entity = std::tuple<Ts...>;

template<class T, class ...Ts>
constexpr bool contains() {
  return (std::is_same_v<T, Ts> || ...);
}

using RigidBody = Component<Ogre::RigidBody *>;
using Mesh = Component<Ogre::Entity *>;
using Light = Component<Ogre::Light *>;
using Skeleton = Component<Ogre::Skeleton *>;

} // namespace ecs

#endif // OPENOBLIVION_RESOLVERS_ECS_HPP
