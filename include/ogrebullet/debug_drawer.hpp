#ifndef OPENOBL_OGREBULLET_DEBUG_DRAWER_HPP
#define OPENOBL_OGREBULLET_DEBUG_DRAWER_HPP

#include <btBulletDynamicsCommon.h>
#include <OgreColourValue.h>
#include <OgrePrerequisites.h>
#include <OgreVector3.h>

namespace Ogre {

class DebugDrawer : public btIDebugDraw {
 public:
  explicit DebugDrawer(SceneManager *mgr, const String &group);

  void drawLine(const btVector3 &from,
                const btVector3 &to,
                const btVector3 &colour) override;

  void drawContactPoint(const btVector3 &/*pointOnB*/,
                        const btVector3 &/*normalOnB*/,
                        btScalar /*distance*/,
                        int /*lifetime*/,
                        const btVector3 &/*color*/) override {}

  void reportErrorWarning(const char */*warningString*/) override {}

  void draw3dText(const btVector3 &/*location*/,
                  const char */*textString*/) override {}

  void setDebugMode(int debugMode) override {
    mDebugMode = debugMode;
  }

  int getDebugMode() const override {
    return mDebugMode;
  }

  void clearLines() override;

  void build();
  void enable(bool enable);
  bool isEnabled();
  ManualObject *getObject();

 private:
  struct Vertex {
    Vector3 pos{};
    ColourValue col{};
    explicit Vertex(Vector3 pPos, ColourValue pCol) : pos(pPos), col(pCol) {}
  };
  ManualObject *mObject{};
  std::vector<Vertex> mVertices{};
  bool mEnabled{true};
  int mDebugMode{btIDebugDraw::DBG_DrawWireframe};
};

} // namespace Ogre

#endif // OPENOBL_OGRE_BULLET_DEBUG_DRAWER_HPP
