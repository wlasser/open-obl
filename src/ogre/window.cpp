#include "ogre/window.hpp"

namespace Ogre {

RenderWindowPtr makeRenderWindow(Root *root,
                                 const String &windowName,
                                 unsigned int width,
                                 unsigned int height,
                                 std::map<std::string, std::string> *params) {
  auto win = root->createRenderWindow(windowName, width, height, false, params);
  if (win == nullptr) {
    OGRE_EXCEPT(Exception::ERR_RENDERINGAPI_ERROR,
                "Failed to create Ogre::RenderWindow",
                "makeOgreWindow");
  }

  return {win, [root](auto *target) { root->destroyRenderTarget(target); }};
}

} // namespace Ogre
