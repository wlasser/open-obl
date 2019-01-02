#ifndef OPENOBLIVION_GUI_IMAGE_HPP
#define OPENOBLIVION_GUI_IMAGE_HPP

#include "gui/screen.hpp"
#include "gui/ui_element.hpp"
#include <OgrePanelOverlayElement.h>
#include <OgreMaterial.h>

namespace gui {

class Image : public UiElement {
 private:
  Ogre::PanelOverlayElement *mOverlay{};
  Ogre::MaterialPtr mMatPtr{};

  /// Width of the source texture, in pixels
  float mTexWidth{1.0f};
  /// Height of the source texture, in pixels
  float mTexHeight{1.0f};
  /// Percentage zoom factor.
  float mZoom{100.0f};
  /// Whether this Image receives user input events
  bool mIsTarget{false};
  /// Id of this element for user input handling
  int mId{-1};

  /// Update the texture UVs according to the current dimensions and zoom.
  /// \param dims The normalized screen dimensions
  /// \pre `mOverlay != nullptr`
  void updateUVs(const Ogre::Vector2 &dims)
  /*C++20: [[expects: mOverlay != nullptr]]*/;

 public:
  Image(std::string name);

  ~Image();

  void set_x(float x) override;
  void set_y(float y) override;
  void set_width(float width) override;
  void set_height(float height) override;
  void set_filename(std::string filename) override;
  void set_zoom(float zoom) override;
  void set_visible(bool visible) override;
  void set_target(bool isTarget) override;
  void set_id(float id) override;

  std::optional<gui::Trait<float>> make_clicked() const override;
  std::optional<gui::Trait<float>> make_shiftclicked() const override;
  std::optional<gui::Trait<float>> make_mouseover() const override;

  Ogre::OverlayElement *getOverlayElement() override {
    return mOverlay;
  }
};

} // namespace gui

#endif // OPENOBLIVION_GUI_IMAGE_HPP
