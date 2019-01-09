#ifndef OPENOBLIVION_CONSOLE_MODE_HPP
#define OPENOBLIVION_CONSOLE_MODE_HPP

#include "application_context.hpp"
#include "game_settings.hpp"
#include "modes/mode.hpp"
#include <gsl/gsl>
#include <imgui/imgui.h>
#include <string>
#include <string_view>
#include <vector>

namespace oo {

class ConsoleMode {
 private:
  int handleInputCompletion(gsl::not_null<ImGuiInputTextCallbackData *> data);
  int handleInputHistory(gsl::not_null<ImGuiInputTextCallbackData *> data);

  int textEditCallback(gsl::not_null<ImGuiInputTextCallbackData *> data);

  void windowPreInit();
  void displayHistory();
  void displayPrompt();

  oo::ConsoleEngine &consoleEngine;

  std::string executeCommand(const std::string &cmd);

  std::string mPrompt{"$ "};
  ImVec4 mPromptColor{ImColor{0.0f, 1.0f, 0.0f, 1.0f}};

  constexpr static inline std::size_t BufferSize{256};
  std::array<char, BufferSize> mBuffer{};
  std::vector<std::string> mHistory{};
  bool mNeedToScrollHistoryToBottom{false};

 public:
  using transition_t = ModeTransition<ConsoleMode>;

  explicit ConsoleMode(ApplicationContext &ctx)
      : consoleEngine(ctx.getConsoleEngine()) {}

  void enter(ApplicationContext &ctx) {
    refocus(ctx);
  }

  void refocus(ApplicationContext &) {
    sdl::setRelativeMouseMode(false);
  }

  /// Pops this state if event::Console is pressed.
  /// All other events are already forwarded to ImGui by Application so do not
  /// need to be handled again.
  transition_t handleEvent(ApplicationContext &ctx, const sdl::Event &event);

  /// Display and update the developer console.
  void update(ApplicationContext &ctx, float);
};

} // namespace oo

#endif // OPENOBLIVION_CONSOLE_MODE_HPP
