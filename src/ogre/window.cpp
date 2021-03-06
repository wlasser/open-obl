#include "ogre/window.hpp"
#include <OgreException.h>
#include <OgreRoot.h>

namespace Ogre {

RenderWindowPtr makeRenderWindow(Root *root,
                                 const String &windowName,
                                 unsigned int width,
                                 unsigned int height,
                                 const std::map<std::string,
                                                std::string> *params) {
  auto win{root->createRenderWindow(windowName, width, height, false, params)};
  if (win == nullptr) {
    OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                "Failed to create Ogre::RenderWindow",
                "makeOgreWindow");
  }

  return {win, [root](auto *target) { root->destroyRenderTarget(target); }};
}

} // namespace Ogre
