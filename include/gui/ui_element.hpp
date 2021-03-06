#ifndef OPENOBL_GUI_UI_ELEMENT_HPP
#define OPENOBL_GUI_UI_ELEMENT_HPP

#include "gui/trait.hpp"
#include <OgreOverlayElement.h>
#include <optional>
#include <string>
#include <variant>

namespace gui {

class UiElement {
 private:
  int mChildCount{};

 protected:
  std::string mName{};

 public:
  /// Notify this element about the number of child uiElements it has.
  void setChildCount(int childCount) {
    mChildCount = childCount;
  }

  /// Get the number of children uiElements that this element has.
  int getChildCount() {
    return mChildCount;
  }

  /// \name Trait Setter Functions
  /// These are used to set observable properties of the concrete
  /// representative, usually using values from the dynamic representative.
  ///@{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

  /// Position of left edge, relative to position of locus ancestor
  virtual void set_x(float x) {}

  /// Position of top edge, relative to position of locus ancestor
  virtual void set_y(float y) {}

  /// Width in pixels
  virtual void set_width(float width) {}

  /// Height in pixels
  virtual void set_height(float height) {}

  /// Z-Order of this element relative to its parent.
  /// Elements with higher depth will be placed on top of elements with lower
  /// depth.
  virtual void set_depth(float depth) {}

  /// Transparency. 0 is completely transparent, 255 is completely opaque
  virtual void set_alpha(float alpha) {}

  /// Red component of text, from 0 to 255
  virtual void set_red(float red) {}

  /// Green component of text, from 0 to 255
  virtual void set_green(float green) {}

  /// Blue component of text, from 0 to 255
  virtual void set_blue(float blue) {}

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

  /// If true, this uiElement will receive mouse and keyboard focus.
  virtual void set_target(bool isTarget) {}

  /// Type of this element with regards to user input, analogous to the
  /// `<class>` of a Menu.
  /// The engine does not care about the names of uiElements, instead relevant
  /// uiElements are given a numerical `id`. Each menu has a set of valid `id`s,
  /// each with some menu-specific meaning, that can be given to the uiElements
  /// making up the menu to identify them to the engine. For example, the `id`
  /// of `3` may be given to a `"back"` button. For every valid `id` in the menu
  /// there should be exactly one uiElement with an id trait whose value is that
  /// `id`.
  virtual void set_id(float id) {}

  /// Sound to play when this uiElement is clicked.
  /// Should only have an effect if this uiElement's `target` trait is true and
  /// this uiElement has an `id` trait.
  virtual void set_clicksound(float clicksound) {}
  ///@}

  /// Text to display.
  virtual void set_string(std::string string) {}

  /// Font of the text to display.
  virtual void set_font(float font) {}

  /// Whether the text is left, right, or centre justified.
  virtual void set_justify(float justify) {}

  /// Override this to specify the user trait interface of the ui element; the
  /// default should be that every user trait index is Unimplemented, with user
  /// traits being given implemented types in sequential order as needed.
  virtual TraitTypeId userTraitType(int index) const {
    return TraitTypeId::Unimplemented;
  }

  using UserValue = std::variant<float, bool, std::string>;

  /// Override this to set the user trait with the given index, doing nothing
  /// if the particular `{index, value}` combination is unimplemented or
  /// invalid.
  virtual void set_user(int index, UserValue value) {}

  /// Override this to get the value of the user trait with the given index,
  /// throwing or returning a default if the `index` trait is unimplemented.
  virtual UserValue get_user(int index) {
    throw std::runtime_error("No user trait interface defined");
  }

  using TraitVariant = std::variant<gui::Trait<float>,
                                    gui::Trait<std::string>,
                                    gui::Trait<bool>>;

  virtual void
  setOutputUserTraitSources(std::vector<TraitVariant *> traits) const {}

#pragma GCC diagnostic pop

  /// Every UiElement is required to have a name which identifies it uniquely in
  /// the scope of the surrounding menu, or if the UiElement is a menu, then in
  /// the scope of the application.
  virtual std::string get_name() const {
    return mName;
  }

  virtual void set_name(std::string name) {
    mName = std::move(name);
  }

  /// \name Provided Traits
  /// Some UiElements have traits that must be generated internally, not
  /// supplied manually by the ui designer. For instance, a designer would not
  /// know the width of a text box, even if they knew the contents of the box.
  /// The UiElement is therefore allowed to supply its own implementation
  /// traits, called 'provided traits', which should have no dependencies and no
  /// setter.
  /// Preconditions in the documentation of these functions denote requirements
  /// for the trait to be added to the uiElement.
  /// @{
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
  /// Width of an image file, in pixels.
  virtual std::optional<gui::Trait<float>> make_filewidth() const {
    return {};
  }
  /// Height of an image file, in pixels.
  virtual std::optional<gui::Trait<float>> make_fileheight() const {
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
  /// Takes the value `1` when the concrete representative is clicked, then
  /// resets to `0` during the same frame.
  /// \pre The uiElement's `target` trait must be true
  /// \pre The uiElement must have a valid `id` trait.
  virtual std::optional<gui::Trait<float>> make_clicked() const {
    return {};
  }
  /// Takes the value `1` when the concrete representative is clicked while
  /// holding shift, then resets to `0` during the same frame.
  /// \pre The uiElement's `target` trait must be true
  /// \pre The uiElement must have a valid `id` trait.
  virtual std::optional<gui::Trait<float>> make_shiftclicked() const {
    return {};
  }
  /// Takes the value `1` when the mouse cursor is moved over the concrete
  /// representative, and `0` otherwise.
  /// \pre The uiElement's `target` trait must be true
  /// \pre The uiElement must have a valid `id` trait.
  virtual std::optional<gui::Trait<float>> make_mouseover() const {
    return {};
  }
  /// The number of child uiElements that this element has.
  virtual std::optional<gui::Trait<float>> make_childcount() const {
    return gui::Trait<float>(get_name() + ".childcount", mChildCount);
  }
  /// Alternative spelling of childcount.
  virtual std::optional<gui::Trait<float>> make_child_count() const {
    return gui::Trait<float>(get_name() + ".child_count", mChildCount);
  }
  ///@}

  /// Get the renderable representation of this uiElement.
  virtual Ogre::OverlayElement *getOverlayElement() const {
    return nullptr;
  }

  /// Interaction setters
  /// These functions are called when a user event occurs on this element. They
  /// are effectively setters, but are not intended to be called by trait
  /// functions and so are named differently.
  ///@{

  /// Called when this element is clicked.
  virtual void notify_clicked() {}

  /// Called when this element is shift-clicked.
  virtual void notify_shiftclicked() {}

  /// Called when the mouse cursor is over this element.
  virtual void notify_mouseover() {}

  /// Called at the end of a frame to reset this element's event state.
  virtual void clearEvents() {}
  ///@}

  /// Interaction getters
  /// These functions return whether this element has received a user event this
  /// frame. These are intended to be called by this element's traits of the
  /// corresponding name. For instance, the trait constructed by
  /// `make_clicked()` should use `is_clicked()` to determine its value.
  /// @{

  /// Whether this element has been clicked this frame.
  virtual bool is_clicked() const {
    return false;
  }

  /// Whether this element has been shift-clicked this frame.
  virtual bool is_shiftclicked() const {
    return false;
  }

  /// Whether the mouse cursor is over this element during this frame.
  virtual bool is_mouseover() const {
    return false;
  }

  /// \see set_id(float)
  virtual int get_id() const {
    return -2;
  }

  /// \see set_clicksound(float)
  virtual int get_clicksound() const {
    return 0;
  }
  ///@}

  virtual ~UiElement() = 0;

  UiElement() = default;
  UiElement(const UiElement &) = delete;
  UiElement &operator=(const UiElement &) = delete;
  UiElement(UiElement &&) = delete;
  UiElement &operator=(UiElement &&) = delete;
};
inline UiElement::~UiElement() = default;

} // namespace gui

#endif // OPENOBL_GUI_UI_ELEMENT_HPP
