#ifndef OPENOBL_DEBUG_DRAW_IMPL_HPP
#define OPENOBL_DEBUG_DRAW_IMPL_HPP

#include "bitflag.hpp"
#include "ogrebullet/debug_drawer.hpp"
#include <boost/circular_buffer.hpp>
#include <gsl/gsl>
#include <OgreMatrix4.h>
#include <memory>

namespace oo {

class OctreeNode;
class GameMode;

class DebugDrawImpl {
 private:
  struct DebugDrawFlags : Bitflag<8u, DebugDrawFlags> {
    static constexpr enum_t None{0u};
    static constexpr enum_t Collision{1u << 0u};
    static constexpr enum_t Occlusion{1u << 1u};
  };

  DebugDrawFlags mDebugDrawFlags{DebugDrawFlags::make(DebugDrawFlags::None)};
  std::unique_ptr<Ogre::DebugDrawer> mDebugDrawer{};

  GameMode *mGameMode;
  constexpr static std::size_t NUM_FPS_SAMPLES{64u};
  boost::circular_buffer<float> mFrameTimes;
  bool mFpsDisplayEnabled{false};

  /// Use the debug drawer to draw a line from the given `node` to each of its
  /// children, then from each each child to their children, and so on.
  void drawNodeChildren(gsl::not_null<Ogre::Node *> node,
                        const Ogre::Affine3 &t = Ogre::Affine3::IDENTITY)
  /*C++20: [[expects : mDebugDrawer != nullptr]]*/;

  /// Use the debug drawer to draw the skeleton of the given `entity`.
  void drawSkeleton(gsl::not_null<oo::Entity *> entity)
  /*C++20: [[expects : mDebugDrawer != nullptr]]*/;

  /// Use the debug drawer to draw the bounding box of the given `entity`.
  void drawBoundingBox(gsl::not_null<oo::Entity *> entity)
  /*C++20: [[expects : mDebugDrawer != nullptr]]*/;

  /// Use the debug drawer to draw the bounding box of the given scene node.
  void drawBoundingBox(gsl::not_null<Ogre::SceneNode *> node)
  /*C++20: [[expects : mDebugDrawer != nullptr]]*/;

  /// Use the debug drawer to draw the bounding box of the given octree node.
  void drawBoundingBox(gsl::not_null<oo::OctreeNode *> node)
  /*C++20: [[expects : mDebugDrawer != nullptr]]*/;

 public:
  /// Draw a window displaying the current fps and other timing information.
  /// If the fps display is inactive then nothing is drawn but the frame time
  /// is still recorded.
  void drawFpsDisplay(float delta);

  /// Draw all enabled debug information, if any.
  /// Does nothing if the debug drawer is inactive.
  void drawDebug();

  void setDebugDrawerEnabled(bool enable);
  void refreshDebugDrawer();
  void setDrawCollisionGeometryEnabled(bool enabled);
  void setDrawOcclusionGeometryEnabled(bool enabled);
  void setDisplayFpsEnabled(bool enabled);
  bool getDrawCollisionGeometryEnabled() const noexcept;
  bool getDrawOcclusionGeometryEnabled() const noexcept;
  bool getDisplayFpsEnabled() const noexcept;

  explicit DebugDrawImpl(GameMode *gameMode) noexcept;
  ~DebugDrawImpl() = default;
  DebugDrawImpl(const DebugDrawImpl &) = delete;
  DebugDrawImpl &operator=(const DebugDrawImpl &) = delete;
  DebugDrawImpl(DebugDrawImpl &&) = delete;
  DebugDrawImpl &operator=(DebugDrawImpl &&) = delete;
};

} // namespace oo

#endif // OPENOBL_DEBUG_DRAW_IMPL_HPP
