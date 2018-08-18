#ifndef OPENOBLIVION_APP_HPP
#define OPENOBLIVION_APP_HPP

#include "engine/bsa.hpp"
#include <Ogre.h>
#include <Bites/OgreApplicationContext.h>
#include <Bites/OgreInput.h>

class App : public OgreBites::ApplicationContext,
            public OgreBites::InputListener {
 private:
  std::unique_ptr<engine::BSAArchiveFactory> bsaArchiveFactory;
 public:
  App() : OgreBites::ApplicationContext(Ogre::String("Open Oblivion")),
          OgreBites::InputListener(),
          bsaArchiveFactory(new engine::BSAArchiveFactory) {}

  void setup() override;

  bool keyPressed(const OgreBites::KeyboardEvent &event) override;
};

#endif // OPENOBLIVION_APP_HPP
