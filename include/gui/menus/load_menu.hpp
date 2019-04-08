#ifndef OPENOBLIVION_GUI_LOAD_MENU_HPP
#define OPENOBLIVION_GUI_LOAD_MENU_HPP

#include "gui/menu.hpp"

namespace gui {

template<>
class Menu<MenuType::LoadMenu> : public UiElement {
 private:
  /// `user0`: Dummy value, unused.
  float mUser0{};

  bool mVisible{true};

  /// Parent `Ogre::Overlay` of this menu.
  Ogre::Overlay *mOverlay{};

  /// Toplevel container for the `Ogre::OverlayElement`s.
  Ogre::OverlayContainer *mOverlayContainer{};

  UserTraitInterface<float> mInterface{std::make_tuple(&mUser0)};

 public:
  Menu<MenuType::LoadMenu>();
  ~Menu<MenuType::LoadMenu>() override;

  auto getUserOutputTraitInterface() const {
    return std::make_tuple<float *>(nullptr);
  }

  BUILD_USER_TRAIT_INTERFACE(mInterface);

  void set_visible(bool visible) override;

  std::optional<gui::Trait<float>> make_x() const override;
  std::optional<gui::Trait<float>> make_y() const override;

  Ogre::Overlay *getOverlay() const;
  Ogre::OverlayElement *getOverlayElement() const override;
};

using LoadMenu = Menu<MenuType::LoadMenu>;

} // namespace gui

#endif // OPENOBLIVION_GUI_LOAD_MENU_HPP