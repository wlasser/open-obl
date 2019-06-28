#ifndef OPENOBL_MESH_HPP
#define OPENOBL_MESH_HPP

#include "mesh/submesh.hpp"
#include <gsl/gsl>
#include <OgreAnimation.h>
#include <OgreAxisAlignedBox.h>
#include <OgreResource.h>
#include "windows_cleanup.hpp"

/// \defgroup OpenOBLMesh OGRE Mesh Replacement Library
/// Provides similar but API-incompatible alternatives to mesh-related
/// functionality in OGRE.
///
/// The classes in this library offer alternative implementations of
/// `Ogre::Entity`, `Ogre::Mesh`, and so on, but with a more limited feature-set
/// and occasional API-breaking changes compared to OGRE. While OGRE is very
/// powerful, it is not possible to implement some required features under its
/// design, such as two entities based on the same mesh having different
/// skeletons, not just different skeleton instances.

namespace oo {

/// \addtogroup OpenOBLMesh
/// @{

using MeshPtr = std::shared_ptr<oo::Mesh>;

class Mesh : public Ogre::Resource {
 public:
  using SubMeshList = std::vector<std::unique_ptr<oo::SubMesh>>;
  using SubMeshNameMap = std::unordered_map<std::string, uint16_t>;

  Mesh(Ogre::ResourceManager *creator, const std::string &name,
       Ogre::ResourceHandle handle, const std::string &group,
       bool isManual = false, Ogre::ManualResourceLoader *loader = nullptr);
  ~Mesh() override;

  oo::SubMesh *createSubMesh();
  oo::SubMesh *createSubMesh(const std::string &name);

  uint16_t _getSubMeshIndex(const std::string &name) const;

  oo::SubMesh *getSubMesh(const std::string &name) const;

  void destroySubMesh(uint16_t index);
  void destroySubMesh(const std::string &name);

  const SubMeshList &getSubMeshes() const;
  const SubMeshNameMap &getSubMeshNameMap() const;

  oo::MeshPtr clone(const std::string &newName,
                    const std::string &newGroup = Ogre::BLANKSTRING);

  const Ogre::AxisAlignedBox &getBounds() const;
  float getBoundingSphereRadius() const;

  void _setBounds(const Ogre::AxisAlignedBox &bounds, bool pad = true);
  void _setBoundingSphereRadius(float radius);

  Ogre::HardwareBufferManagerBase *getHardwareBufferManager() const;

 private:
  SubMeshList mSubMeshList{};
  SubMeshNameMap mSubMeshNameMap{};

  /// Local bounding box.
  Ogre::AxisAlignedBox mAABB{};
  /// Local bounding sphere.
  float mBoundRadius{};
  // mBoneBoundingRadius TODO: Move into Entity and compute for each skeleton

  Ogre::HardwareBufferManagerBase *mBufMgr{};

 protected:
  void loadImpl() override;
  void unloadImpl() override;
};

/// @}

} // namespace oo

#endif // OPENOBL_MESH_HPP
