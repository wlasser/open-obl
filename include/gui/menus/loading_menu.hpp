#ifndef OPENOBLIVION_GUI_LOADING_MENU_HPP
#define OPENOBLIVION_GUI_LOADING_MENU_HPP

#include "gui/menu.hpp"
#include "gui/trait.hpp"
#include <string>

namespace gui {

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
  struct UserInterface {
    using type = decltype(mInterface)::interface_t;
    type value{};
  };
  BUILD_USER_TRAIT_INTERFACE(mInterface);
};

using LoadingMenu = Menu<MenuType::LoadingMenu>;

} // namespace gui

#endif // OPENOBLIVION_GUI_LOADING_MENU_HPP
