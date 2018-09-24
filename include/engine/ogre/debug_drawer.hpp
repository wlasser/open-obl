#ifndef OPENOBLIVION_ENGINE_DEBUG_DRAWER_HPP
#define OPENOBLIVION_ENGINE_DEBUG_DRAWER_HPP

#include <btBulletDynamicsCommon.h>
#include <Ogre.h>

namespace Ogre {

class DebugDrawer : public btIDebugDraw {
 public:
  explicit DebugDrawer(SceneManager *mgr, const String &group);

  void drawLine(const Vector3 &from,
                const Vector3 &to,
                const ColourValue &colour);

  void drawLine(const btVector3 &from,
                const btVector3 &to,
                const btVector3 &colour) override;

  void drawContactPoint(const btVector3 &pointOnB,
                        const btVector3 &normalOnB,
                        btScalar distance,
                        int lifetime,
                        const btVector3 &color) override {}

  void reportErrorWarning(const char *warningString) override {}

  void draw3dText(const btVector3 &location, const char *textString) override {}

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
  };
  ManualObject *mObject{};
  std::vector<Vertex> mVertices{};
  bool mEnabled{true};
  int mDebugMode{btIDebugDraw::DBG_DrawWireframe};
};

} // namespace Ogre

#endif // OPENOBLIVION_DEBUG_DRAWER_HPP
