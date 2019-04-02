#include "mesh.hpp"
#include "mesh_manager.hpp"
#include <OgreResourceManager.h>
#include <OgreHardwareBufferManager.h>
#include <mesh.hpp>

namespace oo {

Mesh::Mesh(Ogre::ResourceManager *creator, const std::string &name,
           Ogre::ResourceHandle handle, const std::string &group,
           bool isManual, Ogre::ManualResourceLoader *loader)
    : Ogre::Resource(creator, name, handle, group, isManual, loader) {}

Mesh::~Mesh() {
  unload();
}

void Mesh::loadImpl() {
  OGRE_EXCEPT(Ogre::Exception::ERR_NOT_IMPLEMENTED,
              "There is no default loader, please supply a "
              "ManualResourceLoader",
              "Mesh::load()");
}

void Mesh::unloadImpl() {
  mSubMeshList.clear();
  mSubMeshNameMap.clear();
}

oo::SubMesh *Mesh::createSubMesh() {
  auto &subMesh{mSubMeshList.emplace_back(std::make_unique<oo::SubMesh>())};
  subMesh->parent = this;
  if (isLoaded()) _dirtyState();
  return subMesh.get();
}

oo::SubMesh *Mesh::createSubMesh(const std::string &name) {
  if (mSubMeshNameMap.contains(name)) {
    OGRE_EXCEPT(Ogre::Exception::ERR_DUPLICATE_ITEM,
                "A SubMesh with the name " + name + " already exists.",
                "Mesh::createSubMesh");
  }
  auto *subMesh{createSubMesh()};
  mSubMeshNameMap.emplace(name, mSubMeshList.size() - 1u);
  return subMesh;
}

oo::SubMesh *Mesh::getSubMesh(const std::string &name) const {
  return mSubMeshList[_getSubMeshIndex(name)].get();
}

void Mesh::destroySubMesh(uint16_t index) {
  if (index >= mSubMeshList.size()) {
    OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS,
                "Index out of bounds.",
                "Mesh::destroySubMesh");
  }
  mSubMeshList.erase(std::next(mSubMeshList.begin(), index));
  // Delete the submesh with this name, if any. Also need to decrement the index
  // of any named submeshes that are stored after the deleted submesh.
  for (auto it{mSubMeshNameMap.begin()}; it != mSubMeshNameMap.end();) {
    if (it->second == index) {
      mSubMeshNameMap.erase(it->first);
    } else if (it->second > index) {
      --it->second;
      ++it;
    }
  }
}

void Mesh::destroySubMesh(const std::string &name) {
  const auto index{_getSubMeshIndex(name)};
  destroySubMesh(index);
}

uint16_t Mesh::_getSubMeshIndex(const std::string &name) const {
  auto it{mSubMeshNameMap.find(name)};
  if (it == mSubMeshNameMap.end()) {
    OGRE_EXCEPT(Ogre::Exception::ERR_ITEM_NOT_FOUND,
                "No SubMesh named " + name + " found.",
                "Mesh::_getSubMeshIndex");
  }
  return it->second;
}

const Mesh::SubMeshList &Mesh::getSubMeshes() const {
  return mSubMeshList;
}

const Mesh::SubMeshNameMap &Mesh::getSubMeshNameMap() const {
  return mSubMeshNameMap;
}

oo::MeshPtr Mesh::clone(const std::string &newName,
                        const std::string &newGroup) {
  auto &meshMgr{oo::MeshManager::getSingleton()};
  oo::MeshPtr mesh{meshMgr.createManual(newName, newGroup.empty() ? mGroup
                                                                  : newGroup)};
  if (!mesh) return mesh;

  mesh->mBufMgr = mBufMgr;
  mesh->mAABB = mAABB;
  mesh->mBoundRadius = mBoundRadius;

  // Clone the submeshes, copying their names (if any).
  // Ogre does not appear to do this, it gives them all blank names.
  for (auto it{mSubMeshList.begin()}; it != mSubMeshList.end(); ++it) {
    const std::string &name = [&]() -> const std::string & {
      for (const auto&[k, v] : mSubMeshNameMap) {
        if (v == std::distance(it, mSubMeshList.begin())) return k;
      }
      return Ogre::BLANKSTRING;
    }();
    (*it)->clone(name, mesh.get());
  }

  // Ogre calls load and touch (which calls load again) here, but since mesh is
  // manually loaded and has no loader all it does is call preLoadImpl and
  // postLoadImpl, neither of which we use (though Ogre does).

  return mesh;
}

const Ogre::AxisAlignedBox &Mesh::getBounds() const {
  return mAABB;
}

float Mesh::getBoundingSphereRadius() const {
  return mBoundRadius;
}

void Mesh::_setBounds(const Ogre::AxisAlignedBox &bounds, bool pad) {
  mAABB = bounds;
  mBoundRadius = Ogre::Math::boundingRadiusFromAABB(mAABB);

  if (pad && mAABB.isFinite()) {
    const auto fPad{oo::MeshManager::getSingleton().getBoundsPaddingFactor()};
    const auto min{mAABB.getMinimum()}, max{mAABB.getMaximum()};
    // C++20: mAABB.setExtents(std::lerp(min, max, -fPad), std::lerp(max, min, -fPad));
    //        The current version is probably clearer, but I like this
    //        interpretation.
    mAABB.setExtents(min - (max - min) * fPad, max + (max - min) * fPad);
    mBoundRadius += mBoundRadius * fPad;
  }
}

void Mesh::_setBoundingSphereRadius(float radius) {
  mBoundRadius = radius;
}

Ogre::HardwareBufferManagerBase *Mesh::getHardwareBufferManager() const {
  return mBufMgr ? mBufMgr : Ogre::HardwareBufferManager::getSingletonPtr();
}

} // namespace oo