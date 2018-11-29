#ifndef OPENOBLIVION_SDL_SDL_HPP
#define OPENOBLIVION_SDL_SDL_HPP

#include <boost/format.hpp>
#include "SDL.h"
#include "SDL_syswm.h"
#include <stdexcept>
#include <string>
#include <type_traits>

// WTF X.h
#undef InputFocus
#undef None
#undef Bool
#undef DestroyAll
#undef True
#undef False

namespace sdl {

// TODO: Use boost::exception to store the error code and propagate it up
class SDLException : public std::runtime_error {
 public:
  explicit SDLException(const std::string &functionName)
      : std::runtime_error(boost::str(
      boost::format("%s failed: %s") % functionName % SDL_GetError())) {}
};

// RAII wrapper for SDL, calls SDL_Init on construction and SDL_Quit on
// destruction for automatic cleanup.
class Init {
 public:
  Init() {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
      throw SDLException("SDL_Init");
    }
  }

  ~Init() {
    SDL_Quit();
  }
};

enum class WindowFlags : std::underlying_type_t<SDL_WindowFlags> {
  Fullscreen = SDL_WINDOW_FULLSCREEN,
  OpenGL = SDL_WINDOW_OPENGL,
  Shown = SDL_WINDOW_SHOWN,
  Hidden = SDL_WINDOW_HIDDEN,
  Borderless = SDL_WINDOW_BORDERLESS,
  Resizable = SDL_WINDOW_RESIZABLE,
  Minimized = SDL_WINDOW_MINIMIZED,
  Maximized = SDL_WINDOW_MAXIMIZED,
  InputGrabbed = SDL_WINDOW_INPUT_GRABBED,
  InputFocus = SDL_WINDOW_INPUT_FOCUS,
  MouseFocus = SDL_WINDOW_MOUSE_FOCUS,
  FullscreenDesktop = SDL_WINDOW_FULLSCREEN_DESKTOP,
  Foreign = SDL_WINDOW_FOREIGN,
  HighDPI = SDL_WINDOW_ALLOW_HIGHDPI,
  MouseCapture = SDL_WINDOW_MOUSE_CAPTURE,
  AlwaysOnTop = SDL_WINDOW_ALWAYS_ON_TOP,
  SkipTaskbar = SDL_WINDOW_SKIP_TASKBAR,
  Utility = SDL_WINDOW_UTILITY,
  Tooltip = SDL_WINDOW_TOOLTIP,
  PopupMenu = SDL_WINDOW_POPUP_MENU,
  Vulkan = SDL_WINDOW_VULKAN
};
inline constexpr WindowFlags operator|(WindowFlags a, WindowFlags b) {
  return WindowFlags{static_cast<uint32_t>(a) | static_cast<uint32_t>(b)};
}
inline constexpr WindowFlags &operator|=(WindowFlags &a, WindowFlags b) {
  a = a | b;
  return a;
}

enum class EventType : uint32_t {
  Quit = SDL_QUIT,
  AppTerminating = SDL_APP_TERMINATING,
  AppLowMemory = SDL_APP_LOWMEMORY,
  AppWillEnterBackground = SDL_APP_WILLENTERBACKGROUND,
  AppDidEnterBackground = SDL_APP_DIDENTERBACKGROUND,
  AppWillEnterForeground = SDL_APP_WILLENTERFOREGROUND,
  AppDidEnterForeground = SDL_APP_DIDENTERFOREGROUND,
  WindowEvent = SDL_WINDOWEVENT,
  SysWMEvent = SDL_SYSWMEVENT,
  KeyDown = SDL_KEYDOWN,
  KeyUp = SDL_KEYUP,
  TextEditing = SDL_TEXTEDITING,
  TextInput = SDL_TEXTINPUT,
  KeymapChanged = SDL_KEYMAPCHANGED,
  MouseMotion = SDL_MOUSEMOTION,
  MouseButtonDown = SDL_MOUSEBUTTONDOWN,
  MouseButtonUp = SDL_MOUSEBUTTONUP,
  MouseWheel = SDL_MOUSEWHEEL,
  JoyAxisMotion = SDL_JOYAXISMOTION,
  JoyBallMotion = SDL_JOYBALLMOTION,
  JoyHatMotion = SDL_JOYHATMOTION,
  JoyButtonDown = SDL_JOYBUTTONDOWN,
  JoyButtonUp = SDL_JOYBUTTONUP,
  JoyDeviceAdded = SDL_JOYDEVICEADDED,
  JoyDeviceRemoved = SDL_JOYDEVICEREMOVED,
  ControllerAxisMotion = SDL_CONTROLLERAXISMOTION,
  ControllerButtonDown = SDL_CONTROLLERBUTTONDOWN,
  ControllerButtonUp = SDL_CONTROLLERBUTTONUP,
  ControllerDeviceAdded = SDL_CONTROLLERDEVICEADDED,
  ControllerDeviceRemoved = SDL_CONTROLLERDEVICEREMOVED,
  ControllerDeviceRemapped = SDL_CONTROLLERDEVICEREMAPPED,
  FingerDown = SDL_FINGERDOWN,
  FingerUp = SDL_FINGERUP,
  FingerMotion = SDL_FINGERMOTION,
  DollarGesture = SDL_DOLLARGESTURE,
  DollarRecord = SDL_DOLLARRECORD,
  MultiGesture = SDL_MULTIGESTURE,
  ClipboardUpdate = SDL_CLIPBOARDUPDATE,
  DropFile = SDL_DROPFILE,
  DropText = SDL_DROPTEXT,
  DropBegin = SDL_DROPBEGIN,
  DropComplete = SDL_DROPCOMPLETE,
  AudioDeviceAdded = SDL_AUDIODEVICEADDED,
  AudioDeviceRemoved = SDL_AUDIODEVICEREMOVED,
  RenderTargetsReset = SDL_RENDER_TARGETS_RESET,
  RenderDeviceReset = SDL_RENDER_DEVICE_RESET,
  UserEvent = SDL_USEREVENT,
};

enum class KeyCode : uint32_t {
  Unknown = SDLK_UNKNOWN,
  Return = SDLK_RETURN,
  Escape = SDLK_ESCAPE,
  Backspace = SDLK_BACKSPACE,
  Tab = SDLK_TAB,
  Space = SDLK_SPACE,
  Exclaim = SDLK_EXCLAIM,
  Quotedbl = SDLK_QUOTEDBL,
  Hash = SDLK_HASH,
  Percent = SDLK_PERCENT,
  Dollar = SDLK_DOLLAR,
  Ampersand = SDLK_AMPERSAND,
  Quote = SDLK_QUOTE,
  Leftparen = SDLK_LEFTPAREN,
  Rightparen = SDLK_RIGHTPAREN,
  Asterisk = SDLK_ASTERISK,
  Plus = SDLK_PLUS,
  Comma = SDLK_COMMA,
  Minus = SDLK_MINUS,
  Period = SDLK_PERIOD,
  Slash = SDLK_SLASH,
  N0 = SDLK_0,
  N1 = SDLK_1,
  N2 = SDLK_2,
  N3 = SDLK_3,
  N4 = SDLK_4,
  N5 = SDLK_5,
  N6 = SDLK_6,
  N7 = SDLK_7,
  N8 = SDLK_8,
  N9 = SDLK_9,
  Colon = SDLK_COLON,
  Semicolon = SDLK_SEMICOLON,
  Less = SDLK_LESS,
  Equals = SDLK_EQUALS,
  Greater = SDLK_GREATER,
  Question = SDLK_QUESTION,
  At = SDLK_AT,
  Leftbracket = SDLK_LEFTBRACKET,
  Backslash = SDLK_BACKSLASH,
  Rightbracket = SDLK_RIGHTBRACKET,
  Caret = SDLK_CARET,
  Underscore = SDLK_UNDERSCORE,
  Backquote = SDLK_BACKQUOTE,
  A = SDLK_a,
  B = SDLK_b,
  C = SDLK_c,
  D = SDLK_d,
  E = SDLK_e,
  F = SDLK_f,
  G = SDLK_g,
  H = SDLK_h,
  I = SDLK_i,
  J = SDLK_j,
  K = SDLK_k,
  L = SDLK_l,
  M = SDLK_m,
  N = SDLK_n,
  O = SDLK_o,
  P = SDLK_p,
  Q = SDLK_q,
  R = SDLK_r,
  S = SDLK_s,
  T = SDLK_t,
  U = SDLK_u,
  V = SDLK_v,
  W = SDLK_w,
  X = SDLK_x,
  Y = SDLK_y,
  Z = SDLK_z,
  Capslock = SDLK_CAPSLOCK,
  F1 = SDLK_F1,
  F2 = SDLK_F2,
  F3 = SDLK_F3,
  F4 = SDLK_F4,
  F5 = SDLK_F5,
  F6 = SDLK_F6,
  F7 = SDLK_F7,
  F8 = SDLK_F8,
  F9 = SDLK_F9,
  F10 = SDLK_F10,
  F11 = SDLK_F11,
  F12 = SDLK_F12,
  Printscreen = SDLK_PRINTSCREEN,
  Scrolllock = SDLK_SCROLLLOCK,
  Pause = SDLK_PAUSE,
  Insert = SDLK_INSERT,
  Home = SDLK_HOME,
  Pageup = SDLK_PAGEUP,
  Delete = SDLK_DELETE,
  End = SDLK_END,
  Pagedown = SDLK_PAGEDOWN,
  Right = SDLK_RIGHT,
  Left = SDLK_LEFT,
  Down = SDLK_DOWN,
  Up = SDLK_UP,
  Numlockclear = SDLK_NUMLOCKCLEAR,
  Kp_divide = SDLK_KP_DIVIDE,
  Kp_multiply = SDLK_KP_MULTIPLY,
  Kp_minus = SDLK_KP_MINUS,
  Kp_plus = SDLK_KP_PLUS,
  Kp_enter = SDLK_KP_ENTER,
  Kp_1 = SDLK_KP_1,
  Kp_2 = SDLK_KP_2,
  Kp_3 = SDLK_KP_3,
  Kp_4 = SDLK_KP_4,
  Kp_5 = SDLK_KP_5,
  Kp_6 = SDLK_KP_6,
  Kp_7 = SDLK_KP_7,
  Kp_8 = SDLK_KP_8,
  Kp_9 = SDLK_KP_9,
  Kp_0 = SDLK_KP_0,
  Kp_period = SDLK_KP_PERIOD,
  Application = SDLK_APPLICATION,
  Power = SDLK_POWER,
  Kp_equals = SDLK_KP_EQUALS,
  F13 = SDLK_F13,
  F14 = SDLK_F14,
  F15 = SDLK_F15,
  F16 = SDLK_F16,
  F17 = SDLK_F17,
  F18 = SDLK_F18,
  F19 = SDLK_F19,
  F20 = SDLK_F20,
  F21 = SDLK_F21,
  F22 = SDLK_F22,
  F23 = SDLK_F23,
  F24 = SDLK_F24,
  Execute = SDLK_EXECUTE,
  Help = SDLK_HELP,
  Menu = SDLK_MENU,
  Select = SDLK_SELECT,
  Stop = SDLK_STOP,
  Again = SDLK_AGAIN,
  Undo = SDLK_UNDO,
  Cut = SDLK_CUT,
  Copy = SDLK_COPY,
  Paste = SDLK_PASTE,
  Find = SDLK_FIND,
  Mute = SDLK_MUTE,
  Volumeup = SDLK_VOLUMEUP,
  Volumedown = SDLK_VOLUMEDOWN,
  Kp_comma = SDLK_KP_COMMA,
  Kp_equalsas400 = SDLK_KP_EQUALSAS400,
  Alterase = SDLK_ALTERASE,
  Sysreq = SDLK_SYSREQ,
  Cancel = SDLK_CANCEL,
  Clear = SDLK_CLEAR,
  Prior = SDLK_PRIOR,
  Return2 = SDLK_RETURN2,
  Separator = SDLK_SEPARATOR,
  Out = SDLK_OUT,
  Oper = SDLK_OPER,
  Clearagain = SDLK_CLEARAGAIN,
  Crsel = SDLK_CRSEL,
  Exsel = SDLK_EXSEL,
  Kp_00 = SDLK_KP_00,
  Kp_000 = SDLK_KP_000,
  Thousandsseparator = SDLK_THOUSANDSSEPARATOR,
  Decimalseparator = SDLK_DECIMALSEPARATOR,
  Currencyunit = SDLK_CURRENCYUNIT,
  Currencysubunit = SDLK_CURRENCYSUBUNIT,
  Kp_leftparen = SDLK_KP_LEFTPAREN,
  Kp_rightparen = SDLK_KP_RIGHTPAREN,
  Kp_leftbrace = SDLK_KP_LEFTBRACE,
  Kp_rightbrace = SDLK_KP_RIGHTBRACE,
  Kp_tab = SDLK_KP_TAB,
  Kp_backspace = SDLK_KP_BACKSPACE,
  Kp_a = SDLK_KP_A,
  Kp_b = SDLK_KP_B,
  Kp_c = SDLK_KP_C,
  Kp_d = SDLK_KP_D,
  Kp_e = SDLK_KP_E,
  Kp_f = SDLK_KP_F,
  Kp_xor = SDLK_KP_XOR,
  Kp_power = SDLK_KP_POWER,
  Kp_percent = SDLK_KP_PERCENT,
  Kp_less = SDLK_KP_LESS,
  Kp_greater = SDLK_KP_GREATER,
  Kp_ampersand = SDLK_KP_AMPERSAND,
  Kp_dblampersand = SDLK_KP_DBLAMPERSAND,
  Kp_verticalbar = SDLK_KP_VERTICALBAR,
  Kp_dblverticalbar = SDLK_KP_DBLVERTICALBAR,
  Kp_colon = SDLK_KP_COLON,
  Kp_hash = SDLK_KP_HASH,
  Kp_space = SDLK_KP_SPACE,
  Kp_at = SDLK_KP_AT,
  Kp_exclam = SDLK_KP_EXCLAM,
  Kp_memstore = SDLK_KP_MEMSTORE,
  Kp_memrecall = SDLK_KP_MEMRECALL,
  Kp_memclear = SDLK_KP_MEMCLEAR,
  Kp_memadd = SDLK_KP_MEMADD,
  Kp_memsubtract = SDLK_KP_MEMSUBTRACT,
  Kp_memmultiply = SDLK_KP_MEMMULTIPLY,
  Kp_memdivide = SDLK_KP_MEMDIVIDE,
  Kp_plusminus = SDLK_KP_PLUSMINUS,
  Kp_clear = SDLK_KP_CLEAR,
  Kp_clearentry = SDLK_KP_CLEARENTRY,
  Kp_binary = SDLK_KP_BINARY,
  Kp_octal = SDLK_KP_OCTAL,
  Kp_decimal = SDLK_KP_DECIMAL,
  Kp_hexadecimal = SDLK_KP_HEXADECIMAL,
  Lctrl = SDLK_LCTRL,
  Lshift = SDLK_LSHIFT,
  Lalt = SDLK_LALT,
  Lgui = SDLK_LGUI,
  Rctrl = SDLK_RCTRL,
  Rshift = SDLK_RSHIFT,
  Ralt = SDLK_RALT,
  Rgui = SDLK_RGUI,
  Mode = SDLK_MODE,
  Audionext = SDLK_AUDIONEXT,
  Audioprev = SDLK_AUDIOPREV,
  Audiostop = SDLK_AUDIOSTOP,
  Audioplay = SDLK_AUDIOPLAY,
  Audiomute = SDLK_AUDIOMUTE,
  Mediaselect = SDLK_MEDIASELECT,
  Www = SDLK_WWW,
  Mail = SDLK_MAIL,
  Calculator = SDLK_CALCULATOR,
  Computer = SDLK_COMPUTER,
  Ac_search = SDLK_AC_SEARCH,
  Ac_home = SDLK_AC_HOME,
  Ac_back = SDLK_AC_BACK,
  Ac_forward = SDLK_AC_FORWARD,
  Ac_stop = SDLK_AC_STOP,
  Ac_refresh = SDLK_AC_REFRESH,
  Ac_bookmarks = SDLK_AC_BOOKMARKS,
  Brightnessdown = SDLK_BRIGHTNESSDOWN,
  Brightnessup = SDLK_BRIGHTNESSUP,
  Displayswitch = SDLK_DISPLAYSWITCH,
  Kbdillumtoggle = SDLK_KBDILLUMTOGGLE,
  Kbdillumdown = SDLK_KBDILLUMDOWN,
  Kbdillumup = SDLK_KBDILLUMUP,
  Eject = SDLK_EJECT,
  Sleep = SDLK_SLEEP,
  App1 = SDLK_APP1,
  App2 = SDLK_APP2,
  Audiorewind = SDLK_AUDIOREWIND,
  Audiofastforward = SDLK_AUDIOFASTFORWARD,
};

enum class MouseButton : uint8_t {
  Left = SDL_BUTTON_LEFT,
  Middle = SDL_BUTTON_MIDDLE,
  Right = SDL_BUTTON_RIGHT,
  Extra1 = SDL_BUTTON_X1,
  Extra2 = SDL_BUTTON_X2
};

enum class WindowEventType : std::underlying_type_t<SDL_WindowEventID> {
  None = SDL_WINDOWEVENT_NONE,
  Shown = SDL_WINDOWEVENT_SHOWN,
  Hidden = SDL_WINDOWEVENT_HIDDEN,
  Exposed = SDL_WINDOWEVENT_EXPOSED,
  Moved = SDL_WINDOWEVENT_MOVED,
  Resized = SDL_WINDOWEVENT_RESIZED,
  SizeChanged = SDL_WINDOWEVENT_SIZE_CHANGED,
  Minimized = SDL_WINDOWEVENT_MINIMIZED,
  Maximized = SDL_WINDOWEVENT_MAXIMIZED,
  Restored = SDL_WINDOWEVENT_RESTORED,
  Enter = SDL_WINDOWEVENT_ENTER,
  Leave = SDL_WINDOWEVENT_LEAVE,
  FocusGained = SDL_WINDOWEVENT_FOCUS_GAINED,
  FocusLost = SDL_WINDOWEVENT_FOCUS_LOST,
  Close = SDL_WINDOWEVENT_CLOSE,
  TakeFocus = SDL_WINDOWEVENT_TAKE_FOCUS,
  HitTest = SDL_WINDOWEVENT_HIT_TEST,
};

// Type aliases to move into the namespace
using Window = SDL_Window;
using Event = SDL_Event;
using WindowEvent = SDL_WindowEvent;
using KeyboardEvent = SDL_KeyboardEvent;
using MouseMotionEvent = SDL_MouseMotionEvent;
using MouseButtonEvent = SDL_MouseButtonEvent;
using MouseWheelEvent = SDL_MouseWheelEvent;
using TextInputEvent = SDL_TextInputEvent;
using SysWMInfo = SDL_SysWMinfo; // Note the capitalization change

// Resources are wrapped in unique_ptrs with custom deleters for automatic
// cleanup.
using WindowPtr = std::unique_ptr<Window, decltype(&SDL_DestroyWindow)>;

WindowPtr makeWindow(const std::string &windowName, int width, int height,
                     WindowFlags flags);
SysWMInfo getSysWMInfo(Window *window);
std::string getWindowParent(const SysWMInfo &windowInfo);
void setRelativeMouseMode(bool on);
bool pollEvent(Event &event);
EventType typeOf(const Event &event);
WindowEventType typeOf(const WindowEvent &event);
KeyCode keyCodeOf(const KeyboardEvent &event);
MouseButton mouseButtonOf(const MouseButtonEvent &event);

} // namespace sdl

#endif // OPENOBLIVION_SDL_SDL_HPP
