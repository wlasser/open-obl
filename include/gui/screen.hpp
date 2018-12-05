#ifndef OPENOBLIVION_GUI_SCREEN_HPP
#define OPENOBLIVION_GUI_SCREEN_HPP

#include "gui/trait.hpp"
#include "settings.hpp"
#include <OgreRoot.h>
#include <OgreRenderTarget.h>

namespace gui {

/// `screen` is an implementation defined element describing screen dimensions
/// in normalized coordinates (NC). If `screenWidth / screenHeight >= 1` then
/// the height is normalized to `960px` and the width computed according to the
/// aspect ratio. Otherwise, the width is normalized to `1280px` and the height
/// is computed according to the aspect ratio. `screen` has the following
/// traits:
///  - `<width>`: the screen width in NC
///  - `<height>`: the screen height in NC
///  - `<cropX>`: the horizontal safe zone margin width in NC
///  - `<cropY>`: the vertical safe zone margin height in NC
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
  Dimensions getNormalizedDimensions() const {
    const int rawW{mRawWidth};
    const int rawH{mRawHeight};
    const int width{rawW >= rawH ? ((960 * rawW) / rawH) : 1280};
    const int height{rawW < rawH ? ((1280 * rawH) / rawW) : 960};
    return {width, height};
  }

 public:

  ScreenElement() {
    auto *root{Ogre::Root::getSingletonPtr()};
    if (!root) return;
    auto *target{root->getRenderTarget(oo::RENDER_TARGET)};
    if (!target) return;
    mRawWidth = static_cast<int>(target->getWidth());
    mRawHeight = static_cast<int>(target->getHeight());
  }

  Trait<int> makeWidthTrait() const {
    const auto[width, _] = getNormalizedDimensions();
    return Trait<int>("__screen.width", width);
  }

  Trait<int> makeHeightTrait() const {
    const auto[_, height] = getNormalizedDimensions();
    return Trait<int>("__screen.height", height);
  }

  // TODO: What are the actual screen crop values?
  Trait<int> makeCropXTrait() const {
    return Trait<int>("__screen.cropX", 32);
  }

  Trait<int> makeCropYTrait() const {
    return Trait<int>("__screen.cropY", 32);
  }
};

} // namespace gui

#endif // OPENOBLIVION_GUI_SCREEN_HPP
