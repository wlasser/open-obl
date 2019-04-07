#ifndef OPENOBLIVION_GUI_VERTICAL_SCROLL_HPP
#define OPENOBLIVION_GUI_VERTICAL_SCROLL_HPP

#include "gui/elements/image.hpp"
#include "gui/trait.hpp"
#include <array>

namespace gui {

class VerticalScroll : public Image {
 private:
  /// `user0`: Unused.
  float mUser0{};
  /// `user1`: Minimum value that can be scrolled to.
  /// Usually zero.
  float mScrollMinValue{};
  /// `user2`: Maximum value that can be scrolled to.
  /// Should be the number of items in the list minus the number of visible
  /// items, so that when this value is scrolled to the page is full.
  float mScrollMaxValue{};
  /// `user3`: Number of items to advance when the scroll *button* is clicked.
  float mScrollBtnStep{};
  /// `user4`: Number of items to advance when the scroll *bar* is clicked.
  float mScrollBarStep{};
  /// `user5`: Starting position of the scroll bar.
  float mScrollBarStart{};
  /// `user6`: `<id>` for the vertical scroll marker.
  float mScrollMarkerId{};
  /// `user7`: The current value of the scroll bar.
  float mScrollCurrentValue{};
  /// `user8`: Number of items visible at once.
  float mScrollNumVisibleItems{};
  /// `user9`: Number of items to advance when scrolled?
  float mScrollScrStep{};
  /// `user10`: Unknown.
  float mUser10{};

  UserTraitInterface<float, float, float, float, float,
                     float, float, float, float, float,
                     float> mInterface{std::make_tuple(
      &mUser0, &mScrollMinValue, &mScrollMaxValue, &mScrollBtnStep,
      &mScrollBarStep, &mScrollBarStart, &mScrollMarkerId, &mScrollCurrentValue,
      &mScrollNumVisibleItems, &mScrollScrStep, &mUser10)};

 public:
  explicit VerticalScroll(std::string name) : Image(std::move(name)) {}

  auto getUserOutputTraitInterface() const {
    float *const fnull{nullptr};
    return std::make_tuple(&mUser0, fnull, fnull, fnull, fnull, fnull, fnull,
                           &mScrollCurrentValue, fnull, fnull, &mUser10);
  }

  BUILD_USER_TRAIT_INTERFACE(mInterface);
};

class VerticalScrollMarker : public Image {
 private:
  /// `user0`--`user7` are all unused.
  std::array<float, 8> mUnused{};
  /// `user8`: Interval to change value by when scrolling.
  float mScrollInterval{};
  /// `user9`: Unknown
  float mUser9{};
  /// `user10`: Unknown
  float mUser10{};

  UserTraitInterface<float, float, float, float, float,
                     float, float, float, float, float,
                     float> mInterface{std::make_tuple(
      &mUnused[0], &mUnused[1], &mUnused[2], &mUnused[3], &mUnused[4],
      &mUnused[5], &mUnused[6], &mUnused[7], &mScrollInterval, &mUser9,
      &mUser10)};

 public:
  explicit VerticalScrollMarker(std::string name) : Image(std::move(name)) {}

  auto getUserOutputTraitInterface() const {
    float *const fnull{nullptr};
    return std::make_tuple(&mUnused[0], &mUnused[1], &mUnused[2], &mUnused[3],
                           &mUnused[4], &mUnused[5], &mUnused[6], &mUnused[7],
                           fnull, &mUser9, fnull);
  }

  BUILD_USER_TRAIT_INTERFACE(mInterface);
};

} // namespace gui

#endif // OPENOBLIVION_GUI_VERTICAL_SCROLL_HPP
