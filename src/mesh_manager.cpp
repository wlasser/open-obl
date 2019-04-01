#include "mesh_manager.hpp"

namespace Ogre {

template<> oo::MeshManager *Singleton<oo::MeshManager>::msSingleton = nullptr;

} // namespace Ogre

namespace oo {

MeshManager &MeshManager::getSingleton() {
  assert(msSingleton);
  return *msSingleton;
}

MeshManager *MeshManager::getSingletonPtr() {
  return msSingleton;
}

MeshManager::MeshManager() {
  // This is intended to completely replace Ogre::MeshManager.
  mLoadOrder = 350.0f;
  mResourceType = "Mesh";
  Ogre::ResourceGroupManager::getSingleton()
      ._registerResourceManager(mResourceType, this);
}

MeshManager::~MeshManager() {
  Ogre::ResourceGroupManager::getSingleton()
      ._unregisterResourceManager(mResourceType);
}

MeshPtr MeshManager::create(const std::string &name,
                            const std::string &group,
                            bool isManual,
                            Ogre::ManualResourceLoader *loader,
                            const Ogre::NameValuePairList *createParams) {
  return std::static_pointer_cast<Mesh>(
      createResource(name, group, isManual, loader, createParams));
}

MeshPtr MeshManager::createManual(const std::string &name,
                                  const std::string &group,
                                  Ogre::ManualResourceLoader *loader) {
  return create(name, group, true, loader);
}

MeshPtr MeshManager::getByName(const std::string &name,
                               const std::string &group) {
  return std::static_pointer_cast<Mesh>(getResourceByName(name, group));
}

float MeshManager::getBoundsPaddingFactor() const {
  return mBoundsPaddingFactor;
}

void MeshManager::setBoundsPaddingFactor(float paddingFactor) {
  mBoundsPaddingFactor = paddingFactor;
}

Ogre::Resource *MeshManager::createImpl(const std::string &name,
                                        Ogre::ResourceHandle handle,
                                        const std::string &group,
                                        bool isManual,
                                        Ogre::ManualResourceLoader *loader,
                                        const Ogre::NameValuePairList *) {
  return OGRE_NEW oo::Mesh(this, name, handle, group, isManual, loader);
}

} // namespace oo
