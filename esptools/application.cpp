#include "application.hpp"

namespace oo {

std::tuple<std::unique_ptr<Ogre::Root>, std::unique_ptr<Ogre::GL3PlusPlugin>>
Application::createOgreRoot() {
  auto root{std::make_unique<Ogre::Root>("", "", "")};
  auto gl3PlusPlugin{std::make_unique<Ogre::GL3PlusPlugin>()};
  root->installPlugin(gl3PlusPlugin.get());

  constexpr const char *subsysName{"OpenGL 3+ Rendering Subsystem"};
  if (auto *renderSystem{root->getRenderSystemByName(subsysName)}) {
    root->setRenderSystem(renderSystem);
  } else {
    throw std::runtime_error("Invalid render system");
  }

  root->initialise(false);
  root->addFrameListener(this);

  return {std::move(root), std::move(gl3PlusPlugin)};
}

std::tuple<sdl::WindowPtr, Ogre::RenderWindowPtr>
Application::createWindow(const std::string &windowName) {
  auto sdlWindow{sdl::makeWindow(windowName, 1024, 768, sdl::WindowFlags{})};
  const auto sdlWindowInfo{sdl::getSysWMInfo(sdlWindow.get())};
  const auto parent{sdl::getWindowParent(sdlWindowInfo)};

  const std::map<std::string, std::string> params{
      {"parentWindowHandle", parent}
  };
  auto ogreWindow{Ogre::makeRenderWindow(mOgreRoot.get(),
                                         windowName,
                                         1024u, 768u,
                                         &params)};
  return {std::move(sdlWindow), std::move(ogreWindow)};
}

void Application::pollEvents() {
  sdl::Event sdlEvent;
  while (sdl::pollEvent(sdlEvent)) {
    if (sdl::typeOf(sdlEvent) == sdl::EventType::Quit) {
      quit();
      return;
    }

    mImguiMgr->handleEvent(sdlEvent);
    auto imguiIo{ImGui::GetIO()};
    if (imguiIo.WantCaptureKeyboard
        && sdl::isKeyboardEvent(sdlEvent))
      continue;
    if (imguiIo.WantCaptureMouse && sdl::isMouseEvent(sdlEvent)) continue;
  }
}

Application::Application(const std::string &windowName)
    : Ogre::FrameListener() {
  auto &&[ogreRoot, renderPlugin]{createOgreRoot()};
  mOgreRoot = std::move(ogreRoot);
  mGl3PlusPlugin = std::move(renderPlugin);

  mSdlInit = std::make_unique<sdl::Init>();
  mWindows = createWindow(windowName);

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

  auto &windowPtr{std::get<Ogre::RenderWindowPtr>(mWindows)};
  if (windowPtr->hasViewportWithZOrder(0)) windowPtr->removeViewport(0);
  windowPtr->addViewport(mCamera);
  mOgreRoot->getRenderSystem()->_setViewport(mCamera->getViewport());
}

bool Application::frameStarted(const Ogre::FrameEvent &event) {
  pollEvents();

  mImguiMgr->newFrame(event.timeSinceLastFrame);
  ImGui::ShowDemoWindow();

  return true;
}

} // namespace oo