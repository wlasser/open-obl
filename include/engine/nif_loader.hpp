#ifndef OPENOBLIVION_ENGINE_NIF_LOADER_HPP
#define OPENOBLIVION_ENGINE_NIF_LOADER_HPP

#include "nif/basic.hpp"
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
#include <type_traits>

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

  Ogre::LogManager *logger{nullptr};

 public:
  void loadResource(Ogre::Resource *resource) override;
  //void prepareResource(Ogre::Resource *resource) override;

  // Read the nif file from the input stream and dump to the output stream in
  // obj format. Used for debugging.
  void dumpAsObj(std::istream &in, std::ostream &out);
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
