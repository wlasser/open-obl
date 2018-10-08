#ifndef OPENOBLIVION_ENGINE_GUI_SCREEN_HPP
#define OPENOBLIVION_ENGINE_GUI_SCREEN_HPP

#include "engine/gui/trait.hpp"
#include "game_settings.hpp"

namespace engine::gui {

class ScreenElement {
 private:
  GameSetting<int> mRawWidth{"Display.iSize W", 0};
  GameSetting<int> mRawHeight{"Display.iSize H", 0};

  struct Dimensions {
    int width;
    int height;
  };

  // If screen width / screen height >= 1 then the height is normalized to 960px
  // and the width computed by the aspect ratio. Otherwise, the width is
  // normalized to 1280px and the height is computed with the aspect ratio.
  Dimensions getNormalizedDimensions() const {
    const int rawW{mRawWidth.get()};
    const int rawH{mRawHeight.get()};
    const int width{rawW >= rawH ? ((960 * rawW) / rawH) : 1280};
    const int height{rawW < rawH ? ((1280 * rawH) / rawW) : 960};
    return {width, height};
  }

 public:

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

}

#endif // OPENOBLIVION_ENGINE_GUI_SCREEN_HPP
