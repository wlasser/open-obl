#ifndef OPENOBLIVION_ENGINE_NIF_LOADER_STATE_HPP
#define OPENOBLIVION_ENGINE_NIF_LOADER_STATE_HPP

#include "engine/nifloader/loader.hpp"
#include <boost/graph/adjacency_list.hpp>

namespace engine::nifloader {

// Used to tag blocks to keep track of loading.
enum class LoadStatus {
  Unloaded,
  Loading,
  Loaded
};

// A block and its load status. Blocks can be implicitly promoted to
// unloaded TaggedBlocks, used in construction of the block graph.
struct TaggedBlock {
  Block block{};
  mutable LoadStatus tag{LoadStatus::Unloaded};

  // NOLINTNEXTLINE(google-explicit-constructor)
  TaggedBlock(Block block) : block(std::move(block)) {}
  // Required by BlockGraph
  TaggedBlock() = default;
};

// When constructing the mesh we want to iterate over the block graph, but
// because of references and pointers we will have to jump around and load
// things out of order when needed. To detect cycles and ensure that some blocks
// are only loaded once, we tag each block with a LoadStatus.
using TaggedBlockGraph = boost::adjacency_list<boost::vecS,
                                               boost::vecS,
                                               boost::bidirectionalS,
                                               TaggedBlock>;

// Used for RAII management of block load status. Should be constructed with
// the tag of the block that is being loaded at the same scope of the block,
// so that it goes out of scope when the block has finished loading.
// Automatically detects cycles.
class Tagger {
  LoadStatus &tag;
 public:
  explicit Tagger(LoadStatus &tag);
  ~Tagger();
};

// Convert the translation, rotation, and scale parameters into Ogre coordinates
// and return a combined transformation matrix.
Ogre::Matrix4 getTransform(nif::NiAVObject *block);

} // namespace engine::nifloader

#endif // OPENOBLIVION_ENGINE_NIF_LOADER_STATE_HPP
