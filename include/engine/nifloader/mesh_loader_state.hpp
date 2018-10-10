#ifndef OPENOBLIVION_ENGINE_NIFLOADER_MESH_LOADER_STATE_HPP
#define OPENOBLIVION_ENGINE_NIFLOADER_MESH_LOADER_STATE_HPP

#include "engine/nifloader/loader_state.hpp"
#include <Ogre.h>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace engine::nifloader {

// Ogre::SubMeshes do not store bounding box information, only Ogre::Meshes do,
// but we need it to compute the overall bounding box.
struct BoundedSubmesh {
  Ogre::SubMesh *submesh{};
  Ogre::AxisAlignedBox bbox{};
};

// We need an Ogre::Material to apply a texture to, but in nif files the two are
// completely separate. We use this structure as a temporary owner for the
// textures before passing control to Ogre when a material is available.
struct TextureFamily {
  using TexturePtr = std::unique_ptr<Ogre::TextureUnitState>;
  TexturePtr base{};
  TexturePtr normal{};
  TexturePtr dark{};
  TexturePtr detail{};
  TexturePtr gloss{};
  TexturePtr glow{};
  // Bump textures are treated differently and we use normal maps anyway
  /* TexturePtr bump{}; */
  std::vector<TexturePtr> decals{};
};

// Compute the minimum bounding box of the vertices in the block, subject to the
// given Ogre-coordinate transformation
Ogre::AxisAlignedBox getBoundingBox(nif::NiGeometryData *block,
                                    Ogre::Matrix4 transformation);

// Returns true if the triangle has a counterclockwise winding order, and
// false otherwise
bool isWindingOrderCCW(Ogre::Vector3 v1, Ogre::Vector3 n1,
                       Ogre::Vector3 v2, Ogre::Vector3 n2,
                       Ogre::Vector3 v3, Ogre::Vector3 n3);

// Return the number of triangles with a counterclockwise winding order.
// The mesh should have normals.
long numCCWTriangles(nif::NiTriShapeData *block);

// Append '_n' to the filename, preserving the extension
std::filesystem::path toNormalMap(std::filesystem::path texFile);

class MeshLoaderState {
 private:
  friend class TBGVisitor;

  template<class T, class S>
  T *getBlock(nif::basic::Ref<S> ref);

  BoundedSubmesh parseNiTriBasedGeom(nif::NiTriBasedGeom *block,
                                     LoadStatus &tag,
                                     const Ogre::Matrix4 &transform);

  std::shared_ptr<Ogre::Material>
  parseNiMaterialProperty(nif::NiMaterialProperty *block,
                          LoadStatus &tag);

  // See parseTexDesc for why the pass is necessary.
  TextureFamily
  parseNiTexturingProperty(nif::NiTexturingProperty *block,
                           LoadStatus &tag,
                           Ogre::Pass *pass);

  // When setting the texture name of a texture unit, Ogre looks up and loads
  // the texture using the resource group of its parent. Thus contrary to what
  // addTextureUnitState seems to suggest, one should not create a
  // Ogre::TextureUnitState with a nullptr parent, and we have to supply the
  // parent pass here.
  std::unique_ptr<Ogre::TextureUnitState>
  parseTexDesc(nif::compound::TexDesc *tex,
               Ogre::Pass *parent,
               const std::optional<std::string> &textureOverride = {});

  void parseNiSourceTexture(nif::NiSourceTexture *block,
                            LoadStatus &tag,
                            Ogre::TextureUnitState *tex,
                            const std::optional<std::string> &textureOverride = {});

  // Reads vertex, normal, and texcoord data from NiGeometryData and prepares it
  // for rendering.
  std::unique_ptr<Ogre::VertexData>
  generateVertexData(nif::NiGeometryData *block, Ogre::Matrix4 transformation,
                     std::vector<nif::compound::Vector3> *bitangents,
                     std::vector<nif::compound::Vector3> *tangents);

  // Reads triangle data from NiTriShapeData and prepares it for rendering.
  std::unique_ptr<Ogre::IndexData>
  generateIndexData(nif::NiTriShapeData *block);

  // Reads triangle strip data from NiTriStripsData and prepares it for
  // rendering.
  std::unique_ptr<Ogre::IndexData>
  generateIndexData(nif::NiTriStripsData *block);

  TaggedBlockGraph blocks;
  Ogre::Mesh *mesh;
  Ogre::LogManager &logger{Ogre::LogManager::getSingleton()};

 public:
  explicit MeshLoaderState(Ogre::Mesh *mesh, BlockGraph blocks);
};

class TBGVisitor {
 public:
  using Graph = TaggedBlockGraph;
  using vertex_descriptor = Graph::vertex_descriptor;
  using edge_descriptor = Graph::edge_descriptor;

  void initialize_vertex(vertex_descriptor v, const Graph &g) {}
  void start_vertex(vertex_descriptor v, const Graph &g);
  void discover_vertex(vertex_descriptor v, const Graph &g);
  void examine_edge(edge_descriptor e, const Graph &g) {}
  void tree_edge(edge_descriptor e, const Graph &g) {}
  void back_edge(edge_descriptor e, const Graph &g) {}
  void forward_or_cross_edge(edge_descriptor e, const Graph &g) {}
  void finish_edge(edge_descriptor e, const Graph &g) {}
  void finish_vertex(vertex_descriptor v, const Graph &g);

  explicit TBGVisitor(MeshLoaderState &state) : state(state) {}

 private:
  Ogre::Matrix4 transform{Ogre::Matrix4::IDENTITY};
  MeshLoaderState &state;
};

template<class T, class S>
T *MeshLoaderState::getBlock(nif::basic::Ref<S> ref) {
  auto val = static_cast<int32_t>(ref);
  if (val < 0 || val >= blocks.vertex_set().size()) {
    throw std::out_of_range("Nonexistent reference");
  }
  return dynamic_cast<T *>(blocks[val].block.get());
}

} // namespace engine::nifloader

#endif // OPENOBLIVION_ENGINE_NIFLOADER_MESH_LOADER_STATE_HPP
