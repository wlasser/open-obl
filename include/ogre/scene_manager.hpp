#ifndef OPENOBL_OGRE_SCENE_MANAGER_HPP
#define OPENOBL_OGRE_SCENE_MANAGER_HPP

#include "ogre/deferred_light_pass.hpp"
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <gsl/gsl>

namespace oo {

//===----------------------------------------------------------------------===//
// Deferred Lighting Base Scene Manager
//===----------------------------------------------------------------------===//
class DeferredSceneManager : public Ogre::SceneManager {
 public:
  explicit DeferredSceneManager(const Ogre::String &name);
  ~DeferredSceneManager() override = default;

  const Ogre::String &getTypeName() const override;

  oo::DeferredLight *createLight(const Ogre::String &name) override;
  oo::DeferredLight *getLight(const Ogre::String &name) const override;
  bool hasLight(const Ogre::String &name) const override;
  void destroyLight(const Ogre::String &name) override;
  void destroyAllLights() override;

  void findLightsAffectingFrustum(const Ogre::Camera *camera) override;

  DeferredFogListener *getFogListener() noexcept;

  DeferredFogListener mFogListener;
};

class DeferredSceneManagerFactory : public Ogre::SceneManagerFactory {
 public:
  friend DeferredSceneManager;

  DeferredSceneManagerFactory() = default;
  ~DeferredSceneManagerFactory() override = default;

  gsl::owner<Ogre::SceneManager *>
  createInstance(const Ogre::String &instanceName) override;
  void destroyInstance(gsl::owner<Ogre::SceneManager *> instance) override;

 protected:
  void initMetaData() const override;

 private:
  constexpr static const char *FACTORY_TYPE_NAME{"oo::DeferredSceneManager"};
};

} // namespace oo

#endif // OPENOBL_OGRE_SCENE_MANAGER_HPP
