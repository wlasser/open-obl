#ifndef OPENOBLIVION_NIF_LOADER_HPP
#define OPENOBLIVION_NIF_LOADER_HPP

#include "nif/niobject.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <polymorphic_value.h>
#include <algorithm>
#include <istream>
#include <map>
#include <string>
#include <type_traits>

/// \defgroup OpenOblivionNifloader NIF Loader
/// Parsers for NIF files into different OGRE resources.
///
/// A single NIF file can contain descriptions of what amounts to several
/// different `Ogre::Resource`s. For example, NIF files can contain collision
/// and animation information as well as mesh information. Because of this,
/// a single NIF file is arguably more structurally similar to a subgraph of the
/// scene graph than a single resource. Nonetheless, because NIF files use a
/// much wider variety of types of scene node than Ogre, we do in fact treat NIF
/// files as a resource.
///
/// In particular, each NIF is an `Ogre::Resource` in its own right---an
/// `Ogre::NifResource`---and it is not predetermined that a NIF file
/// represents an `Ogre::Mesh`, or `Ogre::Skeleton`, or another resource.
/// Rather, each `Ogre::NifResource` is expected to be loaded once generically
/// and then used several times as a starting point for other specialized
/// loaders which interpret the `Ogre::NifResource` as a conventional resource.
/// Any irrelevant parts of the NIF are simply ignored during this second
/// stage.
///
/// The order of operations above is slightly backwards; in actuality a single
/// NIF file---as it appears in an `Ogre::ResourceLocation`---is declared as
/// each type of `Ogre::Resource` that it could represent, but never explicitly
/// as an `Ogre::NifResource`. When the user loads a NIF file with the loader
/// corresponding to the resource type they require, the loader first attempts
/// to load the NIF as an `Ogre::NifResource`, then proceeds with the
/// specialized loading that it was declared with.
///
/// In the case of a `record::STAT` for example, the specified NIF file is
/// expected to represent both an `Ogre::Mesh` and an `Ogre::CollisionObject`.
/// Both the `oo::MeshLoader` and `oo::CollisionObjectLoader` will load the NIF
/// as an `Ogre::NifResource`, but the loading will only actually happen once
/// because OGRE helpfully caches the `Ogre::NifResource` after the first load.
/// This means that the NIF file still only has to be loaded from disk once,
/// even though it is used to construct multiple different resources.
///
/// Unlike the specialized loaders, there is no explicit `oo::NifLoader`
/// because there is only one way to load an `Ogre::NifResource`. The main work
/// is done in `Ogre::NifResource::load()` via `oo::createBlockGraph()`.

namespace oo {

/// \addtogroup OpenOblivionNifloader
/// @{

/// Read the first line of the Nif header, grab the version, then jump back so
/// that the header can be read properly.
/// This method is necessary because to instantiate a header we need a version,
/// but we don't know the version unless we've read the header.
nif::Version peekVersion(std::istream &);

/// Polymorphic representation of a generic `nif::NiObject`, used as a node in
/// the node hierarchy.
/// This is an experiment to try out a feature that might make it in to C++20,
/// as an alternative to `std::shared_ptr`. Note that we can't use
/// `std::unique_ptr`, because Boost requires the type to be copyable.
using Block = jbcoe::polymorphic_value<nif::NiObject>;

/// DAG representing the hierarchy of `nif::NiObject`s in a Nif file.
/// If there is an edge from `oo::Block` A to `oo::Block` B then B is a child
/// of A.
/// \remark Note the direction of logical implication here; not all nodes have
///         an edge to their children due to how the specialized loaders work.
///         This is unfortunate, and ultimately needs fixing.
/// \todo Elaborate on what edges are missing and why.
using BlockGraph = boost::adjacency_list<boost::vecS, boost::vecS,
                                         boost::bidirectionalS, Block>;

/// Parse a Nif file into a hierarchy of `nif::NiObject`s.
BlockGraph createBlockGraph(std::istream &is);

/// Add an edge from u to v. Does not check that v is a valid reference.
template<class T>
void addEdge(BlockGraph &blocks,
             BlockGraph::vertex_descriptor u,
             nif::basic::Ref<T> v) {
  auto vDesc = static_cast<BlockGraph::vertex_descriptor>(
      static_cast<int32_t>(v));
  boost::add_edge(u, vDesc, blocks);
}

/// Read a block of type T from the stream and add a pointer to it as a vertex
/// in the block graph.
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

template<class T, class S>
T &getBlock(BlockGraph &g, nif::basic::Ref<S> ref) {
  const auto val{static_cast<int32_t>(ref)};
  if (val < 0 || static_cast<std::size_t>(val) >= boost::num_vertices(g)) {
    throw std::out_of_range("Nonexistent reference");
  }
  return dynamic_cast<T &>(*g[val]);
}

template<class T, class S>
const T &getBlock(const BlockGraph &g, nif::basic::Ref<S> ref) {
  const auto val{static_cast<int32_t>(ref)};
  if (val < 0 || static_cast<std::size_t>(val) >= boost::num_vertices(g)) {
    throw std::out_of_range("Nonexistent reference");
  }
  return dynamic_cast<const T &>(*g[val]);
}

template<class T, class S>
T &getBlock(BlockGraph &g, nif::basic::Ptr<S> ptr) {
  const auto val{static_cast<int32_t>(ptr)};
  if (static_cast<std::size_t>(val) >= boost::num_vertices(g)) {
    throw std::out_of_range("Nonexistent pointer");
  }
  return dynamic_cast<T &>(*g[val]);
}

// TODO: This is awful, stop doing dynamic_cast checks!
template<class T, class S,
    class = std::enable_if_t<std::is_convertible_v<T *, S *>>>
bool checkRefType(const BlockGraph &g, nif::basic::Ref<S> ref) {
  const auto val{static_cast<int32_t>(ref)};
  if (val < 0 || static_cast<std::size_t>(val) >= boost::num_vertices(g)) {
    return false;
  }
  // This is horrific.
  return dynamic_cast<const T *>(&*g[val]) != nullptr;
}

/// Returns an iterator into the BlockGraph vertex_set.
// TODO: Get rid of this, it's hideous.
template<class T> auto getBlockIndex(const BlockGraph &g, const T &block) {
  // TODO: This is way more work than we need to do here
  auto comp = [&](auto i) {
    return dynamic_cast<const T *>(&*g[i]) == &block;
  };
  return std::find_if(g.vertex_set().begin(), g.vertex_set().end(), comp);
}

///@}

} // namespace oo

#endif // OPENOBLIVION_NIF_LOADER_HPP
