#ifndef OPENOBLIVION_OGRE_SCENE_MANAGER_HPP
#define OPENOBLIVION_OGRE_SCENE_MANAGER_HPP

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

  // Instead of overriding the Light methods to construct the DeferredLights,
  // override the MovableObject methods and check for Lights. Otherwise, someone
  // could call createMovableObject on a Light and bypass the DeferredLight
  // construction, for example.
  Ogre::MovableObject *
  createMovableObject(const Ogre::String &name,
                      const Ogre::String &typeName,
                      const Ogre::NameValuePairList *params = nullptr) override;
  void destroyMovableObject(const Ogre::String &name,
                            const Ogre::String &typeName) override;
  void destroyAllMovableObjectsByType(const Ogre::String &typeName) override;
  void destroyAllMovableObjects() override;

  std::vector<oo::DeferredLight *> getLights() const;
  DeferredFogListener *getFogListener() noexcept;

 private:
  struct LightInfo {
    Ogre::Light *light{};
    std::unique_ptr<oo::DeferredLight> geometry{};
    LightInfo(Ogre::Light *pLight,
              std::unique_ptr<oo::DeferredLight> pGeometry);
  };
  std::vector<LightInfo> mLights{};

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

#endif // OPENOBLIVION_OGRE_SCENE_MANAGER_HPP
