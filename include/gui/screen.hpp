#ifndef OPENOBLIVION_GUI_SCREEN_HPP
#define OPENOBLIVION_GUI_SCREEN_HPP

#include "gui/trait.hpp"
#include "settings.hpp"
#include <OgreRenderTarget.h>
#include <OgreRoot.h>
#include <OgreVector.h>
#include <string_view>

namespace gui {

/// \see gui::ScreenElement::getNormalizedDimensions()
/// \todo Flip the dependency here.
Ogre::Vector2 getNormalizedDimensions();

/// `screen` is an implementation defined element describing screen dimensions
/// in normalized coordinates (NC). If `screenWidth / screenHeight >= 1` then
/// the height is normalized to `960px` and the width computed according to the
/// aspect ratio. Otherwise, the width is normalized to `1280px` and the height
/// is computed according to the aspect ratio. `screen` has the following
/// traits:
///  - `<width>`: the screen width in NC
///  - `<height>`: the screen height in NC
///  - `<cropx>`: the horizontal safe zone margin width in NC
///  - `<cropy>`: the vertical safe zone margin height in NC
class ScreenElement {
 private:
  int mRawWidth{0};
  int mRawHeight{0};

  struct Dimensions {
    int width;
    int height;
  };

  /// Get the draw area dimensions.
  /// If `screen width / screen height >= 1` then the height is normalized to
  /// `960px` and the width computed by the aspect ratio. Otherwise, the width
  /// is normalized to `1280px` and the height is computed with the aspect
  /// ratio.
  Dimensions getNormalizedDimensions() const;

  static constexpr inline std::string_view NAME{"__screen"};
  static constexpr inline std::string_view PREFIX{"__screen."};

 public:

  ScreenElement();

  Trait<float> makeWidthTrait() const {
    const auto[width, _] = getNormalizedDimensions();
    return Trait<float>(std::string{PREFIX} + "width", width);
  }

  Trait<float> makeHeightTrait() const {
    const auto[_, height] = getNormalizedDimensions();
    return Trait<float>(std::string{PREFIX} + "height", height);
  }

  Trait<float> makeCropXTrait() const {
    const auto[width, _] = getNormalizedDimensions();
    return Trait<float>(std::string{PREFIX} + "cropx", width * 0.15f);
  }

  Trait<float> makeCropYTrait() const {
    const auto[_, height] = getNormalizedDimensions();
    return Trait<float>(std::string{PREFIX} + "cropy", height * 0.15f);
  }

  /// Return the implementation-defined name of the `ScreenElement`.
  static constexpr std::string_view getName() {
    return NAME;
  }

  /// Return `getName()`, followed by a dot.
  static constexpr std::string_view getPrefix() {
    return PREFIX;
  }
};

} // namespace gui

#endif // OPENOBLIVION_GUI_SCREEN_HPP
