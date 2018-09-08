#ifndef OPENOBLIVION_ENGINE_NIF_LOADER_STATE_HPP
#define OPENOBLIVION_ENGINE_NIF_LOADER_STATE_HPP

#include "engine/nif_loader.hpp"
#include "nif/basic.hpp"
#include "nif/compound.hpp"
#include "nif/niobject.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <OgreAxisAlignedBox.h>
#include <OgreLogManager.h>
#include <OgreMatrix4.h>
#include <OgreMesh.h>
#include <OgrePass.h>
#include <OgreSubMesh.h>
#include <OgreTextureUnitState.h>
#include <OgreVertexIndexData.h>
#include <memory>
#include <stdexcept>
#include <vector>

namespace engine {

// When constructing the mesh we want to iterate over the block graph, but
// because of references and pointers we will have to jump around and load
// things out of order when needed. To detect cycles and ensure that some blocks
// are only loaded once, we tag each block with a LoadStatus.
class NifLoaderState {
 private:
  // Used to tag blocks to keep track of loading.
  enum class LoadStatus {
    Unloaded,
    Loading,
    Loaded
  };

  // A block and its load status. Blocks can be implicitly promoted to
  // unloaded TaggedBlocks, used in construction of the block graph.
  struct TaggedBlock {
    NifLoader::Block block{};
    LoadStatus tag{LoadStatus::Unloaded};

    // NOLINTNEXTLINE(google-explicit-constructor)
    TaggedBlock(NifLoader::Block block) : block(std::move(block)) {}
    // Required by BlockGraph
    TaggedBlock() = default;
  };

  // Used for RAII management of block load status. Should be constructed with
  // the tag of the block that is being loaded at the same scope of the block,
  // so that it goes out of scope when the block has finished loading.
  // Automatically detects cycles.
  class Tagger {
    LoadStatus &tag;
   public:
    explicit Tagger(LoadStatus &tag) : tag{tag} {
      switch (tag) {
        case LoadStatus::Unloaded: tag = LoadStatus::Loading;
          break;
        case LoadStatus::Loading:
          throw std::runtime_error("Cycle detected while loading nif file");
        default:break;
      }
    }
    ~Tagger() {
      tag = LoadStatus::Loaded;
    }
  };

  using TaggedBlockGraph = boost::adjacency_list<boost::vecS,
                                                 boost::vecS,
                                                 boost::bidirectionalS,
                                                 TaggedBlock>;

  TaggedBlockGraph blocks;

  template<class T, class S>
  T *getBlock(nif::basic::Ref<S> ref) {
    auto val = static_cast<int32_t>(ref);
    if (val < 0 || val >= blocks.vertex_set().size()) {
      throw std::out_of_range("Nonexistent reference");
    }
    return dynamic_cast<T *>(blocks[val].block.get());
  }

  struct BoundedSubmesh {
    Ogre::SubMesh *submesh{};
    Ogre::AxisAlignedBox bbox{};
  };
  BoundedSubmesh parseNiTriBasedGeom(nif::NiTriBasedGeom *block,
                                     LoadStatus &tag);

  std::shared_ptr<Ogre::Material>
  parseNiMaterialProperty(nif::NiMaterialProperty *block, LoadStatus &tag);

  struct TextureFamily {
    using TexturePtr = std::unique_ptr<Ogre::TextureUnitState>;
    TexturePtr base{};
    TexturePtr dark{};
    TexturePtr detail{};
    TexturePtr gloss{};
    TexturePtr glow{};
    // Bump textures are treated differently and we use normal maps anyway
    /* TexturePtr bump{}; */
    std::vector<TexturePtr> decals{};
  };
  // See parseTexDesc for why the pass is necessary.
  TextureFamily
  parseNiTexturingProperty(nif::NiTexturingProperty *block, LoadStatus &tag,
                           Ogre::Pass *pass);

  // When setting the texture name of a texture unit, Ogre looks up and loads
  // the texture using the resource group of its parent. Thus contrary to what
  // addTextureUnitState seems to suggest, one should not create a
  // Ogre::TextureUnitState with a nullptr parent, and we have to supply the
  // parent pass here.
  std::unique_ptr<Ogre::TextureUnitState>
  parseTexDesc(nif::compound::TexDesc *tex, Ogre::Pass *parent);

  void parseNiSourceTexture(nif::NiSourceTexture *block, LoadStatus &tag,
                            Ogre::TextureUnitState *tex);

  // Returns true if the triangle has a counterclockwise winding order, and
  // false otherwise
  bool isWindingOrderCCW(Ogre::Vector3 v1, Ogre::Vector3 n1,
                         Ogre::Vector3 v2, Ogre::Vector3 n2,
                         Ogre::Vector3 v3, Ogre::Vector3 n3);

  // Return the number of triangles with a counterclockwise winding order.
  // The mesh should have normals.
  long numCCWTriangles(nif::NiTriShapeData *block);

  // Reads vertex, normal, and texcoord data from NiGeometryData and prepares it
  // for rendering.
  std::unique_ptr<Ogre::VertexData>
  generateVertexData(nif::NiGeometryData *block, Ogre::Matrix4 transformation);

  // Reads triangle data from NiTriShapeData and prepares it for rendering.
  std::unique_ptr<Ogre::IndexData>
  generateIndexData(nif::NiTriShapeData *block);

  // Reads triangle strip data from NiTriStripsData and prepares it for
  // rendering.
  std::unique_ptr<Ogre::IndexData>
  generateIndexData(nif::NiTriStripsData *block);

  Ogre::AxisAlignedBox getBoundingBox(nif::NiGeometryData *block,
                                      Ogre::Matrix4 transformation);

  Ogre::Mesh *mesh;
  Ogre::LogManager &logger{Ogre::LogManager::getSingleton()};

 public:
  explicit NifLoaderState(Ogre::Mesh *mesh, NifLoader::BlockGraph blocks);
};

} // namespace engine

#endif // OPENOBLIVION_ENGINE_NIF_LOADER_STATE_HPP
