#ifndef OPENOBLIVION_GUI_BUTTON_HPP
#define OPENOBLIVION_GUI_BUTTON_HPP

#include "gui/elements/image.hpp"
#include "gui/trait.hpp"
#include <array>

namespace gui {

class Button : public Image {
 private:
  /// `user0`: Button text.
  std::string mButtonText{};
  /// `user1`: Whether the button is enabled.
  bool mButtonEnabled{};
  /// `user2`: Font.
  float mButtonFont{};
  /// `user3`: Whether this button is visible.
  bool mButtonVisible{};
  /// `user4`--`user19` are unused.
  std::array<float, 16> mUnused{};
  /// `user20`: Button style.
  float mButtonStyle{};
  /// `user21`: Red component of button text colour.
  float mButtonRed{};
  /// `user22`: Green component of button text colour.
  float mButtonGreen{};
  /// `user23`: Blue component of button text colour.
  float mButtonBlue{};

  UserTraitInterface<std::string, bool, float, bool,
                     float, float, float, float, float, float, float, float,
                     float, float, float, float, float, float, float, float,
                     float, float, float, float> mInterface{std::make_tuple(
      &mButtonText, &mButtonEnabled, &mButtonFont, &mButtonVisible,
      &mUnused[0], &mUnused[1], &mUnused[2], &mUnused[3], &mUnused[4],
      &mUnused[5], &mUnused[6], &mUnused[7], &mUnused[8], &mUnused[9],
      &mUnused[10], &mUnused[11], &mUnused[12], &mUnused[13], &mUnused[14],
      &mUnused[15], &mButtonStyle, &mButtonRed, &mButtonGreen, &mButtonBlue)};

 public:

  Button(std::string name) : Image(std::move(name)) {}

  auto getUserOutputTraitInterface() {
    return std::make_tuple(
        &mButtonText, &mButtonEnabled, &mButtonFont, &mButtonVisible,
        &mUnused[0], &mUnused[1], &mUnused[2], &mUnused[3], &mUnused[4],
        &mUnused[5], &mUnused[6], &mUnused[7], &mUnused[8], &mUnused[9],
        &mUnused[10], &mUnused[11], &mUnused[12], &mUnused[13], &mUnused[14],
        &mUnused[15], &mButtonStyle, &mButtonRed, &mButtonGreen, &mButtonBlue);
  }

  BUILD_USER_TRAIT_INTERFACE(mInterface);
};

} // namespace gui

#endif // OPENOBLIVION_GUI_BUTTON_HPP
