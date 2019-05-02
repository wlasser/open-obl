#ifndef OPENOBLIVION_SCENE_MANAGER_HPP
#define OPENOBLIVION_SCENE_MANAGER_HPP

#include "deferred_light_pass.hpp"
#include <OgreSceneManager.h>
#include <OgreSceneNode.h>
#include <absl/container/inlined_vector.h>
#include <boost/range/iterator_range.hpp>
#include <gsl/gsl>
#include <set>

namespace oo {

//===----------------------------------------------------------------------===//
// Deferred Lighting Base Scene Manager
//===----------------------------------------------------------------------===//
class DeferredSceneManager : public Ogre::SceneManager {
 public:
  explicit DeferredSceneManager(const Ogre::String &name);
  ~DeferredSceneManager() override = default;

  const Ogre::String &getTypeName() const override;

  // Instead of overriding the Light methods to construct the DeferredLights,
  // override the MovableObject methods and check for Lights. Otherwise, someone
  // could call createMovableObject on a Light and bypass the DeferredLight
  // construction, for example.
  Ogre::MovableObject *
  createMovableObject(const Ogre::String &name,
                      const Ogre::String &typeName,
                      const Ogre::NameValuePairList *params = nullptr) override;
  void destroyMovableObject(const Ogre::String &name,
                            const Ogre::String &typeName) override;
  void destroyAllMovableObjectsByType(const Ogre::String &typeName) override;
  void destroyAllMovableObjects() override;

  std::vector<oo::DeferredLight *> getLights() const;
  DeferredFogListener *getFogListener() noexcept;

 private:
  struct LightInfo {
    Ogre::Light *light{};
    std::unique_ptr<oo::DeferredLight> geometry{};
    LightInfo(Ogre::Light *light, std::unique_ptr<oo::DeferredLight> geometry);
  };
  std::vector<LightInfo> mLights{};

  DeferredFogListener mFogListener;
};

class DeferredSceneManagerFactory : public Ogre::SceneManagerFactory {
 public:
  friend DeferredSceneManager;

  DeferredSceneManagerFactory() = default;
  ~DeferredSceneManagerFactory() override = default;

  gsl::owner<Ogre::SceneManager *>
  createInstance(const Ogre::String &instanceName) override;
  void destroyInstance(gsl::owner<Ogre::SceneManager *> instance) override;

 protected:
  void initMetaData() const override;

 private:
  constexpr static const char *FACTORY_TYPE_NAME{"oo::DeferredSceneManager"};
};

//===----------------------------------------------------------------------===//
// Interior Scene Manager
//===----------------------------------------------------------------------===//
class OctreeSceneNode : public Ogre::SceneNode {
 public:
  explicit OctreeSceneNode(Ogre::SceneManager *creator);
  explicit OctreeSceneNode(Ogre::SceneManager *creator,
                           const Ogre::String &name);
};

class OctreeNode;
using OctreeNodePtr = std::unique_ptr<OctreeNode>;

struct IntegralAAB {
  Ogre::Vector3i min;
  Ogre::Vector3i max;
};

class OctreeNode {
 private:
  /// Bounding box of this region, in octree units.
  oo::IntegralAAB mBbox{};
  /// Octants of this region.
  std::array<oo::OctreeNodePtr, 8u> mOctants{};
  /// Parent node of this region, namely the smallest `OctreeNode` fully
  /// containing this node.
  oo::OctreeNode *mParent{};
  /// SceneNodes that are contained within this region, namely those objects
  /// that are within the region but that intersect at least two octants.
  absl::InlinedVector<oo::OctreeSceneNode *, 2u> mSceneNodes{};

  friend void
  buildOctreeImpl(oo::OctreeNode *parent,
                  const std::set<oo::OctreeSceneNode *> &nodes);

 public:
  /// The side length of the smallest possible OctreeNode, in meters.
  /// OctreeNode sizes are integral with minimum size `1`, corresponding to a
  /// game world size of `UNIT_SIZE`.
  constexpr static float UNIT_SIZE{0.5f};

  OctreeNode(oo::OctreeNode *parent, oo::IntegralAAB bbox);

  oo::IntegralAAB getBoundingBox() const noexcept;
  // TODO: Return an iterator whose value_type is a non-owning pointer.
  std::array<oo::OctreeNodePtr, 8u> &getChildren() noexcept;

  auto getSceneNodes() noexcept {
    return boost::make_iterator_range(mSceneNodes.begin(), mSceneNodes.end());
  }

  auto getSceneNodes() const noexcept {
    return boost::make_iterator_range(mSceneNodes.cbegin(), mSceneNodes.cend());
  }
};

template<class InputIt, class = std::enable_if_t<
    std::is_base_of_v<std::input_iterator_tag,
                      typename std::iterator_traits<InputIt>::iterator_category>
        && std::is_convertible_v<Ogre::SceneNode *,
                                 typename std::iterator_traits<InputIt>::value_type>>>
oo::OctreeNodePtr buildOctree(InputIt first, InputIt last);

void buildOctreeImpl(oo::OctreeNode *parent,
                     const std::set<oo::OctreeSceneNode *> &nodes);

/// \see gui::preOrderDFS()
template<class F> void preOrderDFS(oo::OctreeNode *node, F &&visitor);

class InteriorSceneManager : public DeferredSceneManager {
 public:
  explicit InteriorSceneManager(const Ogre::String &name);
  ~InteriorSceneManager() override = default;

  const Ogre::String &getTypeName() const override;

  void _updateSceneGraph(Ogre::Camera *camera) override;

  void _findVisibleObjects(Ogre::Camera *camera,
                           Ogre::VisibleObjectsBoundsInfo *visibleBounds,
                           bool onlyShadowCasters) override;

  oo::OctreeNode *_getOctree() noexcept;

 protected:
  gsl::owner<oo::OctreeSceneNode *> createSceneNodeImpl() override;

  gsl::owner<oo::OctreeSceneNode *>
  createSceneNodeImpl(const Ogre::String &name) override;

 private:
  std::unique_ptr<oo::OctreeNode> mOctree{};

};

class InteriorSceneManagerFactory : public Ogre::SceneManagerFactory {
 public:
  friend InteriorSceneManager;

  InteriorSceneManagerFactory() = default;
  ~InteriorSceneManagerFactory() override = default;

  gsl::owner<Ogre::SceneManager *>
  createInstance(const Ogre::String &instanceName) override;
  void destroyInstance(gsl::owner<Ogre::SceneManager *> instance) override;

 protected:
  void initMetaData() const override;

 private:
  constexpr static const char *FACTORY_TYPE_NAME{"oo::InteriorSceneManager"};
};

//===----------------------------------------------------------------------===//
// Function template definitions
//===----------------------------------------------------------------------===//

template<class InputIt, class>
oo::OctreeNodePtr buildOctree(InputIt first, InputIt last) {
  std::set<oo::OctreeSceneNode *> nodes{};
  auto inserter{std::inserter(nodes, nodes.begin())};
  std::transform(first, last, inserter, [](Ogre::SceneNode *node) {
    if (auto *n{dynamic_cast<oo::OctreeSceneNode *>(node)}) return n;
    OGRE_EXCEPT(Ogre::Exception::ERR_INVALIDPARAMS,
                "Ogre::SceneNode is not an oo::OctreeSceneNode",
                "oo::buildOctree");
  });

  auto root{std::make_unique<oo::OctreeNode>(nullptr,
                                             oo::IntegralAAB{
                                                 -128 * Ogre::Vector3i{1, 1, 1},
                                                 128 * Ogre::Vector3i{1, 1, 1}
                                             })};
  oo::buildOctreeImpl(root.get(), nodes);
  return root;
}

template<class F> void preOrderDFS(oo::OctreeNode *node, F &&visitor) {
  if (!visitor(node)) return;
  for (const auto &child : node->getChildren()) {
    oo::preOrderDFS(child.get(), std::forward<F>(visitor));
  }
}

} // namespace oo

#endif // OPENOBLIVION_SCENE_MANAGER_HPP
