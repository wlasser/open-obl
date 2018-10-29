#ifndef OPENOBLIVION_ENGINE_NIF_LOADER_HPP
#define OPENOBLIVION_ENGINE_NIF_LOADER_HPP

#include "nif/basic.hpp"
#include "nif/niobject.hpp"
#include "nif/versionable.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <OgreAxisAlignedBox.h>
#include <OgreLogManager.h>
#include <OgreResource.h>
#include <polymorphic_value.h>
#include <spdlog/spdlog.h>
#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>

namespace engine::nifloader {

// To instantiate a header we need a version, but we don't know the version
// unless we've read the header. This function reads the first line of the
// header independently, grabs the version, then jumps back so that the header
// can be read properly.
nif::Version peekVersion(std::istream &);

using Block = jbcoe::polymorphic_value<nif::NiObject>;
using BlockGraph = boost::adjacency_list<boost::vecS, boost::vecS,
                                         boost::bidirectionalS, Block>;

BlockGraph createBlockGraph(std::istream &is);

// Add an edge from u to v. Does not check that v is a valid reference.
template<class T>
void addEdge(BlockGraph &blocks,
             BlockGraph::vertex_descriptor u,
             nif::basic::Ref<T> v) {
  auto vDesc = static_cast<BlockGraph::vertex_descriptor>(
      static_cast<int32_t>(v));
  boost::add_edge(u, vDesc, blocks);
}

// Read a block of type T from the stream and add a pointer to it as a vertex
// in the block graph.
template<class T>
void addVertex(BlockGraph &blocks,
               BlockGraph::vertex_descriptor u,
               [[maybe_unused]] nif::Version nifVersion,
               std::istream &is) {
  if constexpr (std::is_base_of_v<nif::Versionable, T>) {
    blocks[u] = jbcoe::make_polymorphic_value<nif::NiObject, T>(nifVersion);
    blocks[u]->read(is);
  } else {
    blocks[u] = jbcoe::make_polymorphic_value<nif::NiObject, T>();
    blocks[u]->read(is);
  }
}

using AddVertexMap = std::map<std::string, decltype(&addVertex<nif::NiNode>)>;
const AddVertexMap &getAddVertexMap();

} // namespace engine::nifloader

#endif // OPENOBLIVION_ENGINE_NIF_LOADER_HPP
