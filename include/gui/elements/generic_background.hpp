#ifndef OPENOBLIVION_GUI_GENERIC_BACKGROUND_HPP
#define OPENOBLIVION_GUI_GENERIC_BACKGROUND_HPP

#include "gui/elements/rect.hpp"
#include "gui/trait.hpp"

namespace gui {

class GenericBackground : public Rect {
 private:
  /// `user0`: Width of the menu.
  float mWidth{};
  /// `user1`: Height of the menu.
  float mHeight{};

  UserTraitInterface<float, float>
      mInterface{std::make_tuple(&mWidth, &mHeight)};

 public:
  GenericBackground(std::string name) : Rect(std::move(name)) {}

  auto getUserOutputTraitInterface() {
    return std::tuple<float *, float *>{nullptr, nullptr};
  }

  BUILD_USER_TRAIT_INTERFACE(mInterface);
};

} // namespace gui

#endif // OPENOBLIVION_GUI_GENERIC_BACKGROUND_HPP
