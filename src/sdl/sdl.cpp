#include "sdl/sdl.hpp"
#include <boost/format.hpp>
#include "SDL.h"

namespace sdl {

WindowPtr makeWindow(const std::string &windowName, int width, int height,
                     WindowFlags flags) {
  auto win = SDL_CreateWindow(windowName.c_str(),
                              SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                              width, height,
                              static_cast<SDL_WindowFlags>(flags));
  if (!win) throw SDLException("SDL_CreateWindow");

  return WindowPtr(win, SDL_DestroyWindow);
}

SDL_SysWMinfo getSysWMInfo(SDL_Window *window) {
  SDL_SysWMinfo info{};
  // Tell SDL we have version 2, otherwise the next call fails regardless of the
  // actual SDL version.
  SDL_VERSION(&info.version);
  if (!SDL_GetWindowWMInfo(window, &info)) {
    throw SDLException("SDL_GetWindowWMInfo");
  }
  return info;
}

std::string getWindowParent(const SDL_SysWMinfo &windowInfo) {
  // TODO: Support other window managers
  return std::to_string(windowInfo.info.x11.window);
}

void setRelativeMouseMode(bool on) {
  if (SDL_SetRelativeMouseMode(on ? SDL_TRUE : SDL_FALSE)) {
    throw SDLException("SDL_SetRelativeMouseMode");
  }
}

bool pollEvent(Event &event) {
  return SDL_PollEvent(&event) != 0;
}

EventType typeOf(const Event &event) {
  return EventType(event.type);
}

WindowEventType typeOf(const WindowEvent &event) {
  return WindowEventType(event.event);
}

KeyCode keyCodeOf(const KeyboardEvent &event) {
  return KeyCode(event.keysym.sym);
}

MouseButton mouseButtonof(const MouseButtonEvent &event) {
  return MouseButton(event.button);
}

} // namespace sdl
