#ifndef OPENOBLIVION_OGREIMGUI_IMGUI_RENDERABLE_HPP
#define OPENOBLIVION_OGREIMGUI_IMGUI_RENDERABLE_HPP

#include <imgui/imgui.h>
#include <OgreRenderable.h>
#include <OgreRenderOperation.h>

namespace Ogre {

class ImGuiRenderable : public Renderable {
 public:
  ImGuiRenderable();
  ~ImGuiRenderable() override = default;

  bool getCastsShadows() const override {
    return false;
  }

  const LightList &getLights() const override {
    static LightList list{};
    return list;
  }

  const MaterialPtr &getMaterial() const override {
    return mMaterial;
  }

  void getRenderOperation(RenderOperation &op) override {
    op = mRenderOperation;
  }

  Real getSquaredViewDepth(const Camera */*camera*/) const override {
    return 0;
  }

  void getWorldTransforms(Matrix4 *xform) const override {
    *xform = mXform;
  }

  void generateIndexData(const ImDrawIdx *idxBuf, std::size_t numIndices);
  void generateVertexData(const ImDrawVert *vertBuf, std::size_t numVerts);

  MaterialPtr mMaterial{};

 private:
  /*const*/ std::size_t mBytesPerVertex{};
  Matrix4 mXform{};
  RenderOperation mRenderOperation{};
  std::unique_ptr<IndexData> mIndexData{};
  std::unique_ptr<VertexData> mVertexData{};
};

} // namespace Ogre

#endif // OPENOBLIVION_OGREIMGUI_IMGUI_RENDERABLE_HPP
