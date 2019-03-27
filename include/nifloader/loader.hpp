#ifndef OPENOBLIVION_NIF_LOADER_HPP
#define OPENOBLIVION_NIF_LOADER_HPP

#include "nif/niobject.hpp"
#include <boost/graph/adjacency_list.hpp>
#include <OgreMath.h>
#include <polymorphic_value.h>
#include <algorithm>
#include <iosfwd>
#include <map>
#include <string>
#include <type_traits>

/// \defgroup OpenOblivionNifloader NIF Loader
/// Parsers for NIF files into different OGRE resources.
///
/// ### Overview
///
/// An object in the game world usually consists of multiple components; it has
/// an `Ogre::Entity` component defining how it looks, an `Ogre::RigidBody`
/// component defining the physics it obeys, and possibly an
/// `Ogre::SkeletonInstance` describing how it is animated. All these things
/// represent different facets of a single object, and it is unlikely that one
/// would exist without all the others. To OGRE, these three things each come
/// from a different `Ogre::Resource`: an `Ogre::Mesh` for the `Ogre::Entity`,
/// an `Ogre::CollisionShape` for the `Ogre::RigidBody`, and an
/// `Ogre::Skeleton` for the `Ogre::SkeletonInstance`. To a NIF file however,
/// they are all part of the same whole and thus are all described by the same
/// file (with the exception of some skeletons, discussed later).
///
/// A NIF file consists of a hierarchy of *blocks* forming a subgraph of some
/// hypothetical scene graph. Each block corresponds to a node in the graph,
/// each describing different properties of the object represented by the file.
/// In the NIF file, every piece of information is part of the scene graph, such
/// that an engine could---and presumably does, in the original
/// implementation---take the subgraph and copy it directly into the scene graph
/// of the game world. For us it is not the simple, as OGRE does *not* treat
/// every piece of information as an `Ogre::SceneNode`; `Ogre::Entity`s and
/// `Ogre::RigidBody`s are instances of `Ogre::MovableObject`, which are
/// *attached* to `Ogre::SceneNode`s instead of being `Ogre::SceneNode`s
/// themselves.
///
/// Because NIF file contain descriptions of what are, to OGRE, different
/// resources, it is tempting to split apart each NIF file into an `Ogre::Mesh`,
/// an `Ogre::CollisionShape`, and so on. Not only does this present
/// difficulties when multiple different meshes or physics objects are contained
/// in the same file (consider a falling rockslide trap for instance, which has
/// many different rocks), the graph structure of the file carries key
/// information about how to assemble the different components in the game
/// world, and cannot be discarded. Nonetheless, we would still like to use
/// OGRE's very handy resource system, and thus treat each NIF file as a single
/// `Ogre::NifResource`.
///
/// We now have the problem that a single `Ogre::NifResource` actually contains
/// multiple *other* OGRE resources, and moreover the number and type of
/// resources is not known until the `Ogre::NifResource` has been loaded.
/// `Ogre::NifResource`s are therefore not used to produce individual
/// `Ogre::MovableObject`s directly, but instead are used to insert new
/// `Ogre::SceneNode`s with attached `Ogre::MovableObject`s directly into an
/// `Ogre::SceneManager`'s scene graph. This is done primarily by the
/// `oo::insertNif()` function. An `Ogre::NifResource` is then a representation
/// of the subgraph stored in the NIF file, while `oo::insertNif()` is a
/// transformation of that subgraph into a form understandable by OGRE, followed
/// by an inclusion of that transformed subgraph into our scene graph.
///
/// It is notable that multiple calls to `oo::insertNif()` with the same NIF
/// file as input do not require multiple reads of the NIF file; the first call
/// will load the appropriate `Ogre::NifResource`, and any subsequent calls will
/// simply use that resource cached by OGRE. The different `Ogre::Resource`s
/// produced by `oo::insertNif()` are also cached, so multiple insertions of
/// the same NIF file require only relatively quick constructions of
/// `Ogre::MovableObject`s, which would be required in any other implementation
/// of a model loading system.
///
/// ### The Block Graph
///
/// In the NIF file, the vertices of the subgraph, called *blocks*, are given a
/// type deriving from `nif::NiObject`. These types for a very deep hierarchy
/// and, along with a version number for the NIF file, precisely define the
/// properties of that block. The blocks themselves are stored sequentially in
/// some order (think a `std::vector<nif::NiObject *>`), with the position of
/// the block, called its *block index*, uniquely identifying the block in that
/// NIF file.
///
/// The subgraph itself is variously referred to as *the block hierarchy*,
/// *the hierarchy*, or *the block graph*, and is represented by the Boost
/// Graph Library (BGL) type `oo::BlockGraph`. The vertices of this graph type
/// are instances of `oo::Block`, which are owning wrappers around a polymorphic
/// `nif::NiObject` instance. In the NIF file, the various relationships
/// between blocks are expressed using the `nif::basic::Ref<T>` and
/// `nif::basic::Ptr<T>` class templates. These are specialized for each block
/// type (class derived from `nif::NiObject`), with their instances storing the
/// block index of some block of that type---or any type derived from it---in
/// the hierarchy. They therefore act a lot like pointers, though there are some
/// important differences. Firstly, `nif::basic::Ref`s point 'down' the
/// hierarchy, to blocks whose block index is greater than the block containing
/// the `nif::basic::Ref`, whereas `nif::basic::Ptr`s point 'up' the hierarchy.
/// Moreover `nif::basic::Ref`s can be null, whereas `nif::basic::Ptr`s cannot
/// be. This terminology is unfortunate, as it is opposite to the C++ notion of
/// pointers and references!
///
/// \todo It might be nice if `nif::basic::Ptr` and `nif::basic::Ref` can know
///       which block graph they point into somehow, and then become actual
///       fancy pointers which are dereferenceable.
///
/// The distinction between `nif::basic::Ref`s and `nif::basic::Ptr`s is
/// mirrored in the construction of the block graph. Draw an edge from block `A`
/// to block `B` if and only if block `A` contains a `nif::basic::Ref` pointing
/// to block `B`. If this is done, then because `nif::basic::Ref`s point down
/// the hierarchy, every edge will go from a block with index \f$i\f$ to a block
/// with index \f$j > i\f$. The ordering of the blocks is then automatically a
/// topological ordering, and the block graph is a DAG (directed acyclic graph).
/// On the other hand, the use of `nif::basic::Ptr`s as a way to point back up
/// the hierarchy can create cycles if introduced as edges.
///
/// \remark In practice not all edges are added to the graph. The implication
///         'If there is an edge from `A` to `B` then `A` contains a
///         `nif::basic::Ref` pointing to `B`' is true, but the converse is not.
///         Only those edges that are actually needed have been added; see
///         the implementation of `oo::createBlockGraph()` for an list.
///
/// ### Inserting NIF Files
///
/// The primary method of inserting the contents of a NIF file into a scene is
/// through the `oo::insertNif()` function, which does a depth-first visit
/// through the block graph of a NIF file and performs a different action
/// depending on the type of the block. The terminology used by
/// `boost::depth_first_visit` in the BGL is used below. The current working
/// node is set initially to a given root node, usually the scene root.
///  - If a `nif::NiNode` is discovered, then an `Ogre::SceneNode` is added
///    to the scene graph with the same transformation, respecting the
///    parent-child relationships of the `nif::NiNode` with any other
///    `nif::NiNode`s in the file. The transformation is set as the initial
///    state of the node for animation purposes via
///    `Ogre::SceneNode::setInitialState()`.
///
///    The added node is set as the current working node. The first block of
///    most NIF files---at least all those intended to be added directly to the
///    scene---is a `nif::NiNode` representing the root node of the object. One
///    therefore does not usually need to create any new nodes before calling
///    `oo::insertNif()`.
///  - If a `nif::NiNode` is finished then the parent of the current working
///    node is made the new working node, effectively moving a level up the
///    scene graph. This doesn't happen for the NIF's root node, which is
///    returned by the method.
///  - If a `nif::NiTriBasedGeom` is discovered, then an `Ogre::SubMesh` is
///    created using the vertex and material information blocks that the
///    `nif::NiTriBasedGeom` has references to. Sibling `nif::NiTriBasedGeom`
///    blocks represent sibling `Ogre::SubMesh`s of an `Ogre::Mesh`, which we
///    take as being attached to the parent (and current working) node.
///
///    The actual creation of the `Ogre::SubMesh` and `Ogre::Mesh` is performed
///    by the `oo::createMesh()` function, which takes a pointer to a
///    `Ogre::Mesh` resource to populate and the vertex of the parent node in
///    the block graph.
///
///    The resulting `Ogre::Mesh` is used to construct an `Ogre::Entity` which
///    is then attached to the current working node.
///  - If a `nif::bhk::CollisionObject` is discovered, then an
///    `Ogre::CollisionShape` is created using that block and its child
///    `nif::bhk::RigidBody` child by delegating to the
///    `oo::createCollisionObject()` function, analogous to `oo::createMesh()`.
///    The `Ogre::CollisionShape` is used to construct an `Ogre::RigidBody`
///    which is then attached to the current working node and added to the
///    physics world passed to `oo::insertNif`.
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

template<class T, class S>
const T &getBlock(const BlockGraph &g, nif::basic::Ptr<S> ptr) {
  const auto val{static_cast<int32_t>(ptr)};
  if (static_cast<std::size_t>(val) >= boost::num_vertices(g)) {
    throw std::out_of_range("Nonexistent pointer");
  }
  return dynamic_cast<const T &>(*g[val]);
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

/// Convert the translation, rotation, and scale parameters into Ogre
/// coordinates and return a combined transformation matrix.
/// \ingroup OpenOblivionNifloader
Ogre::Matrix4 getTransform(const nif::NiAVObject &block);

///@}

} // namespace oo

#endif // OPENOBLIVION_NIF_LOADER_HPP
