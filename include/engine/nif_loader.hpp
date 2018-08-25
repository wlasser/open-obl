#ifndef OPENOBLIVION_ENGINE_NIF_LOADER_HPP
#define OPENOBLIVION_ENGINE_NIF_LOADER_HPP

#include <OgreResource.h>

namespace engine {

class NifLoader : public Ogre::ManualResourceLoader {
 public:
  void loadResource(Ogre::Resource *resource) override;
  //void prepareResource(Ogre::Resource *resource) override;
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_NIF_LOADER_HPP
