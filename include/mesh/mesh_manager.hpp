#ifndef OPENOBLIVION_MESH_MANAGER_HPP
#define OPENOBLIVION_MESH_MANAGER_HPP

#include "mesh/mesh.hpp"
#include <OgreResourceManager.h>
#include <OgreSingleton.h>

namespace oo {

/// \addtogroup OpenOblivionMesh
/// @{

class MeshManager : public Ogre::ResourceManager,
                    public Ogre::Singleton<oo::MeshManager> {
 public:
  MeshManager();
  ~MeshManager() override;

  MeshPtr create(const std::string &name,
                 const std::string &group,
                 bool isManual = false,
                 Ogre::ManualResourceLoader *loader = nullptr,
                 const Ogre::NameValuePairList *createParams = nullptr);

  MeshPtr createManual(const std::string &name,
                       const std::string &group,
                       Ogre::ManualResourceLoader *loader = nullptr);

  /// \remark Fewer parameters are supported than in OGRE.
  MeshPtr createPlane(const std::string &name,
                      const std::string &group,
                      const Ogre::Plane &plane,
                      float width,
                      float height,
                      int xSegments = 1,
                      int ySegments = 1,
                      bool normals = true,
                      uint16_t uvSets = 1u,
                      float uTile = 1.0f,
                      float vTile = 1.0f,
                      const Ogre::Vector3 &upVector = Ogre::Vector3::UNIT_Z);

  // OGRE_RESOURCE_GROUP_INIT only works in the Ogre namespace
  MeshPtr getByName(const std::string &name, const std::string &group);

  float getBoundsPaddingFactor() const;
  void setBoundsPaddingFactor(float paddingFactor);

  static MeshManager &getSingleton();
  static MeshManager *getSingletonPtr();

 protected:
  Ogre::Resource *createImpl(const std::string &name,
                             Ogre::ResourceHandle handle,
                             const std::string &group,
                             bool isManual,
                             Ogre::ManualResourceLoader *loader,
                             const Ogre::NameValuePairList *params) override;

 private:
  float mBoundsPaddingFactor{0.01f};
};

/// @}

} // namespace oo

#endif // OPENOBLIVION_MESH_MANAGER_HPP
