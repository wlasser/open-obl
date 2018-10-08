#ifndef OPENOBLIVION_ENGINE_GUI_LOADING_MENU_HPP
#define OPENOBLIVION_ENGINE_GUI_LOADING_MENU_HPP

#include "engine/gui/menu.hpp"
#include "engine/gui/trait.hpp"
#include <string>

namespace engine::gui {

template<>
class Menu<MenuType::LoadingMenu> : public UiElement {
 private:
  std::string mName{};

  int mStepNumber{};
  std::string mLoadImage{};
  std::string mLoadText{};
  int mCurrentProgress{};
  int mMaximumProgress{};
  std::string mDebugText{};

  UserTraitInterface<int, std::string, std::string, int, int, std::string>
      mInterface{std::make_tuple(&mStepNumber,
                                 &mLoadImage,
                                 &mLoadText,
                                 &mCurrentProgress,
                                 &mMaximumProgress,
                                 &mDebugText)};

 public:
  BUILD_USER_TRAIT_INTERFACE(mInterface);
};

using LoadingMenu = Menu<MenuType::LoadingMenu>;

} // namespace engine::gui

#endif // OPENOBLIVION_ENGINE_GUI_LOADING_MENU_HPP
