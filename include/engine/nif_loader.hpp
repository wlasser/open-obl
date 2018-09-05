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
};

// When constructing the mesh we want to iterate over the block graph, but
// because of references and pointers we will have to jump around and load
// things out of order when needed. By using a std::set we get efficient
// checking if a block has been loaded, and can remove blocks once loaded to
// avoid loading the same block twice. Since nif files can potentially have
// cycles (not sure if this happens in practice) we tag each vertex with an
// inProgress flag. If we go to load a vertex and find it inProgress, then there
// is a cycle.
class NifLoaderState {
 private:
  // Used to keep track of which blocks have been loaded and are currently
  // being loaded.
  enum class LoadStatus {
    Unloaded,
    Loading,
    Loaded
  };
  struct TaggedBlock {
    NifLoader::Block block{};
    LoadStatus tag{LoadStatus::Unloaded};

    // NOLINTNEXTLINE(google-explicit-constructor)
    TaggedBlock(NifLoader::Block block) : block(std::move(block)) {}
    TaggedBlock() = default;
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
