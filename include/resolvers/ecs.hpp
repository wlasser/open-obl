#ifndef OPENOBLIVION_RESOLVERS_ECS_HPP
#define OPENOBLIVION_RESOLVERS_ECS_HPP

#include "ogrebullet/rigid_body.hpp"
#include <boost/mp11.hpp>
#include <OgreEntity.h>
#include <OgreLight.h>
#include <OgreSkeleton.h>
#include <tuple>
#include <type_traits>

// The types returned by Resolver<T>::make are composed of pointers to Ogre
// objects, so lend themselves to an entity component system. Passing tuples
// around is good enough here though.
namespace ecs {

template<class T, int I = 0>
struct Component {
  constexpr static inline int Index{I};
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

template<class ...Ts>
constexpr int maxIndexOf() {
  return std::max({Ts::Index...});
}

template<int I, class T>
using indexEquals = std::bool_constant<T::Index == I>;

template<class Entity, class ...Us>
constexpr auto subsetOfImpl(const Entity &entity, boost::mp11::mp_list<Us...>) {
  return ecs::Entity<Us...>{std::get<Us>(entity)...};
}

template<int I>
struct subsetOf {
  template<class T> using P = indexEquals<I, T>;

  template<class ...Ts>
  constexpr auto operator()(const Entity<Ts...> &entity) {
    using L = boost::mp11::mp_partition<boost::mp11::mp_list<Ts...>, P>;
    using Head = boost::mp11::mp_front<L>;
    return subsetOfImpl(entity, Head{});
  }
};

template<int I = 0> using RigidBody = Component<Ogre::RigidBody *, I>;
template<int I = 0> using Mesh = Component<Ogre::Entity *, I>;
template<int I = 0> using Light = Component<Ogre::Light *, I>;
template<int I = 0> using Skeleton = Component<Ogre::Skeleton *, I>;

} // namespace ecs

#endif // OPENOBLIVION_RESOLVERS_ECS_HPP
