#ifndef OPENOBL_GUI_IMAGE_HPP
#define OPENOBL_GUI_IMAGE_HPP

#include "gui/elements/interactable_mixin.hpp"
#include "gui/elements/panel_mixin.hpp"
#include "gui/screen.hpp"
#include "gui/ui_element.hpp"
#include <OgrePanelOverlayElement.h>
#include <OgreMaterial.h>

namespace gui {

class Image : public InteractableMixin, public PanelMixin {
 private:
  Ogre::MaterialPtr mMatPtr{};

  /// Width of the source texture, in pixels
  float mTexWidth{1.0f};
  /// Height of the source texture, in pixels
  float mTexHeight{1.0f};
  /// Percentage zoom factor.
  float mZoom{100.0f};

  /// Update the texture UVs according to the current dimensions and zoom.
  /// \param dims The normalized screen dimensions
  /// \pre `mOverlay != nullptr`
  void updateUVs(const Ogre::Vector2 &dims)
  /*C++20: [[expects: mOverlay != nullptr]]*/;

 public:
  explicit Image(std::string name);

  void set_width(float width) override;
  void set_height(float height) override;
  void set_alpha(float alpha) override;
  void set_filename(std::string filename) override;
  void set_zoom(float zoom) override;

  std::optional<gui::Trait<float>> make_filewidth() const override;
  std::optional<gui::Trait<float>> make_fileheight() const override;
};

} // namespace gui

#endif // OPENOBL_GUI_IMAGE_HPP
