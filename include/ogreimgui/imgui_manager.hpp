#ifndef OPENOBLIVION_OGREIMGUI_IMGUI_MANAGER_HPP
#define OPENOBLIVION_OGREIMGUI_IMGUI_MANAGER_HPP

#include "ogreimgui/imgui_renderable.hpp"
#include "sdl/sdl.hpp"
#include <imgui/imgui.h>
#include <OgreRenderQueueListener.h>
#include <OgreRenderTarget.h>
#include <OgreSingleton.h>
#include <OgreResourceGroupManager.h>

namespace Ogre {

class ImGuiManager : public RenderQueueListener,
                     public Singleton<ImGuiManager> {
 public:
  explicit ImGuiManager();
  ~ImGuiManager() override;

  static ImGuiManager &getSingleton();
  static ImGuiManager *getSingletonPtr();

  void renderQueueEnded(uint8 queueGroupId,
                        const String &invocation,
                        bool &repeatThisInvocation) override;

  bool handleEvent(const sdl::Event &event);
  void newFrame(float elapsed);

 private:
  const String mFontTextureName{"__ImGuiFont"};
  const String mMaterialName{"__ImGuiMaterial"};
  const String mResourceGroup{"OOResource"};
  ImGuiRenderable mRenderable{};

  void setKeyMap();
  void createFontTexture();
  void removeFontTexture() noexcept;
  void createMaterial();
  void removeMaterial() noexcept;

  void render(Viewport *viewport, RenderQueue *queue);

  bool handleEvent(const sdl::MouseMotionEvent &event);
  bool handleEvent(const sdl::MouseButtonEvent &event, bool isDown);
  bool handleEvent(const sdl::MouseWheelEvent &event);
  bool handleEvent(const sdl::TextInputEvent &event);
  bool handleEvent(const sdl::KeyboardEvent &event, bool isDown);

  // TODO: Make this a free function
  int asImGui(sdl::KeyCode keyCode) const noexcept;
};

} // namespace Ogre

#endif // OPENOBLIVION_OGREIMGUI_IMGUI_MANAGER_HPP
