#ifndef OPENOBL_ESPTOOLS_APPLICATION_HPP
#define OPENOBL_ESPTOOLS_APPLICATION_HPP

#include "esp/esp_coordinator.hpp"
#include "ogre/window.hpp"
#include "ogreimgui/imgui_manager.hpp"
#include "sdl/sdl.hpp"
#include "util/settings.hpp"
#include <OgreCamera.h>
#include <OgreFrameListener.h>
#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>
#include <RenderSystems/GL3Plus/OgreGL3PlusPlugin.h>

namespace oo {

class Application : Ogre::FrameListener {
 private:
  std::unique_ptr<Ogre::GL3PlusPlugin> mGl3PlusPlugin{};

  std::unique_ptr<Ogre::Root> mOgreRoot{};
  std::unique_ptr<sdl::Init> mSdlInit{};
  std::tuple<sdl::WindowPtr, Ogre::RenderWindowPtr> mWindows{
      std::make_tuple<sdl::WindowPtr, Ogre::RenderWindowPtr>(
          {nullptr, nullptr}, nullptr
      )};

  std::unique_ptr<Ogre::ImGuiManager> mImguiMgr{};

  std::unique_ptr<oo::EspCoordinator> mEspCoordinator{};

  Ogre::SceneManager *mScnMgr{};
  Ogre::Camera *mCamera{};

  std::tuple<std::unique_ptr<Ogre::Root>, std::unique_ptr<Ogre::GL3PlusPlugin>>
  createOgreRoot();

  std::tuple<sdl::WindowPtr, Ogre::RenderWindowPtr>
  createWindow(const std::string &windowName);

  void quit() { mOgreRoot->queueEndRendering(); }

  void pollEvents();

 public:
  explicit Application(const std::string &windowName);

  Ogre::Root *getRoot() { return mOgreRoot.get(); }

  bool frameStarted(const Ogre::FrameEvent &event) override;

  bool frameRenderingQueued(const Ogre::FrameEvent &event) override {
    return true;
  }

  bool frameEnded(const Ogre::FrameEvent &event) override {
    return true;
  }
};

} // namespace oo

#endif // OPENOBL_ESPTOOLS_APPLICATION_HPP
