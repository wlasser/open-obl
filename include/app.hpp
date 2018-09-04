#ifndef OPENOBLIVION_APP_HPP
#define OPENOBLIVION_APP_HPP

#include "engine/bsa.hpp"
#include <Ogre.h>
#include <Bites/OgreApplicationContext.h>
#include <Bites/OgreInput.h>
#include <OgreCameraMan.h>

class App : public OgreBites::ApplicationContext,
            public OgreBites::InputListener {
 private:
  std::unique_ptr<engine::BSAArchiveFactory> bsaArchiveFactory;
  Ogre::SceneNode *cameraNode{};
  std::unique_ptr<OgreBites::CameraMan> cameraMan{};
 public:
  App() : OgreBites::ApplicationContext(Ogre::String("Open Oblivion")),
          OgreBites::InputListener(),
          bsaArchiveFactory(new engine::BSAArchiveFactory) {}

  void setup() override;

  bool keyPressed(const OgreBites::KeyboardEvent &event) override;
  bool keyReleased(const OgreBites::KeyboardEvent &event) override;
  void frameRendered(const Ogre::FrameEvent &event) override;
  bool mouseMoved(const OgreBites::MouseMotionEvent &event) override;
  bool mousePressed(const OgreBites::MouseButtonEvent &event) override;
  bool mouseReleased(const OgreBites::MouseButtonEvent &event) override;
};

#endif // OPENOBLIVION_APP_HPP
