#ifndef OPENOBL_OGRE_WINDOW_HPP
#define OPENOBL_OGRE_WINDOW_HPP

#include <OgrePrerequisites.h>
#include <OgreRenderWindow.h>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>

namespace Ogre {

using RenderWindowPtr = std::unique_ptr<RenderWindow,
                                        std::function<void(RenderWindow * )>>;

RenderWindowPtr makeRenderWindow(Root *root,
                                 const String &windowName,
                                 unsigned int width,
                                 unsigned int height,
                                 const std::map<std::string,
                                                std::string> *params);

} // namespace Ogre

#endif // OPENOBL_OGRE_WINDOW_HPP
