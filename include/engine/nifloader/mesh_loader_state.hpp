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
Ogre::AxisAlignedBox getBoundingBox(const nif::NiGeometryData &block,
                                    Ogre::Matrix4 transformation);

// Returns true if the triangle has a counterclockwise winding order, and
// false otherwise
bool isWindingOrderCCW(Ogre::Vector3 v1, Ogre::Vector3 n1,
                       Ogre::Vector3 v2, Ogre::Vector3 n2,
                       Ogre::Vector3 v3, Ogre::Vector3 n3);

// Return the number of triangles with a counterclockwise winding order.
// The mesh should have normals.
long numCCWTriangles(const nif::NiTriShapeData &block);

// Append '_n' to the filename, preserving the extension
std::filesystem::path toNormalMap(std::filesystem::path texFile);

// Reads vertex, normal, and texcoord data from NiGeometryData and prepares it
// for rendering.
std::unique_ptr<Ogre::VertexData>
generateVertexData(const nif::NiGeometryData &block,
                   Ogre::Matrix4 transformation,
                   std::vector<nif::compound::Vector3> *bitangents,
                   std::vector<nif::compound::Vector3> *tangents);

// Reads triangle data from NiTriShapeData and prepares it for rendering.
std::unique_ptr<Ogre::IndexData>
generateIndexData(const nif::NiTriShapeData &block);

// Reads triangle strip data from NiTriStripsData and prepares it for
// rendering.
std::unique_ptr<Ogre::IndexData>
generateIndexData(const nif::NiTriStripsData &block);

// Set the properties of tex provided by the block. In particular, set the
// texture name of tex to the source texture in block, or textureOverride if it
// is provided. Also set the mipmap format.
void setSourceTexture(const nif::NiSourceTexture &block,
                      Ogre::TextureUnitState *tex,
                      const std::optional<std::string> &textureOverride = {});

void setClampMode(nif::Enum::TexClampMode mode, Ogre::TextureUnitState *tex);
void setFilterMode(nif::Enum::TexFilterMode mode, Ogre::TextureUnitState *tex);
void setTransform(const nif::compound::TexDesc::NiTextureTransform &transform,
                  Ogre::TextureUnitState *tex);

void setMaterialProperties(const nif::NiMaterialProperty &block,
                           Ogre::Pass *pass);

void addGenericVertexShader(Ogre::Pass *pass);
void addGenericFragmentShader(Ogre::Pass *pass);

struct TangentData {
  std::vector<nif::compound::Vector3> bitangents{};
  std::vector<nif::compound::Vector3> tangents{};
};

TangentData getTangentData(const nif::NiBinaryExtraData &extraData);

class MeshLoaderState {
 private:
  friend class TBGVisitor;

  template<class T, class S>
  T &getBlock(nif::basic::Ref<S> ref);

  // TODO: This is awful, stop doing dynamic_cast checks!
  template<class T, class S,
      class = std::enable_if_t<std::is_convertible_v<T *, S *>>>
  bool checkRefType(nif::basic::Ref<S> ref);

  template<class T>
  struct RefResult {
    T &block;
    LoadStatus &tag;
  };

  template<class T, class S>
  RefResult<T> getTaggedBlock(nif::basic::Ref<S> ref);

  // Returns an iterator into TaggedBlockGraph vertex_set.
  // TODO: Get rid of this, it's hideous.
  template<class T>
  auto getBlockIndex(const T &block);

  // NiTriBasedGeom blocks determine discrete pieces of geometry with a single
  // material and texture, and so translate to Ogre::SubMesh objects.
  BoundedSubmesh parseNiTriBasedGeom(const nif::NiTriBasedGeom &block,
                                     const Ogre::Matrix4 &transform);

  std::shared_ptr<Ogre::Material>
  parseNiMaterialProperty(const nif::NiMaterialProperty &block,
                          LoadStatus &tag);

  // When setting the texture name of a texture unit, Ogre looks up and loads
  // the texture using the resource group of its parent. Thus contrary to what
  // addTextureUnitState seems to suggest, one should not create a
  // Ogre::TextureUnitState with a nullptr parent, and we have to supply the
  // parent pass here.
  std::unique_ptr<Ogre::TextureUnitState>
  parseTexDesc(const nif::compound::TexDesc *tex,
               Ogre::Pass *parent,
               const std::optional<std::string> &textureOverride = {});

  // See parseTexDesc for why the pass is necessary.
  TextureFamily parseNiTexturingProperty(const nif::NiTexturingProperty &block,
                                         LoadStatus &tag,
                                         Ogre::Pass *pass);

  TangentData parseTangentData(const nif::NiExtraDataArray &extraDataArray);

  bool attachTextureProperty(const nif::NiPropertyArray &properties,
                             Ogre::Pass *pass);

  bool attachMaterialProperty(const nif::NiPropertyArray &properties,
                              Ogre::SubMesh *submesh);

  TaggedBlockGraph mBlocks;
  Ogre::Mesh *mMesh;

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
T &MeshLoaderState::getBlock(nif::basic::Ref<S> ref) {
  const auto val{static_cast<int32_t>(ref)};
  if (val < 0 || val >= mBlocks.vertex_set().size()) {
    throw std::out_of_range("Nonexistent reference");
  }
  return dynamic_cast<T &>(*mBlocks[val].block);
}

template<class T, class S, class>
bool MeshLoaderState::checkRefType(nif::basic::Ref<S> ref) {
  const auto val{static_cast<int32_t>(ref)};
  if (val < 0 || val >= mBlocks.vertex_set().size()) return false;
  // This is horrific.
  return dynamic_cast<T *>(&*mBlocks[val].block) != nullptr;
}

template<class T, class S>
auto MeshLoaderState::getTaggedBlock(nif::basic::Ref<S> ref) -> RefResult<T> {
  const auto val{static_cast<int32_t>(ref)};
  if (val < 0 || val >= mBlocks.vertex_set().size()) {
    throw std::out_of_range("Nonexistent reference");
  }
  return {dynamic_cast<T &>(*mBlocks[val].block), mBlocks[val].tag};
}

template<class T>
auto MeshLoaderState::getBlockIndex(const T &block) {
  // TODO: This is way more work than we need to do here
  auto comp = [this, &block](auto i) {
    return dynamic_cast<const T *>(&*mBlocks[i].block) == &block;
  };
  return std::find_if(mBlocks.vertex_set().begin(), mBlocks.vertex_set().end(),
                      comp);
}

} // namespace engine::nifloader

#endif // OPENOBLIVION_ENGINE_NIFLOADER_MESH_LOADER_STATE_HPP
