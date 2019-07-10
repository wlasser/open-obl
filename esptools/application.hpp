#ifndef OPENOBL_ESPTOOLS_APPLICATION_HPP
#define OPENOBL_ESPTOOLS_APPLICATION_HPP

#include "application_base/application_base.hpp"
#include "esp/esp_coordinator.hpp"
#include "ogre/window.hpp"
#include "ogreimgui/imgui_manager.hpp"
#include "sdl/sdl.hpp"
#include "util/settings.hpp"
#include <OgreCamera.h>
#include <OgreFrameListener.h>
#include <OgreLogManager.h>
#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>

namespace oo {

class Application : Ogre::FrameListener {
 private:
  Ogre::LogManager mLogMgr{};
  std::unique_ptr<Ogre::LogListener> mLogListener{};
  std::unique_ptr<Ogre::GL3PlusPlugin> mGl3PlusPlugin{};
  std::unique_ptr<Ogre::Root> mOgreRoot{};
  std::unique_ptr<sdl::Init> mSdlInit{};
  oo::Window mWindows{};

  std::unique_ptr<Ogre::ImGuiManager> mImguiMgr{};

  std::unique_ptr<oo::EspCoordinator> mEspCoordinator{};

  Ogre::SceneManager *mScnMgr{};
  Ogre::Camera *mCamera{};

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
