#ifndef OPENOBLIVION_ENGINE_GUI_UI_ELEMENT_HPP
#define OPENOBLIVION_ENGINE_GUI_UI_ELEMENT_HPP

#include "engine/gui/trait.hpp"
#include <string>

namespace engine::gui {

class UiElement {
 protected:
  std::string mName{};

 public:
  // Position of left edge, relative to position of locus ancestor
  virtual void set_x(int x) {}
  // Position of top edge, relative to position of locus ancestor
  virtual void set_y(int y) {}
  // Width in pixels
  virtual void set_width(int width) {}
  // Height in pixels
  virtual void set_height(int height) {}
  // Transparency. 0 is completely transparent, 255 is completely opaque
  virtual void set_alpha(int alpha) {}
  // If true, this element is used to anchor the position of its children
  virtual void set_locus(bool locus) {}
  // If false, this element and all its descendants are hidden and un-clickable
  virtual void set_visible(bool visible) {}
  // Time in seconds for fade-in or fade-out
  virtual void set_menufade(float menufade) {}
  // This is probably distinct from menuFade, but we treat it as an alias
  virtual void set_explorefade(float explorefade) {
    set_menufade(explorefade);
  }

  // Override this to specify the user trait interface of the ui element; the
  // default should be that every user trait index is Unimplemented, with user
  // traits being given implemented types in sequential order as needed.
  virtual TraitTypeId userTraitType(int index) const {
    return TraitTypeId::Unimplemented;
  }

  // Override these to set the user trait with the given index, doing nothing if
  // the particular {index, value} combination is not implemented or invalid.
  // I am not happy with this method of specifying the user trait interface as
  // it artificially groups traits into types, and requires that every index be
  // considered multiple times. If the ui element has n user traits then ideally
  // one would write n functions which took only the type they wanted. Doing
  // this with function templates prevents the functions from being virtual and
  // causes problems with detecting the defined functions. Doing this with
  // runtime polymorphism means picking some upper bound N on the number of
  // user traits and defining 4N functions (one for each type), then overloading
  // only the desired ones in the base. Moreover, translating the runtime index
  // into a function would require a big switch. Maybe this is a good idea, but
  // N needs to be at least 26 and writing 104 functions and a 26 case switch
  // didn't seem worth the effort.
  virtual void set_user(int index, int value) {}
  virtual void set_user(int index, float value) {}
  virtual void set_user(int index, bool value) {}
  virtual void set_user(int index, std::string value) {}

  // Every UiElement is required to have a name which identifies it uniquely in
  // the scope of the surrounding menu, or if the UiElement is a menu, then in
  // the scope of the application.
  virtual std::string get_name() const {
    return mName;
  }

  virtual void set_name(std::string name) {
    mName = name;
  }

  virtual ~UiElement() = 0;
};
inline UiElement::~UiElement() = default;

} // namespace engine::gui

#endif // OPENOBLIVION_ENGINE_GUI_UI_ELEMENT_HPP
