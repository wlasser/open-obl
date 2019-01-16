#ifndef OPENOBLIVION_GUI_TEXT_HPP
#define OPENOBLIVION_GUI_TEXT_HPP

#include "gui/elements/interactable_mixin.hpp"
#include "gui/trait.hpp"
#include "gui/ui_element.hpp"
#include <OgreMaterial.h>
#include <OgreTextAreaOverlayElement.h>
#include <optional>
#include <string>

namespace gui {

class Text : public InteractableMixin {
 private:
  Ogre::TextAreaOverlayElement *mOverlay{};
  Ogre::MaterialPtr mMatPtr{};

  Ogre::MaterialPtr createOrRetrieveMaterial() const;

  void updateFont(std::string fontName);

 public:
  Text(std::string name);

  ~Text();

  void set_alpha(float alpha) override;
  void set_red(float red) override;
  void set_green(float green) override;
  void set_blue(float blue) override;
  void set_x(float x) override;
  void set_y(float y) override;
  void set_depth(float depth) override;
  void set_visible(bool visible) override;
  void set_string(std::string str) override;
  void set_font(float font) override;
  void set_justify(float justify) override;

  std::optional<gui::Trait<float>> make_width() const override;
  std::optional<gui::Trait<float>> make_height() const override;

  Ogre::OverlayElement *getOverlayElement() const override;
};

} // namespace gui

#endif // OPENOBLIVION_GUI_TEXT_HPP
