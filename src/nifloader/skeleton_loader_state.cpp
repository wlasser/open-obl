#include "conversions.hpp"
#include "nifloader/skeleton_loader_state.hpp"
#include <boost/graph/depth_first_search.hpp>
#include <vector>

namespace oo {

SkeletonLoaderState::SkeletonLoaderState(Ogre::Skeleton *skeleton,
                                         oo::BlockGraph blocks) {
  std::vector<boost::default_color_type> colorMap(boost::num_vertices(blocks));
  const auto indexMap{boost::get(boost::vertex_index, blocks)};
  const auto propertyMap{boost::make_iterator_property_map(colorMap.begin(),
                                                           indexMap)};
  SkeletonVisitor visitor(skeleton);
  boost::depth_first_search(blocks, std::move(visitor), propertyMap);
}

void SkeletonVisitor::start_vertex(SkeletonVisitor::vertex_descriptor,
                                   const SkeletonVisitor::Graph &) {
  mTransform = Ogre::Matrix4::IDENTITY;
}

void SkeletonVisitor::discover_vertex(SkeletonVisitor::vertex_descriptor v,
                                      const SkeletonVisitor::Graph &g) {
  const auto &niObject{*g[v]};

  if (dynamic_cast<const nif::NiNode *>(&niObject)) {
    discover_vertex(dynamic_cast<const nif::NiNode &>(niObject), g);
  } else if (dynamic_cast<const nif::BSXFlags *>(&niObject)) {
    discover_vertex(dynamic_cast<const nif::BSXFlags &>(niObject), g);
  }
}

void SkeletonVisitor::finish_vertex(SkeletonVisitor::vertex_descriptor v,
                                    const SkeletonVisitor::Graph &g) {
  const auto &niObject{*g[v]};

  if (dynamic_cast<const nif::NiNode *>(&niObject)) {
    finish_vertex(dynamic_cast<const nif::NiNode &>(niObject), g);
  }
}

void SkeletonVisitor::discover_vertex(const nif::BSXFlags &bsxFlags,
                                      const SkeletonVisitor::Graph &) {
  using Flags = nif::BSXFlags::Flags;
  const Flags flags{bsxFlags.data};
  if ((flags & Flags::bRagdoll) != Flags::bNone) {
    mIsSkeleton = true;
  }
}

void SkeletonVisitor::discover_vertex(const nif::NiNode &node,
                                      const SkeletonVisitor::Graph &) {
  mTransform = mTransform * oo::getTransform(node);
  if (!mIsSkeleton) return;

  // All NiNodes after the root are assumed to be bones
  auto *bone{mSkeleton->createBone(node.name.str())};
  if (mParentBone) mParentBone->addChild(bone);
  mParentBone = bone;

  bone->setInheritOrientation(true);
  bone->setInheritScale(true);

  bone->setPosition(oo::fromBSCoordinates(oo::fromNif(node.translation)));
  bone->setScale(node.scale, node.scale, node.scale);
  bone->setOrientation(oo::fromBSCoordinates(fromNif(node.rotation)));

  bone->setBindingPose();
}

void SkeletonVisitor::finish_vertex(const nif::NiNode &node,
                                    const SkeletonVisitor::Graph &) {
  mTransform = mTransform * oo::getTransform(node).inverse();

  if (mIsSkeleton) {
    mParentBone = dynamic_cast<Ogre::Bone *>(mParentBone->getParent());
  }
}

} // namespace oo