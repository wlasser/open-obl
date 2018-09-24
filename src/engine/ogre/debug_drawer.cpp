#include "engine/ogre/debug_drawer.hpp"

namespace Ogre {

DebugDrawer::DebugDrawer(SceneManager *mgr, const String &group) {
  mObject = mgr->createManualObject();
  mObject->setDynamic(true);
  mObject->begin("DebugDrawer", RenderOperation::OT_LINE_LIST, group);
  mObject->position(Vector3::ZERO);
  mObject->colour(ColourValue::Black);
  mObject->index(0);
  mObject->end();
}

void DebugDrawer::drawLine(const Vector3 &from,
                           const Vector3 &to,
                           const ColourValue &colour) {
  mVertices.push_back({from, colour});
  mVertices.push_back({to, colour});
}

void DebugDrawer::drawLine(const btVector3 &from,
                           const btVector3 &to,
                           const btVector3 &colour) {
  drawLine(Vector3{from.x(), from.y(), from.z()},
           Vector3{to.x(), to.y(), to.z()},
           ColourValue{colour.x(), colour.y(), colour.z()});
}

void DebugDrawer::clearLines() {
  mVertices.clear();
}

void DebugDrawer::build() {
  if (!mEnabled) return;
  mObject->beginUpdate(0);
  mObject->estimateVertexCount(mVertices.size());
  for (const auto &vertex : mVertices) {
    mObject->position(vertex.pos);
    mObject->colour(vertex.col);
  }
  mObject->end();
}

bool DebugDrawer::isEnabled() {
  return mEnabled;
}

void DebugDrawer::enable(bool enable) {
  mEnabled = enable;
}

ManualObject *DebugDrawer::getObject() {
  return mObject;
}

} // namespace Ogre