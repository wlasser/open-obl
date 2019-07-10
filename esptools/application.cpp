#include "application.hpp"
#include "gui.hpp"
#include "job/job.hpp"

namespace oo {

void Application::pollEvents() {
  sdl::Event sdlEvent;
  while (sdl::pollEvent(sdlEvent)) {
    if (sdl::typeOf(sdlEvent) == sdl::EventType::Quit) {
      quit();
      return;
    }

    mImguiMgr->handleEvent(sdlEvent);
    auto imguiIo{ImGui::GetIO()};
    if (imguiIo.WantCaptureKeyboard && sdl::isKeyboardEvent(sdlEvent)) continue;
    if (imguiIo.WantCaptureMouse && sdl::isMouseEvent(sdlEvent)) continue;
  }
}

Application::Application(const std::string &windowName)
    : Ogre::FrameListener() {
  mLogListener = oo::createLoggers("OpenOBL_Editor.log", mLogMgr);

  mOgreRoot = std::make_unique<Ogre::Root>("", "", "");
  mGl3PlusPlugin = oo::startGl3Plus(mOgreRoot.get());

  mOgreRoot->initialise(false);
  mOgreRoot->addFrameListener(this);

  mSdlInit = std::make_unique<sdl::Init>();
  mWindows = oo::Window(mOgreRoot.get(), 1024, 768, windowName, {});

  auto &resGrpMgr{Ogre::ResourceGroupManager::getSingleton()};
  resGrpMgr.createResourceGroup(oo::RESOURCE_GROUP);
  resGrpMgr.addResourceLocation("./shaders", "FileSystem", oo::SHADER_GROUP);

  resGrpMgr.initialiseResourceGroup(oo::RESOURCE_GROUP, false);
  resGrpMgr.initialiseResourceGroup(oo::SHADER_GROUP, true);

  mImguiMgr = std::make_unique<Ogre::ImGuiManager>();

  mScnMgr = mOgreRoot->createSceneManager("DefaultSceneManager",
                                          "__DummySceneManager");
  mCamera = mScnMgr->createCamera("__DummyCamera");
  mScnMgr->addRenderQueueListener(mImguiMgr.get());

  auto *windowPtr{mWindows.getOgreWindow()};
  if (windowPtr->hasViewportWithZOrder(0)) windowPtr->removeViewport(0);
  windowPtr->addViewport(mCamera);
  mOgreRoot->getRenderSystem()->_setViewport(mCamera->getViewport());
}

bool Application::frameStarted(const Ogre::FrameEvent &event) {
  pollEvents();

  mImguiMgr->newFrame(event.timeSinceLastFrame);
  oo::JobCounter imGuiJob(1);
  oo::JobManager::runJob([]() {
    oo::showMainMenuBar();
    ImGui::ShowDemoWindow();
  }, &imGuiJob);
  imGuiJob.wait();

  return true;
}

} // namespace oo