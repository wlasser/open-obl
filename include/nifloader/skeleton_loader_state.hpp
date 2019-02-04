#ifndef OPENOBLIVION_NIFLOADER_SKELETON_LOADER_STATE_HPP
#define OPENOBLIVION_NIFLOADER_SKELETON_LOADER_STATE_HPP

#include "nifloader/loader.hpp"
#include "nifloader/loader_state.hpp"
#include "settings.hpp"
#include <Ogre.h>
#include <spdlog/spdlog.h>

namespace oo {

/// \addtogroup OpenOblivionNifloader

class SkeletonLoaderState {
 public:
  using Graph = oo::BlockGraph;
  using vertex_descriptor = Graph::vertex_descriptor;
  using edge_descriptor = Graph::edge_descriptor;

  void start_vertex(vertex_descriptor, const Graph &);
  void discover_vertex(vertex_descriptor, const Graph &);
  void finish_vertex(vertex_descriptor, const Graph &);

  [[maybe_unused]] void initialize_vertex(vertex_descriptor, const Graph &) {}
  [[maybe_unused]] void examine_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void tree_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void back_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void forward_or_cross_edge(edge_descriptor, const Graph &) {}
  [[maybe_unused]] void finish_edge(edge_descriptor, const Graph &) {}

  explicit SkeletonLoaderState(Ogre::Skeleton *skeleton, Graph blocks);

  explicit SkeletonLoaderState(Ogre::Skeleton *skeleton,
                               Graph blocks,
                               vertex_descriptor start,
                               bool isSkeleton = false);

 private:
  Ogre::Skeleton *mSkeleton{};
  Ogre::Matrix4 mTransform{Ogre::Matrix4::IDENTITY};
  Ogre::Bone *mParentBone{};
  bool mIsSkeleton{false};
  std::shared_ptr<spdlog::logger> mLogger{};

  void discover_vertex(const nif::NiNode &node, const Graph &g);
  void discover_vertex(const nif::BSXFlags &bsxFlags, const Graph &g);

  void finish_vertex(const nif::NiNode &node, const Graph &g);
};

/// @}

} // namespace oo

#endif // OPENOBLIVION_NIFLOADER_SKELETON_LOADER_STATE_HPP
