#ifndef OPENOBLIVION_APP_HPP
#define OPENOBLIVION_APP_HPP

#include <Ogre.h>
#include <Bites/OgreApplicationContext.h>
#include <Bites/OgreInput.h>

class App : public OgreBites::ApplicationContext,
            public OgreBites::InputListener {
 public:
  App() : OgreBites::ApplicationContext(Ogre::String("Open Oblivion")),
          OgreBites::InputListener() {}

  void setup() override;

  bool keyPressed(const OgreBites::KeyboardEvent &event) override;
};

#endif // OPENOBLIVION_APP_HPP
