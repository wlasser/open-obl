#ifndef OPENOBLIVION_ENGINE_NIF_LOADER_HPP
#define OPENOBLIVION_ENGINE_NIF_LOADER_HPP

#include "nif/niobject.hpp"
#include "nif/versionable.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <OgreAxisAlignedBox.h>
#include <OgreLogManager.h>
#include <OgreResource.h>
#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <string>

namespace engine {

class NifLoaderState;

// Handles custom loading of Nif files for Ogre. Each instance of this class is
// expected to load more than one nif file, so it cannot really be stateful.
// This class therefore handles the IO portion of loading, then constructs a
// NifLoaderState object to actually load the mesh.
class NifLoader : public Ogre::ManualResourceLoader {
 private:
  friend NifLoaderState;

  // Vertex properties are required to be copy constructable, so we cannot use
  // std::unique_ptr.
  using Block = std::shared_ptr<nif::NiObject>;
  using BlockGraph = boost::adjacency_list<boost::vecS, boost::vecS,
                                           boost::bidirectionalS, Block>;

  // Used for much briefer addition of vertices to a block graph, saving a large
  // if-else over the block type.
  typedef void(NifLoader::*addVertex_t)(BlockGraph &,
                                        BlockGraph::vertex_descriptor,
                                        nif::Version,
                                        std::istream &);

  // Add an edge from u to v. Does not check that v is a valid reference.
  template<class T>
  void addEdge(BlockGraph &blocks, BlockGraph::vertex_descriptor u,
               nif::basic::Ref<T> v);

  // Read a block of type T from the stream and add a pointer to it as a vertex
  // in the block graph.
  template<class T>
  void addVertex(BlockGraph &blocks, BlockGraph::vertex_descriptor u,
                 nif::Version nifVersion, std::istream &is);

  // To instantiate a header we need a version, but we don't know the version
  // unless we've read the header. This function reads the first line of the
  // header independently, grabs the version, then jumps back so that the header
  // can be read properly.
  nif::Version peekVersion(std::istream &);

  BlockGraph createBlockGraph(std::istream &is);

  Ogre::LogManager &logger{Ogre::LogManager::getSingleton()};

 public:
  void loadResource(Ogre::Resource *resource) override;
  //void prepareResource(Ogre::Resource *resource) override;

  // Read the nif file from the input stream and dump to the output stream in
  // obj format. Used for debugging.
  void dumpAsObj(std::istream &in, std::ostream &out);
};

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

  struct BoundedSubmesh {
    Ogre::SubMesh *submesh{};
    Ogre::AxisAlignedBox bbox{};
  };
  BoundedSubmesh parseNiTriShape(nif::NiTriShape *block, LoadStatus &tag);

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

  // Reads vertex, normal, and texcoord data from NiTriShapeData and prepares it
  // for rendering.
  std::unique_ptr<Ogre::VertexData>
  generateVertexData(nif::NiTriShapeData *block, Ogre::Matrix4 transformation);

  // Reads triangle data from NiTriShapeData and prepares it for rendering.
  std::unique_ptr<Ogre::IndexData>
  generateIndexData(nif::NiTriShapeData *block);

  Ogre::AxisAlignedBox getBoundingBox(nif::NiTriShapeData *block,
                                      Ogre::Matrix4 transformation);

  Ogre::Mesh *mesh;
  Ogre::LogManager &logger{Ogre::LogManager::getSingleton()};

 public:
  explicit NifLoaderState(Ogre::Mesh *mesh, NifLoader::BlockGraph blocks);
};

template<class T>
void NifLoader::addEdge(BlockGraph &blocks,
                        BlockGraph::vertex_descriptor u,
                        nif::basic::Ref<T> v) {
  auto vDesc = static_cast<BlockGraph::vertex_descriptor>(
      static_cast<int32_t>(v));
  boost::add_edge(u, vDesc, blocks);
}

template<class T>
void NifLoader::addVertex(BlockGraph &blocks,
                          BlockGraph::vertex_descriptor u,
                          nif::Version nifVersion,
                          std::istream &is) {
  std::shared_ptr<T> block{};
  if constexpr (std::is_base_of_v<nif::Versionable, T>) {
    block = std::make_shared<T>(nifVersion);
  } else {
    block = std::make_shared<T>();
  }
  block->read(is);
  blocks[u] = std::move(block);
}

} // namespace engine

#endif // OPENOBLIVION_ENGINE_NIF_LOADER_HPP
