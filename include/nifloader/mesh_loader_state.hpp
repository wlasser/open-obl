#ifndef OPENOBLIVION_NIFLOADER_MESH_LOADER_STATE_HPP
#define OPENOBLIVION_NIFLOADER_MESH_LOADER_STATE_HPP

#include "mesh.hpp"
#include "nifloader/loader.hpp"
#include "settings.hpp"
#include "submesh.hpp"
#include <Ogre.h>
#include <spdlog/spdlog.h>
#include <filesystem>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

namespace oo {

/// \addtogroup OpenOblivionNifloader
/// @{

/// `oo::SubMesh`es do not store bounding box information, only `oo::Mesh`es
/// do, but we need it to compute the overall bounding box.
struct BoundedSubmesh {
  oo::SubMesh *submesh{};
  Ogre::AxisAlignedBox bbox{};
};

/// Acts as a temporary owner for textures before passing control to Ogre when
/// a material is available. This is necessary because we need an
/// `Ogre::Material` to apply a texture to, but in nif files the two are
/// completely separate.
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

/// Compute the minimum bounding box of the vertices in the block, subject to
/// the given Ogre coordinate-transformation.
Ogre::AxisAlignedBox
getBoundingBox(const nif::NiGeometryData &block, Ogre::Matrix4 transformation);

/// Returns true if the triangle has a counterclockwise winding order, and
/// false otherwise.
bool isWindingOrderCCW(Ogre::Vector3 v1, Ogre::Vector3 n1,
                       Ogre::Vector3 v2, Ogre::Vector3 n2,
                       Ogre::Vector3 v3, Ogre::Vector3 n3);

/// Return the number of triangles with a counterclockwise winding order.
/// The mesh should have normals.
long numCCWTriangles(const nif::NiTriShapeData &block);

/// Append '_n' to the filename, preserving the extension.
std::filesystem::path toNormalMap(std::filesystem::path texFile);

struct BoneBinding {
  std::array<unsigned short, 4> indices{};
  std::array<float, 4> weights{};
};

/// Get the bone indices and weights of each vertex governed by the
/// `nif::NiSkinPartition`, presumably those owned by some `nif::NiGeometry`
/// block. The indices are relative to the bone list of the
/// `nif::NiSkinInstance` that owns the `nif::NiSkinPartition`.
std::vector<BoneBinding> getBoneBindings(const nif::NiSkinPartition &skin);

struct BoneAssignments {
  std::vector<BoneBinding> bindings;
  std::vector<std::string> names;
};

/// Dispatch to `oo::getBoneBindings(const nif::NiSkinPartition &)` if `block`
/// has a `nif::NiSkinPartition`, and store the names of the used bones.
BoneAssignments getBoneAssignments(const oo::BlockGraph &g,
                                   const nif::NiTriBasedGeom &block);

/// Read vertex, normal, and texcoord data from `nif::NiGeometryData` and
/// prepare it for rendering.
std::unique_ptr<Ogre::VertexData>
generateVertexData(const nif::NiGeometryData &block,
                   Ogre::Matrix4 transformation,
                   std::vector<nif::compound::Vector3> *bitangents,
                   std::vector<nif::compound::Vector3> *tangents,
                   std::vector<BoneBinding> *boneBindings);

/// Read triangle data from `nif::NiTriShapeData` and prepare it for rendering.
std::unique_ptr<Ogre::IndexData>
generateIndexData(const nif::NiTriShapeData &block);

/// Read triangle strip data from `nif::NiTriStripsData` and prepare it for
/// rendering.
std::unique_ptr<Ogre::IndexData>
generateIndexData(const nif::NiTriStripsData &block);

/// Read triangle strip data from `nif::NiGeometryData` by dispatching to the
/// appropriate overload of `oo::generateIndexData()` for the most derived type
/// of `block`. Also notify `subMesh` of the index operation type required to
/// render the generated index data.
std::unique_ptr<Ogre::IndexData>
generateIndexData(const nif::NiGeometryData &block, oo::SubMesh *submesh);

/// Set the properties of tex provided by the block. In particular, set the
/// texture name of `tex` to the source texture in `block`, or `textureOverride`
/// if it is provided. Also set the mipmap format.
void setSourceTexture(const nif::NiSourceTexture &block,
                      Ogre::TextureUnitState *tex,
                      const std::optional<std::string> &textureOverride = {});

void setClampMode(nif::Enum::TexClampMode mode, Ogre::TextureUnitState *tex);
void setFilterMode(nif::Enum::TexFilterMode mode, Ogre::TextureUnitState *tex);
void setTransform(const nif::compound::TexDesc::NiTextureTransform &transform,
                  Ogre::TextureUnitState *tex);

void
setMaterialProperties(const nif::NiMaterialProperty &block, Ogre::Pass *pass);

void addGenericVertexShader(Ogre::Pass *pass);
void addGenericSkinnedVertexShader(Ogre::Pass *pass);
void addGenericFragmentShader(Ogre::Pass *pass);

struct TangentData {
  std::vector<nif::compound::Vector3> bitangents{};
  std::vector<nif::compound::Vector3> tangents{};
};

TangentData getTangentData(const nif::NiBinaryExtraData &extraData);
TangentData parseTangentData(const oo::BlockGraph &g,
                             const nif::NiExtraDataArray &dataArray);

/// \remark When setting the texture name of a texture unit, Ogre looks up and
///         loads the texture using the resource group of its parent. Thus
///         contrary to what addTextureUnitState seems to suggest, one should
///         not create a `Ogre::TextureUnitState` with a nullptr parent, and
///         we have to supply the parent pass here.
std::unique_ptr<Ogre::TextureUnitState>
parseTexDesc(const oo::BlockGraph &g,
             const nif::compound::TexDesc *tex,
             Ogre::Pass *parent,
             const std::optional<std::string> &textureOverride = {});

/// \remark See `parseTexDesc()` for why the pass is necessary.
TextureFamily parseNiTexturingProperty(const oo::BlockGraph &g,
                                       const nif::NiTexturingProperty &block,
                                       Ogre::Pass *pass);

bool attachTextureProperty(const oo::BlockGraph &g,
                           const nif::NiPropertyArray &properties,
                           Ogre::Pass *pass);

std::shared_ptr<Ogre::Material>
parseNiMaterialProperty(const oo::BlockGraph &g,
                        const std::string &meshName,
                        const std::string &meshGroup,
                        const nif::NiMaterialProperty &block);

bool attachMaterialProperty(const oo::BlockGraph &g,
                            const oo::Mesh *mesh,
                            const nif::NiPropertyArray &properties,
                            Ogre::SubMesh *submesh, bool hasSkinning = false);

/// \remark `nif::NiTriBasedGeom` blocks determine discrete pieces of geometry
///         with a single material and texture, and so translate to
///         `oo::SubMesh` objects.
BoundedSubmesh parseNiTriBasedGeom(const oo::BlockGraph &g,
                                   oo::Mesh *mesh,
                                   const nif::NiTriBasedGeom &block,
                                   const Ogre::Matrix4 &transform);

class MeshLoaderState {
 public:
  using Graph = BlockGraph;
  using vertex_descriptor = Graph::vertex_descriptor;
  using edge_descriptor = Graph::edge_descriptor;

  void start_vertex(vertex_descriptor v, const Graph &g);
  void discover_vertex(vertex_descriptor v, const Graph &g);
  void finish_vertex(vertex_descriptor v, const Graph &g);

  [[maybe_unused]] void initialize_vertex(vertex_descriptor, const Graph &) {}
  [[maybe_unused]] void examine_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void tree_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void back_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void forward_or_cross_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void finish_edge(edge_descriptor, const Graph &) {}

  explicit MeshLoaderState(oo::Mesh *mesh, Graph blocks);

 private:
  oo::Mesh *mMesh;
  BlockGraph mBlocks;
  Ogre::Matrix4 mTransform{Ogre::Matrix4::IDENTITY};
  std::shared_ptr<spdlog::logger> mLogger{};
};

void createMesh(oo::Mesh *mesh,
                oo::BlockGraph::vertex_descriptor start,
                const oo::BlockGraph &g);

/// @}

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_MESH_LOADER_STATE_HPP
