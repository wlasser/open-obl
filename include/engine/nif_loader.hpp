#ifndef OPENOBLIVION_ENGINE_NIF_LOADER_HPP
#define OPENOBLIVION_ENGINE_NIF_LOADER_HPP

#include "nif/niobject.hpp"
#include "nif/versionable.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <OgreResource.h>
#include <memory>

namespace engine {

class NifLoader : public Ogre::ManualResourceLoader {
 private:
  // Vertex properties are required to be copy constructible, so we cannot use
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

  // To instantiate a header we need a version, but we don't know the version
  // unless we've read the header. This function reads the first line of the
  // header independently, grabs the version, then jumps back so that the header
  // can be read properly.
  nif::Version peekVersion(std::istream &);

 public:
  void loadResource(Ogre::Resource *resource) override;
  //void prepareResource(Ogre::Resource *resource) override;
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
void NifLoader::addVertex(BlockGraph &blocks, BlockGraph::vertex_descriptor u,
                          nif::Version nifVersion, std::istream &is) {
  auto block = std::make_shared<T>(nifVersion);
  block->read(is);
  blocks[u] = std::move(block);
}

} // namespace engine

#endif // OPENOBLIVION_ENGINE_NIF_LOADER_HPP
