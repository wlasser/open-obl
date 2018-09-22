#ifndef OPENOBLIVION_ENGINE_WINDOW_HPP
#define OPENOBLIVION_ENGINE_WINDOW_HPP

#include <Ogre.h>
#include <functional>
#include <map>
#include <memory>

namespace Ogre {

using RenderWindowPtr = std::unique_ptr<RenderWindow,
                                        std::function<void(RenderWindow *)>>;

RenderWindowPtr makeRenderWindow(Root *root,
                                 const String &windowName,
                                 unsigned int width,
                                 unsigned int height,
                                 std::map<std::string, std::string> *params);

} // namespace Ogre

#endif // OPENOBLIVION_ENGINE_WINDOW_HPP
