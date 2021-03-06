#include "controls.hpp"
#include "modes/console_mode.hpp"
#include "scripting/console_engine.hpp"
#include "util/settings.hpp"
#include <spdlog/spdlog.h>

namespace oo {

std::vector<std::string> &ConsoleMode::getHistory() {
  static std::vector<std::string> history{};
  return history;
}

void ConsoleMode::print(std::string msg) {
  ConsoleMode::getHistory().emplace_back(std::move(msg));
}

int ConsoleMode::textEditCallback(gsl::not_null<ImGuiInputTextCallbackData *> data) {
  switch (data->EventFlag) {
    case ImGuiInputTextFlags_CallbackCompletion: {
      return handleInputCompletion(data);
    }
    case ImGuiInputTextFlags_CallbackHistory: {
      return handleInputHistory(data);
    }
    default: {
      return 0;
    }
  }
}

std::string ConsoleMode::executeCommand(const std::string &cmd) {
  spdlog::get(oo::LOG)->info("Console: {}", cmd);
  consoleEngine->execute(cmd);
  return cmd;
}

void ConsoleMode::windowPreInit() {
  const auto &gameSettings{GameSettings::getSingleton()};
  const int width{gameSettings.iGet("Display.iSize W")};
  const int height{gameSettings.iGet("Display.iSize H")};

  // Full width window halfway down screen
  ImGui::SetNextWindowSize({
                               gsl::narrow_cast<float>(width),
                               gsl::narrow_cast<float>(height) / 2.0f
                           });
  ImGui::SetNextWindowPos({0.0f, gsl::narrow_cast<float>(height) / 2.0f});

  ImGui::SetNextWindowBgAlpha(0.5f);
}

void ConsoleMode::displayHistory() {
  // Leave room for a separator and line of text
  const float footerHeight{ImGui::GetStyle().ItemSpacing.y
                               + ImGui::GetFrameHeightWithSpacing()};
  ImGui::BeginChild("ScrollingRegion",
                    {0.0f, -footerHeight},
                    false, // no border
                    ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
  for (const auto &s : ConsoleMode::getHistory()) {
    ImGui::Text("%s", s.c_str());
  }
  if (mNeedToScrollHistoryToBottom) {
    mNeedToScrollHistoryToBottom = false;
    ImGui::SetScrollHereY(1.0f);
  }
  ImGui::EndChild();
  ImGui::Separator();
}

void ConsoleMode::displayPrompt() {
  static bool hasDoneInitialFocus{false};
  const auto inputFlags{ImGuiInputTextFlags_EnterReturnsTrue
                            | ImGuiInputTextFlags_CallbackHistory
                            | ImGuiInputTextFlags_CallbackCompletion};
  auto inputCallback = [](auto *data) {
    auto *self{static_cast<ConsoleMode *>(data->UserData)};
    return self->textEditCallback(gsl::make_not_null(data));
  };

  ImGui::TextColored(mPromptColor, "%s", mPrompt.c_str());
  ImGui::SameLine(0, 0);

  bool refocusInput{false};
  if (ImGui::InputText("##consoleInput",
                       mBuffer.data(),
                       mBuffer.size(),
                       inputFlags,
                       inputCallback,
                       static_cast<void *>(this))) {
    std::string buffer(mBuffer.data());
    auto &history{ConsoleMode::getHistory()};
    try {
      auto output{executeCommand(buffer)};
      history.push_back(mPrompt + buffer);
      history.push_back(output);
    } catch (const std::exception &e) {
      history.push_back(mPrompt + buffer);
      history.emplace_back(e.what());
    }
    mBuffer[0] = '\0';
    refocusInput = true;
    mNeedToScrollHistoryToBottom = true;
  }
  // Focus on the input text by default
  ImGui::SetItemDefaultFocus();
  if (!hasDoneInitialFocus) {
    ImGui::SetKeyboardFocusHere();
    hasDoneInitialFocus = true;
  }
  if (refocusInput) ImGui::SetKeyboardFocusHere(-1);
}

void ConsoleMode::update(ApplicationContext &/*ctx*/, float /*elapsed*/) {
  windowPreInit();
  if (ImGui::Begin("Console")) {
    displayHistory();
    displayPrompt();
  }
  ImGui::End();
}

ConsoleMode::transition_t
ConsoleMode::handleEvent(ApplicationContext &ctx, const sdl::Event &event) {
  // Pop if a event::Console is down
  if (auto keyEvent{ctx.getKeyMap().translateKey(event)}; keyEvent) {
    if (auto *conEvent{std::get_if<oo::event::Console>(&*keyEvent)}; conEvent) {
      return {conEvent->down, std::nullopt};
    }
  }
  return {false, std::nullopt};
}

int ConsoleMode::handleInputCompletion(gsl::not_null<ImGuiInputTextCallbackData *> /*data*/) {
  return 0;
}

int ConsoleMode::handleInputHistory(gsl::not_null<ImGuiInputTextCallbackData *> /*data*/) {
  return 0;
}

} // namespace oo
