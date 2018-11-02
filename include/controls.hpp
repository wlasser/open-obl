#ifndef OPENOBLIVION_CONTROLS_HPP
#define OPENOBLIVION_CONTROLS_HPP

#include "sdl/sdl.hpp"
#include "game_settings.hpp"
#include <string_view>
#include <unordered_map>
#include <variant>

namespace event {

struct KeyEvent {
  bool down{true};
};
struct Forward : KeyEvent {};
struct Backward : KeyEvent {};
struct SlideLeft : KeyEvent {};
struct SlideRight : KeyEvent {};
struct Use : KeyEvent {};
struct Activate : KeyEvent {};
struct Block : KeyEvent {};
struct Cast : KeyEvent {};
struct ReadyItem : KeyEvent {};
struct Sneak : KeyEvent {};
struct Run : KeyEvent {};
struct AlwaysRun : KeyEvent {};
struct AutoMove : KeyEvent {};
struct Jump : KeyEvent {};
struct TogglePov : KeyEvent {};
struct MenuMode : KeyEvent {};
struct Rest : KeyEvent {};
struct QuickMenu : KeyEvent {};
struct Quick : KeyEvent {
  int n{};
};
struct QuickSave : KeyEvent {};
struct QuickLoad : KeyEvent {};
struct Grab : KeyEvent {};

using KeyVariant = std::variant<
    Forward, Backward, SlideLeft, SlideRight, Use, Activate, Block, Cast,
    ReadyItem, Sneak, Run, AlwaysRun, AutoMove, Jump, TogglePov,
    MenuMode, Rest, QuickMenu, Quick, QuickSave, QuickLoad, Grab>;

struct MouseEvent {
  float delta{};
};
struct Pitch : MouseEvent {};
struct Yaw : MouseEvent {};

using MouseVariant = std::variant<Pitch, Yaw>;

} // namespace event

constexpr static inline std::array<sdl::KeyCode, 256> directInputKeyMap{
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Escape,
    sdl::KeyCode::N1,
    sdl::KeyCode::N2,
    sdl::KeyCode::N3,
    sdl::KeyCode::N4,
    sdl::KeyCode::N5,
    sdl::KeyCode::N6,
    sdl::KeyCode::N7,
    sdl::KeyCode::N8,
    sdl::KeyCode::N9,
    sdl::KeyCode::N0,
    sdl::KeyCode::Minus,
    sdl::KeyCode::Equals,
    sdl::KeyCode::Backspace,
    sdl::KeyCode::Tab,
    sdl::KeyCode::Q,
    sdl::KeyCode::W,
    sdl::KeyCode::E,
    sdl::KeyCode::R,
    sdl::KeyCode::T,
    sdl::KeyCode::Y,
    sdl::KeyCode::U,
    sdl::KeyCode::I,
    sdl::KeyCode::O,
    sdl::KeyCode::P,
    sdl::KeyCode::Leftbracket,
    sdl::KeyCode::Rightbracket,
    sdl::KeyCode::Return,
    sdl::KeyCode::Lctrl,
    sdl::KeyCode::A,
    sdl::KeyCode::S,
    sdl::KeyCode::D,
    sdl::KeyCode::F,
    sdl::KeyCode::G,
    sdl::KeyCode::H,
    sdl::KeyCode::J,
    sdl::KeyCode::K,
    sdl::KeyCode::L,
    sdl::KeyCode::Semicolon,
    sdl::KeyCode::Quote,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Lshift,
    sdl::KeyCode::Backslash,
    sdl::KeyCode::Z,
    sdl::KeyCode::X,
    sdl::KeyCode::C,
    sdl::KeyCode::V,
    sdl::KeyCode::B,
    sdl::KeyCode::N,
    sdl::KeyCode::M,
    sdl::KeyCode::Comma,
    sdl::KeyCode::Period,
    sdl::KeyCode::Slash,
    sdl::KeyCode::Rshift,
    sdl::KeyCode::Asterisk,
    sdl::KeyCode::Lalt,
    sdl::KeyCode::Space,
    sdl::KeyCode::Capslock,
    sdl::KeyCode::F1,
    sdl::KeyCode::F2,
    sdl::KeyCode::F3,
    sdl::KeyCode::F4,
    sdl::KeyCode::F5,
    sdl::KeyCode::F6,
    sdl::KeyCode::F7,
    sdl::KeyCode::F8,
    sdl::KeyCode::F9,
    sdl::KeyCode::F10,
    sdl::KeyCode::Numlockclear,
    sdl::KeyCode::Scrolllock,
    sdl::KeyCode::Kp_7,
    sdl::KeyCode::Kp_8,
    sdl::KeyCode::Kp_9,
    sdl::KeyCode::Kp_minus,
    sdl::KeyCode::Kp_4,
    sdl::KeyCode::Kp_5,
    sdl::KeyCode::Kp_6,
    sdl::KeyCode::Kp_plus,
    sdl::KeyCode::Kp_1,
    sdl::KeyCode::Kp_2,
    sdl::KeyCode::Kp_3,
    sdl::KeyCode::Kp_0,
    sdl::KeyCode::Kp_period,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::F11,
    sdl::KeyCode::F12,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::F13,
    sdl::KeyCode::F14,
    sdl::KeyCode::F15,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Kp_equals,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Kp_at,
    sdl::KeyCode::Kp_colon,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Stop,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Kp_enter,
    sdl::KeyCode::Rctrl,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Kp_comma,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Kp_divide,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Sysreq,
    sdl::KeyCode::Ralt,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Home,
    sdl::KeyCode::Up,
    sdl::KeyCode::Pageup,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Left,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Right,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::End,
    sdl::KeyCode::Down,
    sdl::KeyCode::Pagedown,
    sdl::KeyCode::Insert,
    sdl::KeyCode::Delete,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Unknown,
    sdl::KeyCode::Lgui,
    sdl::KeyCode::Rgui,
    sdl::KeyCode::Application,
    /*...*/
};

constexpr static inline std::array<sdl::MouseButton, 256> directInputMouseMap{
    sdl::MouseButton::Left,
    sdl::MouseButton::Right,
    sdl::MouseButton::Middle,
    sdl::MouseButton::Extra1,
    sdl::MouseButton::Extra2,
    /*...*/
};

class KeyMap {
 private:
  // SDL keycodes are not necessarily contiguous and can be arbitrarily large up
  // to UINT32_MAX, so cannot use an array.
  std::unordered_map<sdl::KeyCode, event::KeyVariant> mKeys{};
  std::unordered_map<sdl::MouseButton, event::KeyVariant> mMouse{};
  // TODO: Joysticks and controllers

 public:
  // Keycodes are given in the ini as four bytes in hexadecimal, AABBCCDD.
  //  - 0xAABB is the directInput keycode, but AA is ignored.
  //  - 0xCC is the mouse button.
  //  - 0xDD is the joystick button.
  // In all cases 0xFF represents null. Returns a pointer to the internally
  // added KeyEvent, if any, and nullptr otherwise.
  template<class T>
  [[maybe_unused]] T *attach(const std::string &keycodes) {
    if (keycodes.size() != 8) return nullptr;
    const int converted{std::stoi(keycodes, nullptr, 16)};
    if (converted < 0) return nullptr;
    auto bytes{static_cast<uint32_t>(converted)};

    auto key{static_cast<uint8_t>((bytes & 0x00ff'00'00) >> 16)};
    auto mouse{static_cast<uint8_t>((bytes & 0x0000'ff'00) >> 8)};
    //auto joystick{static_cast<uint8_t>(bytes & 0x0000'00'ff)};

    using namespace event;

    if (key != 0xff) {
      KeyVariant &var{mKeys[directInputKeyMap[key]] = KeyVariant{}};
      return &var.emplace<T>();
    }

    if (mouse != 0xff) {
      KeyVariant &var{mMouse[directInputMouseMap[mouse]] = KeyVariant{}};
      return &var.emplace<T>();
    }

    return nullptr;
  }

  // Convert an SDL KeyDown or KeyUp event into a KeyEvent using the internal
  // keymap, returning std::nullopt if the key is not bound to any KeyEvent.
  std::optional<event::KeyVariant> translateKey(const sdl::Event &event) const {
    const auto type{sdl::typeOf(event)};
    if (type == sdl::EventType::KeyDown || type == sdl::EventType::KeyUp) {
      if (event.key.repeat) return std::nullopt;
      auto it{mKeys.find(sdl::keyCodeOf(event.key))};
      if (it == mKeys.end()) return std::nullopt;
      event::KeyVariant var{it->second};
      std::visit([type](auto &e) {
        e.down = (type == sdl::EventType::KeyDown);
      }, var);
      return var;
    } else {
      return std::nullopt;
    }
  }

  explicit KeyMap(const GameSettings &settings) {
    // Convenience lambda for omitting redundant "Controls" and std::string
    auto getSetting = [&settings](const std::string &name,
                                  const char *defaultValue) -> std::string {
      return settings.get<std::string>("Controls." + name, defaultValue);
    };

    attach<event::Forward>(getSetting("Forward", "0011FFFF"));
    attach<event::Backward>(getSetting("Back", "001FFFFF"));
    attach<event::SlideLeft>(getSetting("Slide Left", "001EFFFF"));
    attach<event::SlideRight>(getSetting("Slide Right", "0020FFFF"));
    attach<event::Use>(getSetting("Use", "00FF00FF"));
    attach<event::Activate>(getSetting("Activate", "0039FFFF"));
    attach<event::Block>(getSetting("Block", "003801FF"));
    attach<event::Cast>(getSetting("Cast", "002EFFFF"));
    attach<event::ReadyItem>(getSetting("Ready Item", "0021FFFF"));
    attach<event::Sneak>(getSetting("Crouch/Sneak", "001DFFFF"));
    attach<event::Run>(getSetting("Run", "002AFFFF"));
    attach<event::AlwaysRun>(getSetting("Always Run", "003AFFFF"));
    attach<event::AutoMove>(getSetting("Auto Move", "0010FFFF"));
    attach<event::Jump>(getSetting("Jump", "0012FFFF"));
    attach<event::TogglePov>(getSetting("Toggle POV", "001302FF"));
    attach<event::MenuMode>(getSetting("Menu Mode", "000FFFFF"));
    attach<event::Rest>(getSetting("Rest", "0014FFFF"));
    attach<event::QuickMenu>(getSetting("Quick Menu", "003BFFFF"));
    attach<event::Quick>(getSetting("Quick", "0002FFFF"))->n = 1;
    attach<event::Quick>(getSetting("Quick", "0003FFFF"))->n = 2;
    attach<event::Quick>(getSetting("Quick", "0004FFFF"))->n = 3;
    attach<event::Quick>(getSetting("Quick", "0005FFFF"))->n = 4;
    attach<event::Quick>(getSetting("Quick", "0006FFFF"))->n = 5;
    attach<event::Quick>(getSetting("Quick", "0007FFFF"))->n = 6;
    attach<event::Quick>(getSetting("Quick", "0008FFFF"))->n = 7;
    attach<event::Quick>(getSetting("Quick", "0009FFFF"))->n = 8;
    attach<event::QuickSave>(getSetting("QuickSave", "003FFFFF"));
    attach<event::QuickLoad>(getSetting("QuickLoad", "0043FFFF"));
    attach<event::Grab>(getSetting("Grab", "002CFFFF"));
  }
};

#endif // OPENOBLIVION_CONTROLS_HPP
