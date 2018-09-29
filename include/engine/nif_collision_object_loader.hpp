#ifndef OPENOBLIVION_ENGINE_NIF_COLLISION_OBJECT_LOADER_HPP
#define OPENOBLIVION_ENGINE_NIF_COLLISION_OBJECT_LOADER_HPP

#include <Ogre.h>
#include <spdlog/spdlog.h>

namespace engine {

class NifCollisionObjectLoader : public Ogre::ManualResourceLoader {
 private:
  friend class NifCollisionObjectLoaderState;

  std::shared_ptr<spdlog::logger> logger{};

 public:
  void loadResource(Ogre::Resource *resource) override;
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_NIF_COLLISION_OBJECT_LOADER_HPP
