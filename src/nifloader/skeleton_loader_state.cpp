#include "conversions.hpp"
#include "nifloader/skeleton_loader_state.hpp"
#include <boost/graph/depth_first_search.hpp>
#include <vector>

namespace oo {

SkeletonLoaderState::SkeletonLoaderState(Ogre::Skeleton *skeleton, Graph blocks)
    : mSkeleton(skeleton), mLogger(spdlog::get(oo::LOG)) {
  std::vector<boost::default_color_type> colorMap(boost::num_vertices(blocks));
  const auto propertyMap{boost::make_iterator_property_map(
      colorMap.begin(), boost::get(boost::vertex_index, blocks))};

  boost::depth_first_search(blocks, *this, propertyMap);
}

SkeletonLoaderState::SkeletonLoaderState(
    Ogre::Skeleton *skeleton, Graph blocks, vertex_descriptor start,
    bool isSkeleton) : mSkeleton(skeleton),
                       mIsSkeleton(isSkeleton),
                       mLogger(spdlog::get(oo::LOG)) {
  std::vector<boost::default_color_type> colorMap(boost::num_vertices(blocks));
  const auto propertyMap{boost::make_iterator_property_map(
      colorMap.begin(), boost::get(boost::vertex_index, blocks))};

  boost::depth_first_visit(blocks, start, *this, propertyMap);
}

void SkeletonLoaderState::start_vertex(vertex_descriptor, const Graph &) {
  mTransform = Ogre::Matrix4::IDENTITY;
}

void SkeletonLoaderState::discover_vertex(vertex_descriptor v, const Graph &g) {
  const auto &niObject{*g[v]};

  if (dynamic_cast<const nif::NiNode *>(&niObject)) {
    discover_vertex(dynamic_cast<const nif::NiNode &>(niObject), g);
  } else if (dynamic_cast<const nif::BSXFlags *>(&niObject)) {
    discover_vertex(dynamic_cast<const nif::BSXFlags &>(niObject), g);
  }
}

void SkeletonLoaderState::finish_vertex(vertex_descriptor v, const Graph &g) {
  const auto &niObject{*g[v]};

  if (dynamic_cast<const nif::NiNode *>(&niObject)) {
    finish_vertex(dynamic_cast<const nif::NiNode &>(niObject), g);
  }
}

void SkeletonLoaderState::discover_vertex(const nif::BSXFlags &bsxFlags,
                                          const Graph &) {
  using Flags = nif::BSXFlags::Flags;
  const Flags flags{bsxFlags.data};
  if ((flags & Flags::bRagdoll) != Flags::bNone) {
    mIsSkeleton = true;
  }
}

void
SkeletonLoaderState::discover_vertex(const nif::NiNode &node, const Graph &) {
  mTransform = mTransform * oo::getTransform(node);
  if (!mIsSkeleton) return;

  // All NiNodes after the root are assumed to be bones
  auto *bone{mSkeleton->createBone(node.name.str())};
  if (mParentBone) mParentBone->addChild(bone);
  mParentBone = bone;

  bone->setInheritOrientation(true);
  bone->setInheritScale(true);

  bone->setPosition(oo::fromBSCoordinates(node.translation));
  bone->setScale(node.scale, node.scale, node.scale);
  bone->setOrientation(oo::fromBSCoordinates(node.rotation).transpose());

  bone->setBindingPose();
}

void
SkeletonLoaderState::finish_vertex(const nif::NiNode &node, const Graph &) {
  mTransform = mTransform * oo::getTransform(node).inverse();

  if (mIsSkeleton && mParentBone) {
    mParentBone = dynamic_cast<Ogre::Bone *>(mParentBone->getParent());
  }
}

} // namespace oo