#ifndef OPENOBLIVION_NIF_COLLISION_OBJECT_LOADER_HPP
#define OPENOBLIVION_NIF_COLLISION_OBJECT_LOADER_HPP

#include <Ogre.h>
#include <spdlog/spdlog.h>

namespace nifloader {

class CollisionObjectLoader : public Ogre::ManualResourceLoader {
 private:
  friend class CollisionObjectLoaderState;

  std::shared_ptr<spdlog::logger> logger{};

 public:
  void loadResource(Ogre::Resource *resource) override;
};

} // namespace nifloader

#endif // OPENOBLIVION_NIF_COLLISION_OBJECT_LOADER_HPP
