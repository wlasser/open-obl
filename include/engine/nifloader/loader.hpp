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

namespace nifloader {

// To instantiate a header we need a version, but we don't know the version
// unless we've read the header. This function reads the first line of the
// header independently, grabs the version, then jumps back so that the header
// can be read properly.
nif::Version peekVersion(std::istream &);

// Vertex properties are required to be copy constructable, so we cannot use
// std::unique_ptr.
using Block = std::shared_ptr<nif::NiObject>;
using BlockGraph = boost::adjacency_list<boost::vecS, boost::vecS,
                                         boost::bidirectionalS, Block>;

// Add an edge from u to v. Does not check that v is a valid reference.
template<class T>
void addEdge(BlockGraph &blocks, BlockGraph::vertex_descriptor u,
             nif::basic::Ref<T> v);

// Read a block of type T from the stream and add a pointer to it as a vertex
// in the block graph.
template<class T>
void addVertex(BlockGraph &blocks, BlockGraph::vertex_descriptor u,
               nif::Version nifVersion, std::istream &is);

using AddVertexMap = std::map<std::string, decltype(&addVertex<nif::NiNode>)>;
const AddVertexMap &getAddVertexMap();

BlockGraph createBlockGraph(std::istream &is);

// Handles custom loading of Nif files for Ogre. Each instance of this class is
// expected to load more than one nif file, so it cannot really be stateful.
// This class therefore handles the IO portion of loading, then constructs a
// LoaderState object to actually load the mesh.
class Loader : public Ogre::ManualResourceLoader {
 private:
  friend class LoaderState;

  Ogre::LogManager *logger{nullptr};

 public:
  void loadResource(Ogre::Resource *resource) override;
  //void prepareResource(Ogre::Resource *resource) override;
};

template<class T>
void addEdge(BlockGraph &blocks,
             BlockGraph::vertex_descriptor u,
             nif::basic::Ref<T> v) {
  auto vDesc = static_cast<BlockGraph::vertex_descriptor>(
      static_cast<int32_t>(v));
  boost::add_edge(u, vDesc, blocks);
}

template<class T>
void addVertex(BlockGraph &blocks,
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

} // namespace nifloader
} // namespace engine

#endif // OPENOBLIVION_ENGINE_NIF_LOADER_HPP