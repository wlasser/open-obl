#ifndef OPENOBLIVION_MESH_HPP
#define OPENOBLIVION_MESH_HPP

#include "submesh.hpp"
#include <absl/container/flat_hash_map.h>
#include <gsl/gsl>
#include <OgreAnimation.h>
#include <OgreAxisAlignedBox.h>
#include <OgreResource.h>

namespace oo {

using MeshPtr = std::shared_ptr<oo::Mesh>;

class Mesh : public Ogre::Resource {
 public:
  using SubMeshList = std::vector<std::unique_ptr<oo::SubMesh>>;
  using SubMeshNameMap = absl::flat_hash_map<std::string, uint16_t>;

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

} // namespace oo

#endif // OPENOBLIVION_MESH_HPP
