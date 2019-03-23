#ifndef OPENOBLIVION_NIF_COLLISION_OBJECT_LOADER_HPP
#define OPENOBLIVION_NIF_COLLISION_OBJECT_LOADER_HPP

#include <Ogre.h>
#include <spdlog/spdlog.h>

namespace oo {

/// Loader for `Ogre::CollisionShape`s defined in NIF files.
/// \ingroup OpenOblivionNifloader
class CollisionObjectLoader : public Ogre::ManualResourceLoader {
 private:
  friend class CollisionObjectLoaderState;

  std::shared_ptr<spdlog::logger> logger{};

 public:
  void loadResource(Ogre::Resource *resource) override;
};

} // namespace oo

#endif // OPENOBLIVION_NIF_COLLISION_OBJECT_LOADER_HPP
