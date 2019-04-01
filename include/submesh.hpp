#ifndef OPENOBLIVION_SUBMESH_HPP
#define OPENOBLIVION_SUBMESH_HPP

#include <OgreIteratorWrapper.h>
#include <OgrePrerequisites.h>
#include <OgreRenderOperation.h>
#include <OgreResourceGroupManager.h>
#include <memory>

namespace oo {

class Mesh;

class SubMesh {
 private:
  using VertexDataDeleter = std::function<void(Ogre::VertexData *)>;
  using IndexDataDeleter = std::function<void(Ogre::IndexData *)>;

 public:
  Ogre::RenderOperation::OperationType
      operationType{Ogre::RenderOperation::OperationType::OT_TRIANGLE_LIST};

  /// Vertex data is owned by the `oo::SubMesh`, data sharing is not allowed.
  std::unique_ptr<Ogre::VertexData, VertexDataDeleter> vertexData{};

  /// Face index data.
  std::unique_ptr<Ogre::IndexData, IndexDataDeleter> indexData{};

  /// Names of bones, used to translate bone indices to blend indices.
  std::vector<std::string> boneNames{};

  /// Non-owning pointer to parent `oo::Mesh`.
  oo::Mesh *parent{};

  void setMaterialName(const std::string &matName,
                       const std::string &groupName);
  const std::string &getMaterialName() const;
  const std::string &getMaterialGroup() const;

  /// Return whether a material has been assigned to this submesh.
  bool isMatInitialised() const;

  /// Return an `Ogre::RenderOperation` structure required to render this mesh.
  /// \param rend Reference to an `Ogre::RenderOperation` structure to populate.
  void _getRenderOperation(Ogre::RenderOperation &rend);

  /// Make a copy of this submesh and give it a new name.
  /// \param newName The name to give the cloned submesh.
  /// \param parentMesh Parent of the cloned submesh. If null, this submesh's
  ///                   parent is used.
  oo::SubMesh *clone(const std::string &newName,
                     oo::Mesh *parentMesh = nullptr) const;

 private:
  bool mMatInitialized{};
  std::string mMaterialName{};
  std::string mGroupName{};
};

} // namespace oo

#endif // OPENOBLIVION_SUBMESH_HPP
