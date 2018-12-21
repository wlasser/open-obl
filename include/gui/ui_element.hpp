#ifndef OPENOBLIVION_GUI_UI_ELEMENT_HPP
#define OPENOBLIVION_GUI_UI_ELEMENT_HPP

#include "gui/trait.hpp"
#include <OgreOverlayElement.h>
#include <optional>
#include <string>
#include <variant>

namespace gui {

class UiElement {
 protected:
  std::string mName{};

 public:
  /// Position of left edge, relative to position of locus ancestor
  virtual void set_x(float x) {}
  /// Position of top edge, relative to position of locus ancestor
  virtual void set_y(float y) {}
  /// Width in pixels
  virtual void set_width(float width) {}
  /// Height in pixels
  virtual void set_height(float height) {}
  /// Transparency. 0 is completely transparent, 255 is completely opaque
  virtual void set_alpha(float alpha) {}
  /// If true, this element is used to anchor the position of its children
  virtual void set_locus(bool locus) {}
  /// If false, this element and all its descendants are hidden and un-clickable
  virtual void set_visible(bool visible) {}
  /// Time in seconds for fade-in or fade-out
  virtual void set_menufade(float menufade) {}
  /// This is probably distinct from menuFade, but we treat it as an alias
  virtual void set_explorefade(float explorefade) {
    set_menufade(explorefade);
  }
  /// Filename of texture or model to display
  virtual void set_filename(std::string filename) {}
  /// Percentage to scale the image or text by.
  /// A positive `zoom` factor applies a uniform scaling to the image, with
  /// `zoom` interpreted as the target percentage scaling. This is the only
  /// scaling applied, in the sense that if the width or height of the uiElement
  /// differs from that of the source image, after applying the zoom scaling,
  /// then the source image is clipped or tiled appropriately and not scaled
  /// further.
  ///
  /// A zoom factor of `-1` (or more generally any negative number)
  /// non-uniformly scales the source image to the width and height of the
  /// image; no clipping is performed.
  virtual void set_zoom(float zoom) {}

  /// Override this to specify the user trait interface of the ui element; the
  /// default should be that every user trait index is Unimplemented, with user
  /// traits being given implemented types in sequential order as needed.
  virtual TraitTypeId userTraitType(int index) const {
    return TraitTypeId::Unimplemented;
  }

  using UserValue = std::variant<int, float, bool, std::string>;

  /// Override this to set the user trait with the given index, doing nothing
  /// if the particular `{index, value}` combination is unimplemented or
  /// invalid.
  virtual void set_user(int index, UserValue value) {}

  /// Every UiElement is required to have a name which identifies it uniquely in
  /// the scope of the surrounding menu, or if the UiElement is a menu, then in
  /// the scope of the application.
  virtual std::string get_name() const {
    return mName;
  }

  virtual void set_name(std::string name) {
    mName = std::move(name);
  }

  /// Some UiElements have traits that must be generated internally, not
  /// supplied manually by the ui designer. For instance, a designer would not
  /// know the width of a text box, even if they knew the contents of the box.
  /// The UiElement is therefore allowed to supply its own implementation
  /// traits, called 'provided traits', which should have no dependencies and no
  /// setter.
  virtual std::optional<gui::Trait<float>> make_x() const {
    return {};
  }
  virtual std::optional<gui::Trait<float>> make_y() const {
    return {};
  }
  virtual std::optional<gui::Trait<float>> make_width() const {
    return {};
  }
  virtual std::optional<gui::Trait<float>> make_height() const {
    return {};
  }
  virtual std::optional<gui::Trait<float>> make_alpha() const {
    return {};
  }
  virtual std::optional<gui::Trait<bool>> make_locus() const {
    return {};
  }
  virtual std::optional<gui::Trait<bool>> make_visible() const {
    return {};
  }
  virtual std::optional<gui::Trait<float>> make_menufade() const {
    return {};
  }
  virtual std::optional<gui::Trait<float>> make_explorefade() const {
    return {};
  }
  virtual std::optional<gui::Trait<std::string>> make_filename() const {
    return {};
  }
  virtual std::optional<gui::Trait<float>> make_zoom() const {
    return {};
  }

  /// Get the renderable representation of this uiElement.
  virtual Ogre::OverlayElement *getOverlayElement() {
    return nullptr;
  }

  virtual ~UiElement() = 0;
};
inline UiElement::~UiElement() = default;

} // namespace gui

#endif // OPENOBLIVION_GUI_UI_ELEMENT_HPP
